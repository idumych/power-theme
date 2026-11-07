#!/bin/sh
# Build Power Theme's add-ons, the window decorators and control looks
# (Reactionary, Smoke, Gonx, and Haiku's Flat ones if the system lacks
# them), on, and for, the running system.
#
# On Linux it builds them for Hokku, the Haiku API there, which runs
# decorators in each application (libdecorator) rather than in a server:
# against the headers and libraries of Hokku's install, so there is nothing
# to fetch, and into Hokku's add-ons folder in the home folder.
#
# A decorator runs inside app_server and uses its private classes, so it
# must be compiled against the headers of the exact system build it runs
# on and linked against that system's app_server. A control look subclasses
# the private HaikuControlLook, so it must match the system's libbe too. This finds the system's
# revision (from its haiku package, e.g. hrev60072+339), takes the bundled
# headers if they are from that revision, and otherwise fetches them from
# the system's source repository at that revision, with git: Renku's from
# RenkuOS/Source; Haiku's tags from review.haiku-os.org and its commits
# from the haiku/haiku mirror on GitHub (whose partial fetches Gerrit lacks).
# It fetches the branch's commits only (no files), finds the commit as many
# commits past the hrev tag as the revision says, as git describe counts,
# then checks out just the header files. They are kept in
# ~/config/settings/PowerTheme (~/config/cache is read-only on Renku). Without
# the right headers it stops: add-ons built for another system build could
# take app_server down when a theme is applied.
#
#   sh build-decorators.sh [--install-tools] [--headers=auto|bundled|fetch]
#                          [--source=DIR] [--dest=DIR]
#
#   --install-tools  install the compiler first (pkgman: gcc, haiku_devel)
#   --headers        auto (default): bundled if they match, else fetched;
#                    bundled: the bundled ones whatever the system (unsafe)
#   --source         the add-ons' source: Power Theme's "source" folder, or a
#                    Reactionary source tree (default: next to this script)
#   --dest           the add-ons folder to put them in, in "decorators" and
#                    "control_look" (default: the user's non-packaged one)
#   --revision=REV   take this revision (hrev59866+92, or a haiku package
#                    version such as r1~beta6_hrev59866_92) instead of the
#                    system's; with --fetch-only, fetch its headers and stop
#                    (for testing, or to fetch on another machine; the
#                    cache folder is then $POWERTHEME_CACHE, if set)
#
# Output, for Power Theme's build window: "STEP <percent> <what>" lines for the
# progress bar, "ERROR <what>" for failures, everything else is the log.
# Exit status: 0 built, 1 failed, 3 tools are missing (the NEED_TOOLS line
# names their packages; --install-tools installs them).

set -u

say() { echo "STEP $1 $2"; }
fail() { echo "ERROR $1"; exit 1; }

here=$(cd "$(dirname "$0")" && pwd)
source=""
dest=""
headers_mode=auto
install_tools=false
forced_revision=""
fetch_only=false
for arg in "$@"; do
	case $arg in
		--install-tools) install_tools=true ;;
		--headers=*) headers_mode=${arg#--headers=} ;;
		--source=*) source=${arg#--source=} ;;
		--dest=*) dest=${arg#--dest=} ;;
		--revision=*) forced_revision=${arg#--revision=} ;;
		--fetch-only) fetch_only=true ;;
		*) fail "unknown option $arg" ;;
	esac
done

if [ -z "$source" ]; then
	if [ -d "$here/source" ]; then
		source=$here/source
	elif [ -d "$here/../../src/decorator" ]; then
		source=$(cd "$here/../.." && pwd)
	else
		fail "the add-ons' source isn't installed next to $0"
	fi
fi
# only decorators installed where app_server finds them are switched to
reload=false
if [ -z "$dest" ] && ! $fetch_only; then
	dest=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY)
	reload=true
fi
cache=${POWERTHEME_CACHE:-$(finddir B_USER_SETTINGS_DIRECTORY 2>/dev/null)/PowerTheme}

