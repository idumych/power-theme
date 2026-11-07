#!/bin/sh
# Assemble dist/power-theme-<version>.zip: install.sh / uninstall.sh,
# prebuilt files, the theme and icon theme, the full source (with the
# private headers needed to build on the target system), README, licence
# and a screenshot.
#
#   sh packaging/zip/make-zip.sh [<version>]
#
# Run after `make` (on Renku, or with build/ copied from there) and after
# tools/build-icon-theme.py has written build/icons/ClassicOS2000;
# build/BUILT_FOR names the system the prebuilt files were built for.
set -eu

version=${1:-0.6.1}
root=$(cd "$(dirname "$0")/../.." && pwd)
name=power-theme-$version
out=$root/dist/$name

for f in build/Reactionary build/control_look/Reactionary build/PowerTheme \
		build/FlatDecorator build/control_look/FlatControlLook \
		build/Smoke build/control_look/Smoke build/control_look/Gonx \
		build/data/PowerTheme/build-decorators.sh \
		build/icons/ClassicOS2000/index.theme; do
	[ -f "$root/$f" ] || { echo "missing $f" >&2; exit 1; }
done

rm -rf "$out" "$root/dist/$name.zip"
mkdir -p "$out/prebuilt/control_look" "$out/source" "$out/data/themes" \
	"$out/data/icons" "$out/data/PowerTheme"

cp "$root/packaging/zip/install.sh" "$root/packaging/zip/uninstall.sh" \
	"$root/packaging/zip/INSTALL.txt" "$out/"
cp "$root/README.md" "$root/LICENSE" "$out/"
cp "$root/docs/screenshots/power-theme.png" "$out/screenshot.png"

cp "$root/build/Reactionary" "$root/build/PowerTheme" "$root/build/FlatDecorator" \
	"$root/build/Smoke" "$out/prebuilt/"
cp "$root/build/control_look/Reactionary" \
	"$root/build/control_look/FlatControlLook" \
	"$root/build/control_look/Smoke" "$root/build/control_look/Gonx" \
	"$out/prebuilt/control_look/"
cp "$root/build/BUILT_FOR" "$out/prebuilt/BUILT_FOR"

cp -R "$root/data/themes/." "$out/data/themes/"
cp "$root/data/PowerTheme/icon-map" "$out/data/PowerTheme/"
cp -R "$root/build/data/PowerTheme/." "$out/data/PowerTheme/"
# keeps the icon theme's aliases as symbolic links
cp -R "$root/build/icons/ClassicOS2000" "$out/data/icons/"
cp -R "$root/data/icons/NineIconsRedux" "$out/data/icons/"

cp -R "$root/src" "$root/tools" "$root/packaging" "$root/build-headers" \
	"$root/data" "$root/Makefile" "$root/README.md" "$root/LICENSE" \
	"$out/source/"

chmod +x "$out/install.sh" "$out/uninstall.sh"

(cd "$root/dist" && python3 -I - "$name" <<'EOF'
import os, stat, sys, zipfile
name = sys.argv[1]
with zipfile.ZipFile(name + ".zip", "w", zipfile.ZIP_DEFLATED) as z:
	for top, dirs, files in os.walk(name):
		dirs.sort()
		for f in sorted(files + [d for d in dirs
				if os.path.islink(os.path.join(top, d))]):
			path = os.path.join(top, f)
			if os.path.islink(path):
				# stored as a link, as unzip restores them
				info = zipfile.ZipInfo(path)
				info.create_system = 3
				info.external_attr = (stat.S_IFLNK | 0o777) << 16
				z.writestr(info, os.readlink(path))
				continue
			info = zipfile.ZipInfo.from_file(path, path)
			with open(path, "rb") as data:
				z.writestr(info, data.read(), zipfile.ZIP_DEFLATED)
EOF
)
echo "$root/dist/$name.zip"
