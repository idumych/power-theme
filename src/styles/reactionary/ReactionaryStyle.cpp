/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The Reactionary window borders, after the Reactionary ME theme's Windows ME
 * decoration, as a decorator style (DecorStyle.h): a build of them keeps
 * working on later systems. It draws as ReactionaryDecorator does.
 *
 * Sizes are those of the classic look at a 12 pt font: a 4 pixel frame, an
 * 18 pixel title bar with a 1 pixel line under it, and 16 x 14 pixel
 * buttons. They grow with the window title font.
 */


#include <new>
#include <unistd.h>

#include <FindDirectory.h>
#include <Path.h>
#include <Region.h>

#include "StyleKit.h"


using namespace StyleKit;


// The decoration's colours (windowsME-aurorae/decoration.svg): a C0C0C0
// frame, lit from the top left, and title bar gradients ending in light blue
// (active) or light grey (inactive). The buttons are the colour scheme's
// D4D0C8 button face.
static const rgb_color kButtonFace = { 212, 208, 200, 255 };
static const rgb_color kLight = { 255, 255, 255, 255 };
static const rgb_color kShadow = { 128, 128, 128, 255 };
static const rgb_color kDarkShadow = { 64, 64, 64, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kActiveGradientEnd = { 166, 202, 240, 255 };
static const rgb_color kInactiveGradientEnd = { 192, 192, 192, 255 };

static const float kResizeKnobSize = 18.0;

// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";

static bool sBeOSTabs = false;


//! \a color with \a amount (0 to 1) of \a other mixed in.
static rgb_color
mix_color(const rgb_color& color, const rgb_color& other, float amount)
{
	rgb_color mixed = color;
	mixed.red = (uint8)(color.red + (other.red - color.red) * amount);
	mixed.green = (uint8)(color.green + (other.green - color.green) * amount);
	mixed.blue = (uint8)(color.blue + (other.blue - color.blue) * amount);
	return mixed;
}


// Button glyphs, from the decoration's 16 x 14 button bitmaps.
static const char* const kCloseGlyph[] = {
	"................",
	"................",
	"................",
	"....XX....XX....",
	".....XX..XX.....",
	"......XXXX......",
	".......XX.......",
	"......XXXX......",
	".....XX..XX.....",
	"....XX....XX....",
	"................",
	"................",
	"................",
	"................"
};

static const char* const kMinimizeGlyph[] = {
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"....XXXXXX......",
	"....XXXXXX......",
	"................",
	"................",
	"................"
};

static const char* const kZoomGlyph[] = {
	"................",
	"................",
	"...XXXXXXXXX....",
	"...XXXXXXXXX....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...XXXXXXXXX....",
	"................",
	"................",
	"................"
};

static const int32 kGlyphWidth = 16;
static const int32 kGlyphHeight = 14;


struct Geometry {
	float	border;
	float	titleHeight;
	float	buttonWidth;
	float	buttonHeight;
	float	buttonMargin;
		// between the buttons and the title bar's edges
	float	closeGap;
		// between the close button and the others
	float	titleInset;
	float	cornerLength;
		// how far a corner's resize area reaches along each border
};


//!	The frame's colours, from the window border colour. On a dark one, its
//!	bevels are derived from it, instead of Windows' fixed greys.
struct Colors {
	rgb_color	focusTab;
	rgb_color	focusText;
	rgb_color	nonFocusTab;
	rgb_color	nonFocusText;
	rgb_color	frameFace;
	rgb_color	frameLight;
	rgb_color	frameShadow;
	rgb_color	frameDarkShadow;
	rgb_color	frameEdge;
	rgb_color	inactiveGradientEnd;

	Colors(Canvas& canvas)
	{
		focusTab = canvas.UIColor(B_WINDOW_TAB_COLOR);
		focusText = canvas.UIColor(B_WINDOW_TEXT_COLOR);
		nonFocusTab = canvas.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
		nonFocusText = canvas.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

		frameFace = canvas.UIColor(B_WINDOW_BORDER_COLOR);
		if (frameFace.Brightness() < 128) {
			frameLight = mix_color(frameFace, kLight, 0.25f);
			frameShadow = mix_color(frameFace, kBlack, 0.4f);
			frameDarkShadow = kBlack;
			frameEdge = mix_color(frameFace, kLight, 0.12f);
			inactiveGradientEnd = mix_color(frameFace, kLight, 0.3f);
		} else {
			frameLight = kLight;
			frameShadow = kShadow;
			frameDarkShadow = kDarkShadow;
			frameEdge = kButtonFace;
			inactiveGradientEnd = kInactiveGradientEnd;
		}
	}
};


//!	A window's own: its geometry, as its last layout found it.
struct WindowState {
	Geometry	geometry;
};


static Geometry&
geometry_of(const Window& window)
{
	return static_cast<WindowState*>(window.Get()->cookie)->geometry;
}


//	#pragma mark - layout


static bool
has_tab(const Window& window)
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


static bool
is_small(const Window& window)
{
	return window.Look() == B_FLOATING_WINDOW_LOOK
		|| window.Look() == kLeftTitledWindowLook;
}


static bool
has_resize_grip(const Window& window)
{
	return window.Look() == B_DOCUMENT_WINDOW_LOOK
		&& (window.Flags() & B_NOT_RESIZABLE) == 0
		&& window.ResizeRect().IsValid();
}


static Geometry
make_geometry(const Window& window, Canvas& canvas)
{
	const bool small = is_small(window);
	const float size = canvas.FontSize(small ? DECOR_FONT_PLAIN
		: DECOR_FONT_BOLD);
	const float scale = std::max(size / 12.0f, 1.0f);

	font_height fontHeight;
	canvas.GetFontHeight(fontHeight);
	const float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	Geometry geometry;
	geometry.border = 2 + roundf(2 * scale);
	// 18 pixels, or 15 for tool windows, as on Windows, and more when the
	// font needs it
	geometry.titleHeight = std::max(small ? 15.0f : 18.0f,
		textHeight + (small ? 2 : 4));
	geometry.buttonHeight = geometry.titleHeight - 4;
	geometry.buttonWidth = geometry.buttonHeight + 2;
	geometry.buttonMargin = 2;
	geometry.closeGap = 2;
	geometry.titleInset = roundf(3 * scale);
	geometry.cornerLength = roundf(16 * scale);
	return geometry;
}


/*!	The part of a tab that's the title bar: all of it, or inside a BeOS-style
	tab's own frame.
*/
static BRect
title_rect(const decor_tab& tab, const Geometry& geometry)
{
	BRect title = TabRect(tab);
	if (sBeOSTabs) {
		title.left += geometry.border;
		title.right -= geometry.border;
		title.top += geometry.border;
		title.bottom = title.top + geometry.titleHeight - 1;
	}
	return title;
}


//!	How wide a BeOS-style tab has to be, for its title and buttons.
static float
tab_width(const decor_tab& tab, const Geometry& geometry, Canvas& canvas)
{
	float width = 2 * geometry.border + 2 * geometry.titleInset;
	if ((tab.flags & B_NOT_CLOSABLE) == 0)
		width += geometry.buttonMargin + geometry.buttonWidth;
	if ((tab.flags & B_NOT_ZOOMABLE) == 0)
		width += geometry.buttonMargin + geometry.buttonWidth;
	return width + ceilf(canvas.StringWidth(tab.title));
}


/*!	As on BeOS: close at the left end, zoom at the right, and no minimize. If
	they don't both fit, zoom goes first.
*/
static void
layout_tab_buttons(decor_tab& tab, const BRect& title,
	const Geometry& geometry)
{
	BRect button(0, 0, geometry.buttonWidth - 1, geometry.buttonHeight - 1);
	button.OffsetTo(title.left + geometry.buttonMargin,
		title.top + floorf((geometry.titleHeight - geometry.buttonHeight) / 2));

	if ((tab.flags & B_NOT_CLOSABLE) == 0 && button.right <= title.right)
		tab.buttons[DECOR_BUTTON_CLOSE] = D(button);

	button.OffsetTo(title.right - geometry.buttonMargin - geometry.buttonWidth
		+ 1, button.top);
	const BRect close = ButtonRect(tab, DECOR_BUTTON_CLOSE);
	const float taken = close.IsValid() ? close.right : title.left;
	if ((tab.flags & B_NOT_ZOOMABLE) == 0 && button.left > taken)
		tab.buttons[DECOR_BUTTON_ZOOM] = D(button);
}


/*!	Where the title can go: up to the leftmost button, and on a BeOS-style
	tab from after the close button.
*/
static void
title_span(const decor_tab& tab, const Geometry& geometry, float& left,
	float& right)
{
	const BRect title = title_rect(tab, geometry);
	const BRect minimize = ButtonRect(tab, DECOR_BUTTON_MINIMIZE);
	const BRect zoom = ButtonRect(tab, DECOR_BUTTON_ZOOM);
	const BRect close = ButtonRect(tab, DECOR_BUTTON_CLOSE);

	// the title stops short of the leftmost button
	right = title.right - geometry.buttonMargin;
	if (minimize.IsValid())
		right = minimize.left;
	else if (zoom.IsValid())
		right = zoom.left;
	else if (close.IsValid())
		right = close.left;
	right -= geometry.titleInset;

	// on a BeOS-style tab, it starts after the close button
	left = (sBeOSTabs && close.IsValid() ? close.right + 1 : title.left)
		+ geometry.titleInset;
}


/*!	The title bar spans the window, between the frame's sides; tabs of a
	stack share it equally. Each tab has its buttons at its right end:
	minimize and zoom side by side, then a gap and close.
*/
static void
layout_tabs(Window& window, Canvas& canvas, const Geometry& geometry)
{
	const int32 count = window.CountTabs();
	if (count == 0)
		return;

	const BRect frame = window.Frame();
	const BRect borderRect = window.BorderRect();
	BRect titleBar;
	if (sBeOSTabs) {
		// Above the frame, and over its top edge, so that they join up. A lone
		// tab is as wide as its title; a stack's share the window's width.
		titleBar.Set(borderRect.left,
			borderRect.top - geometry.border - geometry.titleHeight,
			borderRect.right, borderRect.top + 1);
		if (count == 1) {
			const decor_tab& tab = window.TabAt(0);
			const float width = std::min(tab_width(tab, geometry, canvas),
				borderRect.Width() + 1);
			const float location = std::min(tab.location,
				borderRect.Width() + 1 - width);
			titleBar.left = borderRect.left + location;
			titleBar.right = titleBar.left + width - 1;
		}
	} else {
		titleBar.Set(frame.left, frame.top - 1 - geometry.titleHeight,
			frame.right, frame.top - 2);
	}
	window.SetTitleBarRect(titleBar);

	const float width = titleBar.Width() + 1;
	for (int32 i = 0; i < count; i++) {
		decor_tab& tab = window.TabAt(i);

		BRect tabRect = titleBar;
		tabRect.left = titleBar.left + floorf(width * i / count);
		tabRect.right = titleBar.left + floorf(width * (i + 1) / count) - 1;
		tab.frame = D(tabRect);
		tab.text_offset = 10;
		if (count > 1)
			tab.location = 0;

		const BRect title = title_rect(tab, geometry);
		if (sBeOSTabs)
			layout_tab_buttons(tab, title, geometry);
		else {
			BRect button(0, 0, geometry.buttonWidth - 1,
				geometry.buttonHeight - 1);
			button.OffsetTo(title.right - geometry.buttonMargin
				- geometry.buttonWidth + 1,
				title.top + floorf((geometry.titleHeight
					- geometry.buttonHeight) / 2));

			if ((tab.flags & B_NOT_CLOSABLE) == 0) {
				tab.buttons[DECOR_BUTTON_CLOSE] = D(button);
				button.OffsetBy(-geometry.buttonWidth - geometry.closeGap, 0);
			}
			if ((tab.flags & B_NOT_ZOOMABLE) == 0) {
				tab.buttons[DECOR_BUTTON_ZOOM] = D(button);
				button.OffsetBy(-geometry.buttonWidth, 0);
			}
			if ((tab.flags & B_NOT_MINIMIZABLE) == 0)
				tab.buttons[DECOR_BUTTON_MINIMIZE] = D(button);

			// buttons that don't fit are left out
			const float minimum = title.left + geometry.titleInset;
			for (int32 b = 0; b < DECOR_BUTTON_COUNT; b++) {
				const BRect rect = ButtonRect(tab, b);
				if (rect.IsValid() && rect.left < minimum)
					tab.buttons[b] = Invalid();
			}
		}

		float left, right;
		title_span(tab, geometry, left, right);
		tab.title_room = std::max(0.0f, right - left);
		tab.truncate = DECOR_TRUNCATE_END;
	}
}


static void
reactionary_layout(decor_window* styleWindow, decor_canvas* styleCanvas)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	Geometry& geometry = geometry_of(window);
	geometry = make_geometry(window, canvas);
	const BRect frame = window.Frame();

	float borderWidth = 0;
	float top = 0;
	switch ((int)window.Look()) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			borderWidth = (int32)geometry.border;
			// the title bar and the line under it, unless it's a tab on top
			top = sBeOSTabs
				? geometry.border : geometry.border + geometry.titleHeight + 1;
			break;

		case B_MODAL_WINDOW_LOOK:
			borderWidth = (int32)geometry.border;
			top = borderWidth;
			break;

		case B_BORDERED_WINDOW_LOOK:
			borderWidth = 1;
			top = 1;
			break;
	}

	if (borderWidth > 0) {
		// the left and right borders don't include the corners
		window.SetBorders(borderWidth,
			BRect(frame.left - borderWidth, frame.top, frame.left - 1,
				frame.bottom),
			BRect(frame.left - borderWidth, frame.top - top,
				frame.right + borderWidth, frame.top - 1),
			BRect(frame.right + 1, frame.top, frame.right + borderWidth,
				frame.bottom),
			BRect(frame.left - borderWidth, frame.bottom + 1,
				frame.right + borderWidth, frame.bottom + borderWidth));
	}

	if (borderWidth > 1) {
		const BRect bottom = window.BottomBorder();
		const float knob = roundf(kResizeKnobSize
			* std::max(canvas.FontSize(is_small(window) ? DECOR_FONT_PLAIN
				: DECOR_FONT_BOLD) / 12.0f, 1.0f));
		window.SetResizeRect(BRect(bottom.right - knob, bottom.bottom - knob,
			bottom.right, bottom.bottom));
	}

	window.SetTabsSlide(sBeOSTabs);
	if (has_tab(window))
		layout_tabs(window, canvas, geometry);
}


