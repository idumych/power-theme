/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Control look after the Reactionary ME KDE theme: Qt's "Windows" style in
 * the theme's Windows ME colours. See ReactionaryControlLook.h.
 *
 * Every bevel is drawn with four fixed shades, as on Windows: white, the
 * face colour (the panel background), mid grey and dark grey. The face
 * follows the system colours; the shades don't.
 */


#include "ReactionaryControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>

#include <Bitmap.h>
#include <CheckBox.h>
#include <InterfaceDefs.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <Rect.h>
#include <ScrollView.h>
#include <String.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


static const rgb_color kWhite = { 255, 255, 255, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kShadow = { 128, 128, 128, 255 };
static const rgb_color kDarkShadow = { 64, 64, 64, 255 };

// Qt's "Windows" check mark, 7 x 7 pixels.
static const char* const kCheckMark[] = {
	"......X",
	".....XX",
	"X...XXX",
	"XX.XXX.",
	"XXXXX..",
	".XXX...",
	"..X...."
};

// The outline of a 12 pixel radio button, one span per row; the rings
// inside it are found by shrinking it (see DrawRadioButton()).
static const int8 kRadioSpans[12][2] = {
	{ 4, 7 }, { 2, 9 }, { 1, 10 }, { 1, 10 }, { 0, 11 }, { 0, 11 },
	{ 0, 11 }, { 0, 11 }, { 1, 10 }, { 1, 10 }, { 2, 9 }, { 4, 7 }
};


static rgb_color
face_color()
{
	return ui_color(B_PANEL_BACKGROUND_COLOR);
}


/*!	\a color, or the view's low colour (failing that, the face) where a
	control passes its transparent view colour.
*/
static rgb_color
solid(const rgb_color& color, BView* view = NULL)
{
	if (color != B_TRANSPARENT_COLOR && color.alpha != 0)
		return color;
	if (view != NULL && view->LowColor() != B_TRANSPARENT_COLOR
		&& view->LowColor().alpha != 0)
		return view->LowColor();
	return face_color();
}


static bool
is_flat(uint32 flags)
{
	return (flags & BControlLook::B_FLAT) != 0
		&& (flags & (BControlLook::B_ACTIVATED
			| BControlLook::B_PARTIALLY_ACTIVATED)) == 0
		&& ((flags & (BControlLook::B_HOVER | BControlLook::B_FOCUSED)) == 0
			|| (flags & BControlLook::B_DISABLED) != 0);
}


static void
inset_by_borders(BRect& rect, float inset, uint32 borders)
{
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		rect.left += inset;
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		rect.top += inset;
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		rect.right -= inset;
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		rect.bottom -= inset;
}


/*!	A one pixel bevel: \a topLeft along the top and left, \a bottomRight
	along the bottom and right, which own the two corners they share with
	the others. Only \a borders are drawn, and \a rect is inset past them.
*/
static void
draw_bevel(BView* view, BRect& rect, const rgb_color& topLeft,
	const rgb_color& bottomRight, uint32 borders = BControlLook::B_ALL_BORDERS)
{
	if (!rect.IsValid())
		return;

	const bool left = (borders & BControlLook::B_LEFT_BORDER) != 0;
	const bool top = (borders & BControlLook::B_TOP_BORDER) != 0;
	const bool right = (borders & BControlLook::B_RIGHT_BORDER) != 0;
	const bool bottom = (borders & BControlLook::B_BOTTOM_BORDER) != 0;

	view->BeginLineArray(4);
	if (top) {
		view->AddLine(rect.LeftTop(),
			BPoint(right ? rect.right - 1 : rect.right, rect.top), topLeft);
	}
	if (left) {
		view->AddLine(rect.LeftTop(),
			BPoint(rect.left, bottom ? rect.bottom - 1 : rect.bottom), topLeft);
	}
	if (right)
		view->AddLine(rect.RightTop(), rect.RightBottom(), bottomRight);
	if (bottom)
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), bottomRight);
	view->EndLineArray();

	inset_by_borders(rect, 1, borders);
}


//! A push button's bevel: white and dark grey outside, face and grey inside.
static void
draw_raised(BView* view, BRect& rect, const rgb_color& face,
	uint32 borders = BControlLook::B_ALL_BORDERS)
{
	draw_bevel(view, rect, kWhite, kDarkShadow, borders);
	draw_bevel(view, rect, face, kShadow, borders);
}


/*!	The bevel of windows, menus, scroll bar arrows and thumbs: face and dark
	grey outside, white and grey inside.
*/
static void
draw_raised_window(BView* view, BRect& rect, const rgb_color& face,
	uint32 borders = BControlLook::B_ALL_BORDERS)
{
	draw_bevel(view, rect, face, kDarkShadow, borders);
	draw_bevel(view, rect, kWhite, kShadow, borders);
}


