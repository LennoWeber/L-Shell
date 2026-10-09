"""Renders the L-Shell default wallpaper: a nautilus cross-section on deep petrol."""
import math
import sys

import cairo
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 3840, 2160
OUT = sys.argv[1]  # usage: python3 tools/render-l-shell-wallpaper.py assets/l-shell-wallpaper.png
FONT = "/usr/share/fonts/google-noto/NotoSans-Light.ttf"

# Palette
BG_TOP = (0.024, 0.078, 0.094)      # #061418
BG_BOTTOM = (0.047, 0.157, 0.169)   # #0c282b
GLOW = (0.12, 0.43, 0.42)           # teal glow
CORAL = (1.00, 0.45, 0.36)          # #ff735c
AMBER = (1.00, 0.76, 0.45)          # #ffc273
PEARL = (1.00, 0.88, 0.76)          # #ffe0c2, shell core
LINE = (1.00, 0.90, 0.80)           # chamber walls


def lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


# ── Nautilus geometry ─────────────────────────────────────────────────────────
GROWTH = math.log(3.0) / (2 * math.pi)   # radius triples per full turn, like a real nautilus
CHAMBERS = 30
STEP = 2 * math.pi / 12                  # 12 chambers per whorl
THETA_END = 5.5 * math.pi
THETA_START = THETA_END - CHAMBERS * STEP
R_OUTER = 640.0                          # radius at the shell opening


def r(theta):
    return R_OUTER * math.exp(GROWTH * (theta - THETA_END))


def pt(theta, radius, cx, cy):
    # Rotate so the opening sits lower-right and the shell reads as a whole.
    a = theta + 0.35
    return cx + radius * math.cos(a), cy + radius * math.sin(a)


def spiral_points(t0, t1, scale, cx, cy, n=48):
    return [pt(t0 + (t1 - t0) * i / n, r(t0 + (t1 - t0) * i / n) * scale, cx, cy) for i in range(n + 1)]


def shell_bbox():
    pts = spiral_points(THETA_START, THETA_END, 1.0, 0, 0, n=2000)
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    return min(xs), max(xs), min(ys), max(ys)


def chamber_path(ctx, k, cx, cy):
    t0 = THETA_START + k * STEP
    t1 = t0 + STEP
    outer = spiral_points(t0, t1, 1.0, cx, cy)
    inner = spiral_points(t1 - 2 * math.pi, t0 - 2 * math.pi, 1.0, cx, cy)
    ctx.new_path()
    ctx.move_to(*outer[0])
    for p in outer[1:]:
        ctx.line_to(*p)
    septum_to(ctx, t1, cx, cy)
    for p in inner[1:]:
        ctx.line_to(*p)
    # Close along the previous septum, traced backwards.
    septum_to(ctx, t0, cx, cy, reverse=True)
    ctx.close_path()


def septum_to(ctx, theta, cx, cy, reverse=False):
    """Curved chamber wall at angle theta, concave toward the opening like a real nautilus."""
    ro, ri = r(theta), r(theta - 2 * math.pi)
    c1 = pt(theta + STEP * 0.42, ro - (ro - ri) * 0.18, cx, cy)
    c2 = pt(theta + STEP * 0.42, ri + (ro - ri) * 0.18, cx, cy)
    if reverse:
        ctx.curve_to(*c2, *c1, *pt(theta, ro, cx, cy))
    else:
        ctx.curve_to(*c1, *c2, *pt(theta - 2 * math.pi, ri, cx, cy))


def draw_shell(ctx, cx, cy, wall):
    pole = pt(THETA_START, 0.0, cx, cy)
    for k in range(CHAMBERS):
        chamber_path(ctx, k, cx, cy)
        t = k / (CHAMBERS - 1)
        base = lerp(PEARL, CORAL, min(1.0, t * 1.15))
        rhythm = 1.0 if k % 2 == 0 else 0.90
        outer_r = r(THETA_START + (k + 1) * STEP)
        g = cairo.RadialGradient(*pole, 0, *pole, outer_r * 1.05)
        g.add_color_stop_rgba(0.0, *lerp(base, AMBER, 0.55), 0.95 * rhythm)
        g.add_color_stop_rgba(1.0, *[c * rhythm for c in base], 0.95)
        ctx.set_source(g)
        ctx.fill()

    ctx.set_line_join(cairo.LINE_JOIN_ROUND)
    ctx.set_line_cap(cairo.LINE_CAP_ROUND)

    # Chamber walls.
    for k in range(CHAMBERS + 1):
        theta = THETA_START + k * STEP
        if r(theta) < 6:
            continue
        ctx.new_path()
        ctx.move_to(*pt(theta, r(theta), cx, cy))
        septum_to(ctx, theta, cx, cy)
        ctx.set_source_rgba(*LINE, 0.85)
        ctx.set_line_width(max(1.5, wall * 0.55 * min(1.0, r(theta) / 220)))
        ctx.stroke()

    # Spiral wall from the pole to the opening.
    pts = spiral_points(THETA_START - 2 * math.pi, THETA_END, 1.0, cx, cy, n=1600)
    ctx.new_path()
    ctx.move_to(*pts[0])
    for p in pts[1:]:
        ctx.line_to(*p)
    ctx.set_source_rgba(*LINE, 0.95)
    ctx.set_line_width(wall)
    ctx.stroke()