static int32
reactionary_region_at(decor_window* styleWindow, decor_point point,
	int32* tabIndex)
{
	Window window(styleWindow);
	const BPoint where(point.x, point.y);

	for (int32 i = 0; i < window.CountTabs(); i++) {
		const decor_tab& tab = window.TabAt(i);
		if (ButtonRect(tab, DECOR_BUTTON_MINIMIZE).Contains(where)) {
			*tabIndex = i;
			return DECOR_REGION_MINIMIZE_BUTTON;
		}
		if (ButtonRect(tab, DECOR_BUTTON_CLOSE).Contains(where)) {
			*tabIndex = i;
			return DECOR_REGION_CLOSE_BUTTON;
		}
		if (ButtonRect(tab, DECOR_BUTTON_ZOOM).Contains(where)) {
			*tabIndex = i;
			return DECOR_REGION_ZOOM_BUTTON;
		}
		if (TabRect(tab).Contains(where)) {
			*tabIndex = i;
			return DECOR_REGION_TAB;
		}
	}

	if (has_resize_grip(window) && window.ResizeRect().Contains(where))
		return DECOR_REGION_RIGHT_BOTTOM_CORNER;

	const BRect frame = window.Frame();
	const BRect borderRect = window.BorderRect();
	const float borderWidth = window.BorderWidth();
	if (borderWidth <= 0 || !borderRect.Contains(where)
		|| frame.Contains(where))
		return DECOR_REGION_NONE;

	const bool onLeft = where.x < frame.left;
	const bool onRight = where.x > frame.right;
	const bool onTop = where.y < borderRect.top + borderWidth;
	const bool onBottom = where.y > frame.bottom;

	if (!onLeft && !onRight && !onTop && !onBottom) {
		// the line under the title bar moves the window, like the title
		if (!has_tab(window))
			return DECOR_REGION_NONE;
		*tabIndex = 0;
		return DECOR_REGION_TAB;
	}

	// Every edge resizes, and the ends of each edge resize from the corner,
	// as on Windows. The window behaviour ignores this for windows that
	// can't be resized.
	const float corner = geometry_of(window).cornerLength;
	const bool left = where.x < borderRect.left + corner;
	const bool right = where.x > borderRect.right - corner;
	const bool top = where.y < borderRect.top + corner;
	const bool bottom = where.y > borderRect.bottom - corner;

	if (left && top)
		return DECOR_REGION_LEFT_TOP_CORNER;
	if (right && top)
		return DECOR_REGION_RIGHT_TOP_CORNER;
	if (left && bottom)
		return DECOR_REGION_LEFT_BOTTOM_CORNER;
	if (right && bottom)
		return DECOR_REGION_RIGHT_BOTTOM_CORNER;

	if (onLeft)
		return DECOR_REGION_LEFT_BORDER;
	if (onRight)
		return DECOR_REGION_RIGHT_BORDER;
	if (onTop)
		return DECOR_REGION_TOP_BORDER;
	return DECOR_REGION_BOTTOM_BORDER;
}


