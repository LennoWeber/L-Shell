#include "shell/expo/expo.h"

#include "capture/toplevel_thumbnail_capture.h"
#include "compositors/compositor_platform.h"
#include "config/config_service.h"
#include "core/deferred_call.h"
#include "core/input/key_modifiers.h"
#include "core/input/keybind_matcher.h"
#include "core/log.h"
#include "core/ui_phase.h"
#include "cursor-shape-v1-client-protocol.h"
#include "i18n/i18n.h"
#include "ipc/ipc_service.h"
#include "render/animation/animation_manager.h"
#include "render/animation/motion_service.h"
#include "render/core/renderer.h"
#include "render/render_context.h"
#include "render/scene/input_area.h"
#include "render/scene/input_dispatcher.h"
#include "render/scene/node.h"
#include "shell/expo/expo_window_tile.h"
#include "shell/surface/material.h"
#include "shell/switcher/window_switcher_entries.h"
#include "shell/switcher/window_switcher_membership.h"
#include "shell/wallpaper/wallpaper_paths.h"
#include "ui/app_icon_colorization.h"
#include "ui/builders.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "util/string_utils.h"
#include "wayland/layer_surface.h"
#include "wayland/wayland_connection.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <linux/input-event-codes.h>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace {

  constexpr Logger kLog("expo");

  // Same frosted, dimmed desktop as the window switcher's overview.
  constexpr float kDimOpacity = 0.7F;
  // Closing is quicker than opening and accelerates away.
  constexpr float kCloseMs = 170.0F;
  constexpr int kThumbnailMaxWidth = 640;
  constexpr int kThumbnailMaxHeight = 400;
  // Resolve icon theme entries large enough for the icon drawn on a snapshot-less tile.
  constexpr float kIconLookupSize = 96.0F;
  constexpr float kCardTintOpacity = 0.4F;
  constexpr float kInactiveCaptionAlpha = 0.72F;
  constexpr float kHeaderAlpha = 0.6F;
  constexpr float kActiveRingAlpha = 0.85F;
  constexpr float kActiveRingWidth = 1.5F;
  constexpr float kSelectedRingWidth = 3.0F;
  // Clear of a selected window's ring (2 px) plus its growth (3 px) when the window fills the desktop.
  constexpr float kRingOutset = 6.0F;
  constexpr float kTitlePillAlpha = 0.92F;
  constexpr float kTitleMaxWidth = 360.0F;
  constexpr float kTitlePaddingH = 10.0F;
  constexpr float kTitlePaddingV = 4.0F;
  constexpr float kTitleGap = 8.0F;
  constexpr int kWallpaperSizeSmall = 1024;
  constexpr int kWallpaperSizeLarge = 2048;

  using switcher_entries::activateWindowSwitcherEntry;
  using switcher_entries::applyLiveToplevel;
  using switcher_entries::canonicalWindowId;
  using switcher_entries::currentFocusedWindowKey;
  using switcher_entries::identityKeysEqual;
  using switcher_entries::indexLiveToplevelsByIdentity;
  using switcher_entries::makeEntryFromAssignment;
  using switcher_entries::makeEntryFromToplevel;

  [[nodiscard]] float shellUiScale(const ConfigService* config) noexcept {
    return config != nullptr ? config->config().accessibility.uiScale : 1.0F;
  }

  [[nodiscard]] const WaylandOutput* findOutput(const WaylandConnection& wayland, wl_output* output) {
    for (const auto& entry : wayland.outputs()) {
      if (entry.output == output) {
        return &entry;
      }
    }
    return nullptr;
  }

  [[nodiscard]] bool isDigits(std::string_view text) {
    return !text.empty() && std::ranges::all_of(text, [](unsigned char c) { return std::isdigit(c) != 0; });
  }

  // "DP-1 · Dell U2720Q": the connector plus whatever names the monitor.
  [[nodiscard]] std::string outputTitle(const WaylandOutput& output) {
    std::string detail;
    if (!output.make.empty() || !output.model.empty()) {
      detail = StringUtils::trim(output.make + " " + output.model);
    } else {
      detail = output.description;
    }
    // wl_output descriptions usually end with the connector in parentheses.
    if (!output.connectorName.empty()) {
      const std::string suffix = " (" + output.connectorName + ")";
      if (detail.ends_with(suffix)) {
        detail.resize(detail.size() - suffix.size());
      }
    }
    if (output.connectorName.empty()) {
      return detail;
    }
    if (detail.empty() || detail == output.connectorName) {
      return output.connectorName;
    }
    return output.connectorName + " · " + detail;
  }

  // Numbered workspaces read as "Desktop 2" like macOS; named ones keep their name.
  [[nodiscard]] std::string workspaceCaption(const Workspace& workspace, std::size_t ordinal) {
    std::string label = workspace.name;
    if (label.empty()) {
      label = workspace.index > 0 ? std::to_string(workspace.index) : std::to_string(ordinal + 1);
    }
    return isDigits(label) ? i18n::tr("expo.desktop", "name", label) : label;
  }

  [[nodiscard]] std::string wallpaperPathFor(const ConfigService* config, const WaylandOutput& output) {
    if (config == nullptr) {
      return {};
    }
    const WallpaperConfig& wallpaper = config->config().wallpaper;
    bool enabled = wallpaper.enabled;
    if (const auto* monitorOverride = wallpaper::findWallpaperMonitorOverride(wallpaper, output);
        monitorOverride != nullptr && monitorOverride->enabled.has_value()) {
      enabled = enabled && *monitorOverride->enabled;
    }
    return enabled ? config->getWallpaperPath(output.connectorName) : std::string{};
  }

} // namespace

struct Expo::Instance {
  wl_output* output = nullptr;
  float scale = 1.0F;
  float iconPixelScale = 1.0F;
  std::unique_ptr<LayerSurface> surface;
  AnimationManager animations;
  std::unique_ptr<Node> sceneRoot;
  InputDispatcher inputDispatcher;
  InputArea* input = nullptr;
  Box* dimmer = nullptr;
  Node* content = nullptr;
  Box* titlePill = nullptr;
  Label* titleLabel = nullptr;
  // Indexed like Expo::m_cards / Expo::m_windows; rects in surface coordinates.
  std::vector<ExpoRect> cardRects;
  std::vector<Box*> cardRings;
  std::vector<ExpoWindowTile*> tiles;
  std::vector<ExpoRect> windowRects;
  std::uint32_t builtWidth = 0;
  std::uint32_t builtHeight = 0;
  bool sceneDirty = true;
  bool contentDirty = false;
  bool titleDirty = false;
  bool pointerInside = false;
  float revealProgress = 0.0F;
};

struct Expo::Catcher {
  wl_output* output = nullptr;
  std::unique_ptr<LayerSurface> surface;
  AnimationManager animations;
  std::unique_ptr<Node> root;
  Box* dimmer = nullptr;
  std::uint32_t builtWidth = 0;
  std::uint32_t builtHeight = 0;
  bool revealed = false;

  Catcher() = default;
  Catcher(const Catcher&) = delete;
  Catcher& operator=(const Catcher&) = delete;
  ~Catcher() {
    animations.cancelAll();
    if (surface != nullptr) {
      surface->setPrepareFrameCallback(nullptr);
      surface->setConfigureCallback(nullptr);
      surface->setSceneRoot(nullptr);
    }
  }
};

Expo::Expo() = default;

Expo::~Expo() {
  cancelThumbnailCaptures();
  destroyCatchers();
  destroySurface();
}

void Expo::initialize(
    WaylandConnection& wayland, RenderContext* renderContext, CompositorPlatform& platform, ConfigService* config,
    AsyncTextureCache* asyncTextures
) {
  m_wayland = &wayland;
  m_renderContext = renderContext;
  m_platform = &platform;
  m_config = config;
  m_asyncTextures = asyncTextures;
  m_thumbnailCapture = std::make_unique<ToplevelThumbnailCapture>(wayland);
}

