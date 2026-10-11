#include "shell/expo/expo_layout.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

  constexpr float kMinMarginX = 48.0F;
  constexpr float kMinMarginY = 40.0F;
  constexpr float kMarginXFraction = 0.05F;
  constexpr float kMarginYFraction = 0.07F;
  constexpr float kColumnGap = 36.0F;
  constexpr float kRowGap = 22.0F;
  constexpr float kCaptionGap = 14.0F;
  constexpr float kCaptionHeight = 20.0F;
  constexpr float kSectionGap = 40.0F;
  constexpr float kHeaderHeight = 18.0F;
  constexpr float kHeaderGap = 12.0F;
  // A lone desktop should not fill the screen: Mission Control keeps space around it.
  constexpr float kMaxCardWidthFraction = 0.5F;
  constexpr float kMaxCardHeightFraction = 0.62F;
  constexpr float kMinCardHeight = 24.0F;
  constexpr float kFewerRowsMinRatio = 0.7F;

  // Grid fallback inside a card.
  constexpr float kGridPaddingFraction = 0.08F;
  constexpr float kGridGapFraction = 0.6F; // of the padding
  constexpr float kFallbackWindowAspect = 16.0F / 10.0F;
  constexpr float kMinWindowSide = 14.0F;

  struct Metrics {
    float marginX = 0.0F;
    float marginY = 0.0F;
    float columnGap = 0.0F;
    float rowGap = 0.0F;
    float captionGap = 0.0F;
    float captionHeight = 0.0F;
    float sectionGap = 0.0F;
    float headerBand = 0.0F; // header plus its gap, 0 without headers
  };

  [[nodiscard]] Metrics metricsFor(const ExpoLayoutInput& input) {
    const float s = std::max(input.scale, 0.1F);
    return Metrics{
        .marginX = std::max(kMinMarginX * s, input.screenWidth * kMarginXFraction),
        .marginY = std::max(kMinMarginY * s, input.screenHeight * kMarginYFraction),
        .columnGap = kColumnGap * s,
        .rowGap = kRowGap * s,
        .captionGap = kCaptionGap * s,
        .captionHeight = kCaptionHeight * s,
        .sectionGap = kSectionGap * s,
        .headerBand = input.showHeaders ? (kHeaderHeight + kHeaderGap) * s : 0.0F,
    };
  }

  [[nodiscard]] float sanitizedAspect(float aspect) {
    return std::isfinite(aspect) && aspect > 0.05F ? std::clamp(aspect, 0.2F, 8.0F) : 16.0F / 9.0F;
  }

  // Columns for cards of `cardWidth`, rebalanced so the last row is not left nearly empty.
  [[nodiscard]] std::size_t columnsFor(float cardWidth, float availableWidth, float gap, std::size_t count) {
    if (count == 0) {
      return 0;
    }
    const float fit = std::floor((availableWidth + gap) / (cardWidth + gap));
    const std::size_t columns = std::clamp<std::size_t>(fit >= 1.0F ? static_cast<std::size_t>(fit) : 1, 1, count);
    const std::size_t rows = (count + columns - 1) / columns;
    return (count + rows - 1) / rows;
  }

  [[nodiscard]] std::size_t rowsFor(std::size_t count, std::size_t columns) {
    return columns == 0 ? 0 : (count + columns - 1) / columns;
  }

  [[nodiscard]] float sectionHeight(std::size_t rows, float cardHeight, const Metrics& m) {
    if (rows == 0) {
      return 0.0F;
    }
    const auto rowCount = static_cast<float>(rows);
    return m.headerBand + (rowCount * (cardHeight + m.captionGap + m.captionHeight)) + ((rowCount - 1.0F) * m.rowGap);
  }

  // Total content height at a uniform card height, or +inf when a card would be wider than the screen.
  [[nodiscard]] float
  contentHeight(const ExpoLayoutInput& input, const Metrics& m, float availableWidth, float cardHeight) {
    float total = 0.0F;
    std::size_t nonEmpty = 0;
    for (const auto& section : input.sections) {
      if (section.cardCount == 0) {
        continue;
      }
      const float cardWidth = cardHeight * sanitizedAspect(section.aspect);
      if (cardWidth > availableWidth + 0.01F) {
        return std::numeric_limits<float>::infinity();
      }
      const std::size_t columns = columnsFor(cardWidth, availableWidth, m.columnGap, section.cardCount);
      total += sectionHeight(rowsFor(section.cardCount, columns), cardHeight, m);
      ++nonEmpty;
    }
    if (nonEmpty > 1) {
      total += static_cast<float>(nonEmpty - 1) * m.sectionGap;
    }
    return total;
  }

  [[nodiscard]] std::size_t
  totalRows(const ExpoLayoutInput& input, const Metrics& m, float availableWidth, float cardHeight) {
    std::size_t rows = 0;
    for (const auto& section : input.sections) {
      const float cardWidth = cardHeight * sanitizedAspect(section.aspect);
      rows += rowsFor(section.cardCount, columnsFor(cardWidth, availableWidth, m.columnGap, section.cardCount));
    }
    return rows;
  }

  [[nodiscard]] float perpendicularGap(float aStart, float aEnd, float bStart, float bEnd) {
    if (aEnd < bStart) {
      return bStart - aEnd;
    }
    if (bEnd < aStart) {
      return aStart - bEnd;
    }
    return 0.0F;
  }

} // namespace

