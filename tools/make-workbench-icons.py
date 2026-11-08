#!/usr/bin/env python3
"""Make the Workbench icon theme, data/icons/Workbench, for the Amiga theme.

    python3 -I tools/make-workbench-icons.py

The icons are Workbench 1.3's as Arthur Choung's HOTDOG draws them, kept as
text in src/third_party/amiga/icons (GPL 3 or later): a drawer for folders, an
open one for the home folder, the RAM disk for the computer, a disk for disks,
the Shell for programs, a text file and the trashcan. Each is scaled to Power
Theme's sizes without smoothing, so that it stays pixelated, and kept in its
proportions in the middle of the square. Everything else is NineIcons Redux's.
"""

import os
import shutil
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
ART = os.path.join(ROOT, "src", "third_party", "amiga", "icons")
OUT = os.path.join(ROOT, "data", "icons", "Workbench")
SIZES = (16, 24, 32, 48, 64)

# each picture, and the context and names it is used for
ICONS = (
	("drawer", "places", ("folder", "inode-directory")),
	("open-drawer", "places", ("user-home", "folder-home")),
	("ram-disk", "devices", ("computer", "user-desktop")),
	("disk", "devices", ("drive-harddisk", "harddrive")),
	("shell", "mimes", ("application-x-executable",)),
	("text-file", "mimes", ("text-x-generic", "text-plain")),
	("trashcan", "places", ("user-trash",)),
)
CONTEXTS = {"places": "Places", "devices": "Devices", "mimes": "MimeTypes"}


def read_art(name):
	palette = {}
	rows = []
	for line in open(os.path.join(ART, name + ".txt")):
		line = line.rstrip("\n")
		if line.startswith("= "):
			_, char, colour = line.split(" ", 2)
			palette[char] = colour.lstrip("#")
		elif line.startswith("|"):
			rows.append(line[1:-1])
	width = max(len(r) for r in rows)
	image = Image.new("RGBA", (width, len(rows)), (0, 0, 0, 0))
	for y, row in enumerate(rows):
		for x, char in enumerate(row):
			if char in palette:
				c = palette[char]
				image.putpixel((x, y), (int(c[0:2], 16), int(c[2:4], 16),
					int(c[4:6], 16), 255))
	return image


def fit(image, size):
	scale = min(size / image.width, size / image.height)
	w = max(1, round(image.width * scale))
	h = max(1, round(image.height * scale))
	scaled = image.resize((w, h), Image.NEAREST)
	out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
	out.paste(scaled, ((size - w) // 2, (size - h) // 2))
	return out


def main():
	shutil.rmtree(OUT, ignore_errors=True)
	directories = []
	for art, context, names in ICONS:
		image = read_art(art)
		for size in SIZES:
			folder = "%s/%d" % (context, size)
			if folder not in directories:
				directories.append(folder)
			os.makedirs(os.path.join(OUT, folder), exist_ok=True)
			icon = fit(image, size)
			for name in names:
				icon.save(os.path.join(OUT, folder, name + ".png"))

	directories.sort(key=lambda d: (d.split("/")[0], int(d.split("/")[1])))
	with open(os.path.join(OUT, "index.theme"), "w") as f:
		f.write("[Icon Theme]\nName=Workbench\n")
		f.write("Comment=Workbench 1.3's drawer, disk, RAM disk, Shell, text file "
			"and trashcan, after HOTDOG; the rest from NineIcons Redux\n")
		f.write("Inherits=NineIconsRedux\n")
		f.write("Directories=" + ",".join(directories) + "\n")
		for directory in directories:
			context, size = directory.split("/")
			f.write("\n[%s]\nContext=%s\nSize=%s\nType=Fixed\n"
				% (directory, CONTEXTS[context], size))
	shutil.copy(os.path.join(ROOT, "src", "third_party", "amiga", "LICENSE"),
		os.path.join(OUT, "LICENSE"))


if __name__ == "__main__":
	main()
