#pragma once

#include "config/config_types.h"
#include "core/timer_manager.h"
#include "render/scene/input_dispatcher.h"
#include "shell/bar/widget_action.h"
#include "wayland/layer_surface.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class CompositorPlatform;
class ConfigService;
class IpcService;
class Node;
class PanelManager;
class RenderContext;
struct PointerEvent;
struct WaylandOutput;
struct wl_output;

// Caelestia-style screen-edge drawers: a thin invisible strip on the middle part of each screen edge. Resting
// the pointer on a strip runs that edge's action; panels opened that way are anchored at the edge.
class ScreenEdges {
public:
  ScreenEdges();
  ~ScreenEdges();

  ScreenEdges(const ScreenEdges&) = delete;
  ScreenEdges& operator=(const ScreenEdges&) = delete;

  void initialize(
      CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
      PanelManager* panels
  );
  void reload();
  void onOutputChange();
  bool onPointerEvent(const PointerEvent& event);

private:
  enum class Edge : std::uint8_t { Top, Bottom, Left, Right };

  struct Strip;
  struct OutputInstance;

  // Strip rectangle in output-local logical px: centered on its edge, `length` of the edge long.
  struct StripRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
  };

  void syncInstances();
  void destroyInstances();
  void buildStrip(OutputInstance& instance, Edge edge, const noctalia::bar::WidgetAction& action);
  void armStrip(OutputInstance& instance, Strip& strip);
  void disarmStrip(Strip& strip);
  void triggerStrip(OutputInstance& instance, Strip& strip);
  [[nodiscard]] static StripRect stripRect(Edge edge, std::int32_t outputWidth, std::int32_t outputHeight,
                                           const EdgesConfig& config);
  // kPanelPositions value a panel opened from this edge floats at.
  [[nodiscard]] static std::string_view panelPositionForEdge(Edge edge) noexcept;

  struct Strip {
    Edge edge = Edge::Top;
    noctalia::bar::WidgetAction action; // parsed from the edge's EdgesConfig entry
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
  PanelManager* m_panels = nullptr;
  std::optional<EdgesConfig> m_edgesConfig; // nullopt when the active layout has no screen edges
  std::vector<std::unique_ptr<OutputInstance>> m_instances;
};
