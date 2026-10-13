#pragma once

#include "core/toml.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

// The built-in L-Shell designs ("systems", see DESIGN.md). Each one is a complete look and layout expressed as a
// config layer: bars and their widgets, dock, island, panel placement, palette, typeface, radii, material,
// motion, notifications and the compositor's window look. ConfigService merges the active design's layer UNDER
// the user's own config, so anything the user sets still wins.
namespace noctalia::config {

  struct DesignPreset {
    std::string_view id;   // [design].id
    std::string_view name; // proper name, shown as is
    // Background, surface, text and accent, for the picker's swatch row (0xRRGGBB).
    std::array<std::uint32_t, 4> swatches{};
    std::string_view toml; // the config layer
  };

  [[nodiscard]] std::span<const DesignPreset> designPresets();
  [[nodiscard]] const DesignPreset* findDesignPreset(std::string_view id);

  // The parsed layer of a design; empty for an unknown id or the default design (whose look is the code defaults).
  [[nodiscard]] toml::table designPresetTable(std::string_view id);

  // Puts the active design's layer under `merged` (the user's config files plus settings overrides). When the
  // user's config files define [bar.*] bars, those replace the design's bars instead of merging with them.
  void applyDesignLayer(toml::table& merged, bool userDefinesBars);

} // namespace noctalia::config