void Expo::registerIpc(IpcService& ipc) {
  ipc.bind(noctalia::cli::msg::expo, [this](const std::string& args) -> std::string {
    const std::string token = StringUtils::trim(args);
    if (token == "close") {
      if (m_active) {
        close();
      }
      return "ok\n";
    }
    if (!token.empty() && token != "toggle") {
      return "error: unknown expo action '" + token + "'\n";
    }
    if (m_platform == nullptr) {
      return "error: compositor unavailable\n";
    }
    wl_output* output = m_platform->preferredInteractiveOutput();
    if (output == nullptr && m_wayland != nullptr && !m_wayland->outputs().empty()) {
      output = m_wayland->outputs().front().output;
    }
    if (output == nullptr) {
      return "error: no output available\n";
    }
    toggle(output);
    return "ok\n";
  });
}

void Expo::onConfigReload() {
  if (!m_active) {
    return;
  }
  refreshModel();
  if (m_instance != nullptr) {
    m_instance->scale = shellUiScale(m_config);
    m_instance->sceneDirty = true;
  }
  requestSceneUpdate();
}

void Expo::onOutputChange() {
  if (!m_active) {
    return;
  }
  const WaylandOutput* output = m_wayland != nullptr ? findOutput(*m_wayland, m_output) : nullptr;
  if (output == nullptr || !output->hasUsableGeometry()) {
    close();
    return;
  }
  // Another monitor came or went: it gets a catcher, and its desktops join or leave the expo.
  syncCatchers();
  scheduleRefresh();
}

void Expo::onWindowsChanged() { scheduleRefresh(); }

void Expo::scheduleRefresh() {
  // Toplevel and workspace callbacks often fire together (and terminals retitle constantly):
  // one refresh per loop iteration is enough. Deferring also keeps a refresh from nesting in
  // another one when reading the model makes a backend re-sync and notify.
  if (!m_active || m_refreshScheduled) {
    return;
  }
  m_refreshScheduled = true;
  DeferredCall::callLater([this, session = m_session]() {
    if (m_session != session) {
      return;
    }
    m_refreshScheduled = false;
    applyRefresh();
  });
}

void Expo::applyRefresh() {
  if (!m_active) {
    return;
  }
  const std::string before = structureSignature();
  refreshModel();
  if (m_instance != nullptr) {
    if (structureSignature() != before) {
      m_instance->sceneDirty = true;
    } else {
      m_instance->contentDirty = true;
      m_instance->titleDirty = true;
    }
  }
  startThumbnailCaptures();
  requestSceneUpdate();
}

void Expo::toggle(wl_output* output) {
  if (m_active) {
    close();
    return;
  }
  open(output);
}

void Expo::open(wl_output* output) {
  if (m_wayland == nullptr || m_renderContext == nullptr || m_platform == nullptr || output == nullptr) {
    return;
  }
  if (m_onOpen) {
    m_onOpen();
  }
  // Reopened while animating closed: keep the surface (same output) and turn the reveal around.
  const bool resumingClose = m_closing && m_instance != nullptr && m_instance->output == output;
  if (m_closing && !resumingClose) {
    closeNow();
  }
  m_closing = false;
  ++m_session;
  m_output = output;
  m_refreshScheduled = false;
  m_failedCaptureKeys.clear();
  m_hoverSelects = false;
  m_hasPointerPosition = false;
  // Hyprland reports no resize events, so the cached window geometry may be stale.
  m_platform->refreshWorkspaceWindowGeometry();
  m_windows.clear();
  m_cards.clear();
  m_sections.clear();
  m_selectedWindow.reset();
  m_selectedCard = 0;
  refreshModel();

  // Start on the focused window like Mission Control, else on the desktop in front of the user.
  const std::string focusedKey = currentFocusedWindowKey(*m_platform);
  std::optional<std::size_t> focused = focusedKey.empty() ? std::nullopt : windowIndexForKey(focusedKey);
  if (!focused.has_value()) {
    // Compositors without exact window ids (sway): match the active foreign-toplevel handle.
    if (const auto active = m_platform->activeToplevel(); active.has_value() && active->handle != nullptr) {
      const auto handle = reinterpret_cast<std::uintptr_t>(active->handle);
      for (std::size_t i = 0; i < m_windows.size() && !focused.has_value(); ++i) {
        if (m_windows[i].entry.closeHandle == handle) {
          focused = i;
        }
      }
    }
  }
  if (focused.has_value()) {
    m_selectedWindow = focused;
    m_selectedCard = m_windows[*focused].card;
  } else {
    for (std::size_t i = 0; i < m_cards.size(); ++i) {
      if (m_cards[i].output == output && m_cards[i].workspace.active) {
        m_selectedCard = i;
        break;
      }
    }
  }

  m_active = true;
  ensureSurface();
  if (m_instance == nullptr) {
    closeNow();
    return;
  }
  if (resumingClose) {
    m_instance->animations.cancelAll();
    m_instance->surface->setKeyboardInteractivity(LayerShellKeyboard::Exclusive);
    m_instance->surface->setClickThrough(false);
    // The rebuild resumes the reveal from where the close left it.
    m_instance->sceneDirty = true;
    for (const auto& catcher : m_catchers) {
      fadeCatcher(*catcher, kDimOpacity);
    }
  }
  syncCatchers();
  requestSceneUpdate();
  startThumbnailCaptures();
}

void Expo::close() {
  if (m_closing || (!m_active && m_instance == nullptr)) {
    return;
  }
  Instance* instance = m_instance.get();
  if (!m_active
      || instance == nullptr
      || instance->surface == nullptr
      || instance->sceneRoot == nullptr
      || instance->content == nullptr
      || instance->dimmer == nullptr) {
    closeNow();
    return;
  }

  // Reverse of the open: fade and shrink back, then go. The session is over at once: input is
  // ignored, the keyboard and clicks go back to the apps, and model work stops.
  m_active = false;
  m_closing = true;
  m_refreshScheduled = false;
  cancelThumbnailCaptures();
  m_failedCaptureKeys.clear();
  instance->inputDispatcher.setSceneRoot(nullptr);
  instance->pointerInside = false;
  instance->surface->setKeyboardInteractivity(LayerShellKeyboard::None);
  instance->surface->setClickThrough(true);
  for (const auto& catcher : m_catchers) {
    fadeCatcher(*catcher, 0.0F);
  }
  instance->animations.cancelAll();
  const float from = instance->revealProgress;
  instance->animations.animateFade(
      from, 0.0F, kCloseMs * from, Easing::EaseInCubic, revealSetter(*instance),
      [this, session = m_session]() {
        DeferredCall::callLater([this, session]() {
          if (m_closing && m_session == session) {
            closeNow();
          }
        });
      },
      instance->content
  );
  instance->surface->requestRedraw();
}

void Expo::closeNow() {
  m_closing = false;
  m_active = false;
  m_refreshScheduled = false;
  cancelThumbnailCaptures();
  m_failedCaptureKeys.clear();
  destroyCatchers();
  destroySurface();
  m_windows.clear();
  m_cards.clear();
  m_sections.clear();
  m_selectedWindow.reset();
  m_selectedCard = 0;
  m_output = nullptr;
}