def surface_to_image(surface):
    buf = np.frombuffer(surface.get_data(), np.uint8).reshape(H, W, 4)
    # Cairo ARGB32 is premultiplied BGRA on little-endian.
    rgba = buf[:, :, [2, 1, 0, 3]].astype(np.float32)
    alpha = rgba[:, :, 3:4] / 255.0
    rgb = np.where(alpha > 0, rgba[:, :, :3] / np.maximum(alpha, 1e-6), 0)
    out = np.concatenate([rgb, rgba[:, :, 3:4]], axis=2)
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA")


def new_layer():
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, W, H)
    ctx = cairo.Context(surface)
    ctx.set_antialias(cairo.ANTIALIAS_BEST)
    return surface, ctx


# ── Background ────────────────────────────────────────────────────────────────
bg_surface, ctx = new_layer()
grad = cairo.LinearGradient(0, 0, W * 0.4, H)
grad.add_color_stop_rgb(0, *BG_TOP)
grad.add_color_stop_rgb(1, *BG_BOTTOM)
ctx.set_source(grad)
ctx.paint()

minx, maxx, miny, maxy = shell_bbox()
cx = W / 2 - (minx + maxx) / 2
cy = H * 0.46 - (miny + maxy) / 2
center_x, center_y = W / 2, H * 0.46

glow = cairo.RadialGradient(center_x, center_y, 0, center_x, center_y, 1500)
glow.add_color_stop_rgba(0, *GLOW, 0.32)
glow.add_color_stop_rgba(0.5, *GLOW, 0.10)
glow.add_color_stop_rgba(1, *GLOW, 0.0)
ctx.set_source(glow)
ctx.paint()

# Faint swell lines across the lower third.
for i in range(7):
    base = H * 0.80 + i * 52
    amp = 34 + i * 6
    phase = i * 0.7
    ctx.new_path()
    for x in range(-40, W + 41, 16):
        y = base + amp * math.sin(x / 520 + phase) + 0.4 * amp * math.sin(x / 210 + phase * 1.7)
        if x == -40:
            ctx.move_to(x, y)
        else:
            ctx.line_to(x, y)
    ctx.set_source_rgba(*GLOW, 0.16 - i * 0.018)
    ctx.set_line_width(3)
    ctx.stroke()

background = surface_to_image(bg_surface).convert("RGB")

# ── Shell with glow ───────────────────────────────────────────────────────────
shell_surface, sctx = new_layer()
draw_shell(sctx, cx, cy, wall=8)
shell = surface_to_image(shell_surface)

halo = shell.filter(ImageFilter.GaussianBlur(70))
halo_alpha = halo.getchannel("A").point(lambda a: int(a * 0.55))
halo.putalpha(halo_alpha)

image = background.convert("RGBA")
image.alpha_composite(halo)
image.alpha_composite(shell)

# ── Wordmark ──────────────────────────────────────────────────────────────────
draw = ImageDraw.Draw(image)
font = ImageFont.truetype(FONT, 84)
text = "L-SHELL"
tracking = 46
widths = [draw.textlength(c, font=font) for c in text]
total = sum(widths) + tracking * (len(text) - 1)
x = (W - total) / 2
y = center_y + (maxy - miny) / 2 + 150
for c, w in zip(text, widths):
    draw.text((x, y), c, font=font, fill=(255, 205, 170, 215))
    x += w + tracking

# Dither to avoid banding in the dark gradient.
arr = np.asarray(image.convert("RGB")).astype(np.float32)
rng = np.random.default_rng(7)
arr += rng.uniform(-0.5, 0.5, arr.shape)
Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB").save(OUT, optimize=True)
print(OUT)
