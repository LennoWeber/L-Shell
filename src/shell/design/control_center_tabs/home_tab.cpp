#include "shell/control_center/tabs/home_tab.h"

#include "compositors/compositor_platform.h"
#include "config/config_service.h"
#include "core/deferred_call.h"
#include "core/input/keybind_matcher.h"
#include "cursor-shape-v1-client-protocol.h"
#include "dbus/bluetooth/bluetooth_service.h"
#include "dbus/mpris/mpris_art.h"
#include "dbus/mpris/mpris_service.h"
#include "dbus/network/inetwork_service.h"
#include "dbus/network/network_display.h"
#include "i18n/i18n.h"
#include "idle/idle_inhibitor.h"
#include "notification/notification_manager.h"
#include "pipewire/pipewire_service.h"
#include "render/core/renderer.h"
#include "render/scene/input_area.h"
#include "scripting/plugin_id.h"
#include "shell/control_center/shortcut_registry.h"
#include "shell/panel/panel_manager.h"
#include "shell/surface/material.h"
#include "system/brightness_service.h"
#include "system/gamma_service.h"
#include "ui/builders.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <wayland-client-protocol.h>

using namespace control_center;

namespace {

  // Tahoe module grid, in logical px before content scale.
  constexpr std::size_t kGridColumns = 4;
  constexpr float kGridGap = 10.0F;
  constexpr float kCellHeight = 72.0F;
  constexpr float kTilePadding = 12.0F;
  constexpr float kRowCircleSize = 28.0F;
  constexpr float kSmallCircleSize = 32.0F;
  constexpr float kSliderTileHeight = 64.0F;
  constexpr float kMediaTileHeight = 64.0F;
  constexpr float kMediaArtSize = 40.0F;
  constexpr float kMediaButtonSize = 28.0F;
  constexpr float kMuteButtonSize = 24.0F;
  constexpr float kShortcutCircleSize = 40.0F;
  constexpr float kShortcutLabelHeight = 16.0F;
  constexpr float kShortcutRowGap = 8.0F;
  constexpr std::size_t kMaxConfiguredShortcuts = 6;
  // control_center.show_home_title: the heading's line box over the grid.
  constexpr float kHomeTitleHeight = 28.0F;

  constexpr auto kSliderCommitInterval = std::chrono::milliseconds(16);
  constexpr auto kSliderStateHoldoff = std::chrono::milliseconds(180);
  constexpr float kSliderSyncEpsilon = 0.005F;

  constexpr std::string_view kSettingsShortcutId = "settings";
  constexpr std::string_view kSessionShortcutId = "session";

  // Configured shortcut types a module tile already covers; the bottom row skips them.
  constexpr std::array<std::string_view, 7> kModuleShortcutTypes{
      "wifi", "bluetooth", "notification", "caffeine", "nightlight", "audio", "media",
  };

  bool coveredByModule(std::string_view type) { return std::ranges::contains(kModuleShortcutTypes, type); }

  // Settings used to be a header button; with no header on the home view it joins the shortcut row.
  class SettingsShortcut final : public Shortcut {
  public:
    std::string_view id() const override { return kSettingsShortcutId; }
    std::string defaultLabel() const override { return i18n::tr("control-center.shortcuts.settings"); }
    std::string_view iconOn() const override { return "settings"; }
    std::string_view iconOff() const override { return "settings"; }
    void onClick() override { PanelManager::instance().openSettingsWindow(); }
  };

  bool showShortcutLabels(const ConfigService* config) {
    return config == nullptr || config->config().controlCenter.showShortcutLabels;
  }

  float shortcutCellHeight(bool labels) {
    return kShortcutCircleSize + (labels ? Style::spaceXs + kShortcutLabelHeight : 0.0F);
  }

  // Slate well for a module tile: a light step over a dark panel, the palette's variant surface on a light one
  // (white for Slate, a deeper paper for Atelier). Soft and glass panels lower the card opacity, which thins the
  // well so the blur shows through.
  ColorSpec tileFill(float cardOpacity, bool light) {
    const float strength = 0.5F + 0.5F * std::clamp(cardOpacity, 0.0F, 1.0F);
    if (light) {
      return colorSpecFromRole(ColorRole::SurfaceVariant, 0.72F * strength);
    }
    return colorSpecFromRole(ColorRole::OnSurface, 0.10F * strength);
  }

  // Round toggle colors: the accent only marks the on state; off is a neutral grey disc.
  Button::ButtonPalette togglePalette(bool on) {
    const auto state = [](ColorSpec bg, ColorSpec glyph) {
      return Button::ButtonStateColors{.bg = bg, .border = clearColorSpec(), .label = glyph};
    };
    if (on) {
      const ColorSpec glyph = colorSpecFromRole(ColorRole::OnPrimary);
      return Button::ButtonPalette{
          .borderWidth = 0.0F,
          .normal = state(colorSpecFromRole(ColorRole::Primary), glyph),
          .hover = state(colorSpecFromRole(ColorRole::Primary, 0.85F), glyph),
          .pressed = state(colorSpecFromRole(ColorRole::Primary, 0.7F), glyph),
          .disabled = state(colorSpecFromRole(ColorRole::Primary, 0.4F), colorSpecFromRole(ColorRole::OnPrimary, 0.7F)),
          .selected = std::nullopt,
      };
    }
    const ColorSpec glyph = colorSpecFromRole(ColorRole::OnSurface);
    return Button::ButtonPalette{
        .borderWidth = 0.0F,
        .normal = state(colorSpecFromRole(ColorRole::OnSurface, 0.10F), glyph),
        .hover = state(colorSpecFromRole(ColorRole::OnSurface, 0.16F), glyph),
        .pressed = state(colorSpecFromRole(ColorRole::OnSurface, 0.22F), glyph),
        .disabled =
            state(colorSpecFromRole(ColorRole::OnSurface, 0.05F), colorSpecFromRole(ColorRole::OnSurface, 0.35F)),
        .selected = std::nullopt,
    };
  }

  // Pushes a toggle's state to its round button. Returns true when the state changed.
  bool applyToggleState(Button* button, bool on, bool enabled, std::int8_t& applied) {
    if (button == nullptr) {
      return false;
    }
    const auto state = static_cast<std::int8_t>((on ? 1 : 0) | (enabled ? 2 : 0));
    if (state == applied) {
      return false;
    }
    applied = state;
    button->setCustomPalette(togglePalette(on));
    button->setEnabled(enabled);
    return true;
  }

  std::unique_ptr<Button> roundButton(
      Button** out, std::string_view glyph, float diameter, float glyphSize, std::function<void()> onClick,
      std::function<void()> onRightClick = nullptr
  ) {
    return ui::button({
        .out = out,
        .glyph = std::string(glyph),
        .glyphSize = glyphSize,
        .minWidth = diameter,
        .minHeight = diameter,
        .maxWidth = diameter,
        .maxHeight = diameter,
        .padding = 0.0F,
        .radius = Style::pillRadius(diameter),
        .onClick = std::move(onClick),
        .onRightClick = std::move(onRightClick),
    });
  }