void Expo::refreshModel() {
  if (m_refreshing) {
    scheduleRefresh();
    return;
  }
  m_refreshing = true;
  struct Reset {
    bool& flag;
    ~Reset() { flag = false; }
  } reset{m_refreshing};

  // Snapshots and the selection survive a refresh, matched by window identity.
  std::unordered_map<std::string, std::shared_ptr<const ScreencopyImage>> thumbnails;
  for (const auto& window : m_windows) {
    if (window.entry.thumbnail != nullptr) {
      thumbnails.emplace(window.entry.identityKey, window.entry.thumbnail);
    }
  }
  std::string selectedKey;
  if (m_selectedWindow.has_value() && *m_selectedWindow < m_windows.size()) {
    selectedKey = m_windows[*m_selectedWindow].entry.identityKey;
  }
  wl_output* selectedCardOutput = nullptr;
  std::string selectedCardId;
  if (m_selectedCard < m_cards.size()) {
    selectedCardOutput = m_cards[m_selectedCard].output;
    selectedCardId = m_cards[m_selectedCard].workspace.id;
  }

  m_sections.clear();
  m_cards.clear();
  m_windows.clear();
  if (m_platform == nullptr || m_wayland == nullptr) {
    m_selectedWindow.reset();
    m_selectedCard = 0;
    return;
  }

  // Sections run left to right like the monitors on the desk.
  // Copies: nothing below may keep pointing into the connection's output list.
  std::vector<WaylandOutput> outputs;
  for (const auto& output : m_wayland->outputs()) {
    if (output.output != nullptr && output.hasUsableGeometry()) {
      outputs.push_back(output);
    }
  }
  std::ranges::stable_sort(outputs, [](const WaylandOutput& a, const WaylandOutput& b) {
    if (a.logicalX != b.logicalX) {
      return a.logicalX < b.logicalX;
    }
    return a.logicalY < b.logicalY;
  });

  struct OutputModel {
    const WaylandOutput* output = nullptr;
    std::vector<Workspace> workspaces;
    std::vector<WorkspaceWindowAssignment> assignments;
  };
  std::vector<OutputModel> models;
  models.reserve(outputs.size());
  bool anyAssignments = false;
  for (const WaylandOutput& output : outputs) {
    OutputModel model{
        .output = &output,
        .workspaces = m_platform->workspaces(output.output),
        .assignments = m_platform->workspaceWindowAssignments(output.output),
    };
    anyAssignments = anyAssignments || !model.assignments.empty();
    models.push_back(std::move(model));
  }

  const int iconSize = static_cast<int>(std::round(kIconLookupSize * shellUiScale(m_config)));
  std::unordered_set<std::string> seenKeys;

  for (const OutputModel& model : models) {
    const WaylandOutput& output = *model.output;
    std::unordered_map<std::string, ToplevelInfo> live;
    indexLiveToplevelsByIdentity(*m_platform, output.output, live);

    // Windows per workspace of this output; without a workspace model a single stand-in desktop.
    const bool hasWorkspaces = !model.workspaces.empty();
    const std::size_t slotCount = hasWorkspaces ? model.workspaces.size() : 1;
    std::vector<std::vector<Window>> slots(slotCount);

    for (const auto& assignment : model.assignments) {
      std::string key = canonicalWindowId(assignment.windowId);
      // Inactive tabs of a window group sit under the visible one: only that one is shown and clicked.
      if (key.empty() || seenKeys.contains(key) || !hasWorkspaces || assignment.hidden) {
        continue;
      }
      const auto workspace = std::ranges::find_if(model.workspaces, [&assignment](const Workspace& candidate) {
        return switcher_membership::workspaceKeyMatchesAssignment(assignment.workspaceKey, candidate);
      });
      // Hyprland special workspaces and the like are not desktops of their own here.
      if (workspace == model.workspaces.end()) {
        continue;
      }
      seenKeys.insert(key);

      Window window;
      window.entry = makeEntryFromAssignment(*m_platform, *m_wayland, m_iconResolver, iconSize, assignment);
      if (const auto liveIt = live.find(key); liveIt != live.end()) {
        applyLiveToplevel(window.entry, liveIt->second);
      }
      window.entry.identityKey = std::move(key);
      if (assignment.width > 0 && assignment.height > 0) {
        window.geometry = ExpoWindowGeometry{
            .x = static_cast<float>(assignment.x - output.logicalX),
            .y = static_cast<float>(assignment.y - output.logicalY),
            .width = static_cast<float>(assignment.width),
            .height = static_cast<float>(assignment.height),
        };
      }
      slots[static_cast<std::size_t>(std::distance(model.workspaces.begin(), workspace))].push_back(std::move(window));
    }

    // No backend tells which workspace a window is on: show this output's windows on its current desktop.
    if (!anyAssignments || !hasWorkspaces) {
      std::size_t target = 0;
      for (std::size_t i = 0; i < model.workspaces.size(); ++i) {
        if (model.workspaces[i].active) {
          target = i;
          break;
        }
      }
      for (const auto& [key, info] : live) {
        if (!seenKeys.insert(key).second) {
          continue;
        }
        Window window;
        window.entry = makeEntryFromToplevel(*m_platform, m_iconResolver, iconSize, info.appId, info);
        window.entry.identityKey = key;
        slots[target].push_back(std::move(window));
      }
    }

    Section section{
        .output = output.output,
        .title = outputTitle(output),
        .wallpaperPath = wallpaperPathFor(m_config, output),
        .logicalWidth = static_cast<float>(output.effectiveLogicalWidth()),
        .logicalHeight = static_cast<float>(output.effectiveLogicalHeight()),
        .cards = {},
    };
    const std::size_t sectionIndex = m_sections.size();
    for (std::size_t slot = 0; slot < slotCount; ++slot) {
      Card card;
      card.output = output.output;
      card.section = sectionIndex;
      if (hasWorkspaces) {
        card.workspace = model.workspaces[slot];
        // Empty desktops in the background are left out; the one on screen always shows.
        if (!card.workspace.active && slots[slot].empty()) {
          continue;
        }
        card.caption = workspaceCaption(card.workspace, slot);
      } else {
        card.hasWorkspace = false;
        card.workspace.active = true;
        card.caption = i18n::tr("expo.desktop", "name", "1");
      }

      std::vector<Window>& windows = slots[slot];
      std::ranges::stable_sort(windows, [](const Window& a, const Window& b) {
        if (a.geometry.y != b.geometry.y) {
          return a.geometry.y < b.geometry.y;
        }
        return a.geometry.x < b.geometry.x;
      });
      const std::size_t cardIndex = m_cards.size();
      for (Window& window : windows) {
        window.card = cardIndex;
        if (const auto thumbnail = thumbnails.find(window.entry.identityKey); thumbnail != thumbnails.end()) {
          window.entry.thumbnail = thumbnail->second;
        }
        card.windows.push_back(m_windows.size());
        m_windows.push_back(std::move(window));
      }
      section.cards.push_back(cardIndex);
      m_cards.push_back(std::move(card));
    }
    if (!section.cards.empty()) {
      m_sections.push_back(std::move(section));
    }
  }

  m_selectedWindow.reset();
  if (!selectedKey.empty()) {
    m_selectedWindow = windowIndexForKey(selectedKey);
  }
  if (m_selectedWindow.has_value()) {
    m_selectedCard = m_windows[*m_selectedWindow].card;
    return;
  }
  for (std::size_t i = 0; i < m_cards.size(); ++i) {
    if (m_cards[i].output == selectedCardOutput && m_cards[i].workspace.id == selectedCardId) {
      m_selectedCard = i;
      return;
    }
  }
  m_selectedCard = m_cards.empty() ? 0 : std::min(m_selectedCard, m_cards.size() - 1);
}

std::string Expo::structureSignature() const {
  std::string signature;
  for (const Section& section : m_sections) {
    signature += std::format(
        "S{}|{}|{}|{}x{}\n", static_cast<const void*>(section.output), section.title, section.wallpaperPath,
        section.logicalWidth, section.logicalHeight
    );
  }
  for (const Card& card : m_cards) {
    signature += std::format(
        "C{}|{}|{}|{}\n", static_cast<const void*>(card.output), card.workspace.id, card.workspace.active ? 1 : 0,
        card.caption
    );
    for (const std::size_t index : card.windows) {
      const Window& window = m_windows[index];
      signature += std::format(
          " W{}|{},{},{},{}\n", window.entry.identityKey, window.geometry.x, window.geometry.y, window.geometry.width,
          window.geometry.height
      );
    }
  }
  return signature;
}

std::optional<std::size_t> Expo::windowIndexForKey(std::string_view key) const {
  for (std::size_t i = 0; i < m_windows.size(); ++i) {
    if (identityKeysEqual(m_windows[i].entry.identityKey, key)) {
      return i;
    }
  }
  return std::nullopt;
}

