#include "shell/island/island_activity.h"

#include <algorithm>
#include <cassert>

namespace shell::island {

  int priority(ActivityKind kind) noexcept {
    switch (kind) {
    case ActivityKind::Battery:
      return 5;
    case ActivityKind::Notification:
      return 4;
    case ActivityKind::Volume:
    case ActivityKind::Brightness:
      return 3;
    case ActivityKind::System:
      return 2;
    case ActivityKind::Media:
      return 1;
    }
    return 0;
  }

  void ActivityTracker::configure(const IslandConfig& config) {
    assert(config.activityMs > 0 && "an activity must stay on screen for some time");
    m_config = config;
    if (m_state.transient.has_value() && !enabled(m_state.transient->kind)) {
      clearTransient();
    }
    if (m_state.ambient.has_value() && !enabled(m_state.ambient->kind)) {
      m_state.ambient.reset();
    }
  }

  bool ActivityTracker::post(Activity activity) {
    // An activity has to show something: a title, or at least a level.
    assert((!activity.title.empty() || activity.level.has_value()) && "empty island activity");
    assert((!activity.level.has_value() || *activity.level >= 0.0F) && "negative activity level");
    if (!enabled(activity.kind)) {
      return false;
    }
    if (m_state.transient.has_value()
        && m_state.transient->kind != activity.kind
        && priority(m_state.transient->kind) > priority(activity.kind)) {
      return false;
    }
    m_state.transient = std::move(activity);
    m_state.transientRemainingMs = static_cast<float>(std::max(m_config.activityMs, 1));
    return true;
  }

  void ActivityTracker::setAmbient(std::optional<Activity> ambient) {
    if (ambient.has_value() && !enabled(ambient->kind)) {
      ambient.reset();
    }
    m_state.ambient = std::move(ambient);
  }

  void ActivityTracker::setHovered(bool hovered) { m_state.hovered = hovered; }

  bool ActivityTracker::advance(float elapsedMs) {
    assert(elapsedMs >= 0.0F && "time runs forward");
    // A transient activity is either on screen with time left or gone; never a leftover with none.
    assert((m_state.transient.has_value() || m_state.transientRemainingMs == 0.0F) && "stale activity timer");
    if (!m_state.transient.has_value() || m_state.hovered) {
      return false;
    }
    m_state.transientRemainingMs -= std::max(elapsedMs, 0.0F);
    if (m_state.transientRemainingMs > 0.0F) {
      return false;
    }
    clearTransient();
    return true;
  }

  void ActivityTracker::clearTransient() {
    m_state.transient.reset();
    m_state.transientRemainingMs = 0.0F;
  }

  Presentation ActivityTracker::presentation() const noexcept {
    if (m_state.transient.has_value() || (m_state.hovered && m_config.expandOnHover)) {
      return Presentation::Expanded;
    }
    if (m_state.ambient.has_value()) {
      return Presentation::Compact;
    }
    return Presentation::Collapsed;
  }

  const ActivityState& ActivityTracker::state() const noexcept { return m_state; }

  bool ActivityTracker::enabled(ActivityKind kind) const noexcept {
    const auto& activities = m_config.activities;
    switch (kind) {
    case ActivityKind::Media:
      return activities.media;
    case ActivityKind::Volume:
      return activities.volume;
    case ActivityKind::Brightness:
      return activities.brightness;
    case ActivityKind::Notification:
      return activities.notification;
    case ActivityKind::Battery:
      return activities.battery;
    case ActivityKind::System:
      return activities.system;
    }
    return false;
  }

} // namespace shell::island
