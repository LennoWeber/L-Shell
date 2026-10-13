#pragma once

#include "config/config_types.h"

#include <filesystem>
#include <string>

// The compositor half of a design: how windows look (rounding, borders, gaps, shadow, the colour behind the
// wallpaper). Hyprland only. The look is applied to the running compositor and also written to a Lua file the
// user's hyprland.lua loads last, so it survives a Hyprland config reload.
namespace shell::design {

  // Lua for Hyprland's config API (`hl.config{…}`); empty when a value is malformed (colours are RRGGBB or
  // RRGGBBAA hex, the shadow offset two integers), so nothing reaches the compositor half-formed.
  [[nodiscard]] std::string hyprlandLua(const DesignWindowsConfig& look);
  // The same look as `hyprctl --batch` keywords, for Hyprland builds without the Lua config.
  [[nodiscard]] std::string hyprlandKeywordBatch(const DesignWindowsConfig& look);
  // <state dir>/hyprland-design.lua.
  [[nodiscard]] std::filesystem::path hyprlandLookPath();
  // Writes the look file and applies the look to the running Hyprland. No-op off Hyprland or with apply = false.
  void applyWindowLook(const DesignWindowsConfig& look);

} // namespace shell::design
