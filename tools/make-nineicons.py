#!/usr/bin/env python3
"""Copy the icons Power Theme can use out of the NineIcons Redux icon theme
(store.kde.org/p/1960539, MIT licence) into data/icons/NineIconsRedux.

    python3 -I tools/make-nineicons.py <nineicons-redux folder>

Power Theme only changes the icons of folders and file types, the ones
data/PowerTheme/icon-map names. The rest of the theme, some 1400 more icons, is
mostly for applications and would never be used. Icons that are symbolic links in
the theme are copied as files.
"""

import configparser
import os
import shutil
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "data", "icons", "NineIconsRedux")
MAP = os.path.join(ROOT, "data", "PowerTheme", "icon-map")


def main():
	if len(sys.argv) != 2:
		sys.exit(__doc__.strip())
	theme = os.path.abspath(sys.argv[1])

	index = configparser.ConfigParser(interpolation=None, strict=False)
	index.optionxform = str
	index.read(os.path.join(theme, "index.theme"))
	folders = index["Icon Theme"]["Directories"].rstrip(",").split(",")

	wanted = set()
	with open(MAP) as f:
		for line in f:
			line = line.strip()
			if line.startswith("#") or "=" not in line:
				continue
			for name in line.split("=", 1)[1].split():
				if any(os.path.exists(os.path.join(theme, folder, name + ".png"))
						for folder in folders):
					wanted.add(name)
					break

	if os.path.exists(OUT):
		shutil.rmtree(OUT)
	os.makedirs(OUT)

	used = []
	count = 0
	for folder in folders:
		if folder.endswith("symbolic"):
			continue
		names = [name for name in sorted(wanted) if os.path.exists(
			os.path.join(theme, folder, name + ".png"))]
		if not names:
			continue
		os.makedirs(os.path.join(OUT, folder))
		for name in names:
			shutil.copyfile(os.path.join(theme, folder, name + ".png"),
				os.path.join(OUT, folder, name + ".png"))
			count += 1
		used.append(folder)

	with open(os.path.join(OUT, "index.theme"), "w") as f:
		f.write("[Icon Theme]\nName=NineIcons Redux\n"
			"Comment=Mac OS 9 style icons for folders and file types\n"
			"Directories=%s\n" % ",".join(used))
		for folder in used:
			f.write("\n[%s]\n" % folder)
			for key, value in index[folder].items():
				f.write("%s=%s\n" % (key, value))

	shutil.copyfile(os.path.join(theme, "LICENSE"), os.path.join(OUT, "LICENSE"))
	print("%d icons, %d names, %d folders" % (count, len(wanted), len(used)))


main()
