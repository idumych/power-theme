/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The QNX window borders as a decorator style (DecorStyle.h), for app_server's
 * own decorator engine: a build of them keeps working on later systems. It
 * draws as QNXDecorator does. Colours and sizes were measured from
 * screenshots of QNX 6.2.1's Photon microGUI with its standard colours, on
 * toastytech.com.
 */


#include <unistd.h>

#include <FindDirectory.h>
#include <Path.h>

#include "StyleKit.h"


using namespace StyleKit;


static const rgb_color kBlack = Grey(0);
static const rgb_color kOuter = Grey(0x3f);
static const rgb_color kRidgeLight = Grey(0xff);
static const rgb_color kRidgeFace = Grey(0xd9);
static const rgb_color kRidgeShade = Grey(0x9d);
static const rgb_color kInner = Grey(0x4b);

static const rgb_color kPanelShade = Grey(0xa6);
static const rgb_color kPanelTop = Grey(0xc5);
static const rgb_color kPanelBottom = Grey(0xe4);
static const rgb_color kWellDark = Grey(0xba);
static const rgb_color kWellLight = Grey(0xf6);
static const rgb_color kBoxEdge = Grey(0x3e);
static const rgb_color kGroove = Grey(0x90);
static const rgb_color kPanelFoot = Grey(0xd8);

static const rgb_color kMenuEdge = Grey(0x48);
static const rgb_color kMenuLight = Grey(0xe3);
static const rgb_color kMenuFace = Grey(0xb7);
static const rgb_color kMenuDark = Grey(0x80);

static const rgb_color kCorner = Grey(0xd8);

static const float kBorder = 6;
static const float kTitleHeight = 21;
static const float kBox = 13;
static const float kResizeCornerLength = 15;

// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";

static bool sBeOSTabs = false;


//	#pragma mark - layout


static bool
has_title(const Window& window)
{
	switch ((int)window.Look()) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			return true;
	}
	return false;
}


//!	Whether the title bar spans the window, rather than being a BeOS-style tab.
static bool
has_full_title_bar(const Window& window)
{
	return has_title(window) && !sBeOSTabs;
}


static bool
has_resize_corner(const Window& window)
{
	return window.Look() == B_DOCUMENT_WINDOW_LOOK
		&& (window.Flags() & B_NOT_RESIZABLE) == 0;
}


//!	How wide a BeOS-style tab has to be: its menu box, title and panel.
static float
tab_width(const decor_tab& tab, Canvas& canvas)
{
	float width = 18 + 8 + 8;
	if ((tab.flags & B_NOT_CLOSABLE) == 0)
		width += 20;
	if ((tab.flags & B_NOT_ZOOMABLE) == 0)
		width += 16 + 10;
	if ((tab.flags & B_NOT_MINIMIZABLE) == 0)
		width += 16;
	width += 6;
	return width + ceilf(canvas.StringWidth(tab.title));
}


//!	The box nearest the bar's right end.
static BRect
first_button_slot(const decor_tab& tab)
{
	const BRect t = TabRect(tab);
	const float top = t.top + 5;
	const float right = t.right - 8;
	return BRect(right - kBox + 1, top, right, top + kBox - 1);
}


//!	How wide the panel of boxes is, from its dark line to the bar's end.
static float
gadgets_width(const decor_tab& tab)
{
	const int32 buttons[3] = { DECOR_BUTTON_MINIMIZE, DECOR_BUTTON_ZOOM,
		DECOR_BUTTON_CLOSE };
	for (int32 i = 0; i < 3; i++) {
		const BRect box = ButtonRect(tab, buttons[i]);
		if (box.IsValid())
			return TabRect(tab).right - box.left + 6;
	}
	return 0;
}