  std::unique_ptr<Button>
  ghostButton(Button** out, std::string_view glyph, float size, float glyphSize, std::function<void()> onClick) {
    return ui::button({
        .out = out,
        .glyph = std::string(glyph),
        .glyphSize = glyphSize,
        .variant = ButtonVariant::Ghost,
        .minWidth = size,
        .minHeight = size,
        .maxWidth = size,
        .maxHeight = size,
        .padding = 0.0F,
        .radius = Style::pillRadius(size),
        .onClick = std::move(onClick),
    });
  }

  // Non-interactive content drops out of hit testing so a click on it reaches the tile overlay behind.
  void passHitsThrough(Node& node) { node.setHitTestVisible(false); }

  std::unique_ptr<Label> titleLabel(Label** out, std::string text, float scale) {
    return ui::label({
        .out = out,
        .text = std::move(text),
        .fontSize = Style::fontSizeBody * scale,
        .fontWeight = FontWeight::SemiBold,
        .color = colorSpecFromRole(ColorRole::OnSurface),
        .maxLines = 1,
        .ellipsize = TextEllipsize::End,
    });
  }

  std::unique_ptr<Label> secondaryLabel(Label** out, std::string text, float scale, FontWeight weight) {
    return ui::label({
        .out = out,
        .text = std::move(text),
        .fontSize = Style::fontSizeMini * scale,
        .fontWeight = weight,
        .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
        .maxLines = 1,
        .ellipsize = TextEllipsize::End,
    });
  }

  // A module caption ("Display", "Sound"); clicks on it fall through to the tile, which opens the detail tab.
  std::unique_ptr<Label> captionLabel(std::string text, float scale) {
    auto label = secondaryLabel(nullptr, std::move(text), scale, FontWeight::SemiBold);
    passHitsThrough(*label);
    return label;
  }

  bool setLabelText(Label* label, std::string_view text) { return label != nullptr && label->setText(text); }

  bool setNodeVisible(Node* node, bool visible) {
    if (node == nullptr || node->visible() == visible) {
      return false;
    }
    node->setVisible(visible);
    return true;
  }

} // namespace

HomeTab::HomeTab(const ControlCenterServices& services)
    : m_mpris(services.mpris), m_httpClient(services.httpClient), m_config(services.config),
      m_network(services.network), m_bluetooth(services.bluetooth), m_notifications(services.notifications),
      m_idleInhibitor(services.idleInhibitor), m_nightLight(services.nightLight), m_platform(services.platform),
      m_brightness(services.brightness), m_audio(services.audio), m_services(services.shortcutServices()) {
  // The tile tint and rim are fixed colors picked per light/dark theme, so re-pick them on a theme change.
  m_paletteConn = paletteChanged().connect([this]() { applyTileChrome(); });
}

HomeTab::~HomeTab() = default;

std::unique_ptr<Flex> HomeTab::create() {
  const float scale = contentScale();
  const float gap = kGridGap * scale;
  const float cell = kCellHeight * scale;
  const float blockHeight = cell * 2.0F + gap;

  m_tiles.fill(Tile{});
  m_wifiRow = {};
  m_bluetoothRow = {};
  m_vpnRow = {};
  m_focusRow = {};
  m_caffeineState = -1;
  m_nightLightState = -1;
  m_lastBrightness = -1.0F;
  m_lastVolume = -1.0F;
  m_mediaIdle = -1;
  m_loadedMediaArtUrl.clear();

  auto root = ui::column({
      .out = &m_rootLayout,
      .align = FlexAlign::Stretch,
      .justify = FlexJustify::Start,
      .gap = gap,
  });

  if (showHomeTitle()) {
    root->addChild(ui::label({
        .text = i18n::tr("control-center.home.heading"),
        .fontSize = Style::fontSizeHeader * scale,
        .fontWeight = FontWeight::SemiBold,
        .fontFamily = Style::displayFont(),
        .color = colorSpecFromRole(ColorRole::OnSurface),
        .height = kHomeTitleHeight * scale,
        .maxLines = 1,
    }));
  }

  // --- Top block: connectivity (2x2) | focus (2x1) above caffeine and night light (1x1 each) ---
  auto connectivity = ui::column({
      .align = FlexAlign::Stretch,
      .justify = FlexJustify::Center,
      .gap = Style::spaceMd * scale,
      .padding = kTilePadding * scale,
      .minHeight = blockHeight,
      .maxHeight = blockHeight,
      .flexGrow = 1.0F,
  });
  connectivity->addChild(buildToggleRow(m_wifiRow, "wifi", i18n::tr("control-center.shortcuts.wifi"), [this]() {
    toggleWifi();
  }));
  addRowOverlay(m_wifiRow, [this]() { navigate("network"); });
  connectivity->addChild(buildToggleRow(
      m_bluetoothRow, "bluetooth", i18n::tr("control-center.shortcuts.bluetooth"), [this]() { toggleBluetooth(); }
  ));
  addRowOverlay(m_bluetoothRow, [this]() { navigate("bluetooth"); });
  connectivity->addChild(buildToggleRow(
      m_vpnRow, network_display::vpnGlyph(), i18n::tr("control-center.home.vpn"), [this]() { toggleVpn(); }
  ));
  addRowOverlay(m_vpnRow, [this]() { navigate("network"); });
  m_vpnRow.root->setVisible(false);
  addTileChrome(TileId::Connectivity, *connectivity, nullptr);

  auto focus = buildToggleRow(m_focusRow, "moon-filled", i18n::tr("control-center.home.focus"), [this]() {
    toggleDoNotDisturb();
  });
  focus->setPadding(0.0F, kTilePadding * scale);
  focus->setMinHeight(cell);
  focus->setMaxHeight(cell);
  addTileChrome(TileId::Focus, *focus, [this]() { navigate("notifications"); });

  auto smallTiles = ui::row({.align = FlexAlign::Stretch, .gap = gap, .minHeight = cell, .maxHeight = cell});
  smallTiles->addChild(buildSmallToggleTile(
      TileId::Caffeine, &m_caffeineButton, "caffeine-off", i18n::tr("control-center.shortcuts.caffeine"),
      [this]() { toggleCaffeine(); }, nullptr
  ));
  smallTiles->addChild(buildSmallToggleTile(
      TileId::NightLight, &m_nightLightButton, "nightlight-off", i18n::tr("control-center.shortcuts.nightlight"),
      [this]() { toggleNightLight(); },
      [this]() {
        if (m_nightLight != nullptr && m_platform != nullptr && m_platform->hasGammaControl()) {
          m_nightLight->toggleForceEnabled();
          PanelManager::instance().refresh();
        }
      }
  ));

  auto rightColumn = ui::column(
      {.align = FlexAlign::Stretch, .gap = gap, .minHeight = blockHeight, .maxHeight = blockHeight, .flexGrow = 1.0F},
      std::move(focus), std::move(smallTiles)
  );
  root->addChild(
      ui::row(
          {.align = FlexAlign::Stretch, .gap = gap, .minHeight = blockHeight, .maxHeight = blockHeight},
          std::move(connectivity), std::move(rightColumn)
      )
  );

  // --- Full-width modules ---
  root->addChild(buildDisplayTile());
  root->addChild(buildSoundTile());
  root->addChild(buildMediaTile());

  // --- Round shortcut buttons ---
  rebuildShortcutPads();
  if (auto rows = buildShortcutRows(); rows != nullptr) {
    root->addChild(std::move(rows));
  }

  applyTileChrome();
  return root;
}

