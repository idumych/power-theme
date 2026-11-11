/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See System7ControlLook.h. Colours and sizes were measured from screenshots of
 * Mac OS 7.5.5 with its standard window colour, on toastytech.com.
 */


#include "System7ControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Button.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }
#define RGB(hex) { (uint8)((hex) >> 16), (uint8)((hex) >> 8), (uint8)(hex), 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);
// what a disabled control's lines and marks are drawn in
static const rgb_color kDisabled = GREY(0x84);
// a scroll arrow when there is nothing to scroll
static const rgb_color kArrowDisabled = GREY(0x73);
static const rgb_color kPressedFace = GREY(0x00);

// the window colour, lavender: light, shaded and dark
static const rgb_color kLight = RGB(0xceceff);
static const rgb_color kShade = RGB(0xa5a5d6);

// scroll bars: a light track, stippled in grey when there is something to
// scroll, and light arrow boxes with hollow arrows
static const rgb_color kTrack = GREY(0xef);
static const rgb_color kTrackDark = GREY(0x9c);
static const rgb_color kArrowFace = GREY(0xef);
static const pattern kStipple = { { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,
	0x55 } };

// an etched line round a group: grey, with white beside it
static const rgb_color kGroupDark = GREY(0x84);

static const float kButtonRadius = 4;
static const float kDefaultRadius = 7;
static const float kIndicatorWidth = 19;


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	rgb_color color)
{
	if (left > right || top > bottom)
		return;
	view->SetHighColor(color);
	view->FillRect(BRect(left, top, right, bottom));
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED | BControlLook::B_CLICKED)) != 0
		&& (flags & BControlLook::B_DISABLED) == 0;
}


static rgb_color
line_color(uint32 flags)
{
	return (flags & BControlLook::B_DISABLED) != 0 ? kDisabled : kBlack;
}


/*!	A button: white, with a black outline whose corners are rounded. Pressed,
	it is black, and its label is drawn in white.
*/
static void
draw_button(BView* view, BRect r, uint32 flags, const rgb_color& background)
{
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(background);
	view->FillRect(r);
	view->SetHighColor(line_color(flags));
	view->FillRoundRect(r, kButtonRadius, kButtonRadius);
	view->SetHighColor(is_pressed(flags) ? kPressedFace : kWhite);
	view->FillRoundRect(r.InsetByCopy(1, 1), kButtonRadius - 1,
		kButtonRadius - 1);
	view->PopState();
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
		case BControlLook::B_LEFT_ARROW:
			points[0] = BPoint(x + base, y - half);
			points[1] = BPoint(x + base, y + half);
			points[2] = BPoint(x - base, y);
			break;
		case BControlLook::B_RIGHT_ARROW:
			points[0] = BPoint(x - base, y - half);
			points[1] = BPoint(x - base, y + half);
			points[2] = BPoint(x + base, y);
			break;
		default:
			points[0] = BPoint(x - half, y - base);
			points[1] = BPoint(x + half, y - base);
			points[2] = BPoint(x, y + base);
			break;
	}
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(color);
	view->FillTriangle(points[0], points[1], points[2]);
	view->PopState();
}


/*!	A scroll arrow as System 7 drew it, pixel by pixel: an arrowhead on a stem,
	hollow, twelve pixels wide and ten tall, pointing up; the other directions
	are turned from it. Pressed, it is filled in.
*/
static void
draw_scroll_arrow(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color, bool filled)
{
	// each row's outline, and the columns filled inside it, pointing up
	static const char* const kRows[] = {
		".....##.....",
		"....#oo#....",
		"...#oooo#...",
		"..#oooooo#..",
		".#oooooooo#.",
		"####oooo####",
		"...#oooo#...",
		"...#oooo#...",
		"...#oooo#...",
		"...######..."
	};
	const int32 rows = sizeof(kRows) / sizeof(kRows[0]);
	const int32 columns = 12;
	const bool across = direction == BControlLook::B_LEFT_ARROW
		|| direction == BControlLook::B_RIGHT_ARROW;
	const float width = across ? rows : columns;
	const float height = across ? columns : rows;
	const float left = floorf((rect.left + rect.right + 1 - width) / 2);
	const float top = floorf((rect.top + rect.bottom + 1 - height) / 2);

	view->PushState();
	view->SetHighColor(color);
	for (int32 row = 0; row < rows; row++) {
		for (int32 column = 0; column < columns; column++) {
			const char c = kRows[row][column];
			if (c == '.' || (c == 'o' && !filled))
				continue;
			float x, y;
			switch (direction) {
				case BControlLook::B_DOWN_ARROW:
					x = column; y = rows - 1 - row;
					break;
				case BControlLook::B_LEFT_ARROW:
					x = row; y = column;
					break;
				case BControlLook::B_RIGHT_ARROW:
					x = rows - 1 - row; y = column;
					break;
				default:
					x = column; y = row;
					break;
			}
			view->FillRect(BRect(left + x, top + y, left + x, top + y));
		}
	}
	view->PopState();
}


