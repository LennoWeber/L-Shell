#pragma once

#include "theme/fixed_palette.h"

#include <cstdint>

namespace noctalia::theme {

  // Re-anchors the dark surface ramp to true black: every surface-family token is
  // lowered by the tone of `surface`, so the background lands on tone 0 and the
  // elevation ladder keeps its relative steps, hue and chroma. Text, accents and
  // outlines are left alone, so contrast against the surfaces is preserved.
  //
  // Only the dark token map is touched — banding on OLED panels is a dark-background
  // problem, and a light palette has no true-black base to anchor to.
  void applyPureBlackDark(TokenMap& darkTokens);
  void applyPureBlackDark(GeneratedPalette& palette);

  void applyHighContrast(TokenMap& tokens, bool isDark);
  void applyHighContrast(GeneratedPalette& palette);

  // One accent color per mode, as opaque ARGB.
  struct AccentArgb {
    std::uint32_t dark = 0;
    std::uint32_t light = 0;
  };

  // macOS-style accent: recolors the primary and secondary families (and what derives from them, such as the
  // surface tint and the terminal selection) with `accent`, leaving every neutral, status and ANSI token alone.
  void applyAccentColor(TokenMap& tokens, std::uint32_t accent, bool isDark);
  void applyAccentColor(GeneratedPalette& palette, const AccentArgb& accent);

  // An accent from a wallpaper-generated palette: the wallpaper's key hue at the saturation and tone of the
  // system accents, so it reads like one of macOS's own. A near-grey wallpaper gives graphite.
  [[nodiscard]] AccentArgb wallpaperAccent(const GeneratedPalette& wallpaper);

} // namespace noctalia::theme
