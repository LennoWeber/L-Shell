#include "shell/switcher/window_switcher_entries.h"

#include "compositors/compositor_detect.h"
#include "compositors/compositor_platform.h"
#include "compositors/hyprland/hyprland_window_id.h"
#include "system/app_identity.h"
#include "system/desktop_entry.h"
#include "system/icon_resolver.h"
#include "system/internal_app_metadata.h"
#include "util/string_utils.h"
#include "wayland/wayland_connection.h"

#include <unordered_set>

namespace {

  [[nodiscard]] std::optional<std::string>
  windowIdForToplevelInfo(const CompositorPlatform& platform, const ToplevelInfo& info) {
    if (info.exactIdentity && !info.identifier.empty()) {
      return info.identifier;
    }
    return platform.compositorWindowIdForToplevelInfo(info);
  }

} // namespace

namespace switcher_entries {

  [[nodiscard]] std::string resolveWindowIconPath(const std::string& appId, IconResolver& iconResolver, int iconSize) {
    if (appId.empty()) {
      return {};
    }

    if (const auto internal = internal_apps::metadataForAppId(appId);
        internal.has_value() && !internal->iconPath.empty()) {
      return internal->iconPath;
    }

    const DesktopEntry desktopEntry = app_identity::resolveRunningDesktopEntry(appId, desktopEntries());
    if (!desktopEntry.icon.empty()) {
      if (const std::string resolved = iconResolver.resolve(desktopEntry.icon, iconSize); !resolved.empty()) {
        return resolved;
      }
    }

    if (const std::string resolved = iconResolver.resolve(appId, iconSize); !resolved.empty()) {
      return resolved;
    }

    return iconResolver.resolve("application-x-executable", iconSize);
  }

  [[nodiscard]] std::string resolveWindowAppLabel(const std::string& appId) {
    if (appId.empty()) {
      return {};
    }
    const DesktopEntry desktopEntry = app_identity::resolveRunningDesktopEntry(appId, desktopEntries());
    if (!desktopEntry.name.empty()) {
      return desktopEntry.name;
    }
    return appId;
  }

  [[nodiscard]] std::uintptr_t wlrHandleForToplevel(const ToplevelInfo& info) noexcept {
    if (info.handle != nullptr) {
      return reinterpret_cast<std::uintptr_t>(info.handle);
    }
    return 0;
  }

  [[nodiscard]] std::uintptr_t extHandleForToplevel(const ToplevelInfo& info) noexcept {
    if (info.extHandle != nullptr) {
      return reinterpret_cast<std::uintptr_t>(info.extHandle);
    }
    return 0;
  }

  [[nodiscard]] std::string canonicalWindowId(std::string_view windowId) {
    if (windowId.empty()) {
      return {};
    }
    if (compositors::isHyprland()) {
      if (const std::string normalized = compositors::hyprland::normalizeWindowId(windowId); !normalized.empty()) {
        return normalized;
      }
    }
    return std::string(windowId);
  }

  [[nodiscard]] std::string identityKeyForToplevelInfo(const CompositorPlatform& platform, const ToplevelInfo& info) {
    if (const auto windowId = windowIdForToplevelInfo(platform, info); windowId.has_value()) {
      return canonicalWindowId(*windowId);
    }
    return toplevel_identity::wlr(wlrHandleForToplevel(info));
  }

  [[nodiscard]] bool identityKeysEqual(const std::string_view lhs, const std::string_view rhs) {
    if (lhs == rhs) {
      return true;
    }
    if (toplevel_identity::isWlr(lhs) || toplevel_identity::isWlr(rhs)) {
      return false;
    }
    return compositors::isHyprland() && compositors::hyprland::windowIdsEqual(lhs, rhs);
  }

  void activateWindowSwitcherEntry(CompositorPlatform& platform, const WindowSwitcherEntry& entry) {
    // Niri: foreign-toplevel activate does not reliably focus/scroll the column.
    if (compositors::isNiri() && !entry.windowId.empty()) {
      platform.focusCompositorWindow(entry.windowId);
      return;
    }
    // Umbriel's exact-id action preserves the switcher's pointer-warp intent without changing taskbar activation.
    if (compositors::isUmbriel() && !entry.windowId.empty()) {
      platform.focusCompositorWindow(entry.windowId, true);
      return;
    }
    // Prefer wlr-foreign-toplevel activate (same path as the taskbar). On Hyprland this
    // raises floating windows and respects cursor:no_warps / scrolling follow_focus;
    // dispatch focuswindow alone does not.
    if (entry.closeHandle != 0) {
      auto* handle = reinterpret_cast<zwlr_foreign_toplevel_handle_v1*>(entry.closeHandle);
      if (platform.containsWlrToplevelHandle(handle)) {
        platform.activateToplevel(handle);
        return;
      }
    }
    if (entry.windowId.empty()) {
      return;
    }
    if (zwlr_foreign_toplevel_handle_v1* handle = platform.toplevelHandleForCompositorWindowId(entry.windowId);
        handle != nullptr && platform.containsWlrToplevelHandle(handle)) {
      platform.activateToplevel(handle);
      return;
    }
    platform.focusCompositorWindow(entry.windowId, true);
  }

  [[nodiscard]] std::string currentFocusedWindowKey(const CompositorPlatform& platform) {
    const auto focusedId = platform.focusedCompositorWindowId();
    if (focusedId.has_value()) {
      return canonicalWindowId(*focusedId);
    }

    const auto active = platform.activeToplevel();
    if (!active.has_value() || active->handle == nullptr) {
      return {};
    }
    if (const auto mappedId = platform.compositorWindowIdForToplevel(active->handle); mappedId.has_value()) {
      return canonicalWindowId(*mappedId);
    }
    return toplevel_identity::wlr(reinterpret_cast<std::uintptr_t>(active->handle));
  }

