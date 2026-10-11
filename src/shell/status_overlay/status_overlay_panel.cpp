#include "shell/status_overlay/status_overlay_panel.h"

#include "config/config_service.h"
#include "core/input/keybind_matcher.h"
#include "dbus/bluetooth/bluetooth_service.h"
#include "dbus/mpris/mpris_service.h"
#include "dbus/network/inetwork_service.h"
#include "dbus/network/network_display.h"
#include "dbus/upower/upower_service.h"
#include "i18n/i18n.h"
#include "pipewire/audio_glyphs.h"
#include "pipewire/pipewire_service.h"
#include "render/scene/input_area.h"
#include "shell/panel/panel_manager.h"
#include "system/brightness_service.h"
#include "time/time_format.h"
#include "ui/builders.h"
#include "ui/controls/box.h"
#include "ui/controls/flex.h"
#include "ui/controls/glyph.h"
#include "ui/controls/label.h"
#include "ui/controls/progress_bar.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <string>
#include <utility>

namespace {

  // Fill sizing takes the extent the compositor assigns; this is only the fallback (clamped to the output).
  constexpr float kFallbackWidth = 1280.0F;
  constexpr float kFallbackHeight = 800.0F;

  constexpr const char* kDefaultTimeFormat = "{:%H:%M}";
  constexpr float kClockFontSize = 88.0F;
  constexpr float kClockToTilesGap = 36.0F;

  constexpr float kTileWidth = 168.0F;
  constexpr float kTileHeight = 96.0F;
  constexpr float kTileGap = Style::spaceMd;
  constexpr std::size_t kMaxColumns = 3; // all six tiles form a 3 x 2 grid
  constexpr float kTileGlyphSize = 16.0F;
  constexpr float kTileValueFontSize = Style::fontSizeTitle + 2.0F;
  constexpr float kLevelHeight = 4.0F;
  constexpr float kLevelGap = 6.0F;
  constexpr float kLevelTrackAlpha = 0.14F;
  constexpr float kLevelNeutralAlpha = 0.45F;

  // Both follow the panel card opacity, so the transparency mode makes the overlay more or less see-through
  // like every other panel. In the default Glass mode (card opacity ~0.65) the tile tint lands near GlassStyle's
  // default and the scrim lays the surface color over the blurred desktop at about 40 %.
  constexpr float kScrimAlphaPerCardOpacity = 0.6F;
  constexpr float kTileTintPerCardOpacity = 0.8F;

  [[nodiscard]] shell::glass::GlassStyle tileGlassStyle(float scale, float cardOpacity) {
    return shell::glass::GlassStyle{
        .material = shell::glass::Material::Glass,
        .radius = Radii{Style::scaledRadiusXl(scale)},
        .tintOpacity = std::clamp(cardOpacity * kTileTintPerCardOpacity, 0.0F, 1.0F),
    };
  }

  struct TileContent {
    bool available = false;
    const char* glyph = "";
    std::string title;
    std::string value;
    float level = 0.0F; // 0..1, read only by tiles with a level bar
    bool valueDimmed = false;
    bool levelNeutral = false;
  };

  [[nodiscard]] std::string percentText(double percent) {
    return std::to_string(std::lround(std::max(0.0, percent))) + "%";
  }

  [[nodiscard]] TileContent batteryContent(const UPowerService* upower) {
    TileContent content;
    // The display device reports no battery on desktops.
    if (upower == nullptr || !upower->state().isPresent) {
      return content;
    }
    const UPowerState& state = upower->state();
    content.available = true;
    content.glyph = batteryGlyphName(state.percentage, state.state);
    // On external power the state ("Charging", "Plugged in") says more than the word "Battery".
    content.title = batteryStatePlugged(state.state).value_or(false) ? batteryStateLabel(state.state)
                                                                     : i18n::tr("status-overlay.battery.title");
    content.value = percentText(state.percentage);
    content.level = static_cast<float>(state.percentage / 100.0);
    return content;
  }

