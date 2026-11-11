/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Theme files, and applying and reverting everything but icons. See Theme.h.
 */


#include "Theme.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef APP_COSMOE
#	include <signal.h>
#	include <sys/file.h>
#	include <sys/wait.h>
#endif

#include <Application.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Menu.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <StringList.h>
#include <Window.h>
#include <fs_attr.h>
#include <be_apps/Tracker/Background.h>

#include <new>
#include <vector>

#include <DecorInfo.h>
#include <DecoratorPrivate.h>
#include <InterfacePrivate.h>

#include "IconInstaller.h"
#include "IconTheme.h"


// private font API, as used by Appearance
extern void _set_system_font_(const char* which, font_family family,
	font_style style, float size);


static const char* kPreviousName = "previous";
static const char* kTrackerSignature = "application/x-vnd.Be-TRAK";
static const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
static const char* kBackgroundImageSet = "be:bgndimginfoset";
static const char* kCurrentName = "current";
static const char* kThemeIconsName = "theme icons";
static const char* kKeepIcons = "keep";
// how long Tracker gets to hear of new icons
static const bigtime_t kIconsSettleTime = 500000;

// Every system colour. Not all values in between are colours, and
// ui_color_name() complains about the others.
static const color_which kColorWhich[] = {
	B_PANEL_BACKGROUND_COLOR, B_MENU_BACKGROUND_COLOR, B_WINDOW_TAB_COLOR,
	B_KEYBOARD_NAVIGATION_COLOR, B_DESKTOP_COLOR,
	B_MENU_SELECTED_BACKGROUND_COLOR, B_MENU_ITEM_TEXT_COLOR,
	B_MENU_SELECTED_ITEM_TEXT_COLOR, B_MENU_SELECTED_BORDER_COLOR,
	B_PANEL_TEXT_COLOR, B_DOCUMENT_BACKGROUND_COLOR, B_DOCUMENT_TEXT_COLOR,
	B_CONTROL_BACKGROUND_COLOR, B_CONTROL_TEXT_COLOR, B_CONTROL_BORDER_COLOR,
	B_CONTROL_HIGHLIGHT_COLOR, B_NAVIGATION_PULSE_COLOR, B_SHINE_COLOR,
	B_SHADOW_COLOR, B_TOOL_TIP_BACKGROUND_COLOR, B_TOOL_TIP_TEXT_COLOR,
	B_WINDOW_TEXT_COLOR, B_WINDOW_INACTIVE_TAB_COLOR,
	B_WINDOW_INACTIVE_TEXT_COLOR, B_WINDOW_BORDER_COLOR,
	B_WINDOW_INACTIVE_BORDER_COLOR, B_CONTROL_MARK_COLOR,
	B_LIST_BACKGROUND_COLOR, B_LIST_SELECTED_BACKGROUND_COLOR,
	B_LIST_ITEM_TEXT_COLOR, B_LIST_SELECTED_ITEM_TEXT_COLOR,
	B_SCROLL_BAR_THUMB_COLOR, B_LINK_TEXT_COLOR, B_LINK_HOVER_COLOR,
	B_LINK_VISITED_COLOR, B_LINK_ACTIVE_COLOR, B_STATUS_BAR_COLOR,
	B_SUCCESS_COLOR, B_FAILURE_COLOR
};

// constant names whose colour goes by another name in the colour settings
static const struct {
	const char*	constant;
	const char*	name;
} kColorAliases[] = {
	{ "B_NAVIGATION_BASE_COLOR", "B_KEYBOARD_NAVIGATION_COLOR" },
	{ "B_TOOL_TIP_BACKGROUND_COLOR", "B_TOOLTIP_BACKGROUND_COLOR" },
	{ "B_TOOL_TIP_TEXT_COLOR", "B_TOOLTIP_TEXT_COLOR" },
	{ "B_MENU_SELECTION_BACKGROUND_COLOR", "B_MENU_SELECTED_BACKGROUND_COLOR" }
};


//	#pragma mark - helpers


/*!	The name the colour settings use for \a name, which may be either that
	or the constant's name; empty if it is no system colour. (Haiku's own
	which_ui_color() can crash on names it doesn't know.)
*/
static BString
color_name(const BString& name)
{
	for (size_t i = 0; i < sizeof(kColorAliases) / sizeof(kColorAliases[0]);
			i++) {
		if (name == kColorAliases[i].constant)
			return kColorAliases[i].name;
	}
	for (size_t i = 0; i < sizeof(kColorWhich) / sizeof(kColorWhich[0]); i++) {
		const char* known = ui_color_name(kColorWhich[i]);
		if (known != NULL && name == known)
			return name;
	}
	return "";
}


static BString
trimmed(const BString& text)
{
	BString result(text);
	result.Trim();
	return result;
}


bool
is_haiku(const char* value)
{
	return value != NULL && strcasecmp(value, "haiku") == 0;
}


const char* const kDarkHaikuIcons = "haiku-dark";


bool
is_dark_haiku(const char* value)
{
	return value != NULL && strcasecmp(value, kDarkHaikuIcons) == 0;
}


bool
parse_color(const char* text, rgb_color& color)
{
	BString value = trimmed(text);
	if (value.StartsWith("#"))
		value.Remove(0, 1);
	if (value.Length() != 6 && value.Length() != 8)
		return false;
	char* end;
	unsigned long number = strtoul(value.String(), &end, 16);
	if (*end != '\0')
		return false;
	if (value.Length() == 6)
		number = number << 8 | 0xff;
	color.red = (number >> 24) & 0xff;
	color.green = (number >> 16) & 0xff;
	color.blue = (number >> 8) & 0xff;
	color.alpha = number & 0xff;
	return true;
}


status_t
themes_settings_path(BPath& path, const char* leaf)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status != B_OK)
		return status;
	// Power Theme was called Themes; its folder moves with the name
	BPath old(path.Path(), "Themes");
	path.Append("PowerTheme");
	if (!BEntry(path.Path()).Exists() && BEntry(old.Path()).Exists())
		rename(old.Path(), path.Path());
	create_directory(path.Path(), 0755);
	if (leaf != NULL)
		path.Append(leaf);
	return B_OK;
}


void
data_directories(BStringList& directories)
{
	static const directory_which kDirectories[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(kDirectories) / sizeof(kDirectories[0]);
			i++) {
		BPath path;
		if (find_directory(kDirectories[i], &path) == B_OK)
			directories.Add(path.Path());
	}
}


static status_t
load_message(const char* leaf, BMessage& message)
{
	BPath path;
	status_t status = themes_settings_path(path, leaf);
	if (status != B_OK)
		return status;
	BFile file(path.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return message.Unflatten(&file);
}


static status_t
save_message(const char* leaf, const BMessage& message)
{
	BPath path;
	status_t status = themes_settings_path(path, leaf);
	if (status != B_OK)
		return status;
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return message.Flatten(&file);
}


void
set_current(const char* theme, const char* icons)
{
	BMessage current;
	load_message(kCurrentName, current);
	if (theme != NULL) {
		current.RemoveName("theme");
		current.AddString("theme", theme);
	}
	if (icons != NULL) {
		current.RemoveName("icons");
		current.AddString("icons", icons);
	}
	save_message(kCurrentName, current);
}


void
get_current(BString& theme, BString& icons)
{
	BMessage current;
	load_message(kCurrentName, current);
	theme = current.GetString("theme", "");
	icons = current.GetString("icons", "");
}


static void
set_current_parts(uint32 parts)
{
	BMessage current;
	load_message(kCurrentName, current);
	current.RemoveName("parts");
	current.AddUInt32("parts", parts);
	save_message(kCurrentName, current);
}


static uint32
get_current_parts()
{
	BMessage current;
	load_message(kCurrentName, current);
	return current.GetUInt32("parts", THEME_ALL & ~THEME_WALLPAPER);
}


bool
icons_follow_mode()
{
	BMessage current;
	load_message(kCurrentName, current);
	return current.GetBool("icons follow mode", false);
}


void
set_icons_follow_mode(bool follow)
{
	BMessage current;
	load_message(kCurrentName, current);
	current.RemoveName("icons follow mode");
	current.AddBool("icons follow mode", follow);
	save_message(kCurrentName, current);
}


BString
theme_icons(const Theme& theme)
{
	BMessage chosen;
	load_message(kThemeIconsName, chosen);
	const char* icons;
	if (chosen.FindString(theme.Name(), &icons) == B_OK)
		return icons;
	return theme.Icons().IsEmpty() ? BString(kKeepIcons) : theme.Icons();
}


void
set_theme_icons(const Theme& theme, const char* icons)
{
	BMessage chosen;
	load_message(kThemeIconsName, chosen);
	chosen.RemoveName(theme.Name());
	chosen.AddString(theme.Name(), icons);
	save_message(kThemeIconsName, chosen);
}


status_t
find_build_script(BString& path)
{
	BStringList directories;
	data_directories(directories);
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		BPath script(directories.StringAt(i), "PowerTheme/build-decorators.sh");
		if (BEntry(script.Path()).Exists()) {
			path = script.Path();
			return B_OK;
		}
	}

	// beside the application, as in a source tree's build folder
	app_info info;
	if (be_app != NULL && be_app->GetAppInfo(&info) == B_OK) {
		BPath folder(&info.ref);
		if (folder.GetParent(&folder) == B_OK) {
			const char* candidates[] = { "build-decorators.sh",
				"../src/themes/build-decorators.sh" };
			for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]);
					i++) {
				BPath script(folder.Path(), candidates[i], true);
				if (BEntry(script.Path()).Exists()) {
					path = script.Path();
					return B_OK;
				}
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}


// build-decorators.sh writes the system revision it built for here, and the
// size and modification time of the app_server and libbe it built against
// in the other
static const char* kAddOnsBuiltName = "addons built for";
static const char* kAddOnsBuiltAgainstName = "addons built against";


