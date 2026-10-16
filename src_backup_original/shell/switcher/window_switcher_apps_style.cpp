#include "shell/switcher/window_switcher_apps_style.h"

#include "ui/style.h"

#include <algorithm>
#include <cmath>

namespace {

  // Logical px, measured against the macOS Tahoe app switcher.
  constexpr float kTileSize = 116.0F; // highlight square; the icon fills ~80% of it
  constexpr float kTileGap = 4.0F;
  constexpr float kPanelPadding = 16.0F;
  constexpr float kCaptionGap = 6.0F;
  constexpr float kMinPanelTiles = 2.6F; // room for the name under a lone icon
  constexpr std::size_t kMaxVisible = 12;
  constexpr float kMinimumLayoutExtent = 1.0F;

} // namespace

WindowSwitcherStyleLayout computeWindowSwitcherAppsLayout(const WindowSwitcherStyleContext& context) {
  WindowSwitcherStyleLayout layout;
  layout.boxed = true;
  layout.iconOnly = true;
  layout.cards.resize(context.windowCount);

  const float scale = context.scale;
  const float outerMargin = Style::controlHeightSm * scale;
  const float padding = kPanelPadding * scale;
  const float gap = kTileGap * scale;
  const float captionHeight = context.showCaption ? Style::windowSwitcherCaptionHeight * scale : 0.0F;
  const float captionGap = context.showCaption ? kCaptionGap * scale : 0.0F;

  // Shrink the tiles before dropping any on a narrow screen, down to two thirds of their size.
  const float availableWidth = std::max(kMinimumLayoutExtent, context.screenWidth - (outerMargin + padding) * 2.0F);
  const std::size_t wanted = std::clamp<std::size_t>(context.windowCount, 1, kMaxVisible);
  float tile = kTileSize * scale;
  const float wantedWidth = tile * static_cast<float>(wanted) + gap * static_cast<float>(wanted - 1);
  if (wantedWidth > availableWidth) {
    tile = std::max(
        tile * (2.0F / 3.0F), (availableWidth - gap * static_cast<float>(wanted - 1)) / static_cast<float>(wanted)
    );
  }
  const auto fitting = static_cast<std::size_t>(std::floor((availableWidth + gap) / (tile + gap)));
  layout.visibleCards = std::clamp<std::size_t>(fitting, 1, wanted);

  const float cardHeight = tile + captionGap + captionHeight;
  const float rowWidth =
      tile * static_cast<float>(layout.visibleCards) + gap * static_cast<float>(layout.visibleCards - 1);
  layout.panelWidth = std::max(rowWidth, tile * kMinPanelTiles) + padding * 2.0F;
  layout.panelHeight = cardHeight + padding * 2.0F;
  layout.stripWidth = rowWidth;
  layout.stripHeight = cardHeight;
  layout.stripX = (layout.panelWidth - rowWidth) * 0.5F;
  layout.stripY = padding;

  if (context.windowCount == 0) {
    return layout;
  }

  const std::size_t visibleCount = std::min(layout.visibleCards, context.windowCount);
  const std::size_t selected = context.selectedIndex % context.windowCount;
  // Scroll only once the selection would leave the row: the row starts at the first window like on macOS.
  std::size_t start = 0;
  if (context.windowCount > visibleCount && selected >= visibleCount - 1) {
    start = std::min(selected + 2 - visibleCount, context.windowCount - visibleCount);
  }
  for (std::size_t slot = 0; slot < visibleCount; ++slot) {
    const std::size_t windowIndex = start + slot;
    const bool isSelected = windowIndex == selected;
    WindowSwitcherCardTarget& target = layout.cards[windowIndex];
    target.visible = true;
    target.showCaption = context.showCaption && isSelected;
    target.wideCaption = false;
    target.depth = isSelected ? WindowSwitcherTileDepth::Selected : WindowSwitcherTileDepth::Neutral;
    target.x = static_cast<float>(slot) * (tile + gap);
    target.y = 0.0F;
    target.width = tile;
    target.height = cardHeight;
    target.opacity = 1.0F;
    target.direction = windowIndex < selected ? -1.0F : (windowIndex > selected ? 1.0F : 0.0F);
    target.zIndex = isSelected ? 2 : 1;
    // The name may run wider than its icon but stays on the glass, half a padding inside its edges.
    const float cardLeft = layout.stripX + target.x;
    target.captionMinX = padding * 0.5F - cardLeft;
    target.captionMaxX = layout.panelWidth - padding * 0.5F - cardLeft;
  }
  return layout;
}
