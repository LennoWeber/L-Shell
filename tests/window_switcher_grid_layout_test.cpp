#include "shell/switcher/window_switcher_grid_style.h"

#include <cstdio>
#include <format>
#include <print>
#include <string_view>

namespace {

  bool expect(bool condition, std::string_view message) {
    if (!condition) {
      std::println(stderr, "window_switcher_grid_layout_test: {}", message);
    }
    return condition;
  }

  [[nodiscard]] bool overlaps(const WindowSwitcherCardTarget& a, const WindowSwitcherCardTarget& b) {
    return a.x < b.x + b.width - 0.5F
        && b.x < a.x + a.width - 0.5F
        && a.y < b.y + b.height - 0.5F
        && b.y < a.y + a.height - 0.5F;
  }

} // namespace

int main() {
  bool ok = true;
  for (std::size_t count = 0; count <= 12; ++count) {
    const WindowSwitcherStyleContext context{
        .screenWidth = 1920.0F,
        .screenHeight = 1080.0F,
        .scale = 1.0F,
        .windowCount = count,
        .selectedIndex = count / 2,
    };
    const WindowSwitcherStyleLayout layout = computeWindowSwitcherGridLayout(context);
    const std::string label = std::format("{} windows", count);

    ok = expect(layout.cards.size() == count, label + ": one card per window") && ok;
    ok = expect(!layout.boxed, label + ": overview is not boxed") && ok;
    ok = expect(layout.stripY > 0.0F, label + ": band above the grid stays free for the workspace bar") && ok;
    if (count == 0) {
      continue;
    }
    ok = expect(layout.columns >= 1 && layout.columns <= count, label + ": sensible column count") && ok;

    for (std::size_t i = 0; i < count; ++i) {
      const auto& card = layout.cards[i];
      ok = expect(card.visible, label + ": every card is visible") && ok;
      ok = expect(card.width > 100.0F, label + ": cards stay readable") && ok;
      ok = expect(
               card.x >= -0.5F
                   && card.y >= -0.5F
                   && card.x + card.width <= layout.stripWidth + 0.5F
                   && card.y + card.height <= layout.stripHeight + 0.5F,
               label + ": cards stay inside the grid area"
           )
          && ok;
      const bool selected = i == context.selectedIndex;
      ok = expect(
               card.depth == (selected ? WindowSwitcherTileDepth::Selected : WindowSwitcherTileDepth::Neutral),
               label + ": only the selection is highlighted, nothing is dimmed"
           )
          && ok;
      for (std::size_t j = i + 1; j < count; ++j) {
        ok = expect(!overlaps(card, layout.cards[j]), label + ": cards do not overlap") && ok;
      }
    }
  }

  const WindowSwitcherStyleLayout single = computeWindowSwitcherGridLayout({
      .screenWidth = 1920.0F,
      .screenHeight = 1080.0F,
      .scale = 1.0F,
      .windowCount = 1,
  });
  ok = expect(single.cards.front().width <= 560.5F, "a single window is capped, not screen-sized") && ok;

  return ok ? 0 : 1;
}