//! A field's sunken bevel: grey and white outside, dark grey and face inside.
static void
draw_sunken(BView* view, BRect& rect, const rgb_color& face,
	uint32 borders = BControlLook::B_ALL_BORDERS)
{
	draw_bevel(view, rect, kShadow, kWhite, borders);
	draw_bevel(view, rect, kDarkShadow, face, borders);
}


//! Windows' dotted focus rectangle: every other pixel.
static void
draw_focus_rect(BView* view, BRect rect, const rgb_color& color)
{
	if (!rect.IsValid())
		return;

	view->PushState();
	view->SetHighColor(color);
	view->SetDrawingMode(B_OP_OVER);
	view->StrokeRect(rect, B_MIXED_COLORS);
	view->PopState();
}


static void
fill_pixel(BView* view, float x, float y, float size)
{
	view->FillRect(BRect(x, y, x + size - 1, y + size - 1));
}


/*!	A solid arrow, set pixel by pixel: rows of 7, 5, 3 and 1 pixels for a
	scroll bar's 16 pixel button, larger in larger rects.
*/
static void
draw_arrow(BView* view, BRect rect, uint32 direction, const rgb_color& color,
	int32 depth = 0)
{
	const bool horizontal = direction == BControlLook::B_LEFT_ARROW
		|| direction == BControlLook::B_RIGHT_ARROW;
	const float across = (horizontal ? rect.Height() : rect.Width()) + 1;
	const float along = (horizontal ? rect.Width() : rect.Height()) + 1;

	if (depth <= 0) {
		depth = (int32)std::max(2.0f,
			std::min(floorf((across + 2) / 4), along));
	}

	const int32 centerAcross = (int32)floorf(horizontal
		? (rect.top + rect.bottom) / 2 : (rect.left + rect.right) / 2);
	const int32 startAlong = (int32)floorf(horizontal
		? (rect.left + rect.right) / 2 : (rect.top + rect.bottom) / 2)
		- depth / 2;

	view->PushState();
	view->SetHighColor(color);
	view->SetDrawingMode(B_OP_COPY);
	for (int32 i = 0; i < depth; i++) {
		// i counts from the arrow's base to its point
		const int32 half = depth - 1 - i;
		int32 position;
		switch (direction) {
			case BControlLook::B_UP_ARROW:
			case BControlLook::B_LEFT_ARROW:
				position = startAlong + depth - 1 - i;
				break;
			default:
				position = startAlong + i;
				break;
		}
		if (horizontal) {
			view->FillRect(BRect(position, centerAcross - half, position,
				centerAcross + half));
		} else {
			view->FillRect(BRect(centerAcross - half, position,
				centerAcross + half, position));
		}
	}
	view->PopState();
}


/*!	An arrow in \a color, or engraved, white under grey, when \a disabled. */
static void
draw_control_arrow(BView* view, BRect rect, uint32 direction,
	const rgb_color& color, bool disabled, int32 depth = 0)
{
	if (disabled) {
		draw_arrow(view, rect.OffsetByCopy(1, 1), direction, kWhite, depth);
		draw_arrow(view, rect, direction, kShadow, depth);
	} else
		draw_arrow(view, rect, direction, color, depth);
}


//	#pragma mark -


ReactionaryControlLook::ReactionaryControlLook()
{
}


ReactionaryControlLook::~ReactionaryControlLook()
{
}


//	#pragma mark - buttons


void
ReactionaryControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
ReactionaryControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
ReactionaryControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
ReactionaryControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
ReactionaryControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
ReactionaryControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
ReactionaryControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


void
ReactionaryControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, float, const rgb_color& base,
	uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


void
ReactionaryControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


//	#pragma mark - menu fields and menus


void
ReactionaryControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	// a drop-down list: a sunken field round the menu bar
	if (!ShouldDraw(view, rect, updateRect))
		return;
	draw_sunken(view, rect, solid(background, view), borders);
}


void
ReactionaryControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
ReactionaryControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
ReactionaryControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
ReactionaryControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
ReactionaryControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
ReactionaryControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor((flags & B_DISABLED) != 0 ? face_color() : kWhite);
	view->FillRect(rect);
	view->PopState();
	inset_by_borders(rect, 1, borders);
}


void
ReactionaryControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// flat, in the face colour
	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);
	view->PopState();
	inset_by_borders(rect, 1, borders);
}


