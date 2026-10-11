#include "shell/workspace_occupancy.h"

#include "compositors/compositor_platform.h"

#include <stdexcept>

namespace shell {

  bool activeWorkspaceHasWindows(const CompositorPlatform& platform, wl_output* output) {
    (void)platform;
    (void)output;
    throw std::logic_error("activeWorkspaceHasWindows: not implemented");
  }

  bool showsHome(const CompositorPlatform& platform, wl_output* output) {
    (void)platform;
    (void)output;
    throw std::logic_error("showsHome: not implemented");
  }

} // namespace shell