static void
reactionary_footprint(decor_window* styleWindow, decor_region* region)
{
	Window window(styleWindow);
	if (window.Look() == B_NO_BORDER_WINDOW_LOOK || window.BorderWidth() <= 0)
		return;

	region->include(region, D(window.BorderRect()));
	if (sBeOSTabs && window.TitleBarRect().IsValid())
		region->include(region, D(window.TitleBarRect()));
	region->exclude(region, D(window.Frame()));

	// the size grip covers the content's bottom right corner
	if (has_resize_grip(window))
		region->include(region, D(window.ResizeRect()));
}


//	#pragma mark - drawing


/*!	One pixel wide frame round \a rect: \a topLeft along the top and left,
	\a bottomRight along the bottom and right, which own the corners they
	share.
*/
static void
draw_bevel(Canvas& canvas, BRect rect, rgb_color topLeft,
	rgb_color bottomRight)
{
	canvas.StrokeLine(rect.LeftTop(), BPoint(rect.right - 1, rect.top),
		topLeft);
	canvas.StrokeLine(rect.LeftTop(), BPoint(rect.left, rect.bottom - 1),
		topLeft);
	canvas.StrokeLine(rect.RightTop(), rect.RightBottom(), bottomRight);
	canvas.StrokeLine(rect.LeftBottom(), rect.RightBottom(), bottomRight);
}