std::unique_ptr<Flex>
HomeTab::buildToggleRow(ToggleRow& row, std::string_view glyph, std::string title, std::function<void()> onToggle) {
  const float scale = contentScale();
  const float circle = kRowCircleSize * scale;
  auto rowRoot = ui::row({.out = &row.root, .align = FlexAlign::Center, .gap = Style::spaceSm * scale});
  rowRoot->addChild(roundButton(&row.circle, glyph, circle, circle * 0.5F, std::move(onToggle)));
  rowRoot->addChild(
      ui::column(
          {.align = FlexAlign::Stretch, .justify = FlexJustify::Center, .flexGrow = 1.0F, .configure = passHitsThrough},
          titleLabel(&row.title, std::move(title), scale), secondaryLabel(&row.status, " ", scale, FontWeight::Normal)
      )
  );
  return rowRoot;
}

std::unique_ptr<Flex> HomeTab::buildSmallToggleTile(
    TileId id, Button** circle, std::string_view glyph, std::string title, std::function<void()> onToggle,
    std::function<void()> onRightClick
) {
  const float scale = contentScale();
  const float cell = kCellHeight * scale;
  const float diameter = kSmallCircleSize * scale;
  auto tileRoot = ui::column({
      .align = FlexAlign::Center,
      .justify = FlexJustify::Center,
      .gap = Style::spaceXs * scale,
      .paddingV = Style::spaceXs * scale,
      .paddingH = Style::spaceSm * scale,
      .minHeight = cell,
      .maxHeight = cell,
      .flexGrow = 1.0F,
  });
  auto button = roundButton(circle, glyph, diameter, diameter * 0.5F, onToggle, std::move(onRightClick));
  // The whole tile toggles and takes keyboard focus; a second tab stop on the circle would repeat it.
  button->setTabStop(false);
  tileRoot->addChild(std::move(button));
  tileRoot->addChild(
      ui::label({
          .text = std::move(title),
          .fontSize = Style::fontSizeMini * scale,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxLines = 1,
          .textAlign = TextAlign::Center,
          .ellipsize = TextEllipsize::End,
          .configure = passHitsThrough,
      })
  );
  addTileChrome(id, *tileRoot, std::move(onToggle));
  return tileRoot;
}

std::unique_ptr<Flex> HomeTab::buildDisplayTile() {
  const float scale = contentScale();
  const float height = kSliderTileHeight * scale;
  const float minBrightness = m_config != nullptr ? m_config->config().brightness.minimumBrightness : 0.0F;
  auto tileRoot = ui::column({
      .align = FlexAlign::Stretch,
      .justify = FlexJustify::Center,
      .gap = Style::spaceXs * scale,
      .paddingH = kTilePadding * scale,
      .minHeight = height,
      .maxHeight = height,
      .visible = false,
  });
  tileRoot->addChild(captionLabel(i18n::tr("control-center.home.display"), scale));
  tileRoot->addChild(
      ui::row(
          {.align = FlexAlign::Center, .gap = Style::spaceSm * scale},
          ui::glyph({
              .glyph = "brightness-low",
              .glyphSize = Style::fontSizeTitle * scale,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .configure = passHitsThrough,
          }),
          ui::slider({
              .out = &m_brightnessSlider,
              .minValue = minBrightness,
              .maxValue = 1.0,
              .step = 0.01,
              .value = 1.0,
              .trackHeight = 6.0F * scale,
              .thumbSize = 18.0F * scale,
              .controlHeight = 24.0F * scale,
              .flexGrow = 1.0F,
              .onValueChanged =
                  [this](double value) {
                    if (!m_syncingSlider) {
                      queueBrightness(static_cast<float>(value));
                    }
                  },
              .onDragEnd = [this]() { flushBrightness(); },
          })
      )
  );
  addTileChrome(TileId::Display, *tileRoot, [this]() { navigate("monitor"); });
  return tileRoot;
}

std::unique_ptr<Flex> HomeTab::buildSoundTile() {
  const float scale = contentScale();
  const float height = kSliderTileHeight * scale;
  const float mute = kMuteButtonSize * scale;
  auto tileRoot = ui::column({
      .align = FlexAlign::Stretch,
      .justify = FlexJustify::Center,
      .gap = Style::spaceXs * scale,
      .paddingH = kTilePadding * scale,
      .minHeight = height,
      .maxHeight = height,
  });
  tileRoot->addChild(captionLabel(i18n::tr("control-center.home.sound"), scale));
  tileRoot->addChild(
      ui::row(
          {.align = FlexAlign::Center, .gap = Style::spaceXs * scale},
          ghostButton(&m_muteButton, "volume-high", mute, Style::fontSizeTitle * scale, [this]() { toggleMute(); }),
          ui::slider({
              .out = &m_volumeSlider,
              .minValue = 0.0,
              .maxValue = volumeSliderMax(),
              .step = 0.01,
              .value = 0.0,
              .trackHeight = 6.0F * scale,
              .thumbSize = 18.0F * scale,
              .controlHeight = 24.0F * scale,
              .wheelAdjustEnabled = true,
              .flexGrow = 1.0F,
              .onValueChanged =
                  [this](double value) {
                    if (!m_syncingSlider) {
                      queueVolume(static_cast<float>(value));
                    }
                  },
              .onDragEnd = [this]() { flushVolume(); },
          })
      )
  );
  addTileChrome(TileId::Sound, *tileRoot, [this]() { navigate("audio"); });
  return tileRoot;
}

