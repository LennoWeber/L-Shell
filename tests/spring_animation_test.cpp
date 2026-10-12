#include "render/animation/animation.h"
#include "render/animation/animation_manager.h"
#include "render/animation/motion_service.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace {

  bool check(bool cond, const char* msg) {
    if (!cond) {
      std::cerr << "FAIL: " << msg << '\n';
    }
    return cond;
  }

  bool near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

  void resetMotion() {
    auto& motion = MotionService::instance();
    motion.setSpeed(1.0F);
    motion.setEnabled(true);
    motion.setSystemReducedMotion(false);
  }

  float maxProgress(const Spring& spring, float v0) {
    float peak = 0.0F;
    for (float t = 0.0F; t <= 3000.0F; t += 0.5F) {
      peak = std::max(peak, evaluateSpring(spring, v0, t).progress);
    }
    return peak;
  }

  // First time the spring counts as settled with the given threshold.
  float settleTimeMs(const Spring& spring, float epsilon) {
    for (float t = 0.0F; t <= 5000.0F; t += 1.0F) {
      if (springSettled(evaluateSpring(spring, 0.0F, t), epsilon)) {
        return t;
      }
    }
    return 1.0e9F;
  }

  void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

} // namespace

int main() {
  bool ok = true;
  resetMotion();

  const Spring smooth{.durationMs = 300.0F, .bounce = 0.0F};
  const Spring snappy{.durationMs = 300.0F, .bounce = 0.15F};
  const Spring island{.durationMs = 400.0F, .bounce = 0.25F};
  const Spring bouncy{.durationMs = 500.0F, .bounce = 0.3F};

  // Endpoints: starts at 0 with the given velocity, ends at 1 at rest.
  for (const Spring& s : {smooth, snappy, island, bouncy}) {
    const SpringSample start = evaluateSpring(s, 0.0F, 0.0F);
    ok &= check(near(start.progress, 0.0F, 1.0e-6F), "spring does not start at 0");
    ok &= check(near(start.velocity, 0.0F, 1.0e-6F), "spring at rest does not start with zero velocity");
    const SpringSample moving = evaluateSpring(s, 0.01F, 0.0F);
    ok &= check(near(moving.velocity, 0.01F, 1.0e-6F), "spring does not start with the initial velocity");
    const SpringSample end = evaluateSpring(s, 0.0F, 4000.0F);
    ok &= check(near(end.progress, 1.0F, 1.0e-5F), "spring does not end at 1");
    ok &= check(near(end.velocity, 0.0F, 1.0e-6F), "spring does not come to rest");
  }

  // Critically damped (bounce 0) from rest is monotonic and never overshoots.
  {
    float previous = 0.0F;
    bool monotonic = true;
    for (float t = 0.0F; t <= 2000.0F; t += 1.0F) {
      const float p = evaluateSpring(smooth, 0.0F, t).progress;
      monotonic = monotonic && p >= previous - 1.0e-6F && p <= 1.0F + 1.0e-6F;
      previous = p;
    }
    ok &= check(monotonic, "critically damped spring is not monotonic");
  }

  // Overshoot bounds match the damping ratio (Apple: snappy ~0.6%, bouncy ~4.6%).
  {
    const float snappyOvershoot = maxProgress(snappy, 0.0F) - 1.0F;
    ok &= check(snappyOvershoot > 0.003F && snappyOvershoot < 0.01F, "snappy overshoot out of range");
    const float islandOvershoot = maxProgress(island, 0.0F) - 1.0F;
    ok &= check(islandOvershoot > 0.02F && islandOvershoot < 0.04F, "island overshoot out of range");
    const float bouncyOvershoot = maxProgress(bouncy, 0.0F) - 1.0F;
    ok &= check(bouncyOvershoot > 0.035F && bouncyOvershoot < 0.055F, "bouncy overshoot out of range");
  }

  // Analytic velocity matches the derivative of position.
  for (const Spring& s : {smooth, island}) {
    for (float t : {10.0F, 80.0F, 200.0F}) {
      constexpr float h = 0.01F;
      const float numeric =
          (evaluateSpring(s, 0.004F, t + h).progress - evaluateSpring(s, 0.004F, t - h).progress) / (2.0F * h);
      ok &= check(near(evaluateSpring(s, 0.004F, t).velocity, numeric, 1.0e-4F), "spring velocity is not dx/dt");
    }
  }

  // Perceptual duration: mostly there at `duration`, settled not long after.
  {
    ok &= check(evaluateSpring(smooth, 0.0F, 300.0F).progress > 0.95F, "smooth spring too slow at its duration");
    ok &= check(settleTimeMs(smooth, 0.002F) < 600.0F, "smooth spring settles too late");
    ok &= check(settleTimeMs(island, 0.002F) < 900.0F, "island spring settles too late");
  }

  // Manager: retargeting with velocity() carries the current velocity (no kink).
  {
    AnimationManager manager;
    float value = 0.0F;
    const auto first = manager.animateSpring(0.0F, 100.0F, snappy, [&value](float v) { value = v; });
    manager.tick(0.0F);
    sleepMs(40);
    manager.tick(0.0F);
    const float carried = manager.velocity(first);
    ok &= check(carried > 0.01F, "running spring reports no velocity");
    manager.cancel(first);
    const auto second =
        manager.animateSpring(value, 50.0F, snappy, [&value](float v) { value = v; }, {}, nullptr, carried);
    ok &= check(near(manager.velocity(second), carried, std::fabs(carried) * 1.0e-3F), "retarget lost the velocity");
    manager.cancel(second);
  }

  // Manager: a spring stops requesting frames once settled and lands exactly on the target.
  {
    AnimationManager manager;
    float value = 0.0F;
    bool completed = false;
    manager.animateSpring(
        10.0F, 250.0F, smooth, [&value](float v) { value = v; }, [&completed]() { completed = true; }
    );
    const auto start = std::chrono::steady_clock::now();
    while (manager.hasActive() && std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
      manager.tick(0.0F);
      sleepMs(8);
    }
    const float elapsedMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
    ok &= check(!manager.hasActive(), "spring never settled");
    ok &= check(completed, "spring completion did not run");
    ok &= check(value == 250.0F, "spring did not land exactly on its target");
    ok &= check(elapsedMs < 1500.0F, "spring kept animating long after it looked done");
  }

  // Speed scaling: a faster global speed settles sooner.
  {
    MotionService::instance().setSpeed(4.0F);
    AnimationManager manager;
    manager.animateSpring(0.0F, 100.0F, smooth, [](float) {});
    const auto start = std::chrono::steady_clock::now();
    while (manager.hasActive() && std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
      manager.tick(0.0F);
      sleepMs(4);
    }
    const float elapsedMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
    ok &= check(elapsedMs < 300.0F, "speed did not scale the spring");
    resetMotion();
  }

  // Reduce Motion: springs and movement snap, fades still run (shortened).
  {
    MotionService::instance().setSystemReducedMotion(true);
    AnimationManager manager;
    float moved = 0.0F;
    manager.animateSpring(0.0F, 40.0F, island, [&moved](float v) { moved = v; });
    ok &= check(moved == 40.0F, "spring did not snap under reduce motion");
    float faded = 0.0F;
    const auto fade = manager.animateFade(0.0F, 1.0F, 400.0F, Easing::Linear, [&faded](float v) { faded = v; });
    ok &= check(fade != 0 && faded == 0.0F, "fade did not run under reduce motion");
    manager.tick(0.0F);
    sleepMs(200);
    manager.tick(0.0F);
    ok &= check(faded == 1.0F && !manager.hasActive(), "reduced fade was not capped to a short fade");
    resetMotion();
  }

  // Turning Reduce Motion on mid-flight snaps movement but lets fades finish.
  {
    AnimationManager manager;
    float moved = 0.0F;
    float faded = 0.0F;
    manager.animateSpring(0.0F, 10.0F, snappy, [&moved](float v) { moved = v; });
    manager.animateFade(0.0F, 1.0F, 1000.0F, Easing::Linear, [&faded](float v) { faded = v; });
    MotionService::instance().setEnabled(false);
    ok &= check(moved == 10.0F, "running spring did not snap when motion was reduced");
    ok &= check(faded < 1.0F, "running fade snapped when motion was reduced");
    manager.cancelAll();
    resetMotion();
  }

  // EaseInCubic endpoints.
  ok &= check(applyEasing(Easing::EaseInCubic, 0.0F) == 0.0F, "EaseInCubic(0) != 0");
  ok &= check(applyEasing(Easing::EaseInCubic, 1.0F) == 1.0F, "EaseInCubic(1) != 1");
  ok &= check(applyEasing(Easing::EaseInCubic, 0.5F) < 0.5F, "EaseInCubic does not accelerate");

  resetMotion();
  return ok ? 0 : 1;
}
