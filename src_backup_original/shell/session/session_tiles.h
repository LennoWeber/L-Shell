#pragma once

#include "config/config_types.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Button;
class Flex;

// Session actions as tiles in a grid of 1px rules (Monolith): each tile its glyph over a short name, left-aligned,
// the selected one a full accent fill. The launcher sheet's foot and the session panel's hairline grid.
namespace session_tiles {

  // Glyph and name sized for a tile: a configured label or glyph wins, the built-in actions get short names.
  [[nodiscard]] std::string glyph(const SessionPanelActionConfig& action);
  [[nodiscard]] std::string label(const SessionPanelActionConfig& action);

  [[nodiscard]] std::unique_ptr<Button>
  makeTile(const SessionPanelActionConfig& action, float scale, std::function<void()> onClick);

  // Lays the tiles out `columns` wide on the rule color, a rule between and around them; a short last row is
  // padded with empty cells so every column keeps its width.
  [[nodiscard]] std::unique_ptr<Flex>
  makeGrid(std::vector<std::unique_ptr<Button>> tiles, std::size_t columns, float scale);

} // namespace session_tiles
