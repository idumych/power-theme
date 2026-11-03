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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

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
#include <fs_attr.h>
#include <be_apps/Tracker/Background.h>

#include <vector>

#include <DecorInfo.h>
#include <DecoratorPrivate.h>
#include <InterfacePrivate.h>

#include "IconTheme.h"


// private font API, as used by Appearance
extern void _set_system_font_(const char* which, font_family family,
	font_style style, float size);


static const char* kPreviousName = "previous";
static const char* kTrackerSignature = "application/x-vnd.Be-TRAK";
static const char* kBackgroundImageSet = "be:bgndimginfoset";
static const char* kCurrentName = "current";

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
	fWallpaperMode(B_BACKGROUND_MODE_SCALED),
	fDeskbarLocation(-1),
	fDeskbarExpanded(true),
	fUITheme(false),
	fHasDesktopColor(false)
{
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
	if (fDeskbarLocation >= 0)
		parts |= THEME_DESKBAR;
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
Theme::Missing(BStringList* what) const
{
	uint32 missing = 0;
	if (!fDecorator.IsEmpty()) {
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
	if (!fControlLook.IsEmpty() && !find_control_look_of(fControlLook, path)) {
		missing |= THEME_CONTROLS;
		if (what != NULL)
			what->Add(BString("the control look \"") << fControlLook << "\"");
	}

	if (!fIcons.IsEmpty() && !is_haiku(fIcons)) {
		IconTheme* icons = load_icon_theme(fIcons);
		if (icons == NULL) {
			missing |= THEME_ICONS;
			if (what != NULL)
				what->Add(BString("the icon theme \"") << fIcons << "\"");
		}
		delete icons;
	}

	if (fBackground.IsEmpty() && !fWallpaper.IsEmpty() && fWallpaper != "none"
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
}


bool
has_saved_appearance()
{
	BMessage previous;
	return load_message(kPreviousName, previous) == B_OK;
}


status_t
apply_appearance(const Theme& theme, uint32 parts, BString& log)
{
	parts &= theme.Parts();

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
		if (changed)
			save_message(kPreviousName, previous);
	}

	if ((parts & THEME_DESKBAR) != 0)
		set_deskbar(theme.DeskbarLocation(), theme.DeskbarExpanded(), log);

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

	set_current(theme.Name(), NULL);
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
	for (int32 i = 0; previous.FindMessage("font", i, &font) == B_OK; i++) {
		BString which, family, style;
		float size;
		if (font.FindString("which", &which) != B_OK
			|| font.FindString("family", &family) != B_OK
			|| font.FindString("style", &style) != B_OK
			|| font.FindFloat("size", &size) != B_OK)
			continue;
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

	BPath path;
	if (themes_settings_path(path, kPreviousName) == B_OK)
		BEntry(path.Path()).Remove();

	set_current("", NULL);
	return B_OK;
}
