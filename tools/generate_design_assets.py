#!/usr/bin/env python3
"""Draws the bundled artwork of the built-in designs (src/config/design_presets.cpp) from their mockups.

  assets/wallpapers/designs/<id>.png   the backdrop each design ships (2560x1600, drawn on the mockups'
                                       1280x800 canvas); slate.png links to the original L-Shell wallpaper
  assets/branding/designs/<id>-mark.svg  the "L" mark a design puts in its bar, with the letter drawn as an
                                       outline from the design's bundled typeface (no font needed at runtime)

Needs fontTools and ImageMagick (with an SVG delegate). Run from anywhere: tools/generate_design_assets.py
"""

import math
import subprocess
import tempfile
from pathlib import Path

from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

ROOT = Path(__file__).resolve().parent.parent
FONTS = ROOT / "assets" / "fonts" / "system"
W, H = 1280, 800


def polygon(points, fill):
    p = " ".join(f"{x / 100 * W:.1f},{y / 100 * H:.1f}" for x, y in points)
    return f'<polygon points="{p}" fill="{fill}"/>'


def enso(cx, cy, r, width, color, rotation):
    # Three quarters of a circle: the mockup's CSS ring leaves its right border (-45..45 degrees) transparent.
    a0, a1 = math.radians(45), math.radians(315)
    x0, y0 = cx + r * math.cos(a0), cy + r * math.sin(a0)
    x1, y1 = cx + r * math.cos(a1), cy + r * math.sin(a1)
    return (
        f'<path d="M {x0:.2f} {y0:.2f} A {r} {r} 0 1 1 {x1:.2f} {y1:.2f}" fill="none" stroke="{color}" '
        f'stroke-width="{width}" stroke-linecap="round" transform="rotate({rotation} {cx} {cy})"/>'
    )


GRID = "".join(f'<rect x="0" y="{y}" width="{W}" height="1" fill="#0e0e0e"/>' for y in range(0, H, 64)) + "".join(
    f'<rect x="{x}" y="0" width="1" height="{H}" fill="#0e0e0e"/>' for x in range(0, W, 64)
)

