#!/bin/sh
# Build powertheme-<version>-1-x86_64.hpkg on a Haiku or Renku system.
#
#   sh packaging/hpkg/build-hpkg.sh <build-dir> <icons-dir> [<version>]
#
# <build-dir> holds PowerTheme and data/PowerTheme (make build/PowerTheme
# decorator-source); <icons-dir> the ClassicOS 2000 icon theme
# (tools/build-icon-theme.py, on a machine with Python and librsvg).
#
# The package has no window decorators or control looks: they use private
# classes of app_server and libbe, so they only fit the system build they
# were compiled for. It has their source and the build script instead, and
# Power Theme's "Build add-ons..." compiles them on the system it runs on, into
# the user's non-packaged add-ons. So the package itself fits any system
# from the one it was built on.
set -eu

build=${1:?usage: build-hpkg.sh <build-dir> <icons-dir> [<version>]}
icons=${2:?usage: build-hpkg.sh <build-dir> <icons-dir> [<version>]}
version=${3:-0.9}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)

haiku=$(pkgman search -i -D haiku 2>/dev/null \
	| awk '$2 == "haiku" { print $3; exit }')
haiku=${haiku%-*}	# drop the package revision
[ -n "$haiku" ] || { echo "can't tell the installed haiku version" >&2; exit 1; }

for f in "$build/PowerTheme" "$build/data/PowerTheme/build-decorators.sh" \
		"$icons/index.theme"; do
	[ -e "$f" ] || { echo "missing $f" >&2; exit 1; }
done

work=$(mktemp -d "${TMPDIR:-/tmp}/powertheme-hpkg.XXXXXX")
trap 'rm -rf "$work"' EXIT

mkdir -p "$work/apps" "$work/bin" "$work/data/themes" "$work/data/icons" \
	"$work/data/fonts/ttfonts" \
	"$work/data/deskbar/menu/Applications" \
	"$work/documentation/packages/powertheme"
cp "$build/PowerTheme" "$work/apps/PowerTheme"
ln -s ../apps/PowerTheme "$work/bin/powertheme"
ln -s ../../../../apps/PowerTheme \
	"$work/data/deskbar/menu/Applications/Power Theme"
cp -R "$root/data/themes/." "$work/data/themes/"
# keeps the icon theme's aliases as symbolic links
cp -R "$icons" "$work/data/icons/ClassicOS2000"
cp -R "$root/data/icons/NineIconsRedux" "$root/data/icons/elementary" \
	"$root/data/icons/NeXT" "$root/data/icons/Workbench" \
	"$work/data/icons/"
# the Amiga theme's pixel font, Topaz as HOTDOG draws it
cp "$root"/data/fonts/*.ttf "$work/data/fonts/ttfonts/"
cp -R "$build/data/PowerTheme" "$work/data/PowerTheme"
cp "$root/data/PowerTheme/icon-map" "$work/data/PowerTheme/icon-map"
cp "$root/README.md" "$root/LICENSE" "$root/packaging/zip/INSTALL.txt" \
	"$work/documentation/packages/powertheme/"

cat > "$work/.PackageInfo" <<INFO
name			powertheme
version			$version-1
architecture	x86_64
summary			"Power Theme: a theme manager with window borders, controls and icon themes"
description		"Power Theme switches the desktop between themes, and back: window \
borders, controls, colours, fonts, wallpaper, the Deskbar's place and \
freedesktop icon themes. It reads Theme Manager themes too. It comes with \
the Reactionary ME (Windows ME), Smoke (in the style of ZETA), Renku, \
Gonx, Haiku, Dano, Tidy, Expose, Platinum (in the style of Mac OS 9), Leopard \
(in the style of Mac OS X 10.5), Gershwix (after Mintar's Platinum theme for \
Mac OS X, in seven colours), NeXT (after Swizcore Studio's NeXTSTEP theme \
for Mac OS X), System 7 (in the style of Mac OS 7.5), Amiga (Workbench 1.3, after \
HOTDOG, with its Topaz font and icons), AmigaOS 3 (Workbench 3.x) and four \
Flat themes, and the ClassicOS 2000, \
NineIcons Redux, elementary, NeXT and Workbench icons. The window \
borders and controls are built on your own system by Power Theme's \
'Build add-ons...' (they use private parts of app_server and libbe, so they \
must match the exact system build); it offers to install the compiler."
packager		"idumych <idumych@users.noreply.github.com>"
vendor			"idumych"
copyrights		"2026 idumych"
licenses {
	"MIT"
	"GNU GPL v3"
}
urls			"https://github.com/idumych/power-theme"
provides {
	powertheme = $version
	app:PowerTheme = $version
	cmd:powertheme = $version
}
requires {
	haiku >= $haiku
	lib:libstdc++
}
INFO

out="$PWD/powertheme-$version-1-x86_64.hpkg"
(cd "$work" && package create -C . "$out")
echo "$out (for haiku $haiku and later)"
