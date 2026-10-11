#include "config/layout_resolver.h"

#include "ui/style.h"

#include <cassert>

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

    // An always-visible dock at the bottom owns the bottom center; a handle line would sit on top of it.
    const DockConfig& dock = config.dock;
    const bool dockAlwaysAtBottom =
        dock.enabled && dock.position == DockEdge::Bottom && !dock.autoHide && !dock.smartAutoHide;
    const auto addHandle = [&out, dockAlwaysAtBottom](const HandleConfig& handle) {
      if (dockAlwaysAtBottom) {
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
    case LayoutMode::Island:
      out.island = layout.island;
      out.islandVisibility = IslandVisibility::Always;
      break;
    case LayoutMode::Edges:
      out.edges = layout.edges;
      break;
    case LayoutMode::Handle:
      addHandle(layout.handle);
      break;
    case LayoutMode::Flow:
      out.bars.push_back(synthesizeBar(layout.flow.bar, kFlowBarName, /*smartHide=*/true));
      out.island = layout.flow.island;
      out.islandVisibility = IslandVisibility::WhileWindowsOpen;
      out.edges = layout.flow.edges;
      addHandle(layout.flow.handle);
      break;
    }
    return out;
  }

} // namespace noctalia::config
