/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * PowerShelf's Leopard layout, in the style of Mac OS X 10.5: a menu bar
 * across the top of the screen with the menus of the window in front, and a
 * dock along the bottom, both of glass that the desktop shows through.
 *
 * The menu bar has a system menu, the front application's menu under its
 * name, that application's own menus, the Deskbar's tray items and a clock.
 * Its menus are copied from the front window's menu bar through the scripting
 * every BMenu answers, as they open, and an item chosen runs in the window.
 * So menus an application fills only as they open, and items of its own
 * kinds, may show differently, and shortcuts aren't shown. The window's own
 * menu bar is hidden, unless its views don't move up into its place.
 *
 * The dock has launchers, the applications running that it doesn't have yet,
 * and past a line, a folder and the Trash, standing on a glass shelf in
 * perspective with their reflections; under the pointer they grow, and a dot
 * marks those running.
 *
 * With no compositor both are opaque, and the dock doesn't grow.
 *
 * Drawn from scratch after screenshots of Leopard; nothing of Apple's is used.
 */


#include "Canvas.h"
#include "IconTheme.h"
#include "Shell.h"

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Node.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <View.h>
#include <Window.h>
#include <fs_attr.h>
#include <sys/stat.h>

#include <algorithm>
#include <math.h>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>


static const bigtime_t kTimeout = 250000;
static const char* kDockSettingsName = "PowerTheme/dock";

static const uint32 kCheck = 'chek';
static const uint32 kRun = 'run ';
static const uint32 kCheckGap = 'gap ';
static const uint32 kAbout = 'abut';
static const uint32 kHide = 'hide';
static const uint32 kHideOthers = 'hido';
static const uint32 kShowAll = 'shwa';
static const uint32 kQuit = 'quit';
static const uint32 kForceQuit = 'fqut';
static const uint32 kRestart = 'rbot';
static const uint32 kAnimate = 'anim';
static const uint32 kDockTick = 'dtik';
static const uint32 kKeep = 'keep';
static const uint32 kRemove = 'remv';
static const uint32 kShowWindow = 'shwn';
static const uint32 kShowInTracker = 'shtr';
static const uint32 kMagnify = 'magn';

static const uint32 kWindowFlags = B_NOT_MOVABLE | B_NOT_RESIZABLE
	| B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS
	| B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS;

// the menu bar: a light glass, a little darker along the bottom
static const float kBarHeight = 22;
static const rgb_color kBarTop = { 0xf4, 0xf4, 0xf4, 0xd8 };
static const rgb_color kBarBottom = { 0xd2, 0xd2, 0xd2, 0xd8 };
static const rgb_color kBarShine = { 0xff, 0xff, 0xff, 0xb0 };
static const rgb_color kBarEdge = { 0x50, 0x50, 0x50, 0xc0 };
static const rgb_color kBarText = { 0, 0, 0, 255 };
static const rgb_color kBarDisabledText = { 0, 0, 0, 0x70 };
static const rgb_color kSelectTop = { 0x63, 0x83, 0xf5, 255 };
static const rgb_color kSelectBottom = { 0x24, 0x52, 0xe6, 255 };
static const rgb_color kSelectedText = { 255, 255, 255, 255 };
// when nothing can be seen through it
static const rgb_color kOpaqueTop = { 0xf2, 0xf2, 0xf2, 255 };
static const rgb_color kOpaqueBottom = { 0xd6, 0xd6, 0xd6, 255 };

// the dock: icons, how much they grow, and the shelf they stand on
static const float kDockIcon = 48;
static const float kDockMagnified = 96;
static const float kDockGap = 6;
static const float kLabelHeight = 24;
static const float kBounce = 26;


static rgb_color
bar_top()
{
	return desktop_composited() ? kBarTop : kOpaqueTop;
}


static rgb_color
bar_bottom()
{
	return desktop_composited() ? kBarBottom : kOpaqueBottom;
}


//!	The menu font, a little bigger, as Leopard's menu bar is.
static BFont
bar_font(bool bold = false)
{
	BFont font(bold ? be_bold_font : be_plain_font);
	menu_info info;
	if (get_menu_info(&info) == B_OK) {
		font.SetFamilyAndStyle(info.f_family, info.f_style);
		font.SetSize(std::max(13.0f, info.font_size + 1));
		if (bold)
			font.SetFace(B_BOLD_FACE);
	}
	return font;
}


/*!	Draws the bar's glass for the part of it from \a left to \a left plus the
	canvas's width, so that the views across it join up.
*/
static void
draw_bar_glass(Canvas& canvas)
{
	const float bottom = canvas.Height() - 1;
	canvas.FillGradient(BRect(0, 0, canvas.Width() - 1, bottom - 1),
		bar_top(), bar_bottom());
	canvas.FillRect(BRect(0, 0, canvas.Width() - 1, 0), kBarShine);
	canvas.FillRect(BRect(0, bottom, canvas.Width() - 1, bottom),
		desktop_composited() ? kBarEdge : with_alpha(kBarEdge, 255));
}


//!	\a text centred down the bar, from \a x.
static void
draw_bar_text(Canvas& canvas, const char* text, const BFont& font, float x,
	rgb_color color)
{
	font_height height;
	font.GetHeight(&height);
	const float baseline = floorf((canvas.Height() - 1 + height.ascent
		- height.descent) / 2) + 1;
	canvas.DrawString(text, font, BPoint(x, baseline), color);
}


//	#pragma mark - the front window's menus


//! Where a menu is in another application's window, for its scripting.
struct MenuPath {
	team_id				team;
	BString				window;		// the window's title
	std::vector<int32>	views;		// child indices from the top view down
	std::vector<int32>	menus;		// item indices from the menu bar down
};


/*!	Adds the specifiers that lead to \a path's menu, and to its item \a item
	if that isn't negative. The last one added is resolved first.
*/
static void
add_specifiers(BMessage& message, const MenuPath& path, int32 item = -1)
{
	if (item >= 0)
		message.AddSpecifier("MenuItem", item);
	for (auto i = path.menus.rbegin(); i != path.menus.rend(); i++)
		message.AddSpecifier("Menu", *i);
	for (auto i = path.views.rbegin(); i != path.views.rend(); i++)
		message.AddSpecifier("View", *i);
	message.AddSpecifier("Window", path.window.String());
}


static bool
ask(team_id team, BMessage& message, BMessage& reply)
{
	BMessenger app(NULL, team);
	if (app.SendMessage(&message, &reply, kTimeout, kTimeout) != B_OK
		|| reply.what == B_MESSAGE_NOT_UNDERSTOOD)
		return false;
	int32 error;
	return reply.FindInt32("error", &error) != B_OK || error == B_OK;
}


//! The number of items in \a path's menu, or -1 if it isn't one.
static int32
count_items(const MenuPath& path)
{
	BMessage message(B_COUNT_PROPERTIES), reply;
	message.AddSpecifier("MenuItem");
	add_specifiers(message, path);
	int32 count;
	if (!ask(path.team, message, reply)
		|| reply.FindInt32("result", &count) != B_OK)
		return -1;
	return count;
}


static bool
item_property(const MenuPath& path, int32 item, const char* property,
	BMessage& reply)
{
	BMessage message(B_GET_PROPERTY);
	message.AddSpecifier(property);
	add_specifiers(message, path, item);
	return ask(path.team, message, reply);
}


static void
set_hidden(team_id team, const BString& window,
	const std::vector<int32>& views, bool hidden)
{
	BMessage message(B_SET_PROPERTY), reply;
	message.AddBool("data", hidden);
	message.AddSpecifier("Hidden");
	MenuPath path = { team, window, views, {} };
	add_specifiers(message, path);
	ask(team, message, reply);
}


static bool
view_frame(team_id team, const BString& window,
	const std::vector<int32>& views, BRect& frame)
{
	BMessage message(B_GET_PROPERTY), reply;
	message.AddSpecifier("Frame");
	MenuPath path = { team, window, views, {} };
	add_specifiers(message, path);
	return ask(team, message, reply)
		&& reply.FindRect("result", &frame) == B_OK;
}


