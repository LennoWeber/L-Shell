#pragma once

#include "compositors/workspace_backend.h"
#include "shell/expo/expo_layout.h"
#include "shell/switcher/window_switcher_tile.h"
#include "system/icon_resolver.h"
#include "wayland/wayland_seat.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class AsyncTextureCache;
class CompositorPlatform;
class ConfigService;
class IpcService;
class RenderContext;
class ToplevelThumbnailCapture;
class WaylandConnection;
struct wl_output;

// Super+O "expo": every workspace of every output as a miniature desktop with its windows at
// their real positions, like hyprexpo and macOS Mission Control. Clicking a window focuses it
// (the compositor brings its workspace and output along); clicking a desktop switches to it.
class Expo {
public:
  Expo();
  ~Expo();

  Expo(const Expo&) = delete;
  Expo& operator=(const Expo&) = delete;

  void initialize(
      WaylandConnection& wayland, RenderContext* renderContext, CompositorPlatform& platform, ConfigService* config,
      AsyncTextureCache* asyncTextures
  );
  void registerIpc(IpcService& ipc);
  // Runs just before the expo opens, so other full-screen overlays can step aside.
  void setOnOpen(std::function<void()> callback) { m_onOpen = std::move(callback); }
  void onConfigReload();
  void onOutputChange();
  // Windows opened, closed, moved or retitled, or a workspace changed.
  void onWindowsChanged();
  void toggle(wl_output* output);
  void close();

  [[nodiscard]] bool isActive() const noexcept { return m_active; }
  [[nodiscard]] bool onPointerEvent(const PointerEvent& event);
  [[nodiscard]] bool onKeyboardEvent(const KeyboardEvent& event);

private:
  struct Instance;

  struct Window {
    WindowSwitcherEntry entry;
    std::size_t card = 0;
    ExpoWindowGeometry geometry; // monitor-local logical pixels
  };

  struct Card {
    wl_output* output = nullptr;
    Workspace workspace;
    // False for the stand-in desktop of an output whose compositor reports no workspaces.
    bool hasWorkspace = true;
    std::size_t section = 0;
    std::string caption;
    std::vector<std::size_t> windows; // reading order (top to bottom, left to right)
  };

  struct Section {
    wl_output* output = nullptr;
    std::string title;
    std::string wallpaperPath;
    float logicalWidth = 0.0F;
    float logicalHeight = 0.0F;
    std::vector<std::size_t> cards;
  };

  struct ThumbnailRequest {
    std::string windowKey;
    std::uintptr_t captureHandle = 0;
  };

  void open(wl_output* output);
  void refreshModel();
  // Changes only when cards, their windows or the window geometry change; titles and
  // snapshots update in place.
  [[nodiscard]] std::string structureSignature() const;
  void ensureSurface();
  void destroySurface();
  void requestSceneUpdate();
  void prepareFrame(Instance& instance);
  void buildScene(Instance& instance, std::uint32_t width, std::uint32_t height);
  void rebindTiles(Instance& instance);
  void syncSelection(bool animate);
  void positionTitlePill(Instance& instance);
  void selectWindow(std::size_t index, bool animate);
  // `preferWindow` selects the card's first window when it has any (keyboard), otherwise the card itself (pointer).
  void selectCard(std::size_t card, bool preferWindow, bool animate);
  void moveSelection(ExpoDirection direction);
  void cycleCard(int delta);
  // Pointer callbacks run inside the instance's InputDispatcher, which close() destroys, so a
  // click acts on the next loop iteration (and not at all once the session ended).
  void runAfterPointerDispatch(std::function<void()> action);
  void activateSelection();
  void activateWindow(std::size_t index);
  void activateCard(std::size_t card);
  void startThumbnailCaptures();
  void captureNextThumbnail();
  void cancelThumbnailCaptures();
  [[nodiscard]] std::optional<std::size_t> windowIndexForKey(std::string_view key) const;

  WaylandConnection* m_wayland = nullptr;
  RenderContext* m_renderContext = nullptr;
  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  AsyncTextureCache* m_asyncTextures = nullptr;
  std::function<void()> m_onOpen;

  std::unique_ptr<Instance> m_instance;
  IconResolver m_iconResolver;
  std::unique_ptr<ToplevelThumbnailCapture> m_thumbnailCapture;
  std::deque<ThumbnailRequest> m_thumbnailQueue;
  std::string m_captureInFlightKey;

  std::vector<Section> m_sections;
  std::vector<Card> m_cards;
  std::vector<Window> m_windows;
  std::optional<std::size_t> m_selectedWindow;
  std::size_t m_selectedCard = 0;
  // The output the expo opened on, which is also the one with keyboard focus.
  wl_output* m_output = nullptr;
  // Bumped on every open; deferred work checks it still belongs to the same session.
  std::uint64_t m_session = 0;
  bool m_active = false;
};