void
ReactionaryControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Menus are raised like windows, in two pixels: the window decorator
	// draws the outer one round the menu's window, this is the inner one.
	view->PushState();
	draw_bevel(view, rect, kWhite, kShadow, borders);
	view->SetHighColor(base);
	view->FillRect(rect);
	view->PopState();
}


void
ReactionaryControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if (dynamic_cast<BMenuBar*>(view) != NULL
		&& dynamic_cast<BMenuField*>(view->Parent()) == NULL) {
		// An open menu bar title sinks into the bar and keeps its colours.
		// The item draws its label in the view's colours right after this,
		// so they are set to the menu's normal ones on purpose.
		const rgb_color face = ui_color(B_MENU_BACKGROUND_COLOR);
		BRect inner(rect);
		draw_bevel(view, inner, kShadow, kWhite);
		view->SetHighColor(face);
		view->FillRect(inner);
		view->SetLowColor(face);
		view->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
		return;
	}

	// a bar in the selection colour, clear of the menu's raised edges
	view->PushState();
	BRect bar(rect);
	BRect bounds(view->Bounds());
	bar.left = std::max(bar.left, bounds.left + 2);
	bar.right = std::min(bar.right, bounds.right - 2);
	view->SetHighColor(base);
	view->FillRect(bar);
	view->PopState();
}


//	#pragma mark - status bars, check boxes, radio buttons


void
ReactionaryControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Windows' progress bar: a thin sunken edge, and blocks of the bar
	// colour with gaps of the face colour between them.
	view->PushState();
	draw_bevel(view, rect, kShadow, kWhite);
	view->SetHighColor(base);
	view->FillRect(rect);

	BRect inner = rect.InsetByCopy(1, 1);
	if (inner.IsValid() && progressPosition > inner.left) {
		const float height = inner.Height() + 1;
		const float block = std::max(3.0f, floorf(height * 2 / 3));
		const float gap = height >= 12 ? 2 : 1;
		view->SetHighColor(barColor);
		view->ClipToRect(BRect(inner.left, inner.top,
			std::min(inner.right, progressPosition - 1), inner.bottom));
		for (float x = inner.left; x < progressPosition && x <= inner.right;
				x += block + gap) {
			view->FillRect(BRect(x, inner.top,
				std::min(x + block - 1, inner.right), inner.bottom));
		}
	}
	view->PopState();
}


void
ReactionaryControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool clicked = (flags & B_CLICKED) != 0;

	view->PushState();
	draw_sunken(view, rect, face_color());
	view->SetHighColor(disabled || clicked ? face_color() : kWhite);
	view->FillRect(rect);

	if ((flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0) {
		// a partly set box shows a grey mark
		rgb_color mark = ui_color(B_CONTROL_TEXT_COLOR);
		if (disabled || (flags & B_PARTIALLY_ACTIVATED) != 0)
			mark = kShadow;

		const float size = std::min(rect.Width(), rect.Height()) + 1;
		const float pixel = std::max(1.0f, floorf(size / 9));
		const BPoint origin(
			rect.left + floorf((rect.Width() + 1 - 7 * pixel) / 2),
			rect.top + floorf((rect.Height() + 1 - 7 * pixel) / 2));
		view->SetHighColor(mark);
		for (int32 y = 0; y < 7; y++) {
			for (int32 x = 0; x < 7; x++) {
				if (kCheckMark[y][x] == 'X') {
					fill_pixel(view, origin.x + x * pixel,
						origin.y + y * pixel, pixel);
				}
			}
		}
	}
	view->PopState();
}