void Expo::ensureSurface() {
  if (m_wayland == nullptr || m_renderContext == nullptr || m_output == nullptr) {
    return;
  }
  const WaylandOutput* output = findOutput(*m_wayland, m_output);
  if (output == nullptr || !output->hasUsableGeometry()) {
    return;
  }
  if (m_instance != nullptr && m_instance->output == m_output && m_instance->surface != nullptr) {
    return;
  }
  destroySurface();

  auto instance = std::make_unique<Instance>();
  instance->output = m_output;
  instance->scale = shellUiScale(m_config);
  instance->iconPixelScale = std::max(1.0F, std::ceil(output->configuredScale()));

  auto config = LayerSurfaceConfig{
      .nameSpace = "noctalia-expo",
      .layer = LayerShellLayer::Overlay,
      .anchor = LayerShellAnchor::Top | LayerShellAnchor::Bottom | LayerShellAnchor::Left | LayerShellAnchor::Right,
      .width = 0,
      .height = 0,
      .exclusiveZone = -1,
      .keyboard = LayerShellKeyboard::Exclusive,
      .defaultWidth = static_cast<std::uint32_t>(output->effectiveLogicalWidth()),
      .defaultHeight = static_cast<std::uint32_t>(output->effectiveLogicalHeight()),
  };
  instance->surface = std::make_unique<LayerSurface>(*m_wayland, std::move(config));
  instance->surface->setRenderContext(m_renderContext);
  instance->surface->setAnimationManager(&instance->animations);

  Instance* instancePtr = instance.get();
  instance->surface->setConfigureCallback([instancePtr](std::uint32_t /*width*/, std::uint32_t /*height*/) {
    if (instancePtr->surface != nullptr) {
      instancePtr->surface->requestLayout();
    }
  });
  instance->surface->setPrepareFrameCallback([this, instancePtr](bool /*needsUpdate*/, bool /*needsLayout*/) {
    if (!m_active || m_instance.get() != instancePtr) {
      return;
    }
    prepareFrame(*instancePtr);
  });
  instance->surface->setClosedCallback([this, session = m_session]() {
    DeferredCall::callLater([this, session]() {
      // Only the session whose surface the compositor closed, not one opened since.
      if (m_session == session) {
        closeNow();
      }
    });
  });

  if (!instance->surface->initialize(m_output)) {
    kLog.warn("failed to initialize expo overlay");
    return;
  }
  m_instance = std::move(instance);
}

void Expo::destroySurface() {
  if (m_instance == nullptr) {
    return;
  }
  m_instance->inputDispatcher.setSceneRoot(nullptr);
  m_instance->animations.cancelAll();
  if (m_instance->surface != nullptr) {
    m_instance->surface->setClosedCallback(nullptr);
    m_instance->surface->setPrepareFrameCallback(nullptr);
    m_instance->surface->setConfigureCallback(nullptr);
    m_instance->surface->setSceneRoot(nullptr);
  }
  m_instance.reset();
}

void Expo::syncCatchers() {
  if (!m_active || m_wayland == nullptr || m_renderContext == nullptr) {
    destroyCatchers();
    return;
  }
  std::erase_if(m_catchers, [this](const std::unique_ptr<Catcher>& catcher) {
    const WaylandOutput* output = findOutput(*m_wayland, catcher->output);
    return output == nullptr || !output->hasUsableGeometry() || catcher->output == m_output;
  });
  for (const WaylandOutput& output : m_wayland->outputs()) {
    if (output.output == nullptr || output.output == m_output || !output.hasUsableGeometry()) {
      continue;
    }
    if (std::ranges::any_of(m_catchers, [&output](const auto& catcher) { return catcher->output == output.output; })) {
      continue;
    }
    auto catcher = std::make_unique<Catcher>();
    catcher->output = output.output;
    auto config = LayerSurfaceConfig{
        .nameSpace = "noctalia-expo",
        .layer = LayerShellLayer::Overlay,
        .anchor = LayerShellAnchor::Top | LayerShellAnchor::Bottom | LayerShellAnchor::Left | LayerShellAnchor::Right,
        .width = 0,
        .height = 0,
        .exclusiveZone = -1,
        .keyboard = LayerShellKeyboard::None,
        .defaultWidth = static_cast<std::uint32_t>(output.effectiveLogicalWidth()),
        .defaultHeight = static_cast<std::uint32_t>(output.effectiveLogicalHeight()),
    };
    catcher->surface = std::make_unique<LayerSurface>(*m_wayland, std::move(config));
    catcher->surface->setRenderContext(m_renderContext);
    catcher->surface->setAnimationManager(&catcher->animations);
    Catcher* catcherPtr = catcher.get();
    catcher->surface->setConfigureCallback([catcherPtr](std::uint32_t /*width*/, std::uint32_t /*height*/) {
      if (catcherPtr->surface != nullptr) {
        catcherPtr->surface->requestLayout();
      }
    });
    catcher->surface->setPrepareFrameCallback([this, catcherPtr](bool /*needsUpdate*/, bool /*needsLayout*/) {
      prepareCatcherFrame(*catcherPtr);
    });
    if (!catcher->surface->initialize(output.output)) {
      kLog.warn("failed to initialize expo backdrop on {}", output.connectorName);
      continue;
    }
    m_catchers.push_back(std::move(catcher));
  }
}

void Expo::destroyCatchers() { m_catchers.clear(); }

std::function<void(float)> Expo::revealSetter(Instance& instance) {
  return [&instance, contentNode = instance.content, dimmer = instance.dimmer](float value) {
    instance.revealProgress = value;
    contentNode->setOpacity(value);
    // Reduce Motion: the expo only fades.
    const float scale = MotionService::instance().reduced()
        ? 1.0F
        : Style::windowSwitcherRevealScale + ((1.0F - Style::windowSwitcherRevealScale) * value);
    contentNode->setScale(scale);
    dimmer->setOpacity(kDimOpacity * value);
  };
}

void Expo::fadeCatcher(Catcher& catcher, float targetOpacity) {
  catcher.animations.cancelAll();
  if (catcher.surface != nullptr) {
    // Dismissing: clicks on the other outputs reach the apps again right away.
    catcher.surface->setClickThrough(targetOpacity <= 0.0F);
    catcher.surface->requestRedraw();
  }
  if (catcher.dimmer == nullptr) {
    return;
  }
  const float from = catcher.dimmer->opacity();
  const float durationMs = targetOpacity > from ? static_cast<float>(Style::animNormal) : kCloseMs;
  catcher.animations.animateFade(
      from, targetOpacity, durationMs, targetOpacity > from ? Easing::EaseOutCubic : Easing::EaseInCubic,
      [dimmer = catcher.dimmer](float value) { dimmer->setOpacity(value); }, {}, catcher.dimmer
  );
}

void Expo::prepareCatcherFrame(Catcher& catcher) {
  if (!m_active || m_renderContext == nullptr || catcher.surface == nullptr) {
    return;
  }
  const std::uint32_t width = catcher.surface->width();
  const std::uint32_t height = catcher.surface->height();
  if (width == 0
      || height == 0
      || (catcher.root != nullptr && width == catcher.builtWidth && height == catcher.builtHeight)) {
    return;
  }
  m_renderContext->makeCurrent(catcher.surface->renderTarget());
  Renderer& renderer = catcher.surface->renderTarget().renderer();
  UiPhaseScope layoutPhase(UiPhase::Layout);

  const auto w = static_cast<float>(width);
  const auto h = static_cast<float>(height);
  catcher.surface->setSceneRoot(nullptr);
  catcher.root = ui::node({});
  catcher.root->setAnimationManager(&catcher.animations);
  catcher.root->setSize(w, h);
  catcher.root->addChild(
      ui::box({
          .out = &catcher.dimmer,
          .fill = colorSpecFromRole(ColorRole::Shadow),
          .width = w,
          .height = h,
          .opacity = catcher.revealed ? kDimOpacity : 0.0F,
          .participatesInLayout = false,
      })
  );
  catcher.root->layout(renderer);
  catcher.builtWidth = width;
  catcher.builtHeight = height;
  catcher.surface->setBlurRegion({InputRect{0, 0, static_cast<int>(width), static_cast<int>(height)}});
  catcher.surface->setSceneRoot(catcher.root.get());
  if (!catcher.revealed) {
    // Fades in with the expo, then nothing runs per frame.
    catcher.revealed = true;
    catcher.animations.animateFade(
        0.0F, kDimOpacity, Style::animNormal, Easing::EaseOutCubic,
        [dimmer = catcher.dimmer](float value) { dimmer->setOpacity(value); }, {}, catcher.dimmer
    );
  }
}

