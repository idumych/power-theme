/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The Themes window. See ThemesWindow.h.
 */


#include "ThemesWindow.h"

#include <stdio.h>

#include <algorithm>

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
#include <TextView.h>
#include <TranslationUtils.h>

#include "BuildWindow.h"
#include "IconInstaller.h"


enum {
	kMsgThemeSelected	= 'thsl',
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

	/*!	Haiku's own icons for the same kinds of files, from the system's
		MIME database, which icon themes leave alone.
	*/
	void SetHaikuIcons()
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
				found = node.InitCheck() == B_OK
					&& BIconUtils::GetVectorIcon(&node, "META:ICON", bitmap)
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
	else if (job->icons != kKeepIcons) {
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
	BWindow(frame, "Themes", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fBusy(false)
{
	fThemeList = new BListView("themes");
	fThemeList->SetSelectionMessage(new BMessage(kMsgThemeSelected));
	BScrollView* themeScroll = new BScrollView("theme scroll", fThemeList, 0,
		false, true);
	themeScroll->SetExplicitMinSize(BSize(be_plain_font->StringWidth(
		"A theme with a long name") + 20, B_SIZE_UNSET));

	fPreview = new PreviewView();
	fDescription = new DescriptionView();

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
	fBuildDecorators = new BButton("build", "Build window borders"
		B_UTF8_ELLIPSIS, new BMessage(kMsgBuildDecorators));
	fBuildDecorators->SetToolTip("Compile the window borders for this "
		"system. They run inside app_server, so they must match the exact "
		"system build.");
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
				.Add(fIconsField)
				.Add(fIconStrip)
				.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
					.Add(fDecorator, 0, 0)
					.Add(fControls, 1, 0)
					.Add(fColors, 0, 1)
					.Add(fFonts, 1, 1)
					.Add(fWallpaper, 0, 2)
					.Add(fDeskbar, 1, 2)
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

	_LoadThemes(selected);
	_UpdateButtons();

	if (!frame.IsValid() || frame.Width() < 10)
		CenterOnScreen();
}


ThemesWindow::~ThemesWindow()
{
}


void
ThemesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgThemeSelected:
			_SelectTheme(fThemeList->CurrentSelection());
			break;

		case kMsgIconsChosen:
		{
			const char* id;
			if (message->FindString("id", &id) == B_OK)
				_SelectIcons(id);
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
			Theme* theme = fThemes.ItemAt(fThemeList->CurrentSelection());
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
			Theme* theme = fThemes.ItemAt(fThemeList->CurrentSelection());
			BString name = theme != NULL ? theme->Name() : BString();
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
				fStatus->SetText(message->GetBool("reverted", false)
					? "Your previous look is back. Restart applications to "
						"see it everywhere."
					: "Done. Restart applications to see the new controls "
						"and fonts in them.");
				fStatus->SetToolTip((const char*)NULL);
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
	Theme* theme = fThemes.ItemAt(fThemeList->CurrentSelection());
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

	int32 select = 0;
	for (int32 i = 0; i < fThemes.CountItems(); i++) {
		const Theme* theme = fThemes.ItemAt(i);
		fThemeList->AddItem(new BStringItem(theme->Name()));
		if (selected != NULL && theme->Name() == selected)
			select = i;
	}

	BMenu* menu = fIconsField->Menu();
	BMessage* keep = new BMessage(kMsgIconsChosen);
	keep->AddString("id", kKeepIcons);
	menu->AddItem(new BMenuItem("Keep the current icons", keep));
	BMessage* haiku = new BMessage(kMsgIconsChosen);
	haiku->AddString("id", kHaikuIcons);
	menu->AddItem(new BMenuItem("Haiku icons", haiku));
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

	for (int32 i = 0; i < fThemes.CountItems(); i++) {
		BStringItem* item = dynamic_cast<BStringItem*>(fThemeList->ItemAt(i));
		if (item == NULL)
			continue;
		BString label(fThemes.ItemAt(i)->Name());
		if (fThemes.ItemAt(i)->Name() == currentTheme)
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
		else {
			for (int32 t = 0; t < fIconThemes.CountItems(); t++) {
				if (fIconThemes.ItemAt(t)->ID() == id)
					label = fIconThemes.ItemAt(t)->Name();
			}
		}
		if ((strcmp(id, kHaikuIcons) == 0 && currentIcons.IsEmpty())
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
	const Theme* theme = fThemes.ItemAt(index);
	fPreview->SetTheme(theme);
	if (theme == NULL) {
		fDescription->SetText("");
		_UpdateButtons();
		return;
	}

	fDescription->SetText(theme->Description());

	// parts whose add-ons or files aren't installed can't be applied
	BStringList missingParts;
	const uint32 missing = theme->Missing(&missingParts);
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
			if ((missing & THEME_DECORATOR) != 0)
				text << " Build window borders" B_UTF8_ELLIPSIS " can make "
					"Reactionary, Smoke, Gonx and Flat ones.";
			fStatus->SetText(text);
			fStatus->SetToolTip(text);
		}
	}

	// the theme's own icons, if they are installed
	BString icons = kKeepIcons;
	if (is_haiku(theme->Icons()))
		icons = kHaikuIcons;
	for (int32 i = 0; i < fIconThemes.CountItems(); i++) {
		if (fIconThemes.ItemAt(i)->ID() == theme->Icons())
			icons = theme->Icons();
	}
	_SelectIcons(icons);
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
	if (strcmp(id, kHaikuIcons) == 0)
		fIconStrip->SetHaikuIcons();
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
	fIconsField->SetEnabled(!busy);
	_UpdateButtons();
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
