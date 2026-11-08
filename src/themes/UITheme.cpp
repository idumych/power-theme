/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Themes made for Theme Manager (github.com/HaikuArchives/HaikuThemeManager,
 * and BeOS and ZETA before it): a folder in a "UIThemes" folder, holding a
 * "Theme" file (a BMessage written out as text) and maybe a screenshot and
 * images.
 *
 * Taken over, as Theme Manager applies them: the system colours, fonts,
 * window decorator, title colours, the desktop's backgrounds and colours,
 * per workspace, and the Deskbar's place. Colours a theme leaves out are Haiku's defaults
 * (light or dark, after its panel colour) rather than whatever the previous
 * theme had, so a theme looks the same whatever came before. Fonts that
 * aren't installed fall back to Noto Sans. Many themes name ZETA
 * decorators that don't exist on Haiku; those get the nearest installed
 * one. Terminal, sound and screensaver settings are left out.
 */


#include "Theme.h"

#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <StringList.h>
#include <be_apps/Tracker/Background.h>

#include <DecorInfo.h>

#include "HaikuDefaults.h"


// Theme Manager's colour names, as in its UI settings add-on
static const struct {
	const char*	key;
	color_which	which;
} kUIColors[] = {
	{ "PanBg", B_PANEL_BACKGROUND_COLOR }, { "PanTx", B_PANEL_TEXT_COLOR },
	{ "DocBg", B_DOCUMENT_BACKGROUND_COLOR }, { "DocTx", B_DOCUMENT_TEXT_COLOR },
	{ "CtlBg", B_CONTROL_BACKGROUND_COLOR }, { "CtlTx", B_CONTROL_TEXT_COLOR },
	{ "CtlBr", B_CONTROL_BORDER_COLOR }, { "CtlHg", B_CONTROL_HIGHLIGHT_COLOR },
	{ "CtlMk", B_CONTROL_MARK_COLOR },
	{ "NavBs", B_KEYBOARD_NAVIGATION_COLOR },
	{ "NavPl", B_NAVIGATION_PULSE_COLOR },
	{ "Shine", B_SHINE_COLOR }, { "Shadow", B_SHADOW_COLOR },
	{ "TipBg", B_TOOL_TIP_BACKGROUND_COLOR }, { "TipTx", B_TOOL_TIP_TEXT_COLOR },
	{ "MenBg", B_MENU_BACKGROUND_COLOR },
	{ "MenSBg", B_MENU_SELECTED_BACKGROUND_COLOR },
	{ "MenTx", B_MENU_ITEM_TEXT_COLOR },
	{ "MenSTx", B_MENU_SELECTED_ITEM_TEXT_COLOR },
	{ "MenSBr", B_MENU_SELECTED_BORDER_COLOR },
	{ "Success", B_SUCCESS_COLOR }, { "Failure", B_FAILURE_COLOR },
	{ "WinTabBg", B_WINDOW_TAB_COLOR }, { "WinTabTx", B_WINDOW_TEXT_COLOR },
	{ "InWinTabBg", B_WINDOW_INACTIVE_TAB_COLOR },
	{ "InWinTabTx", B_WINDOW_INACTIVE_TEXT_COLOR },
	{ "WinBr", B_WINDOW_BORDER_COLOR },
	{ "InWinBr", B_WINDOW_INACTIVE_BORDER_COLOR },
	{ "LstBg", B_LIST_BACKGROUND_COLOR },
	{ "LstSBg", B_LIST_SELECTED_BACKGROUND_COLOR },
	{ "LstTx", B_LIST_ITEM_TEXT_COLOR },
	{ "LstSTx", B_LIST_SELECTED_ITEM_TEXT_COLOR },
	{ "SbThb", B_SCROLL_BAR_THUMB_COLOR },
	{ "LnkTx", B_LINK_TEXT_COLOR }, { "LnkHv", B_LINK_HOVER_COLOR },
	{ "LnkVs", B_LINK_VISITED_COLOR }, { "LnkAc", B_LINK_ACTIVE_COLOR },
	{ "StaBar", B_STATUS_BAR_COLOR }
};

static const struct {
	const char*	key;
	const char*	which;
} kUIFonts[] = {
	{ "be:f:be_plain_font", "plain" }, { "be:f:be_bold_font", "bold" },
	{ "be:f:be_fixed_font", "fixed" }, { "be:f:MenTx", "menu" }
};