/*!	Lays the tabs out side by side across the whole window, each with its menu
	box at its left end, its boxes at its right end and its title between.

	A BeOS-style tab sits on the frame, over its outer lines, and is as wide as
	its title; a stack's tabs share the window's width.
*/
static void
layout_tabs(Window& window, Canvas& canvas)
{
	const int32 count = window.CountTabs();
	if (count == 0)
		return;

	const BRect borderRect = window.BorderRect();
	float left = borderRect.left;
	float width = borderRect.Width() + 1;
	const float height = kTitleHeight;
	// a full bar ends over the content's dark line; a tab, on the frame's
	// dark grey line
	const float bottom = sBeOSTabs ? borderRect.top + 1 : borderRect.top - 1;
	const float top = bottom - height + 1;
	if (sBeOSTabs && count == 1) {
		decor_tab& tab = window.TabAt(0);
		const float tabWidth = std::min(tab_width(tab, canvas), width);
		left += std::min(tab.location, width - tabWidth);
		width = tabWidth;
	}
	const float minTitle = 32;

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		decor_tab& tab = window.TabAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab.frame = D(BRect(tabLeft, top, tabRight, bottom));

		const bool closable = (tab.flags & B_NOT_CLOSABLE) == 0;
		const bool zoomable = (tab.flags & B_NOT_ZOOMABLE) == 0;
		const bool minimizable = (tab.flags & B_NOT_MINIMIZABLE) == 0;

		// from the right: close, a groove, zoom, then minimize
		BRect slot = first_button_slot(tab);
		if (closable) {
			tab.buttons[DECOR_BUTTON_CLOSE] = D(slot);
			slot.OffsetBy(-(kBox + (zoomable || minimizable ? 10 : 3)), 0);
		}
		if (zoomable) {
			tab.buttons[DECOR_BUTTON_ZOOM] = D(slot);
			slot.OffsetBy(-(kBox + 3), 0);
		}
		if (minimizable)
			tab.buttons[DECOR_BUTTON_MINIMIZE] = D(slot);

		const float gadgets = gadgets_width(tab);
		// the title keeps clear of the menu box, and the panel
		const float reserved = 18 + 6;
		const float room = (tabRight - tabLeft + 1) - reserved - gadgets - 6;
		const float taken = reserved + gadgets + 6;

		tab.text_offset = reserved;
		if (sBeOSTabs && count > 1)
			tab.location = 0;
		tab.min_size = 0;
		tab.max_size = taken + ceilf(canvas.StringWidth(tab.title));
		minimum += taken + minTitle;

		tab.title_room = std::max(0.0f, room);
		tab.truncate = count > 1 ? DECOR_TRUNCATE_END : DECOR_TRUNCATE_MIDDLE;
	}

	// app_server keeps the window at least this wide
	window.TabAt(0).min_size = minimum;

	window.SetTitleBarRect(BRect(left, top, left + width - 1, bottom));
}


static void
qnx_layout(decor_window* styleWindow, decor_canvas* styleCanvas)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const BRect frame = window.Frame();

	float borderWidth;
	switch ((int)window.Look()) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			borderWidth = kBorder;
			break;

		// menus and the like: a black line
		case B_BORDERED_WINDOW_LOOK:
			borderWidth = 1;
			break;

		default:
			borderWidth = 0;
			break;
	}

	if (borderWidth > 0) {
		const float w = borderWidth;
		// a full title bar takes the place of the frame's top, all but the
		// dark line over the content
		const float top = has_full_title_bar(window) ? 1 : w;
		window.SetBorders(w,
			BRect(frame.left - w, frame.top, frame.left - 1, frame.bottom),
			BRect(frame.left - w, frame.top - top, frame.right + w,
				frame.top - 1),
			BRect(frame.right + 1, frame.top, frame.right + w, frame.bottom),
			BRect(frame.left - w, frame.bottom + 1, frame.right + w,
				frame.bottom + w));
	}

	const BRect borderRect = window.BorderRect();
	if (has_resize_corner(window)) {
		// the content's corner, as far in as a scroll bar's width
		const float knob = kResizeCornerLength + borderWidth;
		window.SetResizeRect(BRect(borderRect.right - knob,
			borderRect.bottom - knob, borderRect.right, borderRect.bottom));
	}

	window.SetTabsSlide(sBeOSTabs);
	if (has_title(window))
		layout_tabs(window, canvas);
}