void
ReactionaryControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool clicked = (flags & B_CLICKED) != 0;
	const rgb_color face = face_color();

	// The outline, then the same shape shrunk by a pixel for the inner
	// ring, and again for the inside. Each ring is dark above the
	// diagonal from the bottom left to the top right and light below it.
	bool shape[3][12][12];
	for (int32 y = 0; y < 12; y++) {
		for (int32 x = 0; x < 12; x++)
			shape[0][y][x] = x >= kRadioSpans[y][0] && x <= kRadioSpans[y][1];
	}
	for (int32 level = 1; level < 3; level++) {
		for (int32 y = 0; y < 12; y++) {
			for (int32 x = 0; x < 12; x++) {
				shape[level][y][x] = x > 0 && y > 0 && x < 11 && y < 11
					&& shape[level - 1][y][x] && shape[level - 1][y][x - 1]
					&& shape[level - 1][y][x + 1] && shape[level - 1][y - 1][x]
					&& shape[level - 1][y + 1][x];
			}
		}
	}

	const float size = std::min(rect.Width(), rect.Height()) + 1;
	const float pixel = std::max(1.0f, floorf(size / 12));
	const BPoint origin(
		rect.left + floorf((rect.Width() + 1 - 12 * pixel) / 2),
		rect.top + floorf((rect.Height() + 1 - 12 * pixel) / 2));

	view->PushState();
	for (int32 y = 0; y < 12; y++) {
		for (int32 x = 0; x < 12; x++) {
			if (!shape[0][y][x])
				continue;
			const bool upper = x + y <= 10;
			rgb_color color;
			if (!shape[1][y][x])
				color = upper ? kShadow : kWhite;
			else if (!shape[2][y][x])
				color = upper ? kDarkShadow : face;
			else
				color = disabled || clicked ? face : kWhite;
			view->SetHighColor(color);
			fill_pixel(view, origin.x + x * pixel, origin.y + y * pixel, pixel);
		}
	}

	if ((flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0) {
		// a round 4 x 4 dot
		view->SetHighColor(disabled ? kShadow : ui_color(B_CONTROL_TEXT_COLOR));
		for (int32 y = 4; y < 8; y++) {
			for (int32 x = 4; x < 8; x++) {
				if ((x == 4 || x == 7) && (y == 4 || y == 7))
					continue;
				fill_pixel(view, origin.x + x * pixel, origin.y + y * pixel,
					pixel);
			}
		}
	}
	view->PopState();

	rect = BRect(origin.x + 2 * pixel, origin.y + 2 * pixel,
		origin.x + 10 * pixel - 1, origin.y + 10 * pixel - 1);
}


//	#pragma mark - scroll bars


/*!	A scroll bar's border lies on the scroll view's frame where the two
	meet, and takes that frame's colour there; elsewhere it is the face.
*/
void
ReactionaryControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color left = base;
	rgb_color top = base;
	rgb_color right = base;
	rgb_color bottom = base;

	BScrollView* scrollView = dynamic_cast<BScrollView*>(view->Parent());
	if (scrollView != NULL && scrollView->Border() != B_NO_BORDER) {
		const bool fancy = scrollView->Border() == B_FANCY_BORDER;
		const BRect frame = view->Frame();
		BRect line = scrollView->Bounds();
		if (fancy)
			line.InsetBy(1, 1);

		if (fabsf(frame.left - line.left) < 0.5f)
			left = fancy ? kDarkShadow : kShadow;
		if (fabsf(frame.top - line.top) < 0.5f)
			top = fancy ? kDarkShadow : kShadow;
		if (fabsf(frame.right - line.right) < 0.5f)
			right = fancy ? base : kShadow;
		if (fabsf(frame.bottom - line.bottom) < 0.5f)
			bottom = fancy ? base : kShadow;
	}

	view->PushState();
	view->BeginLineArray(4);
	view->AddLine(rect.LeftTop(), rect.RightTop(), top);
	view->AddLine(rect.LeftBottom(), rect.RightBottom(), bottom);
	view->AddLine(BPoint(rect.left, rect.top + 1),
		BPoint(rect.left, rect.bottom - 1), left);
	view->AddLine(BPoint(rect.right, rect.top + 1),
		BPoint(rect.right, rect.bottom - 1), right);
	view->EndLineArray();
	view->PopState();
}


void
ReactionaryControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	BRect inner(rect);
	if (down) {
		// pressed: flat, with a grey edge, and the arrow moves
		view->SetHighColor(kShadow);
		view->StrokeRect(inner);
		inner.InsetBy(1, 1);
	} else
		draw_raised_window(view, inner, base);
	view->SetHighColor(base);
	view->FillRect(inner);

	BRect arrow(rect);
	if (down)
		arrow.OffsetBy(1, 1);
	draw_control_arrow(view, arrow, direction, text,
		(flags & B_DISABLED) != 0);

	view->PopState();
}


void
ReactionaryControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base,
	uint32 flags, orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags,
		orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags,
		orientation);
}


void
ReactionaryControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// the track: white and face pixels in a checkerboard
	view->PushState();
	view->SetDrawingMode(B_OP_COPY);
	view->SetHighColor(kWhite);
	view->SetLowColor(base);
	view->FillRect(rect, B_MIXED_COLORS);
	view->PopState();
}


void
ReactionaryControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// A bar that can't scroll shows no thumb, only its track.
	if ((flags & B_DISABLED) != 0) {
		DrawScrollBarBackground(view, rect, updateRect, face_color(), flags,
			orientation);
		return;
	}

	view->PushState();
	BRect inner(rect);
	draw_raised_window(view, inner, base);
	view->SetHighColor(base);
	view->FillRect(inner);
	view->PopState();
}


