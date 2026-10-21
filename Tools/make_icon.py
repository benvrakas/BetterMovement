"""Draws the Better Movement icon: a Pioneer sliding up a foundation ramp out of a lake, another gliding in on a
parachute, Grass Fields sky, arch and planet behind. Drawn at 2048 px and scaled down.

  python Tools/make_icon.py

Writes Docs/Images/icon-1080.png (full resolution), Docs/Images/icon-512.png (mod page) and Resources/Icon128.png
(shown in the game's Mods menu).
"""
import math
import os
from PIL import Image, ImageDraw, ImageFilter

S = 2048
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUIT = (214, 198, 164)
SUIT_SHADE = (168, 152, 120)
HARNESS = (38, 38, 44)
HELMET = (238, 122, 38)
HELMET_HI = (255, 196, 140)
VISOR = (14, 18, 30)
VISOR_HI = (90, 150, 230)
BOOT = (96, 58, 30)
GLOVE = (60, 44, 32)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(len(a)))


def gradient(size, top, bottom):
    w, h = size
    img = Image.new('RGBA', size)
    d = ImageDraw.Draw(img)
    for y in range(h):
        d.line([(0, y), (w, y)], fill=lerp(top, bottom, y / max(1, h - 1)))
    return img


img = gradient((S, S), (34, 96, 196, 255), (160, 205, 240, 255))
d = ImageDraw.Draw(img)

# Planet low in the sky, as on the Grass Fields horizon.
planet = Image.new('RGBA', (S, S), (0, 0, 0, 0))
pd = ImageDraw.Draw(planet)
pd.ellipse([1500, 140, 1960, 600], fill=(196, 214, 236, 120))
pd.ellipse([1570, 180, 1960, 570], fill=(150, 175, 210, 60))
img.alpha_composite(planet.filter(ImageFilter.GaussianBlur(4)))

# Clouds.
clouds = Image.new('RGBA', (S, S), (0, 0, 0, 0))
cd = ImageDraw.Draw(clouds)
for cx, cy, w, h in [(380, 230, 420, 110), (560, 190, 300, 120), (1200, 420, 520, 120), (1380, 380, 300, 110),
                     (260, 760, 360, 90), (1700, 760, 420, 90)]:
    cd.ellipse([cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], fill=(255, 255, 255, 200))
img.alpha_composite(clouds.filter(ImageFilter.GaussianBlur(26)))

HORIZON = 1240

# Stone arch on the horizon: two tapering pillars under a thick curved span.
arch = Image.new('RGBA', (S, S), (0, 0, 0, 0))
ad = ImageDraw.Draw(arch)
STONE = (96, 106, 124, 255)
STONE_DARK = (74, 82, 98, 255)
ad.polygon([(170, HORIZON + 40), (205, 860), (335, 860), (360, HORIZON + 40)], fill=STONE)
ad.polygon([(560, HORIZON + 40), (585, 860), (715, 860), (740, HORIZON + 40)], fill=STONE_DARK)
ad.arc([200, 600, 720, 1180], 180, 360, fill=STONE, width=130)
ad.arc([200, 600, 720, 1180], 280, 360, fill=STONE_DARK, width=130)
img.alpha_composite(arch)

# Distant tree line and hills.
d = ImageDraw.Draw(img)
d.polygon([(0, HORIZON - 40), (300, HORIZON - 90), (560, HORIZON - 30), (900, HORIZON - 70), (1300, HORIZON - 20),
           (1700, HORIZON - 80), (S, HORIZON - 40), (S, S), (0, S)], fill=(70, 124, 104, 255))
for x in range(-40, S + 80, 70):
    h = 90 + (x * 37 % 70)
    d.ellipse([x - 40, HORIZON - 40 - h, x + 40, HORIZON + 10], fill=(52, 102, 84, 255))
# Meadow.
meadow = gradient((S, S - HORIZON), (98, 168, 72, 255), (54, 112, 40, 255))
img.alpha_composite(meadow, (0, HORIZON))

# Lake in the foreground left.
d = ImageDraw.Draw(img)
lake = [(0, 1380), (360, 1350), (700, 1420), (880, 1600), (800, S), (0, S)]
lake_mask = Image.new('L', (S, S), 0)
ImageDraw.Draw(lake_mask).polygon(lake, fill=255)
img.paste(gradient((S, S), (70, 150, 226, 255), (18, 64, 132, 255)), (0, 0), lake_mask)
d = ImageDraw.Draw(img)
for row, (y0, amp) in enumerate([(1470, 14), (1590, 12), (1720, 10), (1860, 9)]):
    pts = [(x, y0 + amp * math.sin(x / 55.0 + row * 1.7)) for x in range(0, 820 - row * 30, 8)]
    d.line(pts, fill=(220, 240, 255, 170), width=12)

# Foundation ramp: rises from the lake shore to the top right.
FOOT = (700, 1640)
TOP = (S + 40, 900)
ux, uy = (TOP[0] - FOOT[0]), (TOP[1] - FOOT[1])
ln = math.hypot(ux, uy)
ux, uy = ux / ln, uy / ln
nx, ny = uy, -ux
if ny > 0:
    nx, ny = -nx, -ny