ExpoLayout computeExpoLayout(const ExpoLayoutInput& input) {
  ExpoLayout layout;
  const Metrics m = metricsFor(input);
  layout.captionGap = m.captionGap;
  layout.captionHeight = m.captionHeight;
  layout.sections.resize(input.sections.size());

  const float availableWidth = std::max(1.0F, input.screenWidth - (2.0F * m.marginX));
  const float availableHeight = std::max(1.0F, input.screenHeight - (2.0F * m.marginY));

  float maxCardHeight = input.screenHeight * kMaxCardHeightFraction;
  bool anyCards = false;
  for (const auto& section : input.sections) {
    if (section.cardCount == 0) {
      continue;
    }
    anyCards = true;
    maxCardHeight =
        std::min(maxCardHeight, (input.screenWidth * kMaxCardWidthFraction) / sanitizedAspect(section.aspect));
  }
  if (!anyCards) {
    return layout;
  }

  // Taller cards never need fewer rows, so the fitting height is monotonic: bisect it.
  const float minCardHeight = kMinCardHeight * std::max(input.scale, 0.1F);
  float lo = minCardHeight;
  float hi = std::max(minCardHeight, maxCardHeight);
  if (contentHeight(input, m, availableWidth, hi) <= availableHeight) {
    lo = hi;
  } else {
    for (int i = 0; i < 40 && hi - lo > 0.25F; ++i) {
      const float mid = (lo + hi) * 0.5F;
      if (contentHeight(input, m, availableWidth, mid) <= availableHeight) {
        lo = mid;
      } else {
        hi = mid;
      }
    }
  }
  // Mission Control keeps desktops in a row: accept somewhat smaller cards for fewer rows.
  const std::size_t bestRows = totalRows(input, m, availableWidth, lo);
  if (bestRows > 1) {
    float fewerLo = minCardHeight;
    float fewerHi = lo;
    if (totalRows(input, m, availableWidth, fewerLo) < bestRows) {
      for (int i = 0; i < 40 && fewerHi - fewerLo > 0.25F; ++i) {
        const float mid = (fewerLo + fewerHi) * 0.5F;
        if (totalRows(input, m, availableWidth, mid) < bestRows) {
          fewerLo = mid;
        } else {
          fewerHi = mid;
        }
      }
      if (fewerLo >= lo * kFewerRowsMinRatio) {
        lo = fewerLo;
      }
    }
  }
  const float cardHeight = std::floor(lo);

  const float totalHeight = contentHeight(input, m, availableWidth, cardHeight);
  float y = std::isfinite(totalHeight) ? std::round((input.screenHeight - totalHeight) * 0.5F) : m.marginY;
  y = std::max(y, 0.0F);

  bool firstSection = true;
  for (std::size_t sectionIndex = 0; sectionIndex < input.sections.size(); ++sectionIndex) {
    const ExpoSectionInput& section = input.sections[sectionIndex];
    ExpoSectionLayout& out = layout.sections[sectionIndex];
    if (section.cardCount == 0) {
      continue;
    }
    if (!firstSection) {
      y += m.sectionGap;
    }
    firstSection = false;

    const float cardWidth = std::floor(cardHeight * sanitizedAspect(section.aspect));
    out.columns = columnsFor(cardWidth, availableWidth, m.columnGap, section.cardCount);
    const std::size_t rows = rowsFor(section.cardCount, out.columns);

    // The header lines up with the left edge of the widest row.
    const auto fullRowWidth =
        (static_cast<float>(out.columns) * cardWidth) + (static_cast<float>(out.columns - 1) * m.columnGap);
    if (input.showHeaders) {
      out.header = ExpoRect{
          .x = std::round((input.screenWidth - fullRowWidth) * 0.5F),
          .y = y,
          .width = fullRowWidth,
          .height = m.headerBand - (kHeaderGap * std::max(input.scale, 0.1F)),
      };
      y += m.headerBand;
    }

    out.cards.reserve(section.cardCount);
    for (std::size_t row = 0; row < rows; ++row) {
      const std::size_t first = row * out.columns;
      const std::size_t inRow = std::min(out.columns, section.cardCount - first);
      const auto rowWidth = (static_cast<float>(inRow) * cardWidth) + (static_cast<float>(inRow - 1) * m.columnGap);
      float x = std::round((input.screenWidth - rowWidth) * 0.5F);
      for (std::size_t i = 0; i < inRow; ++i) {
        out.cards.push_back(ExpoRect{.x = x, .y = y, .width = cardWidth, .height = cardHeight});
        x += cardWidth + m.columnGap;
      }
      y += cardHeight + m.captionGap + m.captionHeight;
      if (row + 1 < rows) {
        y += m.rowGap;
      }
    }
  }
  return layout;
}

