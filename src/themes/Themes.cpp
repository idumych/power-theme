/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Power Theme: switches the whole look of the desktop (window decorator,
 * controls, colours, fonts and icons) between themes, and back.
 *
 * Without arguments it opens its window. From a Terminal:
 *
 *   powertheme --list
 *   powertheme --apply=NAME [--parts=decorator,controls,colors,fonts,icons]
 *   powertheme --icons=ICON-THEME  only the icons ("haiku" for Haiku's own)
 *   powertheme --revert            back to the look from before any theme
 *   powertheme --status
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <AboutWindow.h>
#include <Application.h>
#include <File.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>
#include <StringList.h>

#include "IconInstaller.h"
#include "IconTheme.h"
#include "Theme.h"
#include "ThemesWindow.h"


static const bigtime_t kScheduleCheckInterval = 30000000;


class ThemesApp : public BApplication {
public:
	ThemesApp()
		:
		BApplication(kThemeAppSignature),
		fWindow(NULL)
	{
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == kMsgThemeSwitched && fWindow != NULL)
			fWindow->PostMessage(message);
		else
			BApplication::MessageReceived(message);
	}

	virtual void ReadyToRun()
	{
		BMessage settings;
		BPath path;
		if (themes_settings_path(path, "settings") == B_OK) {
			BFile file(path.Path(), B_READ_ONLY);
			if (file.InitCheck() == B_OK)
				settings.Unflatten(&file);
		}

		BRect frame = settings.GetRect("frame", BRect());
		const char* theme = settings.GetString("theme", NULL);
		fWindow = new ThemesWindow(frame, theme);
		fWindow->Show();
	}

	virtual void AboutRequested()
	{
		BAboutWindow* about = new BAboutWindow("Power Theme", kThemeAppSignature);
		about->AddDescription("Switches the window borders, controls, "
			"colors, fonts and icons of the desktop between themes, and "
			"back. Icon themes are freedesktop icon themes.");
		about->AddCopyright(2026, "idumych");
		about->AddExtraInfo("Uses nanosvg by Mikko Mononen (zlib license).");
		about->Show();
	}

private:
	ThemesWindow*	fWindow;
};


//	#pragma mark - command line


static void
usage(FILE* out)
{
	fprintf(out,
		"Usage: powertheme [OPTION]\n"
		"Switches the look of the desktop between themes.\n"
		"Without an option, opens the Power Theme window.\n\n"
		"  -l, --list            list the installed themes and icon themes\n"
		"  -a, --apply=NAME      apply the theme called NAME\n"
		"  -p, --parts=LIST      with --apply, only these parts: decorator,\n"
		"                        controls, colors, fonts, icons, wallpaper,\n"
		"                        deskbar\n"
		"  -m, --mode=MODE       switch the theme in use to its light or dark\n"
		"                        version, or toggle between them; with --apply,\n"
		"                        apply that version (light or dark)\n"
		"      --schedule=LIGHT,DARK  switch the theme in use to its light\n"
		"                        version at LIGHT and its dark one at DARK, every\n"
		"                        day (as HH:MM, 24 hour); \"off\" stops that\n"
		"      --light-icons=ID  the icons for the theme in use's light version,\n"
		"      --dark-icons=ID   and for its dark one: an icon theme, \"haiku\", or\n"
		"                        \"keep\" to leave the icons alone. Switching modes\n"
		"                        then changes the icons too\n"
		"  -i, --icons=ID        use only the icon theme in folder ID;\n"
		"                        \"haiku\" puts Haiku's own icons back\n"
		"  -r, --revert          go back to the look from before any theme\n"
		"  -s, --status          show what is in use\n"
		"  -w, --show=NAME       show what applying the theme NAME would do\n"
		"  -t, --restart-tracker restart Tracker, which shows some new icons\n"
		"                        (folders') only after a restart\n"
		"  -b, --build-decorators build the window decorators for this system\n"
		"                        (they run inside app_server, so they must be\n"
		"                        built for the exact system build)\n"
		"      --install-tools   with --build-decorators, install the compiler\n"
		"                        first if it is missing\n"
		"  -h, --help            show this help\n");
}