/*!	Whether the system's app_server and libbe are the ones the add-ons were
	built against, as they check when they load (AddOnGuard.cpp); true if
	that wasn't noted.
*/
static bool
same_system_build()
{
#ifdef APP_COSMOE
	return true;
#else
	BPath path;
	if (themes_settings_path(path, kAddOnsBuiltAgainstName) != B_OK)
		return true;
	FILE* file = fopen(path.Path(), "r");
	if (file == NULL)
		return true;
	static const char* kFiles[] = { "/boot/system/servers/app_server",
		"/boot/system/lib/libbe.so" };
	bool same = true;
	for (size_t i = 0; i < sizeof(kFiles) / sizeof(kFiles[0]) && same; i++) {
		char line[64];
		struct stat st;
		if (fgets(line, sizeof(line), file) == NULL
			|| stat(kFiles[i], &st) != 0)
			break;
		BString running;
		running << (int64)st.st_size << " " << (int64)st.st_mtime;
		same = trimmed(line) == running;
	}
	fclose(file);
	return same;
#endif
}


/*!	The running system's revision, as build-decorators.sh reads it from the
	name of the haiku package (haiku-r1~beta6_hrev60072_640-1-x86_64.hpkg is
	hrev60072+640), or "" if it can't be told.
*/
static BString
system_revision()
{
	BString revision;
#ifndef APP_COSMOE
	BDirectory packages("/boot/system/packages");
	BStringList names;
	BEntry entry;
	while (packages.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK && strncmp(name, "haiku-r", 7) == 0
			&& BString(name).EndsWith(".hpkg"))
			names.Add(name);
	}
	// the first, as the script takes it
	names.Sort();
	if (names.IsEmpty())
		return revision;

	const char* hrev = strstr(names.StringAt(0).String(), "_hrev");
	if (hrev == NULL)
		return revision;
	char* end;
	const unsigned long number = strtoul(hrev + 5, &end, 10);
	if (end == hrev + 5)
		return revision;
	revision << "hrev" << (uint32)number;
	if (*end == '_') {
		char* aheadEnd;
		const unsigned long ahead = strtoul(end + 1, &aheadEnd, 10);
		if (aheadEnd != end + 1 && *aheadEnd == '-')
			revision << "+" << (uint32)ahead;
	}
#endif
	return revision;
}


/*!	Whether every window decorator and control look that Build add-ons makes
	is installed. Others that themes name, such as Dano's BeDecorator, don't
	count: building can't make them.
*/
static bool
buildable_addons_installed()
{
#ifdef APP_COSMOE
	// Hokku's add-ons have folders of their own: go by the themes
	ThemeList themes;
	find_themes(themes);
	for (int32 i = 0; i < themes.CountItems(); i++) {
		if (themes.ItemAt(i)->Missing(NULL, THEME_DECORATOR | THEME_CONTROLS)
				!= 0)
			return false;
	}
	return true;
#else
	// what can be built is what there is source for, next to the script
	BString script;
	BPath source;
	if (find_build_script(script) != B_OK
		|| BPath(script.String()).GetParent(&source) != B_OK
		|| source.Append("source") != B_OK)
		return true;

	static const directory_which kAddOnDirectories[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};
	// the source's folder and the add-ons' folder for each kind
	static const char* kKinds[][2] = {
		{ "decorators", "decorators" },
		{ "control_looks", "control_look" }
	};
	for (size_t k = 0; k < sizeof(kKinds) / sizeof(kKinds[0]); k++) {
		BDirectory folder(BPath(source.Path(), kKinds[k][0]).Path());
		BEntry entry;
		while (folder.GetNextEntry(&entry) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			if (!entry.IsDirectory() || entry.GetName(name) != B_OK)
				continue;
			bool installed = false;
			for (size_t d = 0; d < sizeof(kAddOnDirectories)
					/ sizeof(kAddOnDirectories[0]) && !installed; d++) {
				BPath path;
				if (find_directory(kAddOnDirectories[d], &path) == B_OK
					&& path.Append(kKinds[k][1]) == B_OK
					&& path.Append(name) == B_OK)
					installed = BEntry(path.Path()).Exists();
			}
			if (!installed)
				return false;
		}
	}
	return true;
#endif
}


bool
addons_need_building(BString* why)
{
	BString built;
	BPath path;
	if (themes_settings_path(path, kAddOnsBuiltName) == B_OK) {
		BFile file(path.Path(), B_READ_ONLY);
		char buffer[64];
		const ssize_t bytes = file.InitCheck() == B_OK
			? file.Read(buffer, sizeof(buffer) - 1) : 0;
		if (bytes > 0) {
			buffer[bytes] = '\0';
			built = trimmed(buffer);
		}
	}

	if (!built.IsEmpty()) {
		const BString running = system_revision();
		if ((running.IsEmpty() || running == built) && same_system_build())
			return false;
		if (running.IsEmpty() || running == built) {
			if (why != NULL) {
				*why = "The window borders and controls were built for "
					"another build of this system.";
			}
			return true;
		}
		if (why != NULL) {
			*why = BString("The window borders and controls were built for ")
				<< built << ", and this system is " << running << ".";
		}
		return true;
	}

	// never built here: fine as long as every one Power Theme can build is
	// installed, as when they come with the system
	if (!buildable_addons_installed()) {
		if (why != NULL) {
			*why = "The window borders and controls haven't been built for "
				"this system yet.";
		}
		return true;
	}
	return false;
}


bool
addons_built_before()
{
	BPath path;
	return themes_settings_path(path, kAddOnsBuiltName) == B_OK
		&& BEntry(path.Path()).Exists();
}


/*!	Whether a theme can be applied: not before the window decorators and
	control looks are built for this system, which \a log then says.
*/
static bool
can_apply(BString& log)
{
	BString why;
	if (!addons_need_building(&why))
		return true;
	log << why << " Build them first, with Build add-ons" B_UTF8_ELLIPSIS
		" in Power Theme or powertheme --build-decorators.\n";
	return false;
}


//!	A function of libbe's that not every system has, or NULL.
static void*
libbe_function(const char* symbol)
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		const char* leaf = strrchr(info.name, '/');
		if (strcmp(leaf != NULL ? leaf + 1 : info.name, "libbe.so") != 0)
			continue;

		void* function;
		if (get_image_symbol(info.id, symbol, B_SYMBOL_TYPE_TEXT, &function)
				== B_OK)
			return function;
		return NULL;
	}
	return NULL;
}


//! Whether the running system's libbe has the given function.
static bool
libbe_has(const char* symbol)
{
	return libbe_function(symbol) != NULL;
}


const char*
running_applications_note()
{
#ifdef APP_COSMOE
	// Hokku: each application draws its own windows and loads its control
	// look when it starts
	return "Running applications take the new colours and fonts; restart them "
		"to see the new window borders and controls.";
#endif
	// BPrivate::update_control_look() and BPrivate::update_system_fonts().
	// If these ever change, the note only asks for a restart that's not needed.
	const bool controls = libbe_has("_ZN8BPrivate19update_control_lookEv");
	const bool fonts = libbe_has("_ZN8BPrivate19update_system_fontsEv");

	if (controls && fonts) {
		return "Running applications change along, though some keep fonts or "
			"sizes of their own until they restart.";
	}
	if (controls)
		return "Restart running applications to see the new fonts in them.";
	if (fonts)
		return "Restart running applications to see the new controls in them.";
	return "Restart running applications to see the new controls and fonts in "
		"them.";
}


status_t
restart_tracker()
{
	BMessenger tracker(kTrackerSignature);
	if (tracker.IsValid()) {
		tracker.SendMessage(B_QUIT_REQUESTED);
		for (int32 i = 0; i < 50 && be_roster->IsRunning(kTrackerSignature);
				i++)
			snooze(100000);
	}
	if (be_roster->IsRunning(kTrackerSignature))
		return B_OK;

	// Started on its own, in a session of its own: launched from here it
	// would share a Terminal's output and go when the Terminal closes.
	entry_ref ref;
	status_t status = be_roster->FindApp(kTrackerSignature, &ref);
	if (status != B_OK)
		return status;
	BPath path(&ref);
	pid_t child = fork();
	if (child == 0) {
		setsid();
		int null = open("/dev/null", O_RDWR);
		if (null >= 0) {
			dup2(null, 0);
			dup2(null, 1);
			dup2(null, 2);
			if (null > 2)
				close(null);
		}
		execl(path.Path(), path.Path(), (char*)NULL);
		_exit(1);
	}
	return child < 0 ? errno : B_OK;
}


//	#pragma mark - Theme


Theme::Theme()
	:
	fMode(THEME_MODE_UNKNOWN),
	fWallpaperMode(B_BACKGROUND_MODE_SCALED),
	fDeskbarLocation(-1),
	fDeskbarExpanded(true),
	fScrollBarArrows(0),
	fGreyFontSmoothing(false),
	fShell(-1),
	fUITheme(false),
	fHasDesktopColor(false)
{
}


theme_mode
Theme::Mode() const
{
	if (fMode != THEME_MODE_UNKNOWN)
		return fMode;

	rgb_color panel, text;
	if (fColors.FindColor(ui_color_name(B_PANEL_BACKGROUND_COLOR), &panel)
			!= B_OK
		|| fColors.FindColor(ui_color_name(B_PANEL_TEXT_COLOR), &text)
			!= B_OK) {
		return THEME_MODE_UNKNOWN;
	}
	return text.Brightness() > panel.Brightness()
		? THEME_MODE_DARK : THEME_MODE_LIGHT;
}


