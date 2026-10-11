#include "config/config_types.h"
#include "shell/island/island_activity.h"
#include "tests/test_check.h"

#include <cstdlib>

using shell::island::Activity;
using shell::island::ActivityKind;
using shell::island::ActivityTracker;
using shell::island::Presentation;

int main() {
  IslandConfig config;
  config.activityMs = 1000;
  config.activities.notification = false;

  ActivityTracker tracker;
  tracker.configure(config);
  TEST_CHECK(tracker.presentation() == Presentation::Collapsed);

  // Disabled kinds are refused.
  TEST_CHECK(!tracker.post(Activity{.kind = ActivityKind::Notification, .title = "Mail"}));

  // A transient activity expands the island until it expires.
  TEST_CHECK(tracker.post(Activity{.kind = ActivityKind::Volume, .title = "Volume", .level = 0.5F}));
  TEST_CHECK(tracker.presentation() == Presentation::Expanded);
  TEST_CHECK(!tracker.advance(400.0F));
  // The same kind replaces itself and restarts the clock.
  TEST_CHECK(tracker.post(Activity{.kind = ActivityKind::Volume, .title = "Volume", .level = 0.6F}));
  TEST_CHECK(tracker.state().transientRemainingMs == 1000.0F);
  // A lower-ranked kind does not interrupt a higher-ranked one.
  TEST_CHECK(tracker.post(Activity{.kind = ActivityKind::Battery, .title = "Charging"}));
  TEST_CHECK(!tracker.post(Activity{.kind = ActivityKind::Volume, .title = "Volume"}));
  TEST_CHECK(tracker.state().transient->kind == ActivityKind::Battery);

  // Hover holds the clock; the activity expires afterwards.
  tracker.setHovered(true);
  TEST_CHECK(!tracker.advance(5000.0F));
  tracker.setHovered(false);
  TEST_CHECK(tracker.advance(1000.0F));
  TEST_CHECK(!tracker.state().transient.has_value());
  TEST_CHECK(tracker.presentation() == Presentation::Collapsed);

  // Ambient media keeps the compact indicator; hover expands when configured.
  tracker.setAmbient(Activity{.kind = ActivityKind::Media, .title = "Song"});
  TEST_CHECK(tracker.presentation() == Presentation::Compact);
  tracker.setHovered(true);
  TEST_CHECK(tracker.presentation() == Presentation::Expanded);
  config.expandOnHover = false;
  tracker.configure(config);
  TEST_CHECK(tracker.presentation() == Presentation::Compact);

  // Turning a kind off drops what it is showing.
  config.activities.media = false;
  tracker.configure(config);
  TEST_CHECK(!tracker.state().ambient.has_value());
  TEST_CHECK(shell::island::priority(ActivityKind::Battery) > shell::island::priority(ActivityKind::Volume));
  return EXIT_SUCCESS;
}
