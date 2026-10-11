#include "shell/gesture_handle/handle_gesture.h"

#include <cassert>
#include <cmath>

namespace shell::gesture_handle {

  namespace {

    // Below this travel a press is still a click; jitter must not pick an axis.
    constexpr float kDragSlop = 6.0F;

  } // namespace

  void GestureRecognizer::configure(const GestureSettings& settings) {
    // Distances must be positive, and an axis lock below 1 would let a drag commit to both axes at once.
    assert(settings.pullUpDistance > 0.0F && settings.swipeDistance > 0.0F && "gesture distances must be positive");
    assert(settings.axisLockRatio >= 1.0F && "axis lock ratio below 1 makes the axes ambiguous");
    m_settings = settings;
  }

  void GestureRecognizer::enter() {
    if (m_phase == Phase::Idle) {
      m_phase = Phase::Hovering;
      m_fired = false;
    }
  }

  void GestureRecognizer::leave() {
    // While a button is held the implicit grab keeps the drag alive outside the surface.
    if (m_phase == Phase::Hovering) {
      m_phase = Phase::Idle;
      m_fired = false;
    }
  }

  Outcome GestureRecognizer::dwellElapsed() {
    if (m_phase != Phase::Hovering || m_fired) {
      return Outcome::None;
    }
    m_fired = true;
    return Outcome::Reveal;
  }

  void GestureRecognizer::press(float x, float y) {
    m_phase = Phase::Pressed;
    m_pressX = x;
    m_pressY = y;
    m_fired = false;
  }

  Outcome GestureRecognizer::motion(float x, float y) {
    const float dx = x - m_pressX;
    const float up = m_pressY - y;
    const float adx = std::fabs(dx);
    const float ady = std::fabs(up);

    if (m_phase == Phase::Pressed) {
      if (m_settings.swipe && adx > kDragSlop && adx > ady * m_settings.axisLockRatio) {
        m_phase = Phase::DraggingHorizontal;
      } else if (m_settings.pullUp && up > kDragSlop && ady > adx * m_settings.axisLockRatio) {
        m_phase = Phase::DraggingVertical;
      } else {
        return Outcome::None;
      }
    }
    if (m_fired) {
      return Outcome::None;
    }
    if (m_phase == Phase::DraggingVertical && up >= m_settings.pullUpDistance) {
      m_fired = true;
      return Outcome::Reveal;
    }
    if (m_phase == Phase::DraggingHorizontal && adx >= m_settings.swipeDistance) {
      m_fired = true;
      return dx < 0.0F ? Outcome::SwipePrevious : Outcome::SwipeNext;
    }
    return Outcome::None;
  }

  Outcome GestureRecognizer::release(float x, float y) {
    Outcome outcome = Outcome::None;
    if (m_phase == Phase::Pressed) {
      // A plain click on the line reveals, like a tap on the iPhone home indicator area.
      if (!m_fired) {
        outcome = Outcome::Reveal;
      }
    } else if (m_phase == Phase::DraggingVertical || m_phase == Phase::DraggingHorizontal) {
      outcome = motion(x, y);
    } else {
      return Outcome::None;
    }
    // Back to hovering with the gesture spent: no dwell reveal until the pointer leaves and comes back.
    m_phase = Phase::Hovering;
    m_fired = true;
    return outcome;
  }

  void GestureRecognizer::cancel() {
    m_phase = Phase::Idle;
    m_fired = false;
  }

  Phase GestureRecognizer::phase() const noexcept { return m_phase; }

  float GestureRecognizer::pressX() const noexcept { return m_pressX; }

  float GestureRecognizer::pressY() const noexcept { return m_pressY; }

} // namespace shell::gesture_handle