// ZETA and Dano decorators named by old themes, with what comes nearest
// on Haiku: a decorator, then a control look. The theme's own name is
// always tried first, in case it is installed.
static const struct {
	const char*	decorator;
	const char*	alternatives;
	const char*	controlLook;
} kDecoratorAliases[] = {
	{ "Win2k", "Reactionary | Windows 95", "Reactionary | haiku" },
	{ "Redmond", "Reactionary | Windows 95", "Reactionary | haiku" },
	{ "WinXP", "Reactionary | Windows 95", "Reactionary | haiku" },
	{ "GONX", "Gonx", "Gonx | haiku" },
	{ "gonxish", "Gonx", "Gonx | haiku" },
	{ "Baqua", "Leopard | Mac OS Platinum", "Leopard | haiku" },
	{ "Shark", "BeDecorator", "BeControlLook | haiku" },
	{ "blueBeOS", "BeDecorator", "BeControlLook | haiku" },
	{ "yellowTAB", "BeDecorator", "BeControlLook | haiku" },
	{ "Origin", "BeDecorator", "BeControlLook | haiku" },
	{ "BeDecorator", "BeDecorator", "BeControlLook | haiku" },
	{ "R5", "BeDecorator", "BeControlLook | haiku" },
	{ "Flat", "FlatDecorator", "FlatControlLook | haiku" },
	{ "FlatDecorator", "FlatDecorator", "FlatControlLook | haiku" },
	{ "Smoke", "Smoke", "Smoke | haiku" }
};

// BeOS R5's numbered decorators (window:R5:decor), as Theme Manager maps them
static const char* kR5Decorators[] = { "Default", "Windows 95",
	"AmigaDecorator", "Mac OS Platinum" };


//	#pragma mark - reading the text form of a BMessage


static BString
trim(const BString& text)
{
	BString result(text);
	result.Trim();
	return result;
}


//! What is between the outer parentheses of "type(...)".
static BString
arguments(const BString& value)
{
	int32 open = value.FindFirst('(');
	int32 close = value.FindLast(')');
	if (open < 0 || close < open)
		return "";
	BString result;
	value.CopyInto(result, open + 1, close - open - 1);
	return result;
}


static bool
parse_string(const BString& value, BString& result)
{
	if (!value.StartsWith("string("))
		return false;
	int32 first = value.FindFirst('"');
	int32 last = value.FindLast('"');
	if (first < 0 || last <= first)
		return false;
	value.CopyInto(result, first + 1, last - first - 1);
	return true;
}


static bool
parse_rgb(const BString& value, rgb_color& color)
{
	if (!value.StartsWith("rgb_color("))
		return false;
	BStringList parts;
	arguments(value).Split(",", true, parts);
	if (parts.CountStrings() < 3)
		return false;
	color.red = atoi(parts.StringAt(0));
	color.green = atoi(parts.StringAt(1));
	color.blue = atoi(parts.StringAt(2));
	color.alpha = 255;
	return true;
}


static bool
parse_int(const BString& value, int32& result)
{
	if (!value.StartsWith("int32(") && !value.StartsWith("int64(")
		&& !value.StartsWith("uint32("))
		return false;
	result = (int32)strtoll(arguments(value), NULL, 0);
	return true;
}


static bool
parse_point(const BString& value, BPoint& point)
{
	if (!value.StartsWith("BPoint("))
		return false;
	BString text = arguments(value);
	text.RemoveAll("x:");
	text.RemoveAll("y:");
	BStringList parts;
	text.Split(",", true, parts);
	if (parts.CountStrings() < 2)
		return false;
	point.x = atof(parts.StringAt(0));
	point.y = atof(parts.StringAt(1));
	return true;
}


//! "BFont(Family/Style/12.0, ...)"
static bool
parse_font(const BString& value, BString& family, BString& style, float& size)
{
	if (!value.StartsWith("BFont("))
		return false;
	BString text = arguments(value);
	int32 comma = text.FindFirst(',');
	if (comma >= 0)
		text.Truncate(comma);
	BStringList parts;
	text.Split("/", true, parts);
	if (parts.CountStrings() < 3)
		return false;
	family = trim(parts.StringAt(0));
	style = trim(parts.StringAt(1));
	size = atof(parts.StringAt(2));
	return !family.IsEmpty() && size > 0;
}


//! "name[2]" -> "name", 2
static BString
field_name(const BString& name, int32& index)
{
	index = 0;
	int32 bracket = name.FindFirst('[');
	if (bracket < 0)
		return name;
	index = atoi(name.String() + bracket + 1);
	BString result;
	name.CopyInto(result, 0, bracket);
	return result;
}


//	#pragma mark - Theme::LoadUITheme


