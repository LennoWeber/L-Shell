#pragma once

#include "shell/switcher/window_switcher_style.h"

// Height of the workspace bar the overview draws above the grid (logical px, before scale).
inline constexpr float kWindowSwitcherWorkspaceBarHeight = 44.0F;

// Mission-Control-style grid: every window is visible and none is dimmed. Cards are as large
// as the screen allows, rows are centered, and the band above the grid is left free for the
// workspace bar.
[[nodiscard]] WindowSwitcherStyleLayout computeWindowSwitcherGridLayout(const WindowSwitcherStyleContext& context);