  [[nodiscard]] std::uintptr_t resolveCloseHandle(
      const CompositorPlatform& platform, std::string_view windowId, std::string_view appId, std::string_view title
  ) {
    if (!windowId.empty()) {
      if (zwlr_foreign_toplevel_handle_v1* handle = platform.toplevelHandleForCompositorWindowId(windowId);
          handle != nullptr) {
        return reinterpret_cast<std::uintptr_t>(handle);
      }
    }

    const std::string idLower = StringUtils::toLower(appId);
    for (const auto& info : platform.windowsForApp(idLower, idLower)) {
      if (!windowId.empty()) {
        if (const auto mapped = platform.compositorWindowIdForToplevelInfo(info); mapped.has_value()
            && (compositors::isHyprland() ? compositors::hyprland::windowIdsEqual(*mapped, windowId)
                                          : *mapped == windowId)) {
          return wlrHandleForToplevel(info);
        }
      }
      if (!title.empty() && info.title == title) {
        return wlrHandleForToplevel(info);
      }
    }
    return 0;
  }

  [[nodiscard]] std::uintptr_t
  resolveCaptureHandle(const WaylandConnection& wayland, std::string_view appId, std::string_view title) {
    const std::string idLower = StringUtils::toLower(appId);
    const auto windows = wayland.extWindowsForApp(idLower, idLower);
    ext_foreign_toplevel_handle_v1* matched = nullptr;
    for (const auto& info : windows) {
      if (info.extHandle == nullptr || (!title.empty() && info.title != title)) {
        continue;
      }
      if (matched != nullptr) {
        return 0;
      }
      matched = info.extHandle;
    }
    if (matched != nullptr) {
      return reinterpret_cast<std::uintptr_t>(matched);
    }
    if (windows.size() == 1 && windows.front().extHandle != nullptr) {
      return reinterpret_cast<std::uintptr_t>(windows.front().extHandle);
    }
    return 0;
  }

  [[nodiscard]] WindowSwitcherEntry makeEntryFromToplevel(
      const CompositorPlatform& platform, IconResolver& iconResolver, int iconSize, const std::string& appId,
      const ToplevelInfo& info
  ) {
    WindowSwitcherEntry entry;
    if (const auto windowId = windowIdForToplevelInfo(platform, info); windowId.has_value()) {
      entry.windowId = *windowId;
    }
    entry.closeHandle = wlrHandleForToplevel(info);
    entry.captureHandle = extHandleForToplevel(info);
    entry.appId = info.appId.empty() ? appId : info.appId;
    entry.appLabel = resolveWindowAppLabel(entry.appId);
    entry.title = info.title.empty() ? entry.appLabel : info.title;
    entry.iconPath = resolveWindowIconPath(entry.appId, iconResolver, iconSize);
    return entry;
  }

  void applyLiveToplevel(WindowSwitcherEntry& entry, const ToplevelInfo& live) {
    if (!live.title.empty()) {
      entry.title = live.title;
    }
    if (const std::uintptr_t wlrHandle = wlrHandleForToplevel(live); wlrHandle != 0) {
      entry.closeHandle = wlrHandle;
    }
    if (const std::uintptr_t extHandle = extHandleForToplevel(live); extHandle != 0) {
      entry.captureHandle = extHandle;
    }
  }

  [[nodiscard]] WindowSwitcherEntry makeEntryFromAssignment(
      const CompositorPlatform& platform, const WaylandConnection& wayland, IconResolver& iconResolver, int iconSize,
      const WorkspaceWindowAssignment& assignment
  ) {
    WindowSwitcherEntry entry;
    if (!toplevel_identity::isWlr(assignment.windowId)) {
      entry.windowId = assignment.windowId;
    }
    entry.appId = assignment.appId;
    entry.appLabel = resolveWindowAppLabel(entry.appId);
    entry.title = !assignment.title.empty() ? assignment.title : entry.appLabel;
    entry.iconPath = resolveWindowIconPath(entry.appId, iconResolver, iconSize);
    entry.closeHandle = resolveCloseHandle(platform, entry.windowId, entry.appId, entry.title);
    entry.captureHandle = resolveCaptureHandle(wayland, entry.appId, entry.title);
    return entry;
  }

  void indexLiveToplevelsByIdentity(
      const CompositorPlatform& platform, wl_output* outputFilter, std::unordered_map<std::string, ToplevelInfo>& out
  ) {
    std::unordered_set<std::uintptr_t> seenWlrHandles;
    std::unordered_set<std::uintptr_t> seenExtHandles;

    const auto indexToplevel = [&](const ToplevelInfo& info) {
      const std::uintptr_t wlrHandle = wlrHandleForToplevel(info);
      const std::uintptr_t extHandle = extHandleForToplevel(info);
      if (wlrHandle != 0 && seenWlrHandles.contains(wlrHandle)) {
        return;
      }
      if (extHandle != 0 && seenExtHandles.contains(extHandle)) {
        return;
      }

      const std::string key = identityKeyForToplevelInfo(platform, info);
      if (key.empty()) {
        return;
      }

      if (wlrHandle != 0) {
        seenWlrHandles.insert(wlrHandle);
      }
      if (extHandle != 0) {
        seenExtHandles.insert(extHandle);
      }
      out[key] = info;
    };

    for (const auto& appId : platform.runningAppIds(outputFilter)) {
      const std::string lower = StringUtils::toLower(appId);
      for (const auto& info : platform.enrichedWindowsForApp(lower, lower, outputFilter)) {
        indexToplevel(info);
      }
    }
    for (const auto& info : platform.enrichedWindowsWithoutAppId(outputFilter)) {
      indexToplevel(info);
    }
  }
} // namespace switcher_entries
