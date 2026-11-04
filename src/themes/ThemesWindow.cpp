/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The Power Theme window. See ThemesWindow.h.
 */


#include "ThemesWindow.h"

#include <stdio.h>

#include <algorithm>
#include <vector>

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <File.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <Node.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringItem.h>
#include <StringList.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <fs_attr.h>

#include "BuildWindow.h"
#include "DarkIcons.h"
#include "IconInstaller.h"


enum {
	kMsgThemeSelected	= 'thsl',
	kMsgModeChosen		= 'mdch',
	kMsgScheduleChanged	= 'schd',
	kMsgIconsChosen		= 'icch',
	kMsgApply			= 'aply',
	kMsgRevert			= 'rvrt',
	kMsgProgress		= 'prog',
	kMsgDone			= 'done',
	kMsgPartChanged		= 'part',
	kMsgRestartTracker	= 'rstr',
	kMsgBuildDecorators	= 'bdec'
};

// the icons menu's first two choices
static const char* kKeepIcons = "";
static const char* kHaikuIcons = "-";
static const char* kDarkIcons = "-dark";


//! What the icons menu calls icons that theme_icons() calls \a icons.
static BString
menu_icons(const BString& icons, const IconThemeList& installed)
{
	if (is_haiku(icons))
		return kHaikuIcons;
	if (is_dark_haiku(icons))
		return kDarkIcons;
	for (int32 i = 0; i < installed.CountItems(); i++) {
		if (installed.ItemAt(i)->ID() == icons)
			return icons;
	}
	return kKeepIcons;
}


//! What theme_icons() calls what the icons menu calls \a id.
static BString
theme_icons_of(const BString& id)
{
	if (id == kKeepIcons)
		return "keep";
	if (id == kHaikuIcons)
		return "haiku";
	if (id == kDarkIcons)
		return kDarkHaikuIcons;
	return id;
}


static BString
schedule_time_text(int32 minutes)
{
	char text[16];
	snprintf(text, sizeof(text), "%02" B_PRId32 ":%02" B_PRId32, minutes / 60,
		minutes % 60);
	return text;
}


//	#pragma mark - PreviewView


/*!	The theme's preview.png, scaled to fit, or its colours if it has
	none.
*/
class PreviewView : public BView {
public:
	PreviewView()
		:
		BView("preview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fBitmap(NULL)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetExplicitMinSize(BSize(320, 200));
	}

	~PreviewView()
	{
		delete fBitmap;
	}

	void SetTheme(const Theme* theme)
	{
		delete fBitmap;
		fBitmap = NULL;
		fColors.MakeEmpty();
		if (theme != NULL) {
			fBitmap = BTranslationUtils::GetBitmapFile(theme->PreviewPath());
			// without a preview, the theme's wallpaper gives an idea
			if (fBitmap == NULL && theme->Wallpaper().StartsWith("/")) {
				fBitmap = BTranslationUtils::GetBitmapFile(
					theme->Wallpaper());
			}
			fColors = theme->Colors();
		}
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds = Bounds();
		if (fBitmap != NULL) {
			BRect source = fBitmap->Bounds();
			const float scale = std::min(
				(bounds.Width() + 1) / (source.Width() + 1),
				(bounds.Height() + 1) / (source.Height() + 1));
			const float width = floorf((source.Width() + 1) * scale);
			const float height = floorf((source.Height() + 1) * scale);
			BRect target(0, 0, width - 1, height - 1);
			target.OffsetTo(floorf((bounds.Width() + 1 - width) / 2),
				floorf((bounds.Height() + 1 - height) / 2));
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmap(fBitmap, source, target, B_FILTER_BITMAP_BILINEAR);
			return;
		}

		// swatches of the theme's main colours
		static const char* kSwatches[] = { "B_PANEL_BACKGROUND_COLOR",
			"B_WINDOW_TAB_COLOR", "B_MENU_SELECTED_BACKGROUND_COLOR",
			"B_DOCUMENT_BACKGROUND_COLOR", "B_PANEL_TEXT_COLOR" };
		const int32 count = sizeof(kSwatches) / sizeof(kSwatches[0]);
		float width = floorf((bounds.Width() + 1) / count);
		int32 shown = 0;
		for (int32 i = 0; i < count; i++) {
			rgb_color color;
			if (fColors.FindColor(kSwatches[i], &color) != B_OK)
				continue;
			SetHighColor(color);
			FillRect(BRect(shown * width, 0, (shown + 1) * width - 1,
				bounds.bottom));
			shown++;
		}
		if (shown == 0) {
			SetHighUIColor(B_PANEL_TEXT_COLOR);
			const char* text = "No preview";
			DrawString(text, BPoint(
				floorf((bounds.Width() - StringWidth(text)) / 2),
				floorf(bounds.Height() / 2)));
		}
	}

private:
	BBitmap*	fBitmap;
	BMessage	fColors;
};


//	#pragma mark - DescriptionView


