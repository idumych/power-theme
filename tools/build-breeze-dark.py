#!/usr/bin/env python3
"""Build the copy of Breeze Dark that Power Theme ships.

    python3 -I tools/build-breeze-dark.py <icons folder> <licence folder> <output dir>

<icons folder> holds an installed breeze-icons' breeze and breeze-dark themes,
such as /usr/share/icons. <licence folder> holds its COPYING-ICONS and
COPYING.LIB. Only the icons data/PowerTheme/icon-map names are kept, at the plain
sizes, with no high-resolution folders. Breeze Dark takes the icons it doesn't
have from Breeze, so those are copied in, and the copy inherits nothing. The
icons themselves aren't changed.
"""

import os
import re
import shutil
import sys

THEME_DIR = "breeze-dark"


def icon_names(icon_map):
    names = set()
    with open(icon_map) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                names.update(line.split("=", 1)[1].split())
    return names


def sections(index_theme):
    """index.theme's sections, as name -> their lines"""
    result = {}
    current = None
    with open(index_theme, encoding="utf-8") as f:
        for line in f:
            match = re.match(r"\[(.+)\]\s*$", line)
            if match:
                current = result.setdefault(match.group(1), [])
            elif current is not None and line.strip() \
                    and not line.startswith("#"):
                current.append(line.strip())
    return result


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__.strip())
    icons, licences = sys.argv[1], sys.argv[2]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(os.path.abspath(sys.argv[3]), THEME_DIR)
    names = icon_names(os.path.join(root, "data/PowerTheme/icon-map"))

    index = sections(os.path.join(icons, THEME_DIR, "index.theme"))
    head = index["Icon Theme"]
    directories = [d for d in next(l for l in head
        if l.startswith("Directories=")).split("=", 1)[1].split(",")
        if d and "@" not in d]

    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)

    kept = []
    count = 0
    for directory in directories:
        found = False
        # Breeze Dark's own first, then Breeze's
        for theme in (THEME_DIR, "breeze"):
            folder = os.path.join(icons, theme, directory)
            if not os.path.isdir(folder):
                continue
            for entry in sorted(os.listdir(folder)):
                stem, extension = os.path.splitext(entry)
                target = os.path.join(out, directory, entry)
                if stem not in names or extension not in (".svg", ".png") \
                        or os.path.exists(target):
                    continue
                os.makedirs(os.path.dirname(target), exist_ok=True)
                shutil.copyfile(os.path.join(folder, entry), target)
                count += 1
                found = True
        if found:
            kept.append(directory)

    def first(key):
        return next(l for l in head if l.startswith(key + "=")).split("=", 1)[1]

    with open(os.path.join(out, "index.theme"), "w") as f:
        f.write("[Icon Theme]\nName=%s\nComment=%s\nDirectories=%s\n"
            % (first("Name"), first("Comment"), ",".join(kept)))
        for directory in kept:
            f.write("\n[%s]\n%s\n" % (directory, "\n".join(index[directory])))

    for name in ("COPYING-ICONS", "COPYING.LIB"):
        shutil.copy(os.path.join(licences, name), out)
    print("%s: %d icons in %d folders" % (out, count, len(kept)))


if __name__ == "__main__":
    main()
