<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/branding/l-shell-logo-dark.svg" />
    <img src="assets/branding/l-shell-logo-light.svg" alt="L-Shell" width="420" />
  </picture>
</p>

<p align="center"><b>A calm, macOS-inspired desktop shell for Wayland.</b><br/>
Liquid Glass panels, a Spotlight that finds everything, and a desktop that gets out of your way.</p>

<p><br/></p>

<p align="center">
  <img src="assets/l-shell-wallpaper.png" alt="L-Shell default wallpaper" width="720" />
</p>

## The mark

The L-Shell mark is a **pearl in the corner**: a bold, rounded L whose corner runs in concentric arcs around an
iridescent pearl. The shell wraps the pearl the way L-Shell wraps your apps, and the concentric corner is the same
rule the whole UI follows (an inner radius is always the outer radius minus its padding). It reads as an "L", as a
window corner and as a shell holding something precious, and it stays legible down to a 16 px menu-bar glyph.

<p align="center">
  <img src="assets/branding/l-shell-icon.svg" alt="L-Shell app icon" width="128" />
</p>

Brand assets live in [`assets/branding/`](assets/branding) and are rendered by
[`tools/render-l-shell-brand.py`](tools/render-l-shell-brand.py).

## What makes it L-Shell

- **Spotlight.** A compact search bar in the upper part of the screen that opens instantly, grows as you type and
  groups results under *Top Hit*, *Applications*, *Settings*, *Calculator*, *Emoji* and more. All launcher features
  (prefixes, app actions, app grid, dmenu) stay available.
- **Settings search.** Type what you want to change ("bar position", "dock") and Enter opens the settings window on
  exactly that setting.
- **Mission Control.** `l-shell msg overview` shows every window of the current workspace as a large thumbnail on a
  frosted backdrop, with a workspace bar on top. Click to focus, Esc to leave.
- **Liquid Glass.** Floating panels, Control Center, the session menu and the dock are translucent glass with a
  specular sheen and a hairline rim, and they appear with a quick fade-and-zoom instead of sliding in.
- **Menu bar and dock.** A slim translucent menu bar (logo menu, bold app name, status icons, Spotlight, Control
  Center, clock) and a floating glass dock with running-app dots and magnification.
- **Neutral by default.** The built-in *L-Shell* palette is mostly dark greys with a single system-blue accent; color
  appears only where it carries meaning. Common apps (GTK, Qt, KDE, terminals, btop) follow the same palette.
- **Inter** as the UI font, bundled.
- **Layout modes.** Switch in *Settings → Layout* or with `l-shell-msg layout-mode-set <mode>`:
  - *Flow* (default): the menu bar on an empty desktop; while you work it steps away, a Liquid Glass island at the
    top shows the time and expands for volume, brightness, charging and track changes, the screen edges open
    Spotlight and Control Center, and an iPhone-style home line at the bottom opens a full-screen status overview.
  - *Classic* (your `[bar.*]` bars, as upstream), *Smart Bar*, *Minimal Bar*, *Island*, *Edges* and *Handle*, each
    with its own settings. See [docs/user/layout](docs/user/layout/index.mdx).
- **Control Center like Tahoe.** A compact grid of glass modules: Wi-Fi/Bluetooth/VPN, Focus, Caffeine, Night Light,
  Display and Sound sliders, Now Playing, and round shortcuts.
- **System Settings like Tahoe.** Floating glass sidebar with colored squircle icons, grouped rows, neutral segmented
  controls and pop-up buttons, accent colors (blue, purple, pink, red, orange, yellow, green, graphite, or taken
  from the wallpaper).
- **Instant key bindings.** `l-shell-msg` is an IPC client that links only libc: bound to a key it answers in about
  2 ms instead of loading the whole shell (~45 ms), so Spotlight and Control Center appear the moment you press.

## Build and run

L-Shell builds like upstream Noctalia (C++23, Meson). Dependencies are listed in [BUILDING.md](BUILDING.md).

```sh
meson setup build-release --buildtype=release
meson compile -C build-release l-shell
./build-release/l-shell
```

L-Shell keeps everything separate from an installed Noctalia: its own executable (`l-shell`), config, state and cache
directories (`~/.config/l-shell`, `~/.local/state/l-shell`, `~/.cache/l-shell`), IPC socket and lock file. Both can be
installed side by side; only one should run at a time.

Useful commands:

```sh
l-shell-msg panel-toggle launcher        # Spotlight
l-shell-msg panel-toggle control-center  # Control Center
l-shell-msg overview                     # Mission Control
l-shell-msg settings-toggle              # System Settings
l-shell-msg layout-mode-set flow         # layout mode
```

Bind these to keys with `l-shell-msg` (not `l-shell msg`): it is the fast path. For Hyprland, see
[docs/user/compositor-settings/hyprland.mdx](docs/user/compositor-settings/hyprland.mdx), which also has the Liquid
Glass blur and window shadow settings.

## Staying current with Noctalia

The `Sync upstream Noctalia` workflow runs every day. When Noctalia's `main` has new commits it merges them on the
`upstream-sync` branch and opens a pull request into `main` (run it by hand from the Actions tab any time). A merge
that conflicts with L-Shell's changes arrives as plain upstream `main`, so the pull request shows what to resolve.

Locally:

```sh
git remote add upstream https://github.com/noctalia-dev/noctalia
git fetch upstream && git merge upstream/main
```

## Credits

L-Shell is a fork of [Noctalia](https://github.com/noctalia-dev/noctalia), a native Wayland desktop shell built
directly on Wayland and OpenGL ES. Nearly everything under the hood (bars, widgets, panels, services, theming, IPC)
comes from the Noctalia project and its contributors. Inter is by Rasmus Andersson (SIL Open Font License).

## License

MIT, like Noctalia. See [LICENSE](LICENSE).
