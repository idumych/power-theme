/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * PowerShelf: a desktop shell in place of the Deskbar, laid out as QNX 6's
 * Photon or, in Leopard.cpp, as Mac OS X 10.5 Leopard. Power Theme starts it
 * for themes that ask for it ("shell = photon" or "shell = leopard"), and it
 * stops Deskbar while it runs and brings it back as it quits.
 *
 * Photon's layout has two parts, drawn as Photon drew them: a taskbar along the bottom of
 * the screen, with a Launch button opening the Deskbar's menu of applications,
 * a button for each window, and a clock; and a shelf down the right, with
 * groups of launchers that open and close (applications, utilities and
 * preferences) and a system monitor of the processors, memory and disk. Both
 * sit in a sunken grey trough inside a ridged band, lit from the top left.
 *
 * Drawn from scratch after QNX 6.2.1's Photon, measured in a virtual machine;
 * nothing of QNX's is used. It shows its own clock and status only: replicants
 * that other applications add to the Deskbar's tray don't appear.
 */


#include <Alert.h>
#include <Archivable.h>
#include <Application.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <InterfaceDefs.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Node.h>
#include <NodeInfo.h>
#include <OS.h>
#include <image.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <View.h>
#include <Volume.h>
#include <Window.h>
#include <fs_info.h>

#include "Shell.h"

#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <typeinfo>
#include <vector>


static const char* kSignature = "application/x-vnd.idumych-PowerTheme-shelf";
const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
const char* kTrackerSignature = "application/x-vnd.Be-TRAK";
static const char* kLayoutName = "PowerTheme/shell layout";
static const char* kSettingsName = "PowerTheme/shelf";
// what keeps a crashing replicant from bringing the shell down for good: the
// one being loaded, the ones that crashed while loading, and recent starts
static const char* kLoadingName = "PowerTheme/shelf-loading";
static const char* kGuardName = "PowerTheme/shelf-guard";

static const uint32 kTick = 'tick';
static const uint32 kLayout = 'layt';

// the band round each part: dark lines either side of a ridge
static const rgb_color kOutline = { 0x4b, 0x4b, 0x4b, 255 };
static const rgb_color kWhite = { 0xff, 0xff, 0xff, 255 };
static const rgb_color kRidge = { 0xd8, 0xd8, 0xd8, 255 };
static const rgb_color kRidgeShade = { 0xa6, 0xa6, 0xa6, 255 };
// the trough the buttons sit in, shaded along its top and left
static const rgb_color kTrough = { 0xc0, 0xc0, 0xc0, 255 };
static const rgb_color kTroughShade = { 0x8e, 0x8e, 0x8e, 255 };
// buttons: a face lit along the top and left, shaded along the bottom
static const rgb_color kFace = { 0xd9, 0xd9, 0xd9, 255 };
static const rgb_color kFaceShade = { 0xa7, 0xa7, 0xa7, 255 };
static const rgb_color kHeaderFace = { 0xdb, 0xdb, 0xdb, 255 };
static const rgb_color kToggleFace = { 0xc7, 0xc7, 0xc7, 255 };
static const rgb_color kToggleMark = { 0x60, 0x60, 0x60, 255 };
static const rgb_color kIconCell = { 0xcc, 0xcc, 0xcc, 255 };
// the active window's button: sunk, black edged, cream
static const rgb_color kActiveFace = { 0xf9, 0xf4, 0xe4, 255 };
static const rgb_color kActiveShade = { 0xc7, 0xc2, 0xb2, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };
// the monitor's gauges
static const rgb_color kGauge = { 0xbc, 0xbc, 0xbc, 255 };
static const rgb_color kGaugeShade = { 0x8d, 0x8d, 0x8d, 255 };
static const rgb_color kGaugeEdge = { 0x3f, 0x3f, 0x3f, 255 };
static const rgb_color kLevel = { 0xb5, 0xc4, 0xb0, 255 };
static const rgb_color kLevelLight = { 0xdd, 0xec, 0xd8, 255 };
static const rgb_color kLevelShade = { 0x8d, 0x9c, 0x88, 255 };

// the ridged band, and each part's sizes
static const float kBand = 6;
static const float kTaskbarHeight = 25;
static const float kShelfWidth = 135;
static const float kLaunchWidth = 80;
static const float kTaskWidth = 128;
static const float kClockWidth = 104;
static const float kHeaderHeight = 19;
static const float kItemHeight = 25;
static const float kToggleWidth = 16;
static const float kIconCellWidth = 29;


static float
font_lines(const BFont& font)
{
	font_height height;
	font.GetHeight(&height);
	return ceilf(height.ascent + height.descent + height.leading);
}


// Photon's sizes, made bigger where a theme's fonts need more room
static float
item_height()
{
	return std::max(kItemHeight, font_lines(*be_plain_font) + 8);
}


static float
header_height()
{
	return std::max(kHeaderHeight, font_lines(*be_bold_font) + 2);
}


static float
taskbar_height()
{
	return std::max(kTaskbarHeight, font_lines(*be_plain_font) + 8);
}


static float
launch_width()
{
	return std::max(kLaunchWidth,
		ceilf(be_plain_font->StringWidth("Launch")) + 36);
}


static float
shelf_width()
{
	// its longest titles and labels, in the theme's fonts
	const float title = ceilf(be_bold_font->StringWidth("System Monitor"))
		+ kBand + kToggleWidth + 14;
	const float label = ceilf(be_plain_font->StringWidth("Activity Monitor"))
		+ kBand + kIconCellWidth + 12;
	return std::max(kShelfWidth, std::max(title, label));
}


static float
clock_width()
{
	return std::max(kClockWidth,
		ceilf(be_plain_font->StringWidth("Wed-28 12:00PM")) + 8);
}


static void
fill(BView* view, float l, float t, float r, float b, rgb_color color)
{
	if (l > r || t > b)
		return;
	view->SetHighColor(color);
	view->FillRect(BRect(l, t, r, b));
}


// Whether the QNX theme's controls are in use: then the shell is drawn as
// Photon's, colour for colour; with any other, in that theme's own controls
// and colours.
static bool sPhoton = true;


static bool
photon_look()
{
	if (be_control_look == NULL)
		return false;
	return strstr(typeid(*be_control_look).name(), "QNXControlLook") != NULL;
}


static rgb_color
panel()
{
	return ui_color(B_PANEL_BACKGROUND_COLOR);
}


//!	A button in the theme's own look, \a pressed or not.
static void
themed_button(BView* view, BRect r, bool pressed)
{
	const rgb_color base = panel();
	const uint32 flags = pressed ? BControlLook::B_ACTIVATED : 0;
	view->PushState();
	view->SetHighColor(base);
	view->FillRect(r);
	BRect rect(r);
	be_control_look->DrawButtonFrame(view, rect, r, base, base, flags);
	be_control_look->DrawButtonBackground(view, rect, r, base, flags);
	view->PopState();
}


//!	A raised button: lit along its top and left, shaded along its bottom and right.
static void
draw_raised(BView* view, BRect r, rgb_color face, bool pressed = false)
{
	if (!sPhoton) {
		themed_button(view, r, pressed);
		return;
	}
	fill(view, r.left, r.top, r.right, r.bottom, face);
	fill(view, r.left, r.top, r.right, r.top, pressed ? kFaceShade : kWhite);
	fill(view, r.left, r.top, r.left, r.bottom, pressed ? kFaceShade : kWhite);
	fill(view, r.left + 1, r.bottom, r.right, r.bottom,
		pressed ? kWhite : kFaceShade);
	fill(view, r.right, r.top + 1, r.right, r.bottom,
		pressed ? kWhite : kFaceShade);
}


//!	A group's title bar: Photon's raised grey, or the theme's menu bar.
static void
draw_header(BView* view, BRect r, rgb_color face, bool pressed)
{
	if (sPhoton) {
		draw_raised(view, r, face, pressed);
		return;
	}
	view->PushState();
	BRect rect(r);
	be_control_look->DrawMenuBarBackground(view, rect, r,
		ui_color(B_MENU_BACKGROUND_COLOR),
		pressed ? BControlLook::B_ACTIVATED : 0);
	view->PopState();
}


