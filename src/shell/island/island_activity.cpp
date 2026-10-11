#include "shell/island/island_activity.h"

#include <cstdlib>
#include <stdexcept>

namespace shell::island {

  int priority(ActivityKind kind) noexcept {
    (void)kind;
    std::abort(); // not implemented
  }

  void ActivityTracker::configure(const IslandConfig& config) {
    (void)config;
    throw std::logic_error("ActivityTracker::configure: not implemented");
  }

  bool ActivityTracker::post(Activity activity) {
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
