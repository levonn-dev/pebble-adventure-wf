"""
gen_logo.py - Generate the logo banner and icon for Fox Weather.

Retro pixel style (adapted from the Pebble Adventure logo generator):
  - Press Start 2P bitmap font
  - Gold (#FFD700) title with thick black pixel stroke
  - Title floats over the plains-fox scene (no box)

Outputs:
  docs/logo.png       - 800x340 README header banner
  docs/icon.png       - 200x200 icon

Assets used (from this repo):
  resources/images/backgrounds/color/bg_plains.png
  resources/images/sprites/fox/large/fox_lg_idle_0.png

Usage: python3 tools/gen_logo.py
"""

from PIL import Image, ImageDraw, ImageFont
import os

ROOT     = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS     = os.path.join(ROOT, 'docs')
RES      = os.path.join(ROOT, 'resources', 'images')

BG_PATH  = os.path.join(RES, 'backgrounds', 'color', 'bg_plains.png')
FOX_PATH = os.path.join(RES, 'sprites', 'fox', 'large', 'fox_lg_idle_0.png')

FONT_PATH = '/tmp/PressStart2P.ttf'
FALLBACK_FONTS = [
    '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
    '/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf',
]

TITLE_LINES = ['FOX', 'WEATHER']   # widest line drives the auto-size
SUBTITLE    = 'Rain or shine.'

# Colors
BLACK = (  0,   0,   0, 255)
WHITE = (255, 255, 255, 255)
GOLD  = (255, 215,   0, 255)
CREAM = (255, 240, 200, 255)


def load_font(size):
    for p in [FONT_PATH] + FALLBACK_FONTS:
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def px_scale(img, factor):
    return img.resize((img.width * factor, img.height * factor), Image.NEAREST)


def text_width(font, text):
    bb = font.getbbox(text)
    return bb[2] - bb[0]


def draw_outlined_text(draw, pos, text, font, fill, stroke_color=BLACK, stroke_w=3):
    """Thick pixel-art stroke via offset compositing."""
    x, y = pos
    for dx in range(-stroke_w, stroke_w + 1):
        for dy in range(-stroke_w, stroke_w + 1):
            if dx == 0 and dy == 0:
                continue
            draw.text((x + dx, y + dy), text, font=font, fill=stroke_color)
    draw.text((x, y), text, font=font, fill=fill)


# --- Load assets ---
bg_orig  = Image.open(BG_PATH).convert('RGBA')   # 200x150
fox_orig = Image.open(FOX_PATH).convert('RGBA')  # 85x80


# --- BANNER (720x320) ---
BANNER_W, BANNER_H = 720, 320
BG_SCALE, FOX_SCALE = 4, 3

bg4  = px_scale(bg_orig, BG_SCALE)    # 800x600
fox3 = px_scale(fox_orig, FOX_SCALE)  # 255x240

GROUND_Y_IN_BG4 = int(105 / 150 * bg4.height)  # ~420
GROUND_Y_CROP   = int(BANNER_H * 0.80)          # 272
CROP_TOP        = GROUND_Y_IN_BG4 - GROUND_Y_CROP

bg_crop = bg4.crop((0, CROP_TOP, BANNER_W, CROP_TOP + BANNER_H))
banner  = bg_crop.copy()

# Fox - right side, feet on ground line
fox_x = BANNER_W - fox3.width - 14
fox_y = GROUND_Y_CROP - fox3.height
banner.paste(fox3, (fox_x, fox_y), fox3)

draw = ImageDraw.Draw(banner)

# Auto-size title to fit the left zone (up to fox_x - margin)
TEXT_MAX_W = fox_x - 30
widest = max(TITLE_LINES, key=len)
title_size = 52
for size in range(52, 20, -2):
    if text_width(load_font(size), widest) <= TEXT_MAX_W:
        title_size = size
        break

font_title = load_font(title_size)
font_sub   = load_font(max(9, title_size // 5))

title_x      = 22
cap_h        = font_title.getbbox('A')[3]
line_gap     = 14
stride       = cap_h + line_gap
total_text_h = len(TITLE_LINES) * cap_h + (len(TITLE_LINES) - 1) * line_gap
title_y      = (GROUND_Y_CROP - total_text_h) // 2 - 14

for i, line in enumerate(TITLE_LINES):
    draw_outlined_text(draw, (title_x, title_y + i * stride),
                       line, font_title, GOLD, BLACK, stroke_w=4)

sub_y = title_y + (len(TITLE_LINES) - 1) * stride + cap_h + 16
draw_outlined_text(draw, (title_x + 2, sub_y),
                   SUBTITLE, font_sub, CREAM, BLACK, stroke_w=2)

os.makedirs(DOCS, exist_ok=True)
banner_path = os.path.join(DOCS, 'logo.png')
banner.save(banner_path)
print(f'logo.png  -> {banner_path}  ({banner.width}x{banner.height})')


# --- ICON (200x200) ---
ICON_SIZE   = 200
GROUND_LINE = 176
SKY_TOP = (100, 162, 220)
SKY_MID = (148, 200, 238)

icon = Image.new('RGBA', (ICON_SIZE, ICON_SIZE), SKY_TOP + (255,))
draw_icon = ImageDraw.Draw(icon)
for y in range(GROUND_LINE):
    t = y / GROUND_LINE
    r = int(SKY_TOP[0] + (SKY_MID[0] - SKY_TOP[0]) * t)
    g = int(SKY_TOP[1] + (SKY_MID[1] - SKY_TOP[1]) * t)
    b = int(SKY_TOP[2] + (SKY_MID[2] - SKY_TOP[2]) * t)
    draw_icon.line([(0, y), (ICON_SIZE - 1, y)], fill=(r, g, b, 255))

draw_icon.rectangle([0, GROUND_LINE,     ICON_SIZE-1, GROUND_LINE + 5], fill=(130, 210, 80, 255))
draw_icon.rectangle([0, GROUND_LINE + 5, ICON_SIZE-1, ICON_SIZE - 1],   fill=( 80, 160, 55, 255))

fox2  = px_scale(fox_orig, 2)
fox_ix = (ICON_SIZE - fox2.width) // 2
fox_iy = GROUND_LINE - fox2.height
icon.paste(fox2, (fox_ix, fox_iy), fox2)

DARK_GOLD = (184, 134, 11, 255)
draw_icon = ImageDraw.Draw(icon)
draw_icon.rectangle([0, 0, ICON_SIZE-1, ICON_SIZE-1], outline=BLACK[:3], width=3)
draw_icon.rectangle([3, 3, ICON_SIZE-4, ICON_SIZE-4], outline=GOLD[:3],      width=1)
draw_icon.rectangle([4, 4, ICON_SIZE-5, ICON_SIZE-5], outline=DARK_GOLD[:3], width=1)

icon_path = os.path.join(DOCS, 'icon.png')
icon.save(icon_path)
print(f'icon.png  -> {icon_path}  ({icon.width}x{icon.height})')
