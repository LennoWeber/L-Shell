#include "config/design_presets.h"

#include "config/config_service.h"
#include "core/log.h"

#include <algorithm>

namespace noctalia::config {

  namespace {

    constexpr Logger kLog("design");

    // Every layer names its own bars and widget instances (prefixed with the design id), so switching designs
    // never leaves a bar or widget of the previous one behind. Panels, notifications and the OSD open where the
    // design's chrome lives. Fonts are bundled under assets/fonts/system.

    constexpr std::string_view kMeridian = R"toml(
[theme]
source = "builtin"
builtin = "Meridian"
mode = "dark"
accent = "palette"

[shell]
font_family = "IBM Plex Sans"
corner_radius_scale = 0.5

[shell.animation]
speed = 1.6

[shell.panel]
transparency_mode = "solid"
shadow = false
launcher_position = "bottom_center"
control_center_position = "bottom_right"
session_position = "bottom_right"

[shell.launcher]
style = "command"

[shell.session]
menu = true

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 1.0

[osd]
position = "bottom_right"
background_opacity = 1.0

[bar.meridian]
position = "bottom"
thickness = 34
background_opacity = 1.0
compositor_blur = false
padding = 10
widget_spacing = 12
start = ["meridian_workspaces", "meridian_tasks"]
center = ["meridian_command"]
end = ["tray", "network", "bluetooth", "volume", "meridian_battery", "notifications", "meridian_clock"]

[widget.meridian_workspaces]
type = "workspaces"
style = "minimal"
font_family = "IBM Plex Mono"

[widget.meridian_tasks]
type = "taskbar"
only_active_workspace = true
show_window_title = true
window_title_max_width = 140
icon_scale = 0.8

[widget.meridian_command]
type = "custom_button"
glyph = "search"
label = "Suchen, Befehl, Rechnung …"
capsule = true
capsule_radius = 4
capsule_fill = "surface_variant"
capsule_border = "primary"
capsule_padding = 10

[widget.meridian_command.actions]
left = "panel-toggle launcher"

[widget.meridian_battery]
type = "battery"
show_label = true

[widget.meridian_clock]
type = "clock"
format = "{:%a %d.%m  %H:%M}"
font_family = "IBM Plex Mono"

[design.windows]
rounding = 6
border_size = 1
active_border = "3ddc97aa"
inactive_border = "2a313cff"
gaps_in = 4
gaps_out = 8
shadow = false
background = "0b0e13ff"
)toml";

    constexpr std::string_view kAtelier = R"toml(
[theme]
source = "builtin"
builtin = "Atelier"
mode = "light"
accent = "palette"

[shell]
font_family = "DM Sans"
corner_radius_scale = 1.15

[shell.animation]
speed = 0.9

[shell.shadow]
alpha = 0.18

[shell.panel]
transparency_mode = "solid"
shadow = true
control_center_position = "top_right"

[island]
enabled = false

[dock]
enabled = true
position = "left"
auto_hide = false
smart_auto_hide = false
reserve_space = true
icon_size = 40
margin_edge = 16
main_axis_padding = 10
cross_axis_padding = 10
item_spacing = 10
radius = 18
radius_top_left = 18
radius_top_right = 18
radius_bottom_left = 18
radius_bottom_right = 18
background_opacity = 1.0
border = "#e3dccd"
magnification = false

[notification]
position = "bottom_right"
background_opacity = 1.0

[osd]
position = "bottom_right"
background_opacity = 1.0

[bar.atelier]
position = "top"
thickness = 36
background_opacity = 1.0
compositor_blur = false
padding = 18
widget_spacing = 16
start = ["logo", "atelier_app", "atelier_workspaces"]
end = ["tray", "network", "bluetooth", "volume", "atelier_battery", "control-center", "atelier_clock"]

[widget.atelier_app]
type = "active_window"
display = "app_name"
font_family = "Fraunces"
font_weight = 600

[widget.atelier_workspaces]
type = "workspaces"
style = "minimal"
font_family = "Fraunces"

[widget.atelier_battery]
type = "battery"
show_label = true

[widget.atelier_clock]
type = "clock"
format = "{:%A, %-d. %B  %H:%M}"
font_family = "Fraunces"

[design.windows]
rounding = 14
border_size = 1
active_border = "2e4a7d88"
inactive_border = "ddd6c8ff"
gaps_in = 6
gaps_out = 14
shadow = true
shadow_range = 30
shadow_offset = "0 10"
shadow_color = "3c321e2e"
background = "f3efe6ff"
)toml";

    constexpr std::string_view kMonolith = R"toml(