//!	Plain panel, raised a little in Photon's look.
static void
draw_plain(BView* view, BRect r)
{
	if (sPhoton) {
		draw_raised(view, r, kFace);
		return;
	}
	fill(view, r.left, r.top, r.right, r.bottom, panel());
}


//!	The colour of a button's text, and a title bar's.
static rgb_color
button_text()
{
	return sPhoton ? kBlack : ui_color(B_CONTROL_TEXT_COLOR);
}


static rgb_color
header_text()
{
	return sPhoton ? kBlack : ui_color(B_MENU_ITEM_TEXT_COLOR);
}


//!	The trough: grey, shaded a pixel along its top and left.
static void
draw_trough(BView* view, BRect r)
{
	if (!sPhoton) {
		fill(view, r.left, r.top, r.right, r.bottom,
			tint_color(panel(), B_DARKEN_1_TINT));
		fill(view, r.left, r.top, r.right, r.top,
			tint_color(panel(), B_DARKEN_2_TINT));
		fill(view, r.left, r.top, r.left, r.bottom,
			tint_color(panel(), B_DARKEN_2_TINT));
		return;
	}
	fill(view, r.left, r.top, r.right, r.bottom, kTrough);
	fill(view, r.left, r.top, r.right, r.top, kTroughShade);
	fill(view, r.left, r.top, r.left, r.bottom, kTroughShade);
}


//!	The band's six lines, from its outer edge in.
static void
band_lines(rgb_color lines[6])
{
	const rgb_color photon[6] = { kOutline, kWhite, kRidge, kRidge,
		kRidgeShade, kOutline };
	const rgb_color base = panel();
	const rgb_color themed[6] = { tint_color(base, B_DARKEN_3_TINT),
		tint_color(base, B_LIGHTEN_2_TINT), base, base,
		tint_color(base, B_DARKEN_1_TINT), tint_color(base, B_DARKEN_3_TINT) };
	for (int32 i = 0; i < 6; i++)
		lines[i] = sPhoton ? photon[i] : themed[i];
}


//!	The band round each part, along \a horizontal or down.
static void
draw_band(BView* view, BRect r, bool horizontal)
{
	rgb_color lines[6];
	band_lines(lines);
	for (int32 i = 0; i < 6; i++) {
		if (horizontal)
			fill(view, r.left, r.top + i, r.right, r.top + i, lines[i]);
		else
			fill(view, r.left + i, r.top, r.left + i, r.bottom, lines[i]);
	}
}


BBitmap*
icon_for(const entry_ref& ref, float size)
{
	BBitmap* icon = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGBA32);
	if (BNodeInfo::GetTrackerIcon(&ref, icon, (icon_size)(int32)size) != B_OK) {
		delete icon;
		return NULL;
	}
	return icon;
}


void
draw_icon(BView* view, const BBitmap* icon, BPoint where)
{
	if (icon == NULL)
		return;
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->DrawBitmap(icon, where);
	view->SetDrawingMode(B_OP_COPY);
}


//!	\a text in \a font, shortened to \a width, its baseline centred in \a r.
static void
draw_text(BView* view, const char* text, const BFont& font, BRect r,
	float x, rgb_color color)
{
	BString shown(text);
	font.TruncateString(&shown, B_TRUNCATE_END, r.right - x - 3);
	font_height height;
	font.GetHeight(&height);
	view->SetFont(&font);
	view->SetHighColor(color);
	view->SetLowColor(B_TRANSPARENT_COLOR);
	view->SetDrawingMode(B_OP_OVER);
	const float baseline = floorf((r.top + r.bottom + 1 + height.ascent
		- height.descent) / 2);
	view->DrawString(shown.String(), BPoint(x, baseline));
	view->SetDrawingMode(B_OP_COPY);
}


//	#pragma mark - the Launch menu


//!	The folders the Deskbar's menu is made of, the user's own last.
std::vector<BPath>
menu_folders()
{
	std::vector<BPath> folders;
	const directory_which places[] = { B_SYSTEM_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY, B_USER_DATA_DIRECTORY,
		B_USER_NONPACKAGED_DATA_DIRECTORY };
	for (size_t i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
		BPath path;
		if (find_directory(places[i], &path) == B_OK
			&& path.Append("deskbar/menu") == B_OK)
			folders.push_back(path);
	}
	BPath user;
	if (find_directory(B_USER_DESKBAR_DIRECTORY, &user) == B_OK)
		folders.push_back(user);
	return folders;
}


//	A menu item with the icon of what it opens, as Photon's Launch menu had.
IconMenuItem::IconMenuItem(const char* label, BMessage* message,
	const entry_ref& ref)
	:
	BMenuItem(label, message),
	fIcon(icon_for(ref, kIconSize))
{
}


IconMenuItem::IconMenuItem(BMenu* submenu, const entry_ref* ref)
	:
	BMenuItem(submenu),
	fIcon(ref != NULL ? icon_for(*ref, kIconSize) : NULL)
{
}


IconMenuItem::~IconMenuItem()
{
	delete fIcon;
}


void
IconMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += kIconSize + 6;
	*height = std::max(*height, kIconSize + 2);
}


void
IconMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	const BPoint start = menu->PenLocation();
	const BRect frame = Frame();
	draw_icon(menu, fIcon, BPoint(start.x,
		floorf((frame.top + frame.bottom - kIconSize) / 2) + 1));
	menu->MovePenTo(start.x + kIconSize + 6, start.y);
	BMenuItem::DrawContent();
}


/*!	Fills \a menu from \a folders, merged: a folder in any becomes a submenu
	made from all of them, an entry an item that opens it; sorted by name.
*/
void
fill_menu(BMenu* menu, const std::vector<BPath>& folders, BHandler* target)
{
	std::map<BString, std::vector<BPath>> submenus;
	std::map<BString, entry_ref> items;
	for (const BPath& folder : folders) {
		BDirectory directory(folder.Path());
		BEntry entry;
		while (directory.GetNextEntry(&entry, false) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			entry.GetName(name);
			BEntry resolved(entry);
			resolved.SetTo(&directory, name, true);
			BPath path;
			if (resolved.IsDirectory() && entry.GetPath(&path) == B_OK)
				submenus[name].push_back(path);
			else {
				entry_ref ref;
				if (resolved.GetRef(&ref) == B_OK && resolved.Exists())
					items[name] = ref;
			}
		}
	}
	for (auto& submenu : submenus) {
		BMenu* sub = new BMenu(submenu.first.String());
		fill_menu(sub, submenu.second, target);
		entry_ref folder;
		const bool hasRef = get_ref_for_path(submenu.second.front().Path(),
			&folder) == B_OK;
		menu->AddItem(new IconMenuItem(sub, hasRef ? &folder : NULL));
	}
	if (!submenus.empty() && !items.empty())
		menu->AddSeparatorItem();
	for (auto& item : items) {
		BMessage* message = new BMessage(kLaunch);
		message->AddRef("refs", &item.second);
		BMenuItem* menuItem = new IconMenuItem(item.first.String(), message,
			item.second);
		menuItem->SetTarget(target);
		menu->AddItem(menuItem);
	}
}


void
launch(const entry_ref& ref)
{
	BEntry entry(&ref, true);
	entry_ref target;
	if (entry.GetRef(&target) != B_OK)
		target = ref;
	// a folder opens in Tracker
	if (entry.IsDirectory()) {
		BMessage open(B_REFS_RECEIVED);
		open.AddRef("refs", &target);
		BMessenger(kTrackerSignature).SendMessage(&open);
		return;
	}
	be_roster->Launch(&target);
}


//	#pragma mark - keeping windows clear


