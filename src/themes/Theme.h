/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * A theme: a folder in a "themes" data folder holding theme.settings, and
 * optionally preview.png. theme.settings has one "key = value" per line:
 *
 *   name = Reactionary ME
 *   description = Windows ME style windows and controls
 *   decorator = Reactionary
 *   control look = Reactionary
 *   icons = ClassicOS2000
 *   desktop color = #3A6EA5
 *   wallpaper = desktop.png
 *   wallpaper mode = scaled
 *   deskbar = right top, expanded
 *   font plain = Noto Sans, Regular, 10
 *   font bold = Noto Sans Display | Noto Sans, Medium | Bold, 12
 *   color B_PANEL_BACKGROUND_COLOR = #D4D0C8
 *
 * A font's family and style may list alternatives, separated by "|"; the
 * first that is installed is used. Every key is optional; what a theme leaves out stays as it is. "icons"
 * names an icon theme folder (see IconTheme.h). "control look = haiku"
 * is Haiku's built-in look, and "icons = haiku" Haiku's own icons.
 * "wallpaper" is an image in the theme's folder (or a full path), "none"
 * for none, or "default" for the one Tracker sets up on a new system;
 * "wallpaper mode" is scaled (the default), centered, tiled or manual
 * (the image's top left corner at the screen's). "deskbar" places the
 * Deskbar: top, bottom, left top, right top, left bottom or right bottom,
 * optionally followed by ", expanded" or ", collapsed".
 *
 * Themes made for Theme Manager (HaikuThemeManager, and BeOS and ZETA
 * before it) are read too, from the "UIThemes" folders it uses; see
 * LoadUITheme(). A theme's decorator and control look may list
 * alternatives, separated by "|", like fonts.
 *
 * Applying a theme first saves the look in use, once, so "revert" always
 * gets back to what was there before any theme.
 */
#ifndef THEME_H
#define THEME_H


#include <InterfaceDefs.h>
#include <Message.h>
#include <ObjectList.h>
#include <String.h>


class BPath;
class BStringList;


enum {
	THEME_DECORATOR		= 1 << 0,
	THEME_CONTROLS		= 1 << 1,
	THEME_COLORS		= 1 << 2,
		// system colours and the desktop colour
	THEME_FONTS			= 1 << 3,
	THEME_ICONS			= 1 << 4,
	THEME_WALLPAPER		= 1 << 5,
	THEME_DESKBAR		= 1 << 6,
	THEME_ALL			= 0xff
};


struct ThemeFont {
	BString		which;
		// "plain", "bold", "fixed" or "menu"
	BString		family;
	BString		style;
	float		size;
};


class Theme {
public:
								Theme();

			status_t			Load(const char* folder);
			// a Theme Manager theme: a folder with a "Theme" file
			status_t			LoadUITheme(const char* folder);
			bool				IsUITheme() const { return fUITheme; }
			void				SetName(const char* name) { fName = name; }

			const BString&		Name() const { return fName; }
			const BString&		Description() const { return fDescription; }
			const BString&		Folder() const { return fFolder; }
			const BString&		Decorator() const { return fDecorator; }
			const BString&		ControlLook() const { return fControlLook; }
			const BString&		Icons() const { return fIcons; }
			bool				HasWallpaper() const
									{ return !fWallpaper.IsEmpty()
										|| !fBackground.IsEmpty(); }
			// a Theme Manager theme's backgrounds, one or more workspaces
			// each, as Tracker stores them, and its desktop colours by
			// workspace
			const BMessage&		Background() const { return fBackground; }
			const BMessage&		DesktopColors() const
									{ return fDesktopColors; }
			// the Deskbar's place, a deskbar_location, or -1 for none
			int32				DeskbarLocation() const
									{ return fDeskbarLocation; }
			bool				DeskbarExpanded() const
									{ return fDeskbarExpanded; }
			const BString&		Wallpaper() const { return fWallpaper; }
				// "none", "default", or the image's full path
			int32				WallpaperMode() const
									{ return fWallpaperMode; }
			BString				PreviewPath() const;

			bool				HasDesktopColor() const
									{ return fHasDesktopColor; }
			rgb_color			DesktopColor() const { return fDesktopColor; }
			const BMessage&		Colors() const { return fColors; }
				// ui_color_name() -> rgb_color
			int32				CountFonts() const
									{ return fFonts.CountItems(); }
			const ThemeFont*	FontAt(int32 index) const
									{ return fFonts.ItemAt(index); }

			// what it changes, as THEME_* flags
			uint32				Parts() const;
			// the parts whose add-ons, icons or images aren't installed, as
			// THEME_* flags, with a line for the user about each in \a what
			uint32				Missing(BStringList* what = NULL) const;

private:
			BString				fName;
			BString				fDescription;
			BString				fFolder;
			BString				fDecorator;
			BString				fControlLook;
			BString				fIcons;
			BString				fWallpaper;
			int32				fWallpaperMode;
			BString				fPreview;
			BMessage			fBackground;
			BMessage			fDesktopColors;
			int32				fDeskbarLocation;
			bool				fDeskbarExpanded;
			bool				fUITheme;
			bool				fHasDesktopColor;
			rgb_color			fDesktopColor;
			BMessage			fColors;
			BObjectList<ThemeFont, true> fFonts;
};


typedef BObjectList<Theme, true> ThemeList;


// every installed theme, sorted by name; user folders win over system ones
void		find_themes(ThemeList& themes);

// Applies \a parts of \a theme other than icons; saves the look in use
// first if nothing is saved yet. Messages for the user go to \a log.
status_t	apply_appearance(const Theme& theme, uint32 parts, BString& log);

// What applying \a theme would do on this system, line by line: the
// decorator and control look found, fonts, colours and wallpapers.
void		describe_theme(const Theme& theme, BString& out);

// Puts back the saved look, decorator to fonts; icons are separate.
status_t	revert_appearance(BString& log);

bool		has_saved_appearance();

// the folder for Themes' own state: ~/config/settings/Themes
status_t	themes_settings_path(BPath& path, const char* leaf = NULL);

// the data folders to look for themes, icon themes and the icon map in,
// user ones first
void		data_directories(BStringList& directories);

// Records which theme and icon theme were applied last.
void		set_current(const char* theme, const char* icons);
void		get_current(BString& theme, BString& icons);

// Quits and restarts Tracker, which keeps some icons (folders') until then.
status_t	restart_tracker();

// whether a theme's "control look" or "icons" value means Haiku's own
bool		is_haiku(const char* value);

// "#RRGGBB" to a colour
bool		parse_color(const char* text, rgb_color& color);


#endif	// THEME_H
