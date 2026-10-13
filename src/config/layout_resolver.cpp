#include "config/layout_resolver.h"

#include "ui/style.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace noctalia::config {

  namespace {

    // Floating mode bars sit this far from the screen edge and its ends, as a rounded Tahoe pill.
    constexpr std::int32_t kFloatingMargin = 6;

  } // namespace

  BarConfig synthesizeBar(const ModeBarConfig& mode, std::string_view name, bool smartHide) {
    // Synthesized bars are addressed by name (IPC, peek, settings); a nameless or zero-height bar cannot exist.
    assert(!name.empty() && "a synthesized bar needs a name");
    assert(mode.thickness > 0 && "a mode bar needs a height");
    BarConfig bar;
    bar.name = std::string(name);
    bar.position = mode.position == ModeBarPosition::Bottom ? "bottom" : "top";
    bar.thickness = mode.thickness;
    bar.scale = mode.scale;
    bar.fontScale = mode.fontScale;

    switch (mode.background) {
    case ModeBarBackground::Transparent:
      bar.backgroundOpacity = 0.0F;
      bar.compositorBlur = false;
      bar.glass = false;
      bar.legibilityScrim = !mode.floating;
      break;
    case ModeBarBackground::Glass:
      bar.backgroundOpacity = mode.backgroundOpacity;
      bar.compositorBlur = true;
      bar.glass = true;
      break;
    case ModeBarBackground::Solid:
      bar.backgroundOpacity = mode.backgroundOpacity;
      bar.compositorBlur = false;
      bar.glass = false;
      break;
    }

    if (mode.floating) {
      const auto radius = static_cast<std::int32_t>(Style::radiusXl);
      bar.marginEdge = kFloatingMargin;
      bar.marginEnds = kFloatingMargin;
      bar.radius = radius;
      bar.radiusTopLeft = radius;
      bar.radiusTopRight = radius;
      bar.radiusBottomLeft = radius;
      bar.radiusBottomRight = radius;
      bar.shadow = true;
    }
    bar.concaveEdgeCorners = false;

    bar.reserveSpace = mode.reserveSpace;
    bar.autoHide = false;
    bar.smartAutoHide = smartHide;
    bar.autoHideEdgeReveal = mode.revealOnHover;
    bar.showOnWorkspaceSwitch = mode.showOnWorkspaceSwitch;
    bar.startWidgets = mode.startWidgets;
    bar.centerWidgets = mode.centerWidgets;
    bar.endWidgets = mode.endWidgets;
    return bar;
  }

  ResolvedLayout resolveLayout(const Config& config) {
    const LayoutConfig& layout = config.layout;
    ResolvedLayout out;
    out.mode = layout.mode;

    // The handle line sits in the gap under a bottom dock; an always-visible dock flush with the edge leaves none.
    const DockConfig& dock = config.dock;
    const bool dockCoversHandle = dock.enabled
        && dock.position == DockEdge::Bottom
        && !dock.autoHide
        && !dock.smartAutoHide
        && dock.marginEdge < kHandleCompactMinGap;
    const auto addHandle = [&out, dockCoversHandle](const HandleConfig& handle) {
      if (dockCoversHandle) {
        return;
      }
      out.handle = handle;
      if (handle.reveal == HandleReveal::Bar) {
        out.bars.push_back(synthesizeBar(handle.bar, kHandleBarName, /*smartHide=*/true));
        out.handleBarName = std::string(kHandleBarName);
      }
    };

    switch (layout.mode) {
    case LayoutMode::Classic:
      out.bars = config.bars;
      break;
    case LayoutMode::SmartBar:
      out.bars.push_back(synthesizeBar(layout.smartBar, kSmartBarName, /*smartHide=*/true));
      break;
    case LayoutMode::MinimalBar:
      out.bars.push_back(synthesizeBar(layout.minimalBar, kMinimalBarName, /*smartHide=*/false));
      break;
    case LayoutMode::Island: {
      out.island = layout.island;
      out.islandVisibility = IslandVisibility::Always;
      // Always hidden until the island unfolds it, then on the screen edge over the island's own strip.
      BarConfig bar = synthesizeBar(layout.islandBar, kIslandBarName, /*smartHide=*/false);
      bar.autoHide = true;
      bar.ignoreOtherZones = !layout.islandBar.reserveSpace && layout.islandBar.position == ModeBarPosition::Top;
      out.bars.push_back(std::move(bar));
      out.islandBarName = std::string(kIslandBarName);
      break;
    }
    case LayoutMode::Edges:
      out.edges = layout.edges;
      break;
    case LayoutMode::Handle:
      addHandle(layout.handle);
      break;
    case LayoutMode::Flow:
      out.bars.push_back(synthesizeBar(layout.flow.bar, kFlowBarName, /*smartHide=*/true));
      // Shown only on an empty workspace, where the island is hidden: the bar takes the top edge over the island's
      // reserved strip instead of hanging below it.
      out.bars.back().ignoreOtherZones =
          !layout.flow.bar.reserveSpace && layout.flow.bar.position == ModeBarPosition::Top;
      out.island = layout.flow.island;
      out.islandVisibility = IslandVisibility::WhileWindowsOpen;
      out.islandBarName = std::string(kFlowBarName);
      out.edges = layout.flow.edges;
      addHandle(layout.flow.handle);
      break;
    }
    return out;
  }

  std::int32_t islandReservedZone(const ResolvedLayout& layout, float uiScale) {
    if (!layout.island.has_value() || !layout.island->reserveSpace) {
      return 0;
    }
    const float scale = std::max(0.1F, uiScale);
    return static_cast<std::int32_t>(
        std::ceil(static_cast<float>(layout.island->marginTop + layout.island->height) * scale)
    );
  }

} // namespace noctalia::config
