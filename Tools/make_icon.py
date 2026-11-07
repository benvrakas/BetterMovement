"""Draws the Better Movement logo: one large Pioneer sliding up a ramp out of the water, framed by an orange ring,
with a small parachute above. Drawn at 2048 px and scaled down.

  python Tools/make_icon.py [output folder]

With no folder, writes Docs/Images/icon-1080.png (full resolution), Docs/Images/icon-512.png (mod page) and
Resources/Icon128.png (shown in the game's Mods menu). With a folder, writes all three there instead.
"""
import math
import os
import sys
from PIL import Image, ImageDraw, ImageFilter

S = 2048
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUIT = (226, 212, 180)
SUIT_SHADE = (176, 160, 128)
HARNESS = (34, 34, 40)
HELMET = (246, 128, 36)
HELMET_HI = (255, 200, 140)
VISOR = (12, 16, 28)
VISOR_HI = (100, 160, 236)
BOOT = (100, 60, 30)
GLOVE = (62, 46, 34)
ORANGE = (250, 140, 40)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(len(a)))


def gradient(size, top, bottom):
    w, h = size
    img = Image.new('RGBA', size)
    d = ImageDraw.Draw(img)
    for y in range(h):
        d.line([(0, y), (w, y)], fill=lerp(top, bottom, y / max(1, h - 1)))
    return img


def P(lx, ly, origin, scale, axis_u, axis_n):
    # Local x along axis_u, local y along axis_n.
    return (origin[0] + (lx * axis_u[0] + ly * axis_n[0]) * scale, origin[1] + (lx * axis_u[1] + ly * axis_n[1]) * scale)


def limb(draw, pts, width, color):
    draw.line(pts, fill=color, width=int(width), joint='curve')
    for p in (pts[0], pts[-1]):
        r = width / 2
        draw.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=color)


def helmet(draw, c, r, facing):
    draw.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], fill=HELMET)
    draw.arc([c[0] - r * 0.92, c[1] - r * 0.92, c[0] + r * 0.92, c[1] + r * 0.92], 200, 330, fill=(248, 248, 242), width=int(r * 0.14))
    vx = c[0] + facing[0] * r * 0.32
    vy = c[1] + facing[1] * r * 0.32
    draw.ellipse([vx - r * 0.62, vy - r * 0.5, vx + r * 0.62, vy + r * 0.5], fill=VISOR)
    draw.ellipse([vx - r * 0.42, vy - r * 0.34, vx - r * 0.05, vy - r * 0.12], fill=VISOR_HI)
    draw.ellipse([c[0] - r * 0.7, c[1] - r * 0.85, c[0] - r * 0.25, c[1] - r * 0.55], fill=HELMET_HI)


def pioneer_sliding(layer, origin, scale, u, n):
    # Local frame: x up the slope, y away from the ramp surface. Feet first, sitting low, one hand trailing.
    dr = ImageDraw.Draw(layer)
    p = lambda x, y: P(x, y, origin, scale, u, n)
    w = 0.16 * scale
    hip, neck = p(0.0, 0.26), p(-0.14, 0.88)
    limb(dr, [hip, p(0.34, 0.56), p(0.2, 0.04)], w, SUIT_SHADE)
    limb(dr, [p(0.16, 0.02), p(0.34, 0.0)], w * 1.05, BOOT)
    limb(dr, [p(-0.1, 0.8), p(-0.36, 0.46), p(-0.52, 0.06)], w * 0.8, SUIT_SHADE)
    limb(dr, [p(-0.52, 0.06), p(-0.6, 0.0)], w * 0.85, GLOVE)
    dr.polygon([p(-0.2, 0.32), p(-0.42, 0.42), p(-0.46, 0.88), p(-0.24, 0.92)], fill=HARNESS)
    limb(dr, [hip, neck], w * 1.9, SUIT)
    limb(dr, [p(-0.1, 0.34), p(0.12, 0.32)], w * 0.38, HARNESS)
    dr.line([p(-0.16, 0.82), p(0.06, 0.42)], fill=HARNESS, width=int(w * 0.3))
    dr.line([p(-0.04, 0.68), p(-0.2, 0.64)], fill=(150, 220, 70), width=int(w * 0.24))
    limb(dr, [hip, p(0.5, 0.14), p(0.98, 0.02)], w * 1.05, SUIT)
    kp = p(0.5, 0.15)
    dr.ellipse([kp[0] - w * 0.55, kp[1] - w * 0.55, kp[0] + w * 0.55, kp[1] + w * 0.55], fill=HARNESS)
    limb(dr, [p(0.96, 0.0), p(1.14, 0.0), p(1.14, 0.16)], w * 1.15, BOOT)
    limb(dr, [p(-0.08, 0.8), p(0.2, 0.88), p(0.44, 1.0)], w * 0.85, SUIT)
    limb(dr, [p(0.42, 1.0), p(0.52, 1.04)], w * 0.9, GLOVE)
    helmet(dr, p(-0.18, 1.12), 0.26 * scale, (u[0] * 0.7 + n[0] * 0.2, u[1] * 0.7 + n[1] * 0.2))


