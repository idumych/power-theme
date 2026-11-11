#!/bin/sh
# Build Power Theme's control looks once, for every Renku that has
# create_default_control_look() in libbe, rather than for one system build.
#
# Each look is built on ForwardingControlLook (src/controllook-kit) with
# PT_PREBUILT_LOOK: it subclasses the public BControlLook and hands what it
# doesn't draw itself to libbe's default look, so it uses no private headers
# and no private class layouts, and needs no AddOnGuard. A system without
# the function refuses to load it (a missing symbol), and libbe falls back
# to its default look.
#
#   sh tools/build-prebuilt-looks.sh [--dest=DIR] [--headers=DIR] [--libs=DIR]
#       [NAME...]
#
#   --dest     where to put them, in "control_look" (default: build/prebuilt)
#   --headers  a folder with a newer ControlLook.h than the system's
#   --libs     a folder with a libbe.so that has create_default_control_look,
#              when the system's doesn't
#   NAME       only these looks (default: all of them)
#
# Runs on Renku with the compiler (pkgman: gcc, haiku_devel); the system's
# ControlLook.h must declare create_default_control_look.

set -u

fail() { echo "ERROR $1"; exit 1; }

here=$(cd "$(dirname "$0")/.." && pwd)
dest=$here/build/prebuilt
names=""
headers=/boot/system/develop/headers/os/interface
extra=""
libs=""
for arg in "$@"; do
	case $arg in
		--dest=*) dest=${arg#--dest=} ;;
		--headers=*) headers=${arg#--headers=}; extra="-I$headers" ;;
		--libs=*) libs="-L${arg#--libs=}" ;;
		--*) fail "unknown option $arg" ;;
		*) names="$names $arg" ;;
	esac
done

grep -q create_default_control_look "$headers/ControlLook.h" 2>/dev/null \
	|| fail "this system's ControlLook.h has no create_default_control_look"

kit=$here/src/controllook-kit
flags="-O2 -Wall -Wno-multichar -Wno-unused-variable -fPIC -std=gnu++17
	-DPT_PREBUILT_LOOK $extra -I$kit"
build=$(mktemp -d "${TMPDIR:-/tmp}/prebuilt-looks.XXXXXX") \
	|| fail "no temporary folder"
mkdir -p "$dest/control_look" || fail "can't create $dest/control_look"

# the same list as build-decorators.sh's
looks=$(grep '^control_look:' "$here/src/themes/build-decorators.sh" \
	| sed 's/"$//')

g++ $flags -c "$kit/ForwardingControlLook.cpp" -o "$build/kit.o" 2>&1 \
	|| fail "ForwardingControlLook doesn't compile"

for entry in $looks; do
	name=$(echo "$entry" | cut -d: -f2)
	if [ -n "$names" ]; then
		case " $names " in *" $name "*) ;; *) continue ;; esac
	fi
	folder=$here/$(echo "$entry" | cut -d: -f4)
	sources=$(echo "$entry" | cut -d: -f5)
	resources=$(echo "$entry" | cut -d: -f6)
	objects=$build/kit.o
	for file in "$folder"/$sources; do
		object=$build/$name-$(basename "$file" .cpp).o
		echo "g++ -c $(basename "$file")"
		# shellcheck disable=SC2086
		g++ $flags -I"$folder" -c "$file" -o "$object" 2>&1 \
			|| fail "$name: doesn't compile"
		objects="$objects $object"
	done
	output=$dest/control_look/$name
	# -Bsymbolic: each look calls its own copy of the kit, even with another
	# look loaded in the same team; --no-undefined: and never libbe's private
	# HaikuControlLook in place of something it lacks
	# shellcheck disable=SC2086
	g++ -shared -s -Wl,-Bsymbolic -Wl,--no-undefined -o "$output" $objects \
		$libs -lbe -lstdc++ 2>&1 \
		|| fail "$name: doesn't link"
	for rdef in "$folder"/$resources; do
		rc -o "$build/$name.rsrc" "$rdef" 2>&1 || fail "$name: rc failed"
		xres -o "$output" "$build/$name.rsrc" 2>&1 \
			|| fail "$name: xres failed"
	done
	echo "built $output"
done
rm -rf "$build"