  [[nodiscard]] TileContent networkContent(const INetworkService* network) {
    TileContent content;
    if (network == nullptr || !network->available() || !network->hasStateSnapshot()) {
      return content;
    }
    const NetworkState& state = network->state();
    content.available = true;
    content.glyph = network_display::glyphForState(state);
    switch (state.kind) {
    case NetworkConnectivity::Wired:
      content.title = i18n::tr("status-overlay.network.ethernet");
      break;
    case NetworkConnectivity::Cellular:
      content.title = i18n::tr("status-overlay.network.cellular");
      break;
    default:
      content.title = i18n::tr("status-overlay.network.wifi");
      break;
    }
    const bool radioOnly = state.kind != NetworkConnectivity::Wired && state.kind != NetworkConnectivity::Cellular;
    if (state.connected) {
      content.value = state.kind == NetworkConnectivity::Wireless && !state.ssid.empty()
          ? state.ssid
          : i18n::tr("status-overlay.network.connected");
    } else if (state.resolving) {
      content.value = i18n::tr("status-overlay.network.connecting");
    } else if (radioOnly && !state.wirelessEnabled) {
      content.value = i18n::tr("status-overlay.off");
      content.valueDimmed = true;
    } else {
      content.value = i18n::tr("status-overlay.network.not-connected");
      content.valueDimmed = true;
    }
    return content;
  }

  [[nodiscard]] TileContent bluetoothContent(const BluetoothService* bluetooth) {
    TileContent content;
    if (bluetooth == nullptr || !bluetooth->state().adapterPresent) {
      return content;
    }
    content.available = true;
    content.title = i18n::tr("status-overlay.bluetooth.title");
    if (!bluetooth->state().powered) {
      content.glyph = "bluetooth-off";
      content.value = i18n::tr("status-overlay.off");
      content.valueDimmed = true;
      return content;
    }
    long connected = 0;
    const BluetoothDeviceInfo* first = nullptr;
    for (const BluetoothDeviceInfo& device : bluetooth->devices()) {
      if (device.connected) {
        ++connected;
        if (first == nullptr) {
          first = &device;
        }
      }
    }
    content.glyph = connected > 0 ? "bluetooth-connected" : "bluetooth";
    if (connected == 0) {
      content.value = i18n::tr("status-overlay.on");
    } else if (connected == 1 && !first->alias.empty()) {
      content.value = first->alias;
    } else {
      content.value = i18n::trp("status-overlay.bluetooth.devices", connected);
    }
    return content;
  }

  [[nodiscard]] TileContent volumeContent(const PipeWireService* audio) {
    TileContent content;
    const AudioNode* sink = audio != nullptr ? audio->defaultSink() : nullptr;
    if (sink == nullptr) {
      return content;
    }
    content.available = true;
    content.glyph = audioVolumeGlyph(sink->volume, sink->muted, /*isInput=*/false);
    content.title = i18n::tr("status-overlay.volume.title");
    content.value = sink->muted ? i18n::tr("status-overlay.volume.muted") : percentText(sink->volume * 100.0F);
    content.level = sink->volume;
    content.valueDimmed = sink->muted;
    content.levelNeutral = sink->muted;
    return content;
  }

  // The display the overlay is on; one without brightness control (an external monitor without DDC) falls back
  // to the first controllable display, usually the laptop panel.
  [[nodiscard]] const BrightnessDisplay* overlayDisplay(const BrightnessService& brightness) {
    const BrightnessDisplay* display = brightness.findByOutput(PanelManager::instance().attachedPanelOutput());
    if (display != nullptr && display->controllable) {
      return display;
    }
    const auto& displays = brightness.displays();
    const auto it = std::ranges::find_if(displays, [](const BrightnessDisplay& d) { return d.controllable; });
    return it != displays.end() ? &*it : nullptr;
  }

  [[nodiscard]] TileContent brightnessContent(const BrightnessService* brightness) {
    TileContent content;
    const BrightnessDisplay* display = brightness != nullptr ? overlayDisplay(*brightness) : nullptr;
    if (display == nullptr) {
      return content;
    }
    content.available = true;
    // Same threshold as the bar's brightness widget.
    content.glyph = display->brightness < 0.4F ? "brightness-low" : "brightness-high";
    content.title = i18n::tr("status-overlay.brightness.title");
    content.value = percentText(display->brightness * 100.0F);
    content.level = display->brightness;
    return content;
  }