def pioneer_hanging(layer, origin, scale):
    u, n = (1.0, 0.0), (0.0, -1.0)
    dr = ImageDraw.Draw(layer)
    p = lambda x, y: P(x, y, origin, scale, u, n)
    w = 0.16 * scale
    hip, neck = p(0.0, 0.0), p(0.0, 0.62)
    limb(dr, [hip, p(-0.14, -0.4), p(-0.06, -0.78)], w, SUIT_SHADE)
    limb(dr, [p(-0.06, -0.78), p(0.06, -0.84)], w * 1.05, BOOT)
    limb(dr, [p(-0.16, 0.58), p(-0.34, 0.86), p(-0.3, 1.12)], w * 0.8, SUIT_SHADE)
    limb(dr, [hip, neck], w * 1.9, SUIT)
    dr.line([p(-0.1, 0.5), p(0.12, 0.12)], fill=HARNESS, width=int(w * 0.35))
    limb(dr, [hip, p(0.16, -0.38), p(0.12, -0.8)], w * 1.05, SUIT)
    limb(dr, [p(0.12, -0.8), p(0.26, -0.86)], w * 1.15, BOOT)
    limb(dr, [p(0.16, 0.58), p(0.34, 0.86), p(0.3, 1.12)], w * 0.85, SUIT)
    helmet(dr, p(0.0, 0.86), 0.25 * scale, (0.45, 0.15))
    return p(-0.3, 1.14), p(0.3, 1.14)


def parachute(layer, center, width, height):
    dr = ImageDraw.Draw(layer)
    cells = 5
    pts_top, pts_bot = [], []
    for i in range(cells * 4 + 1):
        a = math.pi + math.pi * i / (cells * 4)
        x = center[0] + math.cos(a) * width / 2
        pts_top.append((x, center[1] + math.sin(a) * height * 0.9 - height * 0.05))
        pts_bot.append((x, center[1] + math.sin(a) * height * 0.55 + height * 0.25))
    dr.polygon(pts_top + pts_bot[::-1], fill=(240, 243, 247, 255))
    for c in range(1, cells):
        i = c * 4
        dr.line([pts_top[i], pts_bot[i]], fill=(170, 178, 190, 255), width=10)
    dr.line(pts_bot, fill=(150, 158, 170, 255), width=8)
    return pts_bot


# Background: deep blue, lighter behind the slider.
img = gradient((S, S), (22, 52, 104, 255), (8, 18, 40, 255))
glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ImageDraw.Draw(glow).ellipse([380, 260, 1720, 1600], fill=(60, 120, 200, 110))
img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(160)))

# Orange ring framing the logo.
ring = Image.new('RGBA', (S, S), (0, 0, 0, 0))
RC, RR, RW = (1024, 950), 700, 92
ImageDraw.Draw(ring).ellipse([RC[0] - RR, RC[1] - RR, RC[0] + RR, RC[1] + RR], outline=ORANGE + (255,), width=RW)
img.alpha_composite(ring)

# Ramp: a foundation slab rising from the water to the right edge.
FOOT = (560, 1520)
TOP = (S + 60, 860)
ux, uy = TOP[0] - FOOT[0], TOP[1] - FOOT[1]
ln = math.hypot(ux, uy)
ux, uy = ux / ln, uy / ln
nx, ny = uy, -ux
if ny > 0:
    nx, ny = -nx, -ny