//	#pragma mark -


System7ControlLook::System7ControlLook()
{
}


System7ControlLook::~System7ControlLook()
{
}


//	#pragma mark - buttons


/*!	The whole button, so that its face fills the gap Haiku leaves inside the
	frame; the background draws the face again. A default button has a thick
	black ring round it, a pixel away, as System 7's had.
*/
void
System7ControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 5 : 2;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	BRect shape(rect);
	if (isDefault) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(background);
		view->FillRect(shape);
		view->SetHighColor(line_color(flags));
		view->FillRoundRect(shape, kDefaultRadius, kDefaultRadius);
		view->SetHighColor(background);
		view->FillRoundRect(shape.InsetByCopy(3, 3), kDefaultRadius - 3,
			kDefaultRadius - 3);
		shape.InsetBy(4, 4);
	}
	draw_button(view, shape, flags, background);

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
System7ControlLook::DrawButtonBackground(BView* view, BRect& rect,
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
		const bool active = (flags & B_ACTIVATED) != 0;
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			active ? kLight : kWhite);
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kBlack);
		view->PopState();
		return;
	}

	// the face inside the frame's outline
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		is_pressed(flags) ? kPressedFace : kWhite);

	view->PopState();

	rect.InsetBy(1, 1);
}


void
System7ControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_triangle(view, BRect(area.right - 12, area.top, area.right - 2,
		area.bottom), B_DOWN_ARROW,
		is_pressed(flags) ? kWhite : line_color(flags), 5, 5);
	view->PopState();
}


//!	A pressed button's label is white on its black face.
void
System7ControlLook::DrawLabel(BView* view, const char* label,
	const BBitmap* icon, BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, const BAlignment& alignment,
	const rgb_color* textColor)
{
	// BButton passes the control text colour; anything else is its own
	const rgb_color standard = ui_color(B_CONTROL_TEXT_COLOR);
	if ((textColor == NULL || (textColor->red == standard.red
			&& textColor->green == standard.green
			&& textColor->blue == standard.blue))
		&& is_pressed(flags) && dynamic_cast<BButton*>(view) != NULL) {
		ForwardingControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
			flags, alignment, &kWhite);
		return;
	}
	ForwardingControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);
}


//	#pragma mark - menus


//!	A white bar with a black line along its bottom.
void
System7ControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
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
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kBlack);

	view->PopState();
}


/*!	A white box with a black outline and a shadow a pixel wide along its right
	and bottom. Haiku draws the field's background in its menu bar, a view two
	pixels inside this rectangle, so the edges are drawn here.
*/
void
System7ControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		const rgb_color line = line_color(flags);
		BRect box(rect.left, rect.top, rect.right - 1, rect.bottom - 1);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			background);
		view->SetHighColor(line);
		view->StrokeRect(box);
		fill_line(view, box.left + 1, box.top + 1, box.right - 1,
			box.bottom - 1, kWhite);
		fill_line(view, rect.right, rect.top + 2, rect.right, rect.bottom, line);
		fill_line(view, rect.left + 2, rect.bottom, rect.right, rect.bottom,
			line);
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


//!	The box's white inside, with a black triangle pointing down at its right.
void
System7ControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	// out to the frame's outline, which is a pixel outside
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, kWhite);

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 3;
		draw_triangle(view, indicator, B_DOWN_ARROW, line_color(flags), 5, 5);
		rect.right = indicator.left - 1;
	}

	view->PopState();
}


void
System7ControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


//!	A white menu with a black line round it.
void
System7ControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if ((borders & B_LEFT_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kBlack);
	if ((borders & B_RIGHT_BORDER) != 0)
		fill_line(view, rect.right, rect.top, rect.right, rect.bottom, kBlack);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kBlack);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kBlack);
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


//!	A selected item is the menu selection colour, black in System 7, flat.
void
System7ControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
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


//!	A black outline round a white track, and in it a bar in the status bar colour.
void
System7ControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(kBlack);
	view->StrokeRect(rect);
	const BRect well(rect.InsetByCopy(1, 1));
	fill_line(view, well.left, well.top, well.right, well.bottom, kWhite);
	if (progressPosition > well.left) {
		fill_line(view, well.left, well.top,
			std::min(progressPosition, well.right), well.bottom, barColor);
	}
	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	A white square with a black outline, and a cross from corner to corner when
	it's on. Held down, the outline is drawn twice.
