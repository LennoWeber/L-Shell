#include "shell/bar/wallpaper_tone.h"

#include "render/core/image_file_loader.h"

#include <algorithm>
#include <cmath>

namespace shell::bar {

  namespace {
    // The decode is downscaled to this size along its longer side: plenty to average a strip.
    constexpr int kSampleSize = 256;
    // The strip the bar covers, as a fraction of the image height (a ~32 px bar on a 1080p screen is 3%; a little
    // more covers the scrim that fades below it).
    constexpr float kStripFraction = 0.06F;
    // Above this mean luminance, white text loses contrast and black reads better.
    constexpr float kLightThreshold = 0.6F;
  } // namespace

  WallpaperTone toneForLuminance(float luminance) noexcept {
    return luminance > kLightThreshold ? WallpaperTone::Light : WallpaperTone::Dark;
  }

  WallpaperTone sampleWallpaperTone(const std::string& path, bool bottomEdge) {
    if (path.empty()) {
      return WallpaperTone::Unknown;
    }
    const auto loaded = loadImageFile(path, kSampleSize);
    if (!loaded
        || loaded->width <= 0
        || loaded->height <= 0
        || loaded->rgba.size() < static_cast<std::size_t>(loaded->width) * loaded->height * 4) {
      return WallpaperTone::Unknown;
    }
    const int width = loaded->width;
    const int height = loaded->height;
    const int rows = std::clamp(static_cast<int>(std::ceil(static_cast<float>(height) * kStripFraction)), 1, height);
    const int firstRow = bottomEdge ? height - rows : 0;

    double sum = 0.0;
    double weight = 0.0;
    for (int y = firstRow; y < firstRow + rows; ++y) {
      const std::uint8_t* row = loaded->rgba.data() + static_cast<std::size_t>(y) * width * 4;
      for (int x = 0; x < width; ++x) {
        const std::uint8_t* px = row + static_cast<std::size_t>(x) * 4;
        const double alpha = px[3] / 255.0;
        // Rec. 709 luma on the stored (gamma-encoded) values: close to how bright the strip looks.
        sum += alpha * (0.2126 * px[0] + 0.7152 * px[1] + 0.0722 * px[2]) / 255.0;
        weight += alpha;
      }
    }
    if (weight <= 0.0) {
      return WallpaperTone::Unknown;
    }
    return toneForLuminance(static_cast<float>(sum / weight));
  }

} // namespace shell::bar
