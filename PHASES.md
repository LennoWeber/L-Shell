# Shell layout modes

Working notes for a phased feature build (structures → signatures → TODOs → trial run → invariants →
implementation). Each phase is committed separately with a `phase N:` prefix and reviewed before the next one.

## Status

| Phase | State |
|---|---|
| 1 Understand and plan | done |
| 2 Structures | done, reviewed |
| 3 Signatures | – |
| 4 TODO markers | – |
| 5 Trial implementation (discarded) | – |
| 6 Invariants | – |
| 7 Implementation | – |

## Environment

- **User's machine**: Fedora 44, Hyprland from the lionheart COPR. Hyprland is the primary compositor target.
- **Cloud container**: Arch and Fedora mirrors are blocked by the network policy, so builds run in an
  Ubuntu 26.04 Docker image (`lshell-builder`, same library generations, clang-format/clang-tidy 22).
  Visual checks use a headless Hyprland 0.53 inside that image.
- **Blur on Hyprland** comes from layer rules matched on the surface namespace
  (`docs/user/compositor-settings/hyprland.mdx`). Every new surface needs a `noctalia-*` namespace that the
  documented rule covers, or the glass reads as a flat tint.

## Goal

Replace "configure a bar" with **switchable layout modes**. Exactly one mode is active at a time (all monitors).
Every mode has its own settings, so tuning one mode never changes another. The Settings window gets a new
**Layout** section with a mode picker plus the active mode's settings; the classic bar section only shows in
Classic mode.

### Decisions from the user

- The Pixel-style handle is primarily a **hover target**: resting the pointer on it (or pulling it up) reveals
  either a **pop-up bar** or a **full-screen status overlay** (battery, network, …). Which one is a setting.
- The full Noctalia bar system stays available as a **Classic** mode, unchanged.
- Spotlight is **out of scope** (another session works on it).
- Everything must look **sleek, minimal and smooth like macOS**. The new parts of this feature follow the design
  rules below from the start.
- The system-wide macOS restyle (all panels, neutral colors, wallpaper color only for accents) is done **in this
  session as a second feature after the layout modes** (see "Next feature"). The other session only does Spotlight.

- The reference is **macOS Tahoe (Liquid Glass)**, as close as possible. Any code may change for it; a small
  diff to upstream Noctalia is no longer a goal when it stands in the way.

## Tahoe reference

- **Menu bar**: about 24–28 px, transparent by default, logo + bold app name on the left, status icons, Control
  Center and date/time on the right. The Minimal Bar preset, the Smart Bar and Flow's home bar follow this.
- **Liquid Glass**: translucent tint over a blurred backdrop, a bright specular rim along the top edge fading
  down, a faint darker rim at the bottom, adapts to light/dark. Approximated with compositor blur + a vertical
  tint gradient + a hairline rim (`shell/surface/glass.*`); a dedicated shader can follow in the restyle feature.
- **Concentric corners**: an inner element's radius = outer radius − padding.
- **Control Center / widgets**: grids of glass modules with large radii, circular toggles that turn accent when on.
  The status overlay follows this.
- **Island**: glass capsule by default (`material = "black"` gives the iPhone look).
- **Handle**: like the iPhone home indicator, 134 × 5 px.

## Design rules (macOS-like)

Apply to everything this feature adds (island, handle, edge drawers, status overlay, mode bar presets):

- **Neutral first, accent rarely.** Surfaces, text, icons and borders use the neutral roles (`surface`,
  `on_surface`, `surface_variant`, `on_surface_variant`, `outline`). The accent (`primary`) appears only on small
  state carriers: level fills (volume/brightness), active toggles, the selected item, the focus ring. Never as a
  large fill or background.
- **Material.** Translucent surfaces with compositor blur ("vibrancy"), a 1px hairline inner border at low alpha,
  a large soft shadow. No hard outlines and no decorative gradients; the only gradient is the glass sheen.
- **Shape.** The island is a full capsule; the handle is a rounded line like the iOS/Pixel home indicator, in
  `on_surface` at reduced alpha; cards and tiles use generous, consistent radii.
- **Type.** System font, few sizes, weight for hierarchy (clock semibold), secondary text in `on_surface_variant`.
- **Motion.** Short, eased, interruptible: size changes animate with an ease-out/spring feel (≈200–350 ms),
  content crossfades instead of popping, nothing bounces or slides further than needed.
- **Density.** Small icons, tight but even spacing, everything still clearly readable.

## The modes

