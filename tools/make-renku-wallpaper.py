#!/usr/bin/env python3
"""Draw the Renku theme's wallpaper after the Renku OS website's front page
(idumych.github.io/renkuos-website-draft): the night colour, a glow of fire
orange in the top right, an indigo haze in the bottom left, and 連句
("renku") written top to bottom in outline down the right side.

    python3 -I tools/make-renku-wallpaper.py <output.jpg> [width height]

Needs PIL and Noto Serif CJK (the outline is drawn with it; only the image
is kept, not the font).
"""

import glob
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont

NIGHT = (0x0C, 0x0D, 0x12)
FIRE = (0xFF, 0x6A, 0x13)
HAZE = (0x1B, 0x21, 0x40)
WASHI = (0xEF, 0xE9, 0xDD)


def glow(size, centre, radius, colour, strength, reach):
    """A radial glow, as CSS's radial-gradient(colour, transparent reach)."""
    w, h = size
    # drawn small and scaled up, for a smooth falloff quickly
    scale = 8
    small = Image.new("L", (w // scale, h // scale), 0)
    px = small.load()
    cx, cy = centre[0] / scale, centre[1] / scale
    rx, ry = radius[0] / scale, radius[1] / scale
    for y in range(small.height):
        for x in range(small.width):
            d = (((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2) ** 0.5
            t = max(0.0, 1 - d / reach)
            px[x, y] = int(255 * strength * t * t * (3 - 2 * t))
    mask = small.resize(size, Image.BICUBIC)
    return Image.new("RGB", size, colour), mask


def font(size):
    for pattern in ("/usr/share/fonts/**/NotoSerifCJK-Black.ttc",
            "/usr/share/fonts/**/NotoSerifCJK-Bold.ttc",
            "/usr/share/fonts/**/NotoSerifCJK*.ttc"):
        found = sorted(glob.glob(pattern, recursive=True))
        if found:
            return ImageFont.truetype(found[0], size, index=0)
    sys.exit("Noto Serif CJK not found")


def main():
    out = sys.argv[1]
    w, h = (int(sys.argv[2]), int(sys.argv[3])) if len(sys.argv) > 3 \
        else (1920, 1080)

    image = Image.new("RGB", (w, h), NIGHT)
    for centre, radius, colour, strength, reach in (
            ((w * 0.8, h * 0.1), (w * 0.62, h * 0.55), FIRE, 0.16, 0.6),
            ((0, h), (w * 0.47, h * 0.65), HAZE, 0.9, 0.65)):
        layer, mask = glow((w, h), centre, radius, colour, strength, reach)
        image.paste(layer, (0, 0), mask)

    # 連句, top to bottom, in a faint outline, half off the right edge
    size = int(h * 0.42)
    face = font(size)
    stroke = max(1, h // 540)
    outline = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(outline)
    fill = Image.new("L", (w, h), 0)
    fill_draw = ImageDraw.Draw(fill)
    x = w - int(size * 0.78)
    y = (h - 2 * size) // 2
    for i, char in enumerate("連句"):
        draw.text((x, y + i * size), char, font=face, fill=255,
            stroke_width=stroke, stroke_fill=255)
        fill_draw.text((x, y + i * size), char, font=face, fill=255)
    outline = ImageChops.subtract(outline, fill)
    image.paste(Image.new("RGB", (w, h), WASHI), (0, 0),
        outline.point(lambda v: v * 16 // 255))

    image.save(out, quality=92)


if __name__ == "__main__":
    main()
