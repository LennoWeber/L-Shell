#include "shell/workspace_occupancy.h"

#include "compositors/compositor_platform.h"

#include <string>
#include <string_view>

namespace shell {

  namespace {

    [[nodiscard]] bool workspaceKeyMatchesAssignment(std::string_view assignmentKey, const Workspace& workspace) {
      if (assignmentKey.empty()) {
        return false;
      }
      if (!workspace.id.empty() && assignmentKey == workspace.id) {
        return true;
      }
      if (!workspace.name.empty() && assignmentKey == workspace.name) {
        return true;
      }
      return workspace.index > 0 && assignmentKey == std::to_string(workspace.index);
    }

  } // namespace

  bool activeWorkspaceHasWindows(const CompositorPlatform& platform, wl_output* output) {
    const auto workspaces = platform.workspaces(output);
    const Workspace* active = nullptr;
    for (const auto& workspace : workspaces) {
      if (workspace.active) {
        active = &workspace;
        break;
      }
    }
    if (active == nullptr) {
      return false;
    }

    const auto assignments = platform.workspaceWindowAssignments(output);
    for (const auto& assignment : assignments) {
      if (workspaceKeyMatchesAssignment(assignment.workspaceKey, *active)) {
        return true;
      }
    }
    if (!assignments.empty()) {
      return false;
    }
    return active->occupied;
  }

  bool showsHome(const CompositorPlatform& platform, wl_output* output) {
    if (platform.hasOverviewState() && platform.isOverviewOpen()) {
      return true;
    }
    return !activeWorkspaceHasWindows(platform, output);
  }

} // namespace shell
