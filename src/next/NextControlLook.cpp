/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See NextControlLook.h. Colours and sizes were measured from the pictures of
 * Swizcore Studio's NeXT 2.0 (2004).
 */


#include "NextControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <InterfaceDefs.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);


/*!	A raised square's greys: its top and left edge, its face, the grey edge
	inside its bottom and right, and the black one outside that.
*/
struct Bevel {
	uint8	light;
	uint8	face;
	uint8	shade;
	uint8	edge;
};

static const Bevel kButton = { 0xff, 0xc0, 0x80, 0x00 };
static const Bevel kButtonDisabled = { 0xe0, 0xc0, 0xa0, 0x80 };
// the scroll bar's thumb and arrows, and the slider's knob
static const Bevel kScroller = { 0xf9, 0xc0, 0x5f, 0x00 };
static const Bevel kKnob = { 0xff, 0xbf, 0x7f, 0x00 };
static const Bevel kHeader = { 0xff, 0xc0, 0x80, 0x80 };

// check boxes: a white top and left, a face and a grey bottom and right, with
// their corners cut
static const uint8 kBoxLight = 0xff;
static const uint8 kBoxFace = 0xc6;
static const uint8 kBoxShade = 0x81;
static const uint8 kBoxDisabledFace = 0xc0;
static const uint8 kBoxDisabledShade = 0xa0;
static const uint8 kBoxDisabledMark = 0x80;

static const uint8 kRadioFace = 0xc5;

// stippled tracks: two greys in a checkerboard
static const pattern kStipple = { { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,
	0x55 } };
static const rgb_color kTrackDark = GREY(0x80);
static const rgb_color kTrackLight = GREY(0xc7);
static const rgb_color kWellDark = GREY(0x7f);
static const rgb_color kWellLight = GREY(0xbd);
static const rgb_color kGrooveDark = GREY(0x81);
static const rgb_color kGrooveLight = GREY(0xc8);
static const rgb_color kScrollBorder = GREY(0x55);
static const rgb_color kArrowDisabled = GREY(0x80);

// a dimple in a scroll bar's thumb, a pit lit from the bottom right
static const uint8 kDimple[4][4] = {
	{ 0x6d, 0x1e, 0x0c, 0x4d },
	{ 0x19, 0x57, 0x92, 0x94 },
	{ 0x27, 0x9c, 0xce, 0xfb },
	{ 0x4c, 0xa1, 0xff, 0xf0 }
};

// text fields: sunk two deep
static const rgb_color kFieldOuterTop = GREY(0x55);
static const rgb_color kFieldInnerTop = GREY(0x00);
static const rgb_color kFieldOuterBottom = GREY(0xff);
static const rgb_color kFieldInnerBottom = GREY(0xaa);

static const rgb_color kGroupDark = GREY(0x80);
static const rgb_color kGroupLight = GREY(0xff);
static const rgb_color kMenuEdge = GREY(0x00);
static const rgb_color kMenuLight = GREY(0xff);

// a tab behind the chosen one: darker
static const uint8 kBackTabFace = 0xaa;

static const float kIndicatorWidth = 20;


static rgb_color
mix(rgb_color a, rgb_color b, float amount)
{
	rgb_color result;
	result.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	result.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	result.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	result.alpha = 255;
	return result;
}


static rgb_color
grey(uint8 level)
{
	rgb_color color = { level, level, level, 255 };
	return color;
}


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	rgb_color color)
{
	if (left > right || top > bottom)
		return;
	view->SetHighColor(color);
	view->FillRect(BRect(left, top, right, bottom));
}


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	uint8 level)
{
	fill_line(view, left, top, right, bottom, grey(level));
}


//!	A checkerboard of two greys.
static void
fill_stipple(BView* view, const BRect& r, rgb_color dark, rgb_color light)
{
	if (!r.IsValid())
		return;
	view->SetHighColor(dark);
	view->SetLowColor(light);
	view->FillRect(r, kStipple);
}


