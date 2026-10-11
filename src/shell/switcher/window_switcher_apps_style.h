#pragma once

#include "shell/switcher/window_switcher_style.h"

// macOS Command-Tab: one row of large app icons on a glass panel; the selected icon sits on a soft highlight with
// its name beneath. Windows that do not fit scroll through a window centered on the selection.
[[nodiscard]] WindowSwitcherStyleLayout computeWindowSwitcherAppsLayout(const WindowSwitcherStyleContext& context);
