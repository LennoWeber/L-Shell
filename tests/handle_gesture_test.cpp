#include "shell/gesture_handle/handle_gesture.h"
#include "tests/test_check.h"

#include <cstdlib>

using shell::gesture_handle::GestureRecognizer;
using shell::gesture_handle::GestureSettings;
using shell::gesture_handle::Outcome;
using shell::gesture_handle::Phase;

int main() {
  // Hover dwell reveals once per hover.
  {
    GestureRecognizer g;
    g.configure(GestureSettings{});
    g.enter();
    TEST_CHECK(g.phase() == Phase::Hovering);
    TEST_CHECK(g.dwellElapsed() == Outcome::Reveal);
    TEST_CHECK(g.dwellElapsed() == Outcome::None);
    g.leave();
    TEST_CHECK(g.phase() == Phase::Idle);
    g.enter();
    TEST_CHECK(g.dwellElapsed() == Outcome::Reveal);
  }
  // Pulling up past the distance reveals; the line keeps the gesture until release.
  {
    GestureRecognizer g;
    g.configure(GestureSettings{.pullUpDistance = 20.0F});
    g.enter();
    g.press(50.0F, 10.0F);
    TEST_CHECK(g.motion(51.0F, 7.0F) == Outcome::None); // within slop
    TEST_CHECK(g.motion(51.0F, 0.0F) == Outcome::None);
    TEST_CHECK(g.phase() == Phase::DraggingVertical);
    TEST_CHECK(g.motion(52.0F, -15.0F) == Outcome::Reveal);
    TEST_CHECK(g.motion(52.0F, -40.0F) == Outcome::None); // fires once
    TEST_CHECK(g.release(52.0F, -40.0F) == Outcome::None);
    TEST_CHECK(g.dwellElapsed() == Outcome::None); // spent until the pointer leaves
  }
  // Sideways drags switch workspace in their direction.
  {
    GestureRecognizer g;
    g.configure(GestureSettings{.swipeDistance = 40.0F});
    g.press(100.0F, 10.0F);
    TEST_CHECK(g.motion(70.0F, 11.0F) == Outcome::None);
    TEST_CHECK(g.phase() == Phase::DraggingHorizontal);
    TEST_CHECK(g.motion(55.0F, 12.0F) == Outcome::SwipePrevious);
    g.cancel();
    g.press(100.0F, 10.0F);
    TEST_CHECK(g.release(150.0F, 10.0F) == Outcome::Reveal); // no motion seen: it was a click
  }
  {
    GestureRecognizer g;
    g.configure(GestureSettings{.swipeDistance = 40.0F});
    g.press(100.0F, 10.0F);
    (void)g.motion(120.0F, 10.0F);
    TEST_CHECK(g.release(160.0F, 10.0F) == Outcome::SwipeNext); // completes on release
  }
  // A click without movement reveals; disabled gestures never lock their axis.
  {
    GestureRecognizer g;
    g.configure(GestureSettings{.pullUp = false, .swipe = false});
    g.press(10.0F, 10.0F);
    TEST_CHECK(g.motion(10.0F, -100.0F) == Outcome::None);
    TEST_CHECK(g.motion(200.0F, 10.0F) == Outcome::None);
    TEST_CHECK(g.phase() == Phase::Pressed);
    TEST_CHECK(g.release(10.0F, 10.0F) == Outcome::Reveal);
  }
  return EXIT_SUCCESS;
}