/*!	A theme's description, wrapped to the view's width, in up to three
	lines.
*/
class DescriptionView : public BView {
public:
	DescriptionView()
		:
		BView("description", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
		SetHighUIColor(B_PANEL_TEXT_COLOR);
		font_height height;
		be_plain_font->GetHeight(&height);
		fLineHeight = ceilf(height.ascent + height.descent + height.leading);
		fAscent = ceilf(height.ascent);
		SetExplicitMinSize(BSize(B_SIZE_UNSET, fLineHeight * 3));
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, fLineHeight * 3));
	}

	void SetText(const char* text)
	{
		fText = text;
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		const float width = Bounds().Width();
		BStringList words;
		fText.Split(" ", true, words);
		BString line;
		float y = fAscent;
		int32 lines = 0;
		for (int32 i = 0; i <= words.CountStrings() && lines < 3; i++) {
			BString candidate(line);
			if (i < words.CountStrings()) {
				if (!candidate.IsEmpty())
					candidate << " ";
				candidate << words.StringAt(i);
				if (StringWidth(candidate) <= width || line.IsEmpty()) {
					line = candidate;
					continue;
				}
			}
			if (lines == 2 && i < words.CountStrings()) {
				// the last line that fits ends the text
				line = candidate;
				TruncateString(&line, B_TRUNCATE_END, width);
			}
			DrawString(line, BPoint(0, y));
			y += fLineHeight;
			lines++;
			line = i < words.CountStrings() ? words.StringAt(i) : "";
		}
	}

private:
	BString	fText;
	float	fLineHeight;
	float	fAscent;
};


//	#pragma mark - IconStripView


/*!	A few icons of the chosen icon theme, as Tracker would show them. */
class IconStripView : public BView {
public:
	IconStripView()
		:
		BView("icon strip", B_WILL_DRAW)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		font_height height;
		be_plain_font->GetHeight(&height);
		fSize = std::max(32.0f, floorf(be_plain_font->Size() / 12 * 32));
		SetExplicitMinSize(BSize(6 * (fSize + 8), fSize + 4));
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, fSize + 4));
	}

	~IconStripView()
	{
		_Clear();
	}

	void SetIconTheme(const IconTheme* theme)
	{
		_Clear();
		if (theme != NULL) {
			// a few common icons, each with the names themes also use
			static const char* kNames[][3] = {
				{ "folder", "inode-directory", NULL },
				{ "text-x-generic", "text-plain", NULL },
				{ "image-x-generic", "image-bmp", NULL },
				{ "audio-x-generic", NULL, NULL },
				{ "computer", "user-desktop", NULL },
				{ "application-x-executable", NULL, NULL }
			};
			for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
				BBitmap* bitmap = NULL;
				for (int n = 0; n < 3 && kNames[i][n] != NULL && bitmap == NULL;
						n++)
					bitmap = theme->Bitmap(kNames[i][n], (int)fSize);
				if (bitmap != NULL)
					fBitmaps.AddItem(bitmap);
			}
		}
		Invalidate();
	}

	/*!	Haiku's own icons for the same kinds of files, or dark versions of
		them, from the system's MIME database, which icon themes leave alone.
	*/
	void SetHaikuIcons(bool dark)
	{
		_Clear();
		// each with a fallback: many types take their icon from their
		// group, or from their preferred application
		static const char* kTypes[][2] = {
			{ "application/x-vnd.Be-directory", NULL },
			{ "text/plain", "text" },
			{ "image/png", "image" },
			{ "audio/mpeg", "audio" },
			{ "application/x-vnd.Be-root", "application/x-vnd.Be-volume" },
			{ "application/x-vnd.Be-elfexecutable", NULL }
		};
		BPath database;
		if (find_directory(B_SYSTEM_DATA_DIRECTORY, &database) != B_OK)
			return;
		database.Append("mime_db");
		for (size_t i = 0; i < sizeof(kTypes) / sizeof(kTypes[0]); i++) {
			BBitmap* bitmap = new BBitmap(BRect(0, 0, fSize - 1, fSize - 1),
				B_RGBA32);
			bool found = false;
			for (int t = 0; t < 2 && kTypes[i][t] != NULL && !found; t++) {
				BString type(kTypes[i][t]);
				BPath path(database.Path(), type.ToLower());
				BNode node(path.Path());
				attr_info info;
				if (node.InitCheck() != B_OK
					|| node.GetAttrInfo("META:ICON", &info) != B_OK
					|| info.size <= 0 || info.size >= 1024 * 1024) {
					continue;
				}
				std::vector<uint8_t> data(info.size);
				if (node.ReadAttr("META:ICON", info.type, 0, &data[0],
						info.size) != info.size) {
					continue;
				}
				std::vector<uint8_t> darkData;
				if (dark && !make_dark_icon(&data[0], data.size(), darkData))
					continue;
				const std::vector<uint8_t>& icon = dark ? darkData : data;
				found = BIconUtils::GetVectorIcon(&icon[0], icon.size(), bitmap)
					== B_OK;
			}
			if (found)
				fBitmaps.AddItem(bitmap);
			else
				delete bitmap;
		}
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		SetDrawingMode(B_OP_ALPHA);
		for (int32 i = 0; i < fBitmaps.CountItems(); i++) {
			BBitmap* bitmap = fBitmaps.ItemAt(i);
			BRect target(0, 0, fSize - 1, fSize - 1);
			target.OffsetTo(i * (fSize + 8), 2);
			DrawBitmap(bitmap, bitmap->Bounds(), target);
		}
	}

private:
	void _Clear()
	{
		for (int32 i = 0; i < fBitmaps.CountItems(); i++)
			delete fBitmaps.ItemAt(i);
		fBitmaps.MakeEmpty();
	}

	BObjectList<BBitmap>	fBitmaps;
	float					fSize;
};


//	#pragma mark - work done outside the window's thread