/*!	The size grip of a document window, in the content's bottom right
	corner where the scroll bars meet: three diagonal ridges, each a white
	line with two mid grey ones below it, on the button face.
*/
static void
draw_resize_grip(Window& window, Canvas& canvas, const Colors& colors,
	BRect invalid)
{
	BRect grip = window.ResizeRect() & window.Frame();
	if (!grip.IsValid() || !invalid.Intersects(grip))
		return;

	canvas.FillRect(grip, colors.frameEdge);

	const int32 size = (int32)std::min(grip.Width(), grip.Height()) + 1;
	for (int32 dy = 0; dy < size; dy++) {
		for (int32 dx = 0; dx < size; dx++) {
			// distance from the corner along the diagonal
			const int32 step = dx + dy;
			if (dx == 0 || dy == 0 || step < 3)
				continue;
			const int32 phase = (step - 3) % 4;
			if (step - 3 >= 12 || phase == 3)
				continue;
			const BPoint point(grip.right - dx + 1, grip.bottom - dy + 1);
			if (!grip.Contains(point))
				continue;
			// counted from the corner out: grey, grey, white, a gap
			canvas.StrokePoint(point,
				phase == 2 ? colors.frameLight : colors.frameShadow);
		}
	}
}


/*!	The classic window frame: a light grey line and a white one along the
	top and left, a mid and a dark grey one along the bottom and right, and
	the frame colour inside them, including the line under the title bar.
*/
static void
reactionary_draw_frame(decor_window* styleWindow, decor_canvas* styleCanvas,
	decor_rect invalid)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const BRect rect = R(invalid);
	if (window.Look() == B_NO_BORDER_WINDOW_LOOK || window.BorderWidth() <= 0
		|| !rect.IsValid())
		return;

	const Colors colors(canvas);
	if (window.Look() == B_BORDERED_WINDOW_LOOK) {
		// Menus and tool tips. A menu's raised edge is two pixels: this
		// outer one, and the inner one the control look draws.
		draw_bevel(canvas, window.BorderRect(), colors.frameEdge,
			colors.frameDarkShadow);
		return;
	}

	BRect outer = window.BorderRect();
	draw_bevel(canvas, outer, colors.frameFace, colors.frameDarkShadow);
	outer.InsetBy(1, 1);
	draw_bevel(canvas, outer, colors.frameLight, colors.frameShadow);
	outer.InsetBy(1, 1);

	BRegion face(outer);
	face.Exclude(window.Frame());
	if (window.TitleBarRect().IsValid())
		face.Exclude(window.TitleBarRect());
	for (int32 i = 0; i < face.CountRects(); i++)
		canvas.FillRect(face.RectAt(i), colors.frameFace);

	if (has_resize_grip(window))
		draw_resize_grip(window, canvas, colors, rect);
}


