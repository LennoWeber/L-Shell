"""Renders the L-Shell brand assets.

The mark ("pearl in the corner"): a bold, rounded L whose corner runs in concentric
arcs around an iridescent pearl. The shell wraps the pearl the way L-Shell wraps
your apps; the concentric corner is the same rule the UI follows.

usage: python3 tools/render-l-shell-brand.py <repo-root>
"""
import math
import pathlib
import sys

import cairo
import numpy as np
from PIL import Image

ROOT = pathlib.Path(sys.argv[1])
BRAND = ROOT / "assets" / "branding"
FONT = "Inter Variable"

# Mark geometry on a 512 grid. The pearl and both corner arcs share one center.
PEARL_C = (282.0, 278.0)
PEARL_R = 64.0
GAP = 22.0
STROKE = 84.0
ARC_R = PEARL_R + GAP + STROKE / 2.0  # centerline radius of the L's corner
TOP_Y = 106.0                          # centerline end of the vertical arm
RIGHT_X = 358.0                        # centerline end of the horizontal arm

PEARL_STOPS = [  # iridescent nacre: blue -> violet -> pink, like Liquid Glass catching light
    (0.0, (0.86, 0.95, 1.00)),
    (0.35, (0.45, 0.70, 1.00)),
    (0.70, (0.66, 0.50, 0.98)),
    (1.0, (0.96, 0.52, 0.80)),
]


def l_path(ctx):
    x = PEARL_C[0] - ARC_R
    ctx.move_to(x, TOP_Y)
    ctx.line_to(x, PEARL_C[1])
    ctx.arc_negative(PEARL_C[0], PEARL_C[1], ARC_R, math.pi, math.pi / 2.0)
    ctx.line_to(RIGHT_X, PEARL_C[1] + ARC_R)


def draw_mark(ctx, l_rgba=(1, 1, 1, 1), pearl=True, mono=None):
    ctx.save()
    l_path(ctx)
    ctx.set_line_width(STROKE)
    ctx.set_line_cap(cairo.LINE_CAP_ROUND)
    ctx.set_line_join(cairo.LINE_JOIN_ROUND)
    ctx.set_source_rgba(*(mono or l_rgba))
    ctx.stroke()

    cx, cy = PEARL_C
    ctx.arc(cx, cy, PEARL_R, 0, 2 * math.pi)
    if mono is not None:
        ctx.set_source_rgba(*mono)
        ctx.fill()
    elif pearl:
        g = cairo.RadialGradient(cx - 22, cy - 26, 4, cx, cy, PEARL_R * 1.05)
        for pos, rgb in PEARL_STOPS:
            g.add_color_stop_rgb(pos, *rgb)
        ctx.set_source(g)
        ctx.fill()
        # specular highlight
        ctx.save()
        ctx.translate(cx - 20, cy - 28)
        ctx.scale(1.0, 0.62)
        hl = cairo.RadialGradient(0, 0, 0, 0, 0, 24)
        hl.add_color_stop_rgba(0, 1, 1, 1, 0.9)
        hl.add_color_stop_rgba(1, 1, 1, 1, 0.0)
        ctx.arc(0, 0, 24, 0, 2 * math.pi)
        ctx.set_source(hl)
        ctx.fill()
        ctx.restore()
    ctx.restore()


def squircle(ctx, x, y, size, n=5.0, steps=256):
    r = size / 2.0
    cx, cy = x + r, y + r
    for i in range(steps + 1):
        t = 2 * math.pi * i / steps
        c, s = math.cos(t), math.sin(t)
        px = cx + r * math.copysign(abs(c) ** (2.0 / n), c)
        py = cy + r * math.copysign(abs(s) ** (2.0 / n), s)
        ctx.line_to(px, py) if i else ctx.move_to(px, py)
    ctx.close_path()


def draw_icon(ctx, size=512.0):
    """App icon: graphite squircle with a glass sheen and the mark."""
    squircle(ctx, 0, 0, size)
    bg = cairo.LinearGradient(0, 0, 0, size)
    bg.add_color_stop_rgb(0, 0.20, 0.20, 0.22)
    bg.add_color_stop_rgb(1, 0.07, 0.07, 0.08)
    ctx.set_source(bg)
    ctx.fill_preserve()
    sheen = cairo.LinearGradient(0, 0, 0, size * 0.55)
    sheen.add_color_stop_rgba(0, 1, 1, 1, 0.10)
    sheen.add_color_stop_rgba(1, 1, 1, 1, 0.0)
    ctx.set_source(sheen)
    ctx.fill_preserve()
    ctx.set_source_rgba(1, 1, 1, 0.12)
    ctx.set_line_width(size / 256.0)
    ctx.stroke()
    ctx.save()
    k = 0.68
    ctx.translate(size * (1 - k) / 2.0 - 8 * k, size * (1 - k) / 2.0 - 0 * k)
    ctx.scale(k * size / 512.0, k * size / 512.0)
    draw_mark(ctx)
    ctx.restore()