struct Job {
	BMessenger	window;
	BString		themeName;
	uint32		parts;
	BString		icons;
};


status_t
ThemesWindow::_ApplyThread(void* data)
{
	Job* job = (Job*)data;
	BString log;

	// found again by name, as the command line does: themes come in two
	// formats, and names are made unique when the list is built
	if (!job->themeName.IsEmpty()) {
		ThemeList themes;
		find_themes(themes);
		const Theme* theme = NULL;
		for (int32 i = 0; i < themes.CountItems(); i++) {
			if (themes.ItemAt(i)->Name() == job->themeName)
				theme = themes.ItemAt(i);
		}
		if (theme != NULL)
			apply_appearance(*theme, job->parts, log);
		else {
			log << "The theme \"" << job->themeName
				<< "\" isn't installed any more.\n";
		}
	}

	int32 changed = 0;
	if (job->icons == kHaikuIcons)
		revert_icons(log);
	else if (job->icons == kDarkIcons) {
		BMessenger window = job->window;
		apply_dark_haiku_icons(log, changed,
			[&window](float progress, const char* what) {
				BMessage message(kMsgProgress);
				message.AddFloat("progress", progress);
				message.AddString("what", what);
				window.SendMessage(&message);
			});
	} else if (job->icons != kKeepIcons) {
		IconTheme* icons = load_icon_theme(job->icons);
		if (icons == NULL)
			log << "The icon theme \"" << job->icons << "\" isn't installed.\n";
		else {
			BMessenger window = job->window;
			apply_icon_theme(*icons, log, changed,
				[&window](float progress, const char* what) {
					BMessage message(kMsgProgress);
					message.AddFloat("progress", progress);
					message.AddString("what", what);
					window.SendMessage(&message);
				});
			delete icons;
		}
	}

	BMessage done(kMsgDone);
	done.AddString("log", log);
	done.AddBool("reverted", false);
	done.AddBool("icons", job->icons != kKeepIcons);
	job->window.SendMessage(&done);
	delete job;
	return B_OK;
}


status_t
ThemesWindow::_RevertThread(void* data)
{
	Job* job = (Job*)data;
	BString log;
	revert_appearance(log);
	revert_icons(log);

	BMessage done(kMsgDone);
	done.AddString("log", log);
	done.AddBool("reverted", true);
	done.AddBool("icons", true);
	job->window.SendMessage(&done);
	delete job;
	return B_OK;
}


status_t
ThemesWindow::_RestartTrackerThread(void* data)
{
	restart_tracker();
	return B_OK;
}


//	#pragma mark - ThemesWindow


