#pragma once

#include <string_view>

struct DesktopEntry;

// The symbol an app is drawn as where the shell shows apps as glyphs (shell.app_icon_glyphs): picked by the desktop
// entry's categories, so a terminal reads as a terminal whatever its own icon looks like.
[[nodiscard]] std::string_view appGlyph(std::string_view categories);
[[nodiscard]] std::string_view appGlyph(const DesktopEntry& entry);
// The glyph of the installed app with this desktop entry id, or the generic app glyph.
[[nodiscard]] std::string_view appGlyphForDesktopId(std::string_view desktopEntryId);
