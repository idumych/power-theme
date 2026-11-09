#!/usr/bin/env python3
"""Make the QNX theme's wallpaper, data/themes/QNX/wallpaper.png.

    python3 -I tools/make-qnx-wallpaper.py

A lavender silk after Photon's default desktop in QNX 6.2.1: soft folds
running down from the top, darker towards the edges, and a fine diagonal
grain, every third pixel along a row a little darker. It is drawn here from
scratch, with nothing of QNX's: no logo, emblem or words. It is large enough
for big screens and is shown unscaled, from the top left corner, so that the
grain stays sharp.
"""

import math
import os

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "data", "themes", "QNX", "wallpaper.png")
WIDTH, HEIGHT = 2560, 1600

# the silk's middle tone, and how much the grain darkens
BASE = (0xba, 0xbc, 0xe2)
GRAIN = 0x17


def folds(x, y):
	"""The light and shade of the folds at x, y: about -1 to 1."""
	u = x / 1024.0
	v = y / 768.0
	# long folds falling from the top right towards the bottom left, bending
	# as silk does, each lit sharply along one side and shading off on the
	# other
	value = 0.0
	for k, (freq, slope, bend, phase, weight) in enumerate((
			(4.2, 1.1, 0.8, 0.3, 0.5), (7.5, 0.8, 1.4, 2.1, 0.3),
			(11.0, 1.3, 0.6, 4.0, 0.2))):
		t = freq * (u + slope * v) + bend * math.sin(2.1 * v + k) + phase
		wave = math.sin(t)
		value += weight * (wave + 0.35 * math.sin(2 * t + 0.6))
	return value


def edge_shade(x, y):
	"""Darker towards the top and the sides of a 1024 by 768 screen and beyond."""
	u = min(x, 1024) / 1024.0
	v = min(y, 768) / 768.0
	left = math.exp(-u * 18)
	top = math.exp(-v * 22)
	return 0.5 * left + 0.4 * top


def main():
	image = Image.new("RGB", (WIDTH, HEIGHT))
	pixels = image.load()
	for y in range(HEIGHT):
		for x in range(WIDTH):
			light = 24 * folds(x, y) - 34 * edge_shade(x, y)
			if (x - y) % 3 == 1:
				light -= GRAIN
			elif (x + 2 * y) % 7 == 0:
				light += 4
			pixels[x, y] = tuple(max(0, min(255, int(round(c + light))))
				for c in BASE)
	image.save(OUT, optimize=True)


if __name__ == "__main__":
	main()
