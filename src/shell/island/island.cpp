#include "shell/island/island.h"

#include "dbus/upower/upower_service.h"
#include "notification/notification.h"
#include "notification/notification_manager.h"
#include "render/scene/node.h"
#include "shell/osd/osd_overlay.h"
#include "wayland/surface.h"
#include "wayland/wayland_seat.h"

#include <cstdlib>
#include <stdexcept>

Island::Island() = default;

Island::~Island() = default;

void Island::initialize(const IslandServices& services) {
  (void)services;
  throw std::logic_error("Island::initialize: not implemented");
}

void Island::reload() { throw std::logic_error("Island::reload: not implemented"); }

void Island::onOutputChange() { throw std::logic_error("Island::onOutputChange: not implemented"); }

void Island::setSuppressed(bool suppressed) {
  (void)suppressed;
  throw std::logic_error("Island::setSuppressed: not implemented");
}

bool Island::applyConfig() { throw std::logic_error("Island::applyConfig: not implemented"); }

void Island::refreshAll() { throw std::logic_error("Island::refreshAll: not implemented"); }

bool Island::shownOn(const Instance& instance) const {
  (void)instance;
  throw std::logic_error("Island::shownOn: not implemented");
}

void Island::onWorkspaceChanged() { throw std::logic_error("Island::onWorkspaceChanged: not implemented"); }

void Island::onSecondTick() { throw std::logic_error("Island::onSecondTick: not implemented"); }

bool Island::onPointerEvent(const PointerEvent& event) {
  (void)event;
  throw std::logic_error("Island::onPointerEvent: not implemented");
}

bool Island::takeOsdContent(const OsdContent& content) {
  (void)content;
  throw std::logic_error("Island::takeOsdContent: not implemented");
}

void Island::onPowerChanged(const UPowerChange& change) {
  (void)change;
  throw std::logic_error("Island::onPowerChanged: not implemented");
}

void Island::onMediaChanged() { throw std::logic_error("Island::onMediaChanged: not implemented"); }

void Island::requestLayout() { throw std::logic_error("Island::requestLayout: not implemented"); }

void Island::requestRedraw() { throw std::logic_error("Island::requestRedraw: not implemented"); }

std::vector<InputRect> Island::surfaceRectsForOutput(wl_output* output) const {
  (void)output;
  throw std::logic_error("Island::surfaceRectsForOutput: not implemented");
}

void Island::onNotificationEvent(const Notification& notification, NotificationEvent event) {
  (void)notification;
  (void)event;
  throw std::logic_error("Island::onNotificationEvent: not implemented");
}

bool Island::isActive() const noexcept {
  std::abort(); // not implemented
}

std::vector<wl_surface*> Island::surfaces() const { throw std::logic_error("Island::surfaces: not implemented"); }

void Island::syncInstances() { throw std::logic_error("Island::syncInstances: not implemented"); }

void Island::createInstance(const WaylandOutput& output) {
  (void)output;
  throw std::logic_error("Island::createInstance: not implemented");
}

void Island::destroyInstances() { throw std::logic_error("Island::destroyInstances: not implemented"); }

void Island::buildScene(Instance& instance, std::uint32_t width, std::uint32_t height) {
  (void)instance;
  (void)width;
  (void)height;
  throw std::logic_error("Island::buildScene: not implemented");
}

void Island::prepareFrame(Instance& instance, bool needsUpdate, bool needsLayout) {
  (void)instance;
  (void)needsUpdate;
  (void)needsLayout;
  throw std::logic_error("Island::prepareFrame: not implemented");
}

void Island::layoutCapsule(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::layoutCapsule: not implemented");
}

void Island::updateContent(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::updateContent: not implemented");
}

void Island::animatePresentation(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::animatePresentation: not implemented");
}

void Island::animateVisibility(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::animateVisibility: not implemented");
}

void Island::syncInputRegion(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::syncInputRegion: not implemented");
}

void Island::syncBlurRegion(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::syncBlurRegion: not implemented");
}

void Island::post(shell::island::Activity activity) {
  (void)activity;
  throw std::logic_error("Island::post: not implemented");
}

void Island::scheduleActivityTimer() { throw std::logic_error("Island::scheduleActivityTimer: not implemented"); }

void Island::onActivityTimer() { throw std::logic_error("Island::onActivityTimer: not implemented"); }

void Island::activateClick(Instance& instance) {
  (void)instance;
  throw std::logic_error("Island::activateClick: not implemented");
}

Island::Instance* Island::instanceForSurface(wl_surface* surface) const noexcept {
  (void)surface;
  std::abort(); // not implemented
}

float Island::capsuleWidth(const Instance& instance) const noexcept {
  (void)instance;
  std::abort(); // not implemented
}

float Island::capsuleHeight(const Instance& instance) const noexcept {
  (void)instance;
  std::abort(); // not implemented
}