/*!	Finds the menu bar in \a window of \a team: the first view, nearest the
	top, that answers as a menu. A pop-up menu's bar is left alone.
*/
static bool
find_menu_bar(team_id team, const char* window, std::vector<int32>& views)
{
	std::vector<std::vector<int32>> queue = { {} };
	int32 asked = 0;
	for (size_t next = 0; next < queue.size() && asked < 64; next++) {
		MenuPath parent = { team, window, queue[next], {} };
		BMessage message(B_COUNT_PROPERTIES), reply;
		message.AddSpecifier("View");
		add_specifiers(message, parent);
		int32 count;
		asked++;
		if (!ask(team, message, reply)
			|| reply.FindInt32("result", &count) != B_OK)
			continue;
		for (int32 i = 0; i < count && asked < 64; i++) {
			MenuPath child = parent;
			child.views.push_back(i);
			BMessage suites(B_GET_SUPPORTED_SUITES), answer;
			add_specifiers(suites, child);
			asked++;
			if (!ask(team, suites, answer))
				continue;
			bool menu = false;
			const char* suite;
			for (int32 s = 0; answer.FindString("suites", s, &suite) == B_OK;
					s++) {
				if (strcmp(suite, "suite/vnd.Be-menu") == 0)
					menu = true;
			}
			if (menu) {
				BMessage name(B_GET_PROPERTY), named;
				name.AddSpecifier("InternalName");
				add_specifiers(name, child);
				const char* internal;
				if (ask(team, name, named)
					&& named.FindString("result", &internal) == B_OK
					&& strcmp(internal, "_mc_mb_") == 0)
					continue;
				views = child.views;
				return true;
			}
			if (child.views.size() < 3)
				queue.push_back(child.views);
		}
	}
	return false;
}


static BMessage*
run_message(const MenuPath& path, int32 item)
{
	BMessage* message = new BMessage(kRun);
	message->AddInt32("team", path.team);
	message->AddString("window", path.window);
	for (int32 view : path.views)
		message->AddInt32("view", view);
	for (int32 menu : path.menus)
		message->AddInt32("menu", menu);
	message->AddInt32("item", item);
	return message;
}


/*!	A copy of a menu in another application, filled each time it opens, so
	that it shows the items as they are then.
*/
class MirrorMenu : public BMenu {
public:
	MirrorMenu(const char* label, const MenuPath& path, BHandler* target)
		:
		BMenu(label),
		fPath(path),
		fTarget(target)
	{
	}

protected:
	virtual bool AddDynamicItem(add_state state)
	{
		if (state != B_INITIAL_ADD)
			return false;
		RemoveItems(0, CountItems(), true);
		const int32 count = count_items(fPath);
		for (int32 i = 0; i < count; i++) {
			BMessage reply;
			const char* label = NULL;
			if (item_property(fPath, i, "Label", reply))
				reply.FindString("result", &label);
			if (label == NULL || label[0] == '\0') {
				AddSeparatorItem();
				continue;
			}

			MenuPath sub = fPath;
			sub.menus.push_back(i);
			BMenuItem* item;
			if (count_items(sub) >= 0)
				item = new BMenuItem(new MirrorMenu(label, sub, fTarget));
			else {
				item = new BMenuItem(label, run_message(fPath, i));
				item->SetTarget(fTarget);
			}

			bool value;
			BMessage answer;
			if (item_property(fPath, i, "Enabled", answer)
				&& answer.FindBool("result", &value) == B_OK)
				item->SetEnabled(value);
			answer.MakeEmpty();
			if (item_property(fPath, i, "Mark", answer)
				&& answer.FindBool("result", &value) == B_OK)
				item->SetMarked(value);
			AddItem(item);
		}
		return false;
	}

private:
	MenuPath			fPath;
	BHandler*			fTarget;
};


//	#pragma mark - the menu bar


//!	A title in the bar: the system menu's leaf, the front application's name
//!	in bold, or one of its menus.
class BarItem : public BMenuItem {
public:
	enum kind { LEAF, APP_NAME, TITLE };

	BarItem(BMenu* menu, kind kind = TITLE)
		:
		BMenuItem(menu),
		fKind(kind)
	{
	}

	BarItem(const char* label, BMessage* message)
		:
		BMenuItem(label, message),
		fKind(TITLE)
	{
	}

	kind Kind() const
	{
		return fKind;
	}

	bool Selected() const
	{
		return IsSelected();
	}

	float ContentWidth() const
	{
		if (fKind == LEAF)
			return 14;
		return ceilf(bar_font(fKind == APP_NAME).StringWidth(Label()));
	}

protected:
	virtual void GetContentSize(float* width, float* height)
	{
		BMenuItem::GetContentSize(width, height);
		*width = fKind == LEAF ? 16 : ContentWidth();
	}

private:
	kind				fKind;
};


/*!	The bar's menus: BMenuBar lays them out and tracks them, and this draws
	them on the glass.
*/
class GlassMenuBar : public BMenuBar {
public:
	GlassMenuBar(BRect frame)
		:
		BMenuBar(frame, "menu bar", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
			B_ITEMS_IN_ROW, false)
	{
	}

	virtual void AttachedToWindow()
	{
		BMenuBar::AttachedToWindow();
		// it draws every pixel itself: not cleared first, so it doesn't
		// flash clear while it draws
		SetViewColor(B_TRANSPARENT_COLOR);
		SetFlags(Flags() | B_TRANSPARENT_BACKGROUND);
	}

	virtual void Draw(BRect update)
	{
		// lays the items out, drawn over below
		BMenuBar::Draw(update);

		const BRect bounds = Bounds();
		if (!fCanvas.SetSize(bounds.IntegerWidth() + 1, kBarHeight))
			return;
		draw_bar_glass(fCanvas);
		for (int32 i = 0; i < CountItems(); i++) {
			BarItem* item = dynamic_cast<BarItem*>(ItemAt(i));
			if (item == NULL)
				continue;
			const BRect frame = item->Frame();
			const bool selected = item->Selected();
			if (selected) {
				fCanvas.FillGradient(BRect(frame.left, 0, frame.right,
					kBarHeight - 2), kSelectTop, kSelectBottom);
			}
			const rgb_color color = selected ? kSelectedText
				: item->IsEnabled() ? kBarText : kBarDisabledText;
			const float x = frame.left + floorf((frame.Width() + 1
				- item->ContentWidth()) / 2);
			if (item->Kind() == BarItem::LEAF)
				_DrawLeaf(BPoint(x, 3), color);
			else {
				draw_bar_text(fCanvas, item->Label(),
					bar_font(item->Kind() == BarItem::APP_NAME), x, color);
			}
		}
		fCanvas.CopyTo(this, BPoint(0, 0));
	}

private:
	//!	A leaf on a stem, leaning right, its top left corner at \a where.
	void _DrawLeaf(BPoint where, rgb_color color)
	{
		const float cx = where.x + 7;
		const float top = where.y + 1;
		const float height = 14;
		for (int32 row = 0; row < height * 4; row++) {
			// four rows a pixel, for smooth edges
			const float y = top + row / 4.0f;
			const float t = row / (height * 4);
			// widest a little below the middle, leaning right as it rises
			const float half = 5.6f * sinf(t * M_PI) * (0.75f + 0.25f * t);
			const float centre = cx + (1 - t) * 1.5f;
			for (int32 x = (int32)floorf(centre - half);
					x <= (int32)ceilf(centre + half); x++) {
				float cover = std::min(1.0f, std::min(x + 1 - (centre - half),
					centre + half - x));
				// a vein down the middle, left clear
				if (fabsf(x + 0.5f - centre) < 0.5f && t > 0.2f && t < 0.85f)
					cover *= 0.25f;
				if (cover > 0)
					fCanvas.Blend(x, (int32)y, color,
						(uint8)(cover * 255 / 4));
			}
		}
		// the stem
		for (int32 i = 0; i < 3; i++)
			fCanvas.Blend((int32)cx - 1 - i / 2, (int32)(top + height) + i,
				color);
	}

	Canvas				fCanvas;
};


