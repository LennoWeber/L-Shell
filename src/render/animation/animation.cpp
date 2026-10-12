#include "render/animation/animation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

float applyEasing(Easing easing, float t) {
  t = std::clamp(t, 0.0F, 1.0F);

  switch (easing) {
  case Easing::Linear:
    return t;

  case Easing::EaseInQuad:
    return t * t;

  case Easing::EaseOutQuad:
    return t * (2.0F - t);

  case Easing::EaseInOutQuad:
    if (t < 0.5F) {
      return 2.0F * t * t;
    }
    return -1.0F + (4.0F - 2.0F * t) * t;

  case Easing::EaseOutCubic: {
    const float f = t - 1.0F;
    return f * f * f + 1.0F;
  }

  case Easing::EaseInOutCubic:
    if (t < 0.5F) {
      return 4.0F * t * t * t;
    } else {
      const float f = 2.0F * t - 2.0F;
      return 0.5F * f * f * f + 1.0F;
    }

  case Easing::EaseOutBack: {
    constexpr float c1 = 1.70158F;
    constexpr float c3 = c1 + 1.0F;
    const float f = t - 1.0F;
    return 1.0F + c3 * f * f * f + c1 * f * f;
  }

  case Easing::EaseInCubic:
    return t * t * t;
  }

  return t;
}

SpringSample evaluateSpring(const Spring& spring, float initialVelocity, float tMs) {
  // Work in seconds-free units: ω0 in rad/ms. x is the offset from the target (x0 = −1).
  const float durationMs = std::max(spring.durationMs, 1.0F);
  const double omega = 2.0 * std::numbers::pi / static_cast<double>(durationMs);
  const double zeta = 1.0 - std::clamp(static_cast<double>(spring.bounce), 0.0, 0.9);
  const double t = std::max(0.0, static_cast<double>(tMs));
  const double x0 = -1.0;
  const double v0 = static_cast<double>(initialVelocity);

  double x = 0.0;
  double v = 0.0;
  if (zeta >= 1.0) {
    // Critically damped: x = (A + B t) e^(−ω t).
    const double a = x0;
    const double b = v0 + omega * x0;
    const double decay = std::exp(-omega * t);
    x = (a + b * t) * decay;
    v = (b - omega * (a + b * t)) * decay;
  } else {
    // Underdamped: x = e^(−ζω t)(A cos ωd t + B sin ωd t).
    const double omegaD = omega * std::sqrt(1.0 - zeta * zeta);
    const double a = x0;
    const double b = (v0 + zeta * omega * x0) / omegaD;
    const double decay = std::exp(-zeta * omega * t);
    const double c = std::cos(omegaD * t);
    const double s = std::sin(omegaD * t);
    x = decay * (a * c + b * s);
    v = decay * ((omegaD * b - zeta * omega * a) * c - (omegaD * a + zeta * omega * b) * s);
  }
  return SpringSample{.progress = static_cast<float>(1.0 + x), .velocity = static_cast<float>(v)};
}

bool springSettled(const SpringSample& sample, float epsilon) {
  // ~one 60 Hz frame of travel must also stay under epsilon so a fast pass through the target
  // is not mistaken for rest.
  constexpr float kFrameMs = 16.0F;
  return std::abs(1.0F - sample.progress) < epsilon && std::abs(sample.velocity) * kFrameMs < epsilon;
}
