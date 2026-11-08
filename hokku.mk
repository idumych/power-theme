# Power Theme for Hokku, the Haiku API on Linux (github.com/idumych/hokku).
# Makefile includes this when COSMOE_ROOT names Hokku's install:
#
#   make COSMOE_ROOT=/usr
#   make COSMOE_ROOT=/usr install [DESTDIR=...] [PREFIX=/usr]
#
# The window decorators and control looks aren't built here. As on Renku,
# Power Theme builds them on the user's system (build-decorators.sh), against
# the Hokku there: a decorator has to match its libdecorator.

BUILD := build/hokku
APP := $(BUILD)/PowerTheme
SOURCE := $(BUILD)/data/PowerTheme
PREFIX ?= /usr

COSMOE_DIRS := config os os/app os/interface os/kernel os/storage os/support \
	os/translation os/locale os/add-ons/graphics os/be_apps/Tracker private \
	private/interface private/shared private/app private/system \
	private/tracker private/storage posix
override CXXFLAGS += -O2 -Wall -Wno-multichar -fPIC -std=gnu++17 \
	-DAPP_COSMOE -I$(COSMOE_ROOT)/include \
	$(addprefix -I$(COSMOE_ROOT)/include/,$(COSMOE_DIRS))
LIBS := -L$(COSMOE_ROOT)/lib -lbe
RC := LD_LIBRARY_PATH=$(COSMOE_ROOT)/lib $(COSMOE_ROOT)/bin/rc

SOURCES := $(wildcard src/themes/*.cpp)
OBJECTS := $(patsubst src/themes/%.cpp,$(BUILD)/themes/%.o,$(SOURCES))

all: $(APP) decorator-source

$(BUILD)/themes/%.o: src/themes/%.cpp $(wildcard src/themes/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Resources are a section of the program on Hokku
$(APP): $(OBJECTS) src/themes/Themes.rdef
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
	$(RC) -o $(BUILD)/Themes.rsrc src/themes/Themes.rdef
	objcopy --add-section .cosmoe_rsrc=$(BUILD)/Themes.rsrc \
		--set-section-flags .cosmoe_rsrc=readonly,data $@

# The add-ons' sources, as build-decorators.sh finds them next to it
decorator-source:
	rm -rf $(SOURCE)
	mkdir -p $(SOURCE)/source/src
	cp src/themes/build-decorators.sh $(SOURCE)/
	cp -R src/decorator src/controllook src/smoke src/dano src/tidy src/leopard src/gershwix src/next src/system7 src/suma \
		src/third_party $(SOURCE)/source/src/
	cp LICENSE $(SOURCE)/source/

DATA := $(DESTDIR)$(PREFIX)/share/cosmoe

install: all
	install -Dm755 $(APP) $(DESTDIR)$(PREFIX)/bin/PowerTheme
	ln -sf PowerTheme $(DESTDIR)$(PREFIX)/bin/powertheme
	mkdir -p $(DATA)/PowerTheme
	cp -R data/themes data/icons $(DATA)/
	cp -R data/PowerTheme/. $(SOURCE)/. $(DATA)/PowerTheme/
	install -Dm644 packaging/hokku/powertheme.desktop \
		$(DESTDIR)$(PREFIX)/share/applications/powertheme.desktop

clean:
	rm -rf $(BUILD)

.PHONY: all decorator-source install clean