/*!	The right end of the bar: the Deskbar's tray items and a clock. The tray
	items are drawn over its glass.
*/
class StatusView : public BView {
public:
	StatusView(BRect frame)
		:
		BView(frame, "status", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
			B_WILL_DRAW | B_FRAME_EVENTS | B_TRANSPARENT_BACKGROUND)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		fTray = new BView(BRect(0, 0, 10, 10), "tray", B_FOLLOW_NONE,
			B_TRANSPARENT_BACKGROUND);
		fTray->SetViewColor(B_TRANSPARENT_COLOR);
		fTray->SetLowColor(kOpaqueBottom);
	}

	virtual void AttachedToWindow()
	{
		AddChild(fTray);
		load_replicants(fTray, 200);
		// those that leave their background to their parent show the glass
		for (int32 i = 0; i < fTray->CountChildren(); i++) {
			BView* child = fTray->ChildAt(i);
			if (child->ViewColor() == B_TRANSPARENT_COLOR)
				child->SetFlags(child->Flags() | B_TRANSPARENT_BACKGROUND);
		}
	}

	//!	How wide it wants to be.
	float PreferredWidth()
	{
		return _TrayWidth() + _ClockWidth() + 12;
	}

	void Arrange()
	{
		const BRect bounds = Bounds();
		float x = 6;
		for (int32 i = 0; i < fTray->CountChildren(); i++) {
			BView* child = fTray->ChildAt(i);
			const float height = child->Bounds().Height() + 1;
			child->MoveTo(x - 6, 0);
			x += child->Bounds().Width() + 1 + 6;
			fTray->ResizeTo(std::max(0.0f, x - 12),
				std::max(fTray->Bounds().Height(), height - 1));
		}
		fTray->MoveTo(6, floorf((bounds.Height() - fTray->Bounds().Height())
			/ 2));
	}

	virtual void Draw(BRect update)
	{
		if (_Render())
			fCanvas.CopyTo(this, BPoint(0, 0));
	}

	/*!	Redraws the clock straight into the window, outside an update, so
		that it doesn't flash clear; the tray items are left as they are.
	*/
	void Tick()
	{
		if (Window() == NULL || Window()->IsHidden() || !_Render())
			return;
		BRegion clock(Bounds());
		clock.Exclude(fTray->Frame());
		ConstrainClippingRegion(&clock);
		fCanvas.CopyTo(this, BPoint(0, 0));
		ConstrainClippingRegion(NULL);
		Flush();
	}

private:
	bool _Render()
	{
		const BRect bounds = Bounds();
		if (!fCanvas.SetSize(bounds.IntegerWidth() + 1, kBarHeight))
			return false;
		draw_bar_glass(fCanvas);
		char text[64];
		const time_t now = time(NULL);
		strftime(text, sizeof(text), "%a %l:%M %p", localtime(&now));
		BString clock(text);
		clock.ReplaceAll("  ", " ");
		const BFont font = bar_font();
		draw_bar_text(fCanvas, clock.String(), font,
			bounds.Width() - ceilf(font.StringWidth(clock.String())) - 10,
			kBarText);
		return true;
	}

	float _TrayWidth()
	{
		float width = 0;
		for (int32 i = 0; i < fTray->CountChildren(); i++)
			width += fTray->ChildAt(i)->Bounds().Width() + 1 + 6;
		return width;
	}

	float _ClockWidth()
	{
		return ceilf(bar_font().StringWidth("Wed 12:00 PM")) + 14;
	}

	BView*				fTray;
	Canvas				fCanvas;
};


class MenuBarWindow : public BWindow {
public:
	MenuBarWindow()
		:
		BWindow(BRect(0, 0, 100, kBarHeight - 1), "Menu bar",
			B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
			kWindowFlags | (desktop_composited() ? kTransparentWindow : 0),
			B_ALL_WORKSPACES),
		fTeam(-1),
		fToken(-1),
		fTicks(0),
		fMenusOpen(false)
	{
		fBar = new GlassMenuBar(Bounds());
		AddChild(fBar);
		fStatus = new StatusView(Bounds());
		AddChild(fStatus);
		_Place();
		_Rebuild();
		BMessage check(kCheck);
		fRunner = new BMessageRunner(BMessenger(this), &check, 250000);
	}

	virtual ~MenuBarWindow()
	{
		delete fRunner;
	}

	virtual bool QuitRequested()
	{
		_Restore();
		return true;
	}

	virtual void MenusBeginning()
	{
		fMenusOpen = true;
	}

	virtual void MenusEnded()
	{
		fMenusOpen = false;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kCheck:
				if (++fTicks % 4 == 0) {
					fStatus->Tick();
					_KeepWindowsClear();
				}
				if (!fMenusOpen)
					_Check();
				break;
			case kRun:
			{
				MenuPath path;
				path.team = message->GetInt32("team", -1);
				path.window = message->GetString("window", "");
				int32 value;
				for (int32 i = 0; message->FindInt32("view", i, &value)
						== B_OK; i++)
					path.views.push_back(value);
				for (int32 i = 0; message->FindInt32("menu", i, &value)
						== B_OK; i++)
					path.menus.push_back(value);
				BMessage run(B_EXECUTE_PROPERTY);
				add_specifiers(run, path, message->GetInt32("item", 0));
				BMessenger(NULL, path.team).SendMessage(&run);
				break;
			}
			case kCheckGap:
				_CheckGap(message);
				break;
			case kLaunch:
			{
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK)
					launch(ref);
				break;
			}
			case kAbout:
				BMessenger(NULL, fTeam).SendMessage(B_ABOUT_REQUESTED);
				break;
			case kHide:
				if (fTeam >= 0)
					do_minimize_team(BRect(), fTeam, false);
				break;
			case kHideOthers:
			case kShowAll:
			{
				BList teams;
				be_roster->GetAppList(&teams);
				for (int32 i = 0; i < teams.CountItems(); i++) {
					const team_id team = (team_id)(addr_t)teams.ItemAt(i);
					if (team == be_app->Team())
						continue;
					if (message->what == kShowAll)
						do_bring_to_front_team(BRect(), team, false);
					else if (team != fTeam)
						do_minimize_team(BRect(), team, false);
				}
				break;
			}
			case kQuit:
				BMessenger(NULL, fTeam).SendMessage(B_QUIT_REQUESTED);
				break;
			case kForceQuit:
				_ForceQuit();
				break;
			case kRestart:
			{
				BAlert* alert = new BAlert("Restart",
					"Restart the computer now?", "Cancel", "Restart", NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				if (alert->Go() == 1)
					system("shutdown -r &");
				break;
			}
			case kShutDown:
				shut_down();
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
		MoveTo(screen.left, screen.top);
		ResizeTo(screen.Width(), kBarHeight - 1);
		_Arrange();
	}

	//!	The menus from the left, and the status items from the right.
	void _Arrange()
	{
		const BRect bounds = Bounds();
		const float status = std::min(bounds.Width() / 2,
			fStatus->PreferredWidth());
		fStatus->MoveTo(bounds.right - status + 1, 0);
		fStatus->ResizeTo(status - 1, bounds.Height());
		fBar->MoveTo(0, 0);
		fBar->ResizeTo(bounds.Width() - status, bounds.Height());
		fStatus->Arrange();
	}