# id -> (ground, shapes), transcribed from each mockup's desktop layer.
WALLPAPERS = {
    "meridian": ("#0b0e13",
                 '<circle cx="1050" cy="650" r="450" fill="none" stroke="#17202b"/>'
                 '<circle cx="1050" cy="650" r="350" fill="none" stroke="#17202b"/>'
                 '<circle cx="1050" cy="650" r="250" fill="none" stroke="#1b2633"/>'),
    # Linen paper with an ink wash: the blot's rim is a little uneven, as if laid with a brush, and the whole sheet
    # gets a paper grain afterwards (GRAIN).
    "atelier": ("#eae3d4",
                '<defs><radialGradient id="wash" cx="0.46" cy="0.44" r="0.6">'
                '<stop offset="0" stop-color="#2e4a7d" stop-opacity="0.10"/>'
                '<stop offset="0.8" stop-color="#2e4a7d" stop-opacity="0.08"/>'
                '<stop offset="1" stop-color="#2e4a7d" stop-opacity="0.15"/></radialGradient>'
                '<filter id="brush" x="-20%" y="-20%" width="140%" height="140%">'
                '<feTurbulence type="fractalNoise" baseFrequency="0.009" numOctaves="3" seed="7"/>'
                '<feDisplacementMap in="SourceGraphic" scale="9" xChannelSelector="R" yChannelSelector="G"/>'
                '</filter></defs>'
                '<circle cx="1130" cy="490" r="310" fill="url(#wash)" filter="url(#brush)"/>'
                '<rect x="0" y="520" width="1280" height="1" fill="#d5cbb7"/>'),
    "monolith": ("#000000", GRID),
    "halo": ("#0f1020",
             '<circle cx="160" cy="660" r="280" fill="#16183a"/><circle cx="1030" cy="250" r="130" fill="#1b1e47"/>'
             '<circle cx="950" cy="120" r="30" fill="#262a63"/>'),
    "strata": ("#121417",
               '<rect x="0" y="660" width="1280" height="140" fill="#15181c"/>'
               '<rect x="0" y="710" width="1280" height="90" fill="#181b20"/>'
               '<rect x="0" y="756" width="1280" height="44" fill="#1c2026"/>'),
    "hearth": ("#17120f",
               '<ellipse cx="350" cy="870" rx="550" ry="450" fill="#211913"/>'
               '<ellipse cx="990" cy="910" rx="550" ry="450" fill="#2a1f18"/>'
               '<circle cx="1015" cy="135" r="45" fill="#3b2a1f"/>'),
    "glacier": ("#d6e1ee",
                polygon([(0, 52), (22, 30), (41, 46), (63, 18), (82, 34), (100, 22), (100, 100), (0, 100)], "#e7eef6")
                + polygon([(0, 74), (18, 58), (37, 70), (55, 50), (76, 66), (100, 54), (100, 100), (0, 100)],
                          "#f4f7fb")),
    "avionik": ("#0a0d12",
                '<rect x="0" y="470" width="1280" height="1" fill="#16202c"/>'
                '<rect x="640" y="32" width="1" height="744" fill="#121a24"/>'
                '<circle cx="640" cy="470" r="120" fill="none" stroke="#16202c"/>'),
    "folio": ("#141317",
              '<circle cx="450" cy="510" r="240" fill="none" stroke="#1a181e" stroke-width="60"/>'
              '<circle cx="1070" cy="630" r="90" fill="#1e1b23"/>'),
    "ma": ("#121110", enso(1040, 610, 269, 22, "#1d1a17", -30)),
    "tahoe": ("#13243d",
              polygon([(0, 38), (18, 30), (36, 40), (58, 26), (80, 36), (100, 24), (100, 100), (0, 100)], "#1c3d63")
              + polygon([(0, 58), (22, 48), (42, 60), (64, 46), (84, 56), (100, 44), (100, 100), (0, 100)], "#2f6496")
              + polygon([(0, 78), (20, 70), (44, 80), (66, 68), (86, 76), (100, 66), (100, 100), (0, 100)],
                        "#5e9bcb")),
    "eleven": ("#0b1b3a",
               '<ellipse cx="640" cy="380" rx="266" ry="254" fill="#16337a" transform="rotate(18 640 380)"/>'
               '<ellipse cx="650" cy="380" rx="184" ry="176" fill="#2557c2" transform="rotate(-12 650 380)"/>'
               '<circle cx="660" cy="380" r="100" fill="#5a8ef0"/>'),
    "bruecke": ("#15171c",
                '<circle cx="252" cy="832" r="451" fill="none" stroke="#252a35" stroke-width="2"/>'
                '<circle cx="952" cy="832" r="451" fill="none" stroke="#252a35" stroke-width="2"/>'
                '<rect x="0" y="640" width="1280" height="2" fill="#252a35"/>'
                '<circle cx="960" cy="180" r="60" fill="#20242e"/>'),
    "kontor": ("#e9e9e7", ""),
    # Band: cool midnight ground with two soft, wide glows — calm depth, no pattern.
    "band": ("#0f1013",
             '<defs>'
             '<radialGradient id="bg1" cx="0.78" cy="0.16" r="0.62">'
             '<stop offset="0" stop-color="#1d2c3d" stop-opacity="0.9"/>'
             '<stop offset="1" stop-color="#1d2c3d" stop-opacity="0"/></radialGradient>'
             '<radialGradient id="bg2" cx="0.12" cy="0.86" r="0.55">'
             '<stop offset="0" stop-color="#12202b" stop-opacity="0.75"/>'
             '<stop offset="1" stop-color="#12202b" stop-opacity="0"/></radialGradient>'
             '</defs>'
             '<rect width="1280" height="800" fill="url(#bg1)"/>'
             '<rect width="1280" height="800" fill="url(#bg2)"/>'),
}

# Designs whose backdrop is paper: a fine grain laid over the rendered wallpaper (strength of the noise).
GRAIN = {"atelier": 0.55}

