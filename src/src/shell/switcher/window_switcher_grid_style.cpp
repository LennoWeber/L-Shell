#include "shell/switcher/window_switcher_grid_style.h"

#include "ui/style.h"

#include <algorithm>
#include <cmath>

namespace {

  constexpr float kMaxCardWidth = 560.0F;
  constexpr float kMinimumLayoutExtent = 1.0F;

  [[nodiscard]] float previewFrameHeight(float cardWidth, float scale) noexcept {
    const float frameInset = Style::spaceXs * scale;
    const float previewWidth = std::max(0.0F, cardWidth - frameInset * 2.0F);
    return previewWidth / Style::windowSwitcherPreviewAspect + frameInset * 2.0F;
  }

  // Inverse of previewFrameHeight: the widest card whose preview fits `frameHeight`.
  [[nodiscard]] float cardWidthForPreviewHeight(float frameHeight, float scale) noexcept {
    const float frameInset = Style::spaceXs * scale;
    return std::max(0.0F, frameHeight - frameInset * 2.0F) * Style::windowSwitcherPreviewAspect + frameInset * 2.0F;
  }

} // namespace

WindowSwitcherStyleLayout computeWindowSwitcherGridLayout(const WindowSwitcherStyleContext& context) {
  WindowSwitcherStyleLayout layout;
  layout.boxed = false;
  layout.cards.resize(context.windowCount);
  layout.visibleCards = context.windowCount;
  layout.panelWidth = std::max(kMinimumLayoutExtent, context.screenWidth);
  layout.panelHeight = std::max(kMinimumLayoutExtent, context.screenHeight);

  const float scale = context.scale;
  const float margin = Style::spaceLg * 2.0F * scale;
  const float topBand = margin + (kWindowSwitcherWorkspaceBarHeight + Style::spaceLg) * scale;
  const float gap = Style::spaceLg * 1.5F * scale;
  const float captionHeight = context.showCaption ? Style::windowSwitcherCaptionHeight * scale : 0.0F;
  const float captionGap = context.showCaption ? Style::spaceMd * scale : 0.0F;

  layout.stripX = margin;
  layout.stripY = topBand;
  layout.stripWidth = std::max(kMinimumLayoutExtent, layout.panelWidth - margin * 2.0F);
  layout.stripHeight = std::max(kMinimumLayoutExtent, layout.panelHeight - topBand - margin);

  const std::size_t count = context.windowCount;
  if (count == 0) {
    return layout;
  }

  // Pick the column count that gives the largest cards.
  std::size_t columns = 1;
  float cardWidth = 0.0F;
  for (std::size_t candidate = 1; candidate <= count; ++candidate) {
    const std::size_t rows = (count + candidate - 1) / candidate;
    const float byWidth = (layout.stripWidth - gap * static_cast<float>(candidate - 1)) / static_cast<float>(candidate);
    const float rowHeight = (layout.stripHeight - gap * static_cast<float>(rows - 1)) / static_cast<float>(rows);
    const float byHeight = cardWidthForPreviewHeight(rowHeight - captionGap - captionHeight, scale);
    const float width = std::max(0.0F, std::min({byWidth, byHeight, kMaxCardWidth * scale}));
    if (width > cardWidth + 0.5F) {
      cardWidth = width;
      columns = candidate;
    }
  }
  layout.columns = columns;

  const std::size_t rows = (count + columns - 1) / columns;
  const float cardHeight = previewFrameHeight(cardWidth, scale) + captionGap + captionHeight;
  const float gridHeight = cardHeight * static_cast<float>(rows) + gap * static_cast<float>(rows - 1);
  const float offsetY = std::max(0.0F, (layout.stripHeight - gridHeight) * 0.5F);

  for (std::size_t index = 0; index < count; ++index) {
    const std::size_t row = index / columns;
    const std::size_t column = index % columns;
    const std::size_t itemsInRow = row + 1 == rows ? count - row * columns : columns;
    const float rowWidth = cardWidth * static_cast<float>(itemsInRow) + gap * static_cast<float>(itemsInRow - 1);
    const float offsetX = std::max(0.0F, (layout.stripWidth - rowWidth) * 0.5F);
    const bool selected = index == context.selectedIndex;

    WindowSwitcherCardTarget& target = layout.cards[index];
    target.visible = true;
    target.showCaption = context.showCaption;
    target.wideCaption = true;
    target.depth = selected ? WindowSwitcherTileDepth::Selected : WindowSwitcherTileDepth::Neutral;
    target.x = std::round(offsetX + static_cast<float>(column) * (cardWidth + gap));
    target.y = std::round(offsetY + static_cast<float>(row) * (cardHeight + gap));
    target.width = cardWidth;
    target.height = cardHeight;
    target.opacity = 1.0F;
    target.zIndex = selected ? 2 : 1;
  }
  return layout;
}
