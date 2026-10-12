#pragma once

#include <cstdint>
#include <unordered_set>

class AnimationManager;

// Global motion settings. Reduce Motion (the [shell.animation].enabled = false config, or the
// desktop's org.freedesktop.appearance reduced-motion preference) means reduce, not off:
// movement, scaling and springs snap, while opacity fades still run briefly.
class MotionService {
public:
  static MotionService& instance();

  MotionService(const MotionService&) = delete;
  MotionService& operator=(const MotionService&) = delete;

  void registerManager(AnimationManager* manager);
  void unregisterManager(AnimationManager* manager);

  void setEnabled(bool enabled);
  void setSystemReducedMotion(bool reduced);
  void setSpeed(float speed);

  // False when motion is reduced, from either source.
  [[nodiscard]] bool enabled() const noexcept { return m_enabled && !m_systemReduced; }
  [[nodiscard]] bool reduced() const noexcept { return !enabled(); }
  [[nodiscard]] float speed() const noexcept { return m_speed; }

private:
  MotionService() = default;
  void applyReducedChange(bool wasReduced);

  bool m_enabled = true;
  bool m_systemReduced = false;
  float m_speed = 1.0F;
  std::unordered_set<AnimationManager*> m_managers;
};