void
ReactionaryControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& _base,
	border_style borderStyle, uint32 flags, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	BRect scrollCorner(rect.right, rect.bottom, rect.right, rect.bottom);
	if (horizontalScrollBarFrame.IsValid())
		scrollCorner.left = horizontalScrollBarFrame.right + 1;
	if (verticalScrollBarFrame.IsValid())
		scrollCorner.top = verticalScrollBarFrame.bottom + 1;

	view->PushState();
	if (borderStyle == B_FANCY_BORDER)
		draw_sunken(view, rect, base, borders);
	else if (borderStyle == B_PLAIN_BORDER)
		draw_bevel(view, rect, kShadow, kShadow, borders);

	if (horizontalScrollBarFrame.IsValid() && verticalScrollBarFrame.IsValid()
		&& scrollCorner.IsValid()) {
		view->SetHighColor(base);
		view->FillRect(scrollCorner & rect);
	}
	view->PopState();
}


void
ReactionaryControlLook::DrawArrowShape(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 direction,
	uint32 flags, float tint)
{
	if (direction > B_DOWN_ARROW) {
		HaikuControlLook::DrawArrowShape(view, rect, updateRect, base,
			direction, flags, tint);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	draw_control_arrow(view, rect, direction, tint_color(base, tint),
		(flags & B_DISABLED) != 0);
}


//	#pragma mark - sliders


rgb_color
ReactionaryControlLook::SliderBarColor(const rgb_color& base)
{
	return face_color();
}


void
ReactionaryControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color leftFillColor,
	rgb_color rightFillColor, float sliderScale, uint32 flags,
	orientation orientation)
{
	// splits the bar, drawing all of it clipped to each side
	HaikuControlLook::DrawSliderBar(view, rect, updateRect, base,
		leftFillColor, rightFillColor, sliderScale, flags, orientation);
}


void
ReactionaryControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// a sunken channel, four pixels deep as on Windows
	if (orientation == B_HORIZONTAL) {
		const float middle = floorf((rect.top + rect.bottom) / 2);
		rect.top = middle - 1;
		rect.bottom = middle + 2;
	} else {
		const float middle = floorf((rect.left + rect.right) / 2);
		rect.left = middle - 1;
		rect.right = middle + 2;
	}

	view->PushState();
	draw_sunken(view, rect, face_color());
	view->SetHighColor(fillColor);
	view->FillRect(rect);
	view->PopState();
}


void
ReactionaryControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	BRect knob(rect);
	knob.right--;
	knob.bottom--;
	draw_raised(view, knob, face_color());
	view->SetHighColor(face_color());
	view->FillRect(knob);
	if ((flags & (B_FOCUSED | B_DISABLED)) == B_FOCUSED)
		draw_focus_rect(view, knob.InsetByCopy(1, 1), kBlack);
	view->PopState();

	rect = knob;
}


void
ReactionaryControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawSliderTriangle(view, rect, updateRect, base, face_color(), flags,
		orientation);
}


/*!	Windows' pointed thumb: a raised box ending in a point towards the
	ticks, below the bar (horizontal) or left of it (vertical).
*/
void
ReactionaryControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	BRect frame(floorf(rect.left), floorf(rect.top), floorf(rect.right) - 1,
		floorf(rect.bottom) - 1);
	const bool horizontal = orientation == B_HORIZONTAL;
	const float across = horizontal ? frame.Width() : frame.Height();
	const float point = floorf(across / 2);

	view->PushState();
	view->SetDrawingMode(B_OP_COPY);

	// the box
	BRect box(frame);
	if (horizontal)
		box.bottom -= point;
	else
		box.left += point;
	view->SetHighColor(fill);
	view->FillRect(box);

	// the point, row by row
	for (float i = 0; i < point; i++) {
		if (horizontal) {
			view->FillRect(BRect(frame.left + i + 1, box.bottom + 1 + i,
				frame.right - i - 1, box.bottom + 1 + i));
		} else {
			view->FillRect(BRect(box.left - 1 - i, frame.top + i + 1,
				box.left - 1 - i, frame.bottom - i - 1));
		}
	}

	// the edges: white on the lit sides, dark grey and grey on the others
	view->BeginLineArray(8);
	if (horizontal) {
		const BPoint tip((frame.left + frame.right) / 2, frame.bottom);
		view->AddLine(frame.LeftTop(), BPoint(frame.right - 1, frame.top),
			kWhite);
		view->AddLine(frame.LeftTop(), box.LeftBottom(), kWhite);
		view->AddLine(box.LeftBottom(), BPoint(floorf(tip.x), tip.y), kWhite);
		view->AddLine(frame.RightTop(), box.RightBottom(), kDarkShadow);
		view->AddLine(box.RightBottom(), BPoint(ceilf(tip.x), tip.y),
			kDarkShadow);
		view->AddLine(BPoint(frame.right - 1, frame.top + 1),
			BPoint(box.right - 1, box.bottom), kShadow);
		view->AddLine(BPoint(box.right - 1, box.bottom + 1),
			BPoint(ceilf(tip.x), tip.y - 1), kShadow);
	} else {
		const BPoint tip(frame.left, (frame.top + frame.bottom) / 2);
		view->AddLine(box.LeftTop(), BPoint(frame.right - 1, frame.top),
			kWhite);
		view->AddLine(box.LeftTop(), BPoint(tip.x, floorf(tip.y)), kWhite);
		view->AddLine(frame.RightTop(), frame.RightBottom(), kDarkShadow);
		view->AddLine(box.LeftBottom(), frame.RightBottom(), kDarkShadow);
		view->AddLine(box.LeftBottom(), BPoint(tip.x, ceilf(tip.y)),
			kDarkShadow);
		view->AddLine(BPoint(box.left + 1, box.bottom - 1),
			BPoint(frame.right - 1, frame.bottom - 1), kShadow);
	}
	view->EndLineArray();

	if ((flags & (B_FOCUSED | B_DISABLED)) == B_FOCUSED)
		draw_focus_rect(view, box.InsetByCopy(2, 2), kBlack);

	view->PopState();
}