/*!	A raised square: a light top and left, the grey and black edges along its
	bottom and right, and its face. Its top right and bottom left corners belong
	to the black edge's row and column.
*/
static void
draw_raised(BView* view, BRect r, const Bevel& bevel, bool fill = true)
{
	if (r.Width() < 2 || r.Height() < 2)
		return;

	const float l = r.left, t = r.top, rr = r.right, b = r.bottom;
	if (fill)
		fill_line(view, l + 1, t + 1, rr - 2, b - 2, bevel.face);
	fill_line(view, l, t, rr - 1, t, bevel.light);
	fill_line(view, l, t + 1, l, b - 1, bevel.light);
	fill_line(view, l + 1, b - 1, rr - 1, b - 1, bevel.shade);
	fill_line(view, rr - 1, t + 1, rr - 1, b - 1, bevel.shade);
	fill_line(view, l, b, rr, b, bevel.edge);
	fill_line(view, rr, t, rr, b, bevel.edge);
}


/*!	A pressed square: black along its top and left, and white elsewhere, as if
	lit from the other side.
*/
static void
draw_pressed(BView* view, BRect r, bool fill = true)
{
	if (r.Width() < 2 || r.Height() < 2)
		return;

	if (fill)
		fill_line(view, r.left, r.top, r.right, r.bottom, kWhite);
	else {
		fill_line(view, r.left, r.bottom, r.right, r.bottom, kWhite);
		fill_line(view, r.right, r.top, r.right, r.bottom, kWhite);
	}
	fill_line(view, r.left, r.top, r.right - 1, r.top, kBlack);
	fill_line(view, r.left, r.top, r.left, r.bottom - 1, kBlack);
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED | BControlLook::B_CLICKED)) != 0
		&& (flags & BControlLook::B_DISABLED) == 0;
}


//!	A button's bevel, raised or pressed, all of it or only its edges.
static void
draw_button(BView* view, BRect r, uint32 flags, bool fill = true)
{
	if (is_pressed(flags))
		draw_pressed(view, r, fill);
	else {
		draw_raised(view, r, (flags & BControlLook::B_DISABLED) != 0
			? kButtonDisabled : kButton, fill);
	}
}


/*!	A small solid triangle, its point towards direction, half wide across and
	tall along.
*/
static void
draw_triangle(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color, float half, float tall)
{
	const float x = floorf((rect.left + rect.right) / 2) + 0.5f;
	const float y = floorf((rect.top + rect.bottom) / 2) + 0.5f;
	const float base = tall / 2;
	BPoint points[3];
	switch (direction) {
		case BControlLook::B_UP_ARROW:
			points[0] = BPoint(x - half, y + base);
			points[1] = BPoint(x + half, y + base);
			points[2] = BPoint(x, y - base);
			break;
		case BControlLook::B_DOWN_ARROW:
			points[0] = BPoint(x - half, y - base);
			points[1] = BPoint(x + half, y - base);
			points[2] = BPoint(x, y + base);
			break;
		case BControlLook::B_LEFT_ARROW:
			points[0] = BPoint(x + base, y - half);
			points[1] = BPoint(x + base, y + half);
			points[2] = BPoint(x - base, y);
			break;
		default:
			points[0] = BPoint(x - base, y - half);
			points[1] = BPoint(x - base, y + half);
			points[2] = BPoint(x + base, y);
			break;
	}
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(color);
	view->FillTriangle(points[0], points[1], points[2]);
	view->PopState();
}


//!	A dimple with its top left corner at x, y.
static void
draw_dimple(BView* view, float x, float y)
{
	for (int32 row = 0; row < 4; row++) {
		for (int32 column = 0; column < 4; column++) {
			fill_line(view, x + column, y + row, x + column, y + row,
				kDimple[row][column]);
		}
	}
}


/*!	A pop-up menu's mark: a small raised bar with a grey shadow two pixels
	down and to the right of it.
*/
static void
draw_popup_mark(BView* view, const BRect& area, bool disabled)
{
	const float width = 9;
	const float height = 6;
	const float left = floorf((area.left + area.right - width - 2) / 2) + 1;
	const float top = floorf((area.top + area.bottom - height - 2) / 2);
	const BRect bar(left, top, left + width - 1, top + height - 1);
	const rgb_color shadow = disabled ? grey(0xa0) : grey(0x80);
	fill_line(view, bar.left + 2, bar.bottom + 1, bar.right + 2, bar.bottom + 2,
		shadow);
	fill_line(view, bar.right + 1, bar.top + 2, bar.right + 2, bar.bottom + 2,
		shadow);
	const Bevel bevel = disabled ? kButtonDisabled
		: Bevel{ 0xff, 0xc0, 0x7f, 0x00 };
	draw_raised(view, bar, bevel);
}


