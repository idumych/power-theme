/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Themes: switches the whole look of the desktop (window decorator,
 * controls, colours, fonts and icons) between themes, and back.
 *
 * Without arguments it opens its window. From a Terminal:
 *
 *   Themes --list
 *   Themes --apply=NAME [--parts=decorator,controls,colors,fonts,icons]
 *   Themes --icons=ICON-THEME     only the icons ("haiku" for Haiku's own)
 *   Themes --revert               back to the look from before any theme
 *   Themes --status
 */


#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <AboutWindow.h>
#include <Application.h>
#include <File.h>
#include <Path.h>
#include <Roster.h>
#include <StringList.h>

#include "IconInstaller.h"
#include "IconTheme.h"
#include "Theme.h"
#include "ThemesWindow.h"


static const char* kSignature = "application/x-vnd.idumych-Themes";


class ThemesApp : public BApplication {
public:
	ThemesApp()
		:
		BApplication(kSignature)
	{
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
		ThemesWindow* window = new ThemesWindow(frame, theme);
		window->Show();
	}

	virtual void AboutRequested()
	{
		BAboutWindow* about = new BAboutWindow("Themes", kSignature);
		about->AddDescription("Switches the window borders, controls, "
			"colors, fonts and icons of the desktop between themes, and "
			"back. Icon themes are freedesktop icon themes.");
		about->AddCopyright(2026, "idumych");
		about->AddExtraInfo("Uses nanosvg by Mikko Mononen (zlib license).");
		about->Show();
	}
};


//	#pragma mark - command line


static void
usage(FILE* out)
{
	fprintf(out,
		"Usage: Themes [OPTION]\n"
		"Switches the look of the desktop between themes.\n"
		"Without an option, opens the Themes window.\n\n"
		"  -l, --list            list the installed themes and icon themes\n"
		"  -a, --apply=NAME      apply the theme called NAME\n"
		"  -p, --parts=LIST      with --apply, only these parts: decorator,\n"
		"                        controls, colors, fonts, icons, wallpaper,\n"
		"                        deskbar\n"
		"  -i, --icons=ID        use only the icon theme in folder ID;\n"
		"                        \"haiku\" puts Haiku's own icons back\n"
		"  -r, --revert          go back to the look from before any theme\n"
		"  -s, --status          show what is in use\n"
		"  -w, --show=NAME       show what applying the theme NAME would do\n"
		"  -t, --restart-tracker restart Tracker, which shows some new icons\n"
		"                        (folders') only after a restart\n"
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
			fprintf(stderr, "Themes: unknown part \"%s\"\n", name.String());
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
		fprintf(stderr, "Themes: the icon theme \"%s\" isn't installed\n",
			id);
		return 1;
	}
	int32 changed;
	status_t status = apply_icon_theme(*theme, log, changed);
	print_log(log);
	if (status == B_OK) {
		printf("Icons from \"%s\" set on %" B_PRId32 " file types and "
			"folders. Folders show theirs after Tracker restarts "
			"(Themes --restart-tracker).\n", theme->Name().String(), changed);
	}
	delete theme;
	return status == B_OK ? 0 : 1;
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
		{ "restart-tracker", no_argument, NULL, 't' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	const char* apply = NULL;
	const char* icons = NULL;
	uint32 parts = THEME_ALL;
	bool list = false, revert = false, status = false, restart = false;
	const char* show = NULL;

	int option;
	while ((option = getopt_long(argc, argv, "la:p:i:rstw:h", kOptions, NULL))
			!= -1) {
		switch (option) {
			case 'l': list = true; break;
			case 'a': apply = optarg; break;
			case 'p':
				parts = parse_parts(optarg);
				if (parts == 0)
					return 2;
				break;
			case 'i': icons = optarg; break;
			case 'r': revert = true; break;
			case 's': status = true; break;
			case 't': restart = true; break;
			case 'w': show = optarg; break;
			case 'h': usage(stdout); return 0;
			default: usage(stderr); return 2;
		}
	}
	if (optind < argc
		|| (!list && !apply && !icons && !revert && !status && !restart
			&& !show)) {
		usage(stderr);
		return 2;
	}

	// colours, fonts and bitmaps need a connection to app_server
	BApplication app(kSignature);

	if (restart) {
		status_t result = restart_tracker();
		if (result != B_OK) {
			fprintf(stderr, "Themes: can't restart Tracker: %s\n",
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
			fprintf(stderr, "Themes: no theme is called \"%s\" (see "
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
		printf("icons: %s\n", currentIcons.IsEmpty()
			? "(Haiku's own)" : currentIcons.String());
		printf("previous look saved: %s\n",
			has_saved_appearance() ? "yes" : "no");
		printf("previous icons saved: %s\n",
			has_saved_icons() ? "yes" : "no");
		return 0;
	}

	if (revert) {
		BString log;
		revert_appearance(log);
		revert_icons(log);
		print_log(log);
		printf("Your previous look is back. Restart running applications "
			"to see it everywhere.\n");
		return 0;
	}

	if (apply != NULL) {
		ThemeList themes;
		find_themes(themes);
		const Theme* theme = NULL;
		for (int32 i = 0; i < themes.CountItems(); i++) {
			if (themes.ItemAt(i)->Name().ICompare(apply) == 0)
				theme = themes.ItemAt(i);
		}
		if (theme == NULL) {
			fprintf(stderr, "Themes: no theme is called \"%s\" (see "
				"--list)\n", apply);
			return 1;
		}

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
		if ((parts & THEME_ICONS) != 0 && !theme->Icons().IsEmpty()
				&& icons == NULL)
			result = apply_icons(theme->Icons());
		printf("\"%s\" applied. Restart running applications to see the "
			"new controls and fonts in them.\n", theme->Name().String());
		if (icons == NULL)
			return result;
	}

	if (icons != NULL)
		return apply_icons(icons);
	return 0;
}


int
main(int argc, char** argv)
{
	if (argc > 1)
		return command_line(argc, argv);

	// The command line runs beside the window, so Themes can't be a single
	// launch application; there is only one window all the same.
	BList running;
	be_roster->GetAppList(kSignature, &running);
	if (!running.IsEmpty()) {
		be_roster->ActivateApp((team_id)(addr_t)running.ItemAt(0));
		return 0;
	}

	ThemesApp app;
	app.Run();
	return 0;
}