std::unique_ptr<Flex> HomeTab::buildMediaTile() {
  const float scale = contentScale();
  const float height = kMediaTileHeight * scale;
  const float art = kMediaArtSize * scale;
  const float button = kMediaButtonSize * scale;
  const float artRadius = Style::scaledRadiusMd(scale);
  auto tileRoot = ui::row({
      .align = FlexAlign::Center,
      .gap = Style::spaceSm * scale,
      .paddingH = kTilePadding * scale,
      .minHeight = height,
      .maxHeight = height,
  });
  tileRoot->addChild(
      ui::column(
          {.align = FlexAlign::Center,
           .justify = FlexJustify::Center,
           .fill = colorSpecFromRole(ColorRole::OnSurface, 0.10F),
           .radius = artRadius,
           .minWidth = art,
           .minHeight = art,
           .maxWidth = art,
           .maxHeight = art,
           .configure = passHitsThrough},
          ui::glyph({
              .out = &m_mediaArtFallback,
              .glyph = "music",
              .glyphSize = art * 0.45F,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
          }),
          ui::image({
              .out = &m_mediaArt,
              .fit = ImageFit::Cover,
              .radius = artRadius,
              .width = art,
              .height = art,
              .visible = false,
              .participatesInLayout = false,
              .configure = [](Image& image) { image.setZIndex(1); },
          })
      )
  );
  tileRoot->addChild(
      ui::column(
          {.align = FlexAlign::Stretch, .justify = FlexJustify::Center, .flexGrow = 1.0F, .configure = passHitsThrough},
          titleLabel(&m_mediaTitle, i18n::tr("control-center.home.media.not-playing"), scale),
          secondaryLabel(&m_mediaArtist, " ", scale, FontWeight::Normal)
      )
  );
  tileRoot->addChild(ghostButton(&m_mediaPrev, "media-prev", button, Style::fontSizeBody * scale, [this]() {
    if (m_mpris != nullptr) {
      (void)m_mpris->previousActive();
    }
  }));
  tileRoot->addChild(ghostButton(&m_mediaPlay, "media-play", button, Style::fontSizeTitle * scale, [this]() {
    if (m_mpris != nullptr) {
      (void)m_mpris->playPauseActive();
    }
  }));
  tileRoot->addChild(ghostButton(&m_mediaNext, "media-next", button, Style::fontSizeBody * scale, [this]() {
    if (m_mpris != nullptr) {
      (void)m_mpris->nextActive();
    }
  }));
  addTileChrome(TileId::Media, *tileRoot, [this]() { navigate("media"); });
  return tileRoot;
}

void HomeTab::rebuildShortcutPads() {
  // Keep Shortcut instances across open/close so plugin file watches and runtimes are not recreated
  // on every Control Center open. Match by id from config.
  std::vector<std::unique_ptr<Shortcut>> previous;
  previous.reserve(m_shortcutPads.size());
  for (auto& pad : m_shortcutPads) {
    previous.push_back(std::move(pad.shortcut));
  }
  m_shortcutPads.clear();
  m_shortcutPads.reserve(previous.size() + 2);

  const auto takeReusable = [&previous](std::string_view type) -> std::unique_ptr<Shortcut> {
    for (auto it = previous.begin(); it != previous.end(); ++it) {
      if (*it != nullptr && (*it)->id() == type) {
        auto found = std::move(*it);
        previous.erase(it);
        return found;
      }
    }
    return nullptr;
  };
  const auto adopt = [this](std::unique_ptr<Shortcut> shortcut, bool reused) {
    if (shortcut == nullptr) {
      return;
    }
    if (reused) {
      shortcut->onPanelOpen();
    }
    ShortcutPad pad;
    pad.shortcut = std::move(shortcut);
    m_shortcutPads.push_back(std::move(pad));
  };

  // A plugin shortcut seeds its Luau runtime once, at construction. Plugin settings changed since the
  // last build means every reusable plugin instance is stale, so drop it and let ShortcutRegistry::create()
  // re-seed from the current config. Match on the id syntax, not the registry: a disabled plugin has
  // already left the registry and must not be reused.
  const bool pluginsChanged = m_config != nullptr && !(m_config->config().plugins == m_lastPlugins);
  if (m_config != nullptr) {
    m_lastPlugins = m_config->config().plugins;
  }

  bool sessionConfigured = false;
  if (m_config != nullptr) {
    const auto& configured = m_config->config().controlCenter.shortcuts;
    const std::size_t count = std::min(configured.size(), kMaxConfiguredShortcuts);
    for (std::size_t i = 0; i < count; ++i) {
      const auto& sc = configured[i];
      if (coveredByModule(sc.type)) {
        continue;
      }
      sessionConfigured = sessionConfigured || sc.type == kSessionShortcutId;
      auto shortcut = takeReusable(sc.type);
      if (shortcut != nullptr && pluginsChanged && scripting::isValidPluginEntryId(sc.type)) {
        shortcut.reset();
      }
      const bool reused = shortcut != nullptr;
      if (!reused) {
        shortcut = ShortcutRegistry::create(sc.type, m_services);
      }
      adopt(std::move(shortcut), reused);
    }
  }

  {
    auto shortcut = takeReusable(kSettingsShortcutId);
    const bool reused = shortcut != nullptr;
    if (!reused) {
      shortcut = std::make_unique<SettingsShortcut>();
    }
    adopt(std::move(shortcut), reused);
  }
  if (!sessionConfigured && (m_config == nullptr || m_config->config().controlCenter.showSessionButton)) {
    auto shortcut = takeReusable(kSessionShortcutId);
    const bool reused = shortcut != nullptr;
    if (!reused) {
      shortcut = ShortcutRegistry::create(kSessionShortcutId, m_services);
    }
    adopt(std::move(shortcut), reused);
  }
}