Expo::Catcher* Expo::catcherForSurface(const wl_surface* surface) const {
  if (surface == nullptr) {
    return nullptr;
  }
  for (const auto& catcher : m_catchers) {
    if (catcher->surface != nullptr && catcher->surface->wlSurface() == surface) {
      return catcher.get();
    }
  }
  return nullptr;
}

void Expo::requestSceneUpdate() {
  if (m_instance != nullptr && m_instance->surface != nullptr) {
    m_instance->surface->requestLayout();
    m_instance->surface->requestRedraw();
  }
}

void Expo::prepareFrame(Instance& instance) {
  if (!m_active || m_renderContext == nullptr || instance.surface == nullptr) {
    return;
  }
  const std::uint32_t width = instance.surface->width();
  const std::uint32_t height = instance.surface->height();
  if (width == 0 || height == 0) {
    return;
  }
  m_renderContext->makeCurrent(instance.surface->renderTarget());
  Renderer& renderer = instance.surface->renderTarget().renderer();

  if (instance.sceneRoot == nullptr
      || instance.sceneDirty
      || width != instance.builtWidth
      || height != instance.builtHeight) {
    buildScene(instance, width, height);
  } else if (instance.contentDirty) {
    rebindTiles(instance);
  }
  if (instance.titleDirty) {
    positionTitlePill(instance);
  }
  if (instance.sceneRoot != nullptr && instance.sceneRoot->layoutDirty()) {
    UiPhaseScope layoutPhase(UiPhase::Layout);
    instance.sceneRoot->layout(renderer);
  }
  if (instance.pointerInside) {
    instance.inputDispatcher.syncPointerHover();
  }
}

void Expo::rebindTiles(Instance& instance) {
  Renderer& renderer = instance.surface->renderTarget().renderer();
  for (std::size_t i = 0; i < instance.tiles.size() && i < m_windows.size(); ++i) {
    if (instance.tiles[i] != nullptr) {
      instance.tiles[i]->bind(renderer, m_windows[i].entry);
    }
  }
  instance.contentDirty = false;
}