std::vector<ExpoRect> placeExpoWindows(
    float cardWidth, float cardHeight, float outputWidth, float outputHeight,
    const std::vector<ExpoWindowGeometry>& windows, float scale
) {
  std::vector<ExpoRect> rects(windows.size());
  if (windows.empty() || cardWidth <= 0.0F || cardHeight <= 0.0F) {
    return rects;
  }

  const float minSide = std::min({kMinWindowSide * std::max(scale, 0.1F), cardWidth, cardHeight});
  const bool realGeometry =
      outputWidth > 0.0F && outputHeight > 0.0F && std::ranges::all_of(windows, [](const ExpoWindowGeometry& window) {
        return window.width > 0.0F && window.height > 0.0F;
      });

  if (realGeometry) {
    const float sx = cardWidth / outputWidth;
    const float sy = cardHeight / outputHeight;
    // Clamps one axis into [0, extent], keeping at least `minSide` so tiny windows stay clickable.
    const auto fitAxis = [minSide](float start, float length, float extent, float& outStart, float& outLength) {
      float a = std::clamp(start, 0.0F, extent);
      float b = std::clamp(start + length, 0.0F, extent);
      if (b - a < minSide) {
        const float center = b > a ? (a + b) * 0.5F : std::clamp(start + (length * 0.5F), 0.0F, extent);
        a = std::clamp(center - (minSide * 0.5F), 0.0F, extent - minSide);
        b = a + minSide;
      }
      outStart = a;
      outLength = b - a;
    };
    for (std::size_t i = 0; i < windows.size(); ++i) {
      const ExpoWindowGeometry& window = windows[i];
      fitAxis(window.x * sx, window.width * sx, cardWidth, rects[i].x, rects[i].width);
      fitAxis(window.y * sy, window.height * sy, cardHeight, rects[i].y, rects[i].height);
    }
    return rects;
  }

  const float padding = std::min(cardWidth, cardHeight) * kGridPaddingFraction;
  const float gap = padding * kGridGapFraction;
  const float availableWidth = std::max(1.0F, cardWidth - (2.0F * padding));
  const float availableHeight = std::max(1.0F, cardHeight - (2.0F * padding));
  const float aspect = outputWidth > 0.0F && outputHeight > 0.0F ? outputWidth / outputHeight : kFallbackWindowAspect;

  const std::size_t count = windows.size();
  std::size_t bestColumns = 1;
  float bestWidth = 0.0F;
  for (std::size_t columns = 1; columns <= count; ++columns) {
    const std::size_t rows = (count + columns - 1) / columns;
    const float byWidth = (availableWidth - (static_cast<float>(columns - 1) * gap)) / static_cast<float>(columns);
    const float byHeight =
        ((availableHeight - (static_cast<float>(rows - 1) * gap)) / static_cast<float>(rows)) * aspect;
    const float width = std::min(byWidth, byHeight);
    if (width > bestWidth + 0.01F) {
      bestWidth = width;
      bestColumns = columns;
    }
  }
  const float tileWidth = std::max(bestWidth, minSide);
  const float tileHeight = std::max(tileWidth / aspect, minSide);
  const std::size_t rows = (count + bestColumns - 1) / bestColumns;
  const float gridHeight = (static_cast<float>(rows) * tileHeight) + (static_cast<float>(rows - 1) * gap);
  float y = (cardHeight - gridHeight) * 0.5F;
  for (std::size_t row = 0; row < rows; ++row) {
    const std::size_t first = row * bestColumns;
    const std::size_t inRow = std::min(bestColumns, count - first);
    const float rowWidth = (static_cast<float>(inRow) * tileWidth) + (static_cast<float>(inRow - 1) * gap);
    float x = (cardWidth - rowWidth) * 0.5F;
    for (std::size_t i = 0; i < inRow; ++i) {
      rects[first + i] = ExpoRect{.x = x, .y = y, .width = tileWidth, .height = tileHeight};
      x += tileWidth + gap;
    }
    y += tileHeight + gap;
  }
  return rects;
}