side = [FOOT, TOP, (S + 40, S + 40), (FOOT[0] - 120, S + 40)]
d.polygon(side, fill=(40, 42, 48, 255))
# Ribs on the foundation sides.
for x in range(FOOT[0] - 100, S + 40, 34):
    t = (x - FOOT[0]) / (TOP[0] - FOOT[0])
    y_top = FOOT[1] + (TOP[1] - FOOT[1]) * max(0.0, t) + 40
    d.line([(x, y_top), (x, S)], fill=(56, 59, 66, 255), width=8)
# Foundation blocks: frames and diagonal braces.
blocks = 4
for i in range(blocks + 1):
    t = i / blocks
    x = FOOT[0] + (TOP[0] - FOOT[0]) * t
    y = FOOT[1] + (TOP[1] - FOOT[1]) * t
    d.line([(x, y + 20), (x - 40 * (1 - t), S)], fill=(20, 21, 25, 255), width=22)
    if i < blocks:
        t2 = (i + 1) / blocks
        x2 = FOOT[0] + (TOP[0] - FOOT[0]) * t2
        y2 = FOOT[1] + (TOP[1] - FOOT[1]) * t2
        d.line([(x, y + 40), (x2, S - 40)], fill=(24, 25, 30, 255), width=18)
# Top frame: light grey rim with the inset tile edge.
d.line([FOOT, TOP], fill=(196, 202, 210, 255), width=44)
d.line([(FOOT[0] + nx * -26, FOOT[1] + ny * -26), (TOP[0] + nx * -26, TOP[1] + ny * -26)], fill=(120, 126, 134, 255), width=10)
d.line([(FOOT[0] + nx * 14, FOOT[1] + ny * 14), (TOP[0] + nx * 14, TOP[1] + ny * 14)], fill=(236, 240, 244, 255), width=8)


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
    # White trim stripe over the top.
    draw.arc([c[0] - r * 0.92, c[1] - r * 0.92, c[0] + r * 0.92, c[1] + r * 0.92], 200, 330, fill=(245, 245, 240), width=int(r * 0.14))
    vx = c[0] + facing[0] * r * 0.32
    vy = c[1] + facing[1] * r * 0.32
    draw.ellipse([vx - r * 0.62, vy - r * 0.5, vx + r * 0.62, vy + r * 0.5], fill=VISOR)
    draw.ellipse([vx - r * 0.42, vy - r * 0.34, vx - r * 0.05, vy - r * 0.12], fill=VISOR_HI)
    draw.ellipse([c[0] - r * 0.7, c[1] - r * 0.85, c[0] - r * 0.25, c[1] - r * 0.55], fill=HELMET_HI)


def pioneer_sliding(layer, origin, scale):
    # Local frame: x up the slope, y away from the ramp surface. Feet first, sitting low, one hand trailing.
    u = (ux, uy)
    n = (nx, ny)
    dr = ImageDraw.Draw(layer)
    p = lambda x, y: P(x, y, origin, scale, u, n)
    w = 0.16 * scale
    hip, neck = p(0.0, 0.26), p(-0.14, 0.88)
    # Far side in shade: tucked leg and the hand trailing on the ramp.
    limb(dr, [hip, p(0.34, 0.56), p(0.2, 0.04)], w, SUIT_SHADE)
    limb(dr, [p(0.16, 0.02), p(0.34, 0.0)], w * 1.05, BOOT)
    limb(dr, [p(-0.1, 0.8), p(-0.36, 0.46), p(-0.52, 0.06)], w * 0.8, SUIT_SHADE)
    limb(dr, [p(-0.52, 0.06), p(-0.6, 0.0)], w * 0.85, GLOVE)
    # Backpack against the back.
    dr.polygon([p(-0.2, 0.32), p(-0.42, 0.42), p(-0.46, 0.88), p(-0.24, 0.92)], fill=HARNESS)
    # Torso with harness, belt and the green chest reflector.
    limb(dr, [hip, neck], w * 1.9, SUIT)
    limb(dr, [p(-0.1, 0.34), p(0.12, 0.32)], w * 0.38, HARNESS)
    dr.line([p(-0.16, 0.82), p(0.06, 0.42)], fill=HARNESS, width=int(w * 0.3))
    dr.line([p(-0.04, 0.68), p(-0.2, 0.64)], fill=(150, 220, 70), width=int(w * 0.24))
    # Near leg, straight out up the slope, knee pad and boot.
    limb(dr, [hip, p(0.5, 0.14), p(0.98, 0.02)], w * 1.05, SUIT)
    kp = p(0.5, 0.15)
    dr.ellipse([kp[0] - w * 0.55, kp[1] - w * 0.55, kp[0] + w * 0.55, kp[1] + w * 0.55], fill=HARNESS)
    limb(dr, [p(0.96, 0.0), p(1.14, 0.0), p(1.14, 0.16)], w * 1.15, BOOT)
    # Near arm forward for balance.
    limb(dr, [p(-0.08, 0.8), p(0.2, 0.88), p(0.44, 1.0)], w * 0.85, SUIT)
    limb(dr, [p(0.42, 1.0), p(0.52, 1.04)], w * 0.9, GLOVE)
    helmet(dr, p(-0.18, 1.12), 0.26 * scale, (ux * 0.7 + nx * 0.2, uy * 0.7 + ny * 0.2))


