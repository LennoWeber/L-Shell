#pragma once

#include "config/config_types.h"

#include <optional>
#include <string>
#include <vector>

// Turns the active [layout] mode into the shell components that carry it out. The bar engine receives
// synthesized BarConfigs; the island, screen edges and bottom handle receive their mode's settings.
namespace noctalia::layout {

  enum class IslandVisibility : std::uint8_t {
    Always = 0,
    // Flow: the island replaces the home bar while the active workspace has windows.
    WhileWindowsOpen = 1,
  };

  struct ResolvedLayout {
    LayoutMode mode = LayoutMode::Classic;
    // Bars the bar engine runs: the [bar.*] list in classic mode, otherwise the mode's synthesized bars.
    std::vector<BarConfig> bars;
    std::optional<IslandConfig> island;
    IslandVisibility islandVisibility = IslandVisibility::Always;
    std::optional<EdgesConfig> edges;
    std::optional<HandleConfig> handle;
    // Synthesized bar the handle reveals; empty unless the handle's reveal is "bar".
    std::string handleBarName;

    bool operator==(const ResolvedLayout&) const = default;
  };

} // namespace noctalia::layout
