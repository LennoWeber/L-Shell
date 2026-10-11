#include "shell/screen_edges/screen_edges.h"

#include "render/scene/node.h"
#include "wayland/surface.h"
#include "wayland/wayland_seat.h"

#include <cstdlib>
#include <stdexcept>

ScreenEdges::ScreenEdges() = default;

ScreenEdges::~ScreenEdges() = default;

void ScreenEdges::initialize(
    CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
    PanelManager* panels
) {
  (void)platform;
  (void)config;
  (void)renderContext;
  (void)ipc;
  (void)panels;
  throw std::logic_error("ScreenEdges::initialize: not implemented");
}

void ScreenEdges::reload() { throw std::logic_error("ScreenEdges::reload: not implemented"); }

void ScreenEdges::onOutputChange() { throw std::logic_error("ScreenEdges::onOutputChange: not implemented"); }

void ScreenEdges::setSuppressed(bool suppressed) {
  (void)suppressed;
  throw std::logic_error("ScreenEdges::setSuppressed: not implemented");
}

bool ScreenEdges::onPointerEvent(const PointerEvent& event) {
  (void)event;
  throw std::logic_error("ScreenEdges::onPointerEvent: not implemented");
}

std::vector<wl_surface*> ScreenEdges::surfaces() const {
  throw std::logic_error("ScreenEdges::surfaces: not implemented");
}

std::vector<InputRect> ScreenEdges::surfaceRectsForOutput(wl_output* output) const {
  (void)output;
  throw std::logic_error("ScreenEdges::surfaceRectsForOutput: not implemented");
}

void ScreenEdges::syncInstances() { throw std::logic_error("ScreenEdges::syncInstances: not implemented"); }

void ScreenEdges::destroyInstances() { throw std::logic_error("ScreenEdges::destroyInstances: not implemented"); }

void ScreenEdges::buildStrip(OutputInstance& instance, Edge edge, const noctalia::bar::WidgetAction& action) {
  (void)instance;
  (void)edge;
  (void)action;
  throw std::logic_error("ScreenEdges::buildStrip: not implemented");
}

void ScreenEdges::armStrip(OutputInstance& instance, Strip& strip) {
  (void)instance;
  (void)strip;
  throw std::logic_error("ScreenEdges::armStrip: not implemented");
}

void ScreenEdges::disarmStrip(Strip& strip) {
  (void)strip;
  throw std::logic_error("ScreenEdges::disarmStrip: not implemented");
}

void ScreenEdges::triggerStrip(OutputInstance& instance, Strip& strip) {
  (void)instance;
  (void)strip;
  throw std::logic_error("ScreenEdges::triggerStrip: not implemented");
}

InputRect ScreenEdges::stripRect(
    Edge edge, std::int32_t outputWidth, std::int32_t outputHeight, const EdgesConfig& config
) {
  (void)edge;
  (void)outputWidth;
  (void)outputHeight;
  (void)config;
  throw std::logic_error("ScreenEdges::stripRect: not implemented");
}

std::string_view ScreenEdges::panelPositionForEdge(Edge edge) noexcept {
  (void)edge;
  std::abort(); // not implemented
}
