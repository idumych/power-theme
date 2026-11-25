/*
 * Copyright 2026, Ian Dumych, idumych@gmail.com.
 * Distributed under the terms of the GNU GPL, version 3 or later.
 *
 * Expose Air Dark's window borders as a decorator style (DecorStyle.h): a
 * smoked glass frame after the KDE Plasma window decoration of that name by
 * Mark Whittaker (store.kde.org/p/2357318). Its pictures are in ExposeAirArt.h,
 * cut from the decoration by tools/make-expose-air-art.py, and its sizes and
 * colours are those of its exposeairdarkrc.
 *
 * On a composited desktop with glass frames (style interface 5) the frame is
 * glass, as the decoration is on KDE with blur: what is behind the window
 * shows through it, blurred, whatever the user's glass setting is. Elsewhere
 * the same pictures are laid over a dark frame. Where app_server says which
 * button the mouse is over (also style interface 5), it lights up, as on KDE.
 */


#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <new>
#include <vector>

#include <FindDirectory.h>
#include <Path.h>
#include <Window.h>

#include "ExposeAirArt.h"
#include "StyleKit.h"


using namespace StyleKit;


// exposeairdarkrc's layout: borders, title bar, the buttons' room
static const float kBorder = 8;
static const float kTitleHeight = 32;
static const float kSmallTitleHeight = 24;
static const float kTitleLeft = 14;
static const float kButtonsRight = 11;
static const float kTitleGap = 8;
static const int32 kCornerRadius = 6;
// a BeOS-style tab's boxes, as far in from its ends
static const float kBeOSButtonInset = 6;
static const int32 kFrameCornerRadius = 4;
// The art's rows: its outline and highlight on top, a plain one in between,
// and the line over the window's contents at the bottom.
static const int32 kEdgeRows = 2;
static const int32 kPlainRow = 15;

// Power Theme's "BeOS-style tabs": a tab as wide as its title, which can be
// moved along the top, over a frame all round, instead of a title bar.
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";
static bool sBeOSTabs = false;

static const rgb_color kActiveText = { 208, 209, 206, 255 };
static const rgb_color kInactiveText = { 187, 188, 185, 255 };
static const rgb_color kShadow = { 0, 0, 0, 255 };
static const rgb_color kHalo = { 0, 0, 0, 90 };
static const rgb_color kOutline = { 5, 6, 5, 255 };
// What the pictures are laid over without glass
static const rgb_color kSolid = { 30, 31, 28, 255 };


struct FrameArt {
	const AirPicture*	topLeft;
	const AirPicture*	top;
	const AirPicture*	topRight;
	const AirPicture*	left;
	const AirPicture*	right;
	const AirPicture*	bottomLeft;
	const AirPicture*	bottom;
	const AirPicture*	bottomRight;
};

static const FrameArt kActiveFrame = { &kFrameActiveTopLeft,
	&kFrameActiveTop, &kFrameActiveTopRight, &kFrameActiveLeft,
	&kFrameActiveRight, &kFrameActiveBottomLeft, &kFrameActiveBottom,
	&kFrameActiveBottomRight };
static const FrameArt kInactiveFrame = { &kFrameInactiveTopLeft,
	&kFrameInactiveTop, &kFrameInactiveTopRight, &kFrameInactiveLeft,
	&kFrameInactiveRight, &kFrameInactiveBottomLeft, &kFrameInactiveBottom,
	&kFrameInactiveBottomRight };


// A row of the frame's top, as wide and as high as it is drawn: the top
// picture widened smoothly, since stretching its streaks pixel by pixel would
// leave bands. A BeOS-style tab has no line at its bottom, and the frame's top
// under it has no edge along its top.
struct TitleRow {
	int32				width;
	int32				height;
	bool				bottomLine;
	int32				openLeft;
	int32				openRight;
	bool				squareLeft;
	bool				squareRight;
	std::vector<uint32>	pixels;
};

