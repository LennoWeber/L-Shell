# Shell layout modes

Working notes for a phased feature build (structures → signatures → TODOs → trial run → invariants →
implementation). Each phase is committed separately with a `phase N:` prefix and reviewed before the next one.

## Status

| Phase | State |
|---|---|
| 1 Understand and plan | done, waiting for approval |
| 2 Structures | – |
| 3 Signatures | – |
| 4 TODO markers | – |
| 5 Trial implementation (discarded) | – |
| 6 Invariants | – |
| 7 Implementation | – |

## Goal

Replace "configure a bar" with **switchable layout modes**. Exactly one mode is active at a time (all monitors).
Every mode has its own settings, so tuning one mode never changes another. The Settings window gets a new
**Layout** section with a mode picker plus the active mode's settings; the classic bar section only shows in
Classic mode.

### Decisions from the user

- The Pixel-style handle is primarily a **hover target**: resting the pointer on it (or pulling it up) reveals
  either a **pop-up bar** or a **full-screen status overlay** (battery, network, …). Which one is a setting.
- The full Noctalia bar system stays available as a **Classic** mode, unchanged.
- macOS-like Spotlight and theming are **out of scope**. Another session works on them separately.

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
  bar and dock each keep a private copy of that check today. The new components share one new helper. The
  existing copies stay untouched to keep merges from Noctalia painless.
- **Merge-friendliness.** Changes to upstream files stay small and additive. Classic mode must behave exactly
  as upstream.

## Config (draft)

```toml
[layout]
mode = "flow"   # classic | smart_bar | minimal_bar | island | edges | handle | flow

[layout.smart_bar]          # ModeBarConfig, preset: Noctalia look + smart hide
position = "top"            # top | bottom
thickness = 34
floating = false
background_opacity = 1.0
reveal_on_hover = true      # edge hover reveals it while windows are open
show_on_workspace_switch = true
start = ["launcher", "workspaces"]
center = ["clock"]
end = ["tray", "network", "bluetooth", "volume", "battery", "control-center"]

[layout.minimal_bar]        # ModeBarConfig, preset: thin
thickness = 22
scale = 0.8
reserve_space = true
# position/floating/background_opacity/start/center/end as above

[layout.island]             # IslandConfig
collapsed_width = 150
height = 30
expanded_width = 440
margin_top = 6
reserve_space = true
expand_on_hover = true
click_action = "panel-toggle control-center"
activities = ["media", "volume", "brightness", "notification", "battery"]
activity_seconds = 3.0

[layout.edges]              # EdgesConfig
delay_ms = 150
length = 0.5                # fraction of each edge covered by its trigger strip (centered)
close_on_leave = true
top = "panel-open control-center"
bottom = "panel-open launcher"
left = "panel-open status-overlay"
right = "panel-open session"

[layout.handle]             # HandleConfig
reveal = "overlay"          # bar | overlay
width = 120
thickness = 5
margin_bottom = 6
hover_delay_ms = 200
pull_up = true
swipe_workspaces = true
bar = { thickness = 30, start = [], center = ["clock"], end = ["network", "battery"] }  # used when reveal = "bar"

[layout.flow]               # FlowConfig: its own copies of each component's settings
bar    = { ... }            # ModeBarConfig, home bar
island = { ... }            # IslandConfig, reserve_space = false (the bar's strip is used)
edges  = { ... }            # EdgesConfig, default: left = launcher, right = control-center, top/bottom = none
handle = { ... }            # HandleConfig, default reveal = "overlay"
```

Per-mode tables reuse the same sub-schemas, so `[layout.flow.island]` takes exactly the keys of `[layout.island]`.

## Planned changes

### New types (phase 2)

`src/config/config_types.h`
- `enum class LayoutMode { Classic, SmartBar, MinimalBar, Island, Edges, Handle, Flow }`
- `struct ModeBarConfig` (position, thickness, scale, font_scale, floating, background_opacity, reserve_space,
  reveal_on_hover, show_on_workspace_switch, start/center/end)
