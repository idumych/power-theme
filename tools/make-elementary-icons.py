#!/usr/bin/env python3
"""Render the icons Power Theme can use out of the elementary icon theme, as it
was at version 4.3.1 (2017), into data/icons/elementary, for the Leopard themes.

    git clone --depth 1 --branch 4.3.1 https://github.com/elementary/icons.git
    python3 -I tools/make-elementary-icons.py <that checkout>

elementary's icons are by Daniel Foré and the elementary contributors, under the
GNU GPL, version 3 or later (its COPYING and AUTHORS files are copied along).
Version 4.3.1 is the last with its blue-grey folders, which go well with Leopard.

Power Theme only changes the icons of folders and file types, the ones
data/PowerTheme/icon-map names, so only those are kept. The theme is SVG, and
many of its icons have blurred shadows, which the SVG renderer in Power Theme
(nanosvg) can't draw; so they are rendered here with librsvg's rsvg-convert, each
size from its own design, into PNG files: <context>/<size>/<name>.png.
"""

import os
import shutil
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "data", "icons", "elementary")
MAP = os.path.join(ROOT, "data", "PowerTheme", "icon-map")

SIZES = (16, 24, 32, 48)
CONTEXTS = {
	"places": "Places",
	"mimes": "MimeTypes",
	"devices": "Devices",
	"apps": "Applications",
	"actions": "Actions",
	"categories": "Categories",
	"status": "Status",
	"emblems": "Emblems",
}


def wanted_names():
	names = []
	with open(MAP) as f:
		for line in f:
			line = line.strip()
			if line.startswith("#") or "=" not in line:
				continue
			names.extend(line.split("=", 1)[1].split())
	return set(names)


def main():
	if len(sys.argv) != 2:
		sys.exit(__doc__.strip())
	theme = os.path.abspath(sys.argv[1])
	if not os.path.isfile(os.path.join(theme, "index.theme.in")):
		sys.exit("%s: not a checkout of elementary's icons" % theme)

	wanted = wanted_names()

	if os.path.exists(OUT):
		shutil.rmtree(OUT)
	os.makedirs(OUT)

	used = []
	names = set()
	count = 0
	for context in sorted(CONTEXTS):
		for size in SIZES:
			source = os.path.join(theme, context, str(size))
			if not os.path.isdir(source):
				continue
			found = sorted(name[:-4] for name in os.listdir(source)
				if name.endswith(".svg") and name[:-4] in wanted)
			if not found:
				continue
			folder = "%s/%d" % (context, size)
			os.makedirs(os.path.join(OUT, folder))
			for name in found:
				svg = os.path.realpath(os.path.join(source, name + ".svg"))
				png = os.path.join(OUT, folder, name + ".png")
				subprocess.run(["rsvg-convert", "-w", str(size), "-h", str(size),
					"-o", png, svg], check=True)
				names.add(name)
				count += 1
			used.append(folder)

	with open(os.path.join(OUT, "index.theme"), "w") as f:
		f.write("[Icon Theme]\nName=elementary (4.3.1)\n"
			"Comment=elementary's icons for folders and file types, from 2017\n"
			"Directories=%s\n" % ",".join(used))
		for folder in used:
			context, size = folder.split("/")
			f.write("\n[%s]\nContext=%s\nSize=%s\nType=Fixed\n"
				% (folder, CONTEXTS[context], size))

	for name in ("COPYING", "AUTHORS"):
		shutil.copyfile(os.path.join(theme, name), os.path.join(OUT, name))
	print("%d icons, %d names, %d folders" % (count, len(names), len(used)))


main()