def pioneer_hanging(layer, origin, scale):
    # Upright: x right, y up (screen y down).
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
    dr.line([p(-0.16, 0.06), p(0.16, 0.06)], fill=HARNESS, width=int(w * 0.4))
    limb(dr, [hip, p(0.16, -0.38), p(0.12, -0.8)], w * 1.05, SUIT)
    limb(dr, [p(0.12, -0.8), p(0.26, -0.86)], w * 1.15, BOOT)
    limb(dr, [p(0.16, 0.58), p(0.34, 0.86), p(0.3, 1.12)], w * 0.85, SUIT)
    helmet(dr, p(0.0, 0.86), 0.25 * scale, (0.45, 0.15))
    return p(-0.3, 1.14), p(0.3, 1.14)


def parachute(layer, center, width, height):
    dr = ImageDraw.Draw(layer)
    cells = 7
    top_y = center[1] - height / 2
    pts_top, pts_bot = [], []
    for i in range(cells * 4 + 1):
        a = math.pi + math.pi * i / (cells * 4)
        x = center[0] + math.cos(a) * width / 2
        pts_top.append((x, center[1] + math.sin(a) * height * 0.9 - height * 0.05))
        pts_bot.append((x, center[1] + math.sin(a) * height * 0.55 + height * 0.25))
    dr.polygon(pts_top + pts_bot[::-1], fill=(232, 236, 240, 255))
    for c in range(1, cells):
        i = c * 4
        dr.line([pts_top[i], pts_bot[i]], fill=(168, 176, 186, 255), width=10)
    for c in range(cells):
        i = c * 4 + 2
        mid_t = pts_top[i]
        dr.line([(mid_t[0], mid_t[1] + 6), (pts_bot[i][0], pts_bot[i][1] - 6)], fill=(250, 252, 255, 255), width=6)
    dr.line(pts_bot, fill=(150, 158, 170, 255), width=8)
    dr.line(pts_top, fill=(250, 252, 255, 255), width=6)
    return pts_bot


people = Image.new('RGBA', (S, S), (0, 0, 0, 0))

# Speed streaks behind the slider, then the slider.
t = 0.42
base = (FOOT[0] + (TOP[0] - FOOT[0]) * t, FOOT[1] + (TOP[1] - FOOT[1]) * t)
origin = (base[0] + nx * 12, base[1] + ny * 12)
streaks = Image.new('RGBA', (S, S), (0, 0, 0, 0))
sd = ImageDraw.Draw(streaks)
for off, length in [(0.3, 520), (0.65, 640), (1.0, 460)]:
    start = (origin[0] - ux * 170 + nx * off * 250, origin[1] - uy * 170 + ny * off * 250)
    steps = 30
    for s in range(steps):
        a0 = s / steps
        p0 = (start[0] - ux * length * a0, start[1] - uy * length * a0)
        p1 = (start[0] - ux * length * (a0 + 1 / steps), start[1] - uy * length * (a0 + 1 / steps))
        sd.line([p0, p1], fill=(255, 255, 255, int(210 * (1 - a0) ** 1.6)), width=16)
img.alpha_composite(streaks)
# Spray kicked up where the slide left the lake.
d = ImageDraw.Draw(img)
for dx, dy, r in [(-60, -70, 22), (-140, -150, 16), (-20, -170, 12), (-200, -60, 18), (-110, -250, 9), (-250, -170, 11)]:
    d.ellipse([FOOT[0] + dx - r, FOOT[1] + dy - r, FOOT[0] + dx + r, FOOT[1] + dy + r], fill=(225, 242, 255, 235))

pioneer_sliding(people, origin, 250)

# Parachuter in the upper left.
chute_center = (1020, 260)
bot = parachute(people, chute_center, 560, 200)
hang_origin = (1050, 700)
lh, rh = pioneer_hanging(people, hang_origin, 200)
pdraw = ImageDraw.Draw(people)
for i in (0, 6, 12, 16, 22, 28):
    target = lh if i < 14 else rh
    pdraw.line([bot[i], target], fill=(40, 44, 52, 255), width=5)

shadow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
shadow.paste((0, 0, 0, 110), (0, 0), people.split()[3])
img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(14)), (14, 18))
img.alpha_composite(people)

final = img.convert('RGB')
final.resize((1080, 1080), Image.LANCZOS).save(os.path.join(ROOT, 'Docs', 'Images', 'icon-1080.png'))
final.resize((512, 512), Image.LANCZOS).save(os.path.join(ROOT, 'Docs', 'Images', 'icon-512.png'))
final.resize((128, 128), Image.LANCZOS).save(os.path.join(ROOT, 'Resources', 'Icon128.png'))
print('Wrote Docs/Images/icon-1080.png, Docs/Images/icon-512.png and Resources/Icon128.png')