/*!	A window that opens under what a layout takes is moved out, and shrunk if
	it doesn't fit, and one that is zoomed or resized by its application,
	rather than with the mouse, is fitted to the rest of the screen. One
	dragged under them stays there.
*/
void
keep_clear(const client_window_info* info, BRect free, BRect screen,
	std::map<int32, BRect>& frames)
{
	if (info->is_mini)
		return;
	const BRect frame(info->window_left, info->window_top,
		info->window_right, info->window_bottom);
	auto known = frames.find(info->server_token);
	const bool seen = known != frames.end();
	// a new size with no button held: a zoom, or the application's own
	BPoint where;
	uint32 buttons = 0;
	get_mouse(&where, &buttons);
	const bool resized = seen && buttons == 0
		&& (known->second.Width() != frame.Width()
			|| known->second.Height() != frame.Height());
	if (buttons == 0 || !seen)
		frames[info->server_token] = frame;

	// the window with its border and tab
	const float border = info->border_size;
	const float tab = info->tab_height;
	const BRect outer(frame.left - border, frame.top - tab - border,
		frame.right + border, frame.bottom + border);
	if (free.Contains(outer))
		return;

	// zoomed, as Haiku zooms with no Deskbar: across the whole screen
	const bool zoomed = outer.left <= screen.left + border
		&& outer.right >= screen.right - border
		&& outer.Height() >= screen.Height() * 0.9f;
	if (seen && !zoomed && !resized)
		return;

	const bool resizable = (info->flags & B_NOT_RESIZABLE) == 0;
	BRect fitted(frame);
	if (zoomed && resizable) {
		fitted.Set(free.left + border, free.top + tab + border,
			free.right - border, free.bottom - border);
	} else {
		// in from the right and bottom, then the left and top
		if (outer.right > free.right)
			fitted.OffsetBy(free.right - outer.right, 0);
		if (outer.bottom > free.bottom)
			fitted.OffsetBy(0, free.bottom - outer.bottom);
		if (fitted.left - border < free.left)
			fitted.OffsetBy(free.left + border - fitted.left, 0);
		if (fitted.top - tab - border < free.top)
			fitted.OffsetBy(0, free.top + tab + border - fitted.top);
		if (resizable) {
			fitted.right = std::min(fitted.right, free.right - border);
			fitted.bottom = std::min(fitted.bottom, free.bottom - border);
		}
	}
	if (fitted == frame)
		return;

	BMessage message(B_SET_PROPERTY);
	message.AddRect("data", fitted);
	message.AddSpecifier("Frame");
	message.AddSpecifier("Window", info->name);
	BMessenger(NULL, info->team).SendMessage(&message);
}