	void _KeepWindowsClear()
	{
		const BRect screen = BScreen(this).Frame();
		const BRect room(screen.left, screen.top + kBarHeight, screen.right,
			screen.bottom - (kDockIcon + kDockIcon * 0.3f + 3));
		app_info appInfo;
		BList teams;
		be_roster->GetAppList(&teams);
		for (int32 t = 0; t < teams.CountItems(); t++) {
			const team_id team = (team_id)(addr_t)teams.ItemAt(t);
			if (team == be_app->Team())
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
					&& info->name[0] != '\0')
					keep_clear(info, room, screen, fFrames);
				free(info);
			}
			free(tokens);
		}
	}

	bool _IsDeskbar(team_id team)
	{
		app_info info;
		return be_roster->GetRunningAppInfo(team, &info) == B_OK
			&& strcasecmp(info.signature, kDeskbarSignature) == 0;
	}

	//! Follows the window in front.
	void _Check()
	{
		team_id team = -1;
		int32 token = -1;
		BString name;
		int32* tokens = NULL;
		int32 count = 0;
		if (BPrivate::get_window_order(current_workspace(), &tokens, &count)
				!= B_OK)
			return;
		for (int32 i = 0; i < count && team < 0; i++) {
			client_window_info* info = get_window_info(tokens[i]);
			if (info == NULL)
				continue;
			// a normal window shown in this workspace, not ours or the
			// Deskbar's
			if (info->team != be_app->Team() && !info->is_mini
				&& info->show_hide_level <= 0
				&& info->feel == B_NORMAL_WINDOW_FEEL
				&& !_IsDeskbar(info->team)) {
				team = info->team;
				token = info->server_token;
				name = info->name;
			}
			free(info);
		}
		free(tokens);

		if (team == fTeam && token == fToken && name == fName)
			return;
		fTeam = team;
		fToken = token;
		fName = name;
		_Rebuild();
	}

	BMenu* _SystemMenu()
	{
		BMenu* menu = new BMenu("System");
		entry_ref ref;
		if (find_program("AboutSystem", ref)) {
			BMessage* message = new BMessage(kLaunch);
			message->AddRef("refs", &ref);
			menu->AddItem(new BMenuItem("About this computer", message));
		}
		if (find_program("SoftwareUpdater", ref)) {
			BMessage* message = new BMessage(kLaunch);
			message->AddRef("refs", &ref);
			menu->AddItem(new BMenuItem("Software update" B_UTF8_ELLIPSIS,
				message));
		}
		menu->AddSeparatorItem();
		BMenu* applications = new BMenu("Applications");
		fill_menu(applications, menu_folders(), this);
		menu->AddItem(applications);
		if (find_program("PowerTheme", ref)) {
			BMessage* message = new BMessage(kLaunch);
			message->AddRef("refs", &ref);
			menu->AddItem(new BMenuItem("Themes", message));
		}
		menu->AddSeparatorItem();
		if (fTeam >= 0) {
			BString label;
			label.SetToFormat("Force quit %s" B_UTF8_ELLIPSIS,
				_AppName().String());
			menu->AddItem(new BMenuItem(label, new BMessage(kForceQuit)));
			menu->AddSeparatorItem();
		}
		menu->AddItem(new BMenuItem("Restart" B_UTF8_ELLIPSIS,
			new BMessage(kRestart)));
		menu->AddItem(new BMenuItem("Shut down" B_UTF8_ELLIPSIS,
			new BMessage(kShutDown)));
		menu->SetTargetForItems(this);
		return menu;
	}

	BString _AppName()
	{
		app_info info;
		if (fTeam >= 0 && be_roster->GetRunningAppInfo(fTeam, &info) == B_OK)
			return info.ref.name;
		return "Tracker";
	}

	void _ForceQuit()
	{
		if (fTeam < 0)
			return;
		BString text;
		text.SetToFormat("Force %s to quit? Anything it hasn't saved is lost.",
			_AppName().String());
		BAlert* alert = new BAlert("Force quit", text, "Cancel",
			"Force quit", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		if (alert->Go() == 1)
			kill_team(fTeam);
	}

	void _Rebuild()
	{
		fBar->RemoveItems(0, fBar->CountItems(), true);
		fBar->AddItem(new BarItem(_SystemMenu(), BarItem::LEAF));

		// the application's own menu, under its name
		const BString appName = _AppName();
		BMenu* appMenu = new BMenu(appName);
		BString label;
		if (fTeam >= 0) {
			label.SetToFormat("About %s", appName.String());
			appMenu->AddItem(new BMenuItem(label, new BMessage(kAbout)));
			appMenu->AddSeparatorItem();
			label.SetToFormat("Hide %s", appName.String());
			appMenu->AddItem(new BMenuItem(label, new BMessage(kHide)));
		}
		appMenu->AddItem(new BMenuItem("Hide others",
			new BMessage(kHideOthers)));
		appMenu->AddItem(new BMenuItem("Show all", new BMessage(kShowAll)));
		if (fTeam >= 0) {
			appMenu->AddSeparatorItem();
			label.SetToFormat("Quit %s", appName.String());
			appMenu->AddItem(new BMenuItem(label, new BMessage(kQuit)));
		}
		appMenu->SetTargetForItems(this);
		fBar->AddItem(new BarItem(appMenu, BarItem::APP_NAME));
		_AddWindowMenus();
		fBar->Invalidate();
	}

	void _AddWindowMenus()
	{
		if (fTeam < 0)
			return;
		const std::pair<team_id, int32> key(fTeam, fToken);
		auto found = fBars.find(key);
		if (found == fBars.end()) {
			std::vector<int32> views;
			if (!find_menu_bar(fTeam, fName, views))
				return;
			found = fBars.insert(std::make_pair(key, views)).first;
		}
		if (fHidden.find(key) == fHidden.end()
			&& fKept.find(key) == fKept.end()) {
			// what is below it now, to see whether it moves up
			std::vector<int32> below = found->second;
			below.back()++;
			BRect bar, under;
			if (view_frame(fTeam, fName, found->second, bar)
				&& view_frame(fTeam, fName, below, under)
				&& under.top > bar.top) {
				set_hidden(fTeam, fName, found->second, true);
				fHidden[key] = found->second;
				BMessage check(kCheckGap);
				check.AddInt32("team", fTeam);
				check.AddInt32("token", fToken);
				check.AddFloat("top", under.top);
				BMessageRunner::StartSending(BMessenger(this), &check, 300000,
					1);
			} else
				fKept.insert(key);
		}

		MenuPath path = { fTeam, fName, found->second, {} };
		const int32 count = count_items(path);
		if (count < 0) {
			// gone, or the window moved its views: look again next time
			fBars.erase(found);
			return;
		}
		for (int32 i = 0; i < count; i++) {
			BMessage reply;
			const char* text = NULL;
			if (!item_property(path, i, "Label", reply)
				|| reply.FindString("result", &text) != B_OK
				|| text == NULL || text[0] == '\0')
				continue;
			MenuPath sub = path;
			sub.menus.push_back(i);
			if (count_items(sub) >= 0)
				fBar->AddItem(new BarItem(new MirrorMenu(text, sub, this)));
			else {
				BMenuItem* item = new BarItem(text, run_message(path, i));
				item->SetTarget(this);
				fBar->AddItem(item);
			}
		}
	}

	/*!	A window laid out by hand leaves a gap where its menu bar was: it
		keeps its menu bar then.
	*/
	void _CheckGap(BMessage* message)
	{
		const std::pair<team_id, int32> key(message->GetInt32("team", -1),
			message->GetInt32("token", -1));
		auto hidden = fHidden.find(key);
		client_window_info* info = get_window_info(key.second);
		if (hidden == fHidden.end() || info == NULL) {
			free(info);
			return;
		}
		const BString window = info->name;
		free(info);
		std::vector<int32> below = hidden->second;
		below.back()++;
		BRect under;
		if (view_frame(key.first, window, below, under)
			&& under.top >= message->GetFloat("top", 0)) {
			set_hidden(key.first, window, hidden->second, false);
			fHidden.erase(hidden);
			fKept.insert(key);
		}
	}

	//! Puts back every menu bar this hid.
	void _Restore()
	{
		for (const auto& hidden : fHidden) {
			const team_id team = hidden.first.first;
			const int32 token = hidden.first.second;
			client_window_info* info = get_window_info(token);
			if (info == NULL)
				continue;
			if (info->team == team)
				set_hidden(team, info->name, hidden.second, false);
			free(info);
		}
		fHidden.clear();
	}

	GlassMenuBar*		fBar;
	StatusView*			fStatus;
	BMessageRunner*		fRunner;
	team_id				fTeam;
	int32				fToken;
	BString				fName;
	int32				fTicks;
	bool				fMenusOpen;
	// each window's frame when last seen with no button held
	std::map<int32, BRect> fFrames;
	// each window's menu bar, and those this hid
	std::map<std::pair<team_id, int32>, std::vector<int32>> fBars;
	std::map<std::pair<team_id, int32>, std::vector<int32>> fHidden;
	// those that keep theirs, as their views don't move up without it
	std::set<std::pair<team_id, int32>> fKept;
};


//	#pragma mark - the dock


enum dock_kind {
	DOCK_APP,
	DOCK_FOLDER,
	DOCK_TRASH,
	DOCK_SEPARATOR
};

struct DockItem {
	dock_kind	kind;
	entry_ref	ref;
	BString		signature;
	BString		label;
	bool		kept;		// a launcher, rather than only while it runs
	team_id		team;		// running, or -1
	BBitmap*	icon;		// at the size it rests at
	BBitmap*	big;		// at the size it grows to
	bigtime_t	bounceStart;
	// laid out
	float		size;		// as drawn now, easing towards target
	float		target;
	float		x;
};


static BString
app_signature(const entry_ref& ref)
{
	BFile file(&ref, B_READ_ONLY);
	BAppFileInfo info(&file);
	char signature[B_MIME_TYPE_LENGTH];
	if (info.InitCheck() == B_OK && info.GetSignature(signature) == B_OK)
		return signature;
	return "";
}


void data_directories(BStringList& directories);