status_t
Theme::Load(const char* folder)
{
	fFolder = folder;
	BPath path(folder, "theme.settings");
	BFile file(path.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK)
		return status;
	if (size > 256 * 1024)
		return B_BAD_DATA;

	BString text;
	char* buffer = text.LockBuffer(size + 1);
	ssize_t bytesRead = file.Read(buffer, size);
	text.UnlockBuffer(bytesRead > 0 ? bytesRead : 0);

	fName = BPath(folder).Leaf();

	BStringList lines;
	text.Split("\n", false, lines);
	for (int32 i = 0; i < lines.CountStrings(); i++) {
		BString line = trimmed(lines.StringAt(i));
		if (line.IsEmpty() || line.StartsWith("#"))
			continue;
		int32 equals = line.FindFirst('=');
		if (equals < 0)
			continue;
		BString key, value;
		line.CopyInto(key, 0, equals);
		line.CopyInto(value, equals + 1, line.Length() - equals - 1);
		key = trimmed(key);
		value = trimmed(value);
		key.ToLower();

		if (key == "name")
			fName = value;
		else if (key == "variant of")
			fVariantOf = value;
		else if (key == "mode")
			fMode = mode_from_name(value);
		else if (key == "description")
			fDescription = value;
		else if (key == "decorator")
			fDecorator = value;
		else if (key == "control look")
			fControlLook = value;
		else if (key == "icons")
			fIcons = value;
		else if (key == "wallpaper") {
			if (value.ICompare("none") == 0 || value.ICompare("default") == 0)
				fWallpaper = value.ToLower();
			else if (value.StartsWith("/"))
				fWallpaper = value;
			else
				fWallpaper = BPath(folder, value).Path();
		} else if (key == "deskbar") {
			BStringList fields;
			value.Split(",", true, fields);
			BString place = trimmed(fields.StringAt(0)).ToLower();
			static const char* kPlaces[] = { "top", "bottom", "left top",
				"right top", "left bottom", "right bottom" };
			for (int32 p = 0; p < 6; p++) {
				if (place == kPlaces[p])
					fDeskbarLocation = p;
			}
			if (fields.CountStrings() > 1)
				fDeskbarExpanded = trimmed(fields.StringAt(1)).ICompare(
					"collapsed") != 0;
		} else if (key == "scroll bar arrows") {
			value.ToLower();
			if (value == "double")
				fScrollBarArrows = 2;
			else if (value == "single")
				fScrollBarArrows = 1;
		} else if (key == "font smoothing") {
			value.ToLower();
			fGreyFontSmoothing = value == "grey" || value == "gray"
				|| value == "greyscale" || value == "grayscale";
		} else if (key == "shell") {
			value.ToLower();
			if (value == "photon")
				fShell = 1;
			else if (value == "deskbar")
				fShell = 0;
		} else if (key == "wallpaper mode") {
			value.ToLower();
			if (value == "centered" || value == "centred")
				fWallpaperMode = B_BACKGROUND_MODE_CENTERED;
			else if (value == "tiled")
				fWallpaperMode = B_BACKGROUND_MODE_TILED;
			else if (value == "manual")
				fWallpaperMode = B_BACKGROUND_MODE_USE_ORIGIN;
			else
				fWallpaperMode = B_BACKGROUND_MODE_SCALED;
		}
		else if (key == "desktop color")
			fHasDesktopColor = parse_color(value, fDesktopColor);
		else if (key.StartsWith("color ")) {
			BString name = trimmed(key.String() + 6);
			name = color_name(name.ToUpper());
			rgb_color color;
			if (!name.IsEmpty() && parse_color(value, color))
				fColors.AddColor(name, color);
		} else if (key.StartsWith("font ")) {
			BStringList fields;
			value.Split(",", false, fields);
			if (fields.CountStrings() != 3)
				continue;
			ThemeFont* font = new ThemeFont;
			font->which = trimmed(key.String() + 5);
			font->family = trimmed(fields.StringAt(0));
			font->style = trimmed(fields.StringAt(1));
			font->size = atof(fields.StringAt(2));
			if (font->size < 4 || font->size > 100
				|| (font->which != "plain" && font->which != "bold"
					&& font->which != "fixed" && font->which != "menu")) {
				delete font;
				continue;
			}
			fFonts.AddItem(font);
		}
	}
	return B_OK;
}


BString
Theme::PreviewPath() const
{
	if (!fPreview.IsEmpty())
		return fPreview;
	BPath path(fFolder, "preview.png");
	return path.Path();
}


uint32
Theme::Parts() const
{
	uint32 parts = 0;
	if (!fDecorator.IsEmpty())
		parts |= THEME_DECORATOR;
	if (!fControlLook.IsEmpty())
		parts |= THEME_CONTROLS;
	if (fHasDesktopColor || !fColors.IsEmpty())
		parts |= THEME_COLORS;
	if (!fFonts.IsEmpty())
		parts |= THEME_FONTS;
	if (!fIcons.IsEmpty())
		parts |= THEME_ICONS;
	if (HasWallpaper())
		parts |= THEME_WALLPAPER;
#ifdef APP_COSMOE
	if (fDeskbarLocation >= 0)
		parts |= THEME_DESKBAR;
#else
	// every theme chooses between the Deskbar and the Photon shell, even one
	// that doesn't say where the Deskbar goes
	parts |= THEME_DESKBAR;
#endif
	return parts;
}


static bool find_control_look(const char* name, BString& path);


/*!	The first installed decorator of \a names ("A | B | Default"), found by
	name, by path, or by its add-on's file name.
*/
static DecorInfo*
find_decorator(DecorInfoUtility& decorators, const BString& names)
{
	BStringList alternatives;
	names.Split("|", true, alternatives);
	for (int32 i = 0; i < alternatives.CountStrings(); i++) {
		BString name = trimmed(alternatives.StringAt(i));
#ifdef APP_COSMOE
		// Hokku's "Default" is the Linux desktop's own frame; Haiku's is
		// called Haiku there
		if (name == "Default")
			name = "Haiku";
#endif
		DecorInfo* decor = decorators.FindDecorator(name);
		if (decor != NULL)
			return decor;
		const BString leaf = BPath(name).Leaf() != NULL
			? BPath(name).Leaf() : name.String();
		for (int32 d = 0; d < decorators.CountDecorators(); d++) {
			decor = decorators.DecoratorAt(d);
			if (decor == NULL || decor->Ref() == NULL)
				continue;
			BPath path(decor->Ref());
			if (path.Leaf() != NULL && leaf.ICompare(path.Leaf()) == 0)
				return decor;
		}
	}
	return NULL;
}


/*!	The first installed control look of \a names; "haiku" is the built-in
	one, an empty path.
*/
static bool
find_control_look_of(const BString& names, BString& path)
{
	BStringList alternatives;
	names.Split("|", true, alternatives);
	for (int32 i = 0; i < alternatives.CountStrings(); i++) {
		BString name = trimmed(alternatives.StringAt(i));
		if (is_haiku(name)) {
			path = "";
			return true;
		}
		if (find_control_look(name, path))
			return true;
	}
	return false;
}


uint32
Theme::Missing(BStringList* what, uint32 parts) const
{
	uint32 missing = 0;
	if ((parts & THEME_DECORATOR) != 0 && !fDecorator.IsEmpty()) {
		DecorInfoUtility decorators;
		if (find_decorator(decorators, fDecorator) == NULL) {
			missing |= THEME_DECORATOR;
			if (what != NULL) {
				what->Add(BString("the window decorator \"") << fDecorator
					<< "\"");
			}
		}
	}

	BString path;
	if ((parts & THEME_CONTROLS) != 0 && !fControlLook.IsEmpty()
		&& !find_control_look_of(fControlLook, path)) {
		missing |= THEME_CONTROLS;
		if (what != NULL)
			what->Add(BString("the control look \"") << fControlLook << "\"");
	}

	if ((parts & THEME_ICONS) != 0 && !fIcons.IsEmpty() && !is_haiku(fIcons)
		&& !is_dark_haiku(fIcons)) {
		IconTheme* icons = load_icon_theme(fIcons);
		if (icons == NULL) {
			missing |= THEME_ICONS;
			if (what != NULL)
				what->Add(BString("the icon theme \"") << fIcons << "\"");
		}
		delete icons;
	}

	if ((parts & THEME_WALLPAPER) != 0 && fBackground.IsEmpty()
		&& !fWallpaper.IsEmpty() && fWallpaper != "none"
		&& fWallpaper != "default" && !BEntry(fWallpaper).Exists()) {
		missing |= THEME_WALLPAPER;
		if (what != NULL) {
			what->Add(BString("the wallpaper \"") << BPath(fWallpaper).Leaf()
				<< "\"");
		}
	}
	return missing;
}


void
find_themes(ThemeList& themes)
{
	BStringList directories;
	data_directories(directories);

	BStringList seen;
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		BPath root(directories.StringAt(i), "themes");
		BDirectory directory(root.Path());
		BEntry entry;
		while (directory.GetNextEntry(&entry, true) == B_OK) {
			if (!entry.IsDirectory())
				continue;
			BPath path;
			entry.GetPath(&path);
			Theme* theme = new Theme;
			if (theme->Load(path.Path()) != B_OK
				|| seen.HasString(theme->Name())) {
				delete theme;
				continue;
			}
			seen.Add(theme->Name());
			themes.AddItem(theme);
		}
	}

	// Theme Manager's themes, from the folders it uses (user ones first,
	// so a user's copy of a theme wins)
	static const directory_which kUIThemeDirectories[] = {
		B_USER_SETTINGS_DIRECTORY,
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};
	BStringList seenUIThemes;
	for (size_t i = 0;
			i < sizeof(kUIThemeDirectories) / sizeof(kUIThemeDirectories[0]);
			i++) {
		BPath root;
		if (find_directory(kUIThemeDirectories[i], &root) != B_OK)
			continue;
		root.Append("UIThemes");
		BDirectory directory(root.Path());
		BEntry entry;
		while (directory.GetNextEntry(&entry, true) == B_OK) {
			BPath path;
			if (!entry.IsDirectory() || entry.GetPath(&path) != B_OK
				|| seenUIThemes.HasString(path.Leaf()))
				continue;
			Theme* theme = new Theme;
			if (theme->LoadUITheme(path.Path()) != B_OK) {
				delete theme;
				continue;
			}
			seenUIThemes.Add(path.Leaf());
			// themes of the same name are told apart
			if (seen.HasString(theme->Name())) {
				BString name(theme->Name());
				name << " (Theme Manager)";
				theme->SetName(name);
			}
			seen.Add(theme->Name());
			themes.AddItem(theme);
		}
	}

	themes.SortItems([](const Theme* a, const Theme* b) {
		return a->Name().ICompare(b->Name());
	});
}