d = ImageDraw.Draw(img)
# Water in the lower left, up to the ramp's foot.
water = [(0, 1500), (FOOT[0] + 60, 1500), (FOOT[0] - 60, S), (0, S)]
mask = Image.new('L', (S, S), 0)
ImageDraw.Draw(mask).polygon(water, fill=255)
img.paste(gradient((S, S), (70, 160, 236, 255), (20, 70, 150, 255)), (0, 0), mask)
d = ImageDraw.Draw(img)
for row, y0 in enumerate([1600, 1740, 1880]):
    pts = [(x, y0 + 16 * math.sin(x / 60.0 + row * 1.9)) for x in range(0, FOOT[0] - row * 40, 8)]
    d.line(pts, fill=(225, 242, 255, 220), width=18)

side = [FOOT, TOP, (S + 60, S + 60), (FOOT[0] - 140, S + 60)]
d.polygon(side, fill=(44, 46, 54, 255))
for i in range(4):
    t = i / 3
    x = FOOT[0] + (TOP[0] - FOOT[0]) * t
    y = FOOT[1] + (TOP[1] - FOOT[1]) * t
    d.line([(x, y + 30), (x - 60 * (1 - t), S)], fill=(26, 27, 32, 255), width=30)
d.line([FOOT, TOP], fill=(214, 220, 228, 255), width=70)
d.line([(FOOT[0] + nx * 22, FOOT[1] + ny * 22), (TOP[0] + nx * 22, TOP[1] + ny * 22)], fill=(244, 247, 250, 255), width=14)

# Speed streaks behind the slider.
t = 0.36
base = (FOOT[0] + (TOP[0] - FOOT[0]) * t, FOOT[1] + (TOP[1] - FOOT[1]) * t)
origin = (base[0] + nx * 30, base[1] + ny * 30)
streaks = Image.new('RGBA', (S, S), (0, 0, 0, 0))
sd = ImageDraw.Draw(streaks)
for off, length, width in [(0.25, 640, 30), (0.62, 820, 34), (1.0, 560, 28)]:
    start = (origin[0] - ux * 230 + nx * off * 380, origin[1] - uy * 230 + ny * off * 380)
    steps = 36
    for s in range(steps):
        a0 = s / steps
        p0 = (start[0] - ux * length * a0, start[1] - uy * length * a0)
        p1 = (start[0] - ux * length * (a0 + 1 / steps), start[1] - uy * length * (a0 + 1 / steps))
        sd.line([p0, p1], fill=(255, 255, 255, int(235 * (1 - a0) ** 1.4)), width=width)
img.alpha_composite(streaks)

# Spray where the slide left the water.
d = ImageDraw.Draw(img)
for dx, dy, r in [(-50, -80, 34), (-150, -170, 26), (-10, -200, 20), (-230, -70, 28), (-120, -300, 14), (-290, -200, 18)]:
    d.ellipse([FOOT[0] + dx - r, FOOT[1] + dy - r, FOOT[0] + dx + r, FOOT[1] + dy + r], fill=(230, 245, 255, 240))

people = Image.new('RGBA', (S, S), (0, 0, 0, 0))
pioneer_sliding(people, origin, 420, (ux, uy), (nx, ny))

# Small parachuter in the upper left, inside the ring.
chute_center = (560, 380)
bot = parachute(people, chute_center, 380, 140)
lh, rh = pioneer_hanging(people, (580, 690), 130)
pdraw = ImageDraw.Draw(people)
for i in (0, 5, 10, 15, 20):
    target = lh if i < 10 else rh
    pdraw.line([bot[i], target], fill=(220, 226, 236, 255), width=6)

shadow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
shadow.paste((0, 0, 0, 130), (0, 0), people.split()[3])
img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(16)), (16, 20))
img.alpha_composite(people)

final = img.convert('RGB')
if len(sys.argv) > 1:
    out = sys.argv[1]
    paths = [os.path.join(out, 'icon-1080.png'), os.path.join(out, 'icon-512.png'), os.path.join(out, 'Icon128.png')]
else:
    paths = [os.path.join(ROOT, 'Docs', 'Images', 'icon-1080.png'), os.path.join(ROOT, 'Docs', 'Images', 'icon-512.png'),
             os.path.join(ROOT, 'Resources', 'Icon128.png')]
for path, size in zip(paths, (1080, 512, 128)):
    final.resize((size, size), Image.LANCZOS).save(path)
print('Wrote ' + ', '.join(paths))
