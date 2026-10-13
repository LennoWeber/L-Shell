#pragma once

#include "config/config_types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Turns the active [layout] mode into the shell components that carry it out. The bar engine receives
// synthesized BarConfigs; the island, screen edges and bottom handle receive their mode's settings.
namespace noctalia::config {

  enum class IslandVisibility : std::uint8_t {
    Always = 0,
    // Flow: the island replaces the home bar while the active workspace has windows.
    WhileWindowsOpen = 1,
  };

  struct ResolvedLayout {
    // Classic with no bars until ConfigService resolves the first parsed config.
    LayoutMode mode = LayoutMode::Classic;
    // Bars the bar engine runs: the [bar.*] list in classic mode, otherwise the mode's synthesized bars.
    std::vector<BarConfig> bars;
    std::optional<IslandConfig> island;
    IslandVisibility islandVisibility = IslandVisibility::Always;
    std::optional<EdgesConfig> edges;
    std::optional<HandleConfig> handle;
    // Synthesized bar the handle reveals; empty unless the handle's reveal is "bar".
    std::string handleBarName;
    // Synthesized bar hovering the island unfolds (flow's home bar, island mode's own bar); empty without one.
    std::string islandBarName;

    bool operator==(const ResolvedLayout&) const = default;
  };

  // Names of the bars the layout modes synthesize; they key attached panels and `bar-*` IPC like [bar.*] names.
  inline constexpr std::string_view kSmartBarName = "smart";
  inline constexpr std::string_view kMinimalBarName = "minimal";
  inline constexpr std::string_view kHandleBarName = "handle";
  inline constexpr std::string_view kFlowBarName = "flow";
  inline constexpr std::string_view kIslandBarName = "island";

  // A bottom dock floating at least this far (logical px) above the edge leaves the handle line room in the gap
  // under it, like the home indicator under the iPad dock; a tighter always-visible dock leaves no room at all.
  inline constexpr std::int32_t kHandleCompactMinGap = 5;

  // A full BarConfig for a mode bar: the [bar.*] defaults (the macOS menu bar) with the mode's own settings on top.
  // `smartHide` hides it while the active workspace has windows (smart bar, flow's home bar, the handle's bar).
  [[nodiscard]] BarConfig synthesizeBar(const ModeBarConfig& mode, std::string_view name, bool smartHide);

  // What `config.layout.mode` puts on screen. Classic passes `config.bars` through unchanged.
  [[nodiscard]] ResolvedLayout resolveLayout(const Config& config);

  // Exclusive zone the island reserves at the top (logical px), 0 without an island or with reserve_space off.
  // A top bar without a zone of its own covers this strip too, so insets below such a bar subtract it.
  [[nodiscard]] std::int32_t islandReservedZone(const ResolvedLayout& layout, float uiScale);

} // namespace noctalia::config