| Mode id | Name | What is on screen |
|---|---|---|
| `classic` | Classic | Noctalia bars exactly as configured under `[bar.*]`. |
| `smart_bar` | Smart Bar | One bar, visible on an empty workspace, hidden while windows are open. Touching the edge brings it back. |
| `minimal_bar` | Minimal Bar | An always-visible, very thin bar: small icons, small text, no capsules. |
| `island` | Island | A pill at the top center. Collapsed it shows the clock. It expands on hover, and briefly on its own for events (track change, volume/brightness change, notification, charger, low battery). Click opens Control Center. |
| `edges` | Edges | Nothing permanent on screen. Resting the pointer on the middle of a screen edge opens that edge's menu, which closes again when the pointer leaves it (Caelestia style). |
| `handle` | Handle | A short line at the bottom center while windows are open. Hover or pull up → pop-up bar or full-screen status overlay. Swipe left/right on it → previous/next workspace. |
| `flow` | **Flow** (new) | The combination: on an empty workspace a slim top bar is shown. When an app is open the bar gives way to the island in the same strip, the handle appears at the bottom, and the left/right edges open the launcher and Control Center. |

`flow` becomes the default mode of L-Shell.

### Why Flow

It joins everything the user listed into one coherent rule: *home shows information, apps get the screen*.
The empty workspace ("home") gets a readable minimal bar. Once a window is open, the bar is replaced by the
island in the same reserved strip (so nothing jumps). The bottom handle gives a glanceable full-screen status
view, and the screen edges reach the menus without any visible chrome.

## Architecture

The modes are built from four **components**. A mode is a recipe that turns its own settings into component
configs. The resolver is a pure function, so it can be unit tested.

```
[layout] mode + per-mode tables
        │  resolveLayout(const Config&)  (pure)
        ▼
ResolvedLayout { bars, island?, islandVisibility, edges?, handle? }
        │
        ├─ bars    → existing Bar (reused unchanged, fed through ConfigService::activeBars())
        ├─ island  → new Island component
        ├─ edges   → new ScreenEdges component
        └─ handle  → new GestureHandle component (+ new StatusOverlayPanel)
```

| Mode | bars | island | edges | handle |
|---|---|---|---|---|
| classic | `[bar.*]` | – | – | – |
| smart_bar | 1 synthesized bar, smart auto-hide | – | – | – |
| minimal_bar | 1 synthesized thin bar | – | – | – |
| island | – | always | – | – |
| edges | – | – | yes | – |
| handle | reveal bar (only when `reveal = "bar"`), auto-hide | – | – | yes |
| flow | home bar, smart auto-hide | only while windows are open | yes | yes |

Key choices:

- **Bar engine reuse.** Bar-based modes synthesize `BarConfig`s, so all widgets, auto-hide, attached panels,
  tooltips and actions keep working. `Config::bars` keeps meaning "what `[bar.*]` says" (settings, overrides and
  export stay untouched). Runtime consumers switch to `ConfigService::activeBars()`, which returns the resolved list.
- **Synthesized bar names** are fixed and documented: `smart`, `minimal`, `handle`, `flow`.
- **Island and handle are their own surfaces**, not bar variants: the bar's geometry is full-width and
  size-fixed, while the island must hug and animate its content. Both use one layer surface sized for the
  largest state, with input and blur regions limited to the visible shape.
- **Island content is activity-driven**, not a widget list: a small state machine picks what to show
  (idle clock, media, volume, brightness, notification, battery) with priorities and timeouts. Kept pure for tests.
- **Edges** are thin trigger strips covering the middle part of each edge, so hot corners keep working.
  Actions use the existing widget action grammar (`panel-toggle control-center`, `exec …`, `none`). Panels
  opened from an edge are anchored at that edge and close when the pointer leaves them.
- **Status overlay** is a regular panel (`status-overlay`) that fills the screen, so it also works from IPC,
  edges and bar actions.
- **Workspace occupancy.** Island and handle need "does the active workspace on this output have windows". The
  bar and dock each keep a private copy of that check today (three copies). One shared helper replaces them.
- **Classic mode** must behave exactly as upstream; everything else may change freely.

## Config (as structured in phase 2)

