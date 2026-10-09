#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "render/scene/input_dispatcher.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class CompositorPlatform;
class ConfigService;
class IpcService;
class Node;
class RenderContext;
struct wl_output;

// Caelestia-style screen-edge drawers: a thin invisible strip on the middle part of each screen edge. Resting
// the pointer on a strip runs that edge's action; panels opened that way are anchored at the edge.
class ScreenEdges {
private:
  enum class Edge : std::uint8_t { Top, Bottom, Left, Right };

  struct Strip {
    Edge edge = Edge::Top;
    std::unique_ptr<LayerSurface> surface;
    std::unique_ptr<Node> sceneRoot;
    InputDispatcher inputDispatcher;
    Timer dwellTimer;
  };

  struct OutputInstance {
    std::uint32_t outputName = 0;
    wl_output* output = nullptr;
    std::int32_t logicalWidth = 0;
    std::int32_t logicalHeight = 0;
    std::vector<std::unique_ptr<Strip>> strips;
  };

  CompositorPlatform* m_platform = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  IpcService* m_ipc = nullptr;
  std::optional<EdgesConfig> m_edgesConfig; // nullopt when the active layout has no screen edges
  std::vector<std::unique_ptr<OutputInstance>> m_instances;
};
