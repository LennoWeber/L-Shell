#include "shell/switcher/window_switcher_apps_style.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <print>
#include <string_view>

namespace {

  bool expect(bool condition, std::string_view message) {
    if (!condition) {
      std::println(stderr, "window_switcher_apps_layout_test: {}", message);
    }
    return condition;
  }

} // namespace

int main() {
  bool ok = true;
  for (const float screenWidth : {1920.0F, 640.0F}) {
    for (std::size_t count = 0; count <= 30; ++count) {
      for (std::size_t selected = 0; selected < std::max<std::size_t>(count, 1); ++selected) {
        const WindowSwitcherStyleContext context{
            .screenWidth = screenWidth,
            .screenHeight = 1080.0F,
            .scale = 1.0F,
            .windowCount = count,
            .selectedIndex = selected,
            .showCaption = true,
            .showCount = false,
        };
        const WindowSwitcherStyleLayout layout = computeWindowSwitcherAppsLayout(context);
        const std::string label = std::format("{}px, {} windows, selected {}", screenWidth, count, selected);

        ok = expect(layout.cards.size() == count, label + ": one card per window") && ok;
        ok = expect(layout.boxed && layout.iconOnly, label + ": a boxed icon row") && ok;
        ok = expect(layout.panelWidth <= screenWidth, label + ": the panel fits the screen") && ok;
        std::size_t visible = 0;
        std::size_t captions = 0;
        float lastX = -1.0F;
        for (std::size_t i = 0; i < layout.cards.size(); ++i) {
          const WindowSwitcherCardTarget& card = layout.cards[i];
          if (!card.visible) {
            continue;
          }
          ++visible;
          captions += card.showCaption ? 1U : 0U;
          ok = expect(card.x > lastX, label + ": cards run left to right in window order") && ok;
          ok = expect(card.x + card.width <= layout.stripWidth + 0.5F, label + ": cards stay in the row") && ok;
          lastX = card.x;
        }
        if (count > 0) {
          ok = expect(layout.cards[selected].visible, label + ": the selection is always visible") && ok;
          ok = expect(
                   layout.cards[selected].depth == WindowSwitcherTileDepth::Selected,
                   label + ": the selection is highlighted"
               )
              && ok;
          ok = expect(captions == 1, label + ": only the selection is named") && ok;
          ok = expect(visible == layout.visibleCards || visible == count, label + ": visible count") && ok;
        }
      }
    }
  }
  return ok ? 0 : 1;
}