std::unique_ptr<Flex> HomeTab::buildShortcutRows() {
  if (m_shortcutPads.empty()) {
    return nullptr;
  }
  const float scale = contentScale();
  const bool labels = showShortcutLabels(m_config);
  const float cellHeight = shortcutCellHeight(labels) * scale;
  const float diameter = kShortcutCircleSize * scale;

  auto rows = ui::column({.align = FlexAlign::Stretch, .gap = kShortcutRowGap * scale});
  Flex* row = nullptr;
  for (std::size_t i = 0; i < m_shortcutPads.size(); ++i) {
    if (i % kGridColumns == 0) {
      row = static_cast<Flex*>(rows->addChild(
          ui::row({
              .align = FlexAlign::Start,
              .gap = kGridGap * scale,
              .minHeight = cellHeight,
              .maxHeight = cellHeight,
          })
      ));
    }
    ShortcutPad& pad = m_shortcutPads[i];
    auto cellNode = ui::column({
        .align = FlexAlign::Center,
        .justify = FlexJustify::Start,
        .gap = Style::spaceXs * scale,
        .minHeight = cellHeight,
        .maxHeight = cellHeight,
        .flexGrow = 1.0F,
    });
    auto button = roundButton(
        &pad.button, pad.shortcut->displayIcon(), diameter, Style::fontSizeTitle * 1.2F * scale,
        [this, i]() {
          if (i < m_shortcutPads.size() && m_shortcutPads[i].shortcut != nullptr) {
            m_shortcutPads[i].shortcut->onClick();
            PanelManager::instance().refresh();
          }
        },
        [this, i]() {
          if (i < m_shortcutPads.size() && m_shortcutPads[i].shortcut != nullptr) {
            m_shortcutPads[i].shortcut->onRightClick();
            PanelManager::instance().refresh();
          }
        }
    );
    if (InputArea* area = button->inputArea(); area != nullptr) {
      area->setOnAxisHandler([this, i](const InputArea::PointerData& data) -> bool {
        if (data.axis != WL_POINTER_AXIS_VERTICAL_SCROLL || i >= m_shortcutPads.size()) {
          return false;
        }
        const float steps = data.scrollSteps();
        if (steps == 0.0F || m_shortcutPads[i].shortcut == nullptr) {
          return false;
        }
        // Scroll up moves forward (toward performance); Wayland reports up as a negative delta.
        m_shortcutPads[i].shortcut->onScroll(steps > 0.0F ? -1 : 1);
        return true;
      });
    }
    cellNode->addChild(std::move(button));
    if (labels) {
      cellNode->addChild(
          ui::label({
              .out = &pad.label,
              .text = pad.shortcut->displayLabel(),
              .fontSize = Style::fontSizeMini * scale,
              .color = colorSpecFromRole(ColorRole::OnSurface),
              .maxLines = 1,
              .textAlign = TextAlign::Center,
              .ellipsize = TextEllipsize::End,
          })
      );
    }
    row->addChild(std::move(cellNode));
  }

  // Fill out the last row so its buttons keep the four-column rhythm of the tiles above.
  if (const std::size_t used = m_shortcutPads.size() % kGridColumns; used != 0 && row != nullptr) {
    for (std::size_t i = used; i < kGridColumns; ++i) {
      row->addChild(ui::column({.flexGrow = 1.0F}));
    }
  }
  return rows;
}

void HomeTab::addTileChrome(TileId id, Flex& root, std::function<void()> onActivate) {
  Tile& entry = tile(id);
  entry.root = &root;

  // Inserted first, in z order, so hit testing keeps its sorted fast path.
  auto surface = ui::box({
      .participatesInLayout = false,
      .configure = [](Box& box) {
        box.setZIndex(-2);
        box.setHitTestVisible(false);
      },
  });
  entry.surface = static_cast<Box*>(root.insertChildAt(0, std::move(surface)));

  if (!onActivate) {
    return;
  }
  const auto setFocused = [this, id](bool focused) {
    tile(id).focused = focused;
    applyTileChrome();
    PanelManager::instance().requestRedraw();
  };
  auto overlay = ui::inputArea({
      .cursorShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER,
      .focusable = true,
      .zIndex = -1,
      .participatesInLayout = false,
      .onClick = [onActivate](const InputArea::PointerData&) { onActivate(); },
      .onKeyDown =
          [onActivate](const InputArea::KeyData& key) {
            if (key.pressed && KeybindMatcher::matches(KeybindAction::Validate, key.sym, key.modifiers)) {
              onActivate();
            }
          },
      .onFocusGain = [setFocused]() { setFocused(true); },
      .onFocusLoss = [setFocused]() { setFocused(false); },
  });
  entry.overlay = static_cast<InputArea*>(root.insertChildAt(1, std::move(overlay)));
}

void HomeTab::addRowOverlay(ToggleRow& row, std::function<void()> onActivate) {
  if (row.root == nullptr) {
    return;
  }
  // A row has no tile of its own, so a soft rounded highlight marks its keyboard focus. It is a
  // separate box rather than the row's own fill: that fill node would sit above the overlay in hit testing.
  auto highlight = ui::box({
      .fill = colorSpecFromRole(ColorRole::OnSurface, 0.08F),
      .radius = Style::scaledRadiusMd(contentScale()),
      .visible = false,
      .participatesInLayout = false,
      .configure = [](Box& box) {
        box.setZIndex(-2);
        box.setHitTestVisible(false);
      },
  });
  Box* highlightPtr = static_cast<Box*>(row.root->insertChildAt(0, std::move(highlight)));
  row.highlight = highlightPtr;
  const auto setFocused = [highlightPtr](bool focused) {
    highlightPtr->setVisible(focused);
    PanelManager::instance().requestRedraw();
  };
  auto overlay = ui::inputArea({
      .cursorShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER,
      .focusable = true,
      .zIndex = -1,
      .participatesInLayout = false,
      .onClick = [onActivate](const InputArea::PointerData&) { onActivate(); },
      .onKeyDown =
          [onActivate](const InputArea::KeyData& key) {
            if (key.pressed && KeybindMatcher::matches(KeybindAction::Validate, key.sym, key.modifiers)) {
              onActivate();
            }
          },
      .onFocusGain = [setFocused]() { setFocused(true); },
      .onFocusLoss = [setFocused]() { setFocused(false); },
  });
  row.overlay = static_cast<InputArea*>(row.root->insertChildAt(1, std::move(overlay)));
}

void HomeTab::applyTileChrome() {
  const float scale = contentScale();
  const float radius = Style::scaledRadiusLg(scale);
  const bool light = isResolvedLightTheme();
  const ColorSpec fill = tileFill(panelCardOpacity(), light);
  for (Tile& entry : m_tiles) {
    if (entry.surface == nullptr) {
      continue;
    }
    entry.surface->setRadius(radius);
    entry.surface->setFill(fill);
    if (entry.focused) {
      entry.surface->setBorder(focusRingColorSpec(), Style::focusRingWidth);
      entry.surface->setRimLight(0.0F);
    } else {
      shell::material::applyHairline(*entry.surface, light, scale);
    }
  }
}

void HomeTab::layoutTileChrome() {
  for (Tile& entry : m_tiles) {
    if (entry.root == nullptr) {
      continue;
    }
    const float width = entry.root->width();
    const float height = entry.root->height();
    if (entry.surface != nullptr) {
      entry.surface->setPosition(0.0F, 0.0F);
      entry.surface->setSize(width, height);
    }
    if (entry.overlay != nullptr) {
      entry.overlay->setPosition(0.0F, 0.0F);
      entry.overlay->setSize(width, height);
    }
  }
  for (ToggleRow* row : {&m_wifiRow, &m_bluetoothRow, &m_vpnRow}) {
    if (row->root == nullptr) {
      continue;
    }
    // The highlight bleeds a little past the row so its rounded edge does not hug the circle.
    const float bleed = Style::spaceXs * contentScale();
    if (row->highlight != nullptr) {
      row->highlight->setPosition(-bleed, -bleed);
      row->highlight->setSize(row->root->width() + bleed * 2.0F, row->root->height() + bleed * 2.0F);
    }
    if (row->overlay != nullptr) {
      row->overlay->setPosition(0.0F, 0.0F);
      row->overlay->setSize(row->root->width(), row->root->height());
    }
  }
}