*/
void
System7ControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0 && !disabled;
	const rgb_color line = line_color(flags);

	view->PushState();

	BRect r(rect);
	r.bottom = r.top + r.Width();
	fill_line(view, r.left, r.top, r.right, r.bottom, kWhite);
	view->SetHighColor(line);
	view->StrokeRect(r);
	if (down)
		view->StrokeRect(r.InsetByCopy(1, 1));

	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf((r.top + r.bottom) / 2);
		fill_line(view, r.left + 3, middle, r.right - 3, middle, line);
	} else if ((flags & B_ACTIVATED) != 0) {
		view->StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.bottom));
		view->StrokeLine(BPoint(r.left, r.bottom), BPoint(r.right, r.top));
	}
	view->PopState();
}


/*!	A white circle with a black outline, and a black dot in it when it's on.
	Held down, the outline is drawn twice.
*/
void
System7ControlLook::DrawRadioButton(BView* view, BRect& rect,
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
	view->SetHighColor(line_color(flags));
	view->FillEllipse(ball);
	view->SetHighColor(kWhite);
	view->FillEllipse(ball.InsetByCopy(down ? 2 : 1, down ? 2 : 1));
	if ((flags & B_ACTIVATED) != 0) {
		view->SetHighColor(line_color(flags));
		const float inset = roundf((ball.Width() + 1) * 0.3f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
	}
	view->PopState();
}


//	#pragma mark - scroll bars


void
System7ControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kBlack);
	view->StrokeRect(rect);
	view->PopState();
}


/*!	A light box with a hollow arrow, in black, or grey when there is nothing to
	scroll; held down, the arrow is filled in. A black line parts it from the
	track.
*/
void
System7ControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, kArrowFace);
	switch (direction) {
		case B_UP_ARROW:
			fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
				kBlack);
			rect.bottom--;
			break;
		case B_DOWN_ARROW:
			fill_line(view, rect.left, rect.top, rect.right, rect.top, kBlack);
			rect.top++;
			break;
		case B_LEFT_ARROW:
			fill_line(view, rect.right, rect.top, rect.right, rect.bottom,
				kBlack);
			rect.right--;
			break;
		case B_RIGHT_ARROW:
			fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kBlack);
			rect.left++;
			break;
	}
	draw_scroll_arrow(view, rect, direction,
		disabled ? kArrowDisabled : kBlack, down && !disabled);

	view->PopState();
}


void
System7ControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


/*!	A track stippled in grey, or light and plain when there is nothing to
	scroll.
*/
void
System7ControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	if ((flags & B_DISABLED) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, kTrack);
	else {
		view->SetHighColor(kTrackDark);
		view->SetLowColor(kTrack);
		view->FillRect(rect, kStipple);
	}
	view->PopState();
}


/*!	A box in the window colour with a black outline, lit inside along its top
	and left and shaded along its bottom and right. A bar with nothing to scroll
	has none.
*/
void
System7ControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
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

	// the black lines along the track's sides are the bar's border; the
	// thumb has its own along its ends
	BRect t(rect);
	if (orientation == B_VERTICAL)
		t.InsetBy(-1, 0);
	else
		t.InsetBy(0, -1);
	view->SetHighColor(kBlack);
	view->StrokeRect(t);
	const BRect f(t.InsetByCopy(1, 1));
	fill_line(view, f.left, f.top, f.right, f.bottom, kLight);
	fill_line(view, f.left, f.top, f.right - 1, f.top, kWhite);
	fill_line(view, f.left, f.top, f.left, f.bottom - 1, kWhite);
	fill_line(view, f.left + 1, f.bottom, f.right, f.bottom, kShade);
	fill_line(view, f.right, f.top + 1, f.right, f.bottom, kShade);

	view->PopState();
}


float
System7ControlLook::GetScrollBarWidth(orientation orientation)
{
	return 15;
}


//	#pragma mark - sliders


//!	A thin white groove with a black outline, in the middle of what it's given.
void
System7ControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 2;
		bar.bottom = bar.top + 4;
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 2;
		bar.right = bar.left + 4;
	}
	view->SetHighColor(line_color(flags));
	view->StrokeRect(bar);
	// an application's fill colour, when it has one; Haiku passes a grey tint
	// of the panel for the part with none
	const bool isGrey = fillColor.red == fillColor.green
		&& fillColor.green == fillColor.blue;
	const BRect inside(bar.InsetByCopy(1, 1));
	fill_line(view, inside.left, inside.top, inside.right, inside.bottom,
		!isGrey && (flags & B_DISABLED) == 0 ? fillColor : kWhite);

	view->PopState();
}