//	#pragma mark - drawing


/*!	The frame's bottom right corner, where it resizes the window: its ridge in
	the bar's blue for the last nine pixels each way, and on a document window
	the content's corner plain grey, where the scroll bars meet.
*/
static void
draw_resize_corner(Window& window, Canvas& canvas)
{
	const rgb_color face = canvas.UIColor(window.TopTab().focused
		? B_WINDOW_TAB_COLOR : B_WINDOW_INACTIVE_TAB_COLOR);
	const BRect r = window.BorderRect();
	const float rt = r.right, b = r.bottom;

	// the right side's ridge and the bottom's, up and along nine pixels
	canvas.FillRect(BRect(rt - 4, b - 9, rt - 2, b - 2), face);
	canvas.FillRect(BRect(rt - 9, b - 4, rt - 2, b - 2), face);
	canvas.StrokeLine(BPoint(rt - 9, b - 4), BPoint(rt - 5, b - 4),
		Shift(face, -0x1c));
	canvas.StrokeLine(BPoint(rt - 4, b - 9), BPoint(rt - 4, b - 4),
		Shift(face, -0x1c));
	canvas.StrokeLine(BPoint(rt - 9, b - 2), BPoint(rt - 2, b - 2),
		Shift(face, -0x3c));
	canvas.StrokeLine(BPoint(rt - 2, b - 9), BPoint(rt - 2, b - 2),
		Shift(face, -0x3c));

	if (!has_resize_corner(window))
		return;
	const BRect frame = window.Frame();
	const BRect corner(frame.right - kResizeCornerLength + 1,
		frame.bottom - kResizeCornerLength + 1, frame.right, frame.bottom);
	canvas.FillRect(corner, kCorner);
	canvas.StrokeLine(corner.LeftTop(), corner.RightTop(), kInner);
	canvas.StrokeLine(corner.LeftTop(), corner.LeftBottom(), kInner);
}