[theme]
source = "builtin"
builtin = "Monolith"
mode = "dark"
accent = "palette"

[shell]
font_family = "Space Grotesk"
corner_radius_scale = 0.0

[shell.animation]
speed = 1.4

[shell.panel]
transparency_mode = "solid"
shadow = false
launcher_placement = "attached"
control_center_placement = "attached"

[shell.launcher]
style = "command"

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "top_right"
background_opacity = 1.0

[osd]
position = "top_right"
background_opacity = 1.0

[bar.monolith]
position = "left"
thickness = 60
background_opacity = 1.0
compositor_blur = false
padding = 14
widget_spacing = 18
start = ["monolith_mark", "monolith_workspaces", "monolith_tasks"]
end = ["network", "volume", "monolith_battery", "monolith_clock"]

[widget.monolith_mark]
type = "launcher"
capsule = true
capsule_radius = 0
capsule_fill = "primary"
capsule_foreground = "on_primary"

[widget.monolith_workspaces]
type = "workspaces"
show_labels = false
focused_color = "primary"
occupied_color = "on_surface"
empty_color = "outline"
capsule_radius = 0
pill_scale = 0.6
active_pill_size = 1.0

[widget.monolith_tasks]
type = "taskbar"
active_indicator_color = "primary"

[widget.monolith_battery]
type = "battery"
display_mode = "none"
show_label = true

[widget.monolith_clock]
type = "clock"
vertical_format = "{:%H\n%M}"
font_weight = 700

[design.windows]
rounding = 0
border_size = 1
active_border = "c8ff2eff"
inactive_border = "1f1f1fff"
gaps_in = 4
gaps_out = 12
shadow = false
background = "000000ff"
)toml";

    constexpr std::string_view kHalo = R"toml(
[theme]
source = "builtin"
builtin = "Halo"
mode = "dark"
accent = "palette"

[shell]
font_family = "Outfit"
corner_radius_scale = 1.4

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.82
shadow = true
control_center_position = "top_right"
launcher_position = "bottom_center"

[island]
enabled = true
height = 24
collapsed_width = 200
click_action = "panel-toggle notification-center"

[dock]
enabled = true
position = "bottom"
auto_hide = false
smart_auto_hide = false
reserve_space = true
icon_size = 40
margin_edge = 12
radius = 29
radius_top_left = 29
radius_top_right = 29
radius_bottom_left = 29
radius_bottom_right = 29
background_opacity = 0.72
magnification_scale = 1.12

[notification]
position = "top_right"
background_opacity = 0.82

[osd]
position = "top_center"
background_opacity = 0.82

[bar.halo]
position = "top"
thickness = 34
background_opacity = 0.72
compositor_blur = true
margin_edge = 10
margin_ends = 12
radius = 17
radius_top_left = 17
radius_top_right = 17
radius_bottom_left = 17
radius_bottom_right = 17
shadow = true
padding = 12
start = ["logo", "halo_workspaces", "app_name"]
end = ["network", "bluetooth", "volume", "halo_battery", "control-center"]

[widget.halo_workspaces]
type = "workspaces"
show_labels = false
pill_scale = 0.5
active_pill_size = 3.0
focused_color = "primary"
occupied_color = "on_surface_variant"
empty_color = "outline"

[widget.halo_battery]
type = "battery"
show_label = true

[design.windows]
rounding = 18
border_size = 1
active_border = "8c9eff66"
inactive_border = "ffffff12"
gaps_in = 6
gaps_out = 14
shadow = true
shadow_range = 40
shadow_offset = "0 16"
shadow_color = "00000080"
background = "0f1020ff"
)toml";

    constexpr std::string_view kStrata = R"toml(