# id -> (typeface file, axes, tile shape, tile color, letter color), from the mockups' bar marks. "letter" draws the
# letter alone, without a tile.
MARKS = {
    "atelier": ("fraunces/Fraunces[SOFT,WONK,opsz,wght].ttf", {"wght": 600, "SOFT": 100, "opsz": 24}, "letter", None,
                "#2E4A7D"),
    "halo": ("outfit/Outfit[wght].ttf", {"wght": 600}, "circle", "#8C9EFF", "#0F1020"),
    "monolith": ("spacegrotesk/SpaceGrotesk[wght].ttf", {"wght": 700}, "square", "#C8FF2E", "#000000"),
    "glacier": ("hankengrotesk/HankenGrotesk[wght].ttf", {"wght": 700}, "rounded", "#2563EB", "#FFFFFF"),
    "bruecke": ("manrope/Manrope[wght].ttf", {"wght": 700}, "rounded", "#4F7CFF", "#FFFFFF"),
    "band": ("geist/Geist[wght].ttf", {"wght": 600}, "letter", None, "#E8E9EC"),
}


def render_wallpapers():
    out = ROOT / "assets" / "wallpapers" / "designs"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        for name, (ground, shapes) in WALLPAPERS.items():
            svg = Path(tmp) / f"{name}.svg"
            svg.write_text(
                f'<svg xmlns="http://www.w3.org/2000/svg" width="2560" height="1600" viewBox="0 0 {W} {H}">'
                f'<rect width="{W}" height="{H}" fill="{ground}"/>{shapes}</svg>'
            )
            png = out / f"{name}.png"
            subprocess.run(["magick", "-background", "none", str(svg), "-strip", str(png)], check=True)
            if name in GRAIN:
                # Overlay a fine grey noise on the sheet: a paper tooth, not a pattern.
                subprocess.run(
                    ["magick", str(png), "-alpha", "off", "(", "-size", "2560x1600", "xc:gray50", "-seed", "11",
                     "-attenuate", str(GRAIN[name]), "+noise", "Gaussian", "-channel", "RGB", "-separate",
                     "-delete", "1,2", "-blur", "0x0.7", ")", "-compose", "overlay", "-composite", "-depth", "8",
                     "-strip", str(png)],
                    check=True,
                )
    slate = out / "slate.png"
    if not slate.exists():
        slate.symlink_to("../../l-shell-wallpaper.png")


def letter_path(font_file, axes, size, box):
    font = TTFont(FONTS / font_file)
    if "fvar" in font:
        font = instantiateVariableFont(font, axes)
    glyphs = font.getGlyphSet()
    name = font.getBestCmap()[ord("L")]
    scale = size / font["head"].unitsPerEm
    bounds = BoundsPen(glyphs)
    glyphs[name].draw(bounds)
    x_min, y_min, x_max, y_max = bounds.bounds
    # Center the letter's ink in the tile; font units are y-up.
    dx = (box - (x_max - x_min) * scale) / 2 - x_min * scale
    dy = (box + (y_max - y_min) * scale) / 2 + y_min * scale
    pen = SVGPathPen(glyphs)
    glyphs[name].draw(TransformPen(pen, (scale, 0, 0, -scale, dx, dy)))
    return pen.getCommands()


def render_marks():
    out = ROOT / "assets" / "branding" / "designs"
    out.mkdir(parents=True, exist_ok=True)
    box = 24
    for name, (font_file, axes, shape, tile, letter) in MARKS.items():
        if shape == "letter":
            path = letter_path(font_file, axes, 21.0, box)
            (out / f"{name}-mark.svg").write_text(
                f'<svg xmlns="http://www.w3.org/2000/svg" width="{box}" height="{box}" viewBox="0 0 {box} {box}">'
                f'<path d="{path}" fill="{letter}"/></svg>\n'
            )
            continue
        if shape == "circle":
            tile_svg = f'<circle cx="12" cy="12" r="12" fill="{tile}"/>'
        elif shape == "square":
            tile_svg = f'<rect width="24" height="24" fill="{tile}"/>'
        else:
            tile_svg = f'<rect width="24" height="24" rx="6.5" fill="{tile}"/>'
        path = letter_path(font_file, axes, 13.5, box)
        (out / f"{name}-mark.svg").write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{box}" height="{box}" viewBox="0 0 {box} {box}">'
            f'{tile_svg}<path d="{path}" fill="{letter}"/></svg>\n'
        )


if __name__ == "__main__":
    render_wallpapers()
    render_marks()
