#include "cli/schema_msg.h"
#include "config/config_service.h"
#include "config/config_validate.h"
#include "config/design_presets.h"
#include "shell/design/window_look.h"
#include "tests/test_check.h"
#include "theme/builtin_palettes.h"

#include <algorithm>
#include <cstdlib>
#include <print>
#include <string>

int main() {
  const auto presets = noctalia::config::designPresets();
  TEST_CHECK(!presets.empty() && presets.front().id == "slate");

  // `design-set` offers exactly the built-in designs.
  TEST_CHECK(noctalia::cli::kMsgDesignSetChoices.size() == presets.size());
  for (const auto& preset : presets) {
    TEST_CHECK(std::ranges::contains(noctalia::cli::kMsgDesignSetChoices, preset.id));
    TEST_CHECK(noctalia::config::findDesignPreset(preset.id) == &preset);
  }
  TEST_CHECK(noctalia::config::findDesignPreset("nope") == nullptr);

  for (const auto& preset : presets) {
    const toml::table layer = noctalia::config::designPresetTable(preset.id);
    if (preset.id == "slate") {
      TEST_CHECK(layer.empty()); // the code defaults are Slate
      continue;
    }
    TEST_CHECK(!layer.empty());
    // Every layer validates without a single warning: a typo or an out-of-range value would silently fall back.
    const auto diagnostics = noctalia::config::validateMergedConfig(layer, {});
    for (const auto& entry : diagnostics.entries) {
      std::println(stderr, "{}: {}", preset.id, entry.describe());
    }
    TEST_CHECK(diagnostics.entries.empty());
    // Its palette is built in, and its window look reaches the compositor whole.
    const auto palette = layer["theme"]["builtin"].value<std::string>();
    TEST_CHECK(palette.has_value() && noctalia::theme::findBuiltinPalette(*palette) != nullptr);
    DesignConfig design;
    if (const auto* windows = layer["design"]["windows"].as_table(); windows != nullptr) {
      design.windows.rounding = (*windows)["rounding"].value_or(design.windows.rounding);
      design.windows.activeBorder = (*windows)["active_border"].value_or(design.windows.activeBorder);
      design.windows.inactiveBorder = (*windows)["inactive_border"].value_or(design.windows.inactiveBorder);
      design.windows.shadowColor = (*windows)["shadow_color"].value_or(design.windows.shadowColor);
      design.windows.shadowOffset = (*windows)["shadow_offset"].value_or(design.windows.shadowOffset);
      design.windows.background = (*windows)["background"].value_or(design.windows.background);
    }
    TEST_CHECK(!shell::design::hyprlandLua(design.windows).empty());
    TEST_CHECK(!shell::design::hyprlandKeywordBatch(design.windows).empty());
  }

  // The layer goes under the user's config: the user's value wins, the design fills the rest.
  toml::table merged;
  merged.insert_or_assign("design", toml::table{{"id", "meridian"}});
  merged.insert_or_assign("shell", toml::table{{"font_family", "Mine"}});
  noctalia::config::applyDesignLayer(merged, /*userDefinesBars=*/false);
  TEST_CHECK(merged["shell"]["font_family"].value<std::string>() == "Mine");
  TEST_CHECK(merged["theme"]["builtin"].value<std::string>() == "Meridian");
  TEST_CHECK(merged["bar"]["meridian"].as_table() != nullptr);
  // Bars from the user's own files replace the design's.
  toml::table ownBars;
  ownBars.insert_or_assign("design", toml::table{{"id", "meridian"}});
  noctalia::config::applyDesignLayer(ownBars, /*userDefinesBars=*/true);
  TEST_CHECK(ownBars["bar"].as_table() == nullptr);

  // A malformed colour never reaches the compositor.
  DesignWindowsConfig broken;
  broken.activeBorder = "red\"); os.exit(";
  TEST_CHECK(shell::design::hyprlandLua(broken).empty());
  return EXIT_SUCCESS;
}
