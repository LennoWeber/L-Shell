#include "shell/island/island_activity.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace shell::island {

  int priority(ActivityKind kind) noexcept {
    (void)kind;
    std::abort(); // not implemented
  }

  void ActivityTracker::configure(const IslandConfig& config) {
    assert(config.activityMs > 0 && "an activity must stay on screen for some time");
    (void)config;
    throw std::logic_error("ActivityTracker::configure: not implemented");
  }

  bool ActivityTracker::post(Activity activity) {
    // An activity has to show something: a title, or at least a level.
    assert((!activity.title.empty() || activity.level.has_value()) && "empty island activity");
    assert((!activity.level.has_value() || *activity.level >= 0.0F) && "negative activity level");
    (void)activity;
    throw std::logic_error("ActivityTracker::post: not implemented");
  }

  void ActivityTracker::setAmbient(std::optional<Activity> ambient) {
    (void)ambient;
    throw std::logic_error("ActivityTracker::setAmbient: not implemented");
  }

  void ActivityTracker::setHovered(bool hovered) {
    (void)hovered;
    throw std::logic_error("ActivityTracker::setHovered: not implemented");
  }

  bool ActivityTracker::advance(float elapsedMs) {
    assert(elapsedMs >= 0.0F && "time runs forward");
    // A transient activity is either on screen with time left or gone; never a leftover with none.
    assert((m_state.transient.has_value() || m_state.transientRemainingMs == 0.0F) && "stale activity timer");
    (void)elapsedMs;
    throw std::logic_error("ActivityTracker::advance: not implemented");
  }

  void ActivityTracker::clearTransient() { throw std::logic_error("ActivityTracker::clearTransient: not implemented"); }

  Presentation ActivityTracker::presentation() const noexcept {
    std::abort(); // not implemented
  }

  const ActivityState& ActivityTracker::state() const noexcept {
    std::abort(); // not implemented
  }

  bool ActivityTracker::enabled(ActivityKind kind) const noexcept {
    (void)kind;
    std::abort(); // not implemented
  }

} // namespace shell::island