  [[nodiscard]] TileContent mediaContent(const MprisService* mpris) {
    TileContent content;
    if (mpris == nullptr) {
      return content;
    }
    content.available = true;
    content.title = i18n::tr("status-overlay.media.title");
    const auto player = mpris->activePlayer();
    if (!player.has_value()) {
      content.glyph = "music-off";
      content.value = i18n::tr("status-overlay.media.nothing-playing");
      content.valueDimmed = true;
      return content;
    }
    content.glyph = "music";
    std::string track = !player->title.empty() ? player->title
        : !player->identity.empty()            ? player->identity
                                               : i18n::tr("status-overlay.media.unknown-track");
    const std::string artists = joinedArtists(player->artists);
    content.value = artists.empty() ? std::move(track) : std::format("{} — {}", track, artists);
    return content;
  }

} // namespace

StatusOverlayPanel::StatusOverlayPanel(StatusOverlayServices services) : m_services(services) {}

void StatusOverlayPanel::create() {
  const float scale = contentScale();

  // The root takes the scrim clicks: tiles are input areas of their own and swallow theirs, so only a click
  // outside every tile dismisses.
  auto rootArea = ui::inputArea({
      .onClick = [](const InputArea::PointerData& /*data*/) { PanelManager::instance().close(); },
  });
  // Rounded like the compositor blur region PanelManager publishes for the panel body.
  rootArea->addChild(ui::box({.out = &m_scrim, .radius = Style::scaledRadiusXl(scale)}));

  auto clock = ui::column(
      {.align = FlexAlign::Center},
      ui::label({
          .out = &m_time,
          .fontSize = kClockFontSize * scale,
          .fontWeight = FontWeight::Light,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxLines = 1,
      }),
      ui::label({
          .out = &m_date,
          .fontSize = Style::fontSizeHeader * scale,
          .fontWeight = FontWeight::Medium,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxLines = 1,
      })
  );

  // Wraps into rows of at most kMaxColumns: doLayout pins the width to that many tiles.
  auto grid = ui::row({
      .out = &m_tileGrid,
      .align = FlexAlign::Start,
      .justify = FlexJustify::Center,
      .wrap = true,
      .gap = kTileGap * scale,
      .widthPolicy = FlexSizePolicy::Fill,
  });

  rootArea->addChild(
      ui::column(
          {
              .out = &m_rootLayout,
              .align = FlexAlign::Center,
              .justify = FlexJustify::Center,
              .gap = kClockToTilesGap * scale,
          },
          std::move(clock), std::move(grid)
      )
  );

  buildTiles(scale);
  setRoot(std::move(rootArea));
  if (m_animations != nullptr) {
    root()->setAnimationManager(m_animations);
  }

  m_paletteConn = paletteChanged().connect([this]() {
    // A theme switch only redraws the open panel, and the rim and sheen depend on light/dark rather than on
    // palette roles, so restyle them here instead of waiting for the next layout.
    if (isResolvedLightTheme() != m_glassLightTheme) {
      applyChrome();
      PanelManager::instance().requestRedraw();
    }
  });
  applyChrome();
}

void StatusOverlayPanel::onOpen(std::string_view /*context*/) {
  (void)refreshAll();
  m_refreshTimer.startRepeating(std::chrono::milliseconds(1000), [this]() {
    if (refreshAll()) {
      PanelManager::instance().requestLayout();
      PanelManager::instance().requestRedraw();
    }
  });
}

void StatusOverlayPanel::onClose() {
  m_refreshTimer.stop();
  m_paletteConn.disconnect();
  m_scrim = nullptr;
  m_rootLayout = nullptr;
  m_time = nullptr;
  m_date = nullptr;
  m_tileGrid = nullptr;
  m_tiles.clear();
  clearReleasedRoot();
}

bool StatusOverlayPanel::handleGlobalKey(std::uint32_t sym, std::uint32_t modifiers, bool pressed, bool preedit) {
  if (!pressed || preedit || !KeybindMatcher::matches(KeybindAction::Cancel, sym, modifiers)) {
    return false;
  }
  PanelManager::instance().close();
  return true;
}

float StatusOverlayPanel::preferredWidth() const { return scaled(kFallbackWidth); }

float StatusOverlayPanel::preferredHeight() const { return scaled(kFallbackHeight); }