ThemesWindow::ThemesWindow(BRect frame, const char* selected)
	:
	BWindow(frame, "Power Theme", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fMode(current_mode() == THEME_MODE_DARK
		? THEME_MODE_DARK : THEME_MODE_LIGHT),
	fBusy(false)
{
	fIconChoiceChanged[0] = fIconChoiceChanged[1] = false;

	fThemeList = new BListView("themes");
	fThemeList->SetSelectionMessage(new BMessage(kMsgThemeSelected));
	BScrollView* themeScroll = new BScrollView("theme scroll", fThemeList, 0,
		false, true);
	themeScroll->SetExplicitMinSize(BSize(be_plain_font->StringWidth(
		"A theme with a long name") + 20, B_SIZE_UNSET));

	fPreview = new PreviewView();
	fDescription = new DescriptionView();

	BPopUpMenu* modeMenu = new BPopUpMenu("mode");
	const theme_mode modes[] = { THEME_MODE_LIGHT, THEME_MODE_DARK };
	for (int32 i = 0; i < 2; i++) {
		BMessage* message = new BMessage(kMsgModeChosen);
		message->AddInt32("mode", modes[i]);
		modeMenu->AddItem(new BMenuItem(
			modes[i] == THEME_MODE_LIGHT ? "Light" : "Dark", message));
	}
	fModeField = new BMenuField("mode", "Mode:", modeMenu);

	fSchedule = new BCheckBox("schedule", "Switch mode every day:",
		new BMessage(kMsgScheduleChanged));
	fSchedule->SetToolTip("Switches the theme in use to its light or dark "
		"version at these times, as HH:MM. A theme switched by hand stays until "
		"the next of them.");
	fLightAt = new BTextControl("light at", "Light at:", "",
		new BMessage(kMsgScheduleChanged));
	fDarkAt = new BTextControl("dark at", "Dark at:", "",
		new BMessage(kMsgScheduleChanged));
	BTextControl* timeControls[] = { fLightAt, fDarkAt };
	for (int32 i = 0; i < 2; i++) {
		timeControls[i]->TextView()->SetMaxBytes(5);
		timeControls[i]->TextView()->SetExplicitMinSize(BSize(
			be_plain_font->StringWidth("00:00") + 8, B_SIZE_UNSET));
		timeControls[i]->TextView()->SetExplicitMaxSize(BSize(
			be_plain_font->StringWidth("00:00") + 8, B_SIZE_UNSET));
	}

	BPopUpMenu* iconsMenu = new BPopUpMenu("icons");
	fIconsField = new BMenuField("icons", "Icons:", iconsMenu);
	fIconStrip = new IconStripView();

	fDecorator = new BCheckBox("decorator", "Window borders",
		new BMessage(kMsgPartChanged));
	fControls = new BCheckBox("controls", "Controls",
		new BMessage(kMsgPartChanged));
	fColors = new BCheckBox("colors", "Colors", new BMessage(kMsgPartChanged));
	fFonts = new BCheckBox("fonts", "Fonts", new BMessage(kMsgPartChanged));
	fWallpaper = new BCheckBox("wallpaper", "Wallpaper",
		new BMessage(kMsgPartChanged));
	fDeskbar = new BCheckBox("deskbar", "Deskbar position",
		new BMessage(kMsgPartChanged));

	fProgress = new BStatusBar("progress");
	fProgress->SetMaxValue(1.0f);
	fStatus = new BStringView("status", "");
	fStatus->SetExplicitMinSize(BSize(0, B_SIZE_UNSET));
	fStatus->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fStatus->SetTruncation(B_TRUNCATE_END);

	fRestartTracker = new BButton("restart", "Restart Tracker",
		new BMessage(kMsgRestartTracker));
	fBuildDecorators = new BButton("build", "Build add-ons" B_UTF8_ELLIPSIS,
		new BMessage(kMsgBuildDecorators));
	fBuildDecorators->SetToolTip("Compile the window borders and controls "
		"for this system. They build on app_server's and Haiku's own "
		"private classes, so they must match the exact system build.");
	fRevert = new BButton("revert", "Revert", new BMessage(kMsgRevert));
	fApply = new BButton("apply", "Apply", new BMessage(kMsgApply));

	BBox* previewBox = new BBox("preview box");
	previewBox->SetLabel("Preview");
	BLayoutBuilder::Group<>(previewBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING, B_USE_BIG_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING)
		.Add(fPreview, 3)
		.Add(fDescription, 1)
	.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(themeScroll, 1)
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
				.Add(previewBox)
				.AddGrid(B_USE_SMALL_SPACING, B_USE_DEFAULT_SPACING)
					.AddMenuField(fModeField, 0, 0)
					.AddMenuField(fIconsField, 0, 1)
				.End()
				.Add(fIconStrip)
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
					.Add(fDecorator, 0, 0)
					.Add(fControls, 1, 0)
					.Add(fColors, 0, 1)
					.Add(fFonts, 1, 1)
					.Add(fWallpaper, 0, 2)
					.Add(fDeskbar, 1, 2)
				.End()
				.Add(new BSeparatorView(B_HORIZONTAL))
				.AddGroup(B_HORIZONTAL)
					.Add(fSchedule)
					.Add(fLightAt)
					.Add(fDarkAt)
					.AddGlue()
				.End()
			.End()
		.End()
		.Add(fProgress)
		.AddGroup(B_HORIZONTAL)
			.Add(fStatus, 1)
			.Add(fRestartTracker)
			.Add(fBuildDecorators)
			.Add(fRevert)
			.Add(fApply)
		.End()
	.End();

	fApply->MakeDefault(true);
	fProgress->Hide();
	fRestartTracker->Hide();
	fModeField->Hide();
	modeMenu->SetTargetForItems(this);

	_LoadThemes(selected);
	_UpdateButtons();
	_UpdateSchedule();

	if (!frame.IsValid() || frame.Width() < 10)
		CenterOnScreen();
}


ThemesWindow::~ThemesWindow()
{
}


void
ThemesWindow::Show()
{
	BWindow::Show();

	// a position saved when the window was smaller may not leave room for it
	if (LockLooper()) {
		Layout(false);
		MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
		UnlockLooper();
	}
}


void
ThemesWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	// Enter in a schedule time takes that time, instead of pressing Apply
	const char* bytes;
	if (message->what == B_KEY_DOWN
		&& message->FindString("bytes", &bytes) == B_OK && bytes[0] == B_ENTER
		&& (fLightAt->TextView()->IsFocus()
			|| fDarkAt->TextView()->IsFocus())) {
		_ScheduleChanged();
		return;
	}
	BWindow::DispatchMessage(message, handler);
}


