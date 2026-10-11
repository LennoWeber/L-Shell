#pragma once

#include "config/config_types.h"

#include <cstdint>
#include <optional>
#include <string>

// What the island shows, decided without any rendering: transient activities (an OSD event, a notification, a
// charger change) expand it for a while; a playing track is ambient and keeps a compact indicator in the pill.
namespace shell::island {

  enum class ActivityKind : std::uint8_t {
    Media,
    Volume,
    Brightness,
    Notification,
    Battery,
    System,
  };

  struct Activity {
    ActivityKind kind = ActivityKind::System;
    std::string glyph;
    std::string title;
    std::string subtitle;
    // 0..1 for level-style activities (volume, brightness, battery); drawn as an accent fill.
    std::optional<float> level;
    // Muted output, Wi-Fi off, …: the level fill and glyph render neutral instead of accent.
    bool inactive = false;
    // Volume boosted past 100 %.
    bool overLimit = false;

    bool operator==(const Activity&) const = default;
  };

  enum class Presentation : std::uint8_t {
    Collapsed, // clock only
    Compact,   // clock plus the ambient indicator (e.g. a playing track)
    Expanded,  // hovered, or a transient activity is showing
  };

  struct ActivityState {
    std::optional<Activity> transient;
    float transientRemainingMs = 0.0F;
    std::optional<Activity> ambient;
    bool hovered = false;

    bool operator==(const ActivityState&) const = default;
  };

  // Higher shows first; an incoming activity replaces the current one unless the current one ranks higher and
  // is still on screen.
  [[nodiscard]] int priority(ActivityKind kind) noexcept;

  class ActivityTracker {
  public:
    void configure(const IslandConfig& config);
    // Offers a transient activity. Returns false when its kind is turned off or a higher-ranked one is showing.
    bool post(Activity activity);
    // Long-lived content (a playing track); nullopt clears it.
    void setAmbient(std::optional<Activity> ambient);
    void setHovered(bool hovered);
    // Advances time; returns true when the transient activity expired during this step. Hover holds the clock.
    bool advance(float elapsedMs);
    void clearTransient();

    [[nodiscard]] Presentation presentation() const noexcept;
    [[nodiscard]] const ActivityState& state() const noexcept;
    [[nodiscard]] bool enabled(ActivityKind kind) const noexcept;

  private:
    IslandConfig m_config;
    ActivityState m_state;
  };

} // namespace shell::island
