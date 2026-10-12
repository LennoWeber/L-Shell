#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

enum class Easing : std::uint8_t {
  Linear,
  EaseInQuad,
  EaseOutQuad,
  EaseInOutQuad,
  EaseOutCubic,
  EaseInOutCubic,
  EaseOutBack,
  // Accelerating exit: things leaving move away faster than they arrived.
  EaseInCubic,
};

float applyEasing(Easing easing, float t);

// Apple-style spring (WWDC23 "Animate with springs"): unit mass, stiffness (2π/duration)², damping
// 4π(1−bounce)/duration. `durationMs` is the perceptual duration, not the settle time; bounce 0 is
// critically damped, ~0.15 barely bouncy, ~0.3 noticeably bouncy.
struct Spring {
  float durationMs = 300.0F;
  float bounce = 0.0F;
};

struct SpringSample {
  float progress = 0.0F; // 0 at start, 1 at target (overshoots above 1 when bouncy)
  float velocity = 0.0F; // progress per ms
};

// Closed-form damped spring from progress 0 towards 1, starting with `initialVelocity` (progress
// per ms), evaluated `tMs` after start. Bounce is clamped to [0, 0.9]; durationMs must be > 0.
[[nodiscard]] SpringSample evaluateSpring(const Spring& spring, float initialVelocity, float tMs);

// True once the spring is within `epsilon` of the target and slow enough that the next frame
// would not move it visibly either.
[[nodiscard]] bool springSettled(const SpringSample& sample, float epsilon);

struct Animation {
  float startValue = 0.0F;
  float endValue = 0.0F;
  float durationMs = 0.0F;
  float elapsedMs = 0.0F;
  std::chrono::steady_clock::time_point startedAt;
  Easing easing = Easing::EaseOutQuad;
  // Spring mode (isSpring): `durationMs` is ignored; progress follows `spring` from
  // `springVelocity` (progress per ms of spring time) and ends once settled.
  bool isSpring = false;
  Spring spring;
  float springVelocity = 0.0F;
  float springEpsilon = 0.002F;
  // Spring time runs `timeScale` times faster than wall time ([shell.animation].speed).
  float timeScale = 1.0F;
  std::function<void(float)> setter;
  std::function<void()> onComplete;
  bool finished = false;
};