void
ThemesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgThemeSelected:
			_SelectTheme(fThemeList->CurrentSelection());
			break;

		case kMsgModeChosen:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				_SelectMode((theme_mode)mode);
				_ShowTheme(_SelectedTheme());
			}
			break;
		}

		case kMsgIconsChosen:
		{
			const char* id;
			if (message->FindString("id", &id) == B_OK) {
				const int32 mode = fMode == THEME_MODE_DARK ? 1 : 0;
				fIconChoice[mode] = id;
				fIconChoiceChanged[mode] = true;
				_SelectIcons(id);
			}
			break;
		}

		case kMsgScheduleChanged:
			_ScheduleChanged();
			break;

		case kMsgThemeSwitched:
		{
			// switched from outside the window: by the schedule, or by hand
			_UpdateLabels();
			_UpdateButtons();
			BString name, icons;
			get_current(name, icons);
			const Theme* current = find_theme(fThemes, name);
			const Theme* row = fRows.ItemAt(fThemeList->CurrentSelection());
			if (!fBusy && current != NULL && row != NULL
				&& _RowName(current) == _RowName(row)
				&& current->Mode() != THEME_MODE_UNKNOWN) {
				_SelectMode(current->Mode());
				_ShowTheme(_SelectedTheme());
			}
			break;
		}

		case kMsgPartChanged:
			_UpdateButtons();
			break;

		case kMsgApply:
		{
			if (fBusy)
				break;
			Job* job = new Job;
			job->window = BMessenger(this);
			job->parts = 0;
			const Theme* theme = _SelectedTheme();
			if (theme != NULL) {
				job->themeName = theme->Name();
				if (fDecorator->Value() == B_CONTROL_ON)
					job->parts |= THEME_DECORATOR;
				if (fControls->Value() == B_CONTROL_ON)
					job->parts |= THEME_CONTROLS;
				if (fColors->Value() == B_CONTROL_ON)
					job->parts |= THEME_COLORS;
				if (fFonts->Value() == B_CONTROL_ON)
					job->parts |= THEME_FONTS;
				if (fWallpaper->Value() == B_CONTROL_ON)
					job->parts |= THEME_WALLPAPER;
				if (fDeskbar->Value() == B_CONTROL_ON)
					job->parts |= THEME_DESKBAR;
			}
			job->icons = _ChosenIcons();
			_SaveIconChoices(job->icons != kKeepIcons);

			thread_id thread = spawn_thread(_ApplyThread, "apply theme",
				B_NORMAL_PRIORITY, job);
			if (thread < 0) {
				delete job;
				break;
			}
			_SetBusy(true);
			fStatus->SetText("Applying" B_UTF8_ELLIPSIS);
			resume_thread(thread);
			break;
		}

		case kMsgRevert:
		{
			if (fBusy)
				break;
			Job* job = new Job;
			job->window = BMessenger(this);
			thread_id thread = spawn_thread(_RevertThread, "revert theme",
				B_NORMAL_PRIORITY, job);
			if (thread < 0) {
				delete job;
				break;
			}
			_SetBusy(true);
			fStatus->SetText("Reverting" B_UTF8_ELLIPSIS);
			resume_thread(thread);
			break;
		}

		case kMsgRestartTracker:
		{
			thread_id thread = spawn_thread(_RestartTrackerThread,
				"restart Tracker", B_NORMAL_PRIORITY, NULL);
			if (thread >= 0)
				resume_thread(thread);
			if (!fRestartTracker->IsHidden())
				fRestartTracker->Hide();
			break;
		}

		case kMsgBuildDecorators:
		{
			// one build window
			if (fBuildWindow.IsValid()) {
				BLooper* looper = NULL;
				fBuildWindow.Target(&looper);
				BWindow* window = dynamic_cast<BWindow*>(looper);
				if (window != NULL && window->LockLooper()) {
					window->Activate();
					window->UnlockLooper();
				}
				break;
			}
			BuildWindow* window = new BuildWindow(BMessenger(this));
			fBuildWindow = BMessenger(window);
			window->Show();
			break;
		}

		case kMsgDecoratorsBuilt:
		{
			// window borders that weren't installed may be now
			const Theme* theme = _SelectedTheme();
			BString name = theme != NULL ? theme->Name() : BString();
			fRows.MakeEmpty();
			fThemes.MakeEmpty();
			for (int32 i = fThemeList->CountItems() - 1; i >= 0; i--)
				delete fThemeList->RemoveItem(i);
			fIconThemes.MakeEmpty();
			fIconsField->Menu()->RemoveItems(0,
				fIconsField->Menu()->CountItems(), true);
			_LoadThemes(name.IsEmpty() ? NULL : name.String());
			break;
		}

		case kMsgProgress:
		{
			float progress;
			if (message->FindFloat("progress", &progress) == B_OK) {
				fProgress->Update(progress - fProgress->CurrentValue(),
					"Icons:", message->GetString("what", ""));
			}
			break;
		}

		case kMsgDone:
		{
			_SetBusy(false);
			_UpdateLabels();
			BString log = message->GetString("log", "");
			log.Trim();
			if (!log.IsEmpty()) {
				// the first problem, in full in the tool tip
				BString first(log);
				int32 newline = first.FindFirst('\n');
				if (newline >= 0)
					first.Truncate(newline);
				fStatus->SetText(first);
				fStatus->SetToolTip(log);
			} else {
				BString status(message->GetBool("reverted", false)
					? "Your previous look is back. " : "Done. ");
				status << running_applications_note();
				fStatus->SetText(status);
				fStatus->SetToolTip(status);
			}
			// Tracker keeps a few icons, folders' among them, until it
			// restarts
			if (message->GetBool("icons", false) && fRestartTracker->IsHidden())
				fRestartTracker->Show();
			_UpdateButtons();
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
ThemesWindow::WindowActivated(bool active)
{
	// the command line may have changed what is in use
	if (active) {
		_UpdateLabels();
		_UpdateButtons();
		_UpdateSchedule();
	}
	BWindow::WindowActivated(active);
}


bool
ThemesWindow::QuitRequested()
{
	if (fBusy)
		return false;

	BMessage settings;
	settings.AddRect("frame", Frame());
	const Theme* theme = _SelectedTheme();
	if (theme != NULL)
		settings.AddString("theme", theme->Name());
	BPath path;
	if (themes_settings_path(path, "settings") == B_OK) {
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			settings.Flatten(&file);
	}
	return true;
}


void
ThemesWindow::_LoadThemes(const char* selected)
{
	find_themes(fThemes);
	find_icon_themes(fIconThemes);

	// A pair of light and dark themes gets one row.
	for (int32 i = 0; i < fThemes.CountItems(); i++) {
		const Theme* theme = fThemes.ItemAt(i);
		bool listed = false;
		for (int32 row = 0; row < fRows.CountItems() && !listed; row++)
			listed = _RowName(fRows.ItemAt(row)) == _RowName(theme);
		if (!listed)
			fRows.AddItem(theme);
	}
	for (int32 i = 1; i < fRows.CountItems(); i++) {
		const Theme* theme = fRows.ItemAt(i);
		int32 to = i;
		while (to > 0 && _RowName(fRows.ItemAt(to - 1)).ICompare(
				_RowName(theme)) > 0) {
			to--;
		}
		if (to != i)
			fRows.MoveItem(i, to);
	}

	int32 select = 0;
	for (int32 i = 0; i < fRows.CountItems(); i++) {
		const Theme* theme = fRows.ItemAt(i);
		fThemeList->AddItem(new BStringItem(_RowName(theme)));

		const Theme* light = find_variant(fThemes, *theme, THEME_MODE_LIGHT);
		const Theme* dark = find_variant(fThemes, *theme, THEME_MODE_DARK);
		if (selected == NULL)
			continue;
		if (theme->Name() == selected)
			select = i;
		else if (light != NULL && light->Name() == selected) {
			select = i;
			fMode = THEME_MODE_LIGHT;
		} else if (dark != NULL && dark->Name() == selected) {
			select = i;
			fMode = THEME_MODE_DARK;
		}
	}

	BMenu* menu = fIconsField->Menu();
	BMessage* keep = new BMessage(kMsgIconsChosen);
	keep->AddString("id", kKeepIcons);
	menu->AddItem(new BMenuItem("Keep the current icons", keep));
	BMessage* haiku = new BMessage(kMsgIconsChosen);
	haiku->AddString("id", kHaikuIcons);
	menu->AddItem(new BMenuItem("Haiku icons", haiku));
	BMessage* darkHaiku = new BMessage(kMsgIconsChosen);
	darkHaiku->AddString("id", kDarkIcons);
	menu->AddItem(new BMenuItem("Haiku icons (dark)", darkHaiku));
	if (!fIconThemes.IsEmpty())
		menu->AddSeparatorItem();
	for (int32 i = 0; i < fIconThemes.CountItems(); i++) {
		BMessage* message = new BMessage(kMsgIconsChosen);
		message->AddString("id", fIconThemes.ItemAt(i)->ID());
		menu->AddItem(new BMenuItem(fIconThemes.ItemAt(i)->Name(), message));
	}
	menu->SetTargetForItems(this);
	_UpdateLabels();

	if (!fThemes.IsEmpty())
		fThemeList->Select(select);
	else {
		fDescription->SetText("No themes are installed. Themes go in a "
			"\"themes\" folder in ~/config/non-packaged/data.");
		_SelectIcons(kKeepIcons);
	}
}


/*!	Marks what is in use, which may have changed from the command line. */
void
ThemesWindow::_UpdateLabels()
{
	BString currentTheme, currentIcons;
	get_current(currentTheme, currentIcons);

	const Theme* current = find_theme(fThemes, currentTheme);
	for (int32 i = 0; i < fRows.CountItems(); i++) {
		BStringItem* item = dynamic_cast<BStringItem*>(fThemeList->ItemAt(i));
		if (item == NULL)
			continue;
		BString label(_RowName(fRows.ItemAt(i)));
		if (current != NULL && _RowName(current) == _RowName(fRows.ItemAt(i)))
			label << " (in use)";
		item->SetText(label);
	}
	fThemeList->Invalidate();

	BMenu* menu = fIconsField->Menu();
	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		const char* id;
		if (item->Message() == NULL
			|| item->Message()->FindString("id", &id) != B_OK)
			continue;
		BString label;
		if (strcmp(id, kKeepIcons) == 0)
			label = "Keep the current icons";
		else if (strcmp(id, kHaikuIcons) == 0)
			label = "Haiku icons";
		else if (strcmp(id, kDarkIcons) == 0)
			label = "Haiku icons (dark)";
		else {
			for (int32 t = 0; t < fIconThemes.CountItems(); t++) {
				if (fIconThemes.ItemAt(t)->ID() == id)
					label = fIconThemes.ItemAt(t)->Name();
			}
		}
		if ((strcmp(id, kHaikuIcons) == 0 && currentIcons.IsEmpty())
			|| (strcmp(id, kDarkIcons) == 0 && is_dark_haiku(currentIcons))
			|| (strcmp(id, kKeepIcons) != 0 && currentIcons == id))
			label << " (in use)";
		item->SetLabel(label);
	}

	// the field shows its marked item's label as it was when marked
	BMenuItem* marked = menu->FindMarked();
	if (marked != NULL) {
		marked->SetMarked(false);
		marked->SetMarked(true);
	}
}


