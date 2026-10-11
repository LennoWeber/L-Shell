#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "render/animation/animation_manager.h"
#include "render/scene/input_dispatcher.h"
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
class InputArea;
class IpcService;
class Node;
class PanelManager;
class RenderContext;
struct wl_output;

// The bottom-center home-indicator line of the handle and flow layouts. Shown while the active workspace has
// windows; hovering or pulling it up reveals the pop-up bar or the full-screen status overlay, and dragging it
// sideways switches workspace.
class GestureHandle {
private:
  struct Instance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    // sceneRoot must be destroyed before `animations` — ~Node() calls cancelForOwner().
    AnimationManager animations;
    std::unique_ptr<Node> sceneRoot;
    InputDispatcher inputDispatcher;
    InputArea* inputArea = nullptr; // whole surface: the line plus a forgiving hit margin
    Box* line = nullptr;
    shell::gesture_handle::GestureRecognizer recognizer;
    Timer dwellTimer; // hover dwell before a reveal, HandleConfig::hoverDelayMs
    // 0 = hidden (empty workspace), 1 = shown; animated.
    float visibility = 0.0F;
    AnimationManager::Id visibilityAnim = 0;
    // Line follows the pointer a little while dragging, like the iOS home indicator.
    float dragOffsetX = 0.0F;
    float dragOffsetY = 0.0F;
    AnimationManager::Id dragSettleAnim = 0; // springs the line back after a drag
    bool occupied = false; // active workspace on this output has windows
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  PanelManager* m_panels = nullptr;
  Bar* m_bar = nullptr;
  std::optional<HandleConfig> m_handleConfig; // nullopt when the active layout has no handle
  std::string m_revealBarName;                // synthesized bar to peek on reveal = "bar"
  std::vector<std::unique_ptr<Instance>> m_instances;
};