[theme]
source = "builtin"
builtin = "Strata"
mode = "dark"
accent = "palette"

[shell]
font_family = "Geist"
corner_radius_scale = 1.0

[shell.animation]
speed = 1.4

[shell.panel]
transparency_mode = "solid"
shadow = true
launcher_position = "center"
control_center_position = "top_right"

[shell.launcher]
style = "command"

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "top_right"
background_opacity = 1.0

[osd]
position = "top_right"
background_opacity = 1.0

[bar.strata]
position = "top"
thickness = 36
background_opacity = 1.0
compositor_blur = false
padding = 10
widget_spacing = 10
start = ["strata_workspaces", "strata_tabs"]
end = ["tray", "network", "bluetooth", "volume", "strata_battery", "notifications", "strata_clock"]

[widget.strata_workspaces]
type = "workspaces"
font_family = "Geist Mono"
focused_color = "primary"
pill_scale = 0.8
active_pill_size = 1.4

[widget.strata_tabs]
type = "taskbar"
only_active_workspace = true
show_window_title = true
window_title_max_width = 170
active_indicator_color = "primary"

[widget.strata_battery]
type = "battery"
show_label = true
font_family = "Geist Mono"

[widget.strata_clock]
type = "clock"
format = "{:%H:%M}"
font_family = "Geist Mono"

[design.windows]
rounding = 8
border_size = 1
active_border = "38bdf8ff"
inactive_border = "2b3036ff"
gaps_in = 4
gaps_out = 8
shadow = false
background = "121417ff"
)toml";

    constexpr std::string_view kHearth = R"toml(
[theme]
source = "builtin"
builtin = "Hearth"
mode = "dark"
accent = "palette"

[shell]
font_family = "Plus Jakarta Sans"
corner_radius_scale = 1.4

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.92
shadow = true
launcher_position = "bottom_center"
control_center_position = "bottom_right"
session_position = "bottom_center"

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "top_right"
background_opacity = 0.92

[osd]
position = "top_center"
background_opacity = 0.92

[bar.hearth]
position = "bottom"
thickness = 58
background_opacity = 0.92
compositor_blur = true
margin_edge = 14
margin_ends = 300
radius = 22
radius_top_left = 22
radius_top_right = 22
radius_bottom_left = 22
radius_bottom_right = 22
shadow = true
padding = 10
widget_spacing = 8
start = ["hearth_search", "hearth_apps", "hearth_workspaces"]
end = ["network", "bluetooth", "volume", "battery", "hearth_clock", "notifications"]

[widget.hearth_search]
type = "launcher"
capsule = true
capsule_radius = 14
capsule_fill = "primary"
capsule_foreground = "on_primary"
capsule_padding = 10

[widget.hearth_apps]
type = "taskbar"
icon_scale = 1.1
item_spacing = 6
active_indicator_color = "primary"

[widget.hearth_workspaces]
type = "workspaces"
focused_color = "primary"

[widget.hearth_clock]
type = "clock"
format = "{:%H:%M}\n{:%a, %-d. %b}"
font_weight = 700

[design.windows]
rounding = 16
border_size = 1
active_border = "e08a4f88"
inactive_border = "3a2f28ff"
gaps_in = 6
gaps_out = 14
shadow = true
shadow_range = 30
shadow_offset = "0 12"
shadow_color = "00000073"
background = "17120fff"
)toml";

    constexpr std::string_view kGlacier = R"toml(
[theme]
source = "builtin"
builtin = "Glacier"
mode = "light"
accent = "palette"

[shell]
font_family = "Hanken Grotesk"
corner_radius_scale = 1.15

[shell.shadow]
alpha = 0.22

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.72
shadow = true
control_center_position = "center_right"
launcher_position = "spotlight"

[island]
enabled = false

[dock]
enabled = true
position = "bottom"
auto_hide = false
smart_auto_hide = false
reserve_space = true
icon_size = 44
margin_edge = 12
radius = 20
radius_top_left = 20
radius_top_right = 20
radius_bottom_left = 20
radius_bottom_right = 20
background_opacity = 0.6
border = "#ffffffe6"
magnification_scale = 1.15