//	#pragma mark -


NextControlLook::NextControlLook()
{
}


NextControlLook::~NextControlLook()
{
}


//	#pragma mark - buttons


/*!	The whole bevel, so that its face fills the gap Haiku leaves inside the
	frame; the background draws the face again. A default button has a black
	line round it.
*/
void
NextControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 3 : 2;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	BRect shape(rect);
	if (isDefault) {
		view->SetHighColor((flags & B_DISABLED) != 0 ? grey(0x80) : kBlack);
		view->StrokeRect(shape);
		shape.InsetBy(1, 1);
	}
	draw_button(view, shape, flags);

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
NextControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(1, 1);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	// Tracker's column headers are drawn as a button's background; the view's
	// name is the only thing that tells them from any other button
	if (view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0) {
		if ((flags & B_ACTIVATED) != 0)
			draw_pressed(view, rect);
		else
			draw_raised(view, rect, kHeader);
		view->PopState();
		return;
	}

	// the face inside the frame's edges
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		is_pressed(flags) ? grey(0xff)
			: grey((flags & B_DISABLED) != 0 ? kButtonDisabled.face
				: kButton.face));

	view->PopState();

	rect.InsetBy(1, 1);
}


void
NextControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_popup_mark(view, BRect(area.right - 15, area.top, area.right - 2,
		area.bottom), (flags & B_DISABLED) != 0);
	view->PopState();
}


//	#pragma mark - menus


//!	A grey bar, raised: white along its top and black along its bottom.
void
NextControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		view->PopState();
		return;
	}

	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	fill_line(view, rect.left, rect.top, rect.right, rect.top, kMenuLight);
	if ((borders & B_BOTTOM_BORDER) != 0) {
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kMenuEdge);
	}

	view->PopState();
}


/*!	A button's bevel. Haiku draws the field's background in its menu bar, a
	view two pixels inside this rectangle, so the edges have to be drawn here.
*/
void
NextControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		// an open menu leaves its field raised, as NeXTSTEP's did
		draw_button(view, rect, flags & ~(B_ACTIVATED | B_CLICKED));
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


//!	The button's face, with the pop-up mark at its right end.
void
NextControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	// the face runs out under the frame's edges, which are drawn round it
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		disabled ? kButtonDisabled.face : kButton.face);

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 3;
		draw_popup_mark(view, indicator, disabled);
		rect.right = indicator.left - 1;
	}

	view->PopState();
}


void
NextControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


//!	A grey menu with a black line round it, raised inside that.
void
NextControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if ((borders & B_LEFT_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kMenuEdge);
	if ((borders & B_RIGHT_BORDER) != 0)
		fill_line(view, rect.right, rect.top, rect.right, rect.bottom, kMenuEdge);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kMenuEdge);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kMenuEdge);
	view->PopState();

	if ((borders & B_LEFT_BORDER) != 0)
		rect.left++;
	if ((borders & B_RIGHT_BORDER) != 0)
		rect.right--;
	if ((borders & B_TOP_BORDER) != 0)
		rect.top++;
	if ((borders & B_BOTTOM_BORDER) != 0)
		rect.bottom--;
}


/*!	A selected item is the menu selection colour, white in NeXT's theme, flat.
	The others are raised a little, a white line along their top and a grey one
	along their bottom, as NeXTSTEP's menu items were.
*/
void
NextControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		selected ? ui_color(B_MENU_SELECTED_BACKGROUND_COLOR) : base);
	view->PopState();
}


//	#pragma mark - status bars


/*!	A track stippled in two greys, sunk into the panel, and in it a bar in the
	status bar colour with a light line along its top and a grey one along its
	bottom.
*/
void
NextControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	fill_line(view, rect.left, rect.top, rect.right, rect.top, kBlack);
	fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kBlack);
	fill_line(view, rect.left + 1, rect.bottom, rect.right, rect.bottom, kWhite);
	fill_line(view, rect.right, rect.top + 1, rect.right, rect.bottom, kWhite);
	const BRect well(rect.left + 1, rect.top + 1, rect.right - 1,
		rect.bottom - 1);
	fill_stipple(view, well, kWellDark, kWellLight);

	if (progressPosition > well.left) {
		BRect bar(well);
		bar.right = std::min(progressPosition, well.right);
		fill_line(view, bar.left, bar.top, bar.right, bar.bottom, barColor);
		if (bar.Height() >= 4) {
			fill_line(view, bar.left, bar.top, bar.right, bar.top,
				mix(barColor, kWhite, 0.745f));
			fill_line(view, bar.left, bar.bottom, bar.right, bar.bottom,
				mix(barColor, kWhite, 0.5f));
		}
	}

	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	A square with its corners cut, white along its top and left and grey along
	its bottom and right. Its tick is black with a white line beside it, and
	pressed, the box is white with black along its top and left.