void
ThemesWindow::_SelectTheme(int32 index)
{
	const Theme* theme = fRows.ItemAt(index);

	// A pair in use opens on the version in use.
	if (theme != NULL) {
		BString currentTheme, currentIcons;
		get_current(currentTheme, currentIcons);
		const Theme* current = find_theme(fThemes, currentTheme);
		if (current != NULL && _RowName(current) == _RowName(theme)
			&& current->Mode() != THEME_MODE_UNKNOWN) {
			fMode = current->Mode();
		}
	}

	const bool paired = theme != NULL
		&& find_variant(fThemes, *theme, THEME_MODE_LIGHT) != NULL
		&& find_variant(fThemes, *theme, THEME_MODE_DARK) != NULL;
	if (paired && fModeField->IsHidden())
		fModeField->Show();
	else if (!paired && !fModeField->IsHidden())
		fModeField->Hide();

	_SelectMode(fMode);
	_LoadIconChoices();
	_ShowTheme(_SelectedTheme());
}


void
ThemesWindow::_SelectMode(theme_mode mode)
{
	fMode = mode;
	BMenuItem* item = fModeField->Menu()->ItemAt(
		mode == THEME_MODE_DARK ? 1 : 0);
	if (item != NULL)
		item->SetMarked(true);
}


/*!	The name a theme is listed under: for one of a pair of light and dark
	themes, what they are versions of.
*/
BString
ThemesWindow::_RowName(const Theme* theme) const
{
	if (!theme->VariantOf().IsEmpty()
		&& find_variant(fThemes, *theme, THEME_MODE_LIGHT) != NULL
		&& find_variant(fThemes, *theme, THEME_MODE_DARK) != NULL) {
		return theme->VariantOf();
	}
	return theme->Name();
}