[notification]
position = "top_right"
background_opacity = 0.85

[osd]
position = "top_center"
background_opacity = 0.85

[bar.glacier]
position = "top"
thickness = 30
background_opacity = 0.62
compositor_blur = true
padding = 14
widget_spacing = 16
start = ["logo", "glacier_app", "glacier_workspaces"]
end = ["network", "bluetooth", "volume", "glacier_battery", "glacier_clock"]

[widget.glacier_app]
type = "active_window"
display = "app_name"
font_weight = 700

[widget.glacier_workspaces]
type = "workspaces"
label_source = "name"
max_label_chars = 8
focused_color = "primary"
capsule_radius = 6

[widget.glacier_battery]
type = "battery"
show_label = true

[widget.glacier_clock]
type = "clock"
format = "{:%a %-d. %b %H:%M}"
font_weight = 600

[widget.glacier_clock.actions]
left = "panel-toggle control-center"

[design.windows]
rounding = 14
border_size = 1
active_border = "2563eb66"
inactive_border = "0f172a14"
gaps_in = 6
gaps_out = 14
shadow = true
shadow_range = 40
shadow_offset = "0 14"
shadow_color = "1e325a38"
background = "d6e1eeff"
)toml";

    constexpr std::string_view kAvionik = R"toml(
[theme]
source = "builtin"
builtin = "Avionik"
mode = "dark"
accent = "palette"

[shell]
font_family = "Barlow Semi Condensed"
corner_radius_scale = 0.15

[shell.animation]
speed = 2.5

[shell.panel]
transparency_mode = "solid"
shadow = false
control_center_position = "top_right"
launcher_position = "center"

[shell.launcher]
style = "command"

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 1.0

[osd]
position = "top_right"
background_opacity = 1.0

[bar]
order = ["avionik", "avionik_keys"]

[bar.avionik]
position = "top"
thickness = 32
background_opacity = 1.0
compositor_blur = false
padding = 12
widget_spacing = 18
start = ["avionik_mark", "avionik_workspaces", "avionik_app"]
end = ["avionik_net", "avionik_vol", "avionik_bat", "avionik_cpu", "avionik_clock"]

[widget.avionik_mark]
type = "text"
text = "L-SHELL"
color = "primary"
font_weight = 600

[widget.avionik_workspaces]
type = "workspaces"
style = "minimal"
font_family = "JetBrains Mono"
focused_color = "primary"

[widget.avionik_app]
type = "active_window"
display = "app_name"

[widget.avionik_net]
type = "network"
show_label = true

[widget.avionik_vol]
type = "volume"
show_label = true

[widget.avionik_bat]
type = "battery"
display_mode = "none"
show_label = true
font_family = "JetBrains Mono"

[widget.avionik_cpu]
type = "sysmon"
stat = "cpu_usage"
visualization = "none"
font_family = "JetBrains Mono"

[widget.avionik_clock]
type = "clock"
format = "{:%H:%M:%S}  {:%a %d %b}"
font_family = "JetBrains Mono"
font_weight = 700

[bar.avionik_keys]
position = "bottom"
thickness = 24
background_opacity = 1.0
compositor_blur = false
padding = 12
start = ["avionik_hints"]
end = ["privacy"]

[widget.avionik_hints]
type = "text"
text = "SUPER LAUNCHER    S+C SYSTEM    S+N MELDUNGEN    S+O ÜBERSICHT    S+X SITZUNG"
color = "on_surface_variant"
font_family = "JetBrains Mono"
font_scale = 0.85

[design.windows]
rounding = 0
border_size = 2
active_border = "ffb547ff"
inactive_border = "233041ff"
gaps_in = 4
gaps_out = 8
shadow = false
background = "0a0d12ff"
)toml";

    constexpr std::string_view kFolio = R"toml(
[theme]
source = "builtin"
builtin = "Folio"
mode = "dark"
accent = "palette"

[shell]
font_family = "Sora"
corner_radius_scale = 1.0

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.96
shadow = true
launcher_position = "center"
control_center_position = "bottom_right"

