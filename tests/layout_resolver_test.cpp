#include "config/config_types.h"
#include "config/layout_resolver.h"
#include "tests/test_check.h"

#include <cstdlib>

using noctalia::config::IslandVisibility;
using noctalia::config::resolveLayout;

int main() {
  Config config;
  config.bars = {BarConfig{.name = "main"}, BarConfig{.name = "side", .position = "left"}};

  // Classic: the [bar.*] list as configured, nothing else.
  config.layout.mode = LayoutMode::Classic;
  auto classic = resolveLayout(config);
  TEST_CHECK(classic.bars.size() == 2);
  TEST_CHECK(classic.bars[0].name == "main" && classic.bars[1].name == "side");
  TEST_CHECK(!classic.island.has_value() && !classic.edges.has_value() && !classic.handle.has_value());

  // Smart bar: one synthesized bar that hides while windows are open; [bar.*] is ignored.
  config.layout.mode = LayoutMode::SmartBar;
  auto smart = resolveLayout(config);
  TEST_CHECK(smart.bars.size() == 1);
  TEST_CHECK(smart.bars[0].name == noctalia::config::kSmartBarName);
  TEST_CHECK(smart.bars[0].smartAutoHide && !smart.bars[0].autoHide);

  // Minimal bar: always shown.
  config.layout.mode = LayoutMode::MinimalBar;
  auto minimal = resolveLayout(config);
  TEST_CHECK(minimal.bars.size() == 1 && !minimal.bars[0].smartAutoHide);

  // Island and edges: no bars at all.
  config.layout.mode = LayoutMode::Island;
  auto island = resolveLayout(config);
  TEST_CHECK(island.bars.empty() && island.island.has_value());
  TEST_CHECK(island.islandVisibility == IslandVisibility::Always);
  config.layout.mode = LayoutMode::Edges;
  auto edges = resolveLayout(config);
  TEST_CHECK(edges.bars.empty() && edges.edges.has_value() && !edges.island.has_value());

  // Handle: the line sits in the gap under a floating bottom dock; a dock flush with the edge leaves no room.
  config.layout.mode = LayoutMode::Handle;
  config.dock.enabled = true;
  config.dock.position = DockEdge::Bottom;
  config.dock.smartAutoHide = false;
  config.dock.marginEdge = noctalia::config::kHandleCompactMinGap;
  TEST_CHECK(resolveLayout(config).handle.has_value());
  config.dock.marginEdge = 0;
  TEST_CHECK(!resolveLayout(config).handle.has_value());
  // With the dock hiding, the overlay reveal needs no bar; the bar reveal adds the hidden handle bar.
  config.dock.smartAutoHide = true;
  config.layout.handle.reveal = HandleReveal::Overlay;
  auto handleOverlay = resolveLayout(config);
  TEST_CHECK(handleOverlay.bars.empty() && handleOverlay.handle.has_value() && handleOverlay.handleBarName.empty());
  config.layout.handle.reveal = HandleReveal::Bar;
  auto handleBar = resolveLayout(config);
  TEST_CHECK(handleBar.bars.size() == 1 && handleBar.handleBarName == noctalia::config::kHandleBarName);
  TEST_CHECK(handleBar.bars[0].position == "bottom" && handleBar.bars[0].smartAutoHide);

  // Flow: bar at home, island while working, edges and the handle (unless a flush, always-visible dock covers it).
  config.layout.mode = LayoutMode::Flow;
  config.dock.smartAutoHide = false;
  TEST_CHECK(!resolveLayout(config).handle.has_value());
  config.dock.smartAutoHide = true;
  config.dock.marginEdge = 8;
  auto flow = resolveLayout(config);
  TEST_CHECK(flow.bars.size() == 1 && flow.bars[0].name == noctalia::config::kFlowBarName);
  TEST_CHECK(flow.bars[0].smartAutoHide);
  TEST_CHECK(flow.bars[0].legibilityScrim && !flow.bars[0].compositorBlur); // transparent like the Tahoe menu bar
  TEST_CHECK(flow.bars[0].ignoreOtherZones);    // on the top edge, over the island's reserved strip
  TEST_CHECK(!flow.bars[0].autoHideEdgeReveal); // the island owns the top edge while windows are open
  TEST_CHECK(flow.island.has_value() && flow.islandVisibility == IslandVisibility::WhileWindowsOpen);
  TEST_CHECK(flow.edges.has_value() && flow.handle.has_value());

  // Synthesized bar styling: glass, floating and transparent backgrounds.
  ModeBarConfig glassBar{.background = ModeBarBackground::Glass, .backgroundOpacity = 0.4F, .floating = true};
  const BarConfig glass = noctalia::config::synthesizeBar(glassBar, "g", false);
  TEST_CHECK(glass.glass && glass.compositorBlur && glass.backgroundOpacity == 0.4F);
  TEST_CHECK(glass.marginEdge > 0 && glass.radius > 0 && !glass.concaveEdgeCorners);
  ModeBarConfig clearBar{.background = ModeBarBackground::Transparent};
  const BarConfig clear = noctalia::config::synthesizeBar(clearBar, "c", false);
  TEST_CHECK(!clear.glass && !clear.compositorBlur && clear.backgroundOpacity == 0.0F);
  TEST_CHECK(clear.legibilityScrim && !glass.legibilityScrim); // Tahoe's transparent menu bar keeps a scrim
  ModeBarConfig clearPill{.background = ModeBarBackground::Transparent, .floating = true};
  TEST_CHECK(!noctalia::config::synthesizeBar(clearPill, "p", false).legibilityScrim);

  return EXIT_SUCCESS;
}