/*!	The frame: a black line outside, a dark grey one inside it, a ridge three
	pixels wide lit along each side's top or left, and a dark line round the
	content. A full title bar draws the outer lines along the top itself.
*/
static void
qnx_draw_frame(decor_window* styleWindow, decor_canvas* styleCanvas,
	decor_rect invalid)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const BRect rect = R(invalid);
	if (window.Look() == B_NO_BORDER_WINDOW_LOOK || window.BorderWidth() <= 0
		|| !rect.IsValid())
		return;

	const BRect r = window.BorderRect();
	if (window.Look() == B_BORDERED_WINDOW_LOOK) {
		canvas.StrokeRect(r, kBlack);
		return;
	}

	const bool fullBar = has_full_title_bar(window);
	const float l = r.left, t = r.top, rt = r.right, b = r.bottom;

	// the ridge's face, then its lines
	canvas.FillRect(BRect(l + 2, t, l + 4, b - 2) & rect, kRidgeFace);
	canvas.FillRect(BRect(rt - 4, t, rt - 2, b - 2) & rect, kRidgeFace);
	canvas.FillRect(BRect(l + 2, b - 4, rt - 2, b - 2) & rect, kRidgeFace);
	if (!fullBar)
		canvas.FillRect(BRect(l + 2, t + 2, rt - 2, t + 4) & rect, kRidgeFace);

	const float ridgeTop = fullBar ? t : t + 2;
	// left: lit on its outer side, shaded on its inner
	canvas.StrokeLine(BPoint(l + 2, ridgeTop), BPoint(l + 2, b - 3),
		kRidgeLight);
	canvas.StrokeLine(BPoint(l + 4, fullBar ? t : t + 4),
		BPoint(l + 4, b - 5), kRidgeShade);
	// right: lit on its inner side, shaded on its outer
	canvas.StrokeLine(BPoint(rt - 4, fullBar ? t : t + 4),
		BPoint(rt - 4, b - 5), kRidgeLight);
	canvas.StrokeLine(BPoint(rt - 2, ridgeTop), BPoint(rt - 2, b - 2),
		kRidgeShade);
	// bottom: lit along its top, shaded along its bottom
	canvas.StrokeLine(BPoint(l + 4, b - 4), BPoint(rt - 4, b - 4),
		kRidgeLight);
	canvas.StrokeLine(BPoint(l + 3, b - 2), BPoint(rt - 2, b - 2),
		kRidgeShade);
	if (!fullBar) {
		canvas.StrokeLine(BPoint(l + 2, t + 2), BPoint(rt - 3, t + 2),
			kRidgeLight);
		canvas.StrokeLine(BPoint(l + 4, t + 4), BPoint(rt - 5, t + 4),
			kRidgeShade);
	}

	// the outer lines
	canvas.StrokeLine(BPoint(l, t), BPoint(l, b), kBlack);
	canvas.StrokeLine(BPoint(rt, t), BPoint(rt, b), kBlack);
	canvas.StrokeLine(BPoint(l, b), BPoint(rt, b), kBlack);
	canvas.StrokeLine(BPoint(l + 1, t), BPoint(l + 1, b - 1), kOuter);
	canvas.StrokeLine(BPoint(rt - 1, t), BPoint(rt - 1, b - 1), kOuter);
	canvas.StrokeLine(BPoint(l + 1, b - 1), BPoint(rt - 1, b - 1), kOuter);
	if (!fullBar) {
		canvas.StrokeLine(BPoint(l, t), BPoint(rt, t), kBlack);
		canvas.StrokeLine(BPoint(l + 1, t + 1), BPoint(rt - 1, t + 1), kOuter);
	}

	// the line round the content
	canvas.StrokeRect(BRect(l + 5, fullBar ? t : t + 5, rt - 5, b - 5),
		kInner);

	if (has_resize_corner(window) || ((window.Flags() & B_NOT_RESIZABLE) == 0
			&& window.Look() != B_BORDERED_WINDOW_LOOK))
		draw_resize_corner(window, canvas);
}


/*!	The window menu box, sunk into the bar's left end: a grey box with a dark
	edge and three bars across it, each lit along its top.
*/
static void
draw_menu_box(const decor_tab& tab, Canvas& canvas, rgb_color face)
{
	const BRect t = TabRect(tab);
	const BRect well(t.left + 5, t.top + 6, t.left + 17, t.top + 17);
	if (well.right >= t.right - gadgets_width(tab) - 2)
		return;

	canvas.StrokeLine(well.LeftTop(), well.RightTop(), Shift(face, -0x1e));
	canvas.StrokeLine(well.LeftTop(), well.LeftBottom(), Shift(face, -0x1e));
	canvas.StrokeLine(well.LeftBottom(), well.RightBottom(),
		Shift(face, 0x1e));
	canvas.StrokeLine(BPoint(well.right, well.top + 1), well.RightBottom(),
		Shift(face, 0x1e));

	const BRect box(well.left + 1, well.top + 1, well.right - 1,
		well.bottom - 1);
	canvas.StrokeRect(box, kMenuEdge);
	for (int32 i = 0; i < 3; i++) {
		const float y = box.top + 1 + i * 3;
		if (y + 2 >= box.bottom + 1)
			break;
		canvas.StrokeLine(BPoint(box.left + 1, y), BPoint(box.right - 1, y),
			kMenuLight);
		canvas.StrokeLine(BPoint(box.left + 1, y + 1),
			BPoint(box.right - 1, y + 1), kMenuFace);
		if (y + 2 < box.bottom) {
			canvas.StrokeLine(BPoint(box.left + 1, y + 2),
				BPoint(box.right - 1, y + 2), kMenuDark);
		}
	}
}