[shell.launcher]
style = "spotlight"
app_grid = true
categories = true

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 0.96

[osd]
position = "bottom_center"
background_opacity = 0.96

[bar]
order = ["folio", "folio_status"]

[bar.folio]
position = "top"
thickness = 60
background_opacity = 1.0
compositor_blur = false
padding = 14
widget_spacing = 12
start = ["folio_strip"]
end = ["folio_clock"]

[widget.folio_strip]
type = "taskbar"
group_by_workspace = true
workspace_group_content = "icons"
show_workspace_label = true
workspace_group_capsule = true
focused_color = "primary"
icon_scale = 1.1

[widget.folio_clock]
type = "clock"
format = "{:%H:%M}\n{:%A, %-d. %B}"
font_weight = 300

[bar.folio_status]
position = "bottom"
thickness = 28
background_opacity = 1.0
compositor_blur = false
padding = 14
start = ["app_name"]
center = ["media"]
end = ["network", "bluetooth", "volume", "folio_battery", "notifications", "control-center"]

[widget.folio_battery]
type = "battery"
show_label = true

[design.windows]
rounding = 12
border_size = 1
active_border = "f472b6aa"
inactive_border = "2c2933ff"
gaps_in = 6
gaps_out = 12
shadow = true
shadow_range = 30
shadow_offset = "0 10"
shadow_color = "00000066"
background = "141317ff"
)toml";

    constexpr std::string_view kMa = R"toml(
[theme]
source = "builtin"
builtin = "Ma"
mode = "dark"
accent = "palette"

[shell]
font_family = "Zen Kaku Gothic New"
corner_radius_scale = 0.5

[shell.animation]
speed = 0.8

[shell.panel]
transparency_mode = "solid"
shadow = true
launcher_position = "center"
control_center_position = "bottom_left"
session_position = "bottom_left"

[shell.launcher]
style = "command"

[island]
enabled = false

[dock]
enabled = false

# Ink on a dark ground: Ma has no wallpaper, the compositor's background colour shows.
[wallpaper]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 1.0

[osd]
position = "bottom_right"
background_opacity = 1.0

[bar]
order = ["ma_time", "ma_seal"]

[bar.ma_time]
position = "top"
thickness = 64
background_opacity = 0.0
compositor_blur = false
reserve_space = true
padding = 30
widget_spacing = 16
start = []
end = ["ma_date", "ma_clock"]

[widget.ma_date]
type = "clock"
format = "{:%A, %-d. %B}"
color = "on_surface_variant"

[widget.ma_clock]
type = "clock"
format = "{:%H:%M}"
font_family = "Shippori Mincho"
font_scale = 2.2
font_weight = 400

[bar.ma_seal]
position = "bottom"
thickness = 64
background_opacity = 0.0
compositor_blur = false
reserve_space = false
padding = 24
widget_spacing = 14
start = ["ma_seal_workspaces", "app_name"]
end = ["ma_battery"]

[widget.ma_seal_workspaces]
type = "workspaces"
font_family = "Shippori Mincho"
focused_color = "primary"
occupied_color = "surface_variant"
empty_color = "surface_variant"
capsule_radius = 6
pill_scale = 1.0
active_pill_size = 1.0

[widget.ma_battery]
type = "battery"
show_label = true
hide_when_plugged = true

[design.windows]
rounding = 6
border_size = 1
active_border = "c8361e88"
inactive_border = "2e2b27ff"
gaps_in = 6
gaps_out = 18
shadow = true
shadow_range = 24
shadow_offset = "0 8"
shadow_color = "00000066"
background = "121110ff"
)toml";

    constexpr std::string_view kTahoe = R"toml(
[theme]
source = "builtin"
builtin = "Tahoe"
mode = "dark"
accent = "palette"

[shell]
font_family = "Inter Variable"
corner_radius_scale = 1.6

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.42
shadow = true
control_center_position = "top_right"
launcher_position = "spotlight"

[shell.launcher]
style = "spotlight"

[shell.session]
menu = true

[island]
enabled = false