# the add-ons: kind, name, folder in an installed source, folder in the
# tree, and the source files' and resource file's names
addons="decorators:Reactionary:decorators/Reactionary:src/decorator:*Decorator.cpp:resources.rdef
decorators:Smoke:decorators/Smoke:src/smoke:*Decorator.cpp:resources.rdef
decorators:Tidy:decorators/Tidy:src/tidy:TidyDecorator.cpp:resources.rdef
decorators:TidySquare:decorators/TidySquare:src/tidy:TidySquareDecorator.cpp:square-resources.rdef
decorators:Expose:decorators/Expose:src/third_party/expose/decorator:*Decorator.cpp:resources.rdef
decorators:Platinum:decorators/Platinum:src/third_party/platinum/decorator:*Decorator.cpp:resources.rdef
decorators:Gonx:decorators/Gonx:src/third_party/gonx/decorator:*Decorator.cpp:resources.rdef
decorators:FlatDecorator:decorators/FlatDecorator:src/third_party/haiku-flat/decorator:*Decorator.cpp:resources.rdef
control_look:Reactionary:control_looks/Reactionary:src/controllook:*ControlLook.cpp:*ControlLook.rdef
control_look:Smoke:control_looks/Smoke:src/smoke:*ControlLook.cpp:*ControlLook.rdef
control_look:Dano:control_looks/Dano:src/dano:*ControlLook.cpp:*ControlLook.rdef
control_look:Tidy:control_looks/Tidy:src/tidy:*ControlLook.cpp:*ControlLook.rdef
control_look:Expose:control_looks/Expose:src/third_party/expose/controllook:*ControlLook.cpp:*ControlLook.rdef
control_look:Platinum:control_looks/Platinum:src/third_party/platinum/controllook:*ControlLook.cpp:*ControlLook.rdef
control_look:Gonx:control_looks/Gonx:src/third_party/gonx/controllook:*ControlLook.cpp:*ControlLook.rdef
control_look:FlatControlLook:control_looks/FlatControlLook:src/third_party/haiku-flat/controllook:*ControlLook.cpp:*ControlLook.rdef"


# Hokku: the compiler, Hokku's install, and what to build with

hokku=false
[ "$(uname -s)" = Haiku ] || hokku=true
develFolder=""
develFlags=""
develLink=""
build=""
trap 'rm -rf "$develFolder" "$build"' EXIT
if $hokku; then
	$fetch_only && fail "--fetch-only is for Haiku and Renku"
	say 6 "Finding Hokku"
	addonsDir=$(finddir B_SYSTEM_ADDONS_DIRECTORY 2>/dev/null) \
		|| fail "Hokku isn't installed (no finddir)"
	libDir=$(dirname "$addonsDir")
	prefix=$(dirname "$libDir")
	include=$prefix/include
	missing=""
	command -v g++ >/dev/null 2>&1 || missing="$missing g++"
	command -v objcopy >/dev/null 2>&1 || missing="$missing binutils"
	[ -x "$prefix/bin/rc" ] || missing="$missing Hokku's rc"
	[ -f "$include/private/app_server/Decorator.h" ] \
		&& [ -e "$libDir/libdecorator.so" ] \
		|| missing="$missing Hokku's development files"
	if [ -n "$missing" ]; then
		# installing them is the administrator's, with the distribution's
		# package manager: --install-tools can't
		echo "NEED_TOOLS$missing"
		echo "ERROR not installed:$missing"
		exit 3
	fi
	g++ --version | head -1
	echo "Hokku: $prefix"

	kits=""
	for kit in config os os/app os/interface os/kernel os/storage os/support \
			os/translation os/locale os/add-ons/graphics private \
			private/interface private/shared private/app private/system \
			posix; do
		kits="$kits -I$include/$kit"
	done
	common="-O2 -Wall -Wno-multichar -fPIC -std=gnu++17 -DAPP_COSMOE"
	flags="$common -I$include/private/app_server -I$include $kits"
	lookFlags="$common -Wno-unused-variable -I$include $kits"
	decorLink="-Wl,--no-undefined -L$libDir -ldecorator -lbe"
	lookLink="-Wl,--no-undefined -L$libDir -lbe"
	rc=$prefix/bin/rc
	systemAddons=$addonsDir
	[ -n "$dest" ] || { dest=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY); \
		reload=true; }
fi