```toml
[layout]
mode = "flow"   # classic | smart_bar | minimal_bar | island | edges | handle | flow

# ModeBarConfig — used by smart_bar, minimal_bar, handle.bar and flow.bar (each with its own preset)
[layout.smart_bar]
position = "top"              # top | bottom
thickness = 28
scale = 1.0
font_scale = 1.0
background = "glass"          # transparent | glass | solid
background_opacity = 0.55
floating = false
reserve_space = true
reveal_on_hover = true        # auto-hiding bars: touching the edge reveals
show_on_workspace_switch = true
start = ["launcher", "workspaces", "active_window"]
center = ["clock"]
end = ["tray", "network", "bluetooth", "volume", "battery", "control-center"]

[layout.minimal_bar]          # Tahoe menu bar preset: 24 px, scale 0.85, logo + app name left, clock right
thickness = 24
scale = 0.85
start = ["launcher", "active_window"]
end = ["tray", "network", "battery", "control-center", "clock"]

[layout.island]               # IslandConfig
material = "glass"            # glass | black
height = 32
collapsed_width = 150
expanded_width = 440
expanded_height = 76
margin_top = 6
reserve_space = true
expand_on_hover = true
click_action = "panel-toggle control-center"
activity_ms = 3000
suppress_osd = true           # the island replaces the OSD for what it shows

[layout.island.activities]
media = true
volume = true
brightness = true
notification = false
battery = true
system = true                 # Wi-Fi, Bluetooth, power profile, Do Not Disturb, …

[layout.edges]                # EdgesConfig
delay_ms = 150
trigger_size = 2
length = 0.5                  # fraction of each edge covered by its strip, centered
close_on_leave = true
top = "panel-open control-center"
bottom = "panel-open launcher"
left = "panel-open status-overlay"
right = "panel-open session"

[layout.handle]               # HandleConfig
reveal = "overlay"            # bar | overlay
width = 134
thickness = 5
margin_bottom = 8
hover_delay_ms = 250
pull_up = true
swipe_workspaces = true

[layout.handle.bar]           # ModeBarConfig, bottom bar shown on reveal = "bar"

[layout.flow]                 # FlowConfig: its own copy of every component
[layout.flow.bar]             # home bar: 28 px, launcher + workspaces | … clock
[layout.flow.island]          # height 24, margin_top 2, reserve_space = false (uses the bar's strip)
[layout.flow.edges]           # top/bottom = "none", left = launcher, right = control-center
[layout.flow.handle]
```

Per-mode tables reuse the same sub-schemas, so `[layout.flow.island]` takes exactly the keys of `[layout.island]`.

## Planned changes

### New types (phase 2, done)

`src/config/config_types.h`
- `LayoutMode`, `ModeBarPosition`, `ModeBarBackground`, `IslandMaterial`, `HandleReveal` (+ `EnumOption` tables)
- `ModeBarConfig`, `IslandActivitiesConfig`, `IslandConfig`, `EdgesConfig`, `HandleConfig`, `FlowConfig`,
  `LayoutConfig`; `Config::layout`; `ConfigChangeSet::layout`
- `BarConfig::glass` (Liquid Glass chrome) and `BarConfig::autoHideEdgeReveal`, both defaulting to the current
  behavior so `[bar.*]` bars are unchanged

`src/config/layout_resolver.h` (`noctalia::config`): `IslandVisibility`, `ResolvedLayout`
`src/shell/island/island_activity.h` (`shell::island`): `ActivityKind`, `Activity`, `Presentation`,
`ActivityState`, `ActivityTracker` (state only)
`src/shell/gesture_handle/handle_gesture.h` (`shell::gesture_handle`): `Outcome`, `GestureSettings`, `Phase`,
`GestureRecognizer` (state only; the hover dwell is a timer owned by the handle)
`src/shell/surface/glass.h` (`shell::glass`): `Material`, `GlassStyle`, `GlassNodes`
`src/shell/island/island.h`, `screen_edges/screen_edges.h`, `gesture_handle/gesture_handle.h`,
`status_overlay/status_overlay_panel.h`: classes with their state (no methods yet); `IslandServices`,
`StatusOverlayServices`
`src/shell/panel/panel_manager.h`: `PanelOpenRequest::screenPosition`, `::dismissOnPointerLeave`, matching state
`src/shell/osd/osd_overlay.h`: `m_redirect` (the island takes OSD content)
`src/config/config_service.h`: `m_resolvedLayout`
`src/shell/settings/settings_registry.h`: `SettingsSection::Layout`
`src/app/application.h`: owns `Island`, `ScreenEdges`, `GestureHandle` (ownership is structure; wiring is phase 4/7)

### New functions, constants and data (phase 3)

