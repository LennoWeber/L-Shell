#include "theme/palette_transform.h"

#include "cpp/cam/hct.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace noctalia::theme {

  namespace {

    namespace mcu = material_color_utilities;

    // The dark elevation ladder, plus the two terminal tokens that are the background
    // itself. terminal_selection_bg and terminal_normal_black are deliberately absent:
    // a terminal needs them to stay visible against the background, not to follow it.
    constexpr std::array<std::string_view, 12> kSurfaceRamp = {{
        "background",
        "surface",
        "surface_variant",
        "surface_dim",
        "surface_bright",
        "surface_container_lowest",
        "surface_container_low",
        "surface_container",
        "surface_container_high",
        "surface_container_highest",
        "terminal_background",
        "terminal_cursor_text",
    }};

    [[nodiscard]] std::uint32_t lowerTone(std::uint32_t argb, double shift) {
      mcu::Hct hct(argb);
      hct.set_tone(std::max(0.0, hct.get_tone() - shift));
      return hct.ToInt();
    }

    // Everything expandFixedPaletteMode derives from the primary and secondary roles.
    constexpr std::array<std::string_view, 19> kAccentTokens = {{
        "source_color",
        "primary",
        "on_primary",
        "primary_container",
        "on_primary_container",
        "primary_fixed",
        "primary_fixed_dim",
        "on_primary_fixed",
        "on_primary_fixed_variant",
        "secondary",
        "on_secondary",
        "secondary_container",
        "on_secondary_container",
        "secondary_fixed",
        "secondary_fixed_dim",
        "on_secondary_fixed",
        "on_secondary_fixed_variant",
        "surface_tint",
        "inverse_primary",
    }};

    constexpr std::uint32_t kAccentTextDark = 0xff1d1d1fU;
    constexpr std::uint32_t kAccentTextLight = 0xffffffffU;

    [[nodiscard]] ::Color argbToColor(std::uint32_t argb) {
      return rgba(
          static_cast<float>((argb >> 16U) & 0xffU) / 255.0F, static_cast<float>((argb >> 8U) & 0xffU) / 255.0F,
          static_cast<float>(argb & 0xffU) / 255.0F, 1.0F
      );
    }

    // White text on every accent except the light ones (yellow, light graphite), like macOS controls.
    [[nodiscard]] std::uint32_t textOnAccent(std::uint32_t argb) {
      return mcu::Hct(argb).get_tone() > 72.0 ? kAccentTextDark : kAccentTextLight;
    }

    [[nodiscard]] std::uint32_t withTone(std::uint32_t argb, double tone) {
      mcu::Hct hct(argb);
      hct.set_tone(std::clamp(tone, 0.0, 100.0));
      return hct.ToInt();
    }

    [[nodiscard]] std::uint32_t blendArgb(std::uint32_t a, std::uint32_t b, double t) {
      const auto channel = [t](std::uint32_t x, std::uint32_t y, unsigned shift) {
        const double cx = static_cast<double>((x >> shift) & 0xffU);
        const double cy = static_cast<double>((y >> shift) & 0xffU);
        return static_cast<std::uint32_t>(std::clamp(cx + (cy - cx) * t, 0.0, 255.0) + 0.5) << shift;
      };
      return 0xff000000U | channel(a, b, 16U) | channel(a, b, 8U) | channel(a, b, 0U);
    }

    [[nodiscard]] std::uint32_t tokenOrDefault(const TokenMap& tokens, std::string_view key, std::uint32_t fallback) {
      const auto it = tokens.find(std::string(key));
      return it != tokens.end() ? it->second : fallback;
    }

    [[nodiscard]] std::uint32_t wallpaperAccentForMode(const TokenMap& tokens, double tone) {
      auto it = tokens.find("source_color");
      if (it == tokens.end()) {
        it = tokens.find("primary");
      }
      if (it == tokens.end()) {
        return withTone(0xff0a84ffU, tone);
      }
      mcu::Hct hct(it->second);
      if (hct.get_chroma() < 10.0) {
        return withTone(0xff8c8c8cU, tone); // graphite
      }
      // System accents are vivid: lift a muted wallpaper hue to their saturation before settling the tone.
      hct.set_chroma(std::max(hct.get_chroma(), 52.0));
      hct.set_tone(tone);
      return hct.ToInt();
    }

  } // namespace

  void applyPureBlackDark(TokenMap& darkTokens) {
    const auto base = darkTokens.find("surface");
    if (base == darkTokens.end()) {
      return;
    }
    // Shifting by the base tone puts `surface` on tone 0 and drops everything above it
    // by the same amount; tokens already below the base clamp to black.
    const double shift = mcu::Hct(base->second).get_tone();
    if (shift <= 0.0) {
      return;
    }
    for (const std::string_view key : kSurfaceRamp) {
      if (const auto it = darkTokens.find(std::string(key)); it != darkTokens.end()) {
        it->second = lowerTone(it->second, shift);
      }
    }
  }

  void applyPureBlackDark(GeneratedPalette& palette) {
    if (!palette.dark.empty()) {
      applyPureBlackDark(palette.dark);
    }
  }

  void applyAccentColor(TokenMap& tokens, std::uint32_t accent, bool isDark) {
    if (tokens.empty()) {
      return;
    }
    accent |= 0xff000000U;
    ::Palette basis = mapGeneratedPaletteMode(tokens);
    const std::uint32_t secondary = withTone(accent, mcu::Hct(accent).get_tone() + (isDark ? 10.0 : 8.0));
    basis.primary = argbToColor(accent);
    basis.onPrimary = argbToColor(textOnAccent(accent));
    basis.secondary = argbToColor(secondary);
    basis.onSecondary = argbToColor(textOnAccent(secondary));
    const TokenMap expanded = expandFixedPaletteMode(basis, isDark);
    for (const std::string_view key : kAccentTokens) {
      if (const auto it = expanded.find(std::string(key)); it != expanded.end()) {
        tokens[std::string(key)] = it->second;
      }
    }
    // Selected text: the accent sunk into the background, like macOS's selection highlight.
    if (const auto background = tokens.find("terminal_background"); background != tokens.end()) {
      tokens["terminal_selection_bg"] = blendArgb(accent, background->second, isDark ? 0.55 : 0.65);
      tokens["terminal_selection_fg"] =
          tokenOrDefault(tokens, "terminal_foreground", isDark ? kAccentTextLight : kAccentTextDark);
    }
  }

  void applyAccentColor(GeneratedPalette& palette, const AccentArgb& accent) {
    applyAccentColor(palette.dark, accent.dark, true);
    applyAccentColor(palette.light, accent.light, false);
  }

  AccentArgb wallpaperAccent(const GeneratedPalette& wallpaper) {
    return AccentArgb{
        .dark = wallpaperAccentForMode(wallpaper.dark, 60.0),
        .light = wallpaperAccentForMode(wallpaper.light, 52.0),
    };
  }

  void applyHighContrast(TokenMap& tokens, bool isDark) {
    for (auto& [key, value] : tokens) {
      mcu::Hct hct(value);
      double tone = hct.get_tone();

      if (key == "outline" || key == "outline_variant") {
        // Force borders to be highly visible: extremely light in dark mode, extremely dark in light mode
        tone = isDark ? std::max(tone, 80.0) : std::min(tone, 20.0);
      } else {
        // Aggressive contrast stretch for everything else:
        // Push darks darker and lights lighter to separate backgrounds from foregrounds
        if (tone < 50.0) {
          tone = std::max(0.0, tone - 20.0);
        } else {
          tone = std::min(100.0, tone + 20.0);
        }
      }

      hct.set_tone(tone);
      value = hct.ToInt();
    }
  }

  void applyHighContrast(GeneratedPalette& palette) {
    if (!palette.dark.empty()) {
      applyHighContrast(palette.dark, true);
    }
    if (!palette.light.empty()) {
      applyHighContrast(palette.light, false);
    }
  }

} // namespace noctalia::theme