// Each window's own rows, in its cookie: windows draw their borders on
// threads of their own, at the same time.
struct WindowRows {
	TitleRow			title[2];
	TitleRow			top[2];
};

static decor_style sStyle;


//!	The window's row to build into, or \a scratch if it has none.
static TitleRow&
row_of(decor_window* window, bool top, bool active, TitleRow& scratch)
{
	WindowRows* rows = (WindowRows*)window->cookie;
	if (rows == NULL)
		return scratch;
	return top ? rows->top[active ? 0 : 1] : rows->title[active ? 0 : 1];
}


static uint32
top_look(const decor_window* window)
{
	return window->tabs[window->top_tab]->look;
}


static bool
is_glass(const decor_window* window)
{
	// only app_servers with style interface 5 have the field
	return window->size >= offsetof(decor_window, glass) + sizeof(bool)
		&& window->glass;
}


static bool
focused(const decor_window* window)
{
	return window->tabs[window->top_tab]->focused;
}


static float
title_height(uint32 look)
{
	return look == B_FLOATING_WINDOW_LOOK ? kSmallTitleHeight : kTitleHeight;
}


static uint32
mix(uint32 a, uint32 b, uint32 weight)
{
	// weight of 256 parts for b
	uint32 result = 0;
	for (int shift = 0; shift < 32; shift += 8) {
		uint32 from = (a >> shift) & 0xff;
		uint32 to = (b >> shift) & 0xff;
		result |= ((from * (256 - weight) + to * weight) >> 8) << shift;
	}
	return result;
}


/*!	The row of \a art the title row's row \a y comes from: a lower title bar
	keeps the top and bottom rows, with their lines, and leaves out the middle.
*/
static int32
source_row(int32 y, int32 height, int32 artHeight)
{
	if (height >= artHeight)
		return std::min(y, artHeight - 1);
	int32 half = height / 2;
	return y < half ? y : artHeight - (height - y);
}


static const TitleRow&
build_row(TitleRow& row, bool active, int32 width, int32 height,
	bool bottomLine, int32 openLeft = -1, int32 openRight = -2,
	bool squareLeft = false, bool squareRight = false)
{
	if (row.width == width && row.height == height
		&& row.bottomLine == bottomLine && row.openLeft == openLeft
		&& row.openRight == openRight && row.squareLeft == squareLeft
		&& row.squareRight == squareRight) {
		return row;
	}

	// A corner a tab goes on from is the frame's side, straight up.
	const FrameArt& art = active ? kActiveFrame : kInactiveFrame;
	const AirPicture* left = squareLeft ? art.left : art.topLeft;
	const AirPicture* middle = art.top;
	const AirPicture* right = squareRight ? art.right : art.topRight;

	row.width = width;
	row.height = height;
	row.bottomLine = bottomLine;
	row.openLeft = openLeft;
	row.openRight = openRight;
	row.squareLeft = squareLeft;
	row.squareRight = squareRight;
	row.pixels.assign((size_t)width * height, 0);

	const int32 middleWidth = width - left->width - right->width;
	std::vector<uint32> plain(width, 0);
	for (int32 y = -1; y < height; y++) {
		// the plain row first, for the parts left open
		int32 artY = y < 0 ? kPlainRow : source_row(y, height, middle->height);
		if (!bottomLine && artY >= middle->height - kEdgeRows)
			artY = kPlainRow;
		uint32* out = y < 0 ? &plain[0] : &row.pixels[(size_t)y * width];
		const uint32* leftRow = left->pixels
			+ std::min(artY, (int32)left->height - 1) * left->width;
		const uint32* middleRow = middle->pixels + artY * middle->width;
		const uint32* rightRow = right->pixels
			+ std::min(artY, (int32)right->height - 1) * right->width;

		for (int32 x = 0; x < left->width && x < width; x++)
			out[x] = leftRow[x];
		for (int32 x = 0; x < middleWidth; x++) {
			// 24.8 fixed point position in the picture
			uint32 position = middleWidth > 1
				? (uint32)((uint64)x * (middle->width - 1) * 256
					/ (middleWidth - 1))
				: 0;
			int32 column = position >> 8;
			int32 next = std::min(column + 1, (int32)middle->width - 1);
			out[left->width + x] = mix(middleRow[column], middleRow[next],
				position & 0xff);
		}
		for (int32 x = 0; x < right->width; x++) {
			int32 at = width - right->width + x;
			if (at >= 0)
				out[at] = rightRow[x];
		}

		if (y >= 0 && y < kEdgeRows) {
			for (int32 x = std::max(openLeft, (int32)0);
					x <= std::min(openRight, width - 1); x++) {
				out[x] = plain[x];
			}
		}
	}
	return row;
}


