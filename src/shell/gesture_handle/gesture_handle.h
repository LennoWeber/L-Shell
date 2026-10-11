#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "shell/gesture_handle/handle_gesture.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Bar;
class Box;
class CompositorPlatform;
class ConfigService;
class IpcService;
class Node;
class PanelManager;
class RenderContext;
struct InputRect;
struct PointerEvent;
struct WaylandOutput;
struct wl_output;
struct wl_surface;

// The bottom-center home-indicator line of the handle and flow layouts. Shown while the active workspace has
// windows; hovering or pulling it up reveals the pop-up bar or the full-screen status overlay, and dragging it
// sideways switches workspace.
class GestureHandle {
public:
  GestureHandle();
  ~GestureHandle();

  GestureHandle(const GestureHandle&) = delete;
  GestureHandle& operator=(const GestureHandle&) = delete;

  void initialize(
      CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
      PanelManager* panels, Bar* bar
  );
  void reload();
  void onOutputChange();
  // Hidden while the lock-screen widget editor owns the screen, like the bar and the dock.
  void setSuppressed(bool suppressed);
  void onWorkspaceChanged();
  bool onPointerEvent(const PointerEvent& event);
  void requestLayout();
  void requestRedraw();
  [[nodiscard]] std::vector<wl_surface*> surfaces() const;
  [[nodiscard]] std::vector<InputRect> surfaceRectsForOutput(wl_output* output) const;

private:
  struct Instance;

  // Reads the resolved layout; true when the handle appeared, vanished or changed.
  [[nodiscard]] bool applyConfig();
  void syncInstances();
  void createInstance(const WaylandOutput& output);
  void destroyInstances();
  void buildScene(Instance& instance, std::uint32_t width, std::uint32_t height);
  void updateVisibility(Instance& instance, bool animate);
  void handleOutcome(Instance& instance, shell::gesture_handle::Outcome outcome);
  void reveal(Instance& instance);
  void switchWorkspace(Instance& instance, int direction);
  void settleLine(Instance& instance);
  void applyLineGeometry(Instance& instance);
  void prepareFrame(Instance& instance, bool needsUpdate, bool needsLayout);
  [[nodiscard]] Instance* instanceForSurface(wl_surface* surface) const noexcept;

  struct Instance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    // sceneRoot must be destroyed before `animations` — ~Node() calls cancelForOwner().
    AnimationManager animations;
    std::unique_ptr<Node> sceneRoot;
    Box* line = nullptr;
    float uiScale = 1.0F;
    // Surface and line geometry, fixed at creation (logical px): known before the first configure, so the input
    // region is right from the first commit.
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    int marginBottom = 0;
    float lineX = 0.0F;
    float lineY = 0.0F;
    float lineThickness = 0.0F;
    // Applied visibility (nullopt before the first pass), so repeated workspace/window events cost nothing.
    std::optional<bool> shownApplied;
    shell::gesture_handle::GestureRecognizer recognizer;
    Timer dwellTimer; // hover dwell before a reveal, HandleConfig::hoverDelayMs
    // 0 = hidden (empty workspace), 1 = shown; animated.
    float visibility = 0.0F;
    AnimationManager::Id visibilityAnim = 0;
    // Line follows the pointer a little while dragging, like the iOS home indicator.
    float dragOffsetX = 0.0F;
    float dragOffsetY = 0.0F;
    AnimationManager::Id dragSettleAnim = 0; // springs the line back after a drag
    bool pointerInside = false;
    // reveal = "bar": the line hides while its bar is peeked and comes back once the bar slid away. The bar's name
    // is kept with the peek, so a reload that renames or drops the bar still releases the right one.
    bool peeking = false;
    std::string peekedBar;
    Timer peekReleaseTimer; // ends the bar peek if the pointer never reaches the bar
    Timer peekWatchTimer;   // polls the bar until it is hidden again
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  PanelManager* m_panels = nullptr;
  Bar* m_bar = nullptr;
  std::optional<HandleConfig> m_handleConfig; // nullopt when the active layout has no handle
  std::string m_revealBarName;                // synthesized bar to peek on reveal = "bar"
  bool m_hideAtHome = false;                  // flow, or reveal = "bar": the line yields to the home bar/dock
  bool m_visibilityPassQueued = false;        // coalesces bursts of workspace/window events into one pass
  bool m_suppressed = false;
  std::vector<std::unique_ptr<Instance>> m_instances;
};