//	#pragma mark - applying


static void
set_font(const char* which, const char* familyName, const char* styleName,
	float size)
{
	font_family family;
	font_style style;
	strlcpy(family, familyName, sizeof(family));
	strlcpy(style, styleName, sizeof(style));

	if (strcmp(which, "menu") == 0) {
		// the menu font is part of the menu settings, as in Appearance
		menu_info info;
		if (get_menu_info(&info) != B_OK)
			return;
		strlcpy(info.f_family, family, sizeof(info.f_family));
		strlcpy(info.f_style, style, sizeof(info.f_style));
		info.font_size = size;
		set_menu_info(&info);
	} else
		_set_system_font_(which, family, style, size);
}


static void
save_font(BMessage& message, const char* which, const BFont* font)
{
	font_family family;
	font_style style;
	font->GetFamilyAndStyle(&family, &style);

	BMessage saved;
	saved.AddString("which", which);
	saved.AddString("family", family);
	saved.AddString("style", style);
	saved.AddFloat("size", font->Size());
	message.AddMessage("font", &saved);
}


static bool
font_installed(const char* familyName, const char* styleName)
{
	int32 families = count_font_families();
	for (int32 i = 0; i < families; i++) {
		font_family family;
		if (get_font_family(i, &family) != B_OK
			|| strcmp(family, familyName) != 0)
			continue;
		int32 styles = count_font_styles(family);
		for (int32 j = 0; j < styles; j++) {
			font_style style;
			if (get_font_style(family, j, &style) == B_OK
				&& strcmp(style, styleName) == 0)
				return true;
		}
	}
	return false;
}


/*!	Finds a control look add-on by name in the add-on folders, user ones
	first.
*/
static bool
find_control_look(const char* name, BString& path)
{
	static const directory_which kDirectories[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(kDirectories) / sizeof(kDirectories[0]);
			i++) {
		BPath candidate;
		if (find_directory(kDirectories[i], &candidate) != B_OK)
			continue;
		candidate.Append("control_look");
		candidate.Append(name);
		if (BEntry(candidate.Path()).Exists()) {
			path = candidate.Path();
			return true;
		}
	}
	return false;
}


//	#pragma mark - wallpaper


/*!	The wallpaper is the Desktop folder's background, as the Backgrounds
	preferences sets it: a message in an attribute of the folder, read by
	Tracker when told to.
*/
static status_t
desktop_node(BNode& node)
{
	BPath path;
	status_t status = find_directory(B_DESKTOP_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	return node.SetTo(path.Path());
}


static void
tell_tracker_about_wallpaper()
{
	BMessenger(kTrackerSignature).SendMessage(B_RESTORE_BACKGROUND_IMAGE);
}


//! The wallpaper in use, as it is stored, or "no wallpaper".
static void
save_wallpaper(BMessage& look)
{
	BNode node;
	attr_info info;
	if (desktop_node(node) == B_OK
		&& node.GetAttrInfo(B_BACKGROUND_INFO, &info) == B_OK
		&& info.size > 0 && info.size < 256 * 1024) {
		std::vector<char> data(info.size);
		if (node.ReadAttr(B_BACKGROUND_INFO, info.type, 0, &data[0], info.size)
				== info.size) {
			look.AddData("wallpaper", B_RAW_TYPE, &data[0], info.size);
			return;
		}
	}
	look.AddBool("no wallpaper", true);
}


static void
restore_wallpaper(const BMessage& look)
{
	BNode node;
	if (desktop_node(node) != B_OK)
		return;

	const void* data;
	ssize_t size;
	if (look.FindData("wallpaper", B_RAW_TYPE, &data, &size) == B_OK)
		node.WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0, data, size);
	else if (look.GetBool("no wallpaper", false))
		node.RemoveAttr(B_BACKGROUND_INFO);
	else
		return;
	tell_tracker_about_wallpaper();
}


static status_t write_wallpaper(const BMessage& info, BString& log);


static status_t
set_wallpaper(const Theme& theme, BString& log)
{
	if (!theme.Background().IsEmpty())
		return write_wallpaper(theme.Background(), log);

	BString image = theme.Wallpaper();
	int32 mode = theme.WallpaperMode();
	BPoint offset(0, 0);

	if (image == "none")
		image = "";
	else if (image == "default") {
		// what Tracker sets up on a new system (TrackerInitialState.cpp)
		BPath artwork;
		find_directory(B_SYSTEM_DATA_DIRECTORY, &artwork);
		artwork.Append("artwork/HAIKU logo - white on blue - big.png");
		image = artwork.Path();
		mode = B_BACKGROUND_MODE_USE_ORIGIN;
		BRect screen = BScreen().Frame();
		offset.x = floorf((screen.Width() - 605) * (sqrtf(5) - 1) / 2);
		offset.y = floorf((screen.Height() - 190) * 0.9);
	} else if (!BEntry(image).Exists()) {
		log << "The wallpaper " << BPath(image).Leaf() << " isn't there.\n";
		return B_ENTRY_NOT_FOUND;
	}

	BMessage info;
	info.AddString(B_BACKGROUND_IMAGE, image);
	info.AddInt32(B_BACKGROUND_WORKSPACES, (int32)0xffffffff);
	info.AddInt32(B_BACKGROUND_MODE, mode);
	info.AddBool(B_BACKGROUND_ERASE_TEXT, false);
	info.AddPoint(B_BACKGROUND_ORIGIN, offset);
	info.AddInt32(kBackgroundImageSet, 0);
	return write_wallpaper(info, log);
}


static status_t
write_wallpaper(const BMessage& info, BString& log)
{
	ssize_t size = info.FlattenedSize();
	std::vector<char> data(size);
	status_t status = info.Flatten(&data[0], size);
	BNode node;
	if (status == B_OK)
		status = desktop_node(node);
	if (status == B_OK) {
		ssize_t written = node.WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0,
			&data[0], size);
		if (written < 0)
			status = written;
	}
	if (status != B_OK) {
		log << "Can't set the wallpaper: " << strerror(status) << ".\n";
		return status;
	}
	tell_tracker_about_wallpaper();
	return B_OK;
}


//	#pragma mark - Deskbar


/*!	How tall a window's tab is now, or -1. The Deskbar asks this once, as it
	starts, and sizes its logo's menu by the answer.
*/
static float
window_tab_height()
{
	BWindow* window = new(std::nothrow) BWindow(BRect(), NULL, B_TITLED_WINDOW,
		0);
	if (window == NULL)
		return -1;
	float height = -1;
	BMessage settings;
	BRect tab;
	if (window->GetDecoratorSettings(&settings) == B_OK
		&& settings.FindRect("tab frame", &tab) == B_OK)
		height = tab.Height();
	delete window;
	return height;
}


//! Whether the Deskbar is along the top or bottom, sized by its own contents.
static bool
deskbar_ignores_tab_height(BDeskbar& deskbar)
{
	bool expanded;
	const deskbar_location location = deskbar.Location(&expanded);
	return expanded
		&& (location == B_DESKBAR_TOP || location == B_DESKBAR_BOTTOM);
}


/*!	A Deskbar keeps the tab height it started with, so a shorter tab than the
	logo's height cuts the logo off, whichever look is in use later. Restarting
	it is the only way to tell it. One that didn't care about the height before
	may have an old one to get rid of, now it does. launch_daemon starts it
	again, as it's a service; asking the roster meanwhile can take half a minute.
*/
/*!	Asks the Deskbar to quit and waits for it; the registrar starts it again,
	laid out for the look in use.
*/
static void
restart_deskbar()
{
	const team_id team = be_roster->TeamFor(kDeskbarSignature);
	if (BMessenger(kDeskbarSignature).SendMessage(B_QUIT_REQUESTED) != B_OK)
		return;
	team_info info;
	for (int32 i = 0; i < 50 && get_team_info(team, &info) == B_OK; i++)
		snooze(100000);
}


static void
update_deskbar_for_tab_height(float tabHeight, bool ignoredTabHeight)
{
	BDeskbar deskbar;
	if (!deskbar.IsRunning() || deskbar_ignores_tab_height(deskbar))
		return;
	const float now = window_tab_height();
	if (tabHeight < 0 || now < 0 || (now == tabHeight && !ignoredTabHeight))
		return;

	restart_deskbar();
}


static void
save_scroll_bar_arrows(BMessage& look)
{
	scroll_bar_info info;
	if (get_scroll_bar_info(&info) == B_OK)
		look.AddBool("double arrows", info.double_arrows);
}


static void
set_scroll_bar_arrows(bool doubled)
{
	scroll_bar_info info;
	if (get_scroll_bar_info(&info) == B_OK && info.double_arrows != doubled) {
		info.double_arrows = doubled;
		set_scroll_bar_info(&info);
	}
}


// Appearance's font smoothing switch: libbe's, though not in its headers
typedef void (*set_subpixel_function)(bool);
typedef status_t (*get_subpixel_function)(bool*);
static const char* kSetSubpixel = "_Z25set_subpixel_antialiasingb";
static const char* kGetSubpixel = "_Z25get_subpixel_antialiasingPb";


static void
save_font_smoothing(BMessage& look)
{
	get_subpixel_function get
		= (get_subpixel_function)libbe_function(kGetSubpixel);
	bool subpixel;
	if (get != NULL && get(&subpixel) == B_OK)
		look.AddBool("subpixel smoothing", subpixel);
}


static void
set_font_smoothing(bool subpixel)
{
	get_subpixel_function get
		= (get_subpixel_function)libbe_function(kGetSubpixel);
	set_subpixel_function set
		= (set_subpixel_function)libbe_function(kSetSubpixel);
	bool current;
	if (get == NULL || set == NULL || get(&current) != B_OK
		|| current == subpixel)
		return;
	set(subpixel);
}


