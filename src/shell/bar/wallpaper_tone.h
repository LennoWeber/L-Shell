#pragma once

#include <cstdint>
#include <string>

namespace shell::bar {

  // How bright the wallpaper is where a transparent menu bar sits. macOS picks the menu bar's text color from
  // this, not from the light/dark appearance: white on a dark wallpaper, black on a bright one.
  enum class WallpaperTone : std::uint8_t {
    Unknown,
    Dark,
    Light,
  };

  // Decodes `path` (downscaled) and averages the luminance of the strip along the top (or bottom) edge. Blocking
  // file IO and decoding: call it off the UI thread. Unknown when the image cannot be read.
  [[nodiscard]] WallpaperTone sampleWallpaperTone(const std::string& path, bool bottomEdge);

  // The tone for a mean luminance in [0, 1]; split out for tests.
  [[nodiscard]] WallpaperTone toneForLuminance(float luminance) noexcept;

} // namespace shell::bar
