#!/usr/bin/env python3
"""Build a PNG copy of the ClassicOS 2000 icon theme for Haiku.

    python3 -I tools/build-icon-theme.py <ClassicOS-2000-Icons checkout> <output dir>

The theme is drawn as SVG at 16, 20, 24, 32 and 48 pixels, and a few icons
use blur filters, which the SVG renderer in Themes (nanosvg) can't do. So it
is rendered here, with librsvg's rsvg-convert, into a freedesktop icon theme
of PNG files: <context>/<size>/<name>.png, with every alias the theme's
lists name as a relative symlink to the first, and an index.theme.

The theme's licence allows redistribution; its LICENSE file is copied along.
"""

import os
import shutil
import subprocess
import sys

SIZES = (16, 20, 24, 32, 48)
THEME_DIR = "ClassicOS2000"


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip())
    checkout = os.path.abspath(sys.argv[1])
    out = os.path.join(os.path.abspath(sys.argv[2]), THEME_DIR)
    src = os.path.join(checkout, "src")
    if not os.path.isfile(os.path.join(src, "index.theme")):
        sys.exit("%s: not a ClassicOS-2000-Icons checkout" % checkout)

    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)

    directories = set()
    count = 0
    for size in SIZES:
        lists = os.path.join(src, "list%d" % size)
        for name in sorted(os.listdir(lists)):
            svg = os.path.join(src, "svg%d" % size, name + ".svg")
            with open(os.path.join(lists, name)) as f:
                targets = [line.strip() for line in f if line.strip()]
            if not targets or not os.path.isfile(svg):
                continue

            first = None
            for target in targets:
                context, icon = target.split("/", 1)
                directory = "%s/%d" % (context, size)
                directories.add(directory)
                path = os.path.join(out, directory, icon + ".png")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                if os.path.lexists(path):
                    continue
                if first is None:
                    subprocess.run(["rsvg-convert", "-w", str(size), "-h",
                        str(size), "-o", path, svg], check=True)
                    first = path
                    count += 1
                else:
                    os.symlink(os.path.relpath(first, os.path.dirname(path)),
                        path)

    write_index(src, out, sorted(directories,
        key=lambda d: (d.split("/")[0], int(d.split("/")[1]))))
    shutil.copy(os.path.join(checkout, "LICENSE"), out)
    print("%s: %d icons in %d folders" % (out, count, len(directories)))


def write_index(src, out, directories):
    """The theme's name and comment, with a fixed-size PNG folder list."""
    name, comment = "ClassicOS 2000", ""
    with open(os.path.join(src, "index.theme")) as f:
        for line in f:
            if line.startswith("Name="):
                name = line[5:].strip()
            elif line.startswith("Comment="):
                comment = line[8:].strip()
            elif line.startswith("["):
                if line.strip() != "[Icon Theme]":
                    break

    with open(os.path.join(out, "index.theme"), "w") as f:
        f.write("[Icon Theme]\nName=%s\nComment=%s\n" % (name, comment))
        f.write("Directories=%s\n" % ",".join(directories))
        for directory in directories:
            context, size = directory.split("/")
            f.write("\n[%s]\nSize=%s\nContext=%s\nType=Fixed\n"
                % (directory, size, context.capitalize()))


if __name__ == "__main__":
    main()