/*!	What an icon theme calls the icons of applications, as Power Theme's icon
	map lists them: "app <signature> = <icon name> ...". The first a theme
	has is used.
*/
static std::map<BString, BStringList>
app_icon_names()
{
	std::map<BString, BStringList> names;
	BStringList folders;
	data_directories(folders);
	for (int32 i = 0; i < folders.CountStrings(); i++) {
		BPath path(folders.StringAt(i), "PowerTheme/icon-map");
		BFile file(path.Path(), B_READ_ONLY);
		off_t size;
		if (file.InitCheck() != B_OK || file.GetSize(&size) != B_OK
			|| size <= 0 || size > 1024 * 1024)
			continue;
		BString text;
		const ssize_t read = file.Read(text.LockBuffer(size), size);
		text.UnlockBuffer(read > 0 ? read : 0);
		BStringList lines;
		text.Split("\n", true, lines);
		for (int32 l = 0; l < lines.CountStrings(); l++) {
			BString line = lines.StringAt(l);
			line.Trim();
			const int32 equals = line.FindFirst('=');
			if (!line.StartsWith("app ") || equals < 0)
				continue;
			BString signature, list;
			line.CopyInto(signature, 4, equals - 4);
			line.CopyInto(list, equals + 1, line.Length() - equals - 1);
			signature.Trim();
			signature.ToLower();
			BStringList icons;
			list.Split(" ", true, icons);
			icons.Remove("");
			if (!signature.IsEmpty() && !icons.IsEmpty())
				names[signature] = icons;
		}
		break;
	}
	return names;
}


/*!	The icon theme Power Theme has in use, from its settings, and when they
	last changed.
*/
static BString
current_icon_theme(time_t& changed)
{
	changed = 0;
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("PowerTheme/current") != B_OK)
		return "";
	struct stat st;
	if (stat(path.Path(), &st) == 0)
		changed = st.st_mtime;
	BFile file(path.Path(), B_READ_ONLY);
	BMessage current;
	if (file.InitCheck() != B_OK || current.Unflatten(&file) != B_OK)
		return "";
	return current.GetString("icons", "");
}


class DockView : public BView {
public:
	DockView(BRect frame)
		:
		BView(frame, "dock", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
			| B_TRANSPARENT_BACKGROUND),
		fMouseX(-1),
		fHovered(false),
		fPressed(-1),
		fMagnify(true),
		fIconTheme(NULL),
		fIconsChanged(0),
		fReloadAt(0),
		fTrashFull(false),
		fAnimating(false),
		fFrameSem(create_sem(0, "dock frames")),
		fTicker(-1),
		fTicking(0),
		fFramePending(0),
		fPointer(-1),
		fLastStep(0),
		fUpdatePending(false)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		fTrashFull = _TrashFull();
		fAppIconNames = app_icon_names();
		_WatchIconTheme();
		_Load();
	}

	~DockView()
	{
		// the frame thread goes when its semaphore does
		atomic_set(&fTicking, 0);
		delete_sem(fFrameSem);
		if (fTicker >= 0) {
			status_t result;
			wait_for_thread(fTicker, &result);
		}
		delete fIconTheme;
		for (DockItem& item : fItems) {
			delete item.icon;
			delete item.big;
		}
	}

	//!	How tall the window is: room to grow and for a label, if it can.
	static float WindowHeight()
	{
		return desktop_composited()
			? kDockMagnified + kLabelHeight + 8 + _ShelfDepth() + 4
			: kDockIcon + _ShelfDepth() + 8;
	}

	virtual void AttachedToWindow()
	{
		_Layout();
		fMessenger = BMessenger(this);
		if (fTicker < 0) {
			fTicker = spawn_thread(&_TickerEntry, "dock frames",
				B_DISPLAY_PRIORITY, this);
			if (fTicker >= 0)
				resume_thread(fTicker);
		}
	}

	virtual void Draw(BRect update)
	{
		if (_Render())
			fCanvas.CopyTo(this, B_ORIGIN);
	}

	/*!	Draws a new frame straight into the window, outside an update: an
		update starts by clearing the view, and the compositor can show the
		window between that and the picture, so the dock would flash. Hidden,
		it waits for its first update.
	*/
	void Redraw()
	{
		if (Window() == NULL || Window()->IsHidden()) {
			Invalidate();
			return;
		}
		if (!_Render())
			return;
		fCanvas.CopyTo(this, B_ORIGIN);
		Flush();
	}

private:
	//!	Draws the dock into the canvas.
	bool _Render()
	{
		const BRect bounds = Bounds();
		if (!fCanvas.SetSize(bounds.IntegerWidth() + 1,
				bounds.IntegerHeight() + 1))
			return false;
		_Place();
		if (!desktop_composited()) {
			fCanvas.FillRect(fCanvas.Bitmap()->Bounds(),
				BScreen(Window()).DesktopColor());
		}
		const float front = bounds.bottom - 3;
		const float back = front - _ShelfDepth();
		const float left = _Left() - kDockGap * 2;
		const float right = _Right() + kDockGap * 2;
		const float slant = _ShelfDepth() * 0.9f;

		// while the pointer is over the dock, all of it can be pointed at,
		// so that it doesn't shrink as the pointer crosses a clear pixel
		if (desktop_composited()) {
			const float top = fHovered ? back - kDockMagnified - kBounce
				: back - kDockIcon * 0.6f;
			fCanvas.FillRect(BRect(left, top, right, bounds.bottom),
				make_rgba(0, 0, 0, 1));
		}

		// the shelf: a glass floor, its front edge lit, then its thickness
		fCanvas.FillTrapezoid(back, front, left + slant, right - slant, left,
			right, make_rgba(0xe8, 0xec, 0xf2, 0x70),
			make_rgba(0xf4, 0xf6, 0xfa, 0xa8));
		fCanvas.FillTrapezoid(back, back + 1, left + slant, right - slant,
			left + slant - 1, right - slant + 1,
			make_rgba(0xff, 0xff, 0xff, 0x90), make_rgba(0xff, 0xff, 0xff,
				0x90));
		fCanvas.FillRect(BRect(left, front, right, front),
			make_rgba(0xff, 0xff, 0xff, 0xe0));
		fCanvas.FillGradient(BRect(left, front + 1, right, bounds.bottom),
			make_rgba(0x9a, 0xa0, 0xaa, 0xd0), make_rgba(0x50, 0x54, 0x5c,
				0xd0));

		const float base = back + _ShelfDepth() * 0.4f;
		const bigtime_t now = system_time();
		for (size_t i = 0; i < fItems.size(); i++) {
			const DockItem& item = fItems[i];
			if (item.kind == DOCK_SEPARATOR) {
				_DrawSeparator(item.x, back, front);
				continue;
			}
			// where it is between pixels too, so that it glides
			const float lift = _Bounce(item, now);
			const float left = item.x - item.size / 2;
			const float top = base - item.size - lift;
			// the icon made for its size at rest, else the big one scaled
			const BBitmap* icon = item.size <= kDockIcon || item.big == NULL
				? item.icon : item.big;
			// the reflection, on the floor below
			fCanvas.DrawSmooth(icon, left, base + lift, item.size, 0x50, true,
				std::min(item.size, front - base), back, front - 1);
			fCanvas.DrawSmooth(icon, left, top, item.size,
				(int32)i == fPressed ? 0xa0 : 255);
			if (item.team >= 0) {
				// a light under it on the floor
				const BPoint light(item.x, front - 4);
				fCanvas.FillCircle(light, 6, make_rgba(0x9c, 0xd4, 0xff, 0xc0),
					make_rgba(0x40, 0x9c, 0xff, 0x00));
				fCanvas.FillCircle(light, 2, make_rgba(0xff, 0xff, 0xff),
					make_rgba(0xe0, 0xf2, 0xff));
			}
		}

		// the name of the one under the pointer, above it
		const int32 hovered = _ItemAt(fMouseX);
		if (fHovered && hovered >= 0 && desktop_composited()
			&& fItems[hovered].kind != DOCK_SEPARATOR) {
			const DockItem& item = fItems[hovered];
			BFont font(be_bold_font);
			font.SetSize(std::max(12.0f, be_plain_font->Size()));
			const float width = ceilf(font.StringWidth(item.label.String()));
			const float top = base - item.size - kLabelHeight - 4
				- _Bounce(item, now);
			BRect pill(item.x - width / 2 - 10, top, item.x + width / 2 + 10,
				top + kLabelHeight - 4);
			pill.OffsetBy(std::max(0.0f, bounds.left + 2 - pill.left), 0);
			pill.OffsetBy(std::min(0.0f, bounds.right - 2 - pill.right), 0);
			fCanvas.FillRoundRect(pill, pill.Height() / 2,
				make_rgba(0x10, 0x10, 0x10, 0xb8));
			font_height height;
			font.GetHeight(&height);
			fCanvas.DrawString(item.label.String(), font,
				BPoint(pill.left + 10, floorf((pill.top + pill.bottom + 1
					+ height.ascent - height.descent) / 2)),
				make_rgba(255, 255, 255));
		}
		return true;
	}

