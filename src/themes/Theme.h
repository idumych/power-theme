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
 *   variant of = Reactionary
 *   mode = light
 *   desktop color = #3A6EA5
 *   wallpaper = desktop.png
 *   wallpaper mode = scaled
 *   deskbar = right top, expanded
 *   scroll bar arrows = double
 *   font smoothing = grey
 *   shell = photon
 *   font plain = Noto Sans, Regular, 10
 *   font bold = Noto Sans Display | Noto Sans, Medium | Bold, 12
 *   color B_PANEL_BACKGROUND_COLOR = #D4D0C8
 *
 * A font's family and style may list alternatives, separated by "|"; the
 * first that is installed is used. Every key is optional; what a theme leaves out stays as it is. "icons"
 * names an icon theme folder (see IconTheme.h). "control look = haiku"
 * is Haiku's built-in look, and "icons = haiku" Haiku's own icons.
 * "icons = haiku-dark" is dark versions of them (see DarkIcons.h).
 * "wallpaper" is an image in the theme's folder (or a full path), "none"
 * for none, or "default" for the one Tracker sets up on a new system;
 * "wallpaper mode" is scaled (the default), centered, tiled or manual
 * (the image's top left corner at the screen's). "deskbar" places the
 * Deskbar: top, bottom, left top, right top, left bottom or right bottom,
 * optionally followed by ", expanded" or ", collapsed". "font smoothing =
 * grey" turns off subpixel font smoothing while the theme is in use, for a
 * pixel font whose squares must stay sharp; a theme without it brings back the
 * setting from before any theme. "shell = photon" puts PowerShelf, a taskbar
 * and shelf in the style of QNX's Photon, in the Deskbar's place (see
 * photon_shell()), and "shell = deskbar" or a theme without the key brings
 * the Deskbar back; it goes with the Deskbar's place. Turned on by hand, the
 * shell is drawn in the theme's look, until a theme without it is applied.
 *
 * A theme can be one version of another: "variant of = Flat" and "mode =
 * dark". Two themes that say they are the same theme's light and dark versions
 * are shown as one, and can be switched between with apply_mode(). Each names
 * its own "icons", so a dark version can use a dark icon theme.
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


// the application, and what it is told when apply_mode() switches the theme
extern const char* const kThemeAppSignature;
enum {
	kMsgThemeSwitched = 'tswi'
};


enum theme_mode {
	THEME_MODE_UNKNOWN = 0,
	THEME_MODE_LIGHT,
	THEME_MODE_DARK
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
			// which theme this is a version of, if any
			const BString&		VariantOf() const { return fVariantOf; }
			// what it says it is, or else what its colours make it: light
			// text on a darker panel is dark
			theme_mode			Mode() const;
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
	// 2 or 1 at each end of a scroll bar; 0 if the theme doesn't say, which
	// keeps the setting from before any theme
			int32				ScrollBarArrows() const
									{ return fScrollBarArrows; }
			// whether the theme wants fonts smoothed in grey only, without
			// the coloured edges of subpixel smoothing: for pixel fonts
			bool				GreyFontSmoothing() const
									{ return fGreyFontSmoothing; }
			// PowerShelf in the Deskbar's place (1), the Deskbar (0), or
			// whichever is in use (-1)
			int32				Shell() const
									{ return fShell; }
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
			// which of \a parts have add-ons, icons or images that aren't
			// installed, as THEME_* flags, with a line for the user about each
			// in \a what
			uint32				Missing(BStringList* what = NULL,
									uint32 parts = THEME_ALL) const;

private:
			BString				fName;
			BString				fDescription;
			BString				fFolder;
			BString				fDecorator;
			BString				fControlLook;
			BString				fIcons;
			BString				fVariantOf;
			theme_mode			fMode;
			BString				fWallpaper;
			int32				fWallpaperMode;
			BString				fPreview;
			BMessage			fBackground;
			BMessage			fDesktopColors;
			int32				fDeskbarLocation;
			bool				fDeskbarExpanded;
			int32				fScrollBarArrows;
			bool				fGreyFontSmoothing;
			int32				fShell;
			bool				fUITheme;
			bool				fHasDesktopColor;
			rgb_color			fDesktopColor;
			BMessage			fColors;
			BObjectList<ThemeFont, true> fFonts;
};


typedef BObjectList<Theme, true> ThemeList;


// every installed theme, sorted by name; user folders win over system ones
void		find_themes(ThemeList& themes);

// The other mode: light for dark, and dark for light.
theme_mode	opposite_mode(theme_mode mode);
// "light", "dark" or "unknown", and back
const char*	mode_name(theme_mode mode);
theme_mode	mode_from_name(const char* name);

// The version of \a theme in \a mode: \a theme itself if that's its mode, or
// the theme with the same VariantOf() in that mode. NULL if there's none.
const Theme* find_variant(const ThemeList& themes, const Theme& theme,
				theme_mode mode);
// By name, or by the name of what it's a version of, in \a mode if that isn't
// THEME_MODE_UNKNOWN. A family's light version comes first otherwise.
const Theme* find_theme(const ThemeList& themes, const char* name,
				theme_mode mode = THEME_MODE_UNKNOWN);

// Applies \a parts of \a theme other than icons; saves the look in use
// first if nothing is saved yet. Messages for the user go to \a log.
status_t	apply_appearance(const Theme& theme, uint32 parts, BString& log);

// The theme in use's mode, or THEME_MODE_UNKNOWN without one.
theme_mode	current_mode();

