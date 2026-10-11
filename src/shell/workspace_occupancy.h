#pragma once

class CompositorPlatform;
struct wl_output;

namespace shell {

  // True when the active workspace on `output` (nullptr = the focused one) holds at least one window.
  [[nodiscard]] bool activeWorkspaceHasWindows(const CompositorPlatform& platform, wl_output* output);

  // "Home" for the layout modes: an empty active workspace, or the compositor overview being open.
  [[nodiscard]] bool showsHome(const CompositorPlatform& platform, wl_output* output);

} // namespace shell
