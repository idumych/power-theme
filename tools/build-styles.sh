#!/bin/sh
# Build Power Theme's window decorators once, as decorator styles, for every
# Renku whose app_server runs styles (DecorStyle.h), rather than for one
# system build.
#
# Each decorator's own source is built against the shim in src/styles/shim,
# which stands in for app_server's private classes on top of the stable
# style interface. The result has the decorator's usual name, so themes find
# it where they always have, and it needs no AddOnGuard: an app_server
# without the engine or with an older interface refuses it, and keeps its
# default decorator.
#
#   sh tools/build-styles.sh [--dest=DIR] [NAME...]
#
#   --dest  where to put them, in "decorators" (default: build/prebuilt)
#   NAME    only these decorators (default: all of them)
#
# Runs on Renku or Haiku with the compiler (pkgman: gcc, haiku_devel); the
# interface comes with the source, in src/styles/include.

set -u

fail() { echo "ERROR $1"; exit 1; }

here=$(cd "$(dirname "$0")/.." && pwd)
dest=$here/build/prebuilt
names=""
for arg in "$@"; do
	case $arg in
		--dest=*) dest=${arg#--dest=} ;;
		--*) fail "unknown option $arg" ;;
		*) names="$names $arg" ;;
	esac
done

shim=$here/src/styles/shim
flags="-O2 -Wall -Wno-multichar -Wno-unused-variable -fPIC -std=gnu++17
	-I$shim -I$here/src/styles/include
	-Dinstantiate_decor_addon=shim_instantiate_decor_addon"
build=$(mktemp -d "${TMPDIR:-/tmp}/styles.XXXXXX") || fail "no temporary folder"
mkdir -p "$dest/decorators" || fail "can't create $dest/decorators"

# the same list as build-decorators.sh's
decorators=$(sed -n 's/^\(addons="\)\{0,1\}\(decorators:[^"]*\)"\{0,1\}$/\2/p' \
	"$here/src/themes/build-decorators.sh")

for entry in $decorators; do
	name=$(echo "$entry" | cut -d: -f2)
	if [ -n "$names" ]; then
		case " $names " in *" $name "*) ;; *) continue ;; esac
	fi
	folder=$here/$(echo "$entry" | cut -d: -f4)
	sources=$(echo "$entry" | cut -d: -f5)
	resources=$(echo "$entry" | cut -d: -f6)

	# The shim's TabDecorator is Haiku's, for decorators built on it.
	files=""
	for file in "$folder"/$sources; do
		files="$files $file"
	done
	tab=""
	if grep -q "TabDecorator" $files; then
		tab=$shim/TabDecorator.cpp
	fi

	output=$dest/decorators/$name
	echo "g++ $name"
	# shellcheck disable=SC2086
	g++ $flags -DSHIM_STYLE_NAME="\"$name\"" -I"$folder" -shared -s \
		-o "$output" $files $tab "$shim/StyleShim.cpp" -lbe -lstdc++ 2>&1 \
		|| fail "$name: doesn't build"
	for rdef in "$folder"/$resources; do
		[ -f "$rdef" ] || continue
		rc -o "$build/$name.rsrc" "$rdef" 2>&1 || fail "$name: rc failed"
		xres -o "$output" "$build/$name.rsrc" 2>&1 \
			|| fail "$name: xres failed"
	done
	mimeset -f "$output" 2>/dev/null
done

rm -rf "$build"
echo "Built in $dest/decorators"
