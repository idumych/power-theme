#!/bin/sh
# Build Themes' window decorators (Reactionary, Smoke, Gonx, and Haiku's
# Flat decorator if the system lacks it) on, and for, the running system.
#
# A decorator runs inside app_server and uses its private classes, so it
# must be compiled against the headers of the exact system build it runs
# on and linked against that system's app_server. This finds the system's
# revision (from its haiku package, e.g. hrev60072+339), takes the bundled
# app_server headers if they are from that revision, and otherwise fetches
# them from the system's source repository (RenkuOS/Source, else
# haiku/haiku) at that revision, with git: a commits-only fetch back to the
# hrev tag, then the commit as many commits past it as the revision says,
# then just the header files. They are kept in ~/config/settings/Themes
# (~/config/cache is read-only on Renku).
#
#   sh build-decorators.sh [--install-tools] [--headers=auto|bundled|fetch]
#                          [--source=DIR] [--dest=DIR]
#
#   --install-tools  install the compiler first (pkgman: gcc, haiku_devel)
#   --headers        auto (default): bundled if they match, else fetched,
#                    else bundled with a warning
#   --source         the decorators' source: Themes' "source" folder, or a
#                    Reactionary source tree (default: next to this script)
#   --dest           where to put them (default: the user's non-packaged
#                    add-ons/decorators)
#
# Output, for Themes' build window: "STEP <percent> <what>" lines for the
# progress bar, "ERROR <what>" for failures, everything else is the log.
# Exit status: 0 built, 1 failed, 3 the compiler isn't installed.

set -u

say() { echo "STEP $1 $2"; }
fail() { echo "ERROR $1"; exit 1; }

here=$(cd "$(dirname "$0")" && pwd)
source=""
dest=""
headers_mode=auto
install_tools=false
for arg in "$@"; do
	case $arg in
		--install-tools) install_tools=true ;;
		--headers=*) headers_mode=${arg#--headers=} ;;
		--source=*) source=${arg#--source=} ;;
		--dest=*) dest=${arg#--dest=} ;;
		*) fail "unknown option $arg" ;;
	esac
done

if [ -z "$source" ]; then
	if [ -d "$here/source" ]; then
		source=$here/source
	elif [ -d "$here/../../src/decorator" ]; then
		source=$(cd "$here/../.." && pwd)
	else
		fail "the decorators' source isn't installed next to $0"
	fi
fi
# only decorators installed where app_server finds them are switched to
reload=false
if [ -z "$dest" ]; then
	dest=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY)/decorators
	reload=true
fi
cache=$(finddir B_USER_SETTINGS_DIRECTORY)/Themes

# the decorators: name, folder in an installed source, folder in the tree
decorators="Reactionary:decorators/Reactionary:src/decorator
Smoke:decorators/Smoke:src/smoke
Gonx:decorators/Gonx:src/third_party/gonx/decorator
FlatDecorator:decorators/FlatDecorator:src/third_party/haiku-flat/decorator"


# 1. the tools

say 2 "Checking for the compiler"
if $install_tools; then
	say 3 "Installing the compiler (gcc, haiku_devel)"
	pkgman install -y gcc haiku_devel 2>&1 | grep -v '^\s*$' | tail -20
fi
for tool in g++ rc xres; do
	if ! command -v $tool >/dev/null 2>&1; then
		echo "NEED_TOOLS gcc haiku_devel"
		echo "ERROR $tool isn't installed"
		exit 3
	fi
done
g++ --version | head -1


# 2. the system's revision

say 8 "Finding the system's revision"
package=$(ls /boot/system/packages/haiku-r*.hpkg 2>/dev/null | head -1)
[ -n "$package" ] || fail "no haiku package in /boot/system/packages"
hrev=$(basename "$package" | sed -n 's/.*_hrev\([0-9][0-9]*\).*/\1/p')
ahead=$(basename "$package" | sed -n 's/.*_hrev[0-9][0-9]*_\([0-9][0-9]*\)-.*/\1/p')
[ -n "$hrev" ] || fail "can't read the revision from $(basename "$package")"
revision=hrev$hrev${ahead:++$ahead}
echo "System: $(basename "$package" .hpkg), revision $revision"


