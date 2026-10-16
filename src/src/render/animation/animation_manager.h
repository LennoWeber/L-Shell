#pragma once

#include "render/animation/animation.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

class AnimationManager {
public:
  using Id = std::uint32_t;

  AnimationManager();
  ~AnimationManager();

  AnimationManager(const AnimationManager&) = delete;
  AnimationManager& operator=(const AnimationManager&) = delete;
  AnimationManager(AnimationManager&&) = delete;
  AnimationManager& operator=(AnimationManager&&) = delete;

  // Movement, scale, clip or size tween: snaps to `to` under Reduce Motion.
  Id animate(
      float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
      std::function<void()> onComplete = {}, const void* owner = nullptr
  );
  // Opacity-only tween: under Reduce Motion it still runs, capped to a short fade.
  Id animateFade(
      float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
      std::function<void()> onComplete = {}, const void* owner = nullptr
  );
  // Damped spring from `from` to `to`. `initialVelocity` is in value units per ms (take it from
  // velocity() of the animation being replaced to retarget without a kink). Snaps under Reduce
  // Motion; [shell.animation].speed scales the spring's time. onComplete runs once settled.
  Id animateSpring(
      float from, float to, Spring spring, std::function<void(float)> setter, std::function<void()> onComplete = {},
      const void* owner = nullptr, float initialVelocity = 0.0F
  );
  // Real elapsed-time driver: ignores global motion enable/speed. Use for timeouts,
  // and for visual effects with a deliberate fixed duration that must stay independent
  // of the global animation settings (e.g. the wallpaper crossfade).
  Id animateTimer(
      float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
      std::function<void()> onComplete = {}, const void* owner = nullptr
  );
  void cancel(Id id);
  void cancelAll();
  void reduceMotion();
  // Cancels any animations tagged with the given owner. Called from Node's destructor so that
  // animations holding a raw pointer to a scene node can never outlive their target.
  void cancelForOwner(const void* owner);
  void tick(float deltaMs);
  [[nodiscard]] bool hasActive() const;
  // Current rate of change of an active animation in value units per wall-clock ms; 0 when
  // the id is unknown or finished.
  [[nodiscard]] float velocity(Id id) const;

private:
  enum class Kind : std::uint8_t {
    Movement,
    Fade,
    Timer,
  };

  struct Entry {
    Id id = 0;
    const void* owner = nullptr;
    Kind kind = Kind::Movement;
    // Motion animations start their clock on the first tick, so a slow first frame (surface
    // creation, configure round trip) does not eat a short open animation.
    bool started = false;
    std::chrono::steady_clock::time_point createdAt;
    Animation animation;
  };

  std::vector<Entry> m_animations;
  Id m_nextId = 1;

  Id animateInternal(
      float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
      std::function<void()> onComplete, const void* owner, Kind kind
  );
  Id snapTo(float to, std::function<void(float)>& setter, std::function<void()>& onComplete, const void* owner);
  Id push(Entry entry);
  [[nodiscard]] static float sampleProgress(const Animation& anim, float elapsedMs, float* outVelocity);
};
