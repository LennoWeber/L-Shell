#include "shell/gesture_handle/handle_gesture.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace shell::gesture_handle {

  void GestureRecognizer::configure(const GestureSettings& settings) {
    // Distances must be positive, and an axis lock below 1 would let a drag commit to both axes at once.
    assert(settings.pullUpDistance > 0.0F && settings.swipeDistance > 0.0F && "gesture distances must be positive");
    assert(settings.axisLockRatio >= 1.0F && "axis lock ratio below 1 makes the axes ambiguous");
    (void)settings;
    throw std::logic_error("GestureRecognizer::configure: not implemented");
  }

  void GestureRecognizer::enter() { throw std::logic_error("GestureRecognizer::enter: not implemented"); }

  void GestureRecognizer::leave() { throw std::logic_error("GestureRecognizer::leave: not implemented"); }

  Outcome GestureRecognizer::dwellElapsed() {
    throw std::logic_error("GestureRecognizer::dwellElapsed: not implemented");
  }

  void GestureRecognizer::press(float x, float y) {
    (void)x;
    (void)y;
    throw std::logic_error("GestureRecognizer::press: not implemented");
  }

  Outcome GestureRecognizer::motion(float x, float y) {
    (void)x;
    (void)y;
    throw std::logic_error("GestureRecognizer::motion: not implemented");
  }

  Outcome GestureRecognizer::release(float x, float y) {
    (void)x;
    (void)y;
    throw std::logic_error("GestureRecognizer::release: not implemented");
  }

  void GestureRecognizer::cancel() { throw std::logic_error("GestureRecognizer::cancel: not implemented"); }

  Phase GestureRecognizer::phase() const noexcept {
    std::abort(); // not implemented
  }

  float GestureRecognizer::pressX() const noexcept {
    std::abort(); // not implemented
  }

  float GestureRecognizer::pressY() const noexcept {
    std::abort(); // not implemented
  }

} // namespace shell::gesture_handle