[dock]
enabled = true
position = "bottom"
auto_hide = false
smart_auto_hide = false
reserve_space = true
icon_size = 52
margin_edge = 8
radius = 26
radius_top_left = 26
radius_top_right = 26
radius_bottom_left = 26
radius_bottom_right = 26
background_opacity = 0.25
border = "#ffffff4d"
magnification_scale = 1.45

[notification]
position = "top_right"
background_opacity = 0.42

[osd]
position = "bottom_center"
background_opacity = 0.42

[bar.tahoe]
position = "top"
thickness = 28
background_opacity = 0.28
compositor_blur = true
padding = 16
widget_spacing = 16
start = ["logo", "tahoe_app", "tahoe_title"]
end = ["control-center", "network", "tahoe_battery", "launcher", "tahoe_clock"]

[widget.tahoe_app]
type = "active_window"
display = "app_name"
font_weight = 700

[widget.tahoe_title]
type = "active_window"
display = "text_only"
color = "on_surface_variant"
max_length = 420.0

[widget.tahoe_battery]
type = "battery"
show_label = true

[widget.tahoe_clock]
type = "clock"
format = "{:%a. %-d. %b. %H:%M}"
font_weight = 500

[widget.tahoe_clock.actions]
left = "panel-toggle notification-center"

[design.windows]
rounding = 16
border_size = 1
active_border = "ffffff26"
inactive_border = "ffffff14"
gaps_in = 6
gaps_out = 12
shadow = true
shadow_range = 40
shadow_offset = "0 18"
shadow_color = "00000073"
background = "13243dff"
)toml";

    constexpr std::string_view kEleven = R"toml(
[theme]
source = "builtin"
builtin = "Eleven"
mode = "dark"
accent = "palette"

[shell]
font_family = "Open Sans"
corner_radius_scale = 0.6

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.9
shadow = true
launcher_position = "bottom_center"
control_center_position = "bottom_right"
session_position = "bottom_center"

[shell.launcher]
style = "spotlight"
app_grid = true

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 0.92

[osd]
position = "bottom_center"
background_opacity = 0.92

[bar.eleven]
position = "bottom"
thickness = 48
background_opacity = 0.86
compositor_blur = true
padding = 10
widget_spacing = 4
start = []
center = ["eleven_start", "eleven_search", "eleven_apps"]
end = ["tray", "network", "volume", "battery", "eleven_clock", "notifications"]

[widget.eleven_start]
type = "launcher"
color = "primary"

[widget.eleven_search]
type = "custom_button"
glyph = "search"
label = "Suchen"
capsule = true
capsule_radius = 17
capsule_fill = "surface_variant"
capsule_padding = 12

[widget.eleven_search.actions]
left = "panel-toggle launcher"

[widget.eleven_apps]
type = "taskbar"
icon_scale = 1.05
item_spacing = 4
active_indicator_color = "primary"

[widget.eleven_clock]
type = "clock"
format = "{:%H:%M}\n{:%d.%m.%Y}"
font_scale = 0.9

[design.windows]
rounding = 8
border_size = 1
active_border = "4cc2ff66"
inactive_border = "ffffff14"
gaps_in = 4
gaps_out = 8
shadow = true
shadow_range = 24
shadow_offset = "0 8"
shadow_color = "00000066"
background = "0b1b3aff"
)toml";

    constexpr std::string_view kBruecke = R"toml(
[theme]
source = "builtin"
builtin = "Bruecke"
mode = "dark"
accent = "palette"

[shell]
font_family = "Manrope"
corner_radius_scale = 1.1

[shell.panel]
transparency_mode = "glass"
glass_opacity = 0.82
shadow = true
launcher_position = "bottom_center"
control_center_position = "top_right"

[shell.launcher]
style = "spotlight"
app_grid = true

[island]
enabled = false

[dock]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 0.88

[osd]
position = "top_center"
background_opacity = 0.88

[bar]
order = ["bruecke_top", "bruecke_task"]

[bar.bruecke_top]
position = "top"
thickness = 30
background_opacity = 0.6
compositor_blur = true
padding = 14
widget_spacing = 16
start = ["logo", "bruecke_app", "bruecke_title"]
end = ["control-center", "network", "bluetooth", "volume", "bruecke_battery", "bruecke_clock"]

