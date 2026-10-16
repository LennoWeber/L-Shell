#include "render/animation/animation_manager.h"

#include "render/animation/motion_service.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace {
  // Under Reduce Motion, fades still run but never longer than this (HIG: prefer fades).
  constexpr float kReducedFadeMs = 150.0F;
  // At most one nominal frame of lead-in is credited on the first tick (see Entry::started).
  constexpr float kFirstFrameCreditMs = 16.0F;

  float elapsedSince(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to) {
    return std::chrono::duration<float, std::milli>(to - from).count();
  }

  std::chrono::steady_clock::duration toDuration(float ms) {
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float, std::milli>(ms)
    );
  }

  // Settle threshold in progress units: ~0.5 value units for large (pixel) ranges, never coarser
  // than 0.2% so progress-driven geometry (0..1) still lands within a fraction of a pixel.
  float springEpsilonFor(float range) {
    const float magnitude = std::abs(range);
    return magnitude > 0.0F ? std::clamp(0.5F / magnitude, 0.0005F, 0.002F) : 0.002F;
  }
} // namespace

AnimationManager::AnimationManager() { MotionService::instance().registerManager(this); }

AnimationManager::~AnimationManager() { MotionService::instance().unregisterManager(this); }

AnimationManager::Id AnimationManager::animate(
    float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
    std::function<void()> onComplete, const void* owner
) {
  return animateInternal(from, to, durationMs, easing, std::move(setter), std::move(onComplete), owner, Kind::Movement);
}

AnimationManager::Id AnimationManager::animateFade(
    float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
    std::function<void()> onComplete, const void* owner
) {
  return animateInternal(from, to, durationMs, easing, std::move(setter), std::move(onComplete), owner, Kind::Fade);
}

AnimationManager::Id AnimationManager::animateTimer(
    float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
    std::function<void()> onComplete, const void* owner
) {
  return animateInternal(from, to, durationMs, easing, std::move(setter), std::move(onComplete), owner, Kind::Timer);
}

AnimationManager::Id AnimationManager::snapTo(
    float to, std::function<void(float)>& setter, std::function<void()>& onComplete, const void* owner
) {
  if (setter) {
    setter(to);
  }
  if (!onComplete) {
    return 0;
  }
  // Keep an entry only to run onComplete on the next tick, like a real animation would;
  // callers rely on not being re-entered from inside animate().
  return push(
      Entry{
          .owner = owner,
          .kind = Kind::Movement,
          .started = true,
          .animation = Animation{
              .startValue = to,
              .endValue = to,
              .durationMs = 0.0F,
              .setter = std::move(setter),
              .onComplete = std::move(onComplete),
          },
      }
  );
}

AnimationManager::Id AnimationManager::animateInternal(
    float from, float to, float durationMs, Easing easing, std::function<void(float)> setter,
    std::function<void()> onComplete, const void* owner, Kind kind
) {
  const auto& motion = MotionService::instance();
  if (kind == Kind::Movement && motion.reduced()) {
    return snapTo(to, setter, onComplete, owner);
  }

  float effectiveDurationMs = kind == Kind::Timer ? durationMs : durationMs / motion.speed();
  if (kind == Kind::Fade && motion.reduced()) {
    effectiveDurationMs = std::min(effectiveDurationMs, kReducedFadeMs);
  }

  if (effectiveDurationMs <= 0.0F) {
    if (setter) {
      setter(to);
    }
    if (onComplete) {
      onComplete();
    }
    return 0;
  }

  const auto now = std::chrono::steady_clock::now();
  return push(
      Entry{
          .owner = owner,
          .kind = kind,
          .started = kind == Kind::Timer,
          .createdAt = now,
          .animation = Animation{
              .startValue = from,
              .endValue = to,
              .durationMs = effectiveDurationMs,
              .startedAt = now,
              .easing = easing,
              .setter = std::move(setter),
              .onComplete = std::move(onComplete),
          },
      }
  );
}

AnimationManager::Id AnimationManager::animateSpring(
    float from, float to, Spring spring, std::function<void(float)> setter, std::function<void()> onComplete,
    const void* owner, float initialVelocity
) {
  const auto& motion = MotionService::instance();
  const float range = to - from;
  if (motion.reduced() || std::abs(range) < 1.0e-6F) {
    return snapTo(to, setter, onComplete, owner);
  }

  const float timeScale = motion.speed();
  const auto now = std::chrono::steady_clock::now();
  return push(
      Entry{
          .owner = owner,
          .kind = Kind::Movement,
          .createdAt = now,
          .animation = Animation{
              .startValue = from,
              .endValue = to,
              .startedAt = now,
              .isSpring = true,
              .spring = spring,
              // value per wall ms -> progress per ms of spring time
              .springVelocity = initialVelocity / range / timeScale,
              .springEpsilon = springEpsilonFor(range),
              .timeScale = timeScale,
              .setter = std::move(setter),
              .onComplete = std::move(onComplete),
          },
      }
  );
}