/*!	Where the title goes: in the middle of the blue, clear of the menu box and
	the panel, with its baseline where Photon's was.
*/
static BRect
title_rect(const decor_tab& tab, Canvas& canvas)
{
	const BRect t = TabRect(tab);
	font_height fontHeight;
	canvas.GetFontHeight(fontHeight);
	const float width = canvas.StringWidth(tab.shown_title,
		tab.shown_title_length);

	const float left = t.left + tab.text_offset;
	const float right = t.right - gadgets_width(tab) - 6;
	float x = floorf((left + right + 1 - width) / 2);
	x = std::max(left, std::min(x, right - width));
	// in the middle of the shaded rows
	const float top = t.top + 5;
	const float space = t.bottom - 1 - top;
	const float baseline = floorf(top + (space + fontHeight.ascent
		- fontHeight.descent) / 2 + 1);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


static void
draw_title(const decor_tab& tab, Canvas& canvas)
{
	const BRect title = title_rect(tab, canvas);

	canvas.SetDrawingMode(B_OP_OVER);
	canvas.DrawString(tab.shown_title, tab.shown_title_length,
		BPoint(title.left, title.bottom), canvas.UIColor(tab.focused
			? B_WINDOW_TEXT_COLOR : B_WINDOW_INACTIVE_TEXT_COLOR));
	canvas.SetDrawingMode(B_OP_COPY);
}


/*!	A box on the panel, in a well darker along its top and left: a dark edge
	round a small picture of a window, in greys, lit along its top and left.
	Minimize is a darker pane with a triangle pointing down, over a strip like
	a taskbar; maximize a window with its title bar; close a sunk square with
	a small raised one in it. Pressed, the picture is darker.
*/
static void
draw_box(Canvas& canvas, BRect rect, bool pressed, int32 button)
{
	if (!rect.IsValid() || rect.Width() < 12)
		return;

	const float l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
	const int dim = pressed ? -0x24 : 0;
	auto grey = [dim](int level) {
		return Grey(level + dim);
	};
	auto line = [&canvas](float x0, float y0, float x1, float y1,
			rgb_color c) {
		canvas.StrokeLine(BPoint(x0, y0), BPoint(x1, y1), c);
	};

	// the well
	line(l - 1, t - 1, r, t - 1, kWellDark);
	line(l - 1, t - 1, l - 1, b + 1, kWellDark);
	line(l, b + 1, r + 1, b + 1, kWellLight);
	line(r + 1, t - 1, r + 1, b, kWellLight);
	canvas.StrokeRect(rect, kBoxEdge);

	// a shaded face, inside the edge from l + 1 to r - 1 and t + 1 to b - 1:
	// from \a top to \a bottom, lit along its left and shaded along its right
	auto face = [&](float y0, float y1, int top, int bottom, int lit,
			int shade) {
		for (float y = y0; y <= y1; y++) {
			const float amount = y1 > y0 ? (y - y0) / (y1 - y0) : 0;
			const int level = (int)roundf(top + (bottom - top) * amount);
			line(l + 2, y, r - 2, y, grey(level));
			line(l + 1, y, l + 1, y, grey(level + lit));
			line(r - 1, y, r - 1, y, grey(level - shade));
		}
	};

	switch (button) {
		case DECOR_BUTTON_MINIMIZE:
		{
			// the pane, its triangle, and the strip under a dark line
			line(l + 1, t + 1, r - 2, t + 1, grey(0xbf));
			line(r - 1, t + 1, r - 1, t + 1, grey(0x99));
			face(t + 2, t + 6, 0xa3, 0xa3, 0x1c, 0x21);
			line(l + 1, t + 7, l + 1, t + 7, grey(0x99));
			line(l + 2, t + 7, r - 1, t + 7, grey(0x82));
			for (int32 i = 0; i < 3; i++) {
				line(l + 4 + i, t + 3 + i, l + 8 - i, t + 3 + i,
					grey(0x51));
			}
			line(l + 1, t + 8, r - 1, t + 8, kBoxEdge);
			line(l + 1, t + 9, r - 1, t + 9, grey(0xec));
			line(l + 2, t + 10, r - 2, t + 10, grey(0xba));
			line(l + 1, t + 10, l + 1, t + 10, grey(0xd3));
			line(r - 1, t + 10, r - 1, t + 10, grey(0x96));
			line(l + 2, t + 11, r - 1, t + 11, grey(0x92));
			line(l + 1, t + 11, l + 1, t + 11, grey(0xa8));
			break;
		}
		case DECOR_BUTTON_ZOOM:
		{
			// the title bar, then the window's shaded inside
			line(l + 1, t + 1, r - 2, t + 1, grey(0xec));
			line(r - 1, t + 1, r - 1, t + 1, grey(0xcf));
			line(l + 1, t + 2, l + 1, t + 2, grey(0xea));
			line(l + 2, t + 2, r - 2, t + 2, grey(0xde));
			line(r - 1, t + 2, r - 1, t + 2, grey(0xb2));
			line(l + 1, t + 3, l + 1, t + 3, grey(0xb7));
			line(l + 2, t + 3, r - 1, t + 3, grey(0xaf));
			line(l + 1, t + 4, l + 1, t + 4, grey(0xf2));
			line(l + 2, t + 4, r - 2, t + 4, grey(0xeb));
			line(r - 1, t + 4, r - 1, t + 4, grey(0xd8));
			face(t + 5, t + 10, 0xd1, 0xba, 0x12, 0x2a);
			line(l + 1, t + 11, l + 1, t + 11, grey(0xcf));
			line(l + 2, t + 11, r - 2, t + 11, grey(0x92));
			line(r - 1, t + 11, r - 1, t + 11, grey(0x9b));
			break;
		}
		case DECOR_BUTTON_CLOSE:
		{
			// the face, then a sunk square holding a small raised one
			line(l + 1, t + 1, r - 2, t + 1, grey(0xec));
			line(r - 1, t + 1, r - 1, t + 1, grey(0xcf));
			face(t + 2, t + 10, 0xdf, 0xba, 0x0a, 0x2c);
			line(l + 1, t + 11, l + 1, t + 11, grey(0xa8));
			line(l + 2, t + 11, r - 1, t + 11, grey(0x92));
			const BRect well(l + 3, t + 3, l + 9, t + 9);
			line(well.left, well.top, well.right - 1, well.top, grey(0x92));
			line(well.left, well.top, well.left, well.bottom - 1, grey(0x92));
			line(well.right, well.top, well.right, well.bottom, grey(0xec));
			line(well.left, well.bottom, well.right, well.bottom, grey(0xec));
			const BRect square(well.left + 1, well.top + 1, well.right - 1,
				well.bottom - 1);
			canvas.StrokeRect(square, kBoxEdge);
			line(square.left + 1, square.top + 1, square.right - 1,
				square.top + 1, grey(0xbf));
			line(square.left + 1, square.top + 1, square.left + 1,
				square.bottom - 1, grey(0xbf));
			line(square.left + 2, square.top + 2, square.left + 2,
				square.top + 2, grey(0xa4));
			line(square.right - 1, square.top + 1, square.right - 1,
				square.bottom - 1, grey(0x82));
			line(square.left + 1, square.bottom - 1, square.right - 1,
				square.bottom - 1, grey(0x82));
			break;
		}
		default:
			break;
	}
}


static void
draw_buttons(const decor_tab& tab, Canvas& canvas, BRect invalid)
{
	const int32 order[3] = { DECOR_BUTTON_CLOSE, DECOR_BUTTON_MINIMIZE,
		DECOR_BUTTON_ZOOM };
	for (int32 i = 0; i < 3; i++) {
		const BRect box = ButtonRect(tab, order[i]);
		if (invalid.Intersects(box))
			draw_box(canvas, box, tab.pressed[order[i]], order[i]);
	}
}


/*!	The grey panel at the bar's right end: a dark line and a shaded one along
	its left, shaded from darker at the top to lighter at the bottom, with the
	frame's ridge along its right, and a groove before the close box.
*/
static void
draw_gadgets(const decor_tab& tab, Canvas& canvas, BRect invalid)
{
	const BRect t = TabRect(tab);
	const float width = gadgets_width(tab);
	const float left = t.right - width;
	const float top = t.top + 2;

	canvas.StrokeLine(BPoint(left, top), BPoint(left, t.bottom), kOuter);
	canvas.StrokeLine(BPoint(left + 1, top), BPoint(left + 1, t.bottom),
		kPanelShade);
	// white rows round a shade from darker at the top to lighter at the
	// bottom, a grey row and a dark one under them
	const float right = t.right - 4;
	canvas.StrokeLine(BPoint(left + 2, top), BPoint(right, top), kRidgeLight);
	const float first = top + 1;
	const float last = t.bottom - 3;
	for (float row = first; row <= last; row++) {
		const float amount = last > first ? (row - first) / (last - first) : 0;
		canvas.StrokeLine(BPoint(left + 2, row), BPoint(right, row),
			Mix(kPanelTop, kPanelBottom, amount));
	}
	canvas.StrokeLine(BPoint(left + 2, t.bottom - 2),
		BPoint(right, t.bottom - 2), kRidgeLight);
	canvas.StrokeLine(BPoint(left + 2, t.bottom - 1),
		BPoint(right, t.bottom - 1), kPanelFoot);
	canvas.StrokeLine(BPoint(left + 2, t.bottom), BPoint(right, t.bottom),
		kPanelShade);
	// the frame's ridge, on round the bar's end
	canvas.StrokeLine(BPoint(t.right - 3, top), BPoint(t.right - 3, t.bottom),
		kRidgeFace);
	canvas.StrokeLine(BPoint(t.right - 2, top), BPoint(t.right - 2, t.bottom),
		kRidgeShade);

	const BRect close = ButtonRect(tab, DECOR_BUTTON_CLOSE);
	if (close.IsValid() && (ButtonRect(tab, DECOR_BUTTON_ZOOM).IsValid()
			|| ButtonRect(tab, DECOR_BUTTON_MINIMIZE).IsValid())) {
		const float x = close.left - 4;
		canvas.StrokeLine(BPoint(x, top), BPoint(x, t.bottom - 1), kGroove);
		canvas.StrokeLine(BPoint(x + 1, top), BPoint(x + 1, t.bottom - 1),
			kRidgeLight);
	}

	draw_buttons(tab, canvas, invalid);
}


/*!	The bar: black and dark grey lines along its top and ends, then the blue,
	with a light line along its top and left, a dark one under that and along
	its bottom and right, and between them a shade from light to dark; the
	window menu box sunk into its left end, the title in the middle, and the
	grey panel of boxes at its right end.
*/
static void
qnx_draw_tab(decor_window* styleWindow, decor_canvas* styleCanvas,
	int32 index, decor_rect invalidRect)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const decor_tab& tab = window.TabAt(index);
	const BRect t = TabRect(tab);
	const BRect invalid = R(invalidRect);
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const rgb_color face = canvas.UIColor(tab.focused
		? B_WINDOW_TAB_COLOR : B_WINDOW_INACTIVE_TAB_COLOR);
	const rgb_color light = Shift(face, 0x32);
	const rgb_color dark = Shift(face, -0x32);
	const rgb_color shadeTop = Shift(face, 0x0a);
	const rgb_color shadeBottom = Shift(face, -0x15);

	canvas.StrokeLine(t.LeftTop(), t.RightTop(), kBlack);
	canvas.StrokeLine(BPoint(t.left + 1, t.top + 1),
		BPoint(t.right - 1, t.top + 1), kOuter);
	canvas.StrokeLine(BPoint(t.left, t.top + 1), t.LeftBottom(), kBlack);
	canvas.StrokeLine(BPoint(t.left + 1, t.top + 2),
		BPoint(t.left + 1, t.bottom), kOuter);
	canvas.StrokeLine(BPoint(t.right, t.top + 1), t.RightBottom(), kBlack);
	canvas.StrokeLine(BPoint(t.right - 1, t.top + 2),
		BPoint(t.right - 1, t.bottom), kOuter);

	const float gadgets = gadgets_width(tab);
	// the blue, from inside the outer lines to the panel
	const float left = t.left + 2;
	const float right = gadgets > 0 ? t.right - gadgets - 1 : t.right - 2;
	if (right > left) {
		const float y = t.top + 2;
		canvas.StrokeLine(BPoint(left, y), BPoint(right, y), light);
		canvas.StrokeLine(BPoint(left, y + 1), BPoint(right, y + 1), face);
		canvas.StrokeLine(BPoint(left, y + 2), BPoint(right, y + 2), dark);
		canvas.StrokeLine(BPoint(left, y + 3), BPoint(right, y + 3), face);
		const float first = y + 4;
		const float last = t.bottom - 1;
		for (float row = first; row <= last; row++) {
			if (row < invalid.top || row > invalid.bottom)
				continue;
			const float amount = last > first ? (row - first) / (last - first)
				: 0;
			canvas.StrokeLine(BPoint(left, row), BPoint(right, row),
				Mix(shadeTop, shadeBottom, amount));
		}
		canvas.StrokeLine(BPoint(left, t.bottom), BPoint(right, t.bottom),
			dark);
		canvas.StrokeLine(BPoint(left, y), BPoint(left, t.bottom - 1), light);
		canvas.StrokeLine(BPoint(right, y + 1), BPoint(right, t.bottom), dark);
	}

	draw_menu_box(tab, canvas, face);
	draw_title(tab, canvas);
	if (gadgets > 0)
		draw_gadgets(tab, canvas, invalid);
}