//!	A white knob with a black outline and a line across its middle.
void
System7ControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const rgb_color line = line_color(flags);

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		(flags & B_ACTIVATED) != 0 ? kLight : kWhite);
	view->SetHighColor(line);
	view->StrokeRect(rect);
	if (orientation == B_HORIZONTAL) {
		const float x = floorf((rect.left + rect.right) / 2);
		fill_line(view, x, rect.top + 3, x, rect.bottom - 3, line);
	} else {
		const float y = floorf((rect.top + rect.bottom) / 2);
		fill_line(view, rect.left + 3, y, rect.right - 3, y, line);
	}
	view->PopState();
}


//!	A white pointer with a black outline, its tip towards the marks.
void
System7ControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

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
	view->SetHighColor((flags & B_ACTIVATED) != 0 ? kLight : kWhite);
	view->FillPolygon(points, 5);
	view->SetHighColor(line_color(flags));
	view->StrokePolygon(points, 5);
	view->PopState();
}


//	#pragma mark - text controls


/*!	A black outline round the white field, with a grey line inside it; with the
	focus, that line is the keyboard navigation colour.
*/
void
System7ControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();
	const bool disabled = (flags & B_DISABLED) != 0;
	view->SetHighColor(disabled ? kDisabled : kBlack);
	view->StrokeRect(rect);
	view->SetHighColor((flags & B_FOCUSED) != 0 && !disabled
		? ui_color(B_KEYBOARD_NAVIGATION_COLOR) : kWhite);
	view->StrokeRect(rect.InsetByCopy(1, 1));
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


//!	An etched line round the group: grey, with white beside it.
void
System7ControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		ForwardingControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kWhite);
	view->StrokeRect(BRect(rect.left + 1, rect.top + 1, rect.right,
		rect.bottom));
	view->SetHighColor(kGroupDark);
	view->StrokeRect(BRect(rect.left, rect.top, rect.right - 1,
		rect.bottom - 1));
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - tabs


//!	The page's edge on the tabs' side, as the rectangle's row or column.
static BRect
page_edge(const BRect& r, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			return BRect(r.left, r.top, r.right, r.top);
		case BTabView::kLeftSide:
			return BRect(r.right, r.top, r.right, r.bottom);
		case BTabView::kRightSide:
			return BRect(r.left, r.top, r.left, r.bottom);
		default:
			return BRect(r.left, r.bottom, r.right, r.bottom);
	}
}


/*!	A tab: a black outline round its face, with its corners rounded away from
	the page and open towards it. One behind the chosen tab is pushed two pixels
	back from the page, and sits on its line.
*/
static void
draw_tab(BView* view, const BRect& rect, uint32 side, const rgb_color& face,
	bool open)
{
	BRect r(rect);
	BRect shape(rect);
	const float back = open ? 0 : 2;
	switch (side) {
		case BTabView::kBottomSide:
			r.bottom -= back;
			shape = BRect(r.left, r.top - 4, r.right, r.bottom);
			break;
		case BTabView::kLeftSide:
			r.left += back;
			shape = BRect(r.left, r.top, r.right + 4, r.bottom);
			break;
		case BTabView::kRightSide:
			r.right -= back;
			shape = BRect(r.left - 4, r.top, r.right, r.bottom);
			break;
		default:
			r.top += back;
			shape = BRect(r.left, r.top, r.right, r.bottom + 4);
			break;
	}

	view->PushState();
	view->ClipToRect(rect);
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(kBlack);
	view->FillRoundRect(shape, kButtonRadius, kButtonRadius);
	view->SetHighColor(face);
	view->FillRoundRect(shape.InsetByCopy(1, 1), kButtonRadius - 1,
		kButtonRadius - 1);
	view->PopState();

	if (!open) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom, kBlack);
	}
}


//!	Where a tab's label goes: inside its outline, a little away from the page.
static void
inset_tab(BRect& rect, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			rect.top += 1;
			rect.bottom -= 3;
			rect.left += 4;
			rect.right -= 4;
			break;
		case BTabView::kLeftSide:
			rect.left += 3;
			rect.right -= 1;
			rect.top += 4;
			rect.bottom -= 4;
			break;
		case BTabView::kRightSide:
			rect.left += 1;
			rect.right -= 3;
			rect.top += 4;
			rect.bottom -= 4;
			break;
		default:
			rect.top += 3;
			rect.bottom -= 1;
			rect.left += 4;
			rect.right -= 4;
			break;
	}
}


//!	A black line along the page's edge that the tabs sit on.
void
System7ControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if (borderStyle != B_NO_BORDER) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom, kBlack);
	}
	view->PopState();
}


void
System7ControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, base, true);
		view->PopState();
	}

	inset_tab(rect, side);
}


void
System7ControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, kTrack, false);
		view->PopState();
	}

	inset_tab(rect, side);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::System7ControlLook();
}