void
shut_down()
{
	BAlert* alert = new BAlert("Shutdown",
		"Shut down or restart the computer?", "Cancel", "Restart",
		"Shut down", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	const int32 choice = alert->Go();
	if (choice == 1)
		system("shutdown -r &");
	else if (choice == 2)
		system("shutdown &");
}


//	#pragma mark - the taskbar


struct TaskButton {
	int32		token;
	team_id		team;
	BString		title;
	bool		minimized;
	BRect		frame;
};


class TaskbarView : public BView {
public:
	TaskbarView(BRect frame)
		:
		BView(frame, "taskbar", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		fActive(-1),
		fPressed(-1),
		fLaunchPressed(false)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	~TaskbarView()
	{
		for (auto& icon : fIcons)
			delete icon.second;
	}

	virtual void Draw(BRect update)
	{
		sPhoton = photon_look();
		const BRect b = Bounds();
		// the band stops where the shelf comes down to the trough
		const float shelfLeft = b.right - shelf_width() + 1;
		draw_band(this, BRect(b.left, b.top, shelfLeft + kBand - 1,
			b.top + kBand - 1), true);
		fill(this, shelfLeft + kBand, b.top, b.right, b.top + kBand - 1,
			sPhoton ? kFace : panel());
		const BRect trough(b.left, b.top + kBand, b.right, b.bottom);
		draw_trough(this, trough);

		// Launch, at the left end, with a sun
		const BRect launchButton = _LaunchFrame();
		draw_raised(this, launchButton, kFace, fLaunchPressed);
		_DrawSun(BPoint(launchButton.left + 13, (launchButton.top
			+ launchButton.bottom) / 2 + (fLaunchPressed ? 1 : 0)));
		draw_text(this, "Launch", *be_plain_font, launchButton,
			launchButton.left + 24 + (fLaunchPressed ? 1 : 0), button_text());

		for (size_t i = 0; i < fButtons.size(); i++)
			_DrawButton(fButtons[i], (int32)i == fPressed);

		// the clock, at the right end
		const BRect clock = _ClockFrame();
		draw_raised(this, clock, kFace);
		char text[64];
		time_t now = time(NULL);
		strftime(text, sizeof(text), "%a-%d %I:%M%p", localtime(&now));
		const float width = be_plain_font->StringWidth(text);
		draw_text(this, text, *be_plain_font, clock,
			floorf((clock.left + clock.right - width) / 2), button_text());
	}

	virtual void MouseDown(BPoint where)
	{
		if (_LaunchFrame().Contains(where)) {
			fLaunchPressed = true;
			Invalidate(_LaunchFrame());
			Window()->UpdateIfNeeded();
			_OpenLaunchMenu();
			fLaunchPressed = false;
			Invalidate(_LaunchFrame());
			return;
		}
		for (size_t i = 0; i < fButtons.size(); i++) {
			if (fButtons[i].frame.Contains(where)) {
				fPressed = (int32)i;
				Invalidate(fButtons[i].frame);
				SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
				return;
			}
		}
	}

	virtual void MouseUp(BPoint where)
	{
		if (fPressed < 0 || fPressed >= (int32)fButtons.size()) {
			fPressed = -1;
			return;
		}
		const TaskButton button = fButtons[fPressed];
		Invalidate(button.frame);
		fPressed = -1;
		if (!button.frame.Contains(where))
			return;
		// the window in front is minimized; any other comes to the front
		if (button.token == fActive && !button.minimized)
			do_window_action(button.token, B_MINIMIZE_WINDOW, BRect(), false);
		else
			do_window_action(button.token, B_BRING_TO_FRONT, BRect(), false);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kLaunch:
			{
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK)
					launch(ref);
				break;
			}
			case kShutDown:
				shut_down();
				break;
			default:
				BView::MessageReceived(message);
		}
	}

	//!	Follows the windows, and redraws if they or the time changed.
	void Update(bool clock)
	{
		std::vector<TaskButton> buttons;
		int32 active = -1;
		const BRect screen = BScreen(Window()).Frame();

		// the window in front, of those listed
		int32* order = NULL;
		int32 orderCount = 0;
		BPrivate::get_window_order(current_workspace(), &order, &orderCount);

		app_info appInfo;
		BList teams;
		be_roster->GetAppList(&teams);
		for (int32 t = 0; t < teams.CountItems(); t++) {
			const team_id team = (team_id)(addr_t)teams.ItemAt(t);
			if (team == be_app->Team())
				continue;
			if (be_roster->GetRunningAppInfo(team, &appInfo) == B_OK
				&& strcasecmp(appInfo.signature, kDeskbarSignature) == 0)
				continue;
			int32 count = 0;
			int32* tokens = get_token_list(team, &count);
			for (int32 i = 0; i < count; i++) {
				client_window_info* info = get_window_info(tokens[i]);
				if (info == NULL)
					continue;
				if (info->feel == B_NORMAL_WINDOW_FEEL
					&& info->show_hide_level <= 0
					&& (info->workspaces & (1UL << current_workspace())) != 0
					&& info->name[0] != '\0') {
					keep_clear(info, BRect(screen.left, screen.top,
						screen.right - shelf_width(),
						screen.bottom - kBand - taskbar_height()), screen,
						fFrames);
					TaskButton button;
					button.token = info->server_token;
					button.team = team;
					button.title = info->name;
					button.minimized = info->is_mini;
					buttons.push_back(button);
				}
				free(info);
			}
			free(tokens);
		}
		for (int32 i = 0; i < orderCount && active < 0; i++) {
			for (const TaskButton& button : buttons) {
				if (button.token == order[i] && !button.minimized) {
					active = button.token;
					break;
				}
			}
		}
		free(order);

		// keep the order they first appeared in, new ones at the end
		std::vector<TaskButton> ordered;
		for (const TaskButton& old : fButtons) {
			for (const TaskButton& button : buttons) {
				if (button.token == old.token) {
					ordered.push_back(button);
					break;
				}
			}
		}
		for (const TaskButton& button : buttons) {
			bool known = false;
			for (const TaskButton& old : ordered)
				known |= old.token == button.token;
			if (!known)
				ordered.push_back(button);
		}
		_Layout(ordered);

		bool changed = active != fActive || ordered.size() != fButtons.size();
		for (size_t i = 0; !changed && i < ordered.size(); i++) {
			changed = ordered[i].token != fButtons[i].token
				|| ordered[i].title != fButtons[i].title
				|| ordered[i].minimized != fButtons[i].minimized
				|| ordered[i].frame != fButtons[i].frame;
		}
		fButtons = ordered;
		fActive = active;
		if (changed)
			Invalidate();
		else if (clock)
			Invalidate(_ClockFrame());
	}

private:
	BRect _LaunchFrame() const
	{
		const BRect b = Bounds();
		return BRect(b.left, b.top + kBand, b.left + launch_width() - 1,
			b.bottom);
	}

	BRect _ClockFrame() const
	{
		const BRect b = Bounds();
		return BRect(b.right - clock_width() + 1, b.top + kBand, b.right,
			b.bottom);
	}

	//!	Side by side after Launch, narrower when there are many.
	void _Layout(std::vector<TaskButton>& buttons)
	{
		const BRect b = Bounds();
		const float left = b.left + launch_width() + 2;
		const float right = b.right - clock_width() - 2;
		const float room = right - left + 1;
		float width = kTaskWidth;
		if (!buttons.empty())
			width = std::min(kTaskWidth, floorf(room / buttons.size()) - 2);
		width = std::max(width, 24.0f);
		float x = left;
		for (TaskButton& button : buttons) {
			button.frame.Set(x, b.top + kBand, x + width - 1, b.bottom);
			x += width + 2;
		}
	}

	void _DrawButton(const TaskButton& button, bool pressed)
	{
		BRect r = button.frame;
		const bool active = button.token == fActive || pressed;
		if (active && !sPhoton)
			themed_button(this, r, true);
		else if (active) {
			// the ridge round it, then a black edge round a cream face
			fill(this, r.left, r.top, r.right, r.top, kWhite);
			fill(this, r.left, r.top, r.left, r.bottom, kWhite);
			fill(this, r.left + 1, r.top + 1, r.right, r.bottom, kFace);
			fill(this, r.right, r.top + 1, r.right, r.bottom, kFaceShade);
			fill(this, r.left + 1, r.bottom, r.right, r.bottom, kFaceShade);
			BRect inner(r.left + 3, r.top + 2, r.right - 2, r.bottom - 1);
			SetHighColor(kBlack);
			StrokeRect(inner);
			fill(this, inner.left + 1, inner.top + 1, inner.right - 1,
				inner.bottom - 1, kActiveFace);
			fill(this, inner.left + 1, inner.top + 1, inner.right - 1,
				inner.top + 1, kActiveShade);
			fill(this, inner.left + 1, inner.top + 1, inner.left + 1,
				inner.bottom - 1, kActiveShade);
		} else
			draw_raised(this, r, kFace);

		const float shift = active ? 1 : 0;
		BBitmap* icon = _IconFor(button.team);
		float x = r.left + 7 + shift;
		if (icon != NULL) {
			draw_icon(this, icon, BPoint(x, floorf((r.top + r.bottom
				- kIconSize) / 2) + shift));
			x += kIconSize + 5;
		}
		BString title(button.title);
		if (button.minimized)
			title.Prepend("[").Append("]");
		draw_text(this, title.String(), *be_plain_font, r, x, button_text());
	}

	BBitmap* _IconFor(team_id team)
	{
		auto found = fIcons.find(team);
		if (found != fIcons.end())
			return found->second;
		app_info info;
		BBitmap* icon = NULL;
		if (be_roster->GetRunningAppInfo(team, &info) == B_OK)
			icon = icon_for(info.ref, kIconSize);
		fIcons[team] = icon;
		return icon;
	}

	//!	The sun: an eight-pointed yellow star edged in black, grey inside.
	void _DrawSun(BPoint c)
	{
		const rgb_color yellow = { 0xff, 0xd0, 0x40, 255 };
		const rgb_color grey = { 0xb8, 0xb8, 0xb8, 255 };
		c.x = floorf(c.x) + 0.5f;
		c.y = floorf(c.y) + 0.5f;
		BPoint star[16];
		for (int32 i = 0; i < 16; i++) {
			const float a = i * M_PI / 8 - M_PI / 2;
			const float radius = i % 2 == 0 ? 7.5f : 4.5f;
			star[i] = BPoint(c.x + cosf(a) * radius, c.y + sinf(a) * radius);
		}
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		SetHighColor(yellow);
		FillPolygon(star, 16);
		SetHighColor(kBlack);
		StrokePolygon(star, 16);
		SetHighColor(grey);
		FillEllipse(c, 2, 2);
		SetHighColor(kBlack);
		StrokeEllipse(c, 2.5f, 2.5f);
		SetDrawingMode(B_OP_COPY);
	}

	void _OpenLaunchMenu()
	{
		BPopUpMenu* menu = new BPopUpMenu("Launch", false, false);
		fill_menu(menu, menu_folders(), this);
		menu->AddSeparatorItem();
		BMenuItem* shutDown = new BMenuItem("Shutdown" B_UTF8_ELLIPSIS,
			new BMessage(kShutDown));
		shutDown->SetTarget(this);
		menu->AddItem(shutDown);
		menu->SetTargetForItems(this);
		for (int32 i = 0; i < menu->CountItems(); i++) {
			BMenu* sub = menu->SubmenuAt(i);
			if (sub != NULL)
				sub->SetTargetForItems(this);
		}

		// it opens upwards, from Launch's top left corner
		const BRect launchButton = _LaunchFrame();
		BPoint where = ConvertToScreen(launchButton.LeftTop());
		menu->SetAsyncAutoDestruct(true);
		BRect screen = BScreen(Window()).Frame();
		float width, height;
		menu->GetPreferredSize(&width, &height);
		where.y = std::max(screen.top, where.y - height - 1);
		menu->Go(where, true, true,
			ConvertToScreen(launchButton), true);
	}

	std::vector<TaskButton>	fButtons;
	std::map<team_id, BBitmap*> fIcons;
	// each window's frame when last seen with no button held
	std::map<int32, BRect> fFrames;
	int32				fActive;
	int32				fPressed;
	bool				fLaunchPressed;
};


//	#pragma mark - the crash guard


/*!	launch_daemon starts the shell again whenever it ends, as it does the
	Deskbar, but with no limit; this keeps a replicant that crashes it from
	doing so over and over. A replicant that crashed the shell while it was
	being loaded is left out from then on. After four starts in two minutes
	without a clean quit, the shell starts with no replicants; after six, it
	gives up and brings the Deskbar back.
*/
class CrashGuard {
public:
	enum mode { NORMAL, NO_REPLICANTS, GIVE_UP };

	CrashGuard()
	{
		_Path(fGuard, kGuardName);
		_Path(fLoading, kLoadingName);
		BFile file(fGuard.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK)
			fSettings.Unflatten(&file);

		// a replicant was being loaded when the last one ended
		BFile loading(fLoading.Path(), B_READ_ONLY);
		BMessage crashed;
		if (loading.InitCheck() == B_OK && crashed.Unflatten(&loading) == B_OK) {
			const char* path;
			if (crashed.FindString("path", &path) == B_OK)
				fSettings.AddString("skip", path);
		}
		BEntry(fLoading.Path()).Remove();

		// the recent starts, this one included
		const bigtime_t now = real_time_clock_usecs();
		BMessage starts;
		int64 when;
		int32 recent = 0;
		for (int32 i = 0; fSettings.FindInt64("start", i, &when) == B_OK; i++) {
			if (now - when < 120000000) {
				starts.AddInt64("start", when);
				recent++;
			}
		}
		fSettings.RemoveName("start");
		for (int32 i = 0; starts.FindInt64("start", i, &when) == B_OK; i++)
			fSettings.AddInt64("start", when);
		fSettings.AddInt64("start", now);
		recent++;
		_Save();

		fMode = recent >= 6 ? GIVE_UP : recent >= 4 ? NO_REPLICANTS : NORMAL;
	}

	mode Mode() const
	{
		return fMode;
	}

