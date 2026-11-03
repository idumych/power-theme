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
THEMES := $(BUILD)/Themes
LOOK := $(BUILD)/control_look/Reactionary

FLAT_DECOR := $(BUILD)/FlatDecorator
FLAT_LOOK := $(BUILD)/control_look/FlatControlLook
FLAT := src/third_party/haiku-flat

SMOKE_DECOR := $(BUILD)/Smoke
SMOKE_LOOK := $(BUILD)/control_look/Smoke

all: $(DECOR) $(LOOK) $(THEMES) $(FLAT_DECOR) $(FLAT_LOOK) $(SMOKE_DECOR) \
	$(SMOKE_LOOK)

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

# Themes, the theme manager: an ordinary application, using a few private
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
	ssh $(RC_HOST) 'cat > /tmp/reactionary.rdef && rc -o /tmp/reactionary.rsrc /tmp/reactionary.rdef && cat /tmp/reactionary.rsrc' < $< > $@
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
		$(SMOKE_DECOR) $(SMOKE_LOOK)

.PHONY: all clean demo icon-test