# 3. the headers

bundled=$source/build-headers
bundledRevision=$(cat "$bundled/REVISION" 2>/dev/null || echo unknown)
headers=""

fetch_headers() {
	target=$cache/headers/$revision
	if [ -f "$target/.complete" ]; then
		echo "Using the headers fetched before, in $target"
		headers=$target
		return 0
	fi
	command -v git >/dev/null 2>&1 || { echo "git isn't installed"; return 1; }

	work=$(mktemp -d "${TMPDIR:-/tmp}/themes-headers.XXXXXX") || return 1
	paths=$(cd "$bundled/renku" && find . -type f | sed 's|^\./||')
	for repo in RenkuOS/Source haiku/haiku; do
		url=https://github.com/$repo.git
		say 18 "Looking for hrev$hrev in $repo"
		git ls-remote --tags "$url" "refs/tags/hrev$hrev" 2>/dev/null \
			| grep -q . || continue
		rm -rf "$work/git"
		git init -q "$work/git" || return 1
		git -C "$work/git" remote add origin "$url"
		commit=""
		if [ -z "$ahead" ]; then
			say 22 "Fetching hrev$hrev from $repo"
			git -C "$work/git" fetch -q --depth=1 --filter=tree:0 origin \
				"refs/tags/hrev$hrev:refs/tags/hrev$hrev" && commit=hrev$hrev
		else
			branch=$(git ls-remote --symref "$url" HEAD 2>/dev/null \
				| sed -n 's|^ref: refs/heads/\([^[:space:]]*\).*|\1|p')
			say 22 "Fetching $repo's $branch since hrev$hrev"
			git -C "$work/git" fetch -q --filter=tree:0 \
				--shallow-exclude="hrev$hrev" origin "$branch" || continue
			# as git describe counts: the commit with $ahead commits that
			# the tag doesn't have
			for c in $(git -C "$work/git" rev-list --first-parent FETCH_HEAD); do
				n=$(git -C "$work/git" rev-list --count "$c")
				if [ "$n" -le "$ahead" ]; then
					[ "$n" -eq "$ahead" ] && commit=$c
					break
				fi
			done
		fi
		if [ -z "$commit" ]; then
			echo "$revision isn't on $repo's main branch"
			continue
		fi
		echo "$revision is $repo commit $(git -C "$work/git" rev-parse --short "$commit")"
		say 30 "Fetching the headers"
		# shellcheck disable=SC2086
		if git -C "$work/git" -c advice.detachedHead=false checkout -q \
				"$commit" -- $paths; then
			rm -rf "$target"
			if mkdir -p "$target/renku" \
				&& (cd "$work/git" && tar cf - $paths) \
					| tar xf - -C "$target/renku" \
				&& echo "$revision" > "$target/REVISION" \
				&& touch "$target/.complete"; then
				rm -rf "$work"
				headers=$target
				return 0
			fi
			echo "can't keep the headers in $target"
			rm -rf "$target" "$work"
			return 1
		fi
		echo "$repo at $revision lacks some of the headers"
	done
	rm -rf "$work"
	return 1
}

say 12 "Finding app_server's headers for $revision"
case $headers_mode in
	bundled)
		headers=$bundled
		;;
	fetch|auto)
		if [ "$headers_mode" = auto ] && [ "$bundledRevision" = "$revision" ]; then
			echo "The bundled headers are from $revision"
			headers=$bundled
		elif ! fetch_headers; then
			[ "$headers_mode" = fetch ] && fail "couldn't fetch the headers for $revision"
			echo "WARNING: couldn't fetch the headers for $revision; using the"
			echo "bundled ones, from $bundledRevision. If app_server and the"
			echo "decorators don't agree, switching to them will fail safely."
			headers=$bundled
		fi
		;;
	*) fail "--headers must be auto, bundled or fetch" ;;