void HomeTab::navigate(std::string_view tab) {
  if (m_navigate) {
    m_navigate(tab);
  }
}

float HomeTab::naturalHeight(float scale) const {
  const float gap = kGridGap * scale;
  float height = kCellHeight * 2.0F * scale + gap;
  if (showHomeTitle()) {
    height += kHomeTitleHeight * scale + gap;
  }
  if (targetDisplay() != nullptr) {
    height += gap + kSliderTileHeight * scale;
  }
  height += gap + kSliderTileHeight * scale;
  height += gap + kMediaTileHeight * scale;
  if (const std::size_t items = shortcutItemCount(); items > 0) {
    const auto rows = static_cast<float>((items + kGridColumns - 1) / kGridColumns);
    height +=
        gap + rows * shortcutCellHeight(showShortcutLabels(m_config)) * scale + (rows - 1.0F) * kShortcutRowGap * scale;
  }
  return std::ceil(height);
}

bool HomeTab::showHomeTitle() const {
  return m_config != nullptr && m_config->config().controlCenter.showHomeTitle;
}

std::size_t HomeTab::shortcutItemCount() const {
  if (m_rootLayout != nullptr) {
    return m_shortcutPads.size();
  }
  // Not built yet (sizing before open): estimate from config, as rebuildShortcutPads() will build it.
  std::size_t count = 1; // settings
  if (m_config == nullptr) {
    return count + 1; // session
  }
  const Config& config = m_config->config();
  const auto& configured = config.controlCenter.shortcuts;
  const std::size_t limit = std::min(configured.size(), kMaxConfiguredShortcuts);
  bool sessionConfigured = false;
  for (std::size_t i = 0; i < limit; ++i) {
    const std::string& type = configured[i].type;
    if (coveredByModule(type) || !ShortcutRegistry::isAvailable(type, config)) {
      continue;
    }
    sessionConfigured = sessionConfigured || type == kSessionShortcutId;
    ++count;
  }
  if (!sessionConfigured && config.controlCenter.showSessionButton) {
    ++count;
  }
  return count;
}

bool HomeTab::dragging() const noexcept {
  return (m_brightnessSlider != nullptr && m_brightnessSlider->dragging())
      || (m_volumeSlider != nullptr && m_volumeSlider->dragging());
}

void HomeTab::doLayout(Renderer& renderer, float contentWidth, float bodyHeight) {
  if (m_rootLayout == nullptr) {
    return;
  }
  m_rootLayout->setSize(contentWidth, bodyHeight);
  m_rootLayout->layout(renderer);
  layoutTileChrome();

  const float natural = naturalHeight(contentScale());
  if (std::abs(natural - m_reportedNaturalHeight) > 0.5F) {
    m_reportedNaturalHeight = natural;
    if (m_active && m_naturalHeightChanged) {
      // Resizing the panel surface re-enters layout, so let this pass finish first.
      DeferredCall::callLater([callback = m_naturalHeightChanged]() { callback(); });
    }
  }
}

void HomeTab::doUpdate(Renderer& renderer) {
  if (m_rootLayout == nullptr) {
    return;
  }
  sync(renderer);
}

void HomeTab::setActive(bool active) {
  const bool becameActive = active && !m_active;
  m_active = active;
  if (!active) {
    flushBrightness();
    flushVolume();
    return;
  }
  if (becameActive) {
    // Other tabs were laid out while this body was hidden; defer so the tab container receives its
    // final size before HomeTab::doLayout runs.
    DeferredCall::callLater([]() {
      PanelManager::instance().requestLayout();
      PanelManager::instance().requestUpdateOnly();
    });
  }
}

void HomeTab::onClose() {
  flushBrightness();
  flushVolume();
  m_brightnessTimer.stop();
  m_volumeTimer.stop();
  m_rootLayout = nullptr;
  m_tiles.fill(Tile{});
  m_wifiRow = {};
  m_bluetoothRow = {};
  m_vpnRow = {};
  m_focusRow = {};
  m_caffeineButton = nullptr;
  m_nightLightButton = nullptr;
  m_brightnessSlider = nullptr;
  m_volumeSlider = nullptr;
  m_muteButton = nullptr;
  m_mediaArt = nullptr;
  m_mediaArtFallback = nullptr;
  m_mediaTitle = nullptr;
  m_mediaArtist = nullptr;
  m_mediaPrev = nullptr;
  m_mediaPlay = nullptr;
  m_mediaNext = nullptr;
  m_loadedMediaArtUrl.clear();
  // Keep Shortcut instances alive; drop only the UI pointers destroyed with the scene.
  for (auto& pad : m_shortcutPads) {
    if (pad.shortcut != nullptr) {
      pad.shortcut->onPanelClose();
    }
    pad.button = nullptr;
    pad.label = nullptr;
    pad.appliedState = -1;
  }
}

void HomeTab::onPanelCardOpacityChanged(float /*opacity*/) { applyTileChrome(); }

void HomeTab::sync(Renderer& renderer) {
  syncConnectivity();
  syncToggles();
  syncDisplay();
  syncSound();
  syncMedia(renderer);
  syncShortcuts();
}

void HomeTab::syncConnectivity() {
  bool relayout = false;

  {
    const bool available = m_network != nullptr && m_network->available();
    const NetworkState* state = available ? &m_network->state() : nullptr;
    const bool on = state != nullptr && state->wirelessEnabled;
    applyToggleState(m_wifiRow.circle, on, available, m_wifiRow.appliedState);
    std::string status;
    if (!on) {
      status = i18n::tr("common.states.off");
    } else if (state->kind == NetworkConnectivity::Wireless && state->connected && !state->ssid.empty()) {
      status = state->ssid;
    } else {
      status = i18n::tr("control-center.network.not-connected");
    }
    relayout = setLabelText(m_wifiRow.status, status) || relayout;
  }

  {
    const BluetoothState* state = m_bluetooth != nullptr ? &m_bluetooth->state() : nullptr;
    const bool present = state != nullptr && state->adapterPresent;
    const bool on = present && state->powered;
    applyToggleState(m_bluetoothRow.circle, on, present && !state->rfkillHardBlocked, m_bluetoothRow.appliedState);
    std::string_view connected;
    if (on) {
      for (const auto& device : m_bluetooth->devices()) {
        if (device.connected) {
          connected = device.alias;
          break;
        }
      }
    }
    const std::string status = !on ? i18n::tr("common.states.off")
        : connected.empty()        ? i18n::tr("common.states.on")
                                   : std::string(connected);
    relayout = setLabelText(m_bluetoothRow.status, status) || relayout;
  }

  {
    // The VPN row only exists when the network service knows a VPN connection.
    const bool hasVpn = m_network != nullptr && m_network->available() && !m_network->vpnConnections().empty();
    relayout = setNodeVisible(m_vpnRow.root, hasVpn) || relayout;
    if (hasVpn) {
      std::string_view activeName;
      for (const auto& vpn : m_network->vpnConnections()) {
        if (vpn.active) {
          activeName = vpn.name;
          break;
        }
      }
      const bool on = m_network->state().vpnActive || !activeName.empty();
      applyToggleState(m_vpnRow.circle, on, true, m_vpnRow.appliedState);
      const std::string status = !activeName.empty() ? std::string(activeName)
          : on                                       ? i18n::tr("common.states.on")
                                                     : i18n::tr("common.states.off");
      relayout = setLabelText(m_vpnRow.status, status) || relayout;
    }
  }

  {
    const bool dnd = m_notifications != nullptr && m_notifications->doNotDisturb();
    applyToggleState(m_focusRow.circle, dnd, m_notifications != nullptr, m_focusRow.appliedState);
    const std::string status = dnd ? i18n::tr("control-center.home.do-not-disturb") : i18n::tr("common.states.off");
    relayout = setLabelText(m_focusRow.status, status) || relayout;
  }

  if (relayout) {
    PanelManager::instance().requestLayout();
  }
}

