/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The Themes window: installed themes on the left; on the right a preview,
 * the icons to use with the theme, and which parts of it to apply.
 */
#ifndef THEMES_WINDOW_H
#define THEMES_WINDOW_H


#include <Window.h>

#include "IconTheme.h"
#include "Theme.h"


class BButton;
class BCheckBox;
class BListView;
class BMenuField;
class BStatusBar;
class BStringView;
class DescriptionView;
class IconStripView;
class PreviewView;


class ThemesWindow : public BWindow {
public:
								ThemesWindow(BRect frame,
									const char* selected);
	virtual						~ThemesWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				WindowActivated(bool active);
	virtual	bool				QuitRequested();

private:
	static	status_t			_ApplyThread(void* data);
	static	status_t			_RevertThread(void* data);
	static	status_t			_RestartTrackerThread(void* data);

			void				_LoadThemes(const char* selected);
			void				_UpdateLabels();
			void				_SelectTheme(int32 index);
			void				_SelectIcons(const char* id);
			BString				_ChosenIcons() const;
			void				_SetBusy(bool busy);
			void				_UpdateButtons();

			ThemeList			fThemes;
			IconThemeList		fIconThemes;
			bool				fBusy;

			BListView*			fThemeList;
			PreviewView*		fPreview;
			DescriptionView*	fDescription;
			BMenuField*			fIconsField;
			IconStripView*		fIconStrip;
			BCheckBox*			fDecorator;
			BCheckBox*			fControls;
			BCheckBox*			fColors;
			BCheckBox*			fFonts;
			BCheckBox*			fWallpaper;
			BCheckBox*			fDeskbar;
			BStatusBar*			fProgress;
			BStringView*		fStatus;
			BButton*			fRestartTracker;
			BButton*			fRevert;
			BButton*			fApply;
};


#endif	// THEMES_WINDOW_H