static void
save_deskbar(BMessage& look)
{
	BDeskbar deskbar;
	if (!deskbar.IsRunning())
		return;
	bool expanded = true;
	deskbar_location location = deskbar.Location(&expanded);
	look.AddInt32("deskbar location", (int32)location);
	look.AddBool("deskbar expanded", expanded);
}


static void
set_deskbar(int32 location, bool expanded, BString& log)
{
	if (location < B_DESKBAR_TOP || location > B_DESKBAR_RIGHT_BOTTOM)
		return;
	BDeskbar deskbar;
	if (!deskbar.IsRunning()) {
		log << "Deskbar isn't running, so it wasn't moved.\n";
		return;
	}
	status_t status = deskbar.SetLocation((deskbar_location)location,
		expanded);
	if (status != B_OK)
		log << "Can't move the Deskbar: " << strerror(status) << ".\n";
}


/*!	The whole look in use: decorator, control look, every system colour,
	the desktop colour, the fonts, the wallpaper and the Deskbar's place.
*/
static void
current_appearance(BMessage& look)
{
	BString decorator;
	if (BPrivate::get_decorator(decorator))
		look.AddString("decorator", decorator);

	// an empty path stands for Haiku's built-in look
	BString controlLook;
	BPrivate::get_control_look(controlLook);
	look.AddString("control look", controlLook);

	// every workspace's: Theme Manager themes may set several
	for (int32 i = 0; i < count_workspaces(); i++)
		look.AddColor("desktop", BScreen().DesktopColor(i));

	BMessage colors;
	for (size_t i = 0; i < sizeof(kColorWhich) / sizeof(kColorWhich[0]); i++) {
		const char* name = ui_color_name(kColorWhich[i]);
		if (name != NULL)
			colors.AddColor(name, ui_color(kColorWhich[i]));
	}
	look.AddMessage("colors", &colors);

	save_font(look, "plain", be_plain_font);
	save_font(look, "bold", be_bold_font);
	save_font(look, "fixed", be_fixed_font);
	menu_info menu;
	if (get_menu_info(&menu) == B_OK) {
		BFont menuFont;
		menuFont.SetFamilyAndStyle(menu.f_family, menu.f_style);
		menuFont.SetSize(menu.font_size);
		save_font(look, "menu", &menuFont);
	}

	save_wallpaper(look);
	save_deskbar(look);
	save_scroll_bar_arrows(look);
	save_font_smoothing(look);
}


bool
has_saved_appearance()
{
	BMessage previous;
	return load_message(kPreviousName, previous) == B_OK;
}


theme_mode
opposite_mode(theme_mode mode)
{
	switch (mode) {
		case THEME_MODE_LIGHT:
			return THEME_MODE_DARK;
		case THEME_MODE_DARK:
			return THEME_MODE_LIGHT;
		default:
			return THEME_MODE_UNKNOWN;
	}
}


const char*
mode_name(theme_mode mode)
{
	switch (mode) {
		case THEME_MODE_LIGHT:
			return "light";
		case THEME_MODE_DARK:
			return "dark";
		default:
			return "unknown";
	}
}


theme_mode
mode_from_name(const char* name)
{
	if (strcasecmp(name, "light") == 0)
		return THEME_MODE_LIGHT;
	if (strcasecmp(name, "dark") == 0)
		return THEME_MODE_DARK;
	return THEME_MODE_UNKNOWN;
}


const Theme*
find_variant(const ThemeList& themes, const Theme& theme, theme_mode mode)
{
	if (theme.Mode() == mode)
		return &theme;
	if (theme.VariantOf().IsEmpty())
		return NULL;

	for (int32 i = 0; i < themes.CountItems(); i++) {
		const Theme* other = themes.ItemAt(i);
		if (other->VariantOf() == theme.VariantOf() && other->Mode() == mode)
			return other;
	}
	return NULL;
}


const Theme*
find_theme(const ThemeList& themes, const char* name, theme_mode mode)
{
	// themes with no VariantOf() would all match
	if (name[0] == '\0')
		return NULL;

	const Theme* theme = NULL;
	for (int32 i = 0; i < themes.CountItems() && theme == NULL; i++) {
		if (themes.ItemAt(i)->Name().ICompare(name) == 0)
			theme = themes.ItemAt(i);
	}
	for (int32 i = 0; i < themes.CountItems() && theme == NULL; i++) {
		if (themes.ItemAt(i)->VariantOf().ICompare(name) == 0)
			theme = themes.ItemAt(i);
	}
	if (theme == NULL)
		return NULL;

	if (mode != THEME_MODE_UNKNOWN)
		return find_variant(themes, *theme, mode);
	if (theme->Name().ICompare(name) == 0)
		return theme;

	const Theme* light = find_variant(themes, *theme, THEME_MODE_LIGHT);
	return light != NULL ? light : theme;
}


const char* const kThemeAppSignature = "application/x-vnd.idumych-PowerTheme";
const char* const kThemeSchedulePort = "PowerTheme schedule";

status_t
reapply_addons(BString& log)
{
	BString name, icons;
	get_current(name, icons);
	if (name.IsEmpty())
		return B_OK;
	ThemeList themes;
	find_themes(themes);
	const Theme* theme = find_theme(themes, name);
	if (theme == NULL) {
		log << "The theme \"" << name << "\" isn't installed any more.\n";
		return B_ENTRY_NOT_FOUND;
	}
	// applying a part notes it as the only one applied
	const uint32 parts = get_current_parts();
	const uint32 addons = parts & (THEME_DECORATOR | THEME_CONTROLS);
	if (addons == 0)
		return B_OK;
	const status_t status = apply_appearance(*theme, addons, log);
	set_current_parts(parts);
	return status;
}


#ifndef APP_COSMOE

static const char* kAfterUpdateJob = "powertheme-after-update";


status_t
set_after_update_job(const char* program)
{
	BPath job;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &job, true);
	if (status == B_OK)
		status = job.Append("launch");
	if (status != B_OK)
		return status;
	create_directory(job.Path(), 0755);
	status = job.Append(kAfterUpdateJob);
	if (status != B_OK)
		return status;

	BString text;
	text << "# Written by Power Theme: after a system update, builds its window\n"
		"# decorators and control looks again for the new system.\n"
		"target desktop {\n"
		"\tjob x-vnd.idumych-PowerTheme-after-update {\n"
		"\t\tlaunch \"" << program << "\" --after-update\n"
		"\t\ton initial_volumes_mounted\n"
		"\t}\n"
		"}\n";

	// only when it changed, as Power Theme writes it every time it starts
	BFile file(job.Path(), B_READ_ONLY);
	off_t size = 0;
	if (file.InitCheck() == B_OK && file.GetSize(&size) == B_OK
		&& size == text.Length()) {
		BString saved;
		char* buffer = saved.LockBuffer(size);
		const ssize_t bytes = file.Read(buffer, size);
		saved.UnlockBuffer(bytes > 0 ? bytes : 0);
		if (saved == text)
			return B_OK;
	}

	status = file.SetTo(job.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;
	const ssize_t written = file.Write(text.String(), text.Length());
	return written == text.Length() ? B_OK
		: written < 0 ? (status_t)written : B_IO_ERROR;
}

#endif	// !APP_COSMOE


static const char* kScheduleName = "schedule";
static const char* kScheduleJob = "powertheme-schedule";


status_t
load_schedule(ThemeSchedule& schedule)
{
	schedule.enabled = false;
	schedule.lightAt = 7 * 60;
	schedule.darkAt = 19 * 60;

	BMessage saved;
	status_t status = load_message(kScheduleName, saved);
	if (status != B_OK)
		return status;
	schedule.enabled = saved.GetBool("enabled", false);
	schedule.lightAt = saved.GetInt32("light at", schedule.lightAt);
	schedule.darkAt = saved.GetInt32("dark at", schedule.darkAt);
	return B_OK;
}


static status_t
save_schedule(const ThemeSchedule& schedule, const char* program)
{
	BMessage saved;
	saved.AddBool("enabled", schedule.enabled);
	saved.AddInt32("light at", schedule.lightAt);
	saved.AddInt32("dark at", schedule.darkAt);
	status_t status = save_message(kScheduleName, saved);
	if (status != B_OK)
		return status;

	// what starts it at login: launch_daemon's job on Haiku, the desktop's
	// autostart folder on Linux
	BPath job;
#ifdef APP_COSMOE
	const char* config = getenv("XDG_CONFIG_HOME");
	if (config != NULL && config[0] == '/')
		status = job.SetTo(config);
	else
		status = job.SetTo(getenv("HOME"), ".config");
	if (status == B_OK)
		status = job.Append("autostart");
#else
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &job, true);
	if (status == B_OK)
		status = job.Append("launch");
#endif
	if (status != B_OK)
		return status;
	create_directory(job.Path(), 0755);
#ifdef APP_COSMOE
	BString leaf(kScheduleJob);
	leaf << ".desktop";
	status = job.Append(leaf.String());
#else
	status = job.Append(kScheduleJob);
#endif
	if (status != B_OK)
		return status;

	if (!schedule.enabled) {
		BEntry entry(job.Path());
		return entry.Exists() ? entry.Remove() : B_OK;
	}

	BFile file(job.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	BString text;
#ifdef APP_COSMOE
	text << "# Written by Power Theme; powertheme --schedule=off removes it.\n"
		"[Desktop Entry]\n"
		"Type=Application\n"
		"Name=Power Theme schedule\n"
		"Comment=Switches the theme between its light and dark versions\n"
		"Exec=\"" << program << "\" --run-schedule\n"
		"NoDisplay=true\n";
#else
	text << "# Written by Power Theme; powertheme --schedule=off removes it.\n"
		"target desktop {\n"
		"\tjob x-vnd.idumych-PowerTheme-schedule {\n"
		"\t\tlaunch \"" << program << "\" --run-schedule\n"
		"\t\ton initial_volumes_mounted\n"
		"\t}\n"
		"}\n";
#endif
	ssize_t written = file.Write(text.String(), text.Length());
	return written == text.Length() ? B_OK
		: written < 0 ? (status_t)written : B_IO_ERROR;
}


