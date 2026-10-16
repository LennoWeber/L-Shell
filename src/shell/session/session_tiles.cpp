#include "shell/session/session_tiles.h"

#include "i18n/i18n.h"
#include "shell/session/session_action_meta.h"
#include "ui/builders.h"
#include "ui/controls/button.h"
#include "ui/controls/flex.h"
#include "ui/palette.h"
#include "ui/style.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace session_tiles {

  namespace {

    constexpr float kGlyphSize = 20.0F;
    constexpr float kFontSize = 12.0F;
    constexpr float kPaddingV = 12.0F;
    constexpr float kPaddingH = 10.0F;
    constexpr float kGap = 6.0F;

    [[nodiscard]] bool isSleep(std::string_view action) { return action == "suspend" || action == "lock_and_suspend"; }

    [[nodiscard]] const char* shortLabelKey(std::string_view action) {
      if (action == "lock") {
        return "session.short-actions.lock";
      }
      if (isSleep(action)) {
        return "session.short-actions.sleep";
      }
      if (action == "logout") {
        return "session.short-actions.logout";
      }
      if (action == "reboot") {
        return "session.short-actions.reboot";
      }
      if (action == "shutdown") {
        return "session.short-actions.shutdown";
      }
      return session_action::labelKey(action);
    }

    // Transparent until pointed at; the keyboard selection (and a running countdown) is the accent fill.
    [[nodiscard]] Button::ButtonPalette tilePalette() {
      const auto state = [](ColorSpec bg, ColorSpec label) {
        return Button::ButtonStateColors{.bg = bg, .border = clearColorSpec(), .label = label};
      };
      const Button::ButtonStateColors accent =
          state(colorSpecFromRole(ColorRole::Primary), colorSpecFromRole(ColorRole::OnPrimary));
      return Button::ButtonPalette{
          .borderWidth = 0.0F,
          .normal = state(colorSpecFromRole(ColorRole::Surface), colorSpecFromRole(ColorRole::OnSurface)),
          .hover = state(colorSpecFromRole(ColorRole::Hover), colorSpecFromRole(ColorRole::OnHover)),
          .pressed = accent,
          .disabled =
              state(colorSpecFromRole(ColorRole::Surface), colorSpecFromRole(ColorRole::OnSurfaceVariant)),
          .selected = accent,
      };
    }

    [[nodiscard]] float ruleWidth(float scale) { return std::max(1.0F, std::round(Style::borderWidth * scale)); }

  } // namespace

  std::string glyph(const SessionPanelActionConfig& action) {
    if (action.glyph.has_value() && !action.glyph->empty()) {
      return *action.glyph;
    }
    // Sleep is the moon here, as in the rest of the tile set's vocabulary.
    return isSleep(action.action) ? "moon" : session_action::defaultGlyph(action.action);
  }

  std::string label(const SessionPanelActionConfig& action) {
    if (action.label.has_value() && !action.label->empty()) {
      return *action.label;
    }
    return i18n::tr(shortLabelKey(action.action));
  }

  std::unique_ptr<Button>
  makeTile(const SessionPanelActionConfig& action, float scale, std::function<void()> onClick) {
    return ui::button({
        .text = label(action),
        .glyph = glyph(action),
        .fontSize = kFontSize * scale,
        .glyphSize = kGlyphSize * scale,
        .contentAlign = ButtonContentAlign::Start,
        .customPalette = tilePalette(),
        .paddingV = kPaddingV * scale,
        .paddingH = kPaddingH * scale,
        .gap = kGap * scale,
        .radius = 0.0F,
        .flexGrow = 1.0F,
        .onClick = std::move(onClick),
        .configure =
            [](Button& tile) {
              tile.setDirection(FlexDirection::Vertical);
              tile.setAlign(FlexAlign::Start);
              tile.setJustify(FlexJustify::Start);
              tile.setTabStop(false);
            },
    });
  }

  std::unique_ptr<Flex> makeGrid(std::vector<std::unique_ptr<Button>> tiles, std::size_t columns, float scale) {
    const float rule = ruleWidth(scale);
    columns = std::max<std::size_t>(1, columns);
    auto grid = ui::column({
        .align = FlexAlign::Stretch,
        .gap = rule,
        .padding = rule,
        .fill = colorSpecFromRole(ColorRole::Outline),
    });
    for (std::size_t start = 0; start < tiles.size(); start += columns) {
      auto row = ui::row({.align = FlexAlign::Stretch, .gap = rule});
      for (std::size_t column = 0; column < columns; ++column) {
        const std::size_t index = start + column;
        if (index < tiles.size()) {
          row->addChild(std::move(tiles[index]));
        } else {
          row->addChild(ui::box({.fill = colorSpecFromRole(ColorRole::Surface), .flexGrow = 1.0F}));
        }
      }
      grid->addChild(std::move(row));
    }
    return grid;
  }

} // namespace session_tiles