esac
echo "Headers: $headers"

freetype=/boot/system/develop/headers/freetype2
[ -f "$freetype/ft2build.h" ] || freetype=$bundled/freetype2


# 4. compile

app=$headers/renku/src/servers/app
private=$headers/renku/headers/private
flags="-O2 -Wall -Wno-multichar -fPIC -std=gnu++17
	-iquote $app -iquote $app/stackandtile -iquote $app/decorator
	-iquote $app/drawing -iquote $app/drawing/Painter -iquote $app/font
	-I$private -I$private/app -I$private/shared -I$private/interface
	-I$private/graphics -I$headers/renku/headers/libs/agg -I$freetype"

build=$(mktemp -d "${TMPDIR:-/tmp}/themes-build.XXXXXX") || fail "no temporary folder"
trap 'rm -rf "$build"' EXIT

todo=""
for entry in $decorators; do
	name=${entry%%:*}
	if [ "$name" = FlatDecorator ] \
		&& [ -e /boot/system/add-ons/decorators/FlatDecorator ]; then
		echo "FlatDecorator: the system has it already"
		continue
	fi
	todo="$todo $name"
done
count=$(echo $todo | wc -w)

index=0
built=""
for name in $todo; do
	entry=$(echo "$decorators" | grep "^$name:")
	folder=$source/$(echo "$entry" | cut -d: -f2)
	[ -d "$folder" ] || folder=$source/$(echo "$entry" | cut -d: -f3)
	[ -d "$folder" ] || fail "$name: no source in $source"

	percent=$((40 + index * 50 / count))
	say $percent "Compiling $name"
	objects=""
	for file in "$folder"/*Decorator.cpp; do
		object=$build/$name-$(basename "$file" .cpp).o
		echo "g++ -c $(basename "$file")"
		# shellcheck disable=SC2086
		g++ $flags -c "$file" -o "$object" 2>&1 || fail "$name doesn't compile"
		objects="$objects $object"
	done
	say $((percent + 25 / count)) "Linking $name"
	# shellcheck disable=SC2086
	g++ -shared -s -o "$build/$name" $objects -lbe \
		/boot/system/servers/app_server -lstdc++ 2>&1 \
		|| fail "$name doesn't link"
	rc -o "$build/$name.rsrc" "$folder/resources.rdef" 2>&1 \
		|| fail "$name: rc failed"
	xres -o "$build/$name" "$build/$name.rsrc" 2>&1 \
		|| fail "$name: xres failed"
	built="$built $name"
	index=$((index + 1))
done


# 5. install

say 92 "Installing"
mkdir -p "$dest" || fail "can't create $dest"
app_server_team() {
	ps | awk '$1 == "/boot/system/servers/app_server" { print $2; exit }'
}
active=$(setdecor -l 2>/dev/null | sed -n 's/^\*\([^ ]*\).*/\1/p')
for name in $built; do
	# app_server maps a decorator in use: never overwrite it in place
	cp "$build/$name" "$dest/$name.new" && mv -f "$dest/$name.new" "$dest/$name" \
		|| fail "can't install $name in $dest"
	echo "Installed $dest/$name"
	if $reload && [ "$name" = "$active" ]; then
		# load the new one, and check app_server takes it
		say 96 "Switching to the new $name"
		team=$(app_server_team)
		setdecor Default >/dev/null 2>&1
		setdecor "$name" >/dev/null 2>&1
		sleep 2
		if [ "$(app_server_team)" != "$team" ]; then
			mv -f "$dest/$name" "$dest/$name.failed"
			setdecor Default >/dev/null 2>&1
			fail "app_server didn't take the new $name; it is set aside as $name.failed"
		fi
	fi
done

say 100 "Built:$built"
exit 0