/*!	A BeOS-style tab's frame: the window frame's bevels up its sides and over
	the top, open at the bottom to join the frame below.
*/
static void
draw_tab_frame(Canvas& canvas, const Colors& colors, BRect rect)
{
	const BPoint bottomLeft = rect.LeftBottom();
	const BPoint bottomRight = rect.RightBottom();
	canvas.FillRect(rect, colors.frameFace);
	canvas.StrokeLine(bottomLeft, rect.LeftTop(), colors.frameFace);
	canvas.StrokeLine(rect.LeftTop(), rect.RightTop(), colors.frameFace);
	canvas.StrokeLine(rect.RightTop(), bottomRight, colors.frameDarkShadow);
	canvas.StrokeLine(bottomLeft + BPoint(1, 0), rect.LeftTop() + BPoint(1, 1),
		colors.frameLight);
	canvas.StrokeLine(rect.LeftTop() + BPoint(1, 1),
		rect.RightTop() + BPoint(-1, 1), colors.frameLight);
	canvas.StrokeLine(rect.RightTop() + BPoint(-1, 1),
		bottomRight + BPoint(-1, 0), colors.frameShadow);
}


/*!	A title bar button: the classic raised bevel on the button face, which
	turns over when pressed, moving the glyph one pixel down and right. The
	glyph is a 16 x 14 bitmap, drawn at whole multiples for large fonts.
*/
static void
draw_button(Canvas& canvas, BRect rect, bool pressed,
	const char* const* glyph)
{
	if (!rect.IsValid())
		return;

	BRect bevel(rect);
	if (pressed) {
		draw_bevel(canvas, bevel, kDarkShadow, kLight);
		bevel.InsetBy(1, 1);
		draw_bevel(canvas, bevel, kShadow, kButtonFace);
	} else {
		draw_bevel(canvas, bevel, kLight, kDarkShadow);
		bevel.InsetBy(1, 1);
		draw_bevel(canvas, bevel, kButtonFace, kShadow);
	}
	bevel.InsetBy(1, 1);
	canvas.FillRect(bevel, kButtonFace);

	const float width = rect.Width() + 1;
	const float height = rect.Height() + 1;
	const float pixel = std::max(1.0f, floorf(std::min(width / kGlyphWidth,
		height / kGlyphHeight)));
	BPoint origin(
		rect.left + floorf((width - kGlyphWidth * pixel) / 2),
		rect.top + floorf((height - kGlyphHeight * pixel) / 2));
	if (pressed)
		origin += BPoint(1, 1);

	for (int32 y = 0; y < kGlyphHeight; y++) {
		for (int32 x = 0; x < kGlyphWidth; x++) {
			if (glyph[y][x] != 'X')
				continue;
			const float left = origin.x + x * pixel;
			const float top = origin.y + y * pixel;
			BRect dot(left, top, left + pixel - 1, top + pixel - 1);
			if (bevel.Contains(dot))
				canvas.FillRect(dot, kBlack);
		}
	}
}