static uint32
parse_parts(const char* list)
{
	uint32 parts = 0;
	BStringList names;
	BString(list).Split(",", true, names);
	for (int32 i = 0; i < names.CountStrings(); i++) {
		BString name = names.StringAt(i);
		name.Trim();
		if (name == "decorator")
			parts |= THEME_DECORATOR;
		else if (name == "controls")
			parts |= THEME_CONTROLS;
		else if (name == "colors" || name == "colours")
			parts |= THEME_COLORS;
		else if (name == "fonts")
			parts |= THEME_FONTS;
		else if (name == "icons")
			parts |= THEME_ICONS;
		else if (name == "wallpaper")
			parts |= THEME_WALLPAPER;
		else if (name == "deskbar")
			parts |= THEME_DESKBAR;
		else {
			fprintf(stderr, "powertheme: unknown part \"%s\"\n", name.String());
			return 0;
		}
	}
	return parts;
}


static void
print_log(const BString& log)
{
	if (!log.IsEmpty())
		fprintf(stderr, "%s", log.String());
}


static int
apply_icons(const char* id)
{
	BString log;
	if (is_haiku(id)) {
		revert_icons(log);
		print_log(log);
		printf("Haiku's own icons are back.\n");
		return 0;
	}

	IconTheme* theme = load_icon_theme(id);
	if (theme == NULL) {
		fprintf(stderr, "powertheme: the icon theme \"%s\" isn't installed\n",
			id);
		return 1;
	}
	int32 changed;
	status_t status = apply_icon_theme(*theme, log, changed);
	print_log(log);
	if (status == B_OK) {
		printf("Icons from \"%s\" set on %" B_PRId32 " file types and "
			"folders. Folders show theirs after Tracker restarts "
			"(powertheme --restart-tracker).\n", theme->Name().String(), changed);
	}
	delete theme;
	return status == B_OK ? 0 : 1;
}


/*!	Runs in the background, for as long as there is a schedule. It only acts
	when the mode the schedule asks for changes, so that a theme switched by hand
	stays until the next scheduled time. It starts out without one, so a system
	started at night comes up dark.
*/
class ScheduleApp : public BApplication {
public:
	ScheduleApp()
		:
		BApplication(kThemeScheduleSignature),
		fMode(THEME_MODE_UNKNOWN)
	{
	}

	virtual void ReadyToRun()
	{
		_Check();
		SetPulseRate(kScheduleCheckInterval);
	}

	virtual void Pulse()
	{
		_Check();
	}

private:
	void _Check()
	{
		ThemeSchedule schedule;
		if (load_schedule(schedule) != B_OK || !schedule.enabled) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}

		const theme_mode mode = scheduled_mode_now(schedule);
		if (mode == fMode)
			return;

		fMode = mode;
		BString log;
		apply_mode(mode, log);
		print_log(log);
	}

	theme_mode	fMode;
};


static int
set_schedule_times(const char* times)
{
	ThemeSchedule schedule;
	load_schedule(schedule);
	schedule.enabled = strcasecmp(times, "off") != 0;
	if (schedule.enabled) {
		BString text(times);
		int32 comma = text.FindFirst(',');
		BString light, dark;
		if (comma > 0) {
			text.CopyInto(light, 0, comma);
			text.CopyInto(dark, comma + 1, text.Length() - comma - 1);
		}
		if (comma <= 0 || !parse_schedule_time(light, schedule.lightAt)
			|| !parse_schedule_time(dark, schedule.darkAt)) {
			fprintf(stderr, "powertheme: the schedule is LIGHT,DARK as HH:MM, "
				"such as 07:00,19:00, or off\n");
			return 2;
		}
		if (schedule.lightAt == schedule.darkAt) {
			fprintf(stderr, "powertheme: light and dark need different "
				"times\n");
			return 2;
		}
	}

	status_t status = set_schedule(schedule);
	if (status != B_OK) {
		fprintf(stderr, "powertheme: can't set the schedule: %s\n",
			strerror(status));
		return 1;
	}
	if (schedule.enabled)
		printf("%s\n", schedule_description(schedule).String());
	else
		printf("The schedule is off.\n");
	return 0;
}