public:
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW) {
			if (fHovered) {
				fHovered = false;
				fMouseX = -1;
				_Animate();
			}
			return;
		}
		// it starts to grow only from where its icons rest
		if (!fHovered && where.y < Bounds().bottom - 3 - _ShelfDepth()
				- kDockIcon * 0.6f)
			return;
		fHovered = true;
		fMouseX = where.x;
		_Animate();
	}

	virtual void MouseDown(BPoint where)
	{
		const int32 index = _ItemAt(where.x);
		if (index < 0 || fItems[index].kind == DOCK_SEPARATOR)
			return;
		int32 buttons = 0;
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0
			|| (modifiers() & B_CONTROL_KEY) != 0) {
			_ShowMenu(index, ConvertToScreen(where));
			return;
		}
		fPressed = index;
		Redraw();
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}

	virtual void MouseUp(BPoint where)
	{
		const int32 pressed = fPressed;
		fPressed = -1;
		Redraw();
		if (pressed < 0 || pressed != _ItemAt(where.x))
			return;
		_Open(fItems[pressed]);
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->WasDropped()) {
			_Dropped(message);
			return;
		}
		switch (message->what) {
			case kAnimate:
				_Step();
				break;
			case kKeep:
			case kRemove:
			{
				const int32 index = message->GetInt32("index", -1);
				if (index < 0 || index >= (int32)fItems.size())
					break;
				fItems[index].kept = message->what == kKeep;
				if (!fItems[index].kept && fItems[index].team < 0) {
					delete fItems[index].icon;
					delete fItems[index].big;
					fItems.erase(fItems.begin() + index);
				}
				_Save();
				Update();
				break;
			}
			case kShowWindow:
				do_window_action(message->GetInt32("token", -1),
					B_BRING_TO_FRONT, BRect(), false);
				break;
			case kShowInTracker:
			{
				entry_ref ref;
				BEntry entry;
				entry_ref parent;
				if (message->FindRef("refs", &ref) == B_OK
					&& BEntry(&ref).GetParent(&entry) == B_OK
					&& entry.GetRef(&parent) == B_OK)
					launch(parent);
				break;
			}
			case kQuit:
				BMessenger(NULL, message->GetInt32("team", -1))
					.SendMessage(B_QUIT_REQUESTED);
				break;
			case kMagnify:
				fMagnify = !fMagnify;
				_Save();
				Redraw();
				break;
			default:
				BView::MessageReceived(message);
		}
	}

	//!	Follows the applications running.
	void Update()
	{
		// asking the roster takes a while: not while it moves
		if (fAnimating) {
			fUpdatePending = true;
			return;
		}
		fUpdatePending = false;

		// icons from the icon theme in use, as it changes, and again a moment
		// later for folders' own icons, which Power Theme sets after
		if (_WatchIconTheme()) {
			_ReloadIcons();
			fReloadAt = system_time() + 3000000;
		} else if (fReloadAt != 0 && system_time() >= fReloadAt) {
			fReloadAt = 0;
			_ReloadIcons();
		}
		const bool trashFull = _TrashFull();
		if (trashFull != fTrashFull) {
			fTrashFull = trashFull;
			for (DockItem& item : fItems) {
				if (item.kind == DOCK_TRASH)
					_LoadIcons(item);
			}
			Redraw();
		}

		std::map<BString, team_id> running;
		std::map<entry_ref, team_id> runningFiles;
		std::vector<std::pair<entry_ref, team_id>> others;
		BList teams;
		be_roster->GetAppList(&teams);
		for (int32 i = 0; i < teams.CountItems(); i++) {
			const team_id team = (team_id)(addr_t)teams.ItemAt(i);
			app_info info;
			if (team == be_app->Team()
				|| be_roster->GetRunningAppInfo(team, &info) != B_OK
				|| (info.flags & B_BACKGROUND_APP) != 0
				|| strcasecmp(info.signature, kDeskbarSignature) == 0)
				continue;
			BString signature(info.signature);
			signature.ToLower();
			running[signature] = team;
			runningFiles[info.ref] = team;
			others.push_back(std::make_pair(info.ref, team));
		}

		bool changed = false;
		// the launchers, and those that only stay while they run
		for (size_t i = 0; i < fItems.size(); i++) {
			DockItem& item = fItems[i];
			if (item.kind != DOCK_APP)
				continue;
			BString signature(item.signature);
			signature.ToLower();
			// by its signature, or by its file if it has none
			team_id team = -1;
			auto found = running.find(signature);
			if (found != running.end() && !signature.IsEmpty())
				team = found->second;
			else {
				BEntry entry(&item.ref, true);
				entry_ref file;
				auto byFile = runningFiles.end();
				if (entry.GetRef(&file) == B_OK)
					byFile = runningFiles.find(file);
				if (byFile != runningFiles.end())
					team = byFile->second;
			}
			if (team != item.team) {
				item.team = team;
				if (team >= 0)
					item.bounceStart = 0;
				changed = true;
			}
			if (team < 0 && !item.kept) {
				delete item.icon;
				delete item.big;
				fItems.erase(fItems.begin() + i);
				i--;
				changed = true;
			}
		}
		// new ones go before the line
		for (const auto& other : others) {
			bool known = false;
			for (const DockItem& item : fItems)
				known |= item.kind == DOCK_APP && item.team == other.second;
			if (known)
				continue;
			DockItem item = _MakeItem(DOCK_APP, other.first, false);
			item.team = other.second;
			fItems.insert(fItems.begin() + _SeparatorIndex(), item);
			changed = true;
		}
		if (changed)
			_Animate();
	}