[widget.bruecke_app]
type = "active_window"
display = "app_name"
font_weight = 700

[widget.bruecke_title]
type = "active_window"
display = "text_only"
color = "on_surface_variant"
max_length = 420.0

[widget.bruecke_battery]
type = "battery"
show_label = true

[widget.bruecke_clock]
type = "clock"
format = "{:%a %-d. %b %H:%M}"
font_weight = 600

[bar.bruecke_task]
position = "bottom"
thickness = 52
background_opacity = 0.72
compositor_blur = true
padding = 12
widget_spacing = 6
start = ["bruecke_workspaces"]
center = ["bruecke_start", "bruecke_search", "bruecke_apps"]
end = ["notifications"]

[widget.bruecke_workspaces]
type = "workspaces"
focused_color = "primary"

[widget.bruecke_start]
type = "launcher"
capsule = true
capsule_radius = 12
capsule_fill = "primary"
capsule_foreground = "on_primary"

[widget.bruecke_search]
type = "custom_button"
glyph = "search"
label = "Suchen"
capsule = true
capsule_radius = 18
capsule_fill = "surface_variant"
capsule_padding = 12

[widget.bruecke_search.actions]
left = "panel-toggle launcher"

[widget.bruecke_apps]
type = "taskbar"
icon_scale = 1.05
active_indicator_color = "primary"

[design.windows]
rounding = 12
border_size = 1
active_border = "4f7cff88"
inactive_border = "ffffff14"
gaps_in = 5
gaps_out = 10
shadow = true
shadow_range = 30
shadow_offset = "0 12"
shadow_color = "00000073"
background = "15171cff"
)toml";

    constexpr std::string_view kKontor = R"toml(
[theme]
source = "builtin"
builtin = "Kontor"
mode = "light"
accent = "palette"

[shell]
font_family = "Atkinson Hyperlegible Next"
corner_radius_scale = 0.33

[shell.animation]
enabled = false

[shell.panel]
transparency_mode = "solid"
shadow = false
launcher_position = "center"
control_center_position = "bottom_right"

[shell.launcher]
style = "command"
show_icons = false

[shell.session]
menu = true

[island]
enabled = false

[dock]
enabled = false

[wallpaper]
enabled = false

[notification]
position = "bottom_right"
background_opacity = 1.0

[osd]
position = "bottom_right"
background_opacity = 1.0

[bar.kontor]
position = "bottom"
thickness = 28
background_opacity = 1.0
compositor_blur = false
padding = 12
widget_spacing = 16
hover_highlight = false
start = ["kontor_workspaces", "kontor_tasks"]
end = ["kontor_net", "kontor_bat", "notifications", "kontor_clock"]

[widget.kontor_workspaces]
type = "workspaces"
style = "minimal"
font_family = "Atkinson Hyperlegible Mono"

[widget.kontor_tasks]
type = "taskbar"
only_active_workspace = true
show_window_title = true
window_title_max_width = 160
icon_scale = 0.7

[widget.kontor_net]
type = "network"
show_label = true

[widget.kontor_bat]
type = "battery"
display_mode = "none"
show_label = true

[widget.kontor_clock]
type = "clock"
format = "{:%a %d.%m.  %H:%M}"
font_family = "Atkinson Hyperlegible Mono"

