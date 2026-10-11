#include "shell/gesture_handle/gesture_handle.h"

#include "wayland/surface.h"
#include "wayland/wayland_seat.h"

#include <cstdlib>
#include <stdexcept>

GestureHandle::GestureHandle() = default;

GestureHandle::~GestureHandle() = default;

void GestureHandle::initialize(
    CompositorPlatform& platform, ConfigService& config, RenderContext* renderContext, IpcService* ipc,
    PanelManager* panels, Bar* bar
) {
  (void)platform;
  (void)config;
  (void)renderContext;
  (void)ipc;
  (void)panels;
  (void)bar;
  throw std::logic_error("GestureHandle::initialize: not implemented");
}

void GestureHandle::reload() { throw std::logic_error("GestureHandle::reload: not implemented"); }

void GestureHandle::onOutputChange() { throw std::logic_error("GestureHandle::onOutputChange: not implemented"); }

void GestureHandle::onWorkspaceChanged() {
  throw std::logic_error("GestureHandle::onWorkspaceChanged: not implemented");
}

bool GestureHandle::onPointerEvent(const PointerEvent& event) {
  (void)event;
  throw std::logic_error("GestureHandle::onPointerEvent: not implemented");
}

void GestureHandle::requestLayout() { throw std::logic_error("GestureHandle::requestLayout: not implemented"); }

void GestureHandle::requestRedraw() { throw std::logic_error("GestureHandle::requestRedraw: not implemented"); }

std::vector<wl_surface*> GestureHandle::surfaces() const {
  throw std::logic_error("GestureHandle::surfaces: not implemented");
}

std::vector<InputRect> GestureHandle::surfaceRectsForOutput(wl_output* output) const {
  (void)output;
  throw std::logic_error("GestureHandle::surfaceRectsForOutput: not implemented");
}

void GestureHandle::syncInstances() { throw std::logic_error("GestureHandle::syncInstances: not implemented"); }

void GestureHandle::createInstance(const WaylandOutput& output) {
  (void)output;
  throw std::logic_error("GestureHandle::createInstance: not implemented");
}

void GestureHandle::destroyInstances() { throw std::logic_error("GestureHandle::destroyInstances: not implemented"); }

void GestureHandle::buildScene(Instance& instance, std::uint32_t width, std::uint32_t height) {
  (void)instance;
  (void)width;
  (void)height;
  throw std::logic_error("GestureHandle::buildScene: not implemented");
}

void GestureHandle::updateVisibility(Instance& instance, bool animate) {
  (void)instance;
  (void)animate;
  throw std::logic_error("GestureHandle::updateVisibility: not implemented");
}

void GestureHandle::handleOutcome(Instance& instance, shell::gesture_handle::Outcome outcome) {
  (void)instance;
  (void)outcome;
  throw std::logic_error("GestureHandle::handleOutcome: not implemented");
}

void GestureHandle::reveal(Instance& instance) {
  (void)instance;
  throw std::logic_error("GestureHandle::reveal: not implemented");
}

void GestureHandle::switchWorkspace(Instance& instance, int direction) {
  (void)instance;
  (void)direction;
  throw std::logic_error("GestureHandle::switchWorkspace: not implemented");
}

void GestureHandle::settleLine(Instance& instance) {
  (void)instance;
  throw std::logic_error("GestureHandle::settleLine: not implemented");
}

GestureHandle::Instance* GestureHandle::instanceForSurface(wl_surface* surface) const noexcept {
  (void)surface;
  std::abort(); // not implemented
}
