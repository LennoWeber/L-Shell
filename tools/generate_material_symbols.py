#!/usr/bin/env python3
"""Builds the Material Symbols icon fonts the designs draw their icons with.

L-Shell names its icons after Tabler (see assets/fonts/tabler.json). A design whose spec calls for Material
Symbols (Outlined, Rounded or Sharp) has the glyph renderer draw the Material equivalent of each Tabler icon
instead. This script subsets Google's variable Material Symbols fonts to the equivalents listed in MAP, keeps
the FILL/GRAD/opsz/wght axes, and gives every kept glyph a codepoint of its own in Supplementary Private Use
Area-B (U+100000 + n, the same n in all three styles), so they never collide with Tabler's codepoints.

Usage:
  tools/generate_material_symbols.py <dir with MaterialSymbols{Outlined,Rounded,Sharp}[FILL,GRAD,opsz,wght].ttf
                                      and the matching .codepoints files>

Writes assets/fonts/material-symbols-{outlined,rounded,sharp}.ttf and assets/fonts/material-symbols.json,
which maps a Tabler icon name to {"codepoint": "U+10xxxx", "fill": bool}. "fill" draws the glyph filled
whatever the design's fill setting, for Tabler icons that are filled by name ("bell-filled").
"""

import json
import sys
from pathlib import Path

from fontTools import subset
from fontTools.ttLib import TTFont, newTable
from fontTools.ttLib.tables._c_m_a_p import cmap_format_12

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "fonts"
STYLES = ("Outlined", "Rounded", "Sharp")
BASE = 0x100000