//	#pragma mark - tabs


void
ReactionaryControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	// the pane's edge facing the tabs, which the selected tab breaks
	if (borderStyle != B_NO_BORDER) {
		switch (side) {
			default:
			case BTabView::kTopSide:
				view->SetHighColor(kWhite);
				view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
				break;
			case BTabView::kBottomSide:
				view->SetHighColor(kDarkShadow);
				view->StrokeLine(rect.LeftTop(), rect.RightTop());
				break;
			case BTabView::kLeftSide:
				view->SetHighColor(kWhite);
				view->StrokeLine(rect.RightTop(), rect.RightBottom());
				break;
			case BTabView::kRightSide:
				view->SetHighColor(kDarkShadow);
				view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
				break;
		}
	}
	view->PopState();
}


void
ReactionaryControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawTab(view, rect, base, flags, side, true);
}


void
ReactionaryControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawTab(view, rect, base, flags, side, false);
}


//	#pragma mark - borders


void
ReactionaryControlLook::DrawBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, border_style borderStyle,
	uint32 flags, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	if (borderStyle == B_NO_BORDER)
		return;

	view->PushState();
	if (dynamic_cast<BMenuBar*>(view) != NULL) {
		// menu bars have no edge of their own
		draw_bevel(view, rect, base, base, borders);
	} else if (borderStyle == B_FANCY_BORDER)
		draw_sunken(view, rect, base, borders);
	else
		draw_bevel(view, rect, kShadow, kShadow, borders);
	view->PopState();
}


void
ReactionaryControlLook::DrawRaisedBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	view->PushState();
	draw_bevel(view, rect, kWhite, kShadow, borders);
	view->PopState();
}


/*!	A group box's etched line, or the raised edges of a tab view's pane.
	Both take three pixels, like Haiku's.
*/
void
ReactionaryControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	view->PushState();
	if (dynamic_cast<BTabView*>(view) != NULL) {
		draw_bevel(view, rect, kWhite, kDarkShadow, borders);
		draw_bevel(view, rect, base, kShadow, borders);
	} else {
		draw_bevel(view, rect, kShadow, kWhite, borders);
		draw_bevel(view, rect, kWhite, kShadow, borders);
	}
	draw_bevel(view, rect, base, base, borders);
	view->PopState();
}


void
ReactionaryControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, uint32 flags,
	uint32 borders)
{
	const rgb_color base = solid(_base, view);
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	if ((flags & B_DISABLED) == 0 && (flags & B_INVALID) != 0) {
		const rgb_color failure = ui_color(B_FAILURE_COLOR);
		draw_bevel(view, rect, kShadow, kWhite, borders);
		draw_bevel(view, rect, failure, failure, borders);
	} else
		draw_sunken(view, rect, base, borders);
	view->PopState();
}


//	#pragma mark - labels


/*!	Disabled text is engraved: grey, over the same text in white one pixel
	down and right. Everything else is drawn as Haiku draws it.
*/
void
ReactionaryControlLook::DrawLabel(BView* view, const char* label,
	const rgb_color& base, uint32 flags, const BPoint& where,
	const rgb_color* textColor)
{
	const rgb_color background = solid(base, view);
	const bool lightBackground
		= background.red + background.green + background.blue > 128 * 3;
	if ((flags & B_DISABLED) == 0 || !lightBackground
		|| (flags & B_IGNORE_OUTLINE) != 0) {
		HaikuControlLook::DrawLabel(view, label, base, flags, where,
			textColor);
		return;
	}

	view->PushState();
	view->SetDrawingMode(B_OP_OVER);
	view->SetHighColor(kWhite);
	view->DrawString(label, BPoint(where.x + 1, where.y + 1));
	view->SetHighColor(kShadow);
	view->DrawString(label, where);
	view->PopState();
}