/*!	Chooses the icons for the theme in use's version in \a mode, and uses them
	right away if that's the version in use.
*/
static int
set_mode_icons(theme_mode mode, const char* id)
{
	BString name, inUse;
	get_current(name, inUse);
	ThemeList themes;
	find_themes(themes);
	const Theme* current = find_theme(themes, name);
	const Theme* variant = current != NULL
		? find_variant(themes, *current, mode) : NULL;
	if (variant == NULL) {
		fprintf(stderr, "powertheme: the theme in use has no %s version\n",
			mode_name(mode));
		return 1;
	}

	const bool keep = strcasecmp(id, "keep") == 0;
	if (!keep && !is_haiku(id)) {
		IconTheme* icons = load_icon_theme(id);
		if (icons == NULL) {
			fprintf(stderr, "powertheme: the icon theme \"%s\" isn't "
				"installed\n", id);
			return 1;
		}
		delete icons;
	}

	set_theme_icons(*variant, keep ? "keep" : id);
	set_icons_follow_mode(true);
	printf("\"%s\" now uses %s.\n", variant->Name().String(),
		keep ? "whatever icons are in use" : id);
	if (!keep && variant == current)
		return apply_icons(id);
	return 0;
}


static int
command_line(int argc, char** argv)
{
	static const struct option kOptions[] = {
		{ "list", no_argument, NULL, 'l' },
		{ "apply", required_argument, NULL, 'a' },
		{ "parts", required_argument, NULL, 'p' },
		{ "icons", required_argument, NULL, 'i' },
		{ "revert", no_argument, NULL, 'r' },
		{ "status", no_argument, NULL, 's' },
		{ "show", required_argument, NULL, 'w' },
		{ "mode", required_argument, NULL, 'm' },
		{ "schedule", required_argument, NULL, 'S' },
		{ "light-icons", required_argument, NULL, 'L' },
		{ "dark-icons", required_argument, NULL, 'D' },
		{ "restart-tracker", no_argument, NULL, 't' },
		{ "build-decorators", no_argument, NULL, 'b' },
		{ "install-tools", no_argument, NULL, 'T' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	const char* apply = NULL;
	const char* icons = NULL;
	uint32 parts = THEME_ALL;
	bool list = false, revert = false, status = false, restart = false;
	bool build = false, installTools = false;
	const char* show = NULL;
	const char* mode = NULL;
	const char* scheduleTimes = NULL;
	const char* lightIcons = NULL;
	const char* darkIcons = NULL;

	int option;
	while ((option = getopt_long(argc, argv, "la:p:m:i:rstw:bh", kOptions, NULL))
			!= -1) {
		switch (option) {
			case 'l': list = true; break;
			case 'a': apply = optarg; break;
			case 'p':
				parts = parse_parts(optarg);
				if (parts == 0)
					return 2;
				break;
			case 'm': mode = optarg; break;
			case 'S': scheduleTimes = optarg; break;
			case 'L': lightIcons = optarg; break;
			case 'D': darkIcons = optarg; break;
			case 'i': icons = optarg; break;
			case 'r': revert = true; break;
			case 's': status = true; break;
			case 't': restart = true; break;
			case 'w': show = optarg; break;
			case 'b': build = true; break;
			case 'T': installTools = true; break;
			case 'h': usage(stdout); return 0;
			default: usage(stderr); return 2;
		}
	}
	if (optind < argc
		|| (!list && !apply && !icons && !revert && !status && !restart
			&& !show && !build && !mode && !scheduleTimes && !lightIcons
			&& !darkIcons)) {
		usage(stderr);
		return 2;
	}

	const bool toggle = mode != NULL && strcasecmp(mode, "toggle") == 0;
	const theme_mode wantedMode = mode != NULL ? mode_from_name(mode)
		: THEME_MODE_UNKNOWN;
	if (mode != NULL && !toggle && wantedMode == THEME_MODE_UNKNOWN) {
		fprintf(stderr, "powertheme: the mode is light, dark or toggle, not "
			"\"%s\"\n", mode);
		return 2;
	}
	if (toggle && apply != NULL) {
		fprintf(stderr, "powertheme: --apply takes --mode=light or dark\n");
		return 2;
	}

	// colours, fonts and bitmaps need a connection to app_server
	BApplication app(kThemeAppSignature);

	if (build) {
		BString script;
		if (find_build_script(script) != B_OK) {
			fprintf(stderr, "powertheme: build-decorators.sh isn't installed\n");
			return 1;
		}
		// the script's progress lines are for the window; here they are
		// left as they are, which reads well enough
		fflush(stdout);
		execl("/bin/sh", "sh", script.String(),
			installTools ? "--install-tools" : (char*)NULL, (char*)NULL);
		fprintf(stderr, "powertheme: can't run %s: %s\n", script.String(),
			strerror(errno));
		return 1;
	}

	if (restart) {
		status_t result = restart_tracker();
		if (result != B_OK) {
			fprintf(stderr, "powertheme: can't restart Tracker: %s\n",
				strerror(result));
			return 1;
		}
		if (!list && !apply && !icons && !revert && !status)
			return 0;
	}

	if (show != NULL) {
		ThemeList themes;
		find_themes(themes);
		int found = 0;
		for (int32 i = 0; i < themes.CountItems(); i++) {
			if (themes.ItemAt(i)->Name().ICompare(show) != 0)
				continue;
			BString out;
			describe_theme(*themes.ItemAt(i), out);
			printf("%s", out.String());
			found++;
		}
		if (found == 0) {
			fprintf(stderr, "powertheme: no theme is called \"%s\" (see "
				"--list)\n", show);
			return 1;
		}
		return 0;
	}

	if (list) {
		BString currentTheme, currentIcons;
		get_current(currentTheme, currentIcons);
		ThemeList themes;
		find_themes(themes);
		printf("Themes:\n");
		for (int32 i = 0; i < themes.CountItems(); i++) {
			BStringList missing;
			themes.ItemAt(i)->Missing(&missing);
			BString note;
			if (!themes.ItemAt(i)->VariantOf().IsEmpty()) {
				note << " (" << mode_name(themes.ItemAt(i)->Mode())
					<< " version of " << themes.ItemAt(i)->VariantOf() << ")";
			}
			if (!missing.IsEmpty())
				note << " (not installed: " << missing.Join(", ") << ")";
			printf("  %s%s%s\n", themes.ItemAt(i)->Name().String(),
				themes.ItemAt(i)->Name() == currentTheme ? " (in use)" : "",
				note.String());
		}
		IconThemeList iconThemes;
		find_icon_themes(iconThemes);
		printf("Icon themes (--icons=ID):\n");
		for (int32 i = 0; i < iconThemes.CountItems(); i++) {
			const IconTheme* theme = iconThemes.ItemAt(i);
			printf("  %-20s %s%s\n", theme->ID().String(),
				theme->Name().String(),
				theme->ID() == currentIcons ? " (in use)" : "");
		}
		return 0;
	}

	if (status) {
		BString currentTheme, currentIcons;
		get_current(currentTheme, currentIcons);
		printf("theme: %s\n", currentTheme.IsEmpty()
			? "(none)" : currentTheme.String());
		printf("mode: %s\n", mode_name(current_mode()));
		ThemeSchedule schedule;
		load_schedule(schedule);
		if (schedule.enabled) {
			printf("schedule: light at %02" B_PRId32 ":%02" B_PRId32 ", dark at "
				"%02" B_PRId32 ":%02" B_PRId32 "\n", schedule.lightAt / 60,
				schedule.lightAt % 60, schedule.darkAt / 60,
				schedule.darkAt % 60);
		} else
			printf("schedule: off\n");
		printf("icons: %s\n", currentIcons.IsEmpty()
			? "(Haiku's own)" : currentIcons.String());
		ThemeList themes;
		find_themes(themes);
		const Theme* current = find_theme(themes, currentTheme);
		if (current != NULL && icons_follow_mode()) {
			const theme_mode modes[] = { THEME_MODE_LIGHT, THEME_MODE_DARK };
			for (int32 i = 0; i < 2; i++) {
				const Theme* variant = find_variant(themes, *current, modes[i]);
				if (variant != NULL) {
					printf("%s icons: %s\n", mode_name(modes[i]),
						theme_icons(*variant).String());
				}
			}
		}
		printf("previous look saved: %s\n",
			has_saved_appearance() ? "yes" : "no");
		printf("previous icons saved: %s\n",
			has_saved_icons() ? "yes" : "no");
		return 0;
	}

	if (scheduleTimes != NULL)
		return set_schedule_times(scheduleTimes);

	if (lightIcons != NULL || darkIcons != NULL) {
		int result = 0;
		if (lightIcons != NULL)
			result = set_mode_icons(THEME_MODE_LIGHT, lightIcons);
		if (darkIcons != NULL && result == 0)
			result = set_mode_icons(THEME_MODE_DARK, darkIcons);
		return result;
	}

	if (mode != NULL && apply == NULL) {
		theme_mode target = wantedMode;
		if (toggle) {
			target = opposite_mode(current_mode());
			if (target == THEME_MODE_UNKNOWN) {
				fprintf(stderr, "powertheme: can't tell whether the theme in "
					"use is light or dark\n");
				return 1;
			}
		}

		BString log, applied;
		bool iconsChanged;
		status_t result = apply_mode(target, log, &applied, &iconsChanged);
		print_log(log);
		if (result != B_OK)
			return 1;
		if (applied.IsEmpty())
			printf("The theme in use is %s already.\n", mode_name(target));
		else {
			printf("\"%s\" applied. %s\n", applied.String(),
				running_applications_note());
		}
		if (iconsChanged) {
			printf("Folders show their new icons after Tracker restarts "
				"(powertheme --restart-tracker).\n");
		}
		return 0;
	}

	if (revert) {
		BString log;
		revert_appearance(log);
		revert_icons(log);
		print_log(log);
		printf("Your previous look is back. %s\n",
			running_applications_note());
		return 0;
	}

	if (apply != NULL) {
		ThemeList themes;
		find_themes(themes);
		const Theme* theme = find_theme(themes, apply, wantedMode);
		if (theme == NULL) {
			if (mode != NULL && find_theme(themes, apply) != NULL) {
				fprintf(stderr, "powertheme: \"%s\" has no %s version\n", apply,
					mode);
			} else {
				fprintf(stderr, "powertheme: no theme is called \"%s\" (see "
					"--list)\n", apply);
			}
			return 1;
		}

		const bool wantsIcons = (parts & THEME_ICONS) != 0;

		// leave out what isn't installed, saying so
		BStringList missing;
		parts &= ~theme->Missing(&missing);
		if (!missing.IsEmpty()) {
			fprintf(stderr, "Not installed, so left out: %s.\n",
				missing.Join(", ").String());
		}

		BString log;
		apply_appearance(*theme, parts, log);
		print_log(log);
		int result = 0;
		if (icons != NULL) {
			// so that switching back to this version brings them back
			if (!theme->VariantOf().IsEmpty())
				set_theme_icons(*theme, is_haiku(icons) ? "haiku" : icons);
			set_icons_follow_mode(true);
		} else {
			// what was chosen for this theme, or what it names if that's
			// installed
			const BString wanted = theme_icons(*theme);
			const bool useIcons = wantsIcons && wanted != "keep"
				&& (wanted != theme->Icons() || (parts & THEME_ICONS) != 0);
			set_icons_follow_mode(useIcons);
			if (useIcons)
				result = apply_icons(wanted);
		}
		printf("\"%s\" applied. %s\n", theme->Name().String(),
			running_applications_note());
		if (icons == NULL)
			return result;
	} else if (icons != NULL) {
		// icons of their own, which a change of mode shouldn't replace
		set_icons_follow_mode(false);
	}

	if (icons != NULL)
		return apply_icons(icons);
	return 0;
}


int
main(int argc, char** argv)
{
	if (argc == 2 && strcmp(argv[1], "--run-schedule") == 0) {
		if (be_roster->IsRunning(kThemeScheduleSignature))
			return 0;
		ScheduleApp app;
		app.Run();
		return 0;
	}

	if (argc > 1)
		return command_line(argc, argv);

	// The command line runs beside the window, so Themes can't be a single
	// launch application; there is only one window all the same.
	BList running;
	be_roster->GetAppList(kThemeAppSignature, &running);
	if (!running.IsEmpty()) {
		be_roster->ActivateApp((team_id)(addr_t)running.ItemAt(0));
		return 0;
	}

	ThemesApp app;
	app.Run();
	return 0;
}
