#include "render/animation/motion_service.h"

#include "render/animation/animation_manager.h"

#include <algorithm>
#include <vector>

MotionService& MotionService::instance() {
  static MotionService service;
  return service;
}

void MotionService::registerManager(AnimationManager* manager) {
  if (manager == nullptr) {
    return;
  }
  m_managers.insert(manager);
}

void MotionService::unregisterManager(AnimationManager* manager) {
  if (manager == nullptr) {
    return;
  }
  m_managers.erase(manager);
}

void MotionService::setEnabled(bool enabled) {
  if (m_enabled == enabled) {
    return;
  }
  const bool wasReduced = reduced();
  m_enabled = enabled;
  applyReducedChange(wasReduced);
}

void MotionService::setSystemReducedMotion(bool reduced) {
  if (m_systemReduced == reduced) {
    return;
  }
  const bool wasReduced = this->reduced();
  m_systemReduced = reduced;
  applyReducedChange(wasReduced);
}

void MotionService::applyReducedChange(bool wasReduced) {
  if (!wasReduced && reduced()) {
    const std::vector<AnimationManager*> managers(m_managers.begin(), m_managers.end());
    for (auto* manager : managers) {
      if (manager != nullptr && m_managers.contains(manager)) {
        manager->reduceMotion();
      }
    }
  }
}

void MotionService::setSpeed(float speed) { m_speed = std::clamp(speed, 0.05F, 4.0F); }