// Switches the theme in use to its version in \a mode, with the same parts as
// when it was applied. This is what anything choosing a mode should call,
// whatever it goes by. Icons change too if icons_follow_mode(). \a applied gets
// the name of the theme applied, and stays empty if it was in that mode already;
// \a iconsChanged says whether the icons changed. Returns an error, with a line
// for the user in \a log, if no theme is in use or it has no version in that
// mode.
status_t	apply_mode(theme_mode mode, BString& log, BString* applied = NULL,
				bool* iconsChanged = NULL);

// The icons to use with \a theme: those chosen for it, or else those it names.
// An icon theme's folder name, "haiku", or "keep" to leave the icons alone.
BString		theme_icons(const Theme& theme);
void		set_theme_icons(const Theme& theme, const char* icons);

// Whether switching modes changes the icons as well. It only does once icons
// have been applied with a theme, so that icons set apart from one stay.
bool		icons_follow_mode();
void		set_icons_follow_mode(bool follow);

// When to switch the theme in use between its versions: light from \a lightAt
// and dark from \a darkAt, in minutes after midnight, local time.
struct ThemeSchedule {
	bool		enabled;
	int32		lightAt;
	int32		darkAt;
};

// the background process that keeps to the schedule
extern const char* const kThemeSchedulePort;

// What finds and stops that process: a port on Haiku, and on Hokku, whose
// ports are each process's own, a locked file with its process ID.
// claim_schedule_process() makes this process the one, or returns false if
// another is; wait_for_schedule_stop() waits up to \a timeout for the call
// to stop, and returns whether it came.
bool		schedule_process_running();
void		stop_schedule_process();
bool		claim_schedule_process();
bool		wait_for_schedule_stop(bigtime_t timeout);
void		release_schedule_process();
// Runs \a program with \a argument, in a session of its own, and waits
// for it if \a wait.
status_t	run_program(const char* program, const char* argument, bool wait);

status_t	load_schedule(ThemeSchedule& schedule);
// Saves it, installs or removes the launch_daemon job for the next login, and
// starts or stops the background process. Needs a running BApplication, of this
// program. A running process picks up new times by itself.
status_t	set_schedule(const ThemeSchedule& schedule);
// "HH:MM" in minutes after midnight, or false
bool		parse_schedule_time(const char* text, int32& minutes);
// the mode \a schedule asks for at \a minutes after midnight
theme_mode	scheduled_mode(const ThemeSchedule& schedule, int32 minutes);
// the same, now
theme_mode	scheduled_mode_now(const ThemeSchedule& schedule);
// "Light at 07:00, dark at 19:00, every day."
BString		schedule_description(const ThemeSchedule& schedule);

// Whether window borders that can draw a BeOS-style tab do, instead of a title
// bar across the window.
bool		beos_tabs();
// Changes that, and reloads the window borders in use if they can draw one.
status_t	set_beos_tabs(bool on, BString& log);
// whether the window borders \a theme uses can draw one
bool		has_beos_tabs(const Theme& theme);

// Whether PowerShelf, beside this program, is in the Deskbar's place: a
// taskbar and a shelf of launchers in the style of QNX's Photon.
bool		photon_shell();
// Puts it there, from the next login too, or brings the Deskbar back. Not on
// Hokku.
status_t	set_photon_shell(bool on, BString& log);
// Whether PowerShelf is running now, however it was started.
bool		photon_shell_running();

// What starts PowerShelf at login: launch_daemon's service, which starts it
// again if it ends; an older Power Theme's job, which doesn't; or nothing.
// The user's launch folder is looked in, and the folders a system build can
// ship one in.
enum shell_entry {
	SHELL_ENTRY_SERVICE,
	SHELL_ENTRY_JOB,
	SHELL_ENTRY_NONE
};
shell_entry	photon_shell_entry();
// Whether to offer to add that entry when the shell runs without it.
bool		ask_for_shell_entry();
void		set_ask_for_shell_entry(bool ask);

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

// What to tell users about the applications already running after a change of
// theme. Newer systems change their controls and fonts as they run; older ones
// only do so after a restart.
const char*	running_applications_note();

// Quits and restarts Tracker, which keeps some icons (folders') until then.
status_t	restart_tracker();

// The script that builds the window decorators for the running system
// (build-decorators.sh): in a data folder's "Themes", or next to Themes.
status_t	find_build_script(BString& path);

// Whether the window decorators and control looks have to be built before a
// theme can be applied: when they never were and some are missing, or when
// they were built for another system build (as after a system update). \a why
// says which.
bool		addons_need_building(BString* why = NULL);
// Whether build-decorators.sh has built the add-ons here before.
bool		addons_built_before();

// Applies the window decorator and control look of the theme in use again,
// if it was applied with them, as after building them for a new system.
status_t	reapply_addons(BString& log);

#ifndef APP_COSMOE
// Writes the launch_daemon job that runs "\a program --after-update" at every
// login, which builds the add-ons again after a system update.
status_t	set_after_update_job(const char* program);
#endif

// whether a theme's "control look" or "icons" value means Haiku's own
bool		is_haiku(const char* value);

// what "icons" is for dark versions of Haiku's own, and whether it's that
extern const char* const kDarkHaikuIcons;
bool		is_dark_haiku(const char* value);

// "#RRGGBB" to a colour
bool		parse_color(const char* text, rgb_color& color);


#endif	// THEME_H