const Theme*
ThemesWindow::_SelectedTheme() const
{
	const Theme* row = fRows.ItemAt(fThemeList->CurrentSelection());
	if (row == NULL)
		return NULL;

	const Theme* theme = find_variant(fThemes, *row, fMode);
	return theme != NULL ? theme : row;
}


void
ThemesWindow::_ShowTheme(const Theme* theme)
{
	fPreview->SetTheme(theme);
	if (theme == NULL) {
		fDescription->SetText("");
		_UpdateButtons();
		return;
	}

	fDescription->SetText(theme->Description());

	// parts whose add-ons or files aren't installed can't be applied
	// (a version of a pair with icons chosen for it doesn't need its own)
	const bool iconsChosen = _SelectedIsPair()
		&& fIconChoice[fMode == THEME_MODE_DARK ? 1 : 0] != kKeepIcons;
	BStringList missingParts;
	const uint32 missing = theme->Missing(&missingParts,
		iconsChosen ? THEME_ALL & ~THEME_ICONS : THEME_ALL);
	const uint32 parts = theme->Parts() & ~missing;
	BCheckBox* boxes[] = { fDecorator, fControls, fColors, fFonts,
		fWallpaper, fDeskbar };
	const uint32 flags[] = { THEME_DECORATOR, THEME_CONTROLS, THEME_COLORS,
		THEME_FONTS, THEME_WALLPAPER, THEME_DESKBAR };
	for (int32 i = 0; i < 6; i++) {
		boxes[i]->SetEnabled((parts & flags[i]) != 0);
		boxes[i]->SetValue((parts & flags[i]) != 0);
	}
	if (!fBusy) {
		if (missingParts.IsEmpty()) {
			fStatus->SetText("");
			fStatus->SetToolTip((const char*)NULL);
		} else {
			BString text("Not installed: ");
			text << missingParts.Join(", ") << ".";
			if ((missing & (THEME_DECORATOR | THEME_CONTROLS)) != 0)
				text << " Build add-ons" B_UTF8_ELLIPSIS " can make "
					"Reactionary, Smoke, Gonx and Flat ones.";
			fStatus->SetText(text);
			fStatus->SetToolTip(text);
		}
	}

	// what was chosen for this version of a pair, or else the theme's own
	// icons, if they are installed
	if (_SelectedIsPair()) {
		const bool dark = fMode == THEME_MODE_DARK;
		fIconsField->SetLabel(dark ? "Dark icons:" : "Light icons:");
		_SelectIcons(fIconChoice[dark ? 1 : 0]);
	} else {
		fIconsField->SetLabel("Icons:");
		_SelectIcons(menu_icons(theme->Icons(), fIconThemes));
	}
	// the label is drawn as it changes, before there is room for a longer one
	Layout(false);
	fIconsField->Invalidate();
}


bool
ThemesWindow::_SelectedIsPair() const
{
	const Theme* row = fRows.ItemAt(fThemeList->CurrentSelection());
	return row != NULL && find_variant(fThemes, *row, THEME_MODE_LIGHT) != NULL
		&& find_variant(fThemes, *row, THEME_MODE_DARK) != NULL;
}


void
ThemesWindow::_LoadIconChoices()
{
	fIconChoiceChanged[0] = fIconChoiceChanged[1] = false;
	if (!_SelectedIsPair())
		return;

	const Theme* row = fRows.ItemAt(fThemeList->CurrentSelection());
	const theme_mode modes[] = { THEME_MODE_LIGHT, THEME_MODE_DARK };
	for (int32 i = 0; i < 2; i++) {
		fIconChoice[i] = menu_icons(
			theme_icons(*find_variant(fThemes, *row, modes[i])), fIconThemes);
	}
}


/*!	Keeps the icons chosen for each version of a pair, for when the mode next
	changes. \a applyingIcons says whether icons are being applied now.
*/
void
ThemesWindow::_SaveIconChoices(bool applyingIcons)
{
	bool follow = applyingIcons;
	if (_SelectedIsPair()) {
		const Theme* row = fRows.ItemAt(fThemeList->CurrentSelection());
		const theme_mode modes[] = { THEME_MODE_LIGHT, THEME_MODE_DARK };
		for (int32 i = 0; i < 2; i++) {
			if (fIconChoiceChanged[i]) {
				set_theme_icons(*find_variant(fThemes, *row, modes[i]),
					theme_icons_of(fIconChoice[i]));
				fIconChoiceChanged[i] = false;
			}
			follow |= fIconChoice[i] != kKeepIcons;
		}
	}
	set_icons_follow_mode(follow);
}