[design.windows]
rounding = 4
border_size = 2
active_border = "1f1f1fff"
inactive_border = "cfcfcbff"
gaps_in = 3
gaps_out = 6
shadow = false
background = "e9e9e7ff"
)toml";

    constexpr std::array kPresets{
        DesignPreset{
            .id = "slate",
            .name = "Slate",
            .swatches = {0x141518, 0x1f2125, 0xecedef, 0xf2b04b},
            .toml = "",
        },
        DesignPreset{
            .id = "meridian",
            .name = "Meridian",
            .swatches = {0x0b0e13, 0x161b22, 0xe6eaf0, 0x3ddc97},
            .toml = kMeridian,
        },
        DesignPreset{
            .id = "atelier",
            .name = "Atelier",
            .swatches = {0xf3efe6, 0xfbf9f4, 0x22201c, 0x2e4a7d},
            .toml = kAtelier,
        },
        DesignPreset{
            .id = "monolith",
            .name = "Monolith",
            .swatches = {0x000000, 0x0a0a0a, 0xf5f5f5, 0xc8ff2e},
            .toml = kMonolith,
        },
        DesignPreset{
            .id = "halo",
            .name = "Halo",
            .swatches = {0x0f1020, 0x22254a, 0xeef0ff, 0x8c9eff},
            .toml = kHalo,
        },
        DesignPreset{
            .id = "strata",
            .name = "Strata",
            .swatches = {0x121417, 0x23272d, 0xe8eaed, 0x38bdf8},
            .toml = kStrata,
        },
        DesignPreset{
            .id = "hearth",
            .name = "Hearth",
            .swatches = {0x17120f, 0x2e2520, 0xf3e9e1, 0xe08a4f},
            .toml = kHearth,
        },
        DesignPreset{
            .id = "glacier",
            .name = "Glacier",
            .swatches = {0xd6e1ee, 0xf6f8fb, 0x0f172a, 0x2563eb},
            .toml = kGlacier,
        },
        DesignPreset{
            .id = "avionik",
            .name = "Avionik",
            .swatches = {0x0a0d12, 0x0f141b, 0xdce6f2, 0xffb547},
            .toml = kAvionik,
        },
        DesignPreset{
            .id = "folio",
            .name = "Folio",
            .swatches = {0x141317, 0x1c1a21, 0xf1eef6, 0xf472b6},
            .toml = kFolio,
        },
        DesignPreset{
            .id = "ma",
            .name = "Ma",
            .swatches = {0x121110, 0x1c1a18, 0xece6da, 0xc8361e},
            .toml = kMa,
        },
        DesignPreset{
            .id = "tahoe",
            .name = "Tahoe",
            .swatches = {0x13243d, 0x1e2026, 0xf5f5f7, 0x0a84ff},
            .toml = kTahoe,
        },
        DesignPreset{
            .id = "eleven",
            .name = "Eleven",
            .swatches = {0x0b1b3a, 0x202020, 0xffffff, 0x4cc2ff},
            .toml = kEleven,
        },
        DesignPreset{
            .id = "bruecke",
            .name = "Brücke",
            .swatches = {0x15171c, 0x24262d, 0xf2f3f6, 0x4f7cff},
            .toml = kBruecke,
        },
        DesignPreset{
            .id = "kontor",
            .name = "Kontor",
            .swatches = {0xe9e9e7, 0xffffff, 0x1f1f1f, 0x646464},
            .toml = kKontor,
        },
    };

  } // namespace

  std::span<const DesignPreset> designPresets() { return kPresets; }

  const DesignPreset* findDesignPreset(std::string_view id) {
    const auto it = std::ranges::find(kPresets, id, &DesignPreset::id);
    return it != kPresets.end() ? &*it : nullptr;
  }

  toml::table designPresetTable(std::string_view id) {
    const DesignPreset* preset = findDesignPreset(id);
    if (preset == nullptr) {
      kLog.warn("unknown design \"{}\", using the default", id);
      return {};
    }
    if (preset->toml.empty()) {
      return {};
    }
    try {
      return toml::parse(preset->toml);
    } catch (const toml::parse_error& e) {
      // A built-in layer that does not parse is a bug; fall back to the defaults rather than an empty desktop.
      kLog.error("design \"{}\" does not parse: {}", id, e.what());
      return {};
    }
  }

  void applyDesignLayer(toml::table& merged, bool userDefinesBars) {
    std::string id = "slate";
    if (const toml::table* design = merged["design"].as_table(); design != nullptr) {
      if (const auto value = (*design)["id"].value<std::string>(); value.has_value()) {
        id = *value;
      }
    }
    toml::table layer = designPresetTable(id);
    if (layer.empty()) {
      return;
    }
    if (userDefinesBars) {
      layer.erase("bar");
    }
    ConfigService::deepMerge(layer, merged);
    merged = std::move(layer);
  }

} // namespace noctalia::config
