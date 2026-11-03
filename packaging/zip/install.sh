#!/bin/sh
# Install Reactionary for this user, without a package, and switch it on:
#
#   - the Reactionary window decorator and control look, Smoke ones in the
#     style of ZETA's for Theme Manager themes that ask for them, and
#     Haiku's Flat ones for the Flat themes unless the system has them
#     (add-ons),
#   - Themes, the theme manager (an application, also a command),
#   - the themes (Reactionary ME, Haiku, Gonx, and the Flat themes from
#     haiku_darkstyle) and the ClassicOS 2000 icon theme (data),
#
# all in ~/config/non-packaged.
#
#   sh install.sh             build against this system if a compiler is
#                             installed, otherwise use the prebuilt files
#   sh install.sh --build     always build here (needs gcc, make, haiku_devel)
#   sh install.sh --prebuilt  always use the prebuilt files
#
# The decorator runs inside app_server and uses its private classes, so it
# is safest built on the system it runs on; this script does that whenever
# it can. sh uninstall.sh removes everything again.
set -e

here=$(cd "$(dirname "$0")" && pwd)

mode=auto
case "${1:-}" in
	"") ;;
	--build) mode=build ;;
	--prebuilt) mode=prebuilt ;;
	*) echo "usage: sh install.sh [--build | --prebuilt]" >&2; exit 2 ;;
esac

if [ "$(uname -s)" != Haiku ]; then
	echo "This installs on Renku OS or Haiku." >&2
	exit 1
fi

if pkgman search -i reactionary 2>/dev/null | awk '$2 == "reactionary"' \
		| grep -q .; then
	echo "The reactionary package is installed. Remove it first:" >&2
	echo "    Themes --revert; pkgman uninstall reactionary" >&2
	exit 1
fi

system=$(pkgman search -i -D haiku 2>/dev/null \
	| awk '$2 == "haiku" { print $3; exit }')
system=${system%-*}
builtFor=$(cat "$here/prebuilt/BUILT_FOR" 2>/dev/null || echo unknown)

canBuild() {
	command -v gcc >/dev/null 2>&1 && command -v make >/dev/null 2>&1 \
		&& command -v rc >/dev/null 2>&1 && command -v xres >/dev/null 2>&1 \
		&& [ -f /boot/system/develop/headers/os/interface/View.h ] \
		&& [ -f /boot/system/develop/lib/libbe.so ]
}

if [ $mode = auto ]; then
	if canBuild; then mode=build; else mode=prebuilt; fi
fi

case $mode in
	build)
		if ! canBuild; then
			echo "Building needs a compiler and the system headers:" >&2
			echo "    pkgman install gcc make haiku_devel" >&2
			exit 1
		fi
		echo "Building Reactionary against this system ($system)..."
		work=$(mktemp -d /tmp/reactionary-build.XXXXXX)
		cp -R "$here/source/." "$work/"
		(cd "$work" && make clean >/dev/null && make >"$work/build.log" 2>&1) \
			|| { echo "The build failed; see $work/build.log" >&2; exit 1; }
		files=$work/build
		;;
	prebuilt)
		files=$here/prebuilt
		if [ "$system" != "$builtFor" ]; then
			echo "Note: the prebuilt files were made on $builtFor; this system"
			echo "is ${system:-unknown}. The interfaces they use haven't changed"
			echo "since Haiku R1/beta6, but building here is safer:"
			echo "    pkgman install gcc make haiku_devel && sh install.sh --build"
		fi
		;;
esac

nonpackaged=$(finddir B_USER_NONPACKAGED_DIRECTORY)
addons=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY)
bin=$(finddir B_USER_NONPACKAGED_BIN_DIRECTORY)
data=$(finddir B_USER_NONPACKAGED_DATA_DIRECTORY)
apps=$nonpackaged/apps
deskbar=$(finddir B_USER_SETTINGS_DIRECTORY)/deskbar/menu/Applications
mkdir -p "$addons/decorators" "$addons/control_look" "$bin" "$apps" \
	"$data/themes" "$data/icons" "$data/Themes"

