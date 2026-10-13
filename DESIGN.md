Slate — the L-Shell design language
===

L-Shell has one visual system, the way macOS and Windows each have one. It is called **Slate**. Every shell
surface (bar, island, dock, panels, session menu, OSD, toasts, overlays) is built from the rules below, so the
desktop reads as one product instead of a collection of themed widgets. The rules are enforced in code, not by
convention: the material lives in `src/shell/surface/material.*`, the tokens in `src/ui/style.h`, the palette
in `src/theme/builtin_palettes.cpp` ("L-Shell").

Slate is not a copy of another desktop. It borrows ideas where they earn their place (an island that holds
the status, a strip that belongs to the screen edge) and drops the ones that are only decoration (glass
sheen, lit rims, bouncing springs, wallpaper-tinted chrome).

## 1. Principles

1. **One material.** There is exactly one surface material. No component gets its own tint, rim or gradient.
2. **Fixed identity.** The shell's colours do not follow the wallpaper. The wallpaper is content, the shell is
   the frame around it. (Wallpaper-derived palettes stay available as an explicit choice in Settings.)
3. **Signal, not paint.** Colour marks state. The accent appears on small things that carry meaning — the
   active workspace, the focused app, a level, an armed toggle, the keyboard focus — never as a large fill.
4. **One place for each thing.** Every function has exactly one home that is always visible: the bar for state
   and switches, the island for what is happening now, the dock for apps. Nothing hides behind a hover.
5. **Quiet motion.** Movement explains where something came from and settles at once. No overshoot.

## 2. Material

| Token | Value | Use |
|---|---|---|
| `Style::slateTint` | 0.84 | Fill opacity of every surface (graphite `surface` role) over compositor blur |
| `material::kWellOpacity` | 0.55 | Inner wells: tiles, rows, fields on a surface |
| Hairline | 1 px, white 9 % (dark) / black 10 % (light) | Around every surface and well |
| Shadow | 22 px soft, 6 px drop, black 34 % (dark) / 14 % (light) | **Raised** surfaces only |

- **Raised** surfaces float over content and cast the shadow: island, dock, floating panels, session menu,
  OSD, toasts, switcher. **Flush** surfaces sit on a screen edge and do not: bars, attached panels, tiles.
- The opacity is high on purpose: text stays legible on any wallpaper, and compositor blur rules with an
  alpha cut-off (Hyprland `ignore_alpha 0.4`) always catch the surface.
- `Ink` (opaque black) is the only alternative material, for an island that should vanish into a dark strip.

## 3. Colour

The built-in palette **L-Shell** is the default (`theme.source = "builtin"`, `theme.accent = "palette"`).

| Role | Dark | Light |
|---|---|---|
| `surface` (graphite) | `#141518` | `#f3f3f1` |
| `surface_variant` | `#1f2125` | `#ffffff` |
| `on_surface` | `#ecedef` | `#1a1b1e` |
| `on_surface_variant` | `#9a9da4` | `#5d6067` |
| `outline` | `#2f3137` | `#d8d9dc` |
| `primary` — **Amber signal** | `#f2b04b` | `#b86e00` |
| `tertiary` (positive status) | `#7fd1ae` | `#23845e` |
| `error` | `#ff6b6b` | `#d33a3a` |

Amber was chosen because it is the colour of instruments and indicator lamps: it reads as *state*, it is warm
against cool graphite, and it is neither macOS blue nor Windows blue nor Material purple. Terminal and app
templates (GTK, Qt, KDE, Ghostty, Kitty, foot, Alacritty, btop) follow the same palette.

## 4. Shape

| Token | Value | Use |
|---|---|---|
| `radiusSm` | 5 | Chips, badges, small controls |
| `radiusMd` | 8 | Controls, list rows, fields |
| `radiusLg` | 12 | Tiles, cards, session buttons |
| `radiusXl` | 16 | Surfaces: panels, dock, OSD, toasts |
| pill | h / 2 | The collapsed island, toggles, the signal line |

Radii are **concentric**: an inner radius is the outer radius minus the padding between them.
`shell.corner_radius_scale` is 1.0. Window rounding in Hyprland matches `radiusMd`+2 (10 px).

## 5. The signal line

Slate's recurring mark for "this one is active": a short rounded line, 3 px thick.

