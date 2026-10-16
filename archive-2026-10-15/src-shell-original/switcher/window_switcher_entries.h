#pragma once

#include "shell/switcher/window_switcher_tile.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

class CompositorPlatform;
class IconResolver;
class WaylandConnection;
struct ToplevelInfo;
struct WorkspaceWindowAssignment;
struct wl_output;

// Window identity, metadata and activation helpers shared by the window switcher and the expo.
namespace switcher_entries {

  [[nodiscard]] std::string resolveWindowIconPath(const std::string& appId, IconResolver& iconResolver, int iconSize);
  [[nodiscard]] std::string resolveWindowAppLabel(const std::string& appId);

  [[nodiscard]] std::uintptr_t wlrHandleForToplevel(const ToplevelInfo& info) noexcept;
  [[nodiscard]] std::uintptr_t extHandleForToplevel(const ToplevelInfo& info) noexcept;

  // Compositor window ids in one spelling (Hyprland addresses with or without 0x).
  [[nodiscard]] std::string canonicalWindowId(std::string_view windowId);
  // Process-local identity key: the canonical compositor id when known, else the WLR handle.
  [[nodiscard]] std::string identityKeyForToplevelInfo(const CompositorPlatform& platform, const ToplevelInfo& info);
  [[nodiscard]] bool identityKeysEqual(std::string_view lhs, std::string_view rhs);
  // Identity key of the focused window, empty when nothing is focused.
  [[nodiscard]] std::string currentFocusedWindowKey(const CompositorPlatform& platform);

  // Focuses the window, switching workspace and output when the compositor does so on activation.
  void activateWindowSwitcherEntry(CompositorPlatform& platform, const WindowSwitcherEntry& entry);

  [[nodiscard]] std::uintptr_t resolveCloseHandle(
      const CompositorPlatform& platform, std::string_view windowId, std::string_view appId, std::string_view title
  );
  [[nodiscard]] std::uintptr_t
  resolveCaptureHandle(const WaylandConnection& wayland, std::string_view appId, std::string_view title);

  [[nodiscard]] WindowSwitcherEntry makeEntryFromToplevel(
      const CompositorPlatform& platform, IconResolver& iconResolver, int iconSize, const std::string& appId,
      const ToplevelInfo& info
  );
  [[nodiscard]] WindowSwitcherEntry makeEntryFromAssignment(
      const CompositorPlatform& platform, const WaylandConnection& wayland, IconResolver& iconResolver, int iconSize,
      const WorkspaceWindowAssignment& assignment
  );
  // Prefers the live toplevel's title and handles over the ones resolved from an assignment.
  void applyLiveToplevel(WindowSwitcherEntry& entry, const ToplevelInfo& live);

  // Live foreign toplevels on `outputFilter` (nullptr = all), keyed by identity key.
  void indexLiveToplevelsByIdentity(
      const CompositorPlatform& platform, wl_output* outputFilter, std::unordered_map<std::string, ToplevelInfo>& out
  );

} // namespace switcher_entries
