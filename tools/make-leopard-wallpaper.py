#!/usr/bin/env python3
"""Draw the Leopard themes' wallpaper: an aurora of soft rays fanning up from
low in the middle of the picture, pink at its heart and violet and blue towards
its sides, over a night sky with stars. It's in the spirit of Mac OS X 10.5's
default desktop picture, drawn from scratch here; nothing of Apple's is used.

    python3 -I tools/make-leopard-wallpaper.py [--graphite] <output.jpg> [width height]

--graphite draws Leopard Graphite's: the same rays in cool blues and steel.

Needs PIL. The same seed always gives the same picture.
"""

import math
import random
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter


def lerp(a, b, t):
	return tuple(int(round(x + (y - x) * t)) for x, y in zip(a, b))


PALETTES = {
	"blue": {
		"sky_top": (0x16, 0x0f, 0x33),
		"sky_bottom": (0x07, 0x05, 0x12),
		"haze": (0x1c, 0x3a, 0x6a),
		# from the middle of the fan out to its sides
		"rays": [(0xff, 0xb8, 0xec), (0xff, 0x52, 0xc6), (0xd4, 0x22, 0xa4),
			(0x84, 0x2c, 0xc4), (0x36, 0x42, 0xbc)],
		"core": (0xff, 0xc4, 0xee),
	},
	"graphite": {
		"sky_top": (0x0e, 0x12, 0x1c),
		"sky_bottom": (0x05, 0x06, 0x0a),
		"haze": (0x22, 0x33, 0x48),
		"rays": [(0xe8, 0xf0, 0xff), (0xa6, 0xbf, 0xe0), (0x70, 0x8c, 0xb4),
			(0x4c, 0x60, 0x86), (0x34, 0x40, 0x5c)],
		"core": (0xf4, 0xf8, 0xff),
	},
}


def ray_colour(palette, t):
	"""The colour of a ray t of the way from the fan's middle to its side."""
	stops = palette["rays"]
	t = max(0.0, min(0.999, t)) * (len(stops) - 1)
	i = int(t)
	return lerp(stops[i], stops[i + 1], t - i)


def sky(size, palette):
	w, h = size
	img = Image.new("RGB", (1, h))
	for y in range(h):
		img.putpixel((0, y), lerp(palette["sky_top"], palette["sky_bottom"],
			(y / (h - 1)) ** 0.8))
	img = img.resize(size)

	# a haze low on the left, as of distant light
	haze = Image.new("L", (w // 8, h // 8), 0)
	draw = ImageDraw.Draw(haze)
	draw.ellipse((-w // 40, h // 14, w // 20, h // 7), fill=120)
	haze = haze.resize(size, Image.BICUBIC).filter(
		ImageFilter.GaussianBlur(w // 10))
	img = Image.composite(Image.new("RGB", size, palette["haze"]), img, haze)
	return img


def rays(size, palette, rng):
	"""The fan, as light added to the sky."""
	w, h = size
	scale = 4
	sw, sh = w // scale, h // scale
	ox, oy = sw * 0.5, sh * 0.8
	light = Image.new("RGB", (sw, sh), (0, 0, 0))

	reach = math.hypot(sw, sh)
	for _ in range(150):
		# mostly upwards, thicker near the middle
		spread = rng.gauss(0, 0.42)
		angle = -math.pi / 2 + spread * math.pi / 2
		half = rng.uniform(0.004, 0.035)
		length = reach * rng.uniform(0.45, 1.0)
		start = rng.uniform(0.0, 0.12) * reach
		strength = rng.uniform(0.25, 1.0) * (1 - min(abs(spread), 1) * 0.55)
		colour = ray_colour(palette, abs(spread) * 1.1 + rng.uniform(-0.12, 0.12))

		layer = Image.new("L", (sw, sh), 0)
		draw = ImageDraw.Draw(layer)
		near = (ox + math.cos(angle) * start, oy + math.sin(angle) * start)
		far1 = (ox + math.cos(angle - half) * length,
			oy + math.sin(angle - half) * length)
		far2 = (ox + math.cos(angle + half) * length,
			oy + math.sin(angle + half) * length)
		draw.polygon([near, far1, far2], fill=int(150 * strength))
		layer = layer.filter(ImageFilter.GaussianBlur(rng.uniform(2, 7)))

		tinted = Image.new("RGB", (sw, sh), colour)
		light = ImageChops.screen(light, ImageChops.multiply(tinted,
			Image.merge("RGB", (layer, layer, layer))))

	# brightest a little above where the rays start, fading out with distance
	fade = Image.new("L", (sw, sh), 0)
	px = fade.load()
	for y in range(sh):
		for x in range(sw):
			d = math.hypot(x - ox, (y - oy) * 1.3) / reach
			v = math.exp(-((d - 0.1) / 0.42) ** 2) if d > 0.1 else 1.0
			px[x, y] = int(255 * v)
	light = ImageChops.multiply(light, Image.merge("RGB", (fade, fade, fade)))

	# the heart of it, a soft glow where the rays meet
	core = Image.new("L", (sw, sh), 0)
	draw = ImageDraw.Draw(core)
	r = sw * 0.035
	draw.ellipse((ox - r * 2.2, oy - r * 0.9, ox + r * 2.2, oy + r * 0.3),
		fill=130)
	core = core.filter(ImageFilter.GaussianBlur(sw * 0.025))
	light = ImageChops.add(light, ImageChops.multiply(
		Image.new("RGB", (sw, sh), palette["core"]),
		Image.merge("RGB", (core, core, core))))

	light = light.resize(size, Image.BICUBIC).filter(ImageFilter.GaussianBlur(3))
	return light


def stars(img, rng):
	w, h = img.size
	draw = ImageDraw.Draw(img)
	for _ in range(int(w * h / 6000)):
		x, y = rng.uniform(0, w), rng.uniform(0, h)
		b = rng.random() ** 3
		level = int(110 + 145 * b)
		r = 0.4 + b * 1.1
		old = img.getpixel((int(x), int(y)))
		colour = tuple(max(o, level - rng.randint(0, 30) * (i == 0))
			for i, o in enumerate(old))
		draw.ellipse((x - r, y - r, x + r, y + r), fill=colour)
	return img


def main():
	args = [a for a in sys.argv[1:] if not a.startswith("--")]
	if not args:
		sys.exit(__doc__)
	palette = PALETTES["graphite" if "--graphite" in sys.argv else "blue"]
	out = args[0]
	w, h = (int(args[1]), int(args[2])) if len(args) >= 3 else (1920, 1200)

	rng = random.Random(1005)
	img = sky((w, h), palette)
	img = ImageChops.add(img, rays((w, h), palette, rng))
	img = stars(img, rng)
	img.save(out, quality=90, optimize=True, progressive=True)


if __name__ == "__main__":
	main()