void Expo::buildScene(Instance& instance, std::uint32_t width, std::uint32_t height) {
  UiPhaseScope layoutPhase(UiPhase::Layout);

  const auto w = static_cast<float>(width);
  const auto h = static_cast<float>(height);
  const float s = instance.scale;
  Renderer& renderer = instance.surface->renderTarget().renderer();

  // The dispatcher points into the old tree; detach it before the tree goes.
  instance.inputDispatcher.setSceneRoot(nullptr);
  instance.surface->setSceneRoot(nullptr);
  instance.input = nullptr;
  instance.dimmer = nullptr;
  instance.content = nullptr;
  instance.titlePill = nullptr;
  instance.titleLabel = nullptr;
  instance.cardRects.assign(m_cards.size(), ExpoRect{});
  instance.cardRings.assign(m_cards.size(), nullptr);
  instance.tiles.assign(m_windows.size(), nullptr);
  instance.windowRects.assign(m_windows.size(), ExpoRect{});

  instance.sceneRoot = ui::node({});
  instance.sceneRoot->setAnimationManager(&instance.animations);
  instance.sceneRoot->setSize(w, h);

  auto input = ui::inputArea({});
  input->setFrameSize(w, h);
  input->setFocusable(true);
  input->setAcceptedButtons(InputArea::buttonMask(BTN_LEFT));
  input->addChild(
      ui::box({
          .out = &instance.dimmer,
          .fill = colorSpecFromRole(ColorRole::Shadow),
          .width = w,
          .height = h,
          .opacity = kDimOpacity,
          .participatesInLayout = false,
      })
  );
  auto content = ui::node({.out = &instance.content, .participatesInLayout = false});
  content->setSize(w, h);

  ExpoLayoutInput layoutInput{.screenWidth = w, .screenHeight = h, .scale = s, .showHeaders = m_sections.size() > 1};
  layoutInput.sections.reserve(m_sections.size());
  for (const Section& section : m_sections) {
    layoutInput.sections.push_back(
        ExpoSectionInput{
            .cardCount = section.cards.size(),
            .aspect = section.logicalHeight > 0.0F ? section.logicalWidth / section.logicalHeight : 16.0F / 9.0F,
        }
    );
  }
  const ExpoLayout layout = computeExpoLayout(layoutInput);

  const float cardRadius = Style::scaledRadiusLg(s);
  const shell::material::Spec cardMaterial{.radius = Radii{cardRadius}, .tint = kCardTintOpacity};
  const bool lightTheme = isResolvedLightTheme();
  std::optional<ColorSpec> iconTint;
  if (m_config != nullptr) {
    iconTint = effectiveShellAppIconColorizationTint(m_config->config().shell);
  }

  float widestCard = 0.0F;
  for (const ExpoSectionLayout& sectionLayout : layout.sections) {
    for (const ExpoRect& rect : sectionLayout.cards) {
      widestCard = std::max(widestCard, rect.width);
    }
  }
  const int wallpaperSize = widestCard * instance.iconPixelScale <= static_cast<float>(kWallpaperSizeSmall)
      ? kWallpaperSizeSmall
      : kWallpaperSizeLarge;

  for (std::size_t sectionIndex = 0; sectionIndex < m_sections.size(); ++sectionIndex) {
    const Section& section = m_sections[sectionIndex];
    if (sectionIndex >= layout.sections.size()) {
      break;
    }
    const ExpoSectionLayout& sectionLayout = layout.sections[sectionIndex];

    if (layoutInput.showHeaders && sectionLayout.header.width > 0.0F) {
      Label* header = nullptr;
      content->addChild(
          ui::label({
              .out = &header,
              .text = section.title,
              .fontSize = Style::fontSizeCaption * s,
              .fontWeight = FontWeight::Bold,
              .color = colorSpecFromRole(ColorRole::OnSurface, kHeaderAlpha),
              .maxWidth = sectionLayout.header.width,
              .maxLines = 1,
              .ellipsize = TextEllipsize::End,
              .participatesInLayout = false,
          })
      );
      header->measure(renderer);
      header->setPosition(
          sectionLayout.header.x,
          std::round(sectionLayout.header.y + ((sectionLayout.header.height - header->height()) * 0.5F))
      );
    }

    for (std::size_t slot = 0; slot < section.cards.size() && slot < sectionLayout.cards.size(); ++slot) {
      const std::size_t cardIndex = section.cards[slot];
      const Card& card = m_cards[cardIndex];
      const ExpoRect rect = sectionLayout.cards[slot];
      instance.cardRects[cardIndex] = rect;

      auto cardArea = ui::inputArea({
          .acceptedButtons = InputArea::buttonMask(BTN_LEFT),
          .cursorShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER,
          .x = rect.x,
          .y = rect.y,
          .frameWidth = rect.width,
          // The caption belongs to the card: clicking it switches too.
          .frameHeight = rect.height + layout.captionGap + layout.captionHeight,
          .participatesInLayout = false,
          .onEnter = [this, cardIndex](const InputArea::PointerData&) { selectCardFromPointer(cardIndex); },
          .onMotion = [this, cardIndex](const InputArea::PointerData&) { selectCardFromPointer(cardIndex); },
          .onClick =
              [this, output = card.output, workspaceId = card.workspace.id](const InputArea::PointerData&) {
                runAfterPointerDispatch([this, output, workspaceId]() {
                  for (std::size_t i = 0; i < m_cards.size(); ++i) {
                    if (m_cards[i].output == output && m_cards[i].workspace.id == workspaceId) {
                      activateCard(i);
                      return;
                    }
                  }
                });
              },
      });

      // A Slate desktop with the wallpaper laid over the tint and the hairline on top.
      shell::material::Nodes material = shell::material::build(*cardArea, cardMaterial);
      shell::material::apply(material, cardMaterial, lightTheme, s);
      shell::material::layout(material, 0.0F, 0.0F, rect.width, rect.height, s);
      cardArea->addChild(
          ui::box({
              .fill = clearColorSpec(),
              .radius = cardRadius,
              .width = rect.width,
              .height = rect.height,
              .participatesInLayout = false,
              .configure =
                  [lightTheme, s](Box& rim) {
                    rim.setHitTestVisible(false);
                    rim.setZIndex(1);
                    shell::material::applyHairline(rim, lightTheme, s);
                  },
          })
      );
      if (!section.wallpaperPath.empty() && m_asyncTextures != nullptr) {
        Image* wallpaper = nullptr;
        cardArea->addChild(
            ui::image({
                .out = &wallpaper,
                .fit = ImageFit::Cover,
                .radius = cardRadius,
                .width = rect.width,
                .height = rect.height,
                .participatesInLayout = false,
            })
        );
        wallpaper->setHitTestVisible(false);
        wallpaper->setAsyncReadyCallback([this]() { requestSceneUpdate(); });
        (void)wallpaper->setSourceFileAsync(renderer, *m_asyncTextures, section.wallpaperPath, wallpaperSize, true);
      }

      auto windowLayer = ui::node({.width = rect.width, .height = rect.height, .participatesInLayout = false});
      windowLayer->setZIndex(2);
      std::vector<ExpoWindowGeometry> geometries;
      geometries.reserve(card.windows.size());
      for (const std::size_t windowIndex : card.windows) {
        geometries.push_back(m_windows[windowIndex].geometry);
      }
      const std::vector<ExpoRect> placed =
          placeExpoWindows(rect.width, rect.height, section.logicalWidth, section.logicalHeight, geometries, s);
      // Larger windows first, so small ones stacked on top of them stay visible and clickable.
      std::vector<std::size_t> drawOrder(card.windows.size());
      std::iota(drawOrder.begin(), drawOrder.end(), std::size_t{0});
      std::ranges::stable_sort(drawOrder, [&placed](std::size_t a, std::size_t b) {
        return placed[a].width * placed[a].height > placed[b].width * placed[b].height;
      });
      for (const std::size_t order : drawOrder) {
        const std::size_t windowIndex = card.windows[order];
        const ExpoRect& local = placed[order];
        auto tile = std::make_unique<ExpoWindowTile>(s, instance.iconPixelScale, m_asyncTextures);
        tile->setParticipatesInLayout(false);
        tile->setAppIconColorizeTint(iconTint);
        tile->setTileSize(std::round(local.width), std::round(local.height));
        tile->setPosition(std::round(local.x), std::round(local.y));
        tile->setOnActivate([this, key = m_windows[windowIndex].entry.identityKey]() {
          runAfterPointerDispatch([this, key]() {
            if (const auto index = windowIndexForKey(key); index.has_value()) {
              activateWindow(*index);
            }
          });
        });
        tile->setOnHover([this, windowIndex]() { selectFromPointer(windowIndex); });
        tile->setOnInvalidate([this]() { requestSceneUpdate(); });
        tile->bind(renderer, m_windows[windowIndex].entry);
        instance.tiles[windowIndex] = tile.get();
        instance.windowRects[windowIndex] = ExpoRect{
            .x = rect.x + std::round(local.x),
            .y = rect.y + std::round(local.y),
            .width = std::round(local.width),
            .height = std::round(local.height),
        };
        windowLayer->addChild(std::move(tile));
      }
      cardArea->addChild(std::move(windowLayer));

      const float outset = kRingOutset * s;
      Box* ring = nullptr;
      cardArea->addChild(
          ui::box({
              .out = &ring,
              .fill = clearColorSpec(),
              .radius = cardRadius + outset,
              .visible = false,
              .participatesInLayout = false,
          })
      );
      ring->setHitTestVisible(false);
      ring->setZIndex(3);
      ring->setPosition(-outset, -outset);
      ring->setFrameSize(rect.width + (2.0F * outset), rect.height + (2.0F * outset));
      instance.cardRings[cardIndex] = ring;

      Label* caption = nullptr;
      const bool active = card.workspace.active;
      cardArea->addChild(
          ui::label({
              .out = &caption,
              .text = card.caption,
              .fontSize = Style::fontSizeBody * s,
              .fontWeight = active ? FontWeight::Bold : FontWeight::Medium,
              .color = colorSpecFromRole(ColorRole::OnSurface, active ? 1.0F : kInactiveCaptionAlpha),
              .maxWidth = rect.width,
              .maxLines = 1,
              .textAlign = TextAlign::Center,
              .ellipsize = TextEllipsize::End,
              .participatesInLayout = false,
          })
      );
      caption->measure(renderer);
      caption->setPosition(
          std::round((rect.width - caption->width()) * 0.5F),
          std::round(rect.height + layout.captionGap + ((layout.captionHeight - caption->height()) * 0.5F))
      );

      content->addChild(std::move(cardArea));
    }
  }

  // Mission Control names the window under the pointer in a small pill.
  auto pill = ui::box({
      .out = &instance.titlePill,
      .fill = colorSpecFromRole(ColorRole::Surface, kTitlePillAlpha),
      .border = scaleAlpha(colorSpecFromRole(ColorRole::Outline), Style::disabledOutlineAlpha),
      .borderWidth = Style::borderWidth,
      .visible = false,
      .participatesInLayout = false,
  });
  pill->setHitTestVisible(false);
  pill->setZIndex(10);
  pill->addChild(
      ui::label({
          .out = &instance.titleLabel,
          .fontSize = Style::fontSizeCaption * s,
          .fontWeight = FontWeight::Medium,
          .color = colorSpecFromRole(ColorRole::OnSurface),
          .maxWidth = std::min(kTitleMaxWidth * s, w * 0.4F),
          .maxLines = 1,
          .ellipsize = TextEllipsize::End,
          .participatesInLayout = false,
      })
  );
  content->addChild(std::move(pill));

  input->addChild(std::move(content));
  instance.input = input.get();
  instance.sceneRoot->addChild(std::move(input));

  instance.builtWidth = width;
  instance.builtHeight = height;
  instance.sceneDirty = false;
  instance.contentDirty = false;

  syncSelection(false);
  positionTitlePill(instance);
  instance.sceneRoot->layout(renderer);

  // Frosted desktop behind the whole expo, like the overview.
  instance.surface->setBlurRegion({InputRect{0, 0, static_cast<int>(width), static_cast<int>(height)}});
  instance.surface->setSceneRoot(instance.sceneRoot.get());
  instance.inputDispatcher.setSceneRoot(instance.sceneRoot.get());
  instance.inputDispatcher.setCursorShapeCallback([this](std::uint32_t serial, std::uint32_t shape) {
    if (m_wayland != nullptr) {
      m_wayland->setCursorShape(serial, shape);
    }
  });
  instance.inputDispatcher.setFocus(instance.input);

  // The reveal resumes where it was when a rebuild (sway answering the geometry refresh right
  // after opening) replaced the nodes it animated.
  if (instance.revealProgress < 1.0F && instance.content != nullptr && instance.dimmer != nullptr) {
    const float from = instance.revealProgress;
    auto apply = revealSetter(instance);
    apply(from);
    instance.animations.animateFade(
        from, 1.0F, static_cast<float>(Style::animNormal) * (1.0F - from), Easing::EaseOutCubic, std::move(apply),
        [&instance]() { instance.revealProgress = 1.0F; }, instance.content
    );
  } else if (instance.content != nullptr && instance.dimmer != nullptr) {
    revealSetter(instance)(instance.revealProgress);
  }
}