# Version 0.1 came with a reactionary-theme command instead of Themes.
if [ -x "$bin/reactionary-theme" ]; then
	"$bin/reactionary-theme" revert >/dev/null 2>&1 || true
	rm -f "$bin/reactionary-theme"
fi

# A decorator in use is mapped by app_server, and the control look and
# Themes by running applications: never overwrite them in place, put new
# files in place by renaming.
place() {
	cp "$1" "$2.new"
	mv -f "$2.new" "$2"
}

if setdecor -l 2>/dev/null | grep -q '^\*Reactionary'; then
	setdecor Default >/dev/null
fi
place "$files/Reactionary" "$addons/decorators/Reactionary"
place "$files/control_look/Reactionary" "$addons/control_look/Reactionary"
if setdecor -l 2>/dev/null | grep -q '^\*Smoke'; then
	setdecor Default >/dev/null
fi
place "$files/Smoke" "$addons/decorators/Smoke"
place "$files/control_look/Smoke" "$addons/control_look/Smoke"
# Haiku's Flat decorator and control look come in its haiku_extras
# package; use those if they are there.
systemAddons=$(finddir B_SYSTEM_ADDONS_DIRECTORY)
if [ ! -e "$systemAddons/decorators/FlatDecorator" ]; then
	if setdecor -l 2>/dev/null | grep -q '^\*FlatDecorator'; then
		setdecor Default >/dev/null
	fi
	place "$files/FlatDecorator" "$addons/decorators/FlatDecorator"
fi
if [ ! -e "$systemAddons/control_look/FlatControlLook" ]; then
	place "$files/control_look/FlatControlLook" \
		"$addons/control_look/FlatControlLook"
fi
# Gonx's control look, for the Gonx theme, unless Gonx is installed
if [ ! -e "$addons/control_look/Gonx" ] \
	&& [ ! -e "$systemAddons/control_look/Gonx" ]; then
	place "$files/control_look/Gonx" "$addons/control_look/Gonx"
fi
place "$files/Themes" "$apps/Themes"
# its type, signature and icon, from its resources
mimeset -f "$apps/Themes" 2>/dev/null || true
ln -sf "$apps/Themes" "$bin/Themes"
if [ -d "$(dirname "$deskbar")" ]; then
	mkdir -p "$deskbar"
	ln -sf "$apps/Themes" "$deskbar/Themes"
fi

# the themes (Reactionary ME, and Haiku's own to go back to), the icon
# theme (its aliases are symbolic links) and the map of which icons go
# where
for theme in "$here"/data/themes/*; do
	rm -rf "$data/themes/$(basename "$theme")"
	cp -R "$theme" "$data/themes/"
done
rm -rf "$data/icons/ClassicOS2000"
cp -R "$here/data/icons/ClassicOS2000" "$data/icons/"
cp "$here/data/Themes/icon-map" "$data/Themes/icon-map"
# what Themes' "Build add-ons" needs: the script, the add-ons'
# source and the app_server headers
rm -rf "$data/Themes/source"
cp -R "$here/data/Themes/source" "$data/Themes/"
cp "$here/data/Themes/build-decorators.sh" "$data/Themes/"

appServer() {
	ps | awk '$1 ~ /\/app_server$/ { print $(NF-3); exit }'
}

before=$(appServer)
"$apps/Themes" --apply="Reactionary ME"
sleep 2
after=$(appServer)

if [ -z "$after" ] || [ "$before" != "$after" ]; then
	echo "app_server restarted after switching to Reactionary; switching" >&2
	echo "back." >&2
	"$apps/Themes" --revert || setdecor Default
	exit 1
fi

"$apps/Themes" --restart-tracker || true

cat <<EOF

Reactionary is installed in ~/config/non-packaged and switched on.
Restart running applications (or log out and in) to get the new controls
and fonts everywhere.

Themes (in Deskbar's Applications menu, or "Themes" in a Terminal)
switches between themes and icon themes, and back:

    Themes --revert        go back to your previous look and icons
    sh uninstall.sh        remove Reactionary

If the desktop ever fails to start after a system update: hold Space while
booting, open "Select safe mode options" and choose "Disable user add-ons";
then run sh uninstall.sh, or reinstall with sh install.sh --build.
EOF
