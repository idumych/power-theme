# Reactionary look for Renku OS (and Haiku).
#
# The decorator is an app_server add-on: it is compiled against app_server's
# private headers and linked against the app_server binary of the system it
# will run on, so it should be built on (or for) that system.
#
# On Renku or Haiku itself, `make` builds against the running system, using
# the private headers bundled in build-headers/ (from a Renku source tree;
# FreeType's too). install.sh does this for you.
#
# Cross-build on Linux, against a Renku source tree and a system's sysroot:
#
#   sh tools/sync-sysroot.sh <ssh-host>     # once per Renku build
#   make
#
# For Hokku, the Haiku API on Linux, see hokku.mk:
#
#   make COSMOE_ROOT=/usr

ifdef COSMOE_ROOT
include hokku.mk
else

RENKU_SRC ?= $(HOME)/Projects/renku-src
SYSROOT ?= $(CURDIR)/build/sysroot

ifeq ($(shell uname -s),Haiku)
CXX ?= g++
APP_SERVER ?= /boot/system/servers/app_server
RC ?= rc
XRES ?= xres
SYSROOT_FLAGS :=
else
CROSS ?= $(RENKU_SRC)/generated.x86_64/cross-tools-x86_64/bin/x86_64-unknown-haiku-
CXX := $(CROSS)g++
APP_SERVER ?= $(SYSROOT)/boot/system/servers/app_server
RC ?= ssh $(RC_HOST) rc
XRES ?= $(RENKU_SRC)/generated.x86_64/objects/linux/x86_64/release/tools/xres
SYSROOT_FLAGS := --sysroot=$(SYSROOT)
endif

ifeq ($(shell uname -s),Haiku)
HEADER_ROOT ?= $(CURDIR)/build-headers/renku
FREETYPE_HEADERS ?= $(CURDIR)/build-headers/freetype2
else
HEADER_ROOT ?= $(RENKU_SRC)
FREETYPE_HEADERS ?= $(SYSROOT)/boot/system/develop/headers/freetype2
endif

APP := $(HEADER_ROOT)/src/servers/app
PRIVATE := $(HEADER_ROOT)/headers/private

DECOR_INCLUDES := \
	-iquote $(APP) -iquote $(APP)/stackandtile -iquote $(APP)/decorator \
	-iquote $(APP)/drawing -iquote $(APP)/drawing/Painter -iquote $(APP)/font \
	-I$(PRIVATE) -I$(PRIVATE)/app -I$(PRIVATE)/shared \
	-I$(PRIVATE)/interface -I$(PRIVATE)/graphics \
	-I$(HEADER_ROOT)/headers/libs/agg \
	-I$(FREETYPE_HEADERS)

override CXXFLAGS += -O2 -Wall -Wno-multichar -fPIC $(SYSROOT_FLAGS)

BUILD := build
DECOR := $(BUILD)/Reactionary
THEMES := $(BUILD)/PowerTheme
LOOK := $(BUILD)/control_look/Reactionary

FLAT_DECOR := $(BUILD)/FlatDecorator
FLAT_LOOK := $(BUILD)/control_look/FlatControlLook
FLAT := src/third_party/haiku-flat

SMOKE_DECOR := $(BUILD)/Smoke
SMOKE_LOOK := $(BUILD)/control_look/Smoke
DANO_LOOK := $(BUILD)/control_look/Dano
TIDY_DECOR := $(BUILD)/Tidy
TIDY_SQUARE_DECOR := $(BUILD)/TidySquare
TIDY_LOOK := $(BUILD)/control_look/Tidy
GONX := src/third_party/gonx
GONX_LOOK := $(BUILD)/control_look/Gonx
EXPOSE := src/third_party/expose
EXPOSE_DECOR := $(BUILD)/Expose
EXPOSE_LOOK := $(BUILD)/control_look/Expose
PLATINUM := src/third_party/platinum
PLATINUM_DECOR := $(BUILD)/Platinum
PLATINUM_LOOK := $(BUILD)/control_look/Platinum
LEOPARD_DECOR := $(BUILD)/Leopard
LEOPARD_LOOK := $(BUILD)/control_look/Leopard
GERSHWIX_DECOR := $(BUILD)/Gershwix
GERSHWIX_LOOK := $(BUILD)/control_look/Gershwix
NEXT_DECOR := $(BUILD)/NeXT
NEXT_LOOK := $(BUILD)/control_look/NeXT

# What Power Theme needs to build the add-ons (decorators, control looks) on a
# user's system, laid
# out as build-decorators.sh expects it in a data folder's "PowerTheme".
DECOR_SOURCE := $(BUILD)/data/PowerTheme