status_t
Theme::LoadUITheme(const char* folder)
{
	fFolder = folder;
	fUITheme = true;
	BPath path(folder, "Theme");
	BFile file(path.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	off_t size;
	if (file.GetSize(&size) != B_OK || size > 1024 * 1024)
		return B_BAD_DATA;

	BString text;
	char* buffer = text.LockBuffer(size + 1);
	ssize_t bytesRead = file.Read(buffer, size);
	text.UnlockBuffer(bytesRead > 0 ? bytesRead : 0);
	if (text.FindFirst("BMessage(") < 0)
		return B_BAD_DATA;
		// a flattened message, which no theme seems to use

	fName = BPath(folder).Leaf();
	BString description, screenshot;
	BStringList decorators;
	int32 r5Decorator = -1;
	rgb_color activeTitle, inactiveTitle;
	bool hasActiveTitle = false, hasInactiveTitle = false;
	BMessage colors;
	BMessage background;

	BStringList sections;
	BStringList lines;
	text.Split("\n", false, lines);
	for (int32 i = 0; i < lines.CountStrings(); i++) {
		BString line = trim(lines.StringAt(i));
		if (line.IsEmpty() || line.StartsWith("//"))
			continue;
		if (line == "}") {
			if (!sections.IsEmpty())
				sections.Remove(sections.CountStrings() - 1);
			continue;
		}
		int32 equals = line.FindFirst(" = ");
		if (equals < 0)
			continue;
		BString name, value;
		line.CopyInto(name, 0, equals);
		line.CopyInto(value, equals + 3, line.Length() - equals - 3);
		name = trim(name);
		value = trim(value);
		if (value.StartsWith("BMessage(") && value.EndsWith("{")) {
			sections.Add(name);
			continue;
		}

		int32 index;
		BString field = field_name(name, index);
		const BString section = sections.IsEmpty()
			? BString("") : sections.StringAt(0);

		BString string;
		rgb_color color;
		int32 number;
		BPoint point;
		if (section == "z:theme:infos") {
			if (field == "z:theme:name" && parse_string(value, string)
				&& !string.IsEmpty())
				fName = string;
			else if (field == "z:theme:description"
				&& parse_string(value, string))
				description = string;
			else if (field == "z:theme:screenshot"
				&& parse_string(value, string))
				screenshot = string;
		} else if (section == B_BACKGROUND_INFO) {
			if (field == "be:desktop_color" && parse_rgb(value, color))
				fDesktopColors.AddColor("color", color);
			else if (field == B_BACKGROUND_IMAGE && parse_string(value, string))
				background.AddString(B_BACKGROUND_IMAGE, string);
			else if ((field == B_BACKGROUND_WORKSPACES
					|| field == B_BACKGROUND_MODE
					|| field == "be:bgndimginfoset")
				&& parse_int(value, number))
				background.AddInt32(field, number);
			else if (field == B_BACKGROUND_ORIGIN && parse_point(value, point))
				background.AddPoint(B_BACKGROUND_ORIGIN, point);
			else if (field == B_BACKGROUND_ERASE_TEXT)
				background.AddBool(B_BACKGROUND_ERASE_TEXT,
					value.FindFirst("true") >= 0);
		} else if (section == "ui_settings") {
			BString key = field;
			int32 colon = key.FindLast(':');
			if ((key.StartsWith("be:c:") || key.StartsWith("h:c:"))
				&& parse_rgb(value, color)) {
				key.Remove(0, colon + 1);
				for (size_t c = 0; c < sizeof(kUIColors) / sizeof(kUIColors[0]);
						c++) {
					if (key == kUIColors[c].key && !colors.HasColor(key))
						colors.AddColor(key, color);
				}
			}
			BString family, style;
			float fontSize;
			for (size_t f = 0; f < sizeof(kUIFonts) / sizeof(kUIFonts[0]); f++) {
				if (field != kUIFonts[f].key
					|| !parse_font(value, family, style, fontSize))
					continue;
				ThemeFont* font = new ThemeFont;
				font->which = kUIFonts[f].which;
				// what isn't installed falls back to Haiku's own
				const char* fallback = font->which == "fixed"
					? "Noto Sans Mono" : "Noto Sans";
				font->family = family;
				if (family != fallback)
					font->family << " | " << fallback;
				font->style = style;
				const char* plainStyle = font->which == "bold"
					? "Bold" : "Regular";
				if (style != plainStyle)
					font->style << " | " << plainStyle;
				font->size = fontSize;
				fFonts.AddItem(font);
			}
		} else if (section == "deskbar_settings") {
			if (field == "db:location" && parse_int(value, number)
				&& number >= 0 && number <= 5)
				fDeskbarLocation = number;
			else if (field == "db:expanded")
				fDeskbarExpanded = value.FindFirst("true") >= 0;
		} else if (section == "window_decorations") {
			if (field == "window:decor" && parse_string(value, string)
				&& !string.IsEmpty())
				decorators.Add(string);
			else if (field == "window:R5:decor" && parse_int(value, number))
				r5Decorator = number;
			else if (field == "f:Active Title" && parse_rgb(value, color)) {
				activeTitle = color;
				hasActiveTitle = true;
			} else if (field == "f:Inactive Title" && parse_rgb(value, color)) {
				inactiveTitle = color;
				hasInactiveTitle = true;
			}
		}
	}

	// colours: the theme's, the rest Haiku's defaults
	// (FindColor() clears its result when the field isn't there)
	rgb_color panel = { 216, 216, 216, 255 };
	if (colors.HasColor("PanBg"))
		colors.FindColor("PanBg", &panel);
	const bool dark = panel.red + panel.green + panel.blue < 128 * 3;
	const DefaultColor* defaults = dark ? kHaikuDarkColors : kHaikuLightColors;
	for (int32 c = 0; c < kHaikuDefaultColorCount; c++) {
		rgb_color value = defaults[c].color;
		for (size_t k = 0; k < sizeof(kUIColors) / sizeof(kUIColors[0]); k++) {
			if (kUIColors[k].which == defaults[c].which
				&& colors.HasColor(kUIColors[k].key))
				colors.FindColor(kUIColors[k].key, &value);
		}
		if (defaults[c].which == B_WINDOW_TEXT_COLOR && hasActiveTitle
			&& !colors.HasColor("WinTabTx"))
			value = activeTitle;
		if (defaults[c].which == B_WINDOW_INACTIVE_TEXT_COLOR
			&& hasInactiveTitle && !colors.HasColor("InWinTabTx"))
			value = inactiveTitle;
		if (defaults[c].which == B_DESKTOP_COLOR
			&& fDesktopColors.HasColor("color"))
			fDesktopColors.FindColor("color", &value);
		const char* name = ui_color_name(defaults[c].which);
		if (name != NULL)
			fColors.AddColor(name, value);
	}
	if (fDesktopColors.HasColor("color")) {
		fDesktopColors.FindColor("color", &fDesktopColor);
		fHasDesktopColor = true;
	}

	// the decorator: the theme's own if installed, else the nearest
	BString decorator;
	BString controlLook;
	for (int32 d = 0; d < decorators.CountStrings(); d++) {
		const BString& name = decorators.StringAt(d);
		decorator << (decorator.IsEmpty() ? "" : " | ") << name;
		const char* leaf = BPath(name).Leaf() != NULL
			? BPath(name).Leaf() : name.String();
		for (size_t a = 0;
				a < sizeof(kDecoratorAliases) / sizeof(kDecoratorAliases[0]);
				a++) {
			if (strcasecmp(leaf, kDecoratorAliases[a].decorator) != 0)
				continue;
			decorator << " | " << kDecoratorAliases[a].alternatives;
			if (controlLook.IsEmpty())
				controlLook = kDecoratorAliases[a].controlLook;
		}
	}
	if (decorators.IsEmpty() && r5Decorator >= 0
		&& r5Decorator < (int32)(sizeof(kR5Decorators) / sizeof(char*)))
		decorator = kR5Decorators[r5Decorator];
	if (!decorator.IsEmpty() && decorator != "Default")
		decorator << " | Default";
	fDecorator = decorator;
	// Theme Manager has no control looks; Haiku's own goes with them
	fControlLook = controlLook.IsEmpty() ? BString("haiku") : controlLook;

	// The backgrounds name images where the theme was first installed:
	// look for them in the theme's own folder too.
	if (!background.IsEmpty()) {
		const char* image;
		for (int32 b = 0; background.FindString(B_BACKGROUND_IMAGE, b, &image)
				== B_OK; b++) {
			BString found(image);
			if (!found.IsEmpty() && !BEntry(found).Exists()) {
				BPath local(folder, BPath(image).Leaf());
				found = BEntry(local.Path()).Exists() ? local.Path() : "";
			}
			background.ReplaceString(B_BACKGROUND_IMAGE, b, found);
		}
		fBackground = background;
	} else
		fWallpaper = "none";

	fDescription = description;
	if (!fDescription.IsEmpty() && !fDescription.EndsWith("."))
		fDescription << ".";
	if (!fDescription.IsEmpty())
		fDescription << " ";
	fDescription << "A Theme Manager theme";
	if (!decorators.IsEmpty()) {
		DecorInfoUtility installed;
		if (installed.FindDecorator(decorators.StringAt(0)) == NULL) {
			fDescription << "; its window decorator, "
				<< BPath(decorators.StringAt(0)).Leaf()
				<< ", isn't on this system, so the nearest one is used";
		}
	}
	fDescription << ".";

	fPreview = screenshot.IsEmpty() ? BString("")
		: BString(BPath(folder, screenshot).Path());
	return B_OK;
}