void
ThemesWindow::_SelectIcons(const char* id)
{
	BMenu* menu = fIconsField->Menu();
	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		const char* itemID;
		if (item->Message() != NULL
			&& item->Message()->FindString("id", &itemID) == B_OK
			&& strcmp(itemID, id) == 0) {
			item->SetMarked(true);
			break;
		}
	}

	const IconTheme* theme = NULL;
	for (int32 i = 0; i < fIconThemes.CountItems(); i++) {
		if (fIconThemes.ItemAt(i)->ID() == id)
			theme = fIconThemes.ItemAt(i);
	}
	if (strcmp(id, kHaikuIcons) == 0 || strcmp(id, kDarkIcons) == 0)
		fIconStrip->SetHaikuIcons(strcmp(id, kDarkIcons) == 0);
	else
		fIconStrip->SetIconTheme(theme);
	_UpdateButtons();
}


BString
ThemesWindow::_ChosenIcons() const
{
	BMenuItem* item = fIconsField->Menu()->FindMarked();
	const char* id;
	if (item == NULL || item->Message() == NULL
		|| item->Message()->FindString("id", &id) != B_OK)
		return kKeepIcons;
	return id;
}


void
ThemesWindow::_SetBusy(bool busy)
{
	fBusy = busy;
	if (busy) {
		fProgress->Reset();
		fProgress->Show();
	} else if (!fProgress->IsHidden())
		fProgress->Hide();
	fModeField->SetEnabled(!busy);
	fSchedule->SetEnabled(!busy);
	fLightAt->SetEnabled(!busy);
	fDarkAt->SetEnabled(!busy);
	fIconsField->SetEnabled(!busy);
	_UpdateButtons();
}


/*!	Shows the schedule as saved, which the command line may have changed. Text
	being typed stays.
*/
void
ThemesWindow::_UpdateSchedule()
{
	ThemeSchedule schedule;
	load_schedule(schedule);
	fSchedule->SetValue(schedule.enabled ? B_CONTROL_ON : B_CONTROL_OFF);
	if (!fLightAt->TextView()->IsFocus())
		fLightAt->SetText(schedule_time_text(schedule.lightAt));
	if (!fDarkAt->TextView()->IsFocus())
		fDarkAt->SetText(schedule_time_text(schedule.darkAt));
}


void
ThemesWindow::_ScheduleChanged()
{
	ThemeSchedule saved;
	load_schedule(saved);

	ThemeSchedule schedule = saved;
	schedule.enabled = fSchedule->Value() == B_CONTROL_ON;
	BString problem;
	if (!parse_schedule_time(fLightAt->Text(), schedule.lightAt)
		|| !parse_schedule_time(fDarkAt->Text(), schedule.darkAt)) {
		problem = "Write schedule times like 07:00.";
	} else if (schedule.lightAt == schedule.darkAt)
		problem = "Light and dark need different times.";
	if (!problem.IsEmpty()) {
		fSchedule->SetValue(saved.enabled ? B_CONTROL_ON : B_CONTROL_OFF);
		fStatus->SetText(problem);
		fStatus->SetToolTip(problem);
		return;
	}

	// tidied up, whatever else happens: 7:00 is 07:00
	fLightAt->SetText(schedule_time_text(schedule.lightAt));
	fDarkAt->SetText(schedule_time_text(schedule.darkAt));

	if (schedule.enabled == saved.enabled
		&& schedule.lightAt == saved.lightAt
		&& schedule.darkAt == saved.darkAt) {
		return;
	}

	status_t status = set_schedule(schedule);
	if (status != B_OK) {
		BString text("Can't set the schedule: ");
		text << strerror(status) << ".";
		fSchedule->SetValue(saved.enabled ? B_CONTROL_ON : B_CONTROL_OFF);
		fStatus->SetText(text);
		fStatus->SetToolTip(text);
		return;
	}

	BString text(schedule.enabled
		? schedule_description(schedule) : BString("The schedule is off."));
	if (schedule.enabled) {
		BString name, icons;
		get_current(name, icons);
		const Theme* current = find_theme(fThemes, name);
		const bool paired = current != NULL
			&& find_variant(fThemes, *current, THEME_MODE_LIGHT) != NULL
			&& find_variant(fThemes, *current, THEME_MODE_DARK) != NULL;
		if (!paired) {
			text << " It does nothing until a theme with light and dark "
				"versions is in use.";
		}
	}
	fStatus->SetText(text);
	fStatus->SetToolTip(text);
}


void
ThemesWindow::_UpdateButtons()
{
	const bool anyPart = (fDecorator->IsEnabled()
			&& fDecorator->Value() == B_CONTROL_ON)
		|| (fControls->IsEnabled() && fControls->Value() == B_CONTROL_ON)
		|| (fColors->IsEnabled() && fColors->Value() == B_CONTROL_ON)
		|| (fFonts->IsEnabled() && fFonts->Value() == B_CONTROL_ON)
		|| (fWallpaper->IsEnabled() && fWallpaper->Value() == B_CONTROL_ON)
		|| (fDeskbar->IsEnabled() && fDeskbar->Value() == B_CONTROL_ON)
		|| _ChosenIcons() != kKeepIcons;
	fApply->SetEnabled(!fBusy && anyPart);
	fRevert->SetEnabled(!fBusy
		&& (has_saved_appearance() || has_saved_icons()));
}