void Expo::syncSelection(bool animate) {
  if (m_instance == nullptr) {
    return;
  }
  Instance& instance = *m_instance;
  const float s = instance.scale;
  for (std::size_t i = 0; i < instance.tiles.size(); ++i) {
    if (instance.tiles[i] != nullptr) {
      instance.tiles[i]->setSelected(m_selectedWindow == i, animate);
    }
  }
  for (std::size_t i = 0; i < instance.cardRings.size() && i < m_cards.size(); ++i) {
    Box* ring = instance.cardRings[i];
    if (ring == nullptr) {
      continue;
    }
    if (!m_selectedWindow.has_value() && i == m_selectedCard) {
      ring->setBorder(colorSpecFromRole(ColorRole::Primary), kSelectedRingWidth * s);
      ring->setVisible(true);
    } else if (m_cards[i].workspace.active) {
      // The desktop each monitor shows now, outlined like the current Space in Mission Control.
      ring->setBorder(colorSpecFromRole(ColorRole::OnSurface, kActiveRingAlpha), kActiveRingWidth * s);
      ring->setVisible(true);
    } else {
      ring->setVisible(false);
    }
  }
  instance.titleDirty = true;
  requestSceneUpdate();
}

void Expo::positionTitlePill(Instance& instance) {
  instance.titleDirty = false;
  if (instance.titlePill == nullptr || instance.titleLabel == nullptr || instance.surface == nullptr) {
    return;
  }
  if (!m_selectedWindow.has_value()
      || *m_selectedWindow >= m_windows.size()
      || *m_selectedWindow >= instance.windowRects.size()) {
    instance.titlePill->setVisible(false);
    return;
  }
  const Window& window = m_windows[*m_selectedWindow];
  const std::string& title = !window.entry.title.empty() ? window.entry.title : window.entry.appLabel;
  if (title.empty() || window.card >= instance.cardRects.size()) {
    instance.titlePill->setVisible(false);
    return;
  }

  Renderer& renderer = instance.surface->renderTarget().renderer();
  const float s = instance.scale;
  instance.titleLabel->setText(title);
  instance.titleLabel->measure(renderer);
  const float pillW = std::round(instance.titleLabel->width() + (2.0F * kTitlePaddingH * s));
  const float pillH = std::round(instance.titleLabel->height() + (2.0F * kTitlePaddingV * s));
  instance.titlePill->setRadius(pillH * 0.5F);
  instance.titlePill->setFrameSize(pillW, pillH);
  instance.titleLabel->setPosition(
      std::round(kTitlePaddingH * s), std::round((pillH - instance.titleLabel->height()) * 0.5F)
  );

  // Below the window while that stays on its desktop, else just inside its bottom edge.
  const ExpoRect& windowRect = instance.windowRects[*m_selectedWindow];
  const ExpoRect& cardRect = instance.cardRects[window.card];
  const float gap = kTitleGap * s;
  float y = windowRect.bottom() + gap;
  if (y + pillH > cardRect.bottom() - gap) {
    y = std::max(windowRect.y + gap, windowRect.bottom() - pillH - gap);
  }
  const auto screenW = static_cast<float>(instance.builtWidth);
  const float margin = Style::spaceXs * s;
  const float x = std::clamp(windowRect.centerX() - (pillW * 0.5F), margin, std::max(margin, screenW - pillW - margin));
  instance.titlePill->setPosition(std::round(x), std::round(y));
  instance.titlePill->setVisible(true);
}

void Expo::selectFromPointer(std::size_t window) {
  if (m_hoverSelects) {
    selectWindow(window, true);
  }
}

void Expo::selectCardFromPointer(std::size_t card) {
  if (m_hoverSelects) {
    selectCard(card, false, true);
  }
}

void Expo::selectWindow(std::size_t index, bool animate) {
  if (index >= m_windows.size() || m_selectedWindow == index) {
    return;
  }
  m_selectedWindow = index;
  m_selectedCard = m_windows[index].card;
  syncSelection(animate);
}

void Expo::selectCard(std::size_t card, bool preferWindow, bool animate) {
  if (card >= m_cards.size()) {
    return;
  }
  std::optional<std::size_t> window;
  if (preferWindow && !m_cards[card].windows.empty()) {
    window = m_cards[card].windows.front();
  }
  if (m_selectedCard == card && m_selectedWindow == window) {
    return;
  }
  m_selectedCard = card;
  m_selectedWindow = window;
  syncSelection(animate);
}

void Expo::moveSelection(ExpoDirection direction) {
  if (m_instance == nullptr) {
    return;
  }
  m_hoverSelects = false;
  const Instance& instance = *m_instance;
  // Every window plus the empty desktops, which can only be reached as a whole.
  struct Target {
    bool window = false;
    std::size_t index = 0;
  };
  std::vector<ExpoRect> rects;
  std::vector<Target> targets;
  std::optional<std::size_t> origin;
  for (std::size_t i = 0; i < m_windows.size() && i < instance.windowRects.size(); ++i) {
    if (m_selectedWindow == i) {
      origin = targets.size();
    }
    rects.push_back(instance.windowRects[i]);
    targets.push_back(Target{.window = true, .index = i});
  }
  for (std::size_t i = 0; i < m_cards.size() && i < instance.cardRects.size(); ++i) {
    // A desktop with windows is a target only as the origin, when the pointer selected it.
    const bool originCard = !m_selectedWindow.has_value() && i == m_selectedCard;
    if (!m_cards[i].windows.empty() && !originCard) {
      continue;
    }
    if (originCard) {
      origin = targets.size();
    }
    rects.push_back(instance.cardRects[i]);
    targets.push_back(Target{.window = false, .index = i});
  }
  if (!origin.has_value()) {
    return;
  }
  const auto next = expoSpatialNeighbor(rects, *origin, direction);
  if (!next.has_value()) {
    return;
  }
  const Target& target = targets[*next];
  if (target.window) {
    selectWindow(target.index, true);
  } else {
    selectCard(target.index, false, true);
  }
}

void Expo::cycleCard(int delta) {
  if (m_cards.empty()) {
    return;
  }
  m_hoverSelects = false;
  const auto count = static_cast<long>(m_cards.size());
  const long next = ((static_cast<long>(m_selectedCard) + delta) % count + count) % count;
  selectCard(static_cast<std::size_t>(next), true, true);
}

void Expo::runAfterPointerDispatch(std::function<void()> action) {
  DeferredCall::callLater([this, session = m_session, action = std::move(action)]() {
    if (m_active && m_session == session) {
      action();
    }
  });
}

void Expo::activateSelection() {
  if (m_selectedWindow.has_value()) {
    activateWindow(*m_selectedWindow);
    return;
  }
  if (m_selectedCard < m_cards.size()) {
    activateCard(m_selectedCard);
    return;
  }
  close();
}

void Expo::activateWindow(std::size_t index) {
  if (index >= m_windows.size() || m_platform == nullptr) {
    return;
  }
  const WindowSwitcherEntry entry = m_windows[index].entry;
  CompositorPlatform* platform = m_platform;
  // Hyprland ignores zwlr_foreign_toplevel_handle_v1.activate while an exclusive keyboard
  // layer surface is mapped (hyprwm/Hyprland#4829): close first (at once, so the surface is
  // gone), activate on the next tick.
  closeNow();
  DeferredCall::callLater([platform, entry]() { activateWindowSwitcherEntry(*platform, entry); });
}

void Expo::activateCard(std::size_t card) {
  if (card >= m_cards.size()) {
    return;
  }
  const Card target = m_cards[card];
  CompositorPlatform* platform = m_platform;
  WaylandConnection* wayland = m_wayland;
  // Re-selecting the focused monitor's own desktop would flip back to the previous one on
  // compositors with workspace back-and-forth (Hyprland binds:workspace_back_and_forth).
  const bool alreadyFocused = target.workspace.active && target.output == m_output;
  closeNow();
  if (!target.hasWorkspace || alreadyFocused || platform == nullptr || wayland == nullptr) {
    return;
  }
  DeferredCall::callLater([platform, wayland, output = target.output, workspace = target.workspace]() {
    if (findOutput(*wayland, output) == nullptr) {
      return; // unplugged in between
    }
    platform->activateWorkspace(output, workspace);
  });
}