static const char* const*
glyph_of(int32 button)
{
	switch (button) {
		case DECOR_BUTTON_CLOSE:
			return kCloseGlyph;
		case DECOR_BUTTON_ZOOM:
			return kZoomGlyph;
		default:
			return kMinimizeGlyph;
	}
}


static void
draw_title(const Window& window, const decor_tab& tab, Canvas& canvas,
	const Colors& colors)
{
	const Geometry& geometry = geometry_of(window);
	const BRect tabRect = title_rect(tab, geometry);
	float left, right;
	title_span(tab, geometry, left, right);

	font_height fontHeight;
	canvas.GetFontHeight(fontHeight);

	BPoint position(left, floorf((tabRect.top + tabRect.bottom + 1
		+ fontHeight.ascent - fontHeight.descent) / 2));

	canvas.SetDrawingMode(B_OP_OVER);
	canvas.DrawString(tab.shown_title, tab.shown_title_length, position,
		tab.focused ? colors.focusText : colors.nonFocusText);
	canvas.SetDrawingMode(B_OP_COPY);
}


/*!	A tab is its share of the title bar, shading from left to right. Tabs of
	a stack are kept apart by a line of the frame colour.
*/
static void
reactionary_draw_tab(decor_window* styleWindow, decor_canvas* styleCanvas,
	int32 index, decor_rect invalidRect)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const decor_tab& tab = window.TabAt(index);
	const BRect tabRect = TabRect(tab);
	const BRect invalid = R(invalidRect);
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	const Colors colors(canvas);
	const bool focus = tab.focused;
	if (sBeOSTabs)
		draw_tab_frame(canvas, colors, tabRect);

	BRect bar = title_rect(tab, geometry_of(window));
	if (index != 0) {
		canvas.StrokeLine(bar.LeftTop(), bar.LeftBottom(), colors.frameFace);
		bar.left++;
	}

	Gradient gradient;
	gradient.AddColor(focus ? colors.focusTab : colors.nonFocusTab, 0);
	gradient.AddColor(focus ? kActiveGradientEnd : colors.inactiveGradientEnd,
		255);
	gradient.SetStart(bar.LeftTop());
	gradient.SetEnd(bar.RightTop());
	canvas.FillRect(bar & invalid, gradient);

	draw_title(window, tab, canvas, colors);

	const int32 order[3] = { DECOR_BUTTON_MINIMIZE, DECOR_BUTTON_ZOOM,
		DECOR_BUTTON_CLOSE };
	for (int32 i = 0; i < 3; i++) {
		const BRect button = ButtonRect(tab, order[i]);
		if (invalid.Intersects(button)) {
			draw_button(canvas, button, tab.pressed[order[i]],
				glyph_of(order[i]));
		}
	}
}