def text_extents(text, size, weight=cairo.FONT_WEIGHT_BOLD):
    s = cairo.ImageSurface(cairo.FORMAT_ARGB32, 1, 1)
    c = cairo.Context(s)
    c.select_font_face(FONT, cairo.FONT_SLANT_NORMAL, weight)
    c.set_font_size(size)
    return c.text_extents(text)


def svg(path, w, h, painter):
    surface = cairo.SVGSurface(str(path), w, h)
    painter(cairo.Context(surface))
    surface.finish()


def png(path, w, h, painter):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
    ctx = cairo.Context(surface)
    ctx.set_antialias(cairo.ANTIALIAS_BEST)
    painter(ctx)
    surface.write_to_png(str(path))


def wordmark(ctx, text_rgb):
    draw_icon(ctx, 160.0)
    ctx.select_font_face(FONT, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    ctx.set_font_size(92)
    ext = ctx.text_extents("L-Shell")
    ctx.move_to(200 - ext.x_bearing, 80 - ext.y_bearing - ext.height / 2.0)
    ctx.set_source_rgb(*text_rgb)
    ctx.text_path("L-Shell")
    ctx.fill()


def wallpaper(path, w=3840, h=2160):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
    ctx = cairo.Context(surface)
    ctx.set_antialias(cairo.ANTIALIAS_BEST)
    bg = cairo.LinearGradient(0, 0, w * 0.3, h)
    bg.add_color_stop_rgb(0, 0.035, 0.035, 0.045)
    bg.add_color_stop_rgb(1, 0.075, 0.075, 0.095)
    ctx.set_source(bg)
    ctx.paint()

    cx, cy = w / 2.0, h * 0.45
    for (dx, dy, rad, rgb, a) in [(-260, -120, 1300, (0.30, 0.45, 1.0), 0.20),
                                 (300, 160, 1200, (0.75, 0.35, 0.95), 0.16)]:
        glow = cairo.RadialGradient(cx + dx, cy + dy, 0, cx + dx, cy + dy, rad)
        glow.add_color_stop_rgba(0, *rgb, a)
        glow.add_color_stop_rgba(1, *rgb, 0.0)
        ctx.set_source(glow)
        ctx.paint()

    scale = 1.55
    ctx.save()
    ctx.translate(cx - 256 * scale, cy - 256 * scale)
    ctx.scale(scale, scale)
    draw_mark(ctx, l_rgba=(0.96, 0.96, 0.97, 1))
    ctx.restore()

    ctx.select_font_face(FONT, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(72)
    text = "L - S H E L L"
    ext = ctx.text_extents(text)
    ctx.move_to(cx - ext.width / 2.0 - ext.x_bearing, cy + 256 * scale + 170)
    ctx.set_source_rgba(1, 1, 1, 0.55)
    ctx.show_text(text)
    surface.flush()

    buf = np.frombuffer(surface.get_data(), np.uint8).reshape(h, w, 4)[:, :, [2, 1, 0]].astype(np.float32)
    buf += np.random.default_rng(3).uniform(-0.5, 0.5, buf.shape)
    Image.fromarray(np.clip(buf, 0, 255).astype(np.uint8), "RGB").save(path, optimize=True)


BRAND.mkdir(parents=True, exist_ok=True)
svg(BRAND / "l-shell-mark.svg", 512, 512, lambda c: draw_mark(c))
svg(BRAND / "l-shell-mark-mono.svg", 512, 512, lambda c: draw_mark(c, mono=(0, 0, 0, 1)))
svg(BRAND / "l-shell-icon.svg", 512, 512, lambda c: draw_icon(c))
png(BRAND / "l-shell-icon.png", 512, 512, lambda c: draw_icon(c))
svg(BRAND / "l-shell-logo-dark.svg", 620, 160, lambda c: wordmark(c, (1, 1, 1)))
svg(BRAND / "l-shell-logo-light.svg", 620, 160, lambda c: wordmark(c, (0.11, 0.11, 0.12)))
wallpaper(ROOT / "assets" / "l-shell-wallpaper.png")
print("ok")