all: $(DECOR) $(LOOK) $(THEMES) $(FLAT_DECOR) $(FLAT_LOOK) $(SMOKE_DECOR) \
	$(SMOKE_LOOK) $(DANO_LOOK) $(TIDY_DECOR) $(TIDY_SQUARE_DECOR) $(TIDY_LOOK) \
	$(GONX_LOOK) \
	$(EXPOSE_DECOR) $(EXPOSE_LOOK) $(PLATINUM_DECOR) $(PLATINUM_LOOK) \
	$(LEOPARD_DECOR) $(LEOPARD_LOOK) $(GERSHWIX_DECOR) $(GERSHWIX_LOOK) \
	$(NEXT_DECOR) $(NEXT_LOOK) \
	decorator-source

decorator-source:
	rm -rf $(DECOR_SOURCE)
	mkdir -p $(DECOR_SOURCE)/source/decorators/Reactionary \
		$(DECOR_SOURCE)/source/decorators/Smoke \
		$(DECOR_SOURCE)/source/decorators/Tidy \
		$(DECOR_SOURCE)/source/decorators/TidySquare \
		$(DECOR_SOURCE)/source/decorators/Expose \
		$(DECOR_SOURCE)/source/decorators/Platinum \
		$(DECOR_SOURCE)/source/decorators/Leopard \
		$(DECOR_SOURCE)/source/decorators/Gershwix \
		$(DECOR_SOURCE)/source/decorators/NeXT \
		$(DECOR_SOURCE)/source/decorators/Gonx \
		$(DECOR_SOURCE)/source/decorators/FlatDecorator \
		$(DECOR_SOURCE)/source/control_looks/Reactionary \
		$(DECOR_SOURCE)/source/control_looks/Smoke \
		$(DECOR_SOURCE)/source/control_looks/Dano \
		$(DECOR_SOURCE)/source/control_looks/Tidy \
		$(DECOR_SOURCE)/source/control_looks/Expose \
		$(DECOR_SOURCE)/source/control_looks/Platinum \
		$(DECOR_SOURCE)/source/control_looks/Leopard \
		$(DECOR_SOURCE)/source/control_looks/Gershwix \
		$(DECOR_SOURCE)/source/control_looks/NeXT \
		$(DECOR_SOURCE)/source/control_looks/Gonx \
		$(DECOR_SOURCE)/source/control_looks/FlatControlLook
	cp src/themes/build-decorators.sh $(DECOR_SOURCE)/
	cp -R build-headers $(DECOR_SOURCE)/source/
	cp src/decorator/ReactionaryDecorator.* src/decorator/resources.rdef \
		$(DECOR_SOURCE)/source/decorators/Reactionary/
	cp src/smoke/SmokeDecorator.* src/smoke/resources.rdef \
		$(DECOR_SOURCE)/source/decorators/Smoke/
	cp $(GONX)/decorator/* $(GONX)/LICENSE \
		$(DECOR_SOURCE)/source/decorators/Gonx/
	cp $(FLAT)/decorator/* $(DECOR_SOURCE)/source/decorators/FlatDecorator/
	cp src/controllook/* $(DECOR_SOURCE)/source/control_looks/Reactionary/
	cp src/smoke/SmokeControlLook.* $(DECOR_SOURCE)/source/control_looks/Smoke/
	cp src/dano/DanoControlLook.* $(DECOR_SOURCE)/source/control_looks/Dano/
	cp src/tidy/TidyDecorator.* src/tidy/resources.rdef \
		$(DECOR_SOURCE)/source/decorators/Tidy/
	cp src/tidy/TidySquareDecorator.cpp src/tidy/TidyDecorator.* \
		src/tidy/square-resources.rdef $(DECOR_SOURCE)/source/decorators/TidySquare/
	cp src/tidy/TidyControlLook.* $(DECOR_SOURCE)/source/control_looks/Tidy/
	cp $(EXPOSE)/decorator/* $(EXPOSE)/LICENSE $(EXPOSE)/README \
		$(DECOR_SOURCE)/source/decorators/Expose/
	cp $(EXPOSE)/controllook/* $(EXPOSE)/LICENSE $(EXPOSE)/README \
		$(DECOR_SOURCE)/source/control_looks/Expose/
	cp $(PLATINUM)/decorator/* $(PLATINUM)/LICENSE $(PLATINUM)/README \
		$(DECOR_SOURCE)/source/decorators/Platinum/
	cp $(PLATINUM)/controllook/* $(PLATINUM)/LICENSE $(PLATINUM)/README \
		$(DECOR_SOURCE)/source/control_looks/Platinum/
	cp src/leopard/LeopardDecorator.* src/leopard/LeopardButtons.h \
		src/leopard/resources.rdef $(DECOR_SOURCE)/source/decorators/Leopard/
	cp src/leopard/LeopardControlLook.* \
		$(DECOR_SOURCE)/source/control_looks/Leopard/
	cp src/gershwix/GershwixDecorator.* src/gershwix/resources.rdef \
		$(DECOR_SOURCE)/source/decorators/Gershwix/
	cp src/gershwix/GershwixControlLook.* \
		$(DECOR_SOURCE)/source/control_looks/Gershwix/
	cp src/next/NextDecorator.* src/next/resources.rdef \
		$(DECOR_SOURCE)/source/decorators/NeXT/
	cp src/next/NextControlLook.* \
		$(DECOR_SOURCE)/source/control_looks/NeXT/
	cp $(GONX)/controllook/* $(GONX)/LICENSE \
		$(DECOR_SOURCE)/source/control_looks/Gonx/
	cp $(FLAT)/controllook/* $(DECOR_SOURCE)/source/control_looks/FlatControlLook/
	cp LICENSE $(DECOR_SOURCE)/source/

# Gonx's control look, for the Gonx theme (its decorator is built by Power Theme).
$(BUILD)/GonxControlLook.o: $(GONX)/controllook/GonxControlLook.cpp $(GONX)/controllook/GonxControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/GonxControlLook.rsrc: $(GONX)/controllook/GonxControlLook.rdef

$(GONX_LOOK): $(BUILD)/GonxControlLook.o $(BUILD)/GonxControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/GonxControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/GonxControlLook.rsrc

# Smoke, a decorator in the style of ZETA's, for Theme Manager themes that
# ask for it.
$(BUILD)/SmokeDecorator.o: src/smoke/SmokeDecorator.cpp src/smoke/SmokeDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/SmokeDecorator.rsrc: src/smoke/resources.rdef

$(SMOKE_DECOR): $(BUILD)/SmokeDecorator.o $(BUILD)/SmokeDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/SmokeDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/SmokeDecorator.rsrc

$(BUILD)/SmokeControlLook.o: src/smoke/SmokeControlLook.cpp src/smoke/SmokeControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/SmokeControlLook.rsrc: src/smoke/SmokeControlLook.rdef

$(SMOKE_LOOK): $(BUILD)/SmokeControlLook.o $(BUILD)/SmokeControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/SmokeControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/SmokeControlLook.rsrc

$(BUILD)/DanoControlLook.o: src/dano/DanoControlLook.cpp src/dano/DanoControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/DanoControlLook.rsrc: src/dano/DanoControlLook.rdef

$(DANO_LOOK): $(BUILD)/DanoControlLook.o $(BUILD)/DanoControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/DanoControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/DanoControlLook.rsrc

# Tidy, after the mockups on Haiku's forum.
$(BUILD)/TidyDecorator.o: src/tidy/TidyDecorator.cpp src/tidy/TidyDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/TidyDecorator.rsrc: src/tidy/resources.rdef

$(TIDY_DECOR): $(BUILD)/TidyDecorator.o $(BUILD)/TidyDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/TidyDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/TidyDecorator.rsrc

# Tidy with square corners, for a compositor that rounds windows itself.
$(BUILD)/TidySquareDecorator.o: src/tidy/TidySquareDecorator.cpp src/tidy/TidyDecorator.cpp src/tidy/TidyDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/TidySquareDecorator.rsrc: src/tidy/square-resources.rdef

$(TIDY_SQUARE_DECOR): $(BUILD)/TidySquareDecorator.o $(BUILD)/TidySquareDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/TidySquareDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/TidySquareDecorator.rsrc

$(BUILD)/TidyControlLook.o: src/tidy/TidyControlLook.cpp src/tidy/TidyControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/TidyControlLook.rsrc: src/tidy/TidyControlLook.rdef

$(TIDY_LOOK): $(BUILD)/TidyControlLook.o $(BUILD)/TidyControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/TidyControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/TidyControlLook.rsrc

# Expose, after Mark Whittaker's themes. GPL, unlike the rest; see its README.
$(BUILD)/ExposeDecorator.o: $(EXPOSE)/decorator/ExposeDecorator.cpp $(EXPOSE)/decorator/ExposeDecorator.h $(EXPOSE)/decorator/ExposeButtons.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/ExposeDecorator.rsrc: $(EXPOSE)/decorator/resources.rdef

$(EXPOSE_DECOR): $(BUILD)/ExposeDecorator.o $(BUILD)/ExposeDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/ExposeDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/ExposeDecorator.rsrc

$(BUILD)/ExposeControlLook.o: $(EXPOSE)/controllook/ExposeControlLook.cpp $(EXPOSE)/controllook/ExposeControlLook.h $(EXPOSE)/controllook/ExposeMarks.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/ExposeControlLook.rsrc: $(EXPOSE)/controllook/ExposeControlLook.rdef

$(EXPOSE_LOOK): $(BUILD)/ExposeControlLook.o $(BUILD)/ExposeControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/ExposeControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/ExposeControlLook.rsrc

# Platinum, after Niels's KDE theme. GPL, unlike the rest; see its README.
$(BUILD)/PlatinumDecorator.o: $(PLATINUM)/decorator/PlatinumDecorator.cpp $(PLATINUM)/decorator/PlatinumDecorator.h $(PLATINUM)/decorator/PlatinumButtons.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/PlatinumDecorator.rsrc: $(PLATINUM)/decorator/resources.rdef

$(PLATINUM_DECOR): $(BUILD)/PlatinumDecorator.o $(BUILD)/PlatinumDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/PlatinumDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/PlatinumDecorator.rsrc

$(BUILD)/PlatinumControlLook.o: $(PLATINUM)/controllook/PlatinumControlLook.cpp $(PLATINUM)/controllook/PlatinumControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/PlatinumControlLook.rsrc: $(PLATINUM)/controllook/PlatinumControlLook.rdef

$(PLATINUM_LOOK): $(BUILD)/PlatinumControlLook.o $(BUILD)/PlatinumControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/PlatinumControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/PlatinumControlLook.rsrc

# Leopard, in the style of Mac OS X 10.5.
$(BUILD)/LeopardDecorator.o: src/leopard/LeopardDecorator.cpp src/leopard/LeopardDecorator.h src/leopard/LeopardButtons.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/LeopardDecorator.rsrc: src/leopard/resources.rdef

$(LEOPARD_DECOR): $(BUILD)/LeopardDecorator.o $(BUILD)/LeopardDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/LeopardDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/LeopardDecorator.rsrc

$(BUILD)/LeopardControlLook.o: src/leopard/LeopardControlLook.cpp src/leopard/LeopardControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/LeopardControlLook.rsrc: src/leopard/LeopardControlLook.rdef

$(LEOPARD_LOOK): $(BUILD)/LeopardControlLook.o $(BUILD)/LeopardControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/LeopardControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/LeopardControlLook.rsrc

# Gershwix, after Mintar's Platinum theme for Mac OS X.
$(BUILD)/GershwixDecorator.o: src/gershwix/GershwixDecorator.cpp src/gershwix/GershwixDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/GershwixDecorator.rsrc: src/gershwix/resources.rdef

$(GERSHWIX_DECOR): $(BUILD)/GershwixDecorator.o $(BUILD)/GershwixDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/GershwixDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/GershwixDecorator.rsrc

$(BUILD)/GershwixControlLook.o: src/gershwix/GershwixControlLook.cpp src/gershwix/GershwixControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/GershwixControlLook.rsrc: src/gershwix/GershwixControlLook.rdef

$(GERSHWIX_LOOK): $(BUILD)/GershwixControlLook.o $(BUILD)/GershwixControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/GershwixControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/GershwixControlLook.rsrc

# NeXT, after Swizcore Studio's NeXTSTEP theme for Mac OS X.
$(BUILD)/NextDecorator.o: src/next/NextDecorator.cpp src/next/NextDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/NextDecorator.rsrc: src/next/resources.rdef

$(NEXT_DECOR): $(BUILD)/NextDecorator.o $(BUILD)/NextDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/NextDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/NextDecorator.rsrc

$(BUILD)/NextControlLook.o: src/next/NextControlLook.cpp src/next/NextControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/NextControlLook.rsrc: src/next/NextControlLook.rdef

$(NEXT_LOOK): $(BUILD)/NextControlLook.o $(BUILD)/NextControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/NextControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/NextControlLook.rsrc

# Haiku's Flat decorator and control look, for the Flat themes (Renku's
# repositories don't have the haiku_extras package they come in).
$(BUILD)/FlatDecorator.o: $(FLAT)/decorator/FlatDecorator.cpp $(FLAT)/decorator/FlatDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

$(BUILD)/FlatDecorator.rsrc: $(FLAT)/decorator/resources.rdef

$(FLAT_DECOR): $(BUILD)/FlatDecorator.o $(BUILD)/FlatDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/FlatDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/FlatDecorator.rsrc

$(BUILD)/FlatControlLook.o: $(FLAT)/controllook/FlatControlLook.cpp $(FLAT)/controllook/FlatControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-variable -I$(PRIVATE)/interface -c $< -o $@

$(BUILD)/FlatControlLook.rsrc: $(FLAT)/controllook/FlatControlLook.rdef

$(FLAT_LOOK): $(BUILD)/FlatControlLook.o $(BUILD)/FlatControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/FlatControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/FlatControlLook.rsrc

# Power Theme, the theme manager: an ordinary application, using a few private
# interface kit headers (decorators, control looks, system fonts).
THEMES_SOURCES := $(wildcard src/themes/*.cpp)
THEMES_OBJECTS := $(patsubst src/themes/%.cpp,$(BUILD)/themes/%.o,$(THEMES_SOURCES))

$(BUILD)/themes/%.o: src/themes/%.cpp $(wildcard src/themes/*.h)
	@mkdir -p $(BUILD)/themes
	$(CXX) $(CXXFLAGS) -std=gnu++17 -I$(PRIVATE)/interface -I$(PRIVATE)/shared \
		-c $< -o $@

$(BUILD)/Themes.rsrc: src/themes/Themes.rdef

$(THEMES): $(THEMES_OBJECTS) $(BUILD)/Themes.rsrc
	$(CXX) $(SYSROOT_FLAGS) -s -o $@ $(THEMES_OBJECTS) -lbe -ltranslation \
		-lstdc++
	$(XRES) -o $@ $(BUILD)/Themes.rsrc
	mimeset -f $@ 2>/dev/null || true

$(BUILD)/ReactionaryDecorator.o: src/decorator/ReactionaryDecorator.cpp src/decorator/ReactionaryDecorator.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(DECOR_INCLUDES) -c $< -o $@

# Resources are compiled by Haiku's rc; when cross-building, RC_HOST names a
# Haiku/Renku machine reachable over ssh (rc reads stdin, writes the file).
RC_HOST ?= renku-test
$(BUILD)/%.rsrc:
	@mkdir -p $(BUILD)
ifeq ($(shell uname -s),Haiku)
	$(RC) -o $@ $<
else
	ssh $(RC_HOST) 'cat > /tmp/powertheme.rdef && rc -o /tmp/powertheme.rsrc /tmp/powertheme.rdef && cat /tmp/powertheme.rsrc' < $< > $@
endif

$(BUILD)/ReactionaryDecorator.rsrc: src/decorator/resources.rdef
$(BUILD)/ReactionaryControlLook.rsrc: src/controllook/ReactionaryControlLook.rdef

# Strip at link time: xres appends after the ELF data.
$(DECOR): $(BUILD)/ReactionaryDecorator.o $(BUILD)/ReactionaryDecorator.rsrc
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/ReactionaryDecorator.o \
		-lbe $(APP_SERVER) -lstdc++
	$(XRES) -o $@ $(BUILD)/ReactionaryDecorator.rsrc

# The control look runs inside every application, against libbe only.
$(BUILD)/ReactionaryControlLook.o: src/controllook/ReactionaryControlLook.cpp src/controllook/ReactionaryControlLook.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(PRIVATE)/interface -c $< -o $@

$(LOOK): $(BUILD)/ReactionaryControlLook.o $(BUILD)/ReactionaryControlLook.rsrc
	@mkdir -p $(dir $@)
	$(CXX) $(SYSROOT_FLAGS) -shared -s -o $@ $(BUILD)/ReactionaryControlLook.o \
		-lbe -lstdc++
	$(XRES) -o $@ $(BUILD)/ReactionaryControlLook.rsrc

# A window with every kind of control, for checking the control look.
demo: $(BUILD)/controls-demo
$(BUILD)/controls-demo: tools/controls-demo.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -s -o $@ $< -lbe -lstdc++

# Checks the icon encoder against Haiku's icon renderer.
icon-test: $(BUILD)/icon-test
$(BUILD)/icon-test: tools/icon-test.cpp src/themes/HVIFEncoder.cpp src/themes/HVIFEncoder.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=gnu++17 -o $@ tools/icon-test.cpp \
		src/themes/HVIFEncoder.cpp -lbe -ltranslation -lstdc++

clean:
	rm -rf $(BUILD)/*.o $(BUILD)/*.rsrc $(BUILD)/themes $(DECOR) $(LOOK) \
		$(THEMES) $(BUILD)/icon-test $(FLAT_DECOR) $(FLAT_LOOK) \
		$(SMOKE_DECOR) $(SMOKE_LOOK) $(GONX_LOOK)
	rm -rf $(DECOR_SOURCE)

.PHONY: all clean demo icon-test decorator-source

endif	# COSMOE_ROOT