- Dock: a short neutral line under a running app, a longer amber line under the focused app.
- Bar workspaces: the active workspace is the amber pill, others are neutral.
- Island: the active workspace number in amber at the pill's leading end.

## 6. Type

System UI font (`shell.font_family`, default Inter; Google Sans Flex works well), few sizes:
caption 12, body 13, title 15, header 20. Hierarchy comes from weight (medium for values, bold for the
workspace number), secondary text uses `on_surface_variant`.

## 7. Motion

| Token | Value |
|---|---|
| `springSnappy` | 260 ms, bounce 0.05 — moves |
| `springSmooth` | 260 ms, no bounce — exits, resizes |
| `springIsland` | 340 ms, bounce 0.08 — the island's expand |
| Fades | 180 ms ease-out in, 150 ms ease-in out |
| Popovers | 100 ms open, 80 ms close |

Reduce Motion (Settings → Accessibility) turns every move into a fade.

## 8. Layout: one way to use the system

L-Shell has exactly one layout. There are no modes: every element has one place and one job.

```
+--------------------------------------------------------------------------------------+
| (L)  1 2 3  Ghostty        ( Di 13 Okt  16:43 )        tray wifi bt vol bat  (/) (=) |   menu bar + island
+--------------------------------------------------------------------------------------+
                       .-----------------------------.
                       |  vol  Volume                |    island, expanded for a moment
                       |       ==========-------     |
                       '-----------------------------'

                                 (windows)

                      .------------------------------.
                      |  ::  [] [] [] []              |    dock: at home; while working it hides,
                      |          ==                   |    the bottom edge brings it back
                      '------------------------------'
```

1. **Menu bar** (top, always): system menu (logo: session sheet), numbered workspaces, the focused app on the left;
   tray, network, Bluetooth, volume, battery, search, Control Center on the right. Flush, Slate, 30 px.
2. **Island** (the bar's middle): the date and time. Events — volume, brightness, a new track, the charger, low
   battery, Wi-Fi/Bluetooth toggles — grow it below the bar for a moment; it replaces the floating OSD, so status
   always appears in one place. Hover while music plays shows the track; a click opens Notification Center under
   it. Recording dots (camera, microphone, screen) sit at its trailing end.
3. **Dock** (bottom): pinned and running apps with signal lines. It shows on an empty workspace and steps aside
   while windows are open; the bottom edge brings it back. Windows always get the full height.
4. **Panels** open where their trigger is: Control Center under its bar button, Notification Center under the
   island, Spotlight in the upper middle, the session sheet in the center.
5. **Keyboard first**: every surface has a key (see the Hyprland binds) — Spotlight, Control Center, Notification
   Center, overview/Expo, session sheet, clipboard, screenshots.

What L-Shell deliberately does not have: layout modes, auto-hiding menu bars, hover-only chrome, edge drawers,
wallpaper-tinted colours, floating OSD pop-ups.

## 9. Components

- **Session menu**: a sheet with two tiers — quiet actions (lock, sleep, log out) as tiles, power actions
  (restart, shut down) as wide buttons below them, so what ends the session stands apart. Number keys 1–5.
- **Control Center**: Slate wells with hairlines; circular toggles turn amber when on.
- **OSD / toasts**: raised Slate cards with the hairline; levels use amber, neutral when muted.

## 10. Designs

Slate is the default of a family. `src/config/design_presets.cpp` holds 14 more complete designs (Meridian, Atelier,
Monolith, Halo, Strata, Hearth, Glacier, Avionik, Folio, Ma, Tahoe, Eleven, Brücke, Kontor). A design is a config
layer under the user's config: bars and widget instances (named after the design), dock, island, panel positions,
a fixed palette (`theme.builtin`, same name), a bundled typeface, `corner_radius_scale`, material (`transparency_mode`,
`glass_opacity`, shadows), motion speed, and `[design.windows]` for the compositor. The rules above hold inside
every design; what changes is the material, the palette, the type and the layout. The design menu (`panel-toggle
design`, Super+Alt+D) switches them.

## 11. Checklist for new UI

- Uses `shell::material` (or `Style::slateTint` + `material::applyHairline`), never a custom tint or rim.
- Accent only on a state carrier; large areas stay neutral.
- Radii from the scale, concentric with the parent.
- Raised or flush decided by where the surface lives.
- Motion from the tokens; nothing overshoots.
- It has one home: no second copy of a function elsewhere, nothing reachable only by hover.