void StatusOverlayPanel::doLayout(Renderer& renderer, float width, float height) {
  Node* rootNode = root();
  if (rootNode == nullptr || m_rootLayout == nullptr || m_tileGrid == nullptr) {
    return;
  }
  const float scale = contentScale();

  rootNode->setSize(width, height);
  if (m_scrim != nullptr) {
    m_scrim->setPosition(0.0F, 0.0F);
    m_scrim->setSize(width, height);
  }
  applyChrome();

  std::size_t visibleTiles = 0;
  for (const Tile& tile : m_tiles) {
    if (tile.root != nullptr && tile.root->visible()) {
      ++visibleTiles;
    }
  }
  const float tileWidth = kTileWidth * scale;
  const float gap = kTileGap * scale;
  const auto fitting = static_cast<std::size_t>(std::max(1.0F, std::floor((width + gap) / (tileWidth + gap))));
  const std::size_t columns = std::max<std::size_t>(1, std::min({visibleTiles, kMaxColumns, fitting}));
  const float gridWidth = (static_cast<float>(columns) * tileWidth) + (static_cast<float>(columns - 1) * gap);
  m_tileGrid->setVisible(visibleTiles > 0);
  m_tileGrid->setMinWidth(gridWidth);
  m_tileGrid->setMaxWidth(gridWidth);

  m_rootLayout->setSize(width, height);
  m_rootLayout->layout(renderer);
}

void StatusOverlayPanel::doUpdate(Renderer& /*renderer*/) {
  // An update-only pass (no layout) can still change texts and tile visibility.
  if (refreshAll()) {
    PanelManager::instance().requestLayout();
  }
}

void StatusOverlayPanel::onPanelCardOpacityChanged(float /*opacity*/) { applyChrome(); }

void StatusOverlayPanel::applyChrome() {
  const float scale = contentScale();
  const float opacity = panelCardOpacity();
  m_glassLightTheme = isResolvedLightTheme();
  if (m_scrim != nullptr) {
    m_scrim->setFill(colorSpecFromRole(ColorRole::Surface, kScrimAlphaPerCardOpacity * opacity));
  }
  const shell::glass::GlassStyle style = tileGlassStyle(scale, opacity);
  for (Tile& tile : m_tiles) {
    shell::glass::applyGlass(tile.glass, style, m_glassLightTheme, scale);
  }
}

void StatusOverlayPanel::buildTiles(float scale) {
  static constexpr std::array kKinds{TileKind::Battery, TileKind::Network,    TileKind::Bluetooth,
                                     TileKind::Volume,  TileKind::Brightness, TileKind::Media};
  m_tiles.clear();
  m_tiles.reserve(kKinds.size());
  for (const TileKind kind : kKinds) {
    Tile& tile = m_tiles.emplace_back();
    tile.kind = kind;
    buildTile(tile, scale);
  }
}

void StatusOverlayPanel::buildTile(Tile& tile, float scale) {
  if (m_tileGrid == nullptr) {
    return;
  }
  const float tileWidth = kTileWidth * scale;
  const float tileHeight = kTileHeight * scale;
  const float padding = Style::cardPadding * scale;
  const float innerWidth = tileWidth - (2.0F * padding);
  const float glyphSize = kTileGlyphSize * scale;
  const float headerGap = Style::spaceSm * scale;
  const bool hasLevel =
      tile.kind == TileKind::Battery || tile.kind == TileKind::Volume || tile.kind == TileKind::Brightness;

  // Hidden until the first refresh finds the backing service available.
  auto tileRoot = ui::inputArea({.width = tileWidth, .height = tileHeight, .visible = false});
  tile.glass = shell::glass::buildGlass(*tileRoot, tileGlassStyle(scale, panelCardOpacity()));
  shell::glass::layoutGlass(tile.glass, 0.0F, 0.0F, tileWidth, tileHeight, scale);

  auto content = ui::column({
      .align = FlexAlign::Start,
      .gap = kLevelGap * scale,
      .padding = padding,
      .width = tileWidth,
      .height = tileHeight,
  });
  content->addChild(
      ui::row(
          {.align = FlexAlign::Center, .gap = headerGap},
          ui::glyph({
              .out = &tile.glyph,
              .glyphSize = glyphSize,
              .color = colorSpecFromRole(ColorRole::OnSurface),
          }),
          ui::label({
              .out = &tile.title,
              .fontSize = Style::fontSizeCaption * scale,
              .fontWeight = FontWeight::Medium,
              .color = colorSpecFromRole(ColorRole::OnSurfaceVariant),
              .maxWidth = innerWidth - glyphSize - headerGap,
              .maxLines = 1,
              .ellipsize = TextEllipsize::End,
          })
      )
  );
  // Pushes value and level to the bottom edge; excluded from the gaps so only value and level are spaced apart.
  Node* spacer = content->addChild(ui::spacer());
  content->setChildGapExcluded(spacer, true);
  content->addChild(
      ui::label({
          .out = &tile.value,
          .fontSize = kTileValueFontSize * scale,
          .fontWeight = FontWeight::SemiBold,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxWidth = innerWidth,
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
      })
  );
  if (hasLevel) {
    const float levelHeight = kLevelHeight * scale;
    content->addChild(
        ui::progressBar({
            .out = &tile.level,
            .fill = colorSpecFromRole(ColorRole::Primary),
            .track = colorSpecFromRole(ColorRole::OnSurface, kLevelTrackAlpha),
            .radius = levelHeight * 0.5F,
            .progress = 0.0F,
            .width = innerWidth,
            .height = levelHeight,
        })
    );
  }
  tileRoot->addChild(std::move(content));
  tile.root = m_tileGrid->addChild(std::move(tileRoot));
}

