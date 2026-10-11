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
  public:
    void configure(const GestureSettings& settings);
    // Pointer entered the handle; the owner starts its hover-dwell timer.
    void enter();
    // Pointer left with no button held; ends the hover.
    void leave();
    // The owner's hover-dwell timer fired.
    [[nodiscard]] Outcome dwellElapsed();
    void press(float x, float y);
    [[nodiscard]] Outcome motion(float x, float y);
    [[nodiscard]] Outcome release(float x, float y);
    // Grab lost (popup, focus change): forget the press without firing.
    void cancel();

    [[nodiscard]] Phase phase() const noexcept;
    [[nodiscard]] float pressX() const noexcept;
    [[nodiscard]] float pressY() const noexcept;

  private:
    GestureSettings m_settings;
    Phase m_phase = Phase::Idle;
    float m_pressX = 0.0F;
    float m_pressY = 0.0F;
    // A gesture fires at most once per hover or press; the next one needs a leave or release first.
    bool m_fired = false;
  };

} // namespace shell::gesture_handle