/*!	Check boxes and radio buttons show focus with a dotted rectangle round
	their label.
*/
void
ReactionaryControlLook::DrawLabel(BView* view, const char* label,
	const BBitmap* icon, BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, const BAlignment& alignment,
	const rgb_color* textColor)
{
	HaikuControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);

	if (label == NULL || (flags & (B_FOCUSED | B_DISABLED)) != B_FOCUSED
		|| (dynamic_cast<BCheckBox*>(view) == NULL
			&& dynamic_cast<BRadioButton*>(view) == NULL))
		return;

	float width = 0;
	if (icon != NULL)
		width = icon->Bounds().Width() + DefaultLabelSpacing() + 1;

	BString truncated(label);
	BFont font;
	view->GetFont(&font);
	font.TruncateString(&truncated, B_TRUNCATE_END, rect.Width() + 1 - width);
	width += ceilf(font.StringWidth(truncated.String()));

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	const float height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);

	BRect text = BLayoutUtils::AlignOnRect(rect,
		BSize(width - 1, height - 1), alignment);
	text.InsetBy(-1, 0);
	draw_focus_rect(view, text & view->Bounds(),
		ui_color(B_CONTROL_TEXT_COLOR));
}


//	#pragma mark - private


/*!	Push buttons. A normal button is a two pixel raised bevel; pressed, it
	is a black line and a grey one. The default button keeps two pixels of
	background outside a black line round the bevel, so it looks only a
	little bigger than the others, as on Windows.
*/
void
ReactionaryControlLook::_DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!rect.IsValid())
		return;

	view->PushState();
	view->ClipToRect(rect);

	// Buttons have the face colour, whatever control colour they are given.
	const rgb_color face = face_color();
	const rgb_color behind = solid(background, view);
	const bool pressed = (flags & (B_ACTIVATED | B_CLICKED)) != 0;

	if ((flags & B_FLAT) != 0) {
		// tool bar buttons: nothing until hovered, then one thin bevel
		BRect edge(rect);
		if (is_flat(flags))
			draw_bevel(view, edge, behind, behind, borders);
		else if (pressed)
			draw_bevel(view, edge, kShadow, kWhite, borders);
		else
			draw_bevel(view, edge, kWhite, kShadow, borders);
		draw_bevel(view, edge, behind, behind, borders);
		view->PopState();
		inset_by_borders(rect, 2, borders);
		return;
	}

	if ((flags & B_DEFAULT_BUTTON) != 0) {
		draw_bevel(view, rect, behind, behind, borders);
		draw_bevel(view, rect, behind, behind, borders);
		draw_bevel(view, rect, kBlack, kBlack, borders);
	}

	if (pressed) {
		if ((flags & B_DEFAULT_BUTTON) == 0)
			draw_bevel(view, rect, kBlack, kBlack, borders);
		draw_bevel(view, rect, kShadow, kShadow, borders);
		if ((flags & B_DEFAULT_BUTTON) != 0)
			draw_bevel(view, rect, face, face, borders);
	} else
		draw_raised(view, rect, face, borders);

	view->PopState();
}


