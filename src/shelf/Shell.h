/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * What PowerShelf's layouts share: the Photon layout (PowerShelf.cpp) and the
 * Leopard layout, a menu bar and a dock (Leopard.cpp).
 */
#ifndef POWER_SHELL_H
#define POWER_SHELL_H


#include <Bitmap.h>
#include <Entry.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <View.h>
#include <Window.h>

#include <map>
#include <vector>


// libbe's window list, as Deskbar uses it: BeOS's private window API, not in
// the public headers (headers/private/interface/WindowInfo.h)
struct window_info {
	team_id		team;
	int32		server_token;
	int32		thread;
	int32		client_token;
	int32		client_port;
	uint32		workspaces;
	int32		layer;
	uint32		feel;
	uint32		flags;
	int32		window_left;
	int32		window_top;
	int32		window_right;
	int32		window_bottom;
	int32		show_hide_level;
	bool		is_mini;
} _PACKED;

struct client_window_info : window_info {
	float		tab_height;
	float		border_size;
	char		name[0];
} _PACKED;

enum window_action {
	B_MINIMIZE_WINDOW,
	B_BRING_TO_FRONT
};

client_window_info* get_window_info(int32 token);
int32* get_token_list(team_id app, int32* count);
void do_window_action(int32 window_id, int32 action, BRect zoomRect,
	bool zoom);
void do_minimize_team(BRect zoomRect, team_id app, bool zoom);
void do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom);

namespace BPrivate {
status_t get_window_order(int32 workspace, int32** _tokens, int32* _count);
}


extern const char* kDeskbarSignature;
extern const char* kTrackerSignature;

static const uint32 kLaunch = 'lnch';
static const uint32 kShutDown = 'shut';
static const float kIconSize = 16;

// the layouts, as "shell layout" in Power Theme's settings names them
enum shell_layout {
	SHELL_LAYOUT_PHOTON,
	SHELL_LAYOUT_LEOPARD
};

shell_layout		current_layout();

// a file's or application's icon at \a size, or NULL
BBitmap*			icon_for(const entry_ref& ref, float size);
void				draw_icon(BView* view, const BBitmap* icon, BPoint where);

// the Deskbar's menu, made of these folders
std::vector<BPath>	menu_folders();
void				fill_menu(BMenu* menu, const std::vector<BPath>& folders,
						BHandler* target);
// opens \a ref: a folder in Tracker, anything else as the roster opens it
void				launch(const entry_ref& ref);
// the first of the system's apps, preferences or bin called \a name
bool				find_program(const char* name, entry_ref& ref);
// asks whether to shut down or restart, and does it
void				shut_down();

// the Deskbar's tray items, added to \a tray, each at most \a maxWidth wide
void				load_replicants(BView* tray, float maxWidth);

/*!	Keeps a window clear of the parts of the screen a layout takes, as Haiku
	has no way to set part of it aside: \a free is what's left. \a frames
	remembers each window's frame between calls.
*/
void				keep_clear(const client_window_info* info, BRect free,
						BRect screen, std::map<int32, BRect>& frames);

// windows of the Leopard layout
BWindow*			new_leopard_menu_bar();
BWindow*			new_leopard_dock();


//!	A menu item with the icon of what it opens.
class IconMenuItem : public BMenuItem {
public:
								IconMenuItem(const char* label,
									BMessage* message, const entry_ref& ref);
								IconMenuItem(BMenu* submenu,
									const entry_ref* ref);
	virtual						~IconMenuItem();

protected:
	virtual	void				GetContentSize(float* width, float* height);
	virtual	void				DrawContent();

private:
			BBitmap*			fIcon;
};


#endif	// POWER_SHELL_H