AnimationManager::Id AnimationManager::push(Entry entry) {
  entry.id = m_nextId++;
  if (m_nextId == 0) {
    m_nextId = 1;
  }
  const Id id = entry.id;
  m_animations.push_back(std::move(entry));
  return id;
}

void AnimationManager::cancel(Id id) {
  std::erase_if(m_animations, [id](const Entry& e) { return e.id == id; });
}

void AnimationManager::cancelAll() { m_animations.clear(); }

void AnimationManager::reduceMotion() {
  for (auto& entry : m_animations) {
    if (entry.kind != Kind::Movement || entry.animation.finished) {
      continue;
    }

    auto& anim = entry.animation;
    if (anim.setter) {
      anim.setter(anim.endValue);
    }
    anim.startValue = anim.endValue;
    anim.durationMs = 0.0F;
    anim.isSpring = false;
  }
}

void AnimationManager::cancelForOwner(const void* owner) {
  if (owner == nullptr) {
    return;
  }
  std::erase_if(m_animations, [owner](const Entry& e) { return e.owner == owner; });
}

float AnimationManager::sampleProgress(const Animation& anim, float elapsedMs, float* outVelocity) {
  if (anim.isSpring) {
    const SpringSample sample = evaluateSpring(anim.spring, anim.springVelocity, elapsedMs * anim.timeScale);
    if (outVelocity != nullptr) {
      *outVelocity = sample.velocity * anim.timeScale;
    }
    return sample.progress;
  }

  if (anim.durationMs <= 0.0F) {
    if (outVelocity != nullptr) {
      *outVelocity = 0.0F;
    }
    return 1.0F;
  }
  const float t = std::clamp(elapsedMs / anim.durationMs, 0.0F, 1.0F);
  if (outVelocity != nullptr) {
    if (t >= 1.0F) {
      *outVelocity = 0.0F;
    } else {
      // Numeric derivative of the easing curve (progress per wall ms).
      constexpr float kDt = 0.001F;
      const float t0 = std::max(0.0F, t - kDt);
      const float t1 = std::min(1.0F, t + kDt);
      *outVelocity = (applyEasing(anim.easing, t1) - applyEasing(anim.easing, t0)) / ((t1 - t0) * anim.durationMs);
    }
  }
  return applyEasing(anim.easing, t);
}

void AnimationManager::tick(float /*deltaMs*/) {
  // Collect completed callbacks separately — onComplete may call animate()
  // which would push_back and invalidate iterators during iteration.
  std::vector<std::function<void()>> completedCallbacks;

  const auto now = std::chrono::steady_clock::now();

  for (auto& entry : m_animations) {
    auto& anim = entry.animation;
    if (anim.finished) {
      continue;
    }

    if (!entry.started) {
      entry.started = true;
      anim.startedAt = now - toDuration(std::min(elapsedSince(entry.createdAt, now), kFirstFrameCreditMs));
    }

    // Progress from wall time so a fixed duration stays correct when the compositor
    // delivers sparse `wl_surface.frame` callbacks (common right after cold boot).
    const float wallElapsedMs = elapsedSince(anim.startedAt, now);
    anim.elapsedMs = wallElapsedMs;

    float progress = 0.0F;
    if (anim.isSpring) {
      const SpringSample sample = evaluateSpring(anim.spring, anim.springVelocity, wallElapsedMs * anim.timeScale);
      progress = sample.progress;
      anim.finished = springSettled(sample, anim.springEpsilon);
    } else {
      progress = sampleProgress(anim, wallElapsedMs, nullptr);
      anim.finished = anim.durationMs <= 0.0F || wallElapsedMs >= anim.durationMs;
    }

    // Land exactly on the target, not on start + range * ~1.
    const float value = anim.finished ? anim.endValue : anim.startValue + (anim.endValue - anim.startValue) * progress;

    if (anim.setter) {
      anim.setter(value);
    }

    if (anim.finished && anim.onComplete) {
      completedCallbacks.push_back(std::move(anim.onComplete));
    }
  }

  std::erase_if(m_animations, [](const Entry& e) { return e.animation.finished; });

  for (auto& cb : completedCallbacks) {
    cb();
  }
}

bool AnimationManager::hasActive() const { return !m_animations.empty(); }

float AnimationManager::velocity(Id id) const {
  if (id == 0) {
    return 0.0F;
  }
  const auto it = std::ranges::find_if(m_animations, [id](const Entry& e) { return e.id == id; });
  if (it == m_animations.end() || it->animation.finished || it->kind == Kind::Timer) {
    return 0.0F;
  }
  const auto& anim = it->animation;
  const float elapsedMs = it->started ? elapsedSince(anim.startedAt, std::chrono::steady_clock::now()) : 0.0F;
  float progressVelocity = 0.0F;
  (void)sampleProgress(anim, elapsedMs, &progressVelocity);
  return progressVelocity * (anim.endValue - anim.startValue);
}