# Resources go in the add-on: as an attribute on Haiku, a section on Hokku
add_resources() {
	if $hokku; then
		LD_LIBRARY_PATH=$libDir "$rc" -o "$1.rsrc" "$2" 2>&1 \
			|| fail "$3: rc failed"
		objcopy --add-section .cosmoe_rsrc="$1.rsrc" \
			--set-section-flags .cosmoe_rsrc=readonly,data "$1" 2>&1 \
			|| fail "$3: objcopy failed"
	else
		rc -o "$1.rsrc" "$2" 2>&1 || fail "$3: rc failed"
		xres -o "$1" "$1.rsrc" 2>&1 || fail "$3: xres failed"
	fi
	rm -f "$1.rsrc"
}

# Haiku and Renku: steps 1 to 3 find the system's own headers
if ! $hokku; then

# 1. the system's revision

say 2 "Finding the system's revision"
if [ -n "$forced_revision" ]; then
	# hrev59866+92, or a haiku package version: r1~beta6_hrev59866_92
	case $forced_revision in
		r*)
			package="haiku-$forced_revision-"
			hrev=$(echo "$forced_revision" | sed -n 's/.*_hrev\([0-9][0-9]*\).*/\1/p')
			ahead=$(echo "$forced_revision" | sed -n 's/.*_hrev[0-9][0-9]*_\([0-9][0-9]*\)$/\1/p')
			;;
		*)
			package="(given) $forced_revision"
			hrev=$(echo "$forced_revision" | sed -n 's/^hrev\([0-9][0-9]*\).*/\1/p')
			ahead=$(echo "$forced_revision" | sed -n 's/^hrev[0-9]*+\([0-9][0-9]*\)$/\1/p')
			;;
	esac
	[ -n "$hrev" ] || fail "--revision must look like hrev59866+92 or r1~beta6_hrev59866_92"
	revision=hrev$hrev${ahead:++$ahead}
else
	package=$(ls /boot/system/packages/haiku-r*.hpkg 2>/dev/null | head -1)
	[ -n "$package" ] || fail "no haiku package in /boot/system/packages"
	hrev=$(basename "$package" | sed -n 's/.*_hrev\([0-9][0-9]*\).*/\1/p')
	ahead=$(basename "$package" | sed -n 's/.*_hrev[0-9][0-9]*_\([0-9][0-9]*\)-.*/\1/p')
	[ -n "$hrev" ] || fail "can't read the revision from $(basename "$package")"
	revision=hrev$hrev${ahead:++$ahead}
fi
echo "System: $(basename "$package" .hpkg), revision $revision"


# 2. the tools: the compiler, and git if the headers must be fetched

bundled=$source/build-headers
bundledRevision=$(cat "$bundled/REVISION" 2>/dev/null || echo unknown)
needGit=false
if [ "$headers_mode" = fetch ] || { [ "$headers_mode" = auto ] \
		&& [ "$bundledRevision" != "$revision" ] \
		&& [ ! -f "$cache/headers/$revision/.complete" ]; }; then
	needGit=true
fi

missing_packages() {
	packages=""
	command -v g++ >/dev/null 2>&1 || packages="$packages gcc"
	{ command -v rc >/dev/null 2>&1 && command -v xres >/dev/null 2>&1 \
		&& { [ -f /boot/system/develop/headers/os/interface/View.h ] \
			|| [ -n "$develFolder" ]; }; } \
		|| packages="$packages haiku_devel"
	if $needGit; then
		command -v git >/dev/null 2>&1 || packages="$packages git"
	fi
	echo $packages
}

say 6 "Checking for the compiler"
if $fetch_only; then
	needGit=true
	missing_packages() {
		command -v git >/dev/null 2>&1 || echo git
	}
fi