/*!	The face, a pixel inside the frame. A pressed button's label moves one
	pixel down and right; a focused one gets a dotted rectangle.
*/
void
ReactionaryControlLook::_DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, bool popUp)
{
	if (!rect.IsValid() || !ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(face_color());
	view->FillRect(rect);

	const bool pressed = (flags & (B_ACTIVATED | B_CLICKED)) != 0
		&& !is_flat(flags);
	const bool focused = (flags & (B_FOCUSED | B_DISABLED)) == B_FOCUSED
		&& (flags & B_FLAT) == 0;

	inset_by_borders(rect, 1, borders);
	if (focused)
		draw_focus_rect(view, rect, ui_color(B_CONTROL_TEXT_COLOR));

	if (popUp) {
		// the arrow sits in the room Haiku's look keeps for it
		const float width = ComposeSpacing(B_USE_ITEM_SPACING);
		BRect arrow(rect.right - width + 1, rect.top, rect.right - 1,
			rect.bottom);
		if (pressed)
			arrow.OffsetBy(1, 1);
		// as big as a drop-down list's arrow, though the room is narrow
		const int32 depth = (int32)std::max(2.0f,
			std::min(floorf((rect.Height() + 3) / 4), floorf((width + 1) / 2)));
		draw_control_arrow(view, arrow, B_DOWN_ARROW,
			ui_color(B_CONTROL_TEXT_COLOR), (flags & B_DISABLED) != 0, depth);
		rect.right -= width;
	}
	view->PopState();

	if (pressed)
		rect.OffsetBy(1, 1);
}


/*!	The inside of a drop-down list: white, with a raised button holding a
	down arrow against its right edge. The menu field has drawn the sunken
	frame round it.
*/
void
ReactionaryControlLook::_DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color face = face_color();

	view->PushState();
	view->SetHighColor(disabled ? face : kWhite);
	view->FillRect(rect);

	BRect text(rect);
	if (popupIndicator) {
		// Haiku keeps this much room on the right for the arrow
		const float indicator = ComposeSpacing(B_USE_ITEM_SPACING);
		const float width = std::max(indicator,
			std::min(rect.Height() + 1, indicator + 6));
		BRect button(rect.right - width + 1, rect.top, rect.right,
			rect.bottom);
		BRect inner(button);
		draw_raised_window(view, inner, face);
		view->SetHighColor(face);
		view->FillRect(inner);
		draw_control_arrow(view, button, B_DOWN_ARROW,
			ui_color(B_CONTROL_TEXT_COLOR), disabled);
		text.right = std::min(button.left - 1,
			rect.right - indicator + 1);
	}
	text.InsetBy(1, 1);

	if ((flags & (B_FOCUSED | B_DISABLED)) == B_FOCUSED)
		draw_focus_rect(view, text, ui_color(B_CONTROL_TEXT_COLOR));
	view->PopState();

	rect = text;
}


/*!	A classic tab, for any side of the tab view. Coordinates run along the
	tab's base (u) and from its outer edge in towards the pane (v); the side
	that comes first along u is lit. The selected tab is two pixels taller
	and covers the pane's edge under it.
*/
void
ReactionaryControlLook::_DrawTab(BView* view, BRect& rect,
	const rgb_color& base, uint32 flags, uint32 side, bool active)
{
	rect.left = floorf(rect.left);
	rect.right = floorf(rect.right);
	rect.top = floorf(rect.top);
	rect.bottom = floorf(rect.bottom);

	const bool vertical = side == BTabView::kLeftSide
		|| side == BTabView::kRightSide;
	const bool outerLit = side == BTabView::kTopSide
		|| side == BTabView::kLeftSide;
	const float length = vertical ? rect.Height() : rect.Width();
	const float depth = vertical ? rect.Width() : rect.Height();

	// a rect from tab coordinates
	auto area = [&](float u0, float v0, float u1, float v1) {
		BRect result;
		switch (side) {
			default:
			case BTabView::kTopSide:
				result.Set(rect.left + u0, rect.top + v0, rect.left + u1,
					rect.top + v1);
				break;
			case BTabView::kBottomSide:
				result.Set(rect.left + u0, rect.bottom - v1, rect.left + u1,
					rect.bottom - v0);
				break;
			case BTabView::kLeftSide:
				result.Set(rect.left + v0, rect.top + u0, rect.left + v1,
					rect.top + u1);
				break;
			case BTabView::kRightSide:
				result.Set(rect.right - v1, rect.top + u0, rect.right - v0,
					rect.top + u1);
				break;
		}
		return result;
	};

	view->PushState();
	view->ClipToRect(rect);
	view->SetDrawingMode(B_OP_COPY);

	// the selected tab runs into the pane; the others stop at its edge
	const float top = active ? 0 : 2;
	const float bottom = active ? depth : depth - 1;

	view->SetHighColor(base);
	view->FillRect(area(1, top + 1, length - 2, bottom));

	// the lit side and the rounded corner next to it
	view->SetHighColor(kWhite);
	view->FillRect(area(0, top + 2, 0, bottom));
	view->FillRect(area(1, top + 1, 1, top + 1));

	// the outer edge
	if (outerLit) {
		view->SetHighColor(kWhite);
		view->FillRect(area(2, top, length - 2, top));
	} else {
		view->SetHighColor(kDarkShadow);
		view->FillRect(area(2, top, length - 2, top));
		view->SetHighColor(kShadow);
		view->FillRect(area(2, top + 1, length - 2, top + 1));
	}

	// the shaded side
	view->SetHighColor(kDarkShadow);
	view->FillRect(area(length, top + 2, length, bottom));
	view->FillRect(area(length - 1, top + 1, length - 1, top + 1));
	view->SetHighColor(kShadow);
	view->FillRect(area(length - 1, top + 2, length - 1, bottom));

	view->PopState();

	// the label's room
	rect = area(3, top + 2, length - 3, depth - 1);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::ReactionaryControlLook();
}