static void
draw_picture(decor_canvas* canvas, const AirPicture& picture, BRect to)
{
	if (!to.IsValid())
		return;
	canvas->draw_bitmap(canvas, picture.pixels, picture.width,
		picture.height, picture.width * 4,
		D(BRect(0, 0, picture.width - 1, picture.height - 1)), D(to));
}


/*!	Glass is drawn as it is, over what app_server has cleared; without it,
	over a dark frame. Pictures drawn over others are mixed with them.
*/
static void
set_copying(decor_canvas* canvas, bool glass)
{
	canvas->set_drawing_mode(canvas, glass ? B_OP_COPY : B_OP_ALPHA);
	canvas->set_blending_mode(canvas, B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
}


static void
set_mixing(decor_canvas* canvas)
{
	canvas->set_drawing_mode(canvas, B_OP_ALPHA);
	canvas->set_blending_mode(canvas, B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
}


/*!	Power Theme's setting is read each time a window's border is made: when it
	changes, Power Theme has app_server make them all again, but app_server
	keeps the style loaded.
*/
static void
read_settings()
{
	BPath path;
	sBeOSTabs = find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kBeOSTabsSetting) == B_OK
		&& access(path.Path(), F_OK) == 0;
}


static status_t
air_create_window(decor_window* window, decor_canvas* canvas)
{
	read_settings();

	WindowRows* rows = new(std::nothrow) WindowRows;
	if (rows == NULL)
		return B_NO_MEMORY;
	for (int32 i = 0; i < 2; i++)
		rows->title[i].width = rows->top[i].width = 0;
	window->cookie = rows;
	return B_OK;
}


static void
air_delete_window(decor_window* window)
{
	delete (WindowRows*)window->cookie;
	window->cookie = NULL;
}


/*!	How much of a tab isn't its title: on a title bar, the gap before the
	title, and the minimize, zoom and close buttons together at the right end;
	on a BeOS-style tab, as on BeOS, the close box at its left end and the zoom
	box at its right end, without a minimize button.
*/
static float
tab_chrome(uint32 flags)
{
	const bool closable = (flags & B_NOT_CLOSABLE) == 0;
	const bool zoomable = (flags & B_NOT_ZOOMABLE) == 0;
	if (sBeOSTabs) {
		return (closable ? kBeOSButtonInset + kButtonCloseActive.width
				+ kTitleGap : kTitleLeft)
			+ (zoomable ? kTitleGap + kButtonMaximizeActive.width
				+ kBeOSButtonInset : kTitleGap);
	}

	float buttons = 0;
	if (closable)
		buttons += kButtonCloseActive.width;
	if (zoomable)
		buttons += kButtonMaximizeActive.width;
	if ((flags & B_NOT_MINIMIZABLE) == 0)
		buttons += kButtonMinimizeActive.width;
	return kTitleLeft + kTitleGap + buttons + kButtonsRight;
}


static void
air_update_font(decor_window* window, decor_canvas* canvas)
{
	decor_font font;
	memset(&font, 0, sizeof(font));
	font.which = DECOR_FONT_PLAIN;
	font.flags = B_FORCE_ANTIALIASING;
	canvas->set_font(canvas, &font);
}


static void
air_layout(decor_window* window, decor_canvas* canvas)
{
	const uint32 look = top_look(window);
	if (look == B_NO_BORDER_WINDOW_LOOK)
		return;

	const BRect frame = R(window->frame);
	if (look == B_BORDERED_WINDOW_LOOK) {
		window->border_width = 1;
		window->left_border = D(BRect(frame.left - 1, frame.top,
			frame.left - 1, frame.bottom));
		window->right_border = D(BRect(frame.right + 1, frame.top,
			frame.right + 1, frame.bottom));
		window->top_border = D(BRect(frame.left - 1, frame.top - 1,
			frame.right + 1, frame.top - 1));
		window->bottom_border = D(BRect(frame.left - 1, frame.bottom + 1,
			frame.right + 1, frame.bottom + 1));
		return;
	}

	// A title bar sits right on the window's contents, between the borders'
	// outer edges. A BeOS-style tab sits on a frame that goes all round.
	const float b = kBorder;
	window->border_width = b;
	window->left_border = D(BRect(frame.left - b, frame.top, frame.left - 1,
		frame.bottom));
	window->right_border = D(BRect(frame.right + 1, frame.top,
		frame.right + b, frame.bottom));
	window->top_border = sBeOSTabs ? D(BRect(frame.left - b, frame.top - b,
		frame.right + b, frame.top - 1)) : Invalid();
	window->bottom_border = D(BRect(frame.left - b, frame.bottom + 1,
		frame.right + b, frame.bottom + b));
	window->tabs_slide = sBeOSTabs;

	const int32 count = window->tab_count;
	if (count == 0)
		return;

	float height = title_height(look);
	float bottom = frame.top - 1;
	float left = frame.left - b;
	float width = frame.Width() + 1 + 2 * b;
	if (sBeOSTabs) {
		height -= b;
		bottom -= b;
		if (count == 1) {
			const decor_tab* tab = window->tabs[0];
			// a pixel each for the title's ends, which a width doesn't count
			const float tabWidth = std::min(width, tab_chrome(tab->flags)
				+ ceilf(canvas->string_width(canvas, NULL, tab->title, -1))
				+ 2);
			left += std::max(0.0f, std::min(tab->location,
				width - tabWidth));
			width = tabWidth;
		}
	}
	const BRect bar(left, bottom - height + 1, left + width - 1, bottom);
	window->title_bar = D(bar);

	// a stack's tabs share the room evenly
	for (int32 i = 0; i < count; i++) {
		decor_tab* tab = window->tabs[i];
		BRect tabFrame(bar.left + floorf(width * i / count), bar.top,
			bar.left + floorf(width * (i + 1) / count) - 1, bar.bottom);
		tab->frame = D(tabFrame);
		if (sBeOSTabs && count > 1)
			tab->location = 0;

		for (int32 button = 0; button < DECOR_BUTTON_COUNT; button++)
			tab->buttons[button] = Invalid();
		const bool closable = (tab->flags & B_NOT_CLOSABLE) == 0;
		const bool zoomable = (tab->flags & B_NOT_ZOOMABLE) == 0;

		float textLeft = tabFrame.left + kTitleLeft;
		float textRight;
		if (sBeOSTabs) {
			// close at the left end, zoom at the right end
			if (closable) {
				const float left = tabFrame.left + kBeOSButtonInset;
				tab->buttons[DECOR_BUTTON_CLOSE] = D(BRect(left, tabFrame.top,
					left + kButtonCloseActive.width - 1,
					tabFrame.top + kButtonCloseActive.height - 1));
				textLeft = left + kButtonCloseActive.width + kTitleGap;
			}
			textRight = tabFrame.right - kTitleGap;
			if (zoomable) {
				const float right = tabFrame.right - kBeOSButtonInset;
				tab->buttons[DECOR_BUTTON_ZOOM] = D(BRect(
					right - kButtonMaximizeActive.width + 1, tabFrame.top,
					right, tabFrame.top + kButtonMaximizeActive.height - 1));
				textRight = right - kButtonMaximizeActive.width - kTitleGap;
			}
		} else {
			// close, zoom and minimize, from the right
			float right = tabFrame.right - kButtonsRight;
			const struct {
				int32				button;
				uint32				hiddenBy;
				const AirPicture*	picture;
			} buttons[] = {
				{ DECOR_BUTTON_CLOSE, B_NOT_CLOSABLE, &kButtonCloseActive },
				{ DECOR_BUTTON_ZOOM, B_NOT_ZOOMABLE, &kButtonMaximizeActive },
				{ DECOR_BUTTON_MINIMIZE, B_NOT_MINIMIZABLE,
					&kButtonMinimizeActive }
			};
			for (size_t k = 0; k < sizeof(buttons) / sizeof(buttons[0]); k++) {
				if ((tab->flags & buttons[k].hiddenBy) != 0)
					continue;
				const AirPicture* picture = buttons[k].picture;
				tab->buttons[buttons[k].button] = D(BRect(
					right - picture->width + 1, tabFrame.top, right,
					tabFrame.top + picture->height - 1));
				right -= picture->width;
			}
			textRight = right - kTitleGap;
		}

		tab->text_offset = textLeft - tabFrame.left;
		tab->title_room = std::max(0.0f, textRight - textLeft + 1);
		tab->truncate = DECOR_TRUNCATE_END;
		const float chrome = tab_chrome(tab->flags);
		tab->min_size = chrome + 3 * kTitleGap;
		tab->max_size = sBeOSTabs ? chrome + 2 + ceilf(canvas->string_width(
			canvas, NULL, tab->title, -1)) : 1e6;
	}
}


static void
air_draw_frame(decor_window* window, decor_canvas* canvas,
	decor_rect invalid)
{
	const uint32 look = top_look(window);
	if (look == B_NO_BORDER_WINDOW_LOOK)
		return;

	const BRect frame = R(window->frame);
	if (look == B_BORDERED_WINDOW_LOOK) {
		canvas->set_drawing_mode(canvas, B_OP_COPY);
		canvas->stroke_rect(canvas, D(BRect(frame.left - 1, frame.top - 1,
			frame.right + 1, frame.bottom + 1)), kOutline);
		return;
	}

	const bool glass = is_glass(window);
	const FrameArt& art = focused(window) ? kActiveFrame : kInactiveFrame;
	const BRect left = R(window->left_border);
	const BRect right = R(window->right_border);
	const BRect bottom = R(window->bottom_border);
	const BRect bottomLeft(bottom.left, bottom.top,
		bottom.left + art.bottomLeft->width - 1, bottom.bottom);
	const BRect bottomRight(bottom.right - art.bottomRight->width + 1,
		bottom.top, bottom.right, bottom.bottom);
	const BRect bottomMiddle(bottomLeft.right + 1, bottom.top,
		bottomRight.left - 1, bottom.bottom);

	const BRect top = R(window->top_border);
	if (!glass) {
		canvas->set_drawing_mode(canvas, B_OP_COPY);
		canvas->fill_rect(canvas, D(left), kSolid);
		canvas->fill_rect(canvas, D(right), kSolid);
		canvas->fill_rect(canvas, D(bottom), kSolid);
		if (top.IsValid())
			canvas->fill_rect(canvas, D(top), kSolid);
	}
	set_copying(canvas, glass);
	if (top.IsValid()) {
		// the frame's top, without its edge where the tab sits on it
		const bool active = focused(window);
		const BRect bar = R(window->title_bar);
		TitleRow scratch;
		scratch.width = 0;
		const TitleRow& row = build_row(row_of(window, true, active, scratch),
			active,
			top.IntegerWidth() + 1, top.IntegerHeight() + 1, true,
			bar.IsValid() ? (int32)(bar.left - top.left) + kEdgeRows : -1,
			bar.IsValid() ? (int32)(bar.right - top.left) - kEdgeRows : -2,
			bar.IsValid() && bar.left <= top.left + kFrameCornerRadius,
			bar.IsValid() && bar.right >= top.right - kFrameCornerRadius);
		canvas->draw_bitmap(canvas, &row.pixels[0], row.width, row.height,
			row.width * 4, D(BRect(0, 0, row.width - 1, row.height - 1)),
			D(top));
	}
	draw_picture(canvas, *art.left, left);
	draw_picture(canvas, *art.right, right);
	draw_picture(canvas, *art.bottomLeft, bottomLeft);
	draw_picture(canvas, *art.bottom, bottomMiddle);
	draw_picture(canvas, *art.bottomRight, bottomRight);
	canvas->set_drawing_mode(canvas, B_OP_COPY);
}


//!	Draws the part of the title row within \a clip.
static void
draw_title_row(decor_window* window, decor_canvas* canvas, BRect clip)
{
	const BRect bar = R(window->title_bar);
	clip = clip & bar;
	if (!clip.IsValid())
		return;

	const bool glass = is_glass(window);
	const bool active = focused(window);
	TitleRow scratch;
	scratch.width = 0;
	const TitleRow& row = build_row(row_of(window, false, active, scratch),
		active,
		bar.IntegerWidth() + 1, bar.IntegerHeight() + 1, !sBeOSTabs);
	if (!glass) {
		canvas->set_drawing_mode(canvas, B_OP_COPY);
		canvas->fill_rect(canvas, D(clip), kSolid);
	}
	set_copying(canvas, glass);
	BRect source = clip.OffsetByCopy(-bar.left, -bar.top);
	canvas->draw_bitmap(canvas, &row.pixels[0], row.width, row.height,
		row.width * 4, D(source), D(clip));
	canvas->set_drawing_mode(canvas, B_OP_COPY);
}


static void
air_draw_title(decor_window* window, decor_canvas* canvas, int32 index)
{
	const decor_tab* tab = window->tabs[index];
	const BRect frame = R(tab->frame);
	float ascent, descent, leading;
	canvas->font_height(canvas, NULL, &ascent, &descent, &leading);

	const float height = frame.Height() + 1;
	const BPoint baseline(frame.left + tab->text_offset,
		floorf(frame.top + (height - ascent - descent) / 2 + ascent));

	// Drawn on its own when the title changes: what was there goes first,
	// short of the buttons.
	draw_title_row(window, canvas, BRect(frame.left + tab->text_offset - 2,
		frame.top, frame.left + tab->text_offset + tab->title_room + 2,
		frame.bottom));

	// A dark halo, as the decoration's HaloActive, and its text shadow;
	// mixed in, so that the title ends up opaque on glass.
	set_mixing(canvas);
	const int32 length = tab->shown_title_length;
	for (int32 dy = -1; dy <= 1; dy++) {
		for (int32 dx = -1; dx <= 1; dx++) {
			if (dx != 0 || dy != 0) {
				canvas->draw_string(canvas, tab->shown_title, length,
					P(baseline + BPoint(dx, dy)), kHalo);
			}
		}
	}
	canvas->draw_string(canvas, tab->shown_title, length,
		P(baseline + BPoint(1, 1)), kShadow);
	canvas->draw_string(canvas, tab->shown_title, length, P(baseline),
		tab->focused ? kActiveText : kInactiveText);
	canvas->set_drawing_mode(canvas, B_OP_COPY);
}


static void
air_draw_button(decor_window* window, decor_canvas* canvas, int32 index,
	int32 button, decor_rect invalid)
{
	const decor_tab* tab = window->tabs[index];
	const BRect rect = R(tab->buttons[button]);
	if (!rect.IsValid())
		return;

	const bool active = tab->button_focus;
	const bool pressed = tab->pressed[button];
	// hovered is from version 5; an older app_server's decor_tab is too
	// small to have it
	const bool hovered = !pressed
		&& tab->size >= offsetof(decor_tab, hovered) + sizeof(tab->hovered)
		&& tab->hovered[button];
	const AirPicture* picture;
	switch (button) {
		case DECOR_BUTTON_CLOSE:
			picture = pressed
				? (active ? &kButtonClosePressed : &kButtonClosePressedInactive)
				: hovered
				? (active ? &kButtonCloseHover : &kButtonCloseHoverInactive)
				: (active ? &kButtonCloseActive : &kButtonCloseInactive);
			break;
		case DECOR_BUTTON_ZOOM:
			picture = pressed ? (active ? &kButtonMaximizePressed
					: &kButtonMaximizePressedInactive)
				: hovered ? (active ? &kButtonMaximizeHover
					: &kButtonMaximizeHoverInactive)
				: (active ? &kButtonMaximizeActive : &kButtonMaximizeInactive);
			break;
		case DECOR_BUTTON_MINIMIZE:
			picture = pressed ? (active ? &kButtonMinimizePressed
					: &kButtonMinimizePressedInactive)
				: hovered ? (active ? &kButtonMinimizeHover
					: &kButtonMinimizeHoverInactive)
				: (active ? &kButtonMinimizeActive : &kButtonMinimizeInactive);
			break;
		default:
			return;
	}

	// A button drawn on its own, as it's pressed, starts from a cleared
	// frame: the title row under it comes back first.
	draw_title_row(window, canvas, rect);
	set_mixing(canvas);
	draw_picture(canvas, *picture, rect);
	canvas->set_drawing_mode(canvas, B_OP_COPY);
}


static void
air_draw_tab(decor_window* window, decor_canvas* canvas, int32 index,
	decor_rect invalid)
{
	const decor_tab* tab = window->tabs[index];
	draw_title_row(window, canvas, R(tab->frame));
	air_draw_title(window, canvas, index);
	for (int32 button = 0; button < DECOR_BUTTON_COUNT; button++) {
		if (R(tab->buttons[button]).Intersects(R(invalid)))
			air_draw_button(window, canvas, index, button, invalid);
	}
}


static void
air_rounded_shape(decor_window* window, decor_shape_rect* shape,
	int32 count)
{
	// The title bar's outer corners are round, the bottom ones square; under
	// a BeOS-style tab, the frame's top corners are a little round, too.
	if (count < 2 || top_look(window) == B_BORDERED_WINDOW_LOOK)
		return;
	shape[1].left_top = kCornerRadius;
	shape[count - 1].right_top = kCornerRadius;
	if (sBeOSTabs) {
		// not where the tab goes on from the frame's side, though
		const BRect frame = R(shape[0].rect);
		const BRect bar = R(window->title_bar);
		if (!bar.IsValid() || bar.left > frame.left + kFrameCornerRadius)
			shape[0].left_top = kFrameCornerRadius;
		if (!bar.IsValid() || bar.right < frame.right - kFrameCornerRadius)
			shape[0].right_top = kFrameCornerRadius;
	}
}


extern "C" const decor_style*
instantiate_decor_style(uint32 apiVersion)
{
	// versions 1 and 2 were drafts with other structure layouts
	if (apiVersion < 3)
		return NULL;

	read_settings();

	memset(&sStyle, 0, sizeof(sStyle));
	// Up to what the app_server knows: it reads no further than that.
	sStyle.api_version = std::min(apiVersion, (uint32)DECOR_STYLE_API_VERSION);
	sStyle.size = sStyle.api_version >= 5 ? sizeof(sStyle)
		: sStyle.api_version == 4 ? offsetof(decor_style, flags)
		: offsetof(decor_style, rounded_shape);
	sStyle.name = "Expose Air Dark";
	sStyle.create_window = &air_create_window;
	sStyle.delete_window = &air_delete_window;
	sStyle.update_font = &air_update_font;
	sStyle.layout = &air_layout;
	sStyle.draw_frame = &air_draw_frame;
	sStyle.draw_tab = &air_draw_tab;
	sStyle.draw_title = &air_draw_title;
	sStyle.draw_button = &air_draw_button;
	sStyle.rounded_shape = &air_rounded_shape;
	sStyle.flags = DECOR_STYLE_GLASS;
	return &sStyle;
}