	bool Skipped(const char* path) const
	{
		const char* skip;
		for (int32 i = 0; fSettings.FindString("skip", i, &skip) == B_OK; i++) {
			if (strcmp(skip, path) == 0)
				return true;
		}
		return false;
	}

	//!	Marks \a path as being loaded, or none if NULL.
	void Loading(const char* path)
	{
		if (path == NULL) {
			BEntry(fLoading.Path()).Remove();
			return;
		}
		BMessage message;
		message.AddString("path", path);
		BFile file(fLoading.Path(), B_WRITE_ONLY | B_CREATE_FILE
			| B_ERASE_FILE);
		if (file.InitCheck() == B_OK) {
			message.Flatten(&file);
			file.Sync();
		}
	}

	//!	A clean quit: the starts so far don't count against the next.
	void Quit()
	{
		fSettings.RemoveName("start");
		_Save();
	}

private:
	static void _Path(BPath& path, const char* name)
	{
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
			path.Append(name);
	}

	void _Save()
	{
		BFile file(fGuard.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			fSettings.Flatten(&file);
	}

	BPath				fGuard;
	BPath				fLoading;
	BMessage			fSettings;
	mode				fMode;
};


static CrashGuard* sGuard = NULL;


//	#pragma mark - replicants


// how big a replicant may be: as Deskbar's at its smallest
static const float kReplicantHeight = 16;


/*!	The Deskbar's replicants, as it keeps them: add-ons and applications
	listed in its settings, each asked for its view, which is archived and
	made again from the archive, as the Deskbar's shelf does.
*/
void
load_replicants(BView* tray, float maxWidth)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("deskbar/replicants") != B_OK)
		return;
	BFile file(path.Path(), B_READ_ONLY);
	BMessage settings;
	if (file.InitCheck() != B_OK || settings.Unflatten(&file) != B_OK)
		return;

	if (sGuard != NULL && sGuard->Mode() != CrashGuard::NORMAL)
		return;
	const char* entry;
	for (int32 i = 0; settings.FindString("replicant_path", i, &entry)
			== B_OK; i++) {
		if (sGuard != NULL && sGuard->Skipped(entry))
			continue;
		if (sGuard != NULL)
			sGuard->Loading(entry);
		BEntry addOn(entry, true);
		entry_ref ref;
		if (!addOn.Exists() || addOn.GetRef(&ref) != B_OK)
			continue;
		const image_id image = load_add_on(entry);
		if (image < B_OK)
			continue;
		BView* (*entryFunction)(image_id, const entry_ref*, float, float);
		BView* (*itemFunction)(float, float);
		BView* view = NULL;
		if (get_image_symbol(image, "instantiate_deskbar_entry",
				B_SYMBOL_TYPE_TEXT, (void**)&entryFunction) == B_OK)
			view = entryFunction(image, &ref, maxWidth, kReplicantHeight);
		else if (get_image_symbol(image, "instantiate_deskbar_item",
				B_SYMBOL_TYPE_TEXT, (void**)&itemFunction) == B_OK)
			view = itemFunction(maxWidth, kReplicantHeight);
		if (view == NULL) {
			unload_add_on(image);
			continue;
		}

		BMessage archive;
		const bool archived = view->Archive(&archive) == B_OK;
		BView* made = NULL;
		if (archived) {
			made = dynamic_cast<BView*>(instantiate_object(&archive));
		}
		if (made != NULL)
			delete view;
		else
			made = view;
		// some follow their parent's edges, as the Deskbar's tray never
		// changes size; here they keep their own
		const BRect frame = made->Frame();
		made->SetResizingMode(B_FOLLOW_NONE);
		tray->AddChild(made);
		made->ResizeTo(frame.Width(), frame.Height());
	}
	if (sGuard != NULL)
		sGuard->Loading(NULL);
}


//	#pragma mark - the shelf


struct ShelfItem {
	BString		label;
	entry_ref	ref;
	BBitmap*	icon;
	BRect		frame;
};

struct ShelfGroup {
	BString		title;
	bool		open;
	bool		monitor;
	std::vector<ShelfItem> items;
	BRect		header;
	// the Deskbar's replicants, in a view of their own
	bool		tray;
};

static const float kReplicantGap = 4;
static const float kMonitorHeight = 66;


//!	The first of \a names found in the system's apps, preferences or bin.
bool
find_program(const char* name, entry_ref& ref)
{
	const directory_which places[] = { B_SYSTEM_APPS_DIRECTORY,
		B_SYSTEM_PREFERENCES_DIRECTORY, B_USER_APPS_DIRECTORY,
		B_SYSTEM_BIN_DIRECTORY };
	for (size_t i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
		BPath path;
		if (find_directory(places[i], &path) != B_OK
			|| path.Append(name) != B_OK)
			continue;
		BEntry entry(path.Path(), true);
		if (entry.Exists() && entry.GetRef(&ref) == B_OK)
			return true;
	}
	return false;
}