- `config/schema/config_schema.{h,cpp}`: `layoutSchema()` (+ registration in `config_sections.cpp`)
- `config/schema/ranges.h`: ranges for every new numeric field
- `config/layout_resolver.{h,cpp}`: `resolveLayout(const Config&)`, `synthesizeBar(const ModeBarConfig&, …)`,
  constants for the synthesized bar names (`smart`, `minimal`, `handle`, `flow`)
- `ConfigService::activeBars()`, `ConfigService::resolvedLayout()`
- `shell/workspace_occupancy.{h,cpp}`: `activeWorkspaceHasWindows(const CompositorPlatform&, wl_output*)`
- `shell/island/island_activity.cpp`: `ActivityTracker` (configure, post, advance, setHovered, setAmbient,
  presentation, current)
- `shell/island/island.cpp`: `Island` (initialize, reload, onOutputChange, onWorkspaceChanged, onSecondTick,
  onPointerEvent, takeOsdContent, onNotification, onBatteryChange)
- `shell/screen_edges/screen_edges.cpp`: `ScreenEdges` (initialize, reload, onOutputChange, onPointerEvent)
- `shell/gesture_handle/handle_gesture.cpp`: `GestureRecognizer` (enter, leave, dwellElapsed, press, motion,
  release → `Outcome`)
- `shell/gesture_handle/gesture_handle.cpp`: `GestureHandle` (initialize, reload, onOutputChange,
  onWorkspaceChanged, onPointerEvent)
- `shell/status_overlay/status_overlay_panel.cpp`: `StatusOverlayPanel : Panel`, panel id constant `status-overlay`
- `shell/surface/glass.cpp`: build/apply helpers for `GlassNodes`
- `Bar::peekBar(wl_output*, std::string_view barName)`: reveal an auto-hide bar on request (handle hover)
- `OsdOverlay::setRedirect(...)`
- `Application`: IPC `layout-mode-set <mode>`
- Settings: `kSettingsSections` entry for `Layout` (24 → 25), entry builders for each component, reusable with a
  path prefix

### Places to change (phase 4)

- `src/shell/bar/bar.cpp`, `dock/dock.cpp`, `panel/panel_manager.cpp`, `tray/tray_drawer_panel.cpp`,
  `tray/tray_menu.cpp`, `bar/widgets/taskbar_widget.cpp`, `app/application_internal.h`: `config().bars` →
  `activeBars()`; reload triggers also on `changed.layout`
- `src/shell/bar/bar.cpp`: honor `BarConfig::glass` and `autoHideEdgeReveal`; private
  `activeWorkspaceHasWindows` copies in bar/dock → shared helper
- `src/config/config_service.cpp`: parse `[layout]`, compute the resolved layout after every parse
- `src/config/config_overrides.cpp`: `configEqual`, `computeConfigChangeSet` (designated initializer must set
  `.layout`), override handling; `ConfigChangeSet::any()` in `config_types.h`
- `src/app/application*.cpp`: construct/initialize/reload the components, route pointer, output, workspace,
  overview and second-tick events, register the status overlay panel and IPC, OSD redirect
- `src/shell/panel/panel_manager.cpp`: honor `screenPosition` and `dismissOnPointerLeave` (never while a text
  input in the panel has focus, so the launcher does not close while typing)
- `src/shell/osd/osd_overlay.cpp`: consult the redirect before showing
- `src/shell/settings/settings_registry.{h,cpp}`, `settings_content_common.cpp` (exhaustive section switches),
  sidebar: Layout section, Bar section only in Classic
- `meson.build`: new sources and tests
- `assets/translations/en.json` (+ `de.json`): labels
- `docs/user/layout/index.mdx` (new), `docs/user/bar/index.mdx` (note: bars apply in Classic mode),
  `docs/user/compositor-settings/hyprland.mdx` (blur rule covers the new namespaces)
- `example.toml`: `[layout]` example

### Tests

Following CONTRIBUTING (deterministic contracts only):
- `tests/layout_resolver_test.cpp`: each mode resolves to the documented components; presets; flow visibility
- `tests/handle_gesture_test.cpp`: hover dwell, pull-up threshold, horizontal swipe, cancel
- `tests/island_activity_test.cpp`: priorities, timeouts, hover pin
- The config round-trip test picks up `[layout]` automatically through the section table.

Visual behavior is checked in a running shell (nested/headless compositor if the container allows it,
otherwise on the laptop).

## Risks

- **Build in the cloud container**: Ubuntu 24.04 may lack sdbus-c++ 2 / wireplumber 0.5 / clang-format 22.
  If so, building happens on the laptop and the container only checks what it can.
