#!/usr/bin/env python3
"""Count the lines drawn across the tab of tools/left-tab-test.cpp's window.

A tab's fill is a gradient, so neighbouring columns are nearly the same colour.
Past the last stop of a gradient, app_server used to draw whatever was in
memory: stripes of unrelated colours along the tab.

    left-tab-check.py SHOT.png
"""
import sys

from PIL import Image

# Rows between the close button and the title, columns inside the tab's fill
# towards the window, where the stripes were. The window's content starts at 200.
ROWS = range(217, 229)
COLUMNS = range(188, 196)
MOST = 24

image = Image.open(sys.argv[1]).convert("RGB")
rows = 0
for y in ROWS:
	for x in COLUMNS:
		a = image.getpixel((x, y))
		b = image.getpixel((x + 1, y))
		if max(abs(p - q) for p, q in zip(a, b)) > MOST:
			rows += 1
			break
print(f"rows with lines across the left-titled tab: {rows}")