*/
void
NextControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0 && !disabled;

	view->PushState();

	BRect r(rect);
	r.bottom = r.top + r.Width();
	const float l = r.left, t = r.top, rr = r.right, b = r.bottom;
	const uint8 light = down ? 0x00 : kBoxLight;
	const uint8 face = down ? 0xff : disabled ? kBoxDisabledFace : kBoxFace;
	const uint8 shade = down ? 0xff : disabled ? kBoxDisabledShade : kBoxShade;
	fill_line(view, l + 1, t + 1, rr - 1, b - 1, face);
	fill_line(view, l + 1, t, rr - 1, t, light);
	fill_line(view, l, t + 1, l, b - 1, light);
	fill_line(view, l + 1, b, rr - 1, b, shade);
	fill_line(view, rr, t + 1, rr, b - 1, shade);

	// in a 14 pixel box, the tick's foot is three in from the left and three up
	// from the bottom, and its top three in from the right and the top
	const float size = r.Width() + 1;
	const float unit = size / 14;
	const rgb_color mark = disabled ? grey(kBoxDisabledMark) : kBlack;
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf(t + size / 2) - 1;
		fill_line(view, l + roundf(2 * unit), middle, rr - roundf(2 * unit),
			middle, disabled ? face : 0xff);
		fill_line(view, l + roundf(2 * unit), middle + 1, rr - roundf(2 * unit),
			middle + 1, disabled ? grey(kBoxDisabledMark) : grey(kBoxShade));
	} else if ((flags & B_ACTIVATED) != 0) {
		const BPoint foot(l + roundf(3 * unit), b - roundf(3 * unit));
		const BPoint top(rr - roundf(3 * unit), t + roundf(3 * unit));
		const BPoint stem(foot.x, t + roundf(6 * unit));
		// the white beside each stroke, then the black
		if (!disabled) {
			view->SetHighColor(kWhite);
			view->StrokeLine(foot + BPoint(-1, 0), top + BPoint(-1, 0));
			view->StrokeLine(stem + BPoint(-1, 0), foot + BPoint(-1, 0));
		}
		view->SetHighColor(mark);
		view->StrokeLine(foot, top);
		view->StrokeLine(stem, foot + BPoint(0, -1));
	}
	view->PopState();
}


/*!	A grey disc with a black rim round its top left half. When it's on, a white
	disc fills it; pressed, it is white.
*/
void
NextControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0 && !disabled;

	BRect ball(rect);
	ball.bottom = ball.top + ball.Width();

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	view->SetHighColor(down ? kWhite : grey(disabled ? kBoxDisabledFace
		: kRadioFace));
	view->FillEllipse(ball);

	view->SetHighColor(disabled ? grey(0x80) : kBlack);
	view->SetPenSize(1.5f);
	view->StrokeArc(ball.InsetByCopy(0.5f, 0.5f), 45, 180);
	if (!disabled) {
		view->SetHighColor(kWhite);
		view->SetPenSize(1);
		view->StrokeArc(ball.InsetByCopy(0.5f, 0.5f), 225, 180);
	}

	if ((flags & B_ACTIVATED) != 0) {
		view->SetHighColor(disabled ? grey(0xe0) : kWhite);
		const float inset = roundf((ball.Width() + 1) * 0.15f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
	}
	view->PopState();
}


//	#pragma mark - scroll bars


void
NextControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kScrollBorder);
	view->StrokeRect(rect);
	view->PopState();
}


//!	A raised grey square with a black arrow; pressed, white with black.
void
NextControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	if (down && !disabled)
		draw_pressed(view, rect);
	else
		draw_raised(view, rect, kScroller);

	BRect glyph(rect);
	glyph.right -= 1;
	glyph.bottom -= 1;
	draw_triangle(view, glyph, direction, disabled ? kArrowDisabled : kBlack,
		4, 7);

	view->PopState();
}