class ShelfView : public BView {
public:
	ShelfView(BRect frame)
		:
		BView(frame, "shelf", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		fPressedGroup(-1),
		fPressedItem(-1),
		fCPU(0),
		fMemory(0),
		fDisk(0),
		fSwap(0),
		fLastActive(0),
		fLastTime(0)
	{
		SetViewColor(B_TRANSPARENT_COLOR);

		struct { const char* label; const char* program; } applications[] = {
			{ "Home", NULL }, { "WebPositive", "WebPositive" },
			{ "Mail", "Mail" }, { "Editor", "StyledEdit" },
			{ "Terminal", "Terminal" }, { "Media Player", "MediaPlayer" },
			{ "Images", "ShowImage" }, { "People", "People" } };
		struct { const char* label; const char* program; } utilities[] = {
			{ "Calculator", "DeskCalc" },
			{ "Activity Monitor", "ActivityMonitor" },
			{ "Screenshot", "Screenshot" }, { "Expander", "Expander" },
			{ "Disk Setup", "DriveSetup" }, { "Software", "HaikuDepot" },
			{ "Updates", "SoftwareUpdater" }, { "Installer", "Installer" } };
		struct { const char* label; const char* program; } preferences[] = {
			{ "Network", "Network" }, { "Mouse", "Input" },
			{ "Graphics", "Screen" }, { "Appearance", "Appearance" },
			{ "Backgrounds", "Backgrounds" }, { "Time & Date", "Time" },
			{ "Themes", "PowerTheme" }, { "Localization", "Locale" } };

		ShelfGroup apps = { "Applications", true, false, {}, BRect() };
		for (auto& a : applications) {
			ShelfItem item = { a.label, entry_ref(), NULL, BRect() };
			if (a.program == NULL) {
				BPath home;
				if (find_directory(B_USER_DIRECTORY, &home) != B_OK
					|| get_ref_for_path(home.Path(), &item.ref) != B_OK)
					continue;
			} else if (!find_program(a.program, item.ref))
				continue;
			item.icon = icon_for(item.ref, kIconSize);
			apps.items.push_back(item);
		}
		ShelfGroup utils = { "Utilities", false, false, {}, BRect() };
		for (auto& a : utilities) {
			ShelfItem item = { a.label, entry_ref(), NULL, BRect() };
			if (!find_program(a.program, item.ref))
				continue;
			item.icon = icon_for(item.ref, kIconSize);
			utils.items.push_back(item);
		}
		ShelfGroup configure = { "Configure", true, false, {}, BRect() };
		for (auto& a : preferences) {
			ShelfItem item = { a.label, entry_ref(), NULL, BRect() };
			if (!find_program(a.program, item.ref)) {
				// Power Theme is in the user's or the system's apps
				BPath path;
				if (strcmp(a.program, "PowerTheme") != 0
					|| find_directory(B_SYSTEM_APPS_DIRECTORY, &path) != B_OK)
					continue;
				path.Append(a.program);
				if (get_ref_for_path(path.Path(), &item.ref) != B_OK
					|| !BEntry(&item.ref).Exists())
					continue;
			}
			item.icon = icon_for(item.ref, kIconSize);
			configure.items.push_back(item);
		}
		ShelfGroup monitor = { "System Monitor", true, true, {}, BRect(),
			false };
		ShelfGroup replicants = { "Replicants", true, false, {}, BRect(),
			true };
		fGroups.push_back(apps);
		fGroups.push_back(utils);
		fGroups.push_back(configure);
		fGroups.push_back(monitor);
		fGroups.push_back(replicants);
		_LoadSettings();

		fTray = new BView(BRect(0, 0, 10, 10), "replicants", B_FOLLOW_NONE,
			B_WILL_DRAW);
	}

	~ShelfView()
	{
		for (ShelfGroup& group : fGroups) {
			for (ShelfItem& item : group.items)
				delete item.icon;
		}
	}

	virtual void AttachedToWindow()
	{
		sPhoton = photon_look();
		fTray->SetViewColor(sPhoton ? kFace : panel());
		fTray->SetLowColor(fTray->ViewColor());
		AddChild(fTray);
		load_replicants(fTray, Bounds().Width() - kBand - 8);
		Relayout();
	}

	virtual void FrameResized(float, float)
	{
		Relayout();
		Invalidate();
	}

	//!	Lays the groups out again, and moves the replicants with them.
	void Relayout()
	{
		sPhoton = photon_look();
		const rgb_color color = sPhoton ? kFace : panel();
		if (color != fTray->ViewColor()) {
			fTray->SetViewColor(color);
			fTray->SetLowColor(color);
			fTray->Invalidate();
			for (int32 i = 0; i < fTray->CountChildren(); i++)
				fTray->ChildAt(i)->Invalidate();
		}
		_Layout();
		_PlaceTray();
	}

	virtual void Draw(BRect update)
	{
		sPhoton = photon_look();
		_Layout();
		const BRect b = Bounds();
		draw_band(this, BRect(b.left, b.top, b.left + kBand - 1, b.bottom),
			false);
		_DrawCorner();
		draw_trough(this, BRect(b.left + kBand, b.top, b.right, b.bottom));

		for (size_t g = 0; g < fGroups.size(); g++) {
			const ShelfGroup& group = fGroups[g];
			// a dark line over the lower shelf, and under the upper
			if (group.header.top == fLowerTop) {
				fill(this, group.header.left, group.header.top - 1, b.right,
					group.header.top - 1, sPhoton ? kOutline
						: tint_color(panel(), B_DARKEN_3_TINT));
			}
			_DrawHeader(group, (int32)g == fPressedGroup && fPressedItem < 0);
			if (!group.open)
				continue;
			if (group.monitor) {
				_DrawMonitor(group);
				continue;
			}
			if (group.tray) {
				draw_plain(this, _TrayArea(group));
				continue;
			}
			for (size_t i = 0; i < group.items.size(); i++) {
				_DrawItem(group.items[i], (int32)g == fPressedGroup
					&& (int32)i == fPressedItem);
			}
		}
		if (fUpperBottom >= 0) {
			fill(this, b.left + kBand, fUpperBottom + 1, b.right,
				fUpperBottom + 1, sPhoton ? kOutline
					: tint_color(panel(), B_DARKEN_3_TINT));
		}
	}

	virtual void MouseDown(BPoint where)
	{
		_Layout();
		for (size_t g = 0; g < fGroups.size(); g++) {
			ShelfGroup& group = fGroups[g];
			if (group.header.Contains(where)) {
				fPressedGroup = (int32)g;
				fPressedItem = -1;
				break;
			}
			if (!group.open || group.monitor)
				continue;
			for (size_t i = 0; i < group.items.size(); i++) {
				if (group.items[i].frame.Contains(where)) {
					fPressedGroup = (int32)g;
					fPressedItem = (int32)i;
					break;
				}
			}
		}
		if (fPressedGroup >= 0) {
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			Invalidate();
		}
	}

	virtual void MouseUp(BPoint where)
	{
		if (fPressedGroup < 0)
			return;
		ShelfGroup& group = fGroups[fPressedGroup];
		if (fPressedItem < 0) {
			if (group.header.Contains(where)) {
				group.open = !group.open;
				_SaveSettings();
				Relayout();
			}
		} else if (fPressedItem < (int32)group.items.size()
			&& group.items[fPressedItem].frame.Contains(where))
			launch(group.items[fPressedItem].ref);
		fPressedGroup = -1;
		fPressedItem = -1;
		Invalidate();
	}

	//!	Reads the processors', memory's and disk's use, and redraws the gauges.
	void Update()
	{
		system_info info;
		if (get_system_info(&info) == B_OK) {
			const uint32 count = info.cpu_count;
			cpu_info* cpus = new cpu_info[count];
			if (get_cpu_info(0, count, cpus) == B_OK) {
				bigtime_t active = 0;
				for (uint32 i = 0; i < count; i++)
					active += cpus[i].active_time;
				const bigtime_t now = system_time();
				if (fLastTime > 0 && now > fLastTime) {
					fCPU = std::min(1.0f, std::max(0.0f,
						(float)(active - fLastActive)
							/ ((now - fLastTime) * (float)count)));
				}
				fLastActive = active;
				fLastTime = now;
			}
			delete[] cpus;
			fMemory = info.max_pages > 0
				? (float)info.used_pages / info.max_pages : 0;
			fSwap = info.max_swap_pages > 0 ? 1.0f
				- (float)info.free_swap_pages / info.max_swap_pages : 0;
		}
		fs_info fs;
		BPath boot;
		if (find_directory(B_SYSTEM_DIRECTORY, &boot) == B_OK
			&& fs_stat_dev(dev_for_path(boot.Path()), &fs) == 0
			&& fs.total_blocks > 0)
			fDisk = 1.0f - (float)fs.free_blocks / fs.total_blocks;

		for (const ShelfGroup& group : fGroups) {
			if (group.monitor && group.open)
				Invalidate(BRect(group.header.left, group.header.bottom + 1,
					group.header.right, group.header.bottom + kMonitorHeight));
		}
	}

private:
	//!	The upper groups from the top, the monitor from the bottom.
	void _Layout()
	{
		const BRect b = Bounds();
		const float left = b.left + kBand;
		float y = b.top;
		fUpperBottom = -1;
		for (ShelfGroup& group : fGroups) {
			if (group.monitor)
				continue;
			group.header.Set(left, y, b.right, y + header_height() - 1);
			y += header_height();
			for (ShelfItem& item : group.items) {
				if (group.open) {
					item.frame.Set(left, y, b.right, y + item_height() - 1);
					y += item_height();
				} else
					item.frame.Set(0, 0, -1, -1);
			}
			fUpperBottom = y - 1;
		}
		// the monitor and the replicants, stacked up from the bottom
		float bottom = b.bottom;
		fLowerTop = -1;
		for (auto group = fGroups.rbegin(); group != fGroups.rend(); group++) {
			if (!group->monitor && !group->tray)
				continue;
			float body = 0;
			if (group->open)
				body = group->monitor ? kMonitorHeight : _TrayHeight();
			const float top = std::max(y + 2,
				bottom - header_height() - body + 1);
			group->header.Set(left, top, b.right, top + header_height() - 1);
			bottom = top - 1;
			fLowerTop = top;
		}
	}

	//!	Below a replicants group's title, down to the next group or the end.
	BRect _TrayArea(const ShelfGroup& group) const
	{
		float bottom = Bounds().bottom;
		for (const ShelfGroup& other : fGroups) {
			if (other.header.top > group.header.top)
				bottom = std::min(bottom, other.header.top - 1);
		}
		return BRect(group.header.left, group.header.bottom + 1,
			group.header.right, bottom);
	}

	//!	Where each replicant goes, in rows, and so how tall they all are.
	float _ArrangeTray(bool move)
	{
		const float width = Bounds().Width() - kBand - 6;
		float x = 0, y = 0, rowHeight = 0;
		for (int32 i = 0; i < fTray->CountChildren(); i++) {
			BView* child = fTray->ChildAt(i);
			const float w = child->Bounds().Width() + 1;
			const float h = child->Bounds().Height() + 1;
			if (x > 0 && x + w > width) {
				x = 0;
				y += rowHeight + kReplicantGap;
				rowHeight = 0;
			}
			if (move)
				child->MoveTo(x, y);
			x += w + kReplicantGap;
			rowHeight = std::max(rowHeight, h);
		}
		return fTray->CountChildren() > 0 ? y + rowHeight : 0;
	}

	float _TrayHeight()
	{
		return _ArrangeTray(false) + 8;
	}

	void _PlaceTray()
	{
		for (const ShelfGroup& group : fGroups) {
			if (!group.tray)
				continue;
			if (!group.open) {
				if (!fTray->IsHidden())
					fTray->Hide();
				return;
			}
			const BRect area = _TrayArea(group).InsetByCopy(4, 4);
			fTray->MoveTo(area.LeftTop());
			fTray->ResizeTo(std::max(0.0f, area.Width()),
				std::max(0.0f, area.Height()));
			_ArrangeTray(true);
			if (fTray->IsHidden())
				fTray->Show();
		}
	}

	/*!	Where the shelf's band meets the taskbar's, in the shelf's last rows,
		over the taskbar's band: the outer line turns along the taskbar's top,
		the light lines stop short, and the shaded and inner dark lines meet,
		as a frame's corner does.
	*/
	void _DrawCorner()
	{
		rgb_color lines[6];
		band_lines(lines);
		const BRect b = Bounds();
		const float top = b.bottom - kBand + 1;
		for (int32 i = 1; i < 6; i++) {
			const float y = top + i;
			for (int32 column = 0; column < 6; column++) {
				rgb_color color = lines[2];
				if (i == 5 || column == 5)
					color = lines[5];
				else if (i == 4 || column == 4)
					color = lines[4];
				fill(this, b.left + column, y, b.left + column, y, color);
			}
		}
	}

	void _DrawHeader(const ShelfGroup& group, bool pressed)
	{
		const BRect& h = group.header;
		const BRect toggle(h.left, h.top, h.left + kToggleWidth - 1,
			h.bottom);
		const BRect label(toggle.right + 1, h.top, h.right, h.bottom);
		draw_header(this, toggle, kToggleFace, pressed);
		draw_header(this, label, kHeaderFace, pressed);
		if (!sPhoton) {
			fill(this, toggle.right, toggle.top + 3, toggle.right,
				toggle.bottom - 3, tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
					B_DARKEN_2_TINT));
		}
		// the toggle's minus, with the plus's upright when it's closed
		const float cx = floorf((toggle.left + toggle.right) / 2);
		const float cy = floorf((toggle.top + toggle.bottom) / 2);
		const rgb_color mark = sPhoton ? kToggleMark : header_text();
		fill(this, cx - 3, cy, cx + 2, cy + 1, mark);
		if (!group.open)
			fill(this, cx - 1, cy - 2, cx, cy + 3, mark);
		draw_text(this, group.title.String(), *be_bold_font, label,
			label.left + 5, header_text());
	}