static void
qnx_draw_button(decor_window* styleWindow, decor_canvas* styleCanvas,
	int32 index, int32 button, decor_rect invalid)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const decor_tab& tab = window.TabAt(index);
	draw_box(canvas, ButtonRect(tab, button), tab.pressed[button], button);
}


static void
qnx_update_font(decor_window* window, decor_canvas* styleCanvas)
{
	// Photon's titles are in its plain face, no larger than the bar has room
	Canvas canvas(styleCanvas);
	canvas.SetFont(DECOR_FONT_PLAIN,
		std::min(canvas.FontSize(DECOR_FONT_PLAIN), 12.0f),
		B_FORCE_ANTIALIASING);
}


//	#pragma mark - the style


static decor_style sStyle;


extern "C" const decor_style*
instantiate_decor_style(uint32 apiVersion)
{
	if (apiVersion < 1)
		return NULL;

	BPath path;
	sBeOSTabs = find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kBeOSTabsSetting) == B_OK
		&& access(path.Path(), F_OK) == 0;

	memset(&sStyle, 0, sizeof(sStyle));
	sStyle.size = sizeof(sStyle);
	sStyle.api_version = DECOR_STYLE_API_VERSION;
	sStyle.name = "QNX";
	sStyle.update_font = &qnx_update_font;
	sStyle.layout = &qnx_layout;
	sStyle.draw_frame = &qnx_draw_frame;
	sStyle.draw_tab = &qnx_draw_tab;
	sStyle.draw_button = &qnx_draw_button;
	return &sStyle;
}