# Tabler icon name -> Material Symbols name. A trailing "!" draws it filled.
MAP = {
    "activity": "monitoring",
    "activity-heartbeat": "ecg_heart",
    "adjustments": "tune",
    "adjustments-filled": "tune!",
    "adjustments-horizontal": "tune",
    "album": "album",
    "alert-circle": "error",
    "alert-octagon": "report",
    "alert-triangle": "warning",
    "apps": "apps",
    "app-window": "web_asset",
    "app-window-filled": "web_asset!",
    "arrow-back-up": "undo",
    "arrow-forward-up": "redo",
    "arrow-left": "arrow_back",
    "arrow-right": "arrow_forward",
    "arrow-up-right": "arrow_outward",
    "arrows-exchange": "swap_horiz",
    "arrows-horizontal": "arrow_range",
    "arrows-move": "open_with",
    "arrows-random": "shuffle",
    "arrows-shuffle": "shuffle",
    "arrows-up-down": "swap_vert",
    "at": "alternate_email",
    "automation": "automation",
    "background": "wallpaper",
    "badge": "badge",
    "battery": "battery_0_bar",
    "battery-1": "battery_2_bar",
    "battery-2": "battery_3_bar",
    "battery-3": "battery_5_bar",
    "battery-3-filled": "battery_5_bar",
    "battery-4": "battery_full",
    "battery-charging": "battery_charging_full",
    "battery-charging-2": "battery_charging_full",
    "battery-exclamation": "battery_alert",
    "bell": "notifications",
    "bell-filled": "notifications!",
    "bell-off": "notifications_off",
    "bell-ringing": "notifications_active",
    "bluetooth": "bluetooth",
    "bluetooth-connected": "bluetooth_connected",
    "bluetooth-off": "bluetooth_disabled",
    "blur": "blur_on",
    "bold": "format_bold",
    "bolt": "bolt",
    "bolt-filled": "bolt!",
    "box": "package_2",
    "brand-speedtest": "speed",
    "briefcase": "work",
    "brightness": "brightness_6",
    "brightness-down": "brightness_low",
    "brightness-down-filled": "brightness_low",
    "brightness-up": "light_mode",
    "brightness-up-filled": "light_mode",
    "brush": "brush",
    "bug": "bug_report",
    "calculator": "calculate",
    "calendar": "calendar_today",
    "calendar-cog": "edit_calendar",
    "calendar-event": "event",
    "calendar-filled": "calendar_today!",
    "calendar-off": "event_busy",
    "camera": "photo_camera",
    "camera-filled": "photo_camera!",
    "camera-off": "no_photography",
    "cancel": "cancel",
    "capture": "screenshot_region",
    "cast": "cast",
    "category": "category",
    "cell-signal-1": "signal_cellular_0_bar",
    "cell-signal-2": "signal_cellular_1_bar",
    "cell-signal-3": "signal_cellular_2_bar",
    "cell-signal-4": "signal_cellular_3_bar",
    "cell-signal-5": "signal_cellular_4_bar",
    "cell-signal-off": "signal_cellular_off",
    "check": "check",
    "checkbox": "check_box",
    "chevron-down": "expand_more",
    "chevron-left": "chevron_left",
    "chevron-right": "chevron_right",
    "chevron-up": "expand_less",
    "circle": "circle",
    "circle-check": "check_circle",
    "circle-dot": "radio_button_checked",
    "circle-filled": "circle!",
    "circle-half-2": "contrast",
    "circle-x": "cancel",
    "clipboard": "content_paste",
    "clipboard-copy": "content_copy",
    "clipboard-text": "assignment",
    "clock": "schedule",
    "cloud": "cloud",
    "cloud-bolt": "thunderstorm",
    "cloud-fog": "foggy",
    "cloud-off": "cloud_off",
    "cloud-rain": "rainy",
    "cloud-snow": "weather_snowy",
    "cloud-sun": "partly_cloudy_day",
    "code": "code",
    "color-picker": "colorize",
    "color-swatch": "palette",
    "command": "keyboard_command_key",
    "contrast-filled": "contrast",
    "cookie": "cookie",
    "copy": "content_copy",
    "copy-plus": "library_add",
    "cpu": "memory",
    "crop": "crop",
    "database": "database",
    "device-airpods": "earbuds",
    "device-desktop": "desktop_windows",
    "device-floppy": "save",
    "device-gamepad-2": "sports_esports",
    "device-laptop": "laptop",
    "device-mobile": "smartphone",
    "device-speaker": "speaker",
    "device-tv": "tv",
    "device-watch": "watch",
    "disc": "album",
    "dots": "more_horiz",
    "dots-vertical": "more_vert",
    "download": "download",
    "edit": "edit",
    "eraser": "ink_eraser",
    "ethernet": "lan",
    "exclamation-circle": "error",
    "external-link": "open_in_new",
    "eye": "visibility",
    "eye-off": "visibility_off",
    "file": "draft",
    "file-description": "description",
    "file-text": "article",
    "files": "file_copy",
    "filter": "filter_list",
    "fingerprint": "fingerprint",
    "flag": "flag",
    "flame": "local_fire_department",
    "flask": "science",
    "flip-horizontal": "flip",
    "focus": "center_focus_strong",
    "folder": "folder",
    "function": "function",
    "gauge": "speed",
    "graph": "show_chart",
    "grid-dots": "apps",
    "grip-vertical": "drag_indicator",
    "hand-stop": "back_hand",
    "headphones": "headphones",
    "headset": "headset_mic",
    "heart": "favorite",
    "help": "help",
    "help-circle": "help",
    "highlight": "ink_highlighter",
    "history": "history",
    "home": "home",
    "hourglass": "hourglass",
    "hourglass-empty": "hourglass_empty",
    "key": "key",
    "keyboard": "keyboard",
    "keyboard-filled": "keyboard!",
    "keyboard-off": "keyboard_off",
    "label": "label",
    "layers-intersect": "layers",
    "layout": "space_dashboard",
    "layout-board": "dashboard",
    "layout-board-filled": "dashboard!",
    "layout-grid": "grid_view",
    "layout-sidebar-right": "view_sidebar",
    "leaf": "eco",
    "library-photo": "photo_library",
    "link": "link",
    "list": "list",
    "location": "near_me",
    "location-filled": "near_me!",
    "lock": "lock",
    "login": "login",
    "logout": "logout",
    "mail": "mail",
    "map": "map",
    "map-pin": "location_on",
    "map-pin-off": "location_off",
    "menu": "menu",
    "menu-2": "menu",
    "message": "chat",
    "message-circle": "chat_bubble",
    "message-circle-filled": "chat_bubble!",
    "microphone": "mic",
    "microphone-off": "mic_off",
    "minus": "remove",
    "mood-smile": "mood",
    "mood-smile-beam": "sentiment_very_satisfied",
    "moon": "dark_mode",
    "moon-filled": "dark_mode!",
    "moon-stars": "bedtime",
    "mountain": "landscape",
    "mouse-2": "mouse",
    "mug": "coffee",
    "mug-filled": "coffee!",
    "music": "music_note",
    "music-off": "music_off",
    "navigation": "navigation",
    "navigation-filled": "navigation!",
    "network": "hub",
    "notification": "notifications",
    "paint": "format_paint",
    "palette": "palette",
    "palette-filled": "palette!",
    "password": "password",
    "phone": "call",
    "photo": "image",
    "photo-filled": "image!",
    "photo-off": "hide_image",
    "pin": "push_pin",
    "pin-filled": "push_pin!",
    "pinned": "push_pin",
    "pinned-off": "keep_off",
    "player-pause": "pause",
    "player-pause-filled": "pause!",
    "player-play": "play_arrow",
    "player-play-filled": "play_arrow!",
    "player-skip-back-filled": "skip_previous!",
    "player-skip-forward-filled": "skip_next!",
    "player-stop-filled": "stop!",
    "plug": "power",
    "plug-connected": "power",
    "plug-filled": "power!",
    "plug-off": "power_off",
    "plus": "add",
    "pointer": "arrow_selector_tool",
    "power": "power_settings_new",
    "presentation": "present_to_all",
    "puzzle": "extension",
    "puzzle-filled": "extension!",
    "radio": "radio",
    "refresh": "refresh",
    "reload": "refresh",
    "repeat": "repeat",
    "repeat-once": "repeat_one",
    "rocket": "rocket_launch",
    "run": "directions_run",
    "scale": "balance",
    "school": "school",
    "screen-share": "screen_share",
    "screen-share-off": "stop_screen_share",
    "screenshot": "screenshot_monitor",
    "search": "search",
    "send": "send",
    "settings": "settings",
    "settings-filled": "settings!",
    "shield-check": "verified_user",
    "shield-lock": "shield_lock",
    "snowflake": "ac_unit",
    "square": "square",
    "star": "star",
    "star-filled": "star!",
    "sun": "light_mode",
    "sunrise": "wb_twilight",
    "sunset": "wb_twilight",
    "tag": "sell",
    "target": "target",
    "temperature": "thermostat",
    "terminal": "terminal",
    "thermometer": "thermometer",
    "thumb-up": "thumb_up",
    "thumb-up-filled": "thumb_up!",
    "timezone": "globe",
    "tool": "build",
    "toggle-right": "toggle_on",
    "toggle-left": "toggle_off",
    "layout-grid-filled": "grid_view!",
    "apps-filled": "apps!",
    "trash": "delete",
    "trending-up": "trending_up",
    "typeface": "font_download",
    "typography": "text_fields",
    "upload": "upload",
    "user": "person",
    "user-filled": "person!",
    "video": "videocam",
    "volume": "volume_up",
    "volume-2": "volume_down",
    "volume-3": "volume_mute",
    "volume-off": "volume_off",
    "wallpaper": "wallpaper",
    "wand": "auto_fix_high",
    "wifi": "wifi",
    "wifi-0": "signal_wifi_0_bar",
    "wifi-1": "wifi_1_bar",
    "wifi-2": "wifi_2_bar",
    "wifi-3": "wifi",
    "wifi-off": "wifi_off",
    "window": "web_asset",
    "world": "language",
    "x": "close",
    "zoom": "zoom_in",
    "zzz": "bedtime",
}