void HomeTab::syncToggles() {
  const bool caffeine = m_idleInhibitor != nullptr && m_idleInhibitor->enabled();
  if (applyToggleState(m_caffeineButton, caffeine, m_idleInhibitor != nullptr, m_caffeineState)) {
    m_caffeineButton->setGlyph(caffeine ? "caffeine-on" : "caffeine-off");
  }

  const bool gamma = m_nightLight != nullptr && m_platform != nullptr && m_platform->hasGammaControl();
  const bool forced = gamma && m_nightLight->forceEnabled();
  // On while scheduled, not only while the screen is currently warmed, so a click reads as taken.
  const bool nightLight = gamma && (forced || m_nightLight->enabled());
  applyToggleState(m_nightLightButton, nightLight, gamma, m_nightLightState);
  if (m_nightLightButton != nullptr) {
    m_nightLightButton->setGlyph(forced ? "nightlight-forced" : (nightLight ? "nightlight-on" : "nightlight-off"));
  }
}

const BrightnessDisplay* HomeTab::targetDisplay() const {
  if (m_brightness == nullptr) {
    return nullptr;
  }
  // Like macOS, drive the display the panel is shown on; fall back to the first one we can control.
  if (wl_output* output = PanelManager::instance().attachedPanelOutput(); output != nullptr) {
    if (const auto* display = m_brightness->findByOutput(output); display != nullptr && display->controllable) {
      return display;
    }
  }
  for (const auto& display : m_brightness->displays()) {
    if (display.controllable) {
      return &display;
    }
  }
  return nullptr;
}

void HomeTab::syncDisplay() {
  const BrightnessDisplay* display = targetDisplay();
  if (setNodeVisible(tile(TileId::Display).root, display != nullptr)) {
    PanelManager::instance().requestLayout();
  }
  if (display == nullptr || m_brightnessSlider == nullptr) {
    return;
  }
  if (m_brightnessDisplayId != display->id) {
    flushBrightness();
    m_brightnessDisplayId = display->id;
    m_lastBrightness = -1.0F;
  }
  if (m_brightnessSlider->dragging()
      || m_pendingBrightness >= 0.0F
      || std::chrono::steady_clock::now() < m_brightnessHoldUntil) {
    return;
  }
  const float minBrightness = m_config != nullptr ? m_config->config().brightness.minimumBrightness : 0.0F;
  const float value = std::clamp(display->brightness, minBrightness, 1.0F);
  if (std::abs(value - m_lastBrightness) >= kSliderSyncEpsilon) {
    m_syncingSlider = true;
    m_brightnessSlider->setValue(value);
    m_syncingSlider = false;
    m_lastBrightness = value;
  }
}

void HomeTab::syncSound() {
  const AudioNode* sink = m_audio != nullptr ? m_audio->defaultSink() : nullptr;
  if (m_volumeSlider != nullptr) {
    m_volumeSlider->setEnabled(sink != nullptr);
    const bool hold =
        m_volumeSlider->dragging() || m_pendingVolume >= 0.0F || std::chrono::steady_clock::now() < m_volumeHoldUntil;
    if (sink != nullptr && !hold) {
      const float value = std::clamp(sink->volume, 0.0F, volumeSliderMax());
      if (std::abs(value - m_lastVolume) >= kSliderSyncEpsilon) {
        m_syncingSlider = true;
        m_volumeSlider->setValue(value);
        m_syncingSlider = false;
        m_lastVolume = value;
      }
    }
  }
  if (m_muteButton != nullptr) {
    m_muteButton->setEnabled(sink != nullptr);
    const bool muted = sink == nullptr || sink->muted;
    const float level = sink != nullptr ? sink->volume : 0.0F;
    m_muteButton->setGlyph(muted ? "volume-mute" : (level < 0.34F ? "volume-low" : "volume-high"));
  }
}

void HomeTab::syncMedia(Renderer& renderer) {
  if (m_mediaTitle == nullptr || m_mediaArtist == nullptr) {
    return;
  }
  bool relayout = false;
  const std::optional<MprisPlayerInfo> active = m_mpris != nullptr ? m_mpris->activePlayer() : std::nullopt;
  const auto idle = static_cast<std::int8_t>(active.has_value() ? 0 : 1);
  if (idle != m_mediaIdle) {
    m_mediaIdle = idle;
    m_mediaTitle->setColor(colorSpecFromRole(idle != 0 ? ColorRole::OnSurfaceVariant : ColorRole::OnSurface));
    relayout = setNodeVisible(m_mediaArtist, idle == 0) || relayout;
  }

  if (!active.has_value()) {
    relayout = setLabelText(m_mediaTitle, i18n::tr("control-center.home.media.not-playing")) || relayout;
    for (Button* button : {m_mediaPrev, m_mediaPlay, m_mediaNext}) {
      if (button != nullptr) {
        button->setEnabled(false);
      }
    }
    if (m_mediaPlay != nullptr) {
      m_mediaPlay->setGlyph("media-play");
    }
    if (m_mediaArt != nullptr && !m_loadedMediaArtUrl.empty()) {
      m_mediaArt->clear(renderer);
    }
    setNodeVisible(m_mediaArt, false);
    setNodeVisible(m_mediaArtFallback, true);
    m_loadedMediaArtUrl.clear();
  } else {
    const std::string title =
        active->title.empty() ? i18n::tr("control-center.home.media.unknown-track") : active->title;
    const std::string artists = mpris::joinArtists(active->artists);
    relayout = setLabelText(m_mediaTitle, title) || relayout;
    relayout =
        setLabelText(m_mediaArtist, artists.empty() ? i18n::tr("control-center.home.media.unknown-artist") : artists)
        || relayout;

    const bool playing = active->playbackStatus == "Playing";
    if (m_mediaPrev != nullptr) {
      m_mediaPrev->setEnabled(active->canGoPrevious);
    }
    if (m_mediaNext != nullptr) {
      m_mediaNext->setEnabled(active->canGoNext);
    }
    if (m_mediaPlay != nullptr) {
      m_mediaPlay->setEnabled(playing ? active->canPause : active->canPlay);
      m_mediaPlay->setGlyph(playing ? "media-pause" : "media-play");
    }

    if (m_mediaArt != nullptr) {
      const std::string artUrl = mpris::effectiveArtUrl(*active);
      const bool artRetry = !artUrl.empty() && !m_mediaArt->hasImage();
      if (artUrl != m_loadedMediaArtUrl || artRetry) {
        const std::string artPath = mpris::resolveArtworkSource(
            m_httpClient, m_pendingArtDownloads, artUrl,
            [this] {
              m_loadedMediaArtUrl.clear();
              PanelManager::instance().refresh();
            },
            m_aliveGuard
        );
        bool loaded = false;
        if (!artPath.empty()) {
          const int decodeSize =
              static_cast<int>(std::lround(kMediaArtSize * contentScale() * std::max(1.0F, renderer.renderScale())));
          loaded = m_mediaArt->setSourceFile(renderer, artPath, decodeSize, true, true);
        }
        if (!loaded) {
          m_mediaArt->clear(renderer);
        }
        setNodeVisible(m_mediaArt, loaded);
        setNodeVisible(m_mediaArtFallback, !loaded);
        m_loadedMediaArtUrl = loaded ? artUrl : std::string{};
      }
    }
  }

  if (relayout) {
    PanelManager::instance().requestLayout();
  }
}