std::optional<std::size_t>
expoSpatialNeighbor(const std::vector<ExpoRect>& targets, std::size_t from, ExpoDirection direction) {
  if (from >= targets.size()) {
    return std::nullopt;
  }
  const ExpoRect& origin = targets[from];
  // Perpendicular misalignment costs more than distance, so a lined-up neighbor wins.
  constexpr float kMisalignmentWeight = 3.0F;
  constexpr float kMinStep = 1.0F;

  std::optional<std::size_t> best;
  float bestScore = std::numeric_limits<float>::infinity();
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (i == from) {
      continue;
    }
    const ExpoRect& target = targets[i];
    float primary = 0.0F;
    float secondary = 0.0F;
    switch (direction) {
    case ExpoDirection::Left:
      primary = origin.centerX() - target.centerX();
      secondary = perpendicularGap(origin.y, origin.bottom(), target.y, target.bottom());
      break;
    case ExpoDirection::Right:
      primary = target.centerX() - origin.centerX();
      secondary = perpendicularGap(origin.y, origin.bottom(), target.y, target.bottom());
      break;
    case ExpoDirection::Up:
      primary = origin.centerY() - target.centerY();
      secondary = perpendicularGap(origin.x, origin.right(), target.x, target.right());
      break;
    case ExpoDirection::Down:
      primary = target.centerY() - origin.centerY();
      secondary = perpendicularGap(origin.x, origin.right(), target.x, target.right());
      break;
    }
    if (primary < kMinStep) {
      continue;
    }
    const float score = primary + (secondary * kMisalignmentWeight);
    if (score < bestScore) {
      bestScore = score;
      best = i;
    }
  }
  return best;
}