def read_codepoints(path):
    table = {}
    for line in path.read_text().splitlines():
        name, hexcp = line.split()
        table[name] = int(hexcp, 16)
    return table


def main():
    src = Path(sys.argv[1])
    upstream = read_codepoints(next(src.glob("MaterialSymbolsOutlined*.codepoints")))

    unknown = sorted({m.rstrip("!") for m in MAP.values()} - upstream.keys())
    if unknown:
        sys.exit(f"not in Material Symbols: {', '.join(unknown)}")

    names = sorted({m.rstrip("!") for m in MAP.values()})
    ours = {name: BASE + i for i, name in enumerate(names)}

    for style in STYLES:
        font = TTFont(next(src.glob(f"MaterialSymbols{style}*.ttf")))
        options = subset.Options()
        options.layout_features = []  # ligatures are not used; codepoints address the glyphs
        options.name_IDs = ["*"]
        options.notdef_outline = True
        subsetter = subset.Subsetter(options)
        subsetter.populate(unicodes=[upstream[name] for name in names])
        subsetter.subset(font)

        best = font.getBestCmap()
        glyph_of = {name: best[upstream[name]] for name in names}
        table = cmap_format_12(12)
        table.platformID, table.platEncID, table.language = 3, 10, 0
        table.cmap = {ours[name]: glyph_of[name] for name in names}
        cmap = newTable("cmap")
        cmap.tableVersion = 0
        cmap.tables = [table]
        font["cmap"] = cmap
        for tag in ("GSUB", "GPOS", "GDEF"):
            if tag in font:
                del font[tag]
        font.save(OUT / f"material-symbols-{style.lower()}.ttf")

    index = {
        tabler: {"codepoint": f"U+{ours[m.rstrip('!')]:06X}", "fill": m.endswith("!")}
        for tabler, m in sorted(MAP.items())
    }
    (OUT / "material-symbols.json").write_text(json.dumps(index, indent=1) + "\n")
    print(f"{len(names)} glyphs, {len(MAP)} Tabler names")


if __name__ == "__main__":
    main()
