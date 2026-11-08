#!/bin/sh
# Install Power Theme for this user, without a package, and switch it on:
#
#   - the Reactionary window decorator and control look, Smoke ones in the
#     style of ZETA's for Theme Manager themes that ask for them, and
#     Haiku's Flat ones for the Flat themes unless the system has them
#     (add-ons),
#   - Power Theme, the theme manager (an application, also the powertheme
#     command),
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

if pkgman search -i powertheme 2>/dev/null | awk '$2 == "powertheme"' \
		| grep -q .; then
	echo "The powertheme package is installed. Remove it first:" >&2
	echo "    powertheme --revert; pkgman uninstall powertheme" >&2
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
		echo "Building Power Theme against this system ($system)..."
		work=$(mktemp -d /tmp/powertheme-build.XXXXXX)
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
	"$data/themes" "$data/icons" "$data/PowerTheme"

# Power Theme was called Themes up to 0.2 (and its command
# reactionary-theme in 0.1); its settings folder moves when it first runs.
rm -f "$apps/PowerTheme" "$bin/Themes" "$deskbar/Themes"
rm -rf "$data/Themes"
if [ -x "$bin/reactionary-theme" ]; then
	"$bin/reactionary-theme" revert >/dev/null 2>&1 || true
	rm -f "$bin/reactionary-theme"
fi

# A decorator in use is mapped by app_server, and the control look and
# Power Theme by running applications: never overwrite them in place, put new
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
place "$files/PowerTheme" "$apps/PowerTheme"
# its type, signature and icon, from its resources
mimeset -f "$apps/PowerTheme" 2>/dev/null || true
ln -sf "$apps/PowerTheme" "$bin/powertheme"
if [ -d "$(dirname "$deskbar")" ]; then
	mkdir -p "$deskbar"
	ln -sf "$apps/PowerTheme" "$deskbar/Power Theme"
fi

# the themes (Reactionary ME, and Haiku's own to go back to), the icon
# theme (its aliases are symbolic links) and the map of which icons go
# where
for theme in "$here"/data/themes/*; do
	rm -rf "$data/themes/$(basename "$theme")"
	cp -R "$theme" "$data/themes/"
done
rm -rf "$data/icons/ClassicOS2000" "$data/icons/NineIconsRedux" \
	"$data/icons/elementary"
cp -R "$here/data/icons/ClassicOS2000" "$here/data/icons/NineIconsRedux" \
	"$here/data/icons/elementary" \
	"$data/icons/"
cp "$here/data/PowerTheme/icon-map" "$data/PowerTheme/icon-map"
# what Power Theme's "Build add-ons" needs: the script, the add-ons'
# source and the app_server headers
rm -rf "$data/PowerTheme/source"
cp -R "$here/data/PowerTheme/source" "$data/PowerTheme/"
cp "$here/data/PowerTheme/build-decorators.sh" "$data/PowerTheme/"

appServer() {
	ps | awk '$1 ~ /\/app_server$/ { print $(NF-3); exit }'
}

before=$(appServer)
"$apps/PowerTheme" --apply="Reactionary ME"
sleep 2
after=$(appServer)

if [ -z "$after" ] || [ "$before" != "$after" ]; then
	echo "app_server restarted after switching to Reactionary; switching" >&2
	echo "back." >&2
	"$apps/PowerTheme" --revert || setdecor Default
	exit 1
fi

"$apps/PowerTheme" --restart-tracker || true

cat <<EOF

Power Theme is installed in ~/config/non-packaged, with the Reactionary ME
theme switched on.
Restart running applications (or log out and in) to get the new controls
and fonts everywhere.

Power Theme (in Deskbar's Applications menu, or "powertheme" in a
Terminal) switches between themes and icon themes, and back:

    powertheme --revert    go back to your previous look and icons
    sh uninstall.sh        remove Power Theme

If the desktop ever fails to start after a system update: hold Space while
booting, open "Select safe mode options" and choose "Disable user add-ons";
then run sh uninstall.sh, or reinstall with sh install.sh --build.
EOF