void Expo::startThumbnailCaptures() {
  if (!m_active || m_wayland == nullptr || m_thumbnailCapture == nullptr || !m_thumbnailCapture->available()) {
    return;
  }
  std::unordered_set<std::string> queued;
  for (const auto& request : m_thumbnailQueue) {
    queued.insert(request.windowKey);
  }
  if (!m_captureInFlightKey.empty()) {
    queued.insert(m_captureInFlightKey);
  }
  const auto enqueue = [this, &queued](std::size_t index) {
    const WindowSwitcherEntry& entry = m_windows[index].entry;
    if (entry.thumbnail != nullptr
        || entry.captureHandle == 0
        || entry.identityKey.empty()
        || m_failedCaptureKeys.contains(entry.identityKey)) {
      return;
    }
    if (queued.insert(entry.identityKey).second) {
      m_thumbnailQueue.push_back(
          ThumbnailRequest{.windowKey = entry.identityKey, .captureHandle = entry.captureHandle}
      );
    }
  };
  // The selected window first, then the desktops on screen, then the rest.
  if (m_selectedWindow.has_value() && *m_selectedWindow < m_windows.size()) {
    enqueue(*m_selectedWindow);
  }
  for (const Card& card : m_cards) {
    if (card.workspace.active) {
      for (const std::size_t index : card.windows) {
        enqueue(index);
      }
    }
  }
  for (std::size_t i = 0; i < m_windows.size(); ++i) {
    enqueue(i);
  }
  captureNextThumbnail();
}

void Expo::captureNextThumbnail() {
  if (!m_active || m_wayland == nullptr || m_thumbnailCapture == nullptr || m_thumbnailCapture->busy()) {
    return;
  }
  while (!m_thumbnailQueue.empty()) {
    ThumbnailRequest request = std::move(m_thumbnailQueue.front());
    m_thumbnailQueue.pop_front();
    if (!windowIndexForKey(request.windowKey).has_value()) {
      continue; // closed meanwhile
    }
    auto* handle = reinterpret_cast<ext_foreign_toplevel_handle_v1*>(request.captureHandle);
    bool handleIsLive = false;
    m_wayland->visitExtToplevelHandles([&](ext_foreign_toplevel_handle_v1* live) { handleIsLive |= live == handle; });
    if (!handleIsLive) {
      continue;
    }

    m_captureInFlightKey = request.windowKey;
    m_thumbnailCapture->capture(
        handle, kThumbnailMaxWidth, kThumbnailMaxHeight,
        [this, windowKey = std::move(request.windowKey)](std::optional<ScreencopyImage> image, std::string error) {
          m_captureInFlightKey.clear();
          if (!error.empty()) {
            kLog.debug("thumbnail capture skipped for {}: {}", windowKey, error);
            if (m_active) {
              m_failedCaptureKeys.insert(windowKey);
            }
          } else if (m_active && image.has_value()) {
            auto thumbnail = std::make_shared<ScreencopyImage>(std::move(*image));
            for (auto& window : m_windows) {
              if (window.entry.identityKey == windowKey) {
                window.entry.thumbnail = thumbnail;
              }
            }
            if (m_instance != nullptr) {
              m_instance->contentDirty = true;
            }
            requestSceneUpdate();
          }
          captureNextThumbnail();
        }
    );
    return;
  }
}

void Expo::cancelThumbnailCaptures() {
  m_thumbnailQueue.clear();
  m_captureInFlightKey.clear();
  if (m_thumbnailCapture != nullptr) {
    m_thumbnailCapture->cancelInFlight();
  }
}

bool Expo::onKeyboardEvent(const KeyboardEvent& event) {
  if (!m_active) {
    return false;
  }
  if (m_instance == nullptr) {
    close();
    return false;
  }
  // Keys typed into another surface (a panel opened on top) are not the expo's.
  if (m_instance->surface == nullptr
      || m_wayland == nullptr
      || m_wayland->lastKeyboardSurface() != m_instance->surface->wlSurface()) {
    return false;
  }
  if (!event.pressed || event.preedit) {
    return true;
  }

  const auto matchesAction = [&](KeybindAction action) {
    if (m_config != nullptr) {
      return m_config->matchesKeybind(action, event.sym, event.modifiers);
    }
    return KeybindMatcher::matches(action, event.sym, event.modifiers);
  };

  // Super+O closes it again when the compositor lets the key through instead of running the bind.
  const bool superO = (event.modifiers & KeyMod::Super) != 0 && (event.sym == XKB_KEY_o || event.sym == XKB_KEY_O);
  if (superO || matchesAction(KeybindAction::Cancel)) {
    close();
    return true;
  }
  if (matchesAction(KeybindAction::Validate)) {
    activateSelection();
    return true;
  }
  if (matchesAction(KeybindAction::TabPrevious)) {
    cycleCard(-1);
    return true;
  }
  if (matchesAction(KeybindAction::TabNext)) {
    cycleCard(1);
    return true;
  }
  if (matchesAction(KeybindAction::Left)) {
    moveSelection(ExpoDirection::Left);
  } else if (matchesAction(KeybindAction::Right)) {
    moveSelection(ExpoDirection::Right);
  } else if (matchesAction(KeybindAction::Up)) {
    moveSelection(ExpoDirection::Up);
  } else if (matchesAction(KeybindAction::Down)) {
    moveSelection(ExpoDirection::Down);
  }
  return true;
}

bool Expo::onPointerEvent(const PointerEvent& event) {
  if (!m_active || m_instance == nullptr || m_instance->surface == nullptr) {
    return false;
  }
  Instance* target = m_instance.get();
  const bool onTarget = event.surface != nullptr && event.surface == target->surface->wlSurface();

  if (!onTarget) {
    if (catcherForSurface(event.surface) != nullptr) {
      if (event.type == PointerEvent::Type::Enter && m_wayland != nullptr) {
        m_wayland->setCursorShape(event.serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
      }
      // A click on another monitor dismisses the expo, like a click on its backdrop.
      if (event.type == PointerEvent::Type::Button && event.pressed) {
        close();
      }
      return true;
    }
    // Another shell surface (bar, dock, a panel): the click dismisses the expo and still reaches it.
    if (event.type == PointerEvent::Type::Button && event.pressed) {
      close();
    }
    return false;
  }

  const auto x = static_cast<float>(event.sx);
  const auto y = static_cast<float>(event.sy);
  switch (event.type) {
  case PointerEvent::Type::Enter:
    // The overlay mapping under a resting cursor is no reason to move the selection.
    m_hasPointerPosition = true;
    m_pointerX = event.sx;
    m_pointerY = event.sy;
    target->pointerInside = true;
    target->inputDispatcher.pointerEnter(x, y, event.serial);
    return true;
  case PointerEvent::Type::Leave:
    target->pointerInside = false;
    target->inputDispatcher.pointerLeave();
    return true;
  case PointerEvent::Type::Motion:
    if (m_hasPointerPosition && (std::abs(event.sx - m_pointerX) > 0.5 || std::abs(event.sy - m_pointerY) > 0.5)) {
      m_hoverSelects = true;
    }
    m_hasPointerPosition = true;
    m_pointerX = event.sx;
    m_pointerY = event.sy;
    target->pointerInside = true;
    target->inputDispatcher.pointerMotion(x, y, 0);
    return true;
  case PointerEvent::Type::Button:
    target->pointerInside = true;
    if (event.pressed
        && (target->inputDispatcher.hoveredArea() == nullptr
            || target->inputDispatcher.hoveredArea() == target->input)) {
      close();
      return true;
    }
    (void)target->inputDispatcher.pointerButton(
        x, y, event.button, event.pressed, event.serial, event.time, event.touch
    );
    return true;
  case PointerEvent::Type::Axis:
    return true;
  }
  return false;
}