void
NextControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


//!	A track stippled in two greys.
void
NextControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_stipple(view, rect, kTrackDark, kTrackLight);
	view->PopState();
}


/*!	A raised grey thumb with two dimples across its middle. A bar with nothing
	to scroll has none.
*/
void
NextControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	if ((flags & B_DISABLED) != 0) {
		BRect track(rect);
		DrawScrollBarBackground(view, track, updateRect, base, flags,
			orientation);
		view->PopState();
		return;
	}

	const BRect& t = rect;
	draw_raised(view, t, kScroller);

	// the dimples sit in the face, between the light edge and the two along
	// the bottom and right, three apart along the thumb
	const float across = orientation == B_VERTICAL
		? floorf((t.left + 1 + t.right - 2 - 3) / 2)
		: floorf((t.top + 1 + t.bottom - 2 - 3) / 2);
	const float along = orientation == B_VERTICAL ? t.Height() : t.Width();
	const float start = orientation == B_VERTICAL ? t.top : t.left;
	if (along >= 15) {
		const bool two = along >= 20;
		const float span = two ? 11 : 4;
		const float first = floorf(start + (along - 1 - span) / 2);
		for (int32 i = 0; i < (two ? 2 : 1); i++) {
			const float position = first + i * 7;
			if (orientation == B_VERTICAL)
				draw_dimple(view, across, position);
			else
				draw_dimple(view, position, across);
		}
	}

	view->PopState();
}


float
NextControlLook::GetScrollBarWidth(orientation orientation)
{
	return 15;
}


//	#pragma mark - sliders


//!	A groove stippled in two greys, black along its top and left.
void
NextControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color dark = disabled ? grey(0x80) : kBlack;
	fill_line(view, rect.left, rect.top, rect.right, rect.top, dark);
	fill_line(view, rect.left, rect.top, rect.left, rect.bottom, dark);
	fill_line(view, rect.left + 1, rect.bottom, rect.right, rect.bottom, kWhite);
	fill_line(view, rect.right, rect.top + 1, rect.right, rect.bottom, kWhite);
	const BRect inside(rect.left + 1, rect.top + 1, rect.right - 1,
		rect.bottom - 1);

	// an application's fill colour, when it has one; Haiku passes a grey tint
	// of the panel for the part with none
	const bool isGrey = fillColor.red == fillColor.green
		&& fillColor.green == fillColor.blue;
	if (!isGrey && !disabled) {
		view->SetHighColor(fillColor);
		view->FillRect(inside);
	} else {
		fill_stipple(view, inside, disabled ? grey(0xa0) : kGrooveDark,
			kGrooveLight);
	}

	view->PopState();
}


//!	A square raised knob inside a black line; pressed, white with black.
void
NextControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->SetHighColor(disabled ? grey(0x80) : kBlack);
	view->StrokeRect(rect);
	const BRect knob(rect.InsetByCopy(1, 1));
	if ((flags & B_ACTIVATED) != 0 && !disabled)
		draw_pressed(view, knob);
	else {
		const Bevel bevel = disabled
			? Bevel{ 0xe0, 0xc0, 0xa0, 0xa0 } : kKnob;
		// the knob's grey edge is all its own: the black line round it is
		// already there
		fill_line(view, knob.left, knob.top, knob.right, knob.bottom,
			bevel.face);
		fill_line(view, knob.left, knob.top, knob.right - 1, knob.top,
			bevel.light);
		fill_line(view, knob.left, knob.top, knob.left, knob.bottom - 1,
			bevel.light);
		fill_line(view, knob.left + 1, knob.bottom, knob.right, knob.bottom,
			bevel.shade);
		fill_line(view, knob.right, knob.top, knob.right, knob.bottom,
			bevel.shade);
	}
	view->PopState();
}