# The repository's haiku_devel can be for an older build than the system, and
# pkgman only offers to downgrade the system to install it. These add-ons just
# need its headers and startup objects, so it's unpacked for this build instead.
develFolder=""
develFlags=""
develLink=""
build=""
trap 'rm -rf "$develFolder" "$build"' EXIT
unpack_devel() {
	found=$(pkgman search -D haiku_devel 2>/dev/null \
		| awk '$2 == "haiku_devel" { print $1, $3; exit }')
	repository=${found%% *}
	version=${found#* }
	[ -n "$found" ] || return 1
	base=$(pkgman list-repos -v 2>/dev/null | awk -v name="$repository" '
		$1 == name && NF == 1 { inside = 1; next }
		inside && $1 == "base-url:" && NF > 1 { print $2; exit }')
	[ -n "$base" ] || return 1
	folder=$(mktemp -d "${TMPDIR:-/tmp}/themes-devel.XXXXXX") || return 1
	file=$folder/haiku_devel.hpkg
	if ! curl -fsSL -o "$file" \
			"$base/packages/haiku_devel-$version-x86_64.hpkg" \
		|| ! (cd "$folder" && package extract "$file" >/dev/null); then
		rm -rf "$folder"
		return 1
	fi
	rm -f "$file"
	develFolder=$folder
	headers=$folder/develop/headers
	develFlags="-B$folder/develop/lib/ -idirafter $headers/posix"
	develFlags="$develFlags -idirafter $headers"
	for kit in $(find "$headers/os" -type d); do
		develFlags="$develFlags -idirafter $kit"
	done
	develLink="-B$folder/develop/lib/ -L/boot/system/lib"
}

packages=$(missing_packages)
if [ -n "$packages" ] && $install_tools; then
	say 7 "Installing $packages"
	# one at a time: pkgman installs none of them if it can't install one
	for package in $packages; do
		# shellcheck disable=SC2086
		pkgman install -y $package 2>&1 | grep -v '^[[:space:]]*$' | tail -20
	done
	packages=$(missing_packages)
	case " $packages " in
		*" haiku_devel "*)
			say 8 "Unpacking the development files"
			unpack_devel || echo "Can't unpack haiku_devel"
			packages=$(missing_packages)
			;;
	esac
fi
if [ -n "$packages" ]; then
	echo "NEED_TOOLS $packages"
	echo "ERROR not installed: $packages"
	exit 3
fi
$fetch_only || g++ --version | head -1

# 3. the headers

headers=""

# the commit of a tag, from "git ls-remote" (peeled, if annotated)
tag_commit() {
	git ls-remote --tags "$1" "refs/tags/$2" "refs/tags/$2^{}" 2>/dev/null \
		| sort -k2 | tail -1 | cut -f1
}

# In the fetched branch $1, the first-parent commit with $ahead commits
# that $2 lacks (git describe's count, which only grows along the branch):
# a binary search over the branch's first-parent line since the tag.
find_ahead() {
	list=$work/line
	git -C "$work/git" rev-list --first-parent "$1" "^$2" > "$list" || return 1
	low=1
	high=$(wc -l < "$list")
	while [ "$low" -le "$high" ]; do
		middle=$(( (low + high) / 2 ))
		c=$(sed -n "${middle}p" "$list")
		n=$(git -C "$work/git" rev-list --count "$c" "^$2")
		if [ "$n" -eq "$ahead" ]; then
			echo "$c"
			return 0
		elif [ "$n" -gt "$ahead" ]; then
			low=$((middle + 1))
		else
			high=$((middle - 1))
		fi
	done
	return 1
}

# Every commit on Haiku's master has its own hrev tag, so a revision past
# one (hrev59866+92) is on a release branch: the one the system's version
# names (r1~beta6: r1beta6) first, master last.
release=$(echo "${package##*/}" | sed -n 's/^haiku-\(r[0-9]*\)~\(beta[0-9]*\)_.*/\1\2/p')
haikuBranches="$release r1beta6 r1beta7 r1beta5 master"

fetch_headers() {
	target=$cache/headers/$revision
	if [ -f "$target/.complete" ]; then
		echo "Using the headers fetched before, in $target"
		headers=$target
		return 0
	fi

	work=$(mktemp -d "${TMPDIR:-/tmp}/themes-headers.XXXXXX") || return 1
	paths=$(cd "$bundled/renku" && find . -type f | sed 's|^\./||')
	roundedPaths="src/servers/app/decorator/RoundedDecorator.h
		src/servers/app/WindowShape.h"
	# repository name, where its tags are, where its commits are, branches
	for source_line in \
		"RenkuOS/Source|https://github.com/RenkuOS/Source.git|https://github.com/RenkuOS/Source.git|HEAD" \
		"Haiku|https://review.haiku-os.org/haiku|https://github.com/haiku/haiku.git|$haikuBranches"; do
		repo=${source_line%%|*}
		rest=${source_line#*|}
		tags=${rest%%|*}
		rest=${rest#*|}
		objects=${rest%%|*}
		branches=${rest#*|}

		say 16 "Looking for hrev$hrev in $repo"
		tag=$(tag_commit "$tags" "hrev$hrev")
		[ -n "$tag" ] || continue
		echo "hrev$hrev is $repo commit $(echo "$tag" | cut -c1-10)"

		rm -rf "$work/git"
		git init -q "$work/git" || return 1
		git -C "$work/git" remote add origin "$objects"
		commit=""
		if [ -z "$ahead" ]; then
			say 22 "Fetching hrev$hrev"
			git -C "$work/git" fetch -q --depth=1 --filter=tree:0 origin "$tag" \
				&& commit=$tag
		else
			tried=" "
			for branch in $branches; do
				case $tried in *" $branch "*) continue ;; esac
				tried="$tried$branch "
				if [ "$branch" = HEAD ]; then
					branch=$(git ls-remote --symref "$objects" HEAD 2>/dev/null \
						| sed -n 's|^ref: refs/heads/\([^[:space:]]*\).*|\1|p')
				fi
				say 22 "Fetching $repo's $branch history (no files)"
				git -C "$work/git" fetch -q --filter=tree:0 origin \
					"refs/heads/$branch:refs/remotes/origin/$branch" 2>/dev/null \
					|| continue
				git -C "$work/git" merge-base --is-ancestor "$tag" \
					"origin/$branch" 2>/dev/null || continue
				say 26 "Finding $revision on $branch"
				commit=$(find_ahead "origin/$branch" "$tag") && break
				commit=""
			done
		fi
		if [ -z "$commit" ]; then
			echo "$revision isn't on $repo's branches"
			continue
		fi
		echo "$revision is $repo commit $(echo "$commit" | cut -c1-10)"
		say 30 "Fetching the headers"
		# shellcheck disable=SC2086
		if git -C "$work/git" -c advice.detachedHead=false checkout -q \
				"$commit" -- $paths; then
			# Only a composited app_server has these; FlatDecorator rounds
			# its corners when they are there.
			kept=$paths
			# shellcheck disable=SC2086
			git -C "$work/git" -c advice.detachedHead=false checkout -q \
				"$commit" -- $roundedPaths 2>/dev/null \
				&& kept="$paths $roundedPaths"
			rm -rf "$target"
			if mkdir -p "$target/renku" \
				&& (cd "$work/git" && tar cf - $kept) \
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

say 12 "Finding the headers for $revision"
case $headers_mode in
	bundled)
		headers=$bundled
		;;
	fetch|auto)
		if [ "$headers_mode" = auto ] && [ "$bundledRevision" = "$revision" ]; then
			echo "The bundled headers are from $revision"
			headers=$bundled
		elif ! fetch_headers; then
			fail "couldn't get the headers for $revision (is the network up?)"
		fi
		;;
	*) fail "--headers must be auto, bundled or fetch" ;;
esac
echo "Headers: $headers"
if $fetch_only; then
	say 100 "Fetched the headers for $revision"
	exit 0
fi

freetype=/boot/system/develop/headers/freetype2
[ -f "$freetype/ft2build.h" ] || freetype=$bundled/freetype2


# 4. compile (decorators get app_server's headers; control looks only
# need the private HaikuControlLook.h)

app=$headers/renku/src/servers/app
private=$headers/renku/headers/private
flags="-O2 -Wall -Wno-multichar -fPIC -std=gnu++17
	-iquote $app -iquote $app/stackandtile -iquote $app/decorator
	-iquote $app/drawing -iquote $app/drawing/Painter -iquote $app/font
	-I$private -I$private/app -I$private/shared -I$private/interface
	-I$private/graphics -I$headers/renku/headers/libs/agg -I$freetype"
lookFlags="-O2 -Wall -Wno-multichar -Wno-unused-variable -fPIC
	-std=gnu++17 -I$private/interface"
decorLink="-lbe /boot/system/servers/app_server -lstdc++"
lookLink="-lbe -lstdc++"
systemAddons=/boot/system/add-ons

fi	# Haiku and Renku

build=$(mktemp -d "${TMPDIR:-/tmp}/themes-build.XXXXXX") || fail "no temporary folder"

todo=""
for entry in $addons; do
	kind=$(echo "$entry" | cut -d: -f1)
	name=$(echo "$entry" | cut -d: -f2)
	# Haiku's Flat add-ons come with its haiku_extras package
	if [ -e "$systemAddons/$kind/$name" ]; then
		echo "$name ($kind): the system has it already"
		continue
	fi
	todo="$todo $kind:$name"
done
count=$(echo $todo | wc -w)

index=0
built=""
for item in $todo; do
	kind=${item%%:*}
	name=${item#*:}
	entry=$(echo "$addons" | grep "^$kind:$name:")
	folder=$source/$(echo "$entry" | cut -d: -f3)
	[ -d "$folder" ] || folder=$source/$(echo "$entry" | cut -d: -f4)
	[ -d "$folder" ] || fail "$name: no source in $source"
	sources=$(echo "$entry" | cut -d: -f5)
	resources=$(echo "$entry" | cut -d: -f6)
	if [ "$kind" = decorators ]; then
		label="$name window borders"
		kindflags=$flags
		link=$decorLink
	else
		label="$name controls"
		kindflags=$lookFlags
		link=$lookLink
	fi

	percent=$((35 + index * 55 / count))
	say $percent "Compiling $label"
	objects=""
	for file in "$folder"/$sources; do
		[ -f "$file" ] || fail "$name: no $sources in $folder"
		object=$build/$kind-$name-$(basename "$file" .cpp).o
		echo "g++ -c $(basename "$file")"
		# shellcheck disable=SC2086
		g++ $kindflags $develFlags -c "$file" -o "$object" 2>&1 \
			|| fail "$label: doesn't compile"
		objects="$objects $object"
	done
	output=$build/$kind/$name
	mkdir -p "$build/$kind"
	# shellcheck disable=SC2086
	g++ -shared -s -o "$output" $objects $develLink $link 2>&1 \
		|| fail "$label: doesn't link"
	for rdef in "$folder"/$resources; do
		add_resources "$output" "$rdef" "$label"
	done
	built="$built $kind:$name"
	index=$((index + 1))
done


# 5. install

say 92 "Installing"
app_server_team() {
	ps | awk '$1 == "/boot/system/servers/app_server" { print $2; exit }'
}
active=$(setdecor -l 2>/dev/null | sed -n 's/^\*\([^ ]*\).*/\1/p')
names=""
for item in $built; do
	kind=${item%%:*}
	name=${item#*:}
	mkdir -p "$dest/$kind" || fail "can't create $dest/$kind"
	# app_server and applications map the add-ons they use: never
	# overwrite one in place
	cp "$build/$kind/$name" "$dest/$kind/$name.new" \
		&& mv -f "$dest/$kind/$name.new" "$dest/$kind/$name" \
		|| fail "can't install $name in $dest/$kind"
	echo "Installed $dest/$kind/$name"
	names="$names $name"
	if $reload && $hokku && [ "$kind" = decorators ] \
			&& [ "$name" = "$active" ]; then
		# each application runs its own copy: windows opened from now on
		# have the new one
		echo "Applications started from now on use the new $name"
	elif $reload && [ "$kind" = decorators ] && [ "$name" = "$active" ]; then
		# load the new one, and check app_server takes it
		say 96 "Switching to the new $name window borders"
		team=$(app_server_team)
		setdecor Default >/dev/null 2>&1
		setdecor "$name" >/dev/null 2>&1
		sleep 2
		if [ "$(app_server_team)" != "$team" ]; then
			mv -f "$dest/$kind/$name" "$dest/$kind/$name.failed"
			setdecor Default >/dev/null 2>&1
			fail "app_server didn't take the new $name; it is set aside as $name.failed"
		fi
	fi
done

say 100 "Built:$(echo $names | tr ' ' '\n' | sort -u | tr '\n' ' ')"
exit 0