static void
reactionary_draw_button(decor_window* styleWindow, decor_canvas* styleCanvas,
	int32 index, int32 button, decor_rect invalid)
{
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	const decor_tab& tab = window.TabAt(index);
	draw_button(canvas, ButtonRect(tab, button), tab.pressed[button],
		glyph_of(button));
}


static void
reactionary_update_font(decor_window* styleWindow, decor_canvas* styleCanvas)
{
	// bold titles, as in the theme; tool windows get the plain font
	Window window(styleWindow);
	Canvas canvas(styleCanvas);
	canvas.SetFont(window.CountTabs() > 0 && is_small(window)
		? DECOR_FONT_PLAIN : DECOR_FONT_BOLD, 0, B_FORCE_ANTIALIASING);
}


static status_t
reactionary_create_window(decor_window* window, decor_canvas* canvas)
{
	WindowState* state = new(std::nothrow) WindowState;
	if (state == NULL)
		return B_NO_MEMORY;
	memset(state, 0, sizeof(WindowState));
	window->cookie = state;
	return B_OK;
}


static void
reactionary_delete_window(decor_window* window)
{
	delete static_cast<WindowState*>(window->cookie);
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
	sStyle.name = "Reactionary";
	sStyle.create_window = &reactionary_create_window;
	sStyle.delete_window = &reactionary_delete_window;
	sStyle.update_font = &reactionary_update_font;
	sStyle.layout = &reactionary_layout;
	sStyle.draw_frame = &reactionary_draw_frame;
	sStyle.draw_tab = &reactionary_draw_tab;
	sStyle.draw_button = &reactionary_draw_button;
	sStyle.region_at = &reactionary_region_at;
	sStyle.footprint = &reactionary_footprint;
	return &sStyle;
}