status_t
set_schedule(const ThemeSchedule& schedule)
{
	app_info info;
	status_t status = be_app->GetAppInfo(&info);
	if (status != B_OK)
		return status;

	status = save_schedule(schedule, BPath(&info.ref).Path());
	if (status != B_OK)
		return status;

	if (!schedule.enabled) {
		// gone before this returns, so that turning it back on starts a new one
		stop_schedule_process();
		for (int32 i = 0; i < 20 && schedule_process_running(); i++)
			snooze(50000);
		return B_OK;
	}

	// from the next login, the launch_daemon job or autostart entry starts it
	if (schedule_process_running())
		return B_OK;
	return run_program(BPath(&info.ref).Path(), "--run-schedule", false);
}


//	#pragma mark - the schedule's process


#ifdef APP_COSMOE

static int sScheduleLock = -1;


static BString
schedule_lock_path()
{
	BPath path;
	themes_settings_path(path, "schedule.pid");
	return path.Path();
}


static pid_t
schedule_process()
{
	int fd = open(schedule_lock_path().String(), O_RDONLY);
	if (fd < 0)
		return -1;
	pid_t pid = -1;
	// locked by the running one; the file stays when it ends
	if (flock(fd, LOCK_SH | LOCK_NB) != 0) {
		char text[32] = { 0 };
		if (read(fd, text, sizeof(text) - 1) > 0)
			pid = atoi(text);
	} else
		flock(fd, LOCK_UN);
	close(fd);
	return pid;
}


bool
schedule_process_running()
{
	return schedule_process() > 0;
}


void
stop_schedule_process()
{
	const pid_t pid = schedule_process();
	if (pid > 0)
		kill(pid, SIGTERM);
}


bool
claim_schedule_process()
{
	int fd = open(schedule_lock_path().String(), O_RDWR | O_CREAT, 0644);
	if (fd < 0)
		return false;
	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		close(fd);
		return false;
	}
	char text[32];
	const int length = snprintf(text, sizeof(text), "%d\n", (int)getpid());
	if (ftruncate(fd, 0) != 0 || write(fd, text, length) != length) {
		close(fd);
		return false;
	}
	sScheduleLock = fd;
	return true;
}


bool
wait_for_schedule_stop(bigtime_t timeout)
{
	// stop_schedule_process() ends it with a signal
	snooze(timeout);
	return false;
}


void
release_schedule_process()
{
	if (sScheduleLock >= 0)
		close(sScheduleLock);
	sScheduleLock = -1;
}


status_t
run_program(const char* program, const char* argument, bool wait)
{
	const pid_t child = fork();
	if (child < 0)
		return errno;
	if (child == 0) {
		setsid();
		execl(program, program, argument, (char*)NULL);
		_exit(127);
	}
	if (wait) {
		int status;
		while (waitpid(child, &status, 0) < 0 && errno == EINTR)
			;
	}
	return B_OK;
}

#else	// APP_COSMOE

static port_id sSchedulePort = -1;


bool
schedule_process_running()
{
	return find_port(kThemeSchedulePort) >= 0;
}


void
stop_schedule_process()
{
	const port_id port = find_port(kThemeSchedulePort);
	if (port >= 0)
		write_port_etc(port, 0, NULL, 0, B_RELATIVE_TIMEOUT, 100000);
}


bool
claim_schedule_process()
{
	if (find_port(kThemeSchedulePort) >= 0)
		return false;
	sSchedulePort = create_port(1, kThemeSchedulePort);
	return sSchedulePort >= 0;
}


bool
wait_for_schedule_stop(bigtime_t timeout)
{
	int32 code;
	return read_port_etc(sSchedulePort, &code, NULL, 0, B_RELATIVE_TIMEOUT,
		timeout) != B_TIMED_OUT;
}


void
release_schedule_process()
{
	if (sSchedulePort >= 0)
		delete_port(sSchedulePort);
	sSchedulePort = -1;
}


status_t
run_program(const char* program, const char* argument, bool wait)
{
	const char* arguments[] = { program, argument };
	const thread_id team = load_image(2, arguments,
		const_cast<const char**>(environ));
	if (team < 0)
		return team;
	status_t status = resume_thread(team);
	if (status == B_OK && wait) {
		status_t result;
		wait_for_thread(team, &result);
	}
	return status;
}

#endif	// APP_COSMOE


bool
parse_schedule_time(const char* text, int32& minutes)
{
	int hour, minute;
	char end;
	if (sscanf(text, "%d:%d%c", &hour, &minute, &end) != 2
		|| hour < 0 || hour > 23 || minute < 0 || minute > 59) {
		return false;
	}
	minutes = hour * 60 + minute;
	return true;
}


theme_mode
scheduled_mode(const ThemeSchedule& schedule, int32 minutes)
{
	if (schedule.lightAt < schedule.darkAt) {
		return minutes >= schedule.lightAt && minutes < schedule.darkAt
			? THEME_MODE_LIGHT : THEME_MODE_DARK;
	}
	return minutes >= schedule.darkAt && minutes < schedule.lightAt
		? THEME_MODE_DARK : THEME_MODE_LIGHT;
}


theme_mode
scheduled_mode_now(const ThemeSchedule& schedule)
{
	time_t now = time(NULL);
	struct tm local;
	localtime_r(&now, &local);
	return scheduled_mode(schedule, local.tm_hour * 60 + local.tm_min);
}


BString
schedule_description(const ThemeSchedule& schedule)
{
	char text[64];
	snprintf(text, sizeof(text), "Light at %02" B_PRId32 ":%02" B_PRId32
		", dark at %02" B_PRId32 ":%02" B_PRId32 ", every day.",
		schedule.lightAt / 60, schedule.lightAt % 60, schedule.darkAt / 60,
		schedule.darkAt % 60);
	return text;
}


static const char* kBeOSTabsName = "beos tabs";


//! The window borders that can draw a BeOS-style tab.
static bool
draws_beos_tabs(DecorInfo* decor)
{
	if (decor == NULL || decor->Ref() == NULL)
		return false;
	BPath path(decor->Ref());
	static const char* const kNames[] = { "Reactionary", "Expose", "Platinum",
		"Leopard", "Gershwix", "NeXT",
		"System7", "Amiga", "AmigaOS3", "QNX" };
	for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
		if (path.Leaf() != NULL && strcmp(path.Leaf(), kNames[i]) == 0)
			return true;
	}
	return false;
}


bool
beos_tabs()
{
	BPath path;
	return themes_settings_path(path, kBeOSTabsName) == B_OK
		&& BEntry(path.Path()).Exists();
}


status_t
set_beos_tabs(bool on, BString& log)
{
	BPath path;
	status_t status = themes_settings_path(path, kBeOSTabsName);
	if (status != B_OK)
		return status;
	BEntry entry(path.Path());
	if (on == entry.Exists())
		return B_OK;

	if (on) {
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		status = file.InitCheck();
	} else
		status = entry.Remove();
	if (status != B_OK) {
		log << "Can't change the window tabs: " << strerror(status) << ".\n";
		return status;
	}

	// Add-ons read this as they load.
	DecorInfoUtility decorators;
	DecorInfo* current = decorators.CurrentDecorator();
	if (draws_beos_tabs(current) && decorators.SetDecorator(current) != B_OK)
		log << "Can't reload the window borders.\n";
	return B_OK;
}


bool
has_beos_tabs(const Theme& theme)
{
	DecorInfoUtility decorators;
	return !theme.Decorator().IsEmpty()
		&& draws_beos_tabs(find_decorator(decorators, theme.Decorator()));
}


static const char* kShellName = "photon shell";
static const char* kShellJob = "powertheme-shelf";
static const char* kShellSignature
	= "application/x-vnd.idumych-PowerTheme-shelf";


bool
photon_shell()
{
	BPath path;
	return themes_settings_path(path, kShellName) == B_OK
		&& BEntry(path.Path()).Exists();
}


bool
photon_shell_running()
{
	return be_roster->IsRunning(kShellSignature);
}


shell_entry
photon_shell_entry()
{
	std::vector<BPath> folders;
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append("launch") == B_OK)
		folders.push_back(path);
	const directory_which places[] = { B_SYSTEM_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY, B_USER_DATA_DIRECTORY,
		B_USER_NONPACKAGED_DATA_DIRECTORY };
	for (size_t i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
		if (find_directory(places[i], &path) == B_OK
			&& path.Append("user_launch") == B_OK)
			folders.push_back(path);
	}

	bool job = false;
	for (const BPath& folder : folders) {
		BDirectory directory(folder.Path());
		BEntry entry;
		while (directory.GetNextEntry(&entry, true) == B_OK) {
			BFile file(&entry, B_READ_ONLY);
			off_t size = 0;
			if (file.InitCheck() != B_OK || file.GetSize(&size) != B_OK
				|| size <= 0 || size > 256 * 1024)
				continue;
			BString text;
			char* buffer = text.LockBuffer(size);
			const ssize_t bytes = file.Read(buffer, size);
			text.UnlockBuffer(bytes > 0 ? bytes : 0);
			if (text.IFindFirst("service x-vnd.idumych-PowerTheme-shelf") >= 0)
				return SHELL_ENTRY_SERVICE;
			if (text.IFindFirst("job x-vnd.idumych-PowerTheme-shelf") >= 0)
				job = true;
		}
	}
	return job ? SHELL_ENTRY_JOB : SHELL_ENTRY_NONE;
}


static const char* kNoShellEntryQuestion = "don't offer shell entry";


bool
ask_for_shell_entry()
{
	BPath path;
	return themes_settings_path(path, kNoShellEntryQuestion) != B_OK
		|| !BEntry(path.Path()).Exists();
}