void HomeTab::syncShortcuts() {
  bool relayout = false;
  for (auto& pad : m_shortcutPads) {
    if (pad.shortcut == nullptr || pad.button == nullptr) {
      continue;
    }
    const Shortcut& shortcut = *pad.shortcut;
    applyToggleState(pad.button, shortcut.isToggle() && shortcut.active(), shortcut.enabled(), pad.appliedState);
    pad.button->setGlyph(shortcut.displayIcon());
    relayout = setLabelText(pad.label, shortcut.displayLabel()) || relayout;
  }
  if (relayout) {
    PanelManager::instance().requestLayout();
  }
}

void HomeTab::toggleWifi() {
  if (m_network != nullptr && m_network->available()) {
    m_network->setWirelessEnabled(!m_network->state().wirelessEnabled);
    PanelManager::instance().refresh();
  }
}

void HomeTab::toggleBluetooth() {
  if (m_bluetooth != nullptr && m_bluetooth->state().adapterPresent) {
    m_bluetooth->setPowered(!m_bluetooth->state().powered);
    PanelManager::instance().refresh();
  }
}

void HomeTab::toggleVpn() {
  if (m_network == nullptr) {
    return;
  }
  // Off disconnects every active VPN; on brings up the first configured one (the Network tab lists
  // them all for a specific choice).
  const auto& vpns = m_network->vpnConnections();
  bool deactivated = false;
  for (const auto& vpn : vpns) {
    if (vpn.active) {
      deactivated = m_network->deactivateVpnConnection(vpn) || deactivated;
    }
  }
  if (!deactivated && !vpns.empty() && !m_network->state().vpnActive) {
    (void)m_network->activateVpnConnection(vpns.front());
  }
  PanelManager::instance().refresh();
}

void HomeTab::toggleDoNotDisturb() {
  if (m_notifications != nullptr) {
    (void)m_notifications->toggleDoNotDisturb();
    PanelManager::instance().refresh();
  }
}

void HomeTab::toggleCaffeine() {
  if (m_idleInhibitor != nullptr) {
    m_idleInhibitor->toggle();
    PanelManager::instance().refresh();
  }
}

void HomeTab::toggleNightLight() {
  if (m_nightLight != nullptr && m_platform != nullptr && m_platform->hasGammaControl()) {
    m_nightLight->toggleEnabled();
    PanelManager::instance().refresh();
  }
}

void HomeTab::toggleMute() {
  if (m_audio == nullptr) {
    return;
  }
  if (const AudioNode* sink = m_audio->defaultSink(); sink != nullptr) {
    m_audio->setSinkMuted(sink->id, !sink->muted);
    PanelManager::instance().refresh();
  }
}

float HomeTab::volumeSliderMax() const {
  return (m_config != nullptr && m_config->config().audio.enableOverdrive) ? 1.5F : 1.0F;
}

void HomeTab::queueBrightness(float value) {
  m_pendingBrightness = value;
  if (std::chrono::steady_clock::now() - m_lastBrightnessCommitAt >= kSliderCommitInterval) {
    flushBrightness();
    return;
  }
  if (!m_brightnessTimer.active()) {
    m_brightnessTimer.start(kSliderCommitInterval, [this]() { flushBrightness(); });
  }
}

void HomeTab::flushBrightness() {
  m_brightnessTimer.stop();
  if (m_pendingBrightness < 0.0F || m_brightness == nullptr) {
    m_pendingBrightness = -1.0F;
    return;
  }
  const auto* display = m_brightness->findDisplay(m_brightnessDisplayId);
  if (display != nullptr && display->controllable) {
    m_brightness->setBrightness(m_brightnessDisplayId, m_pendingBrightness);
    m_lastBrightness = m_pendingBrightness;
  }
  m_pendingBrightness = -1.0F;
  m_lastBrightnessCommitAt = std::chrono::steady_clock::now();
  m_brightnessHoldUntil = m_lastBrightnessCommitAt + kSliderStateHoldoff;
}

void HomeTab::queueVolume(float value) {
  const AudioNode* sink = m_audio != nullptr ? m_audio->defaultSink() : nullptr;
  if (sink == nullptr) {
    return;
  }
  m_pendingSinkId = sink->id;
  m_pendingVolume = std::clamp(value, 0.0F, volumeSliderMax());
  if (std::chrono::steady_clock::now() - m_lastVolumeCommitAt >= kSliderCommitInterval) {
    flushVolume();
    return;
  }
  if (!m_volumeTimer.active()) {
    m_volumeTimer.start(kSliderCommitInterval, [this]() { flushVolume(); });
  }
}

void HomeTab::flushVolume() {
  m_volumeTimer.stop();
  if (m_pendingVolume < 0.0F || m_audio == nullptr || m_pendingSinkId == 0) {
    m_pendingVolume = -1.0F;
    return;
  }
  m_audio->setSinkVolume(m_pendingSinkId, m_pendingVolume);
  m_audio->emitVolumePreview(false, m_pendingSinkId, m_pendingVolume);
  m_lastVolume = m_pendingVolume;
  m_pendingVolume = -1.0F;
  m_lastVolumeCommitAt = std::chrono::steady_clock::now();
  m_volumeHoldUntil = m_lastVolumeCommitAt + kSliderStateHoldoff;
}
