#include "shell/design/window_look.h"

#include "compositors/compositor_detect.h"
#include "config/atomic_file.h"
#include "core/log.h"
#include "core/process/process.h"
#include "util/file_utils.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <string_view>
#include <vector>

namespace shell::design {

  namespace {

    constexpr Logger kLog("design");

    [[nodiscard]] bool isHexColor(std::string_view value) {
      return (value.size() == 6 || value.size() == 8)
          && std::ranges::all_of(value, [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; });
    }

    [[nodiscard]] bool isOffset(std::string_view value) {
      const auto space = value.find(' ');
      if (space == std::string_view::npos || space == 0 || space + 1 >= value.size()) {
        return false;
      }
      const auto isInt = [](std::string_view part) {
        if (!part.empty() && part.front() == '-') {
          part.remove_prefix(1);
        }
        return !part.empty()
            && std::ranges::all_of(part, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
      };
      return isInt(value.substr(0, space)) && isInt(value.substr(space + 1));
    }

    [[nodiscard]] bool isValid(const DesignWindowsConfig& look) {
      return isHexColor(look.activeBorder)
          && isHexColor(look.inactiveBorder)
          && isHexColor(look.shadowColor)
          && isHexColor(look.background)
          && isOffset(look.shadowOffset);
    }

    [[nodiscard]] std::string rgba(std::string_view hex) {
      return std::format("rgba({}{})", hex, hex.size() == 6 ? "ff" : "");
    }

  } // namespace

  std::string hyprlandLua(const DesignWindowsConfig& look) {
    if (!isValid(look)) {
      return {};
    }
    return std::format(
        "hl.config({{ general = {{ border_size = {}, gaps_in = {}, gaps_out = {}, col = {{ active_border = \"{}\", "
        "inactive_border = \"{}\" }} }}, decoration = {{ rounding = {}, shadow = {{ enabled = {}, range = {}, "
        "offset = \"{}\", color = \"{}\" }} }}, misc = {{ background_color = \"{}\" }} }})",
        look.borderSize, look.gapsIn, look.gapsOut, rgba(look.activeBorder), rgba(look.inactiveBorder), look.rounding,
        look.shadow ? "true" : "false", look.shadowRange, look.shadowOffset, rgba(look.shadowColor),
        rgba(look.background)
    );
  }

  std::string hyprlandKeywordBatch(const DesignWindowsConfig& look) {
    if (!isValid(look)) {
      return {};
    }
    return std::format(
        "keyword general:border_size {}; keyword general:gaps_in {}; keyword general:gaps_out {}; "
        "keyword general:col.active_border {}; keyword general:col.inactive_border {}; "
        "keyword decoration:rounding {}; keyword decoration:shadow:enabled {}; "
        "keyword decoration:shadow:range {}; keyword decoration:shadow:offset {}; "
        "keyword decoration:shadow:color {}; keyword misc:background_color {}",
        look.borderSize, look.gapsIn, look.gapsOut, rgba(look.activeBorder), rgba(look.inactiveBorder), look.rounding,
        look.shadow ? "true" : "false", look.shadowRange, look.shadowOffset, rgba(look.shadowColor),
        rgba(look.background)
    );
  }

  std::filesystem::path hyprlandLookPath() {
    return std::filesystem::path(FileUtils::stateDir()) / "hyprland-design.lua";
  }

  void applyWindowLook(const DesignWindowsConfig& look) {
    if (!look.apply || !compositors::isHyprland()) {
      return;
    }
    const std::string lua = hyprlandLua(look);
    if (lua.empty()) {
      kLog.warn("design window look has a malformed colour or shadow offset; leaving the compositor alone");
      return;
    }
    const std::string file = "-- Written by L-Shell for the active design; loaded last by hyprland.lua.\n" + lua + "\n";
    if (!writeTextFileAtomic(hyprlandLookPath(), file)) {
      kLog.warn("could not write {}", hyprlandLookPath().string());
    }
    // Lua config first; a Hyprland without it answers with an error, then the keyword form applies the same look.
    // Values travel as arguments, never inside the script text.
    const std::string script = "out=$(hyprctl eval \"$1\" 2>&1); case \"$out\" in ok*) ;; *) hyprctl --batch \"$2\" "
                               ">/dev/null 2>&1 ;; esac";
    if (!process::runAsync(std::vector<std::string>{"sh", "-c", script, "sh", lua, hyprlandKeywordBatch(look)})) {
      kLog.warn("could not run hyprctl to apply the design window look");
    }
  }

} // namespace shell::design