bool StatusOverlayPanel::refreshTile(Tile& tile) {
  if (tile.root == nullptr) {
    return false;
  }
  TileContent content;
  switch (tile.kind) {
  case TileKind::Battery:
    content = batteryContent(m_services.upower);
    break;
  case TileKind::Network:
    content = networkContent(m_services.network);
    break;
  case TileKind::Bluetooth:
    content = bluetoothContent(m_services.bluetooth);
    break;
  case TileKind::Volume:
    content = volumeContent(m_services.audio);
    break;
  case TileKind::Brightness:
    content = brightnessContent(m_services.brightness);
    break;
  case TileKind::Media:
    content = mediaContent(m_services.mpris);
    break;
  }

  bool changed = false;
  if (tile.root->visible() != content.available) {
    tile.root->setVisible(content.available);
    changed = true;
  }
  if (!content.available) {
    return changed;
  }
  if (tile.glyph != nullptr) {
    changed = tile.glyph->setGlyph(content.glyph) || changed;
  }
  if (tile.title != nullptr) {
    changed = tile.title->setText(content.title) || changed;
  }
  if (tile.value != nullptr) {
    changed = tile.value->setText(content.value) || changed;
    if (tile.valueDimmed != content.valueDimmed) {
      tile.valueDimmed = content.valueDimmed;
      tile.value->setColor(colorSpecFromRole(content.valueDimmed ? ColorRole::OnSurfaceVariant : ColorRole::OnSurface));
      changed = true;
    }
  }
  if (tile.level != nullptr) {
    const float level = std::clamp(content.level, 0.0F, 1.0F);
    if (std::abs(tile.level->progress() - level) > 0.001F) {
      tile.level->setProgress(level);
      changed = true;
    }
    if (tile.levelNeutral != content.levelNeutral) {
      tile.levelNeutral = content.levelNeutral;
      tile.level->setFill(
          content.levelNeutral ? colorSpecFromRole(ColorRole::OnSurface, kLevelNeutralAlpha)
                               : colorSpecFromRole(ColorRole::Primary)
      );
      changed = true;
    }
  }
  return changed;
}

bool StatusOverlayPanel::refreshClock() {
  bool changed = false;
  if (m_time != nullptr) {
    const char* timeFormat = kDefaultTimeFormat;
    if (m_services.config != nullptr && !m_services.config->config().shell.timeFormat.empty()) {
      timeFormat = m_services.config->config().shell.timeFormat.c_str();
    }
    changed = m_time->setText(formatLocalTime(timeFormat)) || changed;
  }
  if (m_date != nullptr) {
    // A long date ("Sunday, 11 October"); the pattern is translated because day/month order and punctuation
    // differ per language.
    changed = m_date->setText(formatLocalTime(i18n::tr("status-overlay.date-format").c_str())) || changed;
  }
  return changed;
}

bool StatusOverlayPanel::refreshAll() {
  bool changed = refreshClock();
  for (Tile& tile : m_tiles) {
    changed = refreshTile(tile) || changed;
  }
  return changed;
}