- **Visual checks**: no desktop in the container. A headless compositor + screenshots may work, otherwise the
  user tests on the laptop.
- **Bar interplay in Flow**: smart auto-hide of the home bar and island visibility must flip together on
  workspace/toplevel changes without flicker.
- **Panels from edges**: the panel system has no "close when the pointer leaves" yet, and floating placement
  next to a screen edge without a bar is untested.
- **Handle drag**: dragging beyond the small surface relies on the Wayland implicit pointer grab.
- **Parallel work**: the Spotlight session may touch settings and translations too → expect small merge conflicts.

## Out of scope

- Spotlight-style launcher (other session)
- The system-wide restyle of existing panels (next feature, see below)
- Different modes per monitor
- A real shape morph between bar and island (Flow uses a crossfade first)

## Next feature: system-wide macOS look (planned after this one)

Own phased plan once the layout modes are done. Starting points found during phase 1:

- New built-in palette **macOS** (neutral grey ramps, light and dark, system-blue accent) in
  `src/theme/builtin_palettes.cpp`, made the L-Shell default.
- New theme option **accent-only dynamic color**: a palette transform next to `applyPureBlackDark`
  (`src/theme/palette_transform.*`) that takes only the accent roles (`primary`, `secondary`, `tertiary` and their
  `on_*`) from the wallpaper-generated palette and keeps every surface, text, outline and hover role neutral.
  App templates (GTK, Qt, terminals) follow the same palette automatically.
- Global shape, spacing and motion constants in `src/ui/style.h`; panel chrome (translucency, hairline border,
  shadow) in `src/shell/panel/panel_surface_style.h` and `src/shell/surface/shadow.*`.
- Restyle every panel (Control Center, launcher, notifications, OSD, session, clipboard, tray menus, settings)
  to the design rules above.
- Window title bars with close buttons are drawn by apps or the compositor, not by the shell. They only follow
  through the generated GTK/Qt themes.
- Tahoe's transparent menu bar needs text that adapts to the wallpaper underneath (the shell renders the
  wallpaper, so it can measure its luminance). Until then the mode bars default to `background = "glass"`.
- A real Liquid Glass shader (refraction, specular highlight) for every glass surface.

## Open points

- None yet.

## Review log

### Phase 2 (structures)

Fresh reviewer, diff `6348447..b3bc273`. 15 findings.

Fixed:
1. Mode bars need settings the bar engine lacks → added `BarConfig::glass` and `BarConfig::autoHideEdgeReveal`
   (defaults keep `[bar.*]` unchanged).
2. Components could not open anchored panels → `PanelManager*` plus pre-parsed `WidgetAction`s per edge strip
   and for the island click.
3. No way to anchor a panel at a screen edge → `PanelOpenRequest::screenPosition`.
4. `Layout` has no `kSettingsSections` descriptor → phase 3 (module data); exhaustive switches → phase 4 list.
5. Change-set plumbing → `configEqual`, `computeConfigChangeSet`, `any()` added to the phase 4 list.
6. Bars too tall for Tahoe → smart and flow bar 28 px, flow island 24 px with a 2 px margin.
7. Missing animation ids, input areas and the dwell timer → added.
8. Island clock timer duplicated the shared second tick → removed (`onSecondTick` in phase 3).
10. Names in this file did not match the code → updated.
14. `activitySeconds` (float) → `activityMs` (int), like `[osd]`.
15. `Activity::overLimit` added; `StatusOverlayPanel::m_reveal` removed (the panel manager already reveals);
    the design rule now allows the glass sheen gradient; name constants and ranges → phase 3.
12. `noctalia::layout` → `noctalia::config`, next to the other config helpers.

Declined:
- 8 (part): `m_activityTimer` stays as the wake-up; the remaining time lives only in the tracker.
- 9: owning the components in `Application` is structure; only the wiring is later.
- 11: `IslandMaterial` (config) and `shell::glass::Material` (rendering) live in different layers on purpose;
  `ModeBarPosition` deliberately allows only top/bottom; `ScreenEdges::Edge` is private.
- 13: the defaults stay (they follow Caelestia). Leaving a full-screen panel cannot happen, and the launcher
  case is handled by the focused-text-input rule in the phase 4 list.
- 15 (part): `ResolvedLayout` defaults to Classic on purpose (empty until the first parse, now commented);
  `ConfigService&` vs `ConfigService*` follow each neighbor's convention (bar services vs panels).