void
set_ask_for_shell_entry(bool ask)
{
	BPath path;
	if (themes_settings_path(path, kNoShellEntryQuestion) != B_OK)
		return;
	if (ask)
		BEntry(path.Path()).Remove();
	else
		BFile(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
}


#ifndef APP_COSMOE

//! PowerShelf, in the folder this program is in (past a link to it).
static status_t
shell_program(BPath& path)
{
	image_info image;
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if (image.type != B_APP_IMAGE)
			continue;
		BEntry entry(image.name, true);
		status_t status = entry.GetPath(&path);
		if (status == B_OK)
			status = path.GetParent(&path);
		if (status == B_OK)
			status = path.Append("PowerShelf");
		return status;
	}
	return B_ENTRY_NOT_FOUND;
}

#endif


status_t
set_photon_shell(bool on, BString& log)
{
#ifdef APP_COSMOE
	if (on)
		log << "The Photon shell isn't available on Linux.\n";
	return on ? B_NOT_SUPPORTED : B_OK;
#else
	BPath program;
	status_t status = shell_program(program);
	if (on && (status != B_OK || !BEntry(program.Path()).Exists())) {
		log << "Can't find PowerShelf, the Photon shell.\n";
		return B_ENTRY_NOT_FOUND;
	}

	BPath setting;
	status = themes_settings_path(setting, kShellName);
	BPath job;
	if (status == B_OK)
		status = find_directory(B_USER_SETTINGS_DIRECTORY, &job, true);
	if (status == B_OK)
		status = job.Append("launch");
	if (status == B_OK) {
		create_directory(job.Path(), 0755);
		status = job.Append(kShellJob);
	}
	if (status != B_OK) {
		log << "Can't change the shell: " << strerror(status) << ".\n";
		return status;
	}

	if (!on) {
		BEntry(setting.Path()).Remove();
		BEntry(job.Path()).Remove();
		// launch_daemon restarts the service if it ends, unless it's stopped
		// first; it brings the Deskbar back as it quits
		system("launch_roster stop x-vnd.idumych-PowerTheme-shelf "
			">/dev/null 2>&1");
		BMessenger(kShellSignature).SendMessage(B_QUIT_REQUESTED);
		for (int32 i = 0; i < 40 && be_roster->IsRunning(kShellSignature);
				i++)
			snooze(50000);
		for (int32 i = 0; i < 40
				&& !be_roster->IsRunning("application/x-vnd.Be-TSKB"); i++)
			snooze(50000);
		return B_OK;
	}

	BFile(setting.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	// launch_daemon starts it at login, as a service that it starts again if
	// it ends, as it does the Deskbar; it stops the Deskbar
	BFile file(job.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	BString text;
	text << "# Written by Power Theme; powertheme --shell=deskbar removes it.\n"
		"target desktop {\n"
		"\tservice x-vnd.idumych-PowerTheme-shelf {\n"
		"\t\tlaunch \"" << program.Path() << "\"\n"
		"\t\ton initial_volumes_mounted\n"
		"\t}\n"
		"}\n";
	if (file.Write(text.String(), text.Length()) != text.Length())
		log << "Can't save the shell for the next login.\n";

	if (be_roster->IsRunning(kShellSignature))
		return B_OK;
	// as the service, if launch_daemon knows it from this login; otherwise
	// on its own until the next
	// it may have been disabled, by being turned off or giving up after
	// crashing again and again
	system("launch_roster enable x-vnd.idumych-PowerTheme-shelf "
		">/dev/null 2>&1");
	if (system("launch_roster start x-vnd.idumych-PowerTheme-shelf "
			">/dev/null 2>&1") == 0) {
		for (int32 i = 0; i < 40 && !be_roster->IsRunning(kShellSignature);
				i++)
			snooze(50000);
		if (be_roster->IsRunning(kShellSignature))
			return B_OK;
	}
	entry_ref ref;
	status = get_ref_for_path(program.Path(), &ref);
	if (status == B_OK)
		status = be_roster->Launch(&ref);
	if (status != B_OK && status != B_ALREADY_RUNNING)
		log << "Can't start the Photon shell: " << strerror(status) << ".\n";
	return B_OK;
#endif
}


theme_mode
current_mode()
{
	BString name, icons;
	get_current(name, icons);
	if (name.IsEmpty())
		return THEME_MODE_UNKNOWN;

	ThemeList themes;
	find_themes(themes);
	const Theme* theme = find_theme(themes, name);
	return theme != NULL ? theme->Mode() : THEME_MODE_UNKNOWN;
}


//! Changes to the icons of \a theme, if they aren't in use. Whether it did.
static bool
apply_icons_of(const Theme& theme, BString& log)
{
	const BString wanted = theme_icons(theme);
	BString themeInUse, inUse;
	get_current(themeInUse, inUse);
	if (wanted == kKeepIcons || wanted == inUse
		|| (is_haiku(wanted) && inUse.IsEmpty())) {
		return false;
	}

	if (is_haiku(wanted))
		return revert_icons(log) == B_OK;

	int32 changed;
	if (is_dark_haiku(wanted))
		return apply_dark_haiku_icons(log, changed) == B_OK;

	IconTheme* icons = load_icon_theme(wanted);
	if (icons == NULL) {
		log << "The icon theme \"" << wanted << "\" isn't installed.\n";
		return false;
	}
	const status_t status = apply_icon_theme(*icons, log, changed);
	delete icons;
	return status == B_OK;
}


status_t
apply_mode(theme_mode mode, BString& log, BString* applied, bool* iconsChanged)
{
	if (iconsChanged != NULL)
		*iconsChanged = false;

	if (!can_apply(log))
		return B_NOT_ALLOWED;

	BString name, icons;
	get_current(name, icons);
	if (name.IsEmpty()) {
		log << "No theme is in use, so there's nothing to switch.\n";
		return B_ERROR;
	}

	ThemeList themes;
	find_themes(themes);
	const Theme* current = find_theme(themes, name);
	if (current == NULL) {
		log << "The theme \"" << name << "\" isn't installed any more.\n";
		return B_ENTRY_NOT_FOUND;
	}
	if (current->Mode() == mode)
		return B_OK;

	const Theme* theme = find_variant(themes, *current, mode);
	if (theme == NULL) {
		log << "\"" << name << "\" has no " << mode_name(mode)
			<< " version.\n";
		return B_ENTRY_NOT_FOUND;
	}

	// What isn't installed in this version shouldn't be left out of the
	// other one when switching back.
	const uint32 parts = get_current_parts();
	BStringList missing;
	uint32 available = parts & ~theme->Missing(&missing, parts);
	if (!missing.IsEmpty()) {
		log << "Not installed, so left out: " << missing.Join(", ") << ".\n";
	}

	// Two versions usually share window borders, and reloading those would only
	// redraw every window's frame, leaving lines of it on the windows in front.
	if (theme->Decorator() == current->Decorator())
		available &= ~THEME_DECORATOR;

	// Icons first: Tracker notes that they changed, but only shows it when it
	// next redraws, which the new colours make it do.
	if (icons_follow_mode()) {
		const bool changed = apply_icons_of(*theme, log);
		if (iconsChanged != NULL)
			*iconsChanged = changed;
		if (changed)
			snooze(kIconsSettleTime);
	}

	status_t status = apply_appearance(*theme, available, log);
	set_current_parts(parts);
	if (status != B_OK)
		return status;

	if (applied != NULL)
		*applied = theme->Name();

	// an open window should show what is in use now; the command line also goes
	// by this signature, but doesn't read its messages
	BList running;
	be_roster->GetAppList(kThemeAppSignature, &running);
	for (int32 i = 0; i < running.CountItems(); i++) {
		BMessenger(kThemeAppSignature, (team_id)(addr_t)running.ItemAt(i))
			.SendMessage(kMsgThemeSwitched);
	}
	return B_OK;
}


status_t
apply_appearance(const Theme& theme, uint32 parts, BString& log)
{
	if (!can_apply(log))
		return B_NOT_ALLOWED;

	const uint32 requested = parts;
	parts &= theme.Parts();
	const float tabHeight = window_tab_height();
	BDeskbar deskbar;
	const bool ignoredTabHeight = deskbar_ignores_tab_height(deskbar);

	BMessage previous;
	if (load_message(kPreviousName, previous) != B_OK) {
		current_appearance(previous);
		status_t status = save_message(kPreviousName, previous);
		if (status != B_OK) {
			log << "Can't save the look in use, so nothing was changed: "
				<< strerror(status) << ".\n";
			return status;
		}
	} else {
		// saved by an older Power Theme (then Themes), before it changed these: they are still
		// as they were
		bool changed = false;
		if (!previous.HasData("wallpaper", B_RAW_TYPE)
			&& !previous.HasBool("no wallpaper")) {
			save_wallpaper(previous);
			changed = true;
		}
		if (!previous.HasInt32("deskbar location")) {
			save_deskbar(previous);
			changed = true;
		}
		if (!previous.HasBool("double arrows")) {
			save_scroll_bar_arrows(previous);
			changed = true;
		}
		if (!previous.HasBool("subpixel smoothing")) {
			save_font_smoothing(previous);
			changed = true;
		}
		if (changed)
			save_message(kPreviousName, previous);
	}

	if ((parts & THEME_DESKBAR) != 0) {
		// the Deskbar comes back before it's placed; the shelf goes in its
		// place after
		// a theme without the key has the Deskbar; the shell can be turned
		// on again by hand
		int32 shell = theme.Shell();
		if (shell < 0)
			shell = 0;
		if (shell == 0 && photon_shell())
			set_photon_shell(false, log);
		set_deskbar(theme.DeskbarLocation(), theme.DeskbarExpanded(), log);
		if (shell == 1 && !photon_shell())
			set_photon_shell(true, log);
	}

	// They go with the controls. A theme that says nothing brings back the
	// setting from before any theme, so that one that does doesn't linger.
	if ((requested & THEME_CONTROLS) != 0) {
		set_scroll_bar_arrows(theme.ScrollBarArrows() != 0
			? theme.ScrollBarArrows() == 2
			: previous.GetBool("double arrows", false));
	}

	// Smoothing goes with the fonts: a pixel font wants it in grey, and any
	// other theme brings back the setting from before any theme.
	if ((requested & THEME_FONTS) != 0) {
		set_font_smoothing(theme.GreyFontSmoothing() ? false
			: previous.GetBool("subpixel smoothing", true));
	}

	if ((parts & THEME_WALLPAPER) != 0)
		set_wallpaper(theme, log);

	if ((parts & THEME_DECORATOR) != 0) {
		DecorInfoUtility decorators;
		DecorInfo* decor = find_decorator(decorators, theme.Decorator());
		if (decor == NULL)
			log << "The window decorator \"" << theme.Decorator()
				<< "\" isn't installed.\n";
		else if (decorators.SetDecorator(decor) != B_OK)
			log << "Can't switch to the \"" << theme.Decorator()
				<< "\" window decorator.\n";
	}

	if ((parts & THEME_COLORS) != 0) {
		if (!theme.Colors().IsEmpty())
			set_ui_colors(&theme.Colors());
		if (theme.HasDesktopColor())
			BScreen().SetDesktopColor(theme.DesktopColor(), true);
		// a Theme Manager theme's colours for further workspaces
		rgb_color color;
		for (int32 i = 1; theme.DesktopColors().FindColor("color", i, &color)
				== B_OK && i < count_workspaces(); i++) {
			BScreen().SetDesktopColor(color, i, true);
		}
	}

	// the Deskbar along the top or bottom doesn't lay itself out again for a
	// new font size, and has to be restarted for it
	bool fontResized = false;
	if ((parts & THEME_FONTS) != 0) {
		for (int32 i = 0; i < theme.CountFonts(); i++) {
			const ThemeFont* font = theme.FontAt(i);
			if (font->which == "plain" && font->size != be_plain_font->Size())
				fontResized = true;
		}
	}

	if ((parts & THEME_FONTS) != 0) {
		for (int32 i = 0; i < theme.CountFonts(); i++) {
			const ThemeFont* font = theme.FontAt(i);
			// the first family and style of the alternatives installed
			BStringList families, styles;
			font->family.Split("|", true, families);
			font->style.Split("|", true, styles);
			bool found = false;
			for (int32 f = 0; f < families.CountStrings() && !found; f++) {
				BString family = trimmed(families.StringAt(f));
				for (int32 s = 0; s < styles.CountStrings() && !found; s++) {
					BString style = trimmed(styles.StringAt(s));
					if (font_installed(family, style)) {
						set_font(font->which, family, style, font->size);
						found = true;
					}
				}
			}
			if (!found) {
				log << "The font " << font->family << ", " << font->style
					<< " isn't installed.\n";
			}
		}
	}

	if ((parts & THEME_CONTROLS) != 0) {
		// an empty path is Haiku's built-in look
		BString path;
		if (!find_control_look_of(theme.ControlLook(), path))
			log << "The control look \"" << theme.ControlLook()
				<< "\" isn't installed.\n";
		else
			BPrivate::set_control_look(path);
	}

	if (fontResized)
		restart_deskbar();
	else
		update_deskbar_for_tab_height(tabHeight, ignoredTabHeight);

	set_current(theme.Name(), NULL);
	set_current_parts(requested & ~THEME_ICONS);
	return B_OK;
}


//! The first installed font of a family and style list, or "".
static BString
resolve_font(const ThemeFont* font)
{
	BStringList families, styles;
	font->family.Split("|", true, families);
	font->style.Split("|", true, styles);
	for (int32 f = 0; f < families.CountStrings(); f++) {
		BString family = trimmed(families.StringAt(f));
		for (int32 s = 0; s < styles.CountStrings(); s++) {
			BString style = trimmed(styles.StringAt(s));
			if (font_installed(family, style))
				return BString(family) << ", " << style;
		}
	}
	return "";
}


void
describe_theme(const Theme& theme, BString& out)
{
	out << "name: " << theme.Name() << "\n";
	out << "format: " << (theme.IsUITheme() ? "Theme Manager" : "Power Theme")
		<< "\n";
	out << "folder: " << theme.Folder() << "\n";
	out << "mode: " << mode_name(theme.Mode()) << "\n";
	if (!theme.VariantOf().IsEmpty())
		out << "version of: " << theme.VariantOf() << "\n";

	if (!theme.Decorator().IsEmpty()) {
		DecorInfoUtility decorators;
		DecorInfo* decor = find_decorator(decorators, theme.Decorator());
		out << "decorator: " << theme.Decorator() << " -> "
			<< (decor != NULL ? decor->Name().String() : "(none installed)")
			<< "\n";
	}
	if (!theme.ControlLook().IsEmpty()) {
		BString path;
		bool found = find_control_look_of(theme.ControlLook(), path);
		out << "control look: " << theme.ControlLook() << " -> "
			<< (!found ? "(none installed)"
				: path.IsEmpty() ? "Haiku's built-in" : path.String())
			<< "\n";
	}

	int32 colors = 0;
	char* name;
	type_code type;
	for (int32 i = 0; theme.Colors().GetInfo(B_RGB_32_BIT_TYPE, i, &name,
			&type) == B_OK; i++)
		colors++;
	out << "colours: " << colors;
	rgb_color color;
	if (theme.Colors().FindColor("B_PANEL_BACKGROUND_COLOR", &color) == B_OK) {
		char text[32];
		snprintf(text, sizeof(text), " (panel #%02X%02X%02X)", color.red,
			color.green, color.blue);
		out << text;
	}
	out << "\n";
	for (int32 i = 0; theme.DesktopColors().FindColor("color", i, &color)
			== B_OK; i++) {
		char text[48];
		snprintf(text, sizeof(text), "desktop colour %" B_PRId32
			": #%02X%02X%02X\n", i + 1, color.red, color.green, color.blue);
		out << text;
	}
	if (theme.HasDesktopColor() && theme.DesktopColors().IsEmpty()) {
		color = theme.DesktopColor();
		char text[48];
		snprintf(text, sizeof(text), "desktop colour: #%02X%02X%02X\n",
			color.red, color.green, color.blue);
		out << text;
	}

	for (int32 i = 0; i < theme.CountFonts(); i++) {
		const ThemeFont* font = theme.FontAt(i);
		BString found = resolve_font(font);
		out << "font " << font->which << ": " << font->family << ", "
			<< font->style << ", " << font->size << " -> "
			<< (found.IsEmpty() ? "(none installed)" : found.String())
			<< "\n";
	}

	if (!theme.Background().IsEmpty()) {
		const char* image;
		int32 workspaces;
		for (int32 i = 0; theme.Background().FindString(B_BACKGROUND_IMAGE, i,
				&image) == B_OK; i++) {
			if (theme.Background().FindInt32(B_BACKGROUND_WORKSPACES, i,
					&workspaces) != B_OK)
				workspaces = -1;
			char text[32];
			snprintf(text, sizeof(text), "0x%" B_PRIx32, (uint32)workspaces);
			out << "background (workspaces " << text << "): "
				<< (image[0] == '\0' ? "no image" : image)
				<< (image[0] != '\0' && !BEntry(image).Exists()
					? " (missing)" : "") << "\n";
		}
	} else if (!theme.Wallpaper().IsEmpty())
		out << "wallpaper: " << theme.Wallpaper() << "\n";

	if (theme.DeskbarLocation() >= 0) {
		static const char* kPlaces[] = { "top", "bottom", "left top",
			"right top", "left bottom", "right bottom" };
		out << "deskbar: " << kPlaces[theme.DeskbarLocation()]
			<< (theme.DeskbarExpanded() ? ", expanded" : ", collapsed")
			<< "\n";
	}

	BStringList missing;
	theme.Missing(&missing);
	if (!missing.IsEmpty())
		out << "not installed: " << missing.Join(", ") << "\n";
}


status_t
revert_appearance(BString& log)
{
	BMessage previous;
	status_t status = load_message(kPreviousName, previous);
	if (status != B_OK)
		return status;
	const float tabHeight = window_tab_height();
	BDeskbar deskbar;
	const bool ignoredTabHeight = deskbar_ignores_tab_height(deskbar);

	BString decorator;
	if (previous.FindString("decorator", &decorator) == B_OK)
		BPrivate::set_decorator(decorator);

	BMessage colors;
	if (previous.FindMessage("colors", &colors) == B_OK)
		set_ui_colors(&colors);

	BString controlLook;
	if (previous.FindString("control look", &controlLook) == B_OK)
		BPrivate::set_control_look(controlLook);

	BMessage font;
	bool fontResized = false;
	for (int32 i = 0; previous.FindMessage("font", i, &font) == B_OK; i++) {
		BString which, family, style;
		float size;
		if (font.FindString("which", &which) != B_OK
			|| font.FindString("family", &family) != B_OK
			|| font.FindString("style", &style) != B_OK
			|| font.FindFloat("size", &size) != B_OK)
			continue;
		if (which == "plain" && size != be_plain_font->Size())
			fontResized = true;
		set_font(which, family, style, size);
	}

	rgb_color desktop;
	for (int32 i = 0; previous.FindColor("desktop", i, &desktop) == B_OK
			&& i < count_workspaces(); i++)
		BScreen().SetDesktopColor(desktop, i, true);

	restore_wallpaper(previous);

	int32 location;
	if (previous.FindInt32("deskbar location", &location) == B_OK) {
		BString ignored;
		set_deskbar(location, previous.GetBool("deskbar expanded", true),
			ignored);
	}

	bool doubled;
	if (previous.FindBool("double arrows", &doubled) == B_OK)
		set_scroll_bar_arrows(doubled);

	bool subpixel;
	if (previous.FindBool("subpixel smoothing", &subpixel) == B_OK)
		set_font_smoothing(subpixel);

	// there was none before any theme
	if (photon_shell()) {
		BString ignored;
		set_photon_shell(false, ignored);
	}

	if (fontResized)
		restart_deskbar();
	else
		update_deskbar_for_tab_height(tabHeight, ignoredTabHeight);

	BPath path;
	if (themes_settings_path(path, kPreviousName) == B_OK)
		BEntry(path.Path()).Remove();

	set_current("", NULL);
	return B_OK;
}
