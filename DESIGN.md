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
4. **Everything is one gesture away.** Hidden chrome (the island's bar, edge drawers, the handle's overlay)
   always unfolds from where it lives, and always shows the complete function set, not a reduced preview.
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

## 8. Layout: Flow, one strip

The default layout is **Flow**. The top of the screen is one strip that changes what it holds:

1. **Home** (empty workspace): the full bar, Slate, flush with the top edge — logo, workspaces, tray,
   network, Bluetooth, volume, battery, launcher, Control Center, clock.
2. **Working** (windows open): the bar folds into the **island**, a pill in the same strip showing the
   active workspace, date and time, and the battery. Events (volume, brightness, media, charger,
   notifications) expand it briefly.
3. **Hover the island**: it unfolds back into the full bar over the windows — every bar function, one gesture
   away. Leaving the bar folds it into the island again.

The bottom has the dock (Slate, signal lines) and the handle; the left and right screen edges open the
launcher and Control Center. The Island layout uses the same unfold with its own bar (`[layout.island_bar]`).

## 9. Components

- **Session menu**: a sheet with two tiers — quiet actions (lock, sleep, log out) as tiles, power actions
  (restart, shut down) as wide buttons below them, so what ends the session stands apart. Number keys 1–5.
- **Control Center**: Slate wells with hairlines; circular toggles turn amber when on.
- **Status overlay** (handle): a dimmed full-screen scrim with flush Slate tiles.
- **OSD / toasts**: raised Slate cards with the hairline; levels use amber, neutral when muted.

## 10. Checklist for new UI

- Uses `shell::material` (or `Style::slateTint` + `material::applyHairline`), never a custom tint or rim.
- Accent only on a state carrier; large areas stay neutral.
- Radii from the scale, concentric with the parent.
- Raised or flush decided by where the surface lives.
- Motion from the tokens; nothing overshoots.
- Every function reachable from hidden chrome is also reachable from the visible chrome.