//!	A pointer, grey with a black line round it, its tip towards the marks.
void
NextControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	BRect r(rect);
	r.bottom--;
	BPoint points[5];
	if (orientation == B_HORIZONTAL) {
		const float tip = std::min(6.0f, floorf(r.Width() / 2));
		points[0] = BPoint(r.left, r.top);
		points[1] = BPoint(r.right, r.top);
		points[2] = BPoint(r.right, r.bottom - tip);
		points[3] = BPoint((r.left + r.right) / 2, r.bottom);
		points[4] = BPoint(r.left, r.bottom - tip);
	} else {
		const float tip = std::min(6.0f, floorf(r.Height() / 2));
		points[0] = BPoint(r.left, r.top);
		points[1] = BPoint(r.left, r.bottom);
		points[2] = BPoint(r.right - tip, r.bottom);
		points[3] = BPoint(r.right, (r.top + r.bottom) / 2);
		points[4] = BPoint(r.right - tip, r.top);
	}

	view->SetHighColor(grey((flags & B_ACTIVATED) != 0 && !disabled
		? 0xff : kKnob.face));
	view->FillPolygon(points, 5);
	view->SetHighColor(disabled ? grey(0x80) : kBlack);
	view->StrokePolygon(points, 5);

	view->PopState();
}


//	#pragma mark - text controls


/*!	Sunk into the panel two deep: dark grey and black lines along the top and
	left, white and light grey ones along the bottom and right. With the focus,
	the inner line is the keyboard navigation colour.
*/
void
NextControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;
	const float lighter = disabled ? 0.5f : 0;
	const BRect o(rect);
	const BRect i(rect.InsetByCopy(1, 1));
	const rgb_color outerTop = mix(kFieldOuterTop, base, lighter);
	const rgb_color outerBottom = mix(kFieldOuterBottom, base, lighter);
	fill_line(view, o.left, o.top, o.right, o.top, outerTop);
	fill_line(view, o.left, o.top, o.left, o.bottom, outerTop);
	fill_line(view, o.left + 1, o.bottom, o.right, o.bottom, outerBottom);
	fill_line(view, o.right, o.top + 1, o.right, o.bottom, outerBottom);

	if ((flags & B_FOCUSED) != 0 && !disabled) {
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		view->StrokeRect(i);
	} else {
		const rgb_color innerTop = mix(kFieldInnerTop, base, lighter);
		const rgb_color innerBottom = mix(kFieldInnerBottom, base, lighter);
		fill_line(view, i.left, i.top, i.right, i.top, innerTop);
		fill_line(view, i.left, i.top, i.left, i.bottom, innerTop);
		fill_line(view, i.left + 1, i.bottom, i.right, i.bottom, innerBottom);
		fill_line(view, i.right, i.top + 1, i.right, i.bottom, innerBottom);
	}

	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


//!	An etched line round the group: grey, with white beside it.
void
NextControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kGroupLight);
	view->StrokeRect(BRect(rect.left + 1, rect.top + 1, rect.right,
		rect.bottom));
	view->SetHighColor(kGroupDark);
	view->StrokeRect(BRect(rect.left, rect.top, rect.right - 1,
		rect.bottom - 1));
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - tabs


//!	The page's raised top edge, a white line that the tabs sit on.
void
NextControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawTabFrame(view, rect, updateRect, base, flags,
			borders, borderStyle, side);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if (borderStyle != B_NO_BORDER)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kWhite);
	view->PopState();
}


//!	A tab, raised like a button but open at the bottom, a black edge on its right.
static void
draw_tab(BView* view, const BRect& r, uint8 face)
{
	fill_line(view, r.left + 1, r.top + 1, r.right - 2, r.bottom, face);
	fill_line(view, r.left + 1, r.top, r.right - 2, r.top, kWhite);
	fill_line(view, r.left, r.top + 1, r.left, r.bottom, kWhite);
	fill_line(view, r.right - 1, r.top + 1, r.right - 1, r.bottom,
		grey(kButton.shade));
	fill_line(view, r.right, r.top + 2, r.right, r.bottom, kBlack);
	fill_line(view, r.right - 1, r.top, r.right - 1, r.top, kBlack);
}


void
NextControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawActiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, base.red);
		// open to the page: the face goes down over the page's line
		fill_line(view, rect.left + 1, rect.bottom, rect.right - 2, rect.bottom,
			base);
		view->PopState();
	}

	rect.top += 3;
	rect.bottom -= 1;
	rect.left += 4;
	rect.right -= 4;
}


//!	A tab behind the chosen one is lower and darker, and sits on the line.
void
NextControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawInactiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, BRect(rect.left, rect.top + 2, rect.right, rect.bottom),
			kBackTabFace);
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kWhite);
		view->PopState();
	}

	rect.top += 3;
	rect.bottom -= 1;
	rect.left += 4;
	rect.right -= 4;
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::NextControlLook();
}
