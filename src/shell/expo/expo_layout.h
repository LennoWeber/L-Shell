#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

struct ExpoRect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;

  [[nodiscard]] float right() const noexcept { return x + width; }
  [[nodiscard]] float bottom() const noexcept { return y + height; }
  [[nodiscard]] float centerX() const noexcept { return x + (width * 0.5F); }
  [[nodiscard]] float centerY() const noexcept { return y + (height * 0.5F); }
};

// The workspaces of one output, shown as one section of cards.
struct ExpoSectionInput {
  std::size_t cardCount = 0;
  float aspect = 16.0F / 9.0F; // output width / height; every card keeps its monitor's shape
};

struct ExpoLayoutInput {
  float screenWidth = 0.0F;
  float screenHeight = 0.0F;
  float scale = 1.0F;
  bool showHeaders = false;
  std::vector<ExpoSectionInput> sections;
};

struct ExpoSectionLayout {
  ExpoRect header;             // zero-sized when headers are off or the section is empty
  std::vector<ExpoRect> cards; // the miniature only; its caption sits below it
  std::size_t columns = 0;
};

struct ExpoLayout {
  std::vector<ExpoSectionLayout> sections;
  float captionGap = 0.0F;    // between a card and its caption
  float captionHeight = 0.0F; // caption band below every card
};

// Lays every section's cards out in centered rows, one section under the other, choosing the
// column counts that give the largest cards that still fit the screen.
[[nodiscard]] ExpoLayout computeExpoLayout(const ExpoLayoutInput& input);

// A window in monitor-local logical pixels; width/height <= 0 when the compositor gave no geometry.
struct ExpoWindowGeometry {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

// Window miniatures inside a card of cardWidth x cardHeight showing an output of
// outputWidth x outputHeight, in card-local coordinates. Windows keep their real positions
// when every one of them has geometry; otherwise they are laid out as a tidy grid.
[[nodiscard]] std::vector<ExpoRect> placeExpoWindows(
    float cardWidth, float cardHeight, float outputWidth, float outputHeight,
    const std::vector<ExpoWindowGeometry>& windows, float scale
);

enum class ExpoDirection : std::uint8_t {
  Left,
  Right,
  Up,
  Down,
};

// The target closest to targets[from] in `direction` (rows and columns that line up win),
// or nullopt when nothing lies that way.
[[nodiscard]] std::optional<std::size_t>
expoSpatialNeighbor(const std::vector<ExpoRect>& targets, std::size_t from, ExpoDirection direction);