	void _DrawItem(const ShelfItem& item, bool pressed)
	{
		const BRect& r = item.frame;
		const BRect cell(r.left, r.top, r.left + kIconCellWidth - 1, r.bottom);
		const BRect label(cell.right + 1, r.top, r.right, r.bottom);
		draw_raised(this, cell, kIconCell, pressed);
		draw_raised(this, label, kFace, pressed);
		const float shift = pressed ? 1 : 0;
		draw_icon(this, item.icon, BPoint(floorf((cell.left + cell.right + 1
			- kIconSize) / 2) + shift, floorf((cell.top + cell.bottom + 1
			- kIconSize) / 2) + shift));
		draw_text(this, item.label.String(), *be_plain_font,
			label.OffsetByCopy(0, shift), label.left + 4 + shift, button_text());
	}

	//!	A gauge: a dark edge round a grey track, the level green-grey.
	void _DrawGauge(BRect r, float level)
	{
		if (!sPhoton) {
			BRect rect(r);
			be_control_look->DrawStatusBar(this, rect, r, panel(),
				ui_color(B_STATUS_BAR_COLOR),
				r.left + roundf(r.Width() * level));
			return;
		}
		SetHighColor(kOutline);
		StrokeRect(r);
		const BRect in(r.InsetByCopy(1, 1));
		fill(this, in.left, in.top, in.right, in.bottom, kGauge);
		fill(this, in.left, in.top, in.left, in.bottom, kGaugeShade);
		const float right = in.left + roundf((in.Width()) * level);
		if (right > in.left) {
			fill(this, in.left, in.top, right, in.bottom, kLevel);
			fill(this, in.left, in.top, right, in.top, kLevelLight);
			fill(this, in.left, in.top, in.left, in.bottom, kLevelLight);
			fill(this, right, in.top, right, in.bottom, kLevelShade);
			if (right + 1 <= in.right)
				fill(this, right + 1, in.top, right + 1, in.bottom, kGaugeEdge);
		}
	}

	void _DrawMonitor(const ShelfGroup& group)
	{
		const BRect area(group.header.left, group.header.bottom + 1,
			group.header.right, group.header.bottom + kMonitorHeight);
		draw_plain(this, area);
		const float gaugeLeft = area.left + 24;
		const float gaugeRight = area.right - 4;
		float y = area.top + 4;
		// processors: a chip
		_DrawChip(BPoint(area.left + 5, y + 1));
		_DrawGauge(BRect(gaugeLeft, y, gaugeRight, y + 15), fCPU);
		y += 20;
		// memory: a module
		_DrawModule(BPoint(area.left + 5, y + 4));
		_DrawGauge(BRect(gaugeLeft, y, gaugeRight, y + 15), fMemory);
		y += 20;
		// the disk, and swap
		_DrawDisk(BPoint(area.left + 5, y + 1));
		_DrawGauge(BRect(gaugeLeft, y, gaugeRight, y + 6), fDisk);
		_DrawGauge(BRect(gaugeLeft, y + 8, gaugeRight, y + 14), fSwap);
	}

	void _DrawChip(BPoint p)
	{
		const rgb_color gold = { 0xff, 0xcd, 0x48, 255 };
		for (int32 i = 0; i < 4; i++) {
			fill(this, p.x + 3 + i * 3, p.y, p.x + 3 + i * 3, p.y + 1, gold);
			fill(this, p.x + 3 + i * 3, p.y + 13, p.x + 3 + i * 3, p.y + 14,
				gold);
			fill(this, p.x, p.y + 3 + i * 3, p.x + 1, p.y + 3 + i * 3, gold);
			fill(this, p.x + 13, p.y + 3 + i * 3, p.x + 14, p.y + 3 + i * 3,
				gold);
		}
		fill(this, p.x + 2, p.y + 2, p.x + 12, p.y + 12, kBlack);
		fill(this, p.x + 5, p.y + 5, p.x + 9, p.y + 9, kOutline);
	}

	void _DrawModule(BPoint p)
	{
		const rgb_color board = { 0x5a, 0x8a, 0x50, 255 };
		fill(this, p.x, p.y, p.x + 14, p.y + 6, board);
		for (int32 i = 0; i < 3; i++)
			fill(this, p.x + 1 + i * 5, p.y + 1, p.x + 3 + i * 5, p.y + 4,
				kBlack);
		for (int32 x = 1; x < 14; x += 2)
			fill(this, p.x + x, p.y + 6, p.x + x, p.y + 6, kLevelLight);
	}

	void _DrawDisk(BPoint p)
	{
		draw_raised(this, BRect(p.x, p.y, p.x + 12, p.y + 14), kIconCell);
		fill(this, p.x + 3, p.y + 3, p.x + 9, p.y + 4, kOutline);
		const rgb_color amber = { 0xff, 0xb0, 0x20, 255 };
		fill(this, p.x + 6, p.y + 8, p.x + 12, p.y + 8, amber);
		fill(this, p.x + 12, p.y + 8, p.x + 12, p.y + 14, amber);
	}

