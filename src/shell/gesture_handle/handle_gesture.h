#pragma once

#include <cstdint>

// Pointer gestures on the bottom handle, decided without any rendering or Wayland state. Motion keeps arriving
// outside the small handle surface while a button is held (implicit pointer grab), so a drag can travel freely.
namespace shell::gesture_handle {

  enum class Outcome : std::uint8_t {
    None,
    Reveal,        // hover dwell elapsed, or the line was pulled up
    SwipePrevious, // dragged left: previous workspace
    SwipeNext,     // dragged right: next workspace
  };

  struct GestureSettings {
    float hoverDelayMs = 250.0F;
    float pullUpDistance = 28.0F; // logical px upwards before a drag reveals
    float swipeDistance = 56.0F;  // logical px sideways before a drag switches workspace
    // A drag commits to an axis once one component exceeds the other by this factor.
    float axisLockRatio = 1.5F;
    bool pullUp = true;
    bool swipe = true;
  };

  enum class Phase : std::uint8_t {
    Idle,
    Hovering,
    Pressed,
    DraggingVertical,
    DraggingHorizontal,
  };

  class GestureRecognizer {
  private:
    GestureSettings m_settings;
    Phase m_phase = Phase::Idle;
    float m_hoverElapsedMs = 0.0F;
    float m_pressX = 0.0F;
    float m_pressY = 0.0F;
    // A gesture fires at most once per hover or press; the next one needs a leave or release first.
    bool m_fired = false;
  };

} // namespace shell::gesture_handle