- `struct IslandConfig`, `enum class IslandActivity`
- `struct EdgesConfig`
- `struct HandleConfig`, `enum class HandleReveal { Bar, Overlay }`
- `struct FlowConfig`
- `struct LayoutConfig` (mode + one member per mode, each with its mode's preset defaults)
- `Config::layout`, `ConfigChangeSet::layout`

`src/config/layout_resolver.h`
- `enum class IslandVisibility { Always, WhileWindowsOpen }`
- `struct ResolvedLayout`

`src/shell/island/island_activity.h`: activity state machine types (pure).
`src/shell/gesture_handle/handle_gesture.h`: drag recognizer types (pure).
`src/shell/panel/panel_manager.h`: `PanelOpenRequest::dismissOnPointerLeave`.

### New functions / classes (phase 3)

- `config/schema/config_schema.{h,cpp}`: `layoutSchema()` (+ registration in `config_sections.cpp`)
- `config/layout_resolver.{h,cpp}`: `resolveLayout(const Config&)`, `synthesizeBar(const ModeBarConfig&, name, preset flags)`
- `ConfigService::activeBars()`, `ConfigService::resolvedLayout()`
- `shell/workspace_occupancy.{h,cpp}`: `activeWorkspaceHasWindows(const CompositorPlatform&, wl_output*)`
- `shell/island/island.{h,cpp}`: `Island` (initialize, reload, onOutputChange, onWorkspaceChanged, onPointerEvent, activity feed hooks)
- `shell/island/island_activity.{h,cpp}`: `IslandActivityTracker` (post, tick, current)
- `shell/screen_edges/screen_edges.{h,cpp}`: `ScreenEdges` (initialize, reload, onOutputChange, onPointerEvent)
- `shell/gesture_handle/gesture_handle.{h,cpp}`: `GestureHandle`
- `shell/gesture_handle/handle_gesture.{h,cpp}`: `HandleGestureRecognizer` (press, motion, release, tick → outcome)
- `shell/status_overlay/status_overlay_panel.{h,cpp}`: `StatusOverlayPanel : Panel`
- `Bar::peekBar(wl_output*, std::string_view barName)`: reveal an auto-hide bar on request (handle hover)
- `Application`: ownership + wiring of the three components, IPC `layout-mode-set <mode>`
- Settings: `SettingsSection::Layout` and entry builders for each component, reusable with a path prefix

### Places to change (phase 4)

- `src/shell/bar/bar.cpp`, `dock/dock.cpp`, `panel/panel_manager.cpp`, `tray/tray_drawer_panel.cpp`,
  `tray/tray_menu.cpp`, `bar/widgets/taskbar_widget.cpp`, `app/application_internal.h`: `config().bars` →
  `activeBars()`; reload triggers also on `changed.layout`
- `src/config/config_service.cpp`: parse `[layout]`, compute the resolved layout after every parse, change set
- `src/app/application*.cpp`: construct/initialize/reload the components, route pointer, output, workspace and
  overview events, register the status overlay panel and IPC
- `src/shell/panel/panel_manager.cpp`: honor `dismissOnPointerLeave`
- `src/shell/settings/settings_registry.{h,cpp}`, sidebar: Layout section, Bar section only in Classic
- `meson.build`: new sources and tests
- `assets/translations/en.json` (+ `de.json`): labels
- `docs/user/layout/index.mdx` (new), `docs/user/bar/index.mdx` (note: bars apply in Classic mode)
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
- **Parallel work**: the Spotlight/theme session may touch settings and translations too → expect small merge conflicts.

## Out of scope

- Spotlight-style launcher and macOS-like theming (other session)
- Different modes per monitor
- A real shape morph between bar and island (Flow uses a crossfade first)

## Open points

- None yet.

## Review log

(filled in after phases 2, 3, 4, 6 and 7)