private:
	static float _ShelfDepth()
	{
		return roundf(kDockIcon * 0.3f);
	}

	DockItem _MakeItem(dock_kind kind, const entry_ref& ref, bool kept)
	{
		DockItem item;
		item.kind = kind;
		item.ref = ref;
		item.kept = kept;
		item.team = -1;
		item.icon = NULL;
		item.big = NULL;
		item.bounceStart = 0;
		item.size = kDockIcon;
		item.target = kDockIcon;
		item.x = 0;
		if (kind == DOCK_SEPARATOR)
			return item;
		item.label = ref.name;
		if (kind == DOCK_APP)
			item.signature = app_signature(ref);
		else if (kind == DOCK_TRASH)
			item.label = "Trash";
		_LoadIcons(item);
		return item;
	}

	//!	The launchers kept, or a first set.
	void _Load()
	{
		BMessage settings;
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
			&& path.Append(kDockSettingsName) == B_OK) {
			BFile file(path.Path(), B_READ_ONLY);
			if (file.InitCheck() == B_OK)
				settings.Unflatten(&file);
		}
		fMagnify = settings.GetBool("magnify", true);

		entry_ref ref;
		if (settings.FindRef("app", &ref) == B_OK) {
			for (int32 i = 0; settings.FindRef("app", i, &ref) == B_OK; i++) {
				if (BEntry(&ref).Exists())
					fItems.push_back(_MakeItem(DOCK_APP, ref, true));
			}
		} else {
			const char* programs[] = { "Tracker", "WebPositive", "Mail",
				"People", "Terminal", "StyledEdit", "MediaPlayer",
				"ShowImage", "HaikuDepot", "PowerTheme" };
			for (const char* program : programs) {
				// by its signature: the preferences have a "Tracker" too
				const bool found = strcmp(program, "Tracker") == 0
					? be_roster->FindApp(kTrackerSignature, &ref) == B_OK
					: find_program(program, ref);
				if (found)
					fItems.push_back(_MakeItem(DOCK_APP, ref, true));
			}
		}
		fItems.push_back(_MakeItem(DOCK_SEPARATOR, entry_ref(), true));
		BPath folder;
		if (find_directory(B_USER_DIRECTORY, &folder) == B_OK
			&& get_ref_for_path(folder.Path(), &ref) == B_OK)
			fItems.push_back(_MakeItem(DOCK_FOLDER, ref, true));
		if (find_directory(B_TRASH_DIRECTORY, &folder) == B_OK
			&& get_ref_for_path(folder.Path(), &ref) == B_OK)
			fItems.push_back(_MakeItem(DOCK_TRASH, ref, true));
	}

	void _Save()
	{
		BMessage settings;
		settings.AddBool("magnify", fMagnify);
		for (const DockItem& item : fItems) {
			if (item.kind == DOCK_APP && item.kept)
				settings.AddRef("app", &item.ref);
		}
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
			|| path.Append(kDockSettingsName) != B_OK)
			return;
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			settings.Flatten(&file);
	}

	/*!	Reads which icon theme is in use when Power Theme's settings change,
		and returns whether it's another one.
	*/
	bool _WatchIconTheme()
	{
		time_t changed;
		BString id = current_icon_theme(changed);
		if (changed == fIconsChanged && (fIconTheme != NULL || changed != 0))
			return false;
		fIconsChanged = changed;
		// Haiku's own icons, dark or not, leave applications theirs
		if (id == "haiku" || id == "haiku-dark")
			id = "";
		const BString old = fIconTheme != NULL ? fIconTheme->ID() : "";
		if (id == old)
			return false;
		delete fIconTheme;
		fIconTheme = id.IsEmpty() ? NULL : load_icon_theme(id.String());
		return true;
	}

	bool _TrashFull()
	{
		BPath trash;
		if (find_directory(B_TRASH_DIRECTORY, &trash) != B_OK)
			return false;
		BDirectory directory(trash.Path());
		BEntry entry;
		return directory.GetNextEntry(&entry) == B_OK;
	}

	//!	The names \a item's icon may have in an icon theme.
	BStringList _IconNames(const DockItem& item)
	{
		BStringList names;
		switch (item.kind) {
			case DOCK_TRASH:
				names.Add(fTrashFull ? "user-trash-full" : "user-trash");
				names.Add("user-trash");
				break;
			case DOCK_FOLDER:
			{
				BPath home;
				if (find_directory(B_USER_DIRECTORY, &home) == B_OK
					&& BPath(&item.ref) == home) {
					names.Add("user-home");
					names.Add("folder-home");
				}
				names.Add("folder");
				break;
			}
			case DOCK_APP:
			{
				BString signature(item.signature);
				signature.ToLower();
				auto found = fAppIconNames.find(signature);
				if (found != fAppIconNames.end())
					names = found->second;
				// many themes have icons named after the program
				if (item.ref.name != NULL) {
					BString name(item.ref.name);
					name.ToLower();
					names.Add(name);
				}
				break;
			}
			default:
				break;
		}
		return names;
	}

	//!	\a item's icons, from the icon theme if it has them.
	void _LoadIcons(DockItem& item)
	{
		delete item.icon;
		delete item.big;
		item.icon = item.big = NULL;
		if (item.kind == DOCK_SEPARATOR)
			return;
		if (fIconTheme != NULL) {
			const BStringList names = _IconNames(item);
			item.icon = fIconTheme->BitmapAt(names, kDockIcon);
			if (item.icon != NULL)
				item.big = fIconTheme->BitmapAt(names, kDockMagnified);
		}
		if (item.icon == NULL) {
			item.icon = icon_for(item.ref, kDockIcon);
			item.big = icon_for(item.ref, kDockMagnified);
		}
		// exactly at rest's size and the biggest, so that what's drawn
		// between is never scaled down to less than half
		item.icon = _Exactly(item.icon, kDockIcon);
		item.big = _Exactly(item.big, kDockMagnified);
	}

	static BBitmap* _Exactly(BBitmap* icon, float size)
	{
		if (icon == NULL || icon->Bounds().Width() + 1 == size)
			return icon;
		BBitmap* scaled = Canvas::Scaled(icon, (int32)size);
		delete icon;
		return scaled;
	}

	void _ReloadIcons()
	{
		for (DockItem& item : fItems)
			_LoadIcons(item);
		Redraw();
	}

	int32 _SeparatorIndex() const
	{
		for (size_t i = 0; i < fItems.size(); i++) {
			if (fItems[i].kind == DOCK_SEPARATOR)
				return (int32)i;
		}
		return (int32)fItems.size();
	}

	static float _Width(const DockItem& item)
	{
		return item.kind == DOCK_SEPARATOR ? kDockGap * 3 : item.size + kDockGap;
	}

	/*!	The size each icon is heading for: those near the pointer grow. The
		pointer is placed against the icons at rest, so that the same spot
		gives the same sizes however far they have grown.
	*/
	void _Targets()
	{
		const BRect bounds = Bounds();
		const bool magnify = fMagnify && fHovered && desktop_composited();
		fPointer = -1;
		float rest = 0;
		for (const DockItem& item : fItems)
			rest += item.kind == DOCK_SEPARATOR ? kDockGap * 3
				: kDockIcon + kDockGap;
		if (magnify) {
			float x = floorf((bounds.Width() + 1 - rest) / 2);
			for (size_t i = 0; i < fItems.size(); i++) {
				const float width = fItems[i].kind == DOCK_SEPARATOR
					? kDockGap * 3 : kDockIcon + kDockGap;
				if (fMouseX >= x && fMouseX < x + width)
					fPointer = i + (fMouseX - x) / width;
				x += width;
			}
		}
		for (size_t i = 0; i < fItems.size(); i++) {
			DockItem& item = fItems[i];
			item.target = kDockIcon;
			if (fPointer < 0 || item.kind == DOCK_SEPARATOR)
				continue;
			const float distance = fabsf(i + 0.5f - fPointer);
			const float reach = 2.5f;
			if (distance < reach) {
				const float c = cosf(distance / reach * M_PI / 2);
				item.target = kDockIcon + (kDockMagnified - kDockIcon) * c * c;
			}
		}
	}

	/*!	Each icon's place, from the sizes they have now: the dock widens
		round the spot under the pointer, so that it stays there.
	*/
	void _Place()
	{
		const BRect bounds = Bounds();
		float total = 0;
		for (const DockItem& item : fItems)
			total += _Width(item);
		float left = floorf((bounds.Width() + 1 - total) / 2);
		if (fPointer >= 0 && fPointer < fItems.size()) {
			float before = 0;
			const int32 index = (int32)fPointer;
			for (int32 i = 0; i < index; i++)
				before += _Width(fItems[i]);
			before += (fPointer - index) * _Width(fItems[index]);
			left = fMouseX - before;
		}
		left = std::max(kDockGap * 3, std::min(left,
			bounds.Width() - kDockGap * 3 - total));
		float x = left;
		for (DockItem& item : fItems) {
			item.x = x + _Width(item) / 2;
			x += _Width(item);
		}
	}

	//!	Sizes and places at once, with nothing easing.
	void _Layout()
	{
		_Targets();
		for (DockItem& item : fItems)
			item.size = item.target;
		_Place();
	}

	float _Left() const
	{
		return fItems.empty() ? 0 : fItems.front().x
			- _Width(fItems.front()) / 2;
	}

	float _Right() const
	{
		return fItems.empty() ? 0 : fItems.back().x
			+ _Width(fItems.back()) / 2;
	}

	int32 _ItemAt(float x) const
	{
		for (size_t i = 0; i < fItems.size(); i++) {
			const float half = _Width(fItems[i]) / 2;
			if (x >= fItems[i].x - half && x < fItems[i].x + half)
				return (int32)i;
		}
		return -1;
	}

	//!	How high an icon that is opening hops: twice, then it waits.
	float _Bounce(const DockItem& item, bigtime_t now) const
	{
		if (item.bounceStart == 0)
			return 0;
		const float t = (now - item.bounceStart) / 1000000.0f;
		if (t > 1.6f)
			return 0;
		return kBounce * fabsf(sinf(t * M_PI / 0.8f));
	}

	void _DrawSeparator(float x, float back, float front)
	{
		// dashes from the back of the shelf to the front, leaning out
		const float slant = (x - (_Left() + _Right()) / 2) * 0.04f;
		for (float y = back + 1; y < front; y += 1) {
			if ((int32)(y - back) % 3 == 2)
				continue;
			const float t = (y - back) / (front - back);
			fCanvas.Blend((int32)roundf(x + slant * t), (int32)y,
				make_rgba(0xff, 0xff, 0xff, 0xc0));
			fCanvas.Blend((int32)roundf(x + slant * t) + 1, (int32)y,
				make_rgba(0x40, 0x40, 0x40, 0x60));
		}
	}

	//!	Starts the frames, if they aren't running; each one draws once.
	void _Animate()
	{
		if (fAnimating)
			return;
		fAnimating = true;
		atomic_set(&fTicking, 1);
		release_sem(fFrameSem);
	}

	static status_t _TickerEntry(void* data)
	{
		((DockView*)data)->_Ticker();
		return B_OK;
	}

	/*!	Asks for a frame every 60th of a second while the dock moves, at the
		display's priority: BMessageRunner's come from the registrar, which
		runs at normal priority and falls behind while the screen is busy,
		as it is when the pointer moves. A frame that hasn't been drawn yet
		isn't asked for again, and frames missed aren't made up in a burst.
	*/
	void _Ticker()
	{
		while (acquire_sem(fFrameSem) == B_OK) {
			bigtime_t next = system_time();
			while (atomic_get(&fTicking) != 0) {
				next += 16667;
				next = std::max(next, system_time());
				snooze_until(next, B_SYSTEM_TIMEBASE);
				if (atomic_get(&fTicking) == 0)
					break;
				if (atomic_test_and_set(&fFramePending, 1, 0) == 0)
					fMessenger.SendMessage(kAnimate);
			}
		}
	}

	/*!	A frame of growing, shrinking or hopping: each icon goes part of the
		way to its size, so they ease in and out. Stops when nothing moves.
	*/
	void _Step()
	{
		atomic_set(&fFramePending, 0);
		_Targets();
		// by the time since the last frame, so that a late one catches up
		// rather than stalls
		const bigtime_t stepTime = system_time();
		const float elapsed = fLastStep == 0 ? 16667
			: std::min((bigtime_t)50000, stepTime - fLastStep);
		fLastStep = stepTime;
		const float share = 1 - expf(-elapsed / 45000.0f);
		bool moving = false;
		for (DockItem& item : fItems) {
			const float gap = item.target - item.size;
			if (fabsf(gap) < 0.25f)
				item.size = item.target;
			else {
				item.size += gap * share;
				moving = true;
			}
		}
		_Place();
		Redraw();
		const bigtime_t now = system_time();
		for (DockItem& item : fItems) {
			if (item.bounceStart != 0 && now - item.bounceStart > 1600000)
				item.bounceStart = 0;
			moving |= item.bounceStart != 0;
		}
		if (!moving && fAnimating) {
			fLastStep = 0;
			fAnimating = false;
			atomic_set(&fTicking, 0);
			if (fUpdatePending)
				Update();
		}
	}

	void _Open(DockItem& item)
	{
		if (item.kind != DOCK_APP) {
			launch(item.ref);
			return;
		}
		if (item.team >= 0) {
			// what's in front comes forward; hidden windows are shown
			do_bring_to_front_team(BRect(), item.team, false);
			be_roster->ActivateApp(item.team);
			return;
		}
		item.bounceStart = system_time();
		_Animate();
		launch(item.ref);
	}

	void _ShowMenu(int32 index, BPoint where)
	{
		const DockItem& item = fItems[index];
		BPopUpMenu* menu = new BPopUpMenu("dock", false, false);
		if (item.kind == DOCK_APP && item.team >= 0) {
			int32 count = 0;
			int32* tokens = get_token_list(item.team, &count);
			for (int32 i = 0; i < count; i++) {
				client_window_info* info = get_window_info(tokens[i]);
				if (info == NULL)
					continue;
				if (info->feel == B_NORMAL_WINDOW_FEEL
					&& info->name[0] != '\0') {
					BMessage* message = new BMessage(kShowWindow);
					message->AddInt32("token", info->server_token);
					menu->AddItem(new BMenuItem(info->name, message));
				}
				free(info);
			}
			free(tokens);
			if (menu->CountItems() > 0)
				menu->AddSeparatorItem();
		}
		if (item.kind == DOCK_APP) {
			BMessage* message = new BMessage(item.kept ? kRemove : kKeep);
			message->AddInt32("index", index);
			menu->AddItem(new BMenuItem(item.kept ? "Remove from dock"
				: "Keep in dock", message));
			message = new BMessage(kShowInTracker);
			message->AddRef("refs", &item.ref);
			menu->AddItem(new BMenuItem("Show in Tracker", message));
		} else {
			BMessage* message = new BMessage(kLaunch);
			message->AddRef("refs", &item.ref);
			menu->AddItem(new BMenuItem("Open", message));
		}
		BMenuItem* magnify = new BMenuItem("Magnification",
			new BMessage(kMagnify));
		magnify->SetMarked(fMagnify);
		magnify->SetEnabled(desktop_composited());
		menu->AddSeparatorItem();
		menu->AddItem(magnify);
		if (item.kind == DOCK_APP && item.team >= 0) {
			menu->AddSeparatorItem();
			BMessage* message = new BMessage(kQuit);
			message->AddInt32("team", item.team);
			menu->AddItem(new BMenuItem("Quit", message));
		}
		menu->SetTargetForItems(this);
		menu->SetAsyncAutoDestruct(true);
		menu->Go(where, true, true, true);
	}

	/*!	Files dropped on an application open in it, on the Trash go there,
		and applications dropped elsewhere are kept in the dock.
	*/
	void _Dropped(BMessage* message)
	{
		const BPoint where = ConvertFromScreen(message->DropPoint());
		const int32 index = _ItemAt(where.x);
		entry_ref ref;
		if (message->FindRef("refs", &ref) != B_OK)
			return;
		if (index >= 0 && fItems[index].kind == DOCK_APP) {
			BMessage open(B_REFS_RECEIVED);
			for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
				open.AddRef("refs", &ref);
			be_roster->Launch(&fItems[index].ref, &open);
			return;
		}
		if (index >= 0 && fItems[index].kind == DOCK_TRASH) {
			BPath trash;
			if (find_directory(B_TRASH_DIRECTORY, &trash) != B_OK)
				return;
			BDirectory directory(trash.Path());
			for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK;
					i++) {
				BEntry entry(&ref);
				BPath original;
				entry.GetPath(&original);
				if (entry.MoveTo(&directory) == B_OK) {
					// as Tracker marks it, so it can be put back
					BNode node(&entry);
					node.WriteAttr("_trk/original_path", B_STRING_TYPE, 0,
						original.Path(), strlen(original.Path()) + 1);
				}
			}
			return;
		}
		for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
			if (app_signature(ref).IsEmpty())
				continue;
			int32 at = _SeparatorIndex();
			if (index >= 0 && index < at)
				at = index;
			fItems.insert(fItems.begin() + at, _MakeItem(DOCK_APP, ref,
				true));
		}
		_Save();
		Update();
		_Animate();
	}

	std::vector<DockItem> fItems;
	Canvas				fCanvas;
	float				fMouseX;
	bool				fHovered;
	int32				fPressed;
	bool				fMagnify;
	IconTheme*			fIconTheme;
	std::map<BString, BStringList> fAppIconNames;
	time_t				fIconsChanged;
	bigtime_t			fReloadAt;
	bool				fTrashFull;
	// the frames, asked for by a thread of their own
	bool				fAnimating;
	sem_id				fFrameSem;
	thread_id			fTicker;
	int32				fTicking;
	int32				fFramePending;
	BMessenger			fMessenger;
	// where the pointer is among the icons at rest, or -1
	float				fPointer;
	bigtime_t			fLastStep;
	// the applications running, to be looked at once it stops moving
	bool				fUpdatePending;
};


class DockWindow : public BWindow {
public:
	DockWindow()
		:
		BWindow(BRect(0, 0, 100, 100), "Dock", B_NO_BORDER_WINDOW_LOOK,
			B_FLOATING_ALL_WINDOW_FEEL,
			kWindowFlags | (desktop_composited() ? kTransparentWindow : 0),
			B_ALL_WORKSPACES)
	{
		fView = new DockView(Bounds());
		AddChild(fView);
		_Place();
		BMessage tick(kDockTick);
		fRunner = new BMessageRunner(BMessenger(this), &tick, 1000000);
		fView->Update();
	}

	~DockWindow()
	{
		delete fRunner;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kDockTick:
				fView->Update();
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
		const float height = DockView::WindowHeight();
		MoveTo(screen.left, screen.bottom - height + 1);
		ResizeTo(screen.Width(), height - 1);
	}

	DockView*			fView;
	BMessageRunner*		fRunner;
};


BWindow*
new_leopard_menu_bar()
{
	return new MenuBarWindow();
}


BWindow*
new_leopard_dock()
{
	return new DockWindow();
}