	void _LoadSettings()
	{
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
			|| path.Append(kSettingsName) != B_OK)
			return;
		BFile file(path.Path(), B_READ_ONLY);
		BMessage settings;
		if (file.InitCheck() != B_OK || settings.Unflatten(&file) != B_OK)
			return;
		for (ShelfGroup& group : fGroups) {
			bool open;
			if (settings.FindBool(group.title.String(), &open) == B_OK)
				group.open = open;
		}
	}

	void _SaveSettings()
	{
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
			|| path.Append(kSettingsName) != B_OK)
			return;
		BMessage settings;
		for (const ShelfGroup& group : fGroups)
			settings.AddBool(group.title.String(), group.open);
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			settings.Flatten(&file);
	}

	std::vector<ShelfGroup> fGroups;
	BView*				fTray;
	float				fLowerTop;
	int32				fPressedGroup;
	int32				fPressedItem;
	float				fUpperBottom;
	float				fCPU;
	float				fMemory;
	float				fDisk;
	float				fSwap;
	bigtime_t			fLastActive;
	bigtime_t			fLastTime;
};


//	#pragma mark - windows


/*!	What the shell's look follows: the control look, the panel and menu
	colours and the plain font. A change of theme changes some of them.
*/
class LookWatch {
public:
	LookWatch()
		:
		fLook(NULL)
	{
		Changed();
	}

	bool Changed()
	{
		const rgb_color panelColor = ui_color(B_PANEL_BACKGROUND_COLOR);
		const rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
		const float size = be_plain_font->Size();
		const bool changed = be_control_look != fLook
			|| panelColor != fPanel || menuColor != fMenu || size != fSize;
		fLook = be_control_look;
		fPanel = panelColor;
		fMenu = menuColor;
		fSize = size;
		return changed;
	}

private:
	const BControlLook*	fLook;
	rgb_color			fPanel;
	rgb_color			fMenu;
	float				fSize;
};


static const uint32 kWindowFlags = B_NOT_MOVABLE | B_NOT_RESIZABLE
	| B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS
	| B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS;


class TaskbarWindow : public BWindow {
public:
	TaskbarWindow()
		:
		BWindow(BRect(0, 0, 100, 30), "Taskbar", B_NO_BORDER_WINDOW_LOOK,
			B_FLOATING_ALL_WINDOW_FEEL, kWindowFlags, B_ALL_WORKSPACES),
		fTicks(0)
	{
		fView = new TaskbarView(Bounds());
		AddChild(fView);
		_Place();
		BMessage tick(kTick);
		fRunner = new BMessageRunner(BMessenger(this), &tick, 500000);
		fView->Update(true);
	}

	~TaskbarWindow()
	{
		delete fRunner;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kTick:
				if (fWatch.Changed()) {
					_Place();
					fView->Invalidate();
				}
				fView->Update(++fTicks % 2 == 0);
				break;
			case B_COLORS_UPDATED:
			case B_FONTS_UPDATED:
				fView->Invalidate();
				break;
			case B_SCREEN_CHANGED:
				_Place();
				break;
			default:
				BWindow::MessageReceived(message);
		}
	}

private:
	void _Place()
	{
		const BRect screen = BScreen(this).Frame();
		const float height = kBand + taskbar_height();
		MoveTo(screen.left, screen.bottom - height + 1);
		ResizeTo(screen.Width(), height - 1);
	}

	TaskbarView*		fView;
	BMessageRunner*		fRunner;
	int32				fTicks;
	LookWatch			fWatch;
};


class ShelfWindow : public BWindow {
public:
	ShelfWindow()
		:
		BWindow(BRect(0, 0, 100, 100), "Shelf", B_NO_BORDER_WINDOW_LOOK,
			B_FLOATING_ALL_WINDOW_FEEL, kWindowFlags, B_ALL_WORKSPACES)
	{
		fView = new ShelfView(Bounds());
		AddChild(fView);
		_Place();
		BMessage tick(kTick);
		fRunner = new BMessageRunner(BMessenger(this), &tick, 500000);
		fView->Update();
	}

	~ShelfWindow()
	{
		delete fRunner;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kTick:
				if (fWatch.Changed()) {
					_Place();
					fView->Relayout();
					fView->Invalidate();
				}
				fView->Update();
				break;
			case B_COLORS_UPDATED:
			case B_FONTS_UPDATED:
				fView->Invalidate();
				break;
			case B_SCREEN_CHANGED:
				_Place();
				break;
			default:
				BWindow::MessageReceived(message);
		}
	}

private:
	void _Place()
	{
		const BRect screen = BScreen(this).Frame();
		// down to the taskbar's trough, over the end of its band
		MoveTo(screen.right - shelf_width() + 1, screen.top);
		ResizeTo(shelf_width() - 1, screen.Height() - taskbar_height());
	}

	ShelfView*			fView;
	BMessageRunner*		fRunner;
	LookWatch			fWatch;
};


//	#pragma mark - the application


//!	The layout Power Theme chose, in its settings: Photon's unless Leopard's.
shell_layout
current_layout()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kLayoutName) != B_OK)
		return SHELL_LAYOUT_PHOTON;
	BFile file(path.Path(), B_READ_ONLY);
	char text[32] = {};
	if (file.InitCheck() != B_OK || file.Read(text, sizeof(text) - 1) <= 0)
		return SHELL_LAYOUT_PHOTON;
	return strncasecmp(text, "leopard", 7) == 0
		? SHELL_LAYOUT_LEOPARD : SHELL_LAYOUT_PHOTON;
}


/*!	Stops the Deskbar, and keeps launch_daemon from starting it again, while
	this runs; puts it back as this quits.
*/
static void
set_deskbar_running(bool run)
{
	if (run) {
		// launch_daemon's, which it starts again if it ends. Only if that
		// doesn't come is one started here: launched at once, it would race
		// launch_daemon's, which then fails as "already running" and leaves
		// the service unable to start until the next boot, and this one
		// isn't started again when it ends.
		system("launch_roster start x-vnd.Be-TSKB >/dev/null 2>&1");
		for (int32 i = 0; i < 30 && !be_roster->IsRunning(kDeskbarSignature);
				i++)
			snooze(100000);
		if (!be_roster->IsRunning(kDeskbarSignature))
			be_roster->Launch(kDeskbarSignature);
		return;
	}
	system("launch_roster stop x-vnd.Be-TSKB >/dev/null 2>&1");
	BMessenger(kDeskbarSignature).SendMessage(B_QUIT_REQUESTED);
}


class PowerShelf : public BApplication {
public:
	PowerShelf()
		:
		BApplication(kSignature),
		fLayout(SHELL_LAYOUT_PHOTON)
	{
	}

	virtual void ReadyToRun()
	{
		sGuard = new CrashGuard();
		if (sGuard->Mode() == CrashGuard::GIVE_UP) {
			// crashing over and over: the Deskbar back, and no more restarts
			sGuard->Quit();
			set_deskbar_running(true);
			system("launch_roster stop x-vnd.idumych-PowerTheme-shelf "
				">/dev/null 2>&1");
			Quit();
			return;
		}
		set_deskbar_running(false);
		_ShowLayout();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kLayout:
				// Power Theme changed it
				if (current_layout() != fLayout) {
					_CloseWindows();
					_ShowLayout();
				}
				break;
			default:
				BApplication::MessageReceived(message);
		}
	}

	virtual bool QuitRequested()
	{
		// the windows' own QuitRequested() put back what they changed
		_CloseWindows();
		if (sGuard != NULL)
			sGuard->Quit();
		set_deskbar_running(true);
		return true;
	}

private:
	void _ShowLayout()
	{
		fLayout = current_layout();
		if (fLayout == SHELL_LAYOUT_LEOPARD) {
			fWindows.push_back(new_leopard_menu_bar());
			fWindows.push_back(new_leopard_dock());
		} else {
			fWindows.push_back(new TaskbarWindow());
			fWindows.push_back(new ShelfWindow());
		}
		for (BWindow* window : fWindows)
			window->Show();
	}

	void _CloseWindows()
	{
		for (BWindow* window : fWindows) {
			if (window->Lock()) {
				window->QuitRequested();
				window->Quit();
			}
		}
		fWindows.clear();
	}

	shell_layout		fLayout;
	std::vector<BWindow*> fWindows;
};


int
main()
{
	// it outlasts the Terminal or program that started it
	const int null = open("/dev/null", O_RDWR);
	if (null >= 0) {
		dup2(null, STDIN_FILENO);
		dup2(null, STDOUT_FILENO);
		dup2(null, STDERR_FILENO);
		if (null > STDERR_FILENO)
			close(null);
	}

	PowerShelf app;
	app.Run();
	return 0;
}
