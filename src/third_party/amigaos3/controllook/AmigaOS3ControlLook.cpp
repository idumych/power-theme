/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See AmigaOS3ControlLook.h. The pictures, in AmigaOS3ControlArt.h, are from
 * ../art/gtk-2.0, and each is stretched with the borders its .rc file gives it.
 */


#include "AmigaOS3ControlLook.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <new>
#include <string.h>

#include <Bitmap.h>
#include <Button.h>
#include <InterfaceDefs.h>
#include <Autolock.h>
#include <Locker.h>
#include <Menu.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>

#include "AmigaOS3ControlArt.h"


namespace BPrivate {


static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kWhite = { 255, 255, 255, 255 };

static const float kIndicatorWidth = 18;


//!	A picture as a bitmap, made once and kept.
static BBitmap*
bitmap_for(const Art& art)
{
	static std::map<const uint32*, BBitmap*> sBitmaps;
	static BLocker sLock("amigaos3 art");
	BAutolock locker(sLock);

	std::map<const uint32*, BBitmap*>::iterator found = sBitmaps.find(art.pixels);
	if (found != sBitmaps.end())
		return found->second;

	BBitmap* bitmap = new(std::nothrow) BBitmap(
		BRect(0, 0, art.width - 1, art.height - 1), B_RGBA32);
	if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return NULL;
	}
	// B_RGBA32 is the pixels' ARGB as a little endian word
	const int32 rowBytes = bitmap->BytesPerRow();
	for (int32 y = 0; y < art.height; y++) {
		memcpy((uint8*)bitmap->Bits() + y * rowBytes,
			art.pixels + y * art.width, art.width * 4);
	}
	sBitmaps[art.pixels] = bitmap;
	return bitmap;
}


/*!	Draws a picture over rect, its corners as they are, its edges stretched
	along and its middle across, without smoothing: GTK's stretched pixmaps.
	The borders are the picture's left, right, top and bottom.
*/
static void
draw_art(BView* view, const Art& art, BRect rect, float left, float right,
	float top, float bottom)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL || !rect.IsValid())
		return;

	const float w = art.width;
	const float h = art.height;
	// what fits: a rectangle smaller than the borders shrinks them
	left = std::min(left, floorf((rect.Width() + 1) / 2));
	right = std::min(right, floorf((rect.Width() + 1) / 2));
	top = std::min(top, floorf((rect.Height() + 1) / 2));
	bottom = std::min(bottom, floorf((rect.Height() + 1) / 2));

	const float sx[4] = { 0, left, w - right, w };
	const float sy[4] = { 0, top, h - bottom, h };
	const float dx[4] = { rect.left, rect.left + left, rect.right + 1 - right,
		rect.right + 1 };
	const float dy[4] = { rect.top, rect.top + top, rect.bottom + 1 - bottom,
		rect.bottom + 1 };

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	for (int32 row = 0; row < 3; row++) {
		for (int32 column = 0; column < 3; column++) {
			if (sx[column + 1] <= sx[column] || sy[row + 1] <= sy[row]
				|| dx[column + 1] <= dx[column] || dy[row + 1] <= dy[row])
				continue;
			const BRect source(sx[column], sy[row], sx[column + 1] - 1,
				sy[row + 1] - 1);
			const BRect target(dx[column], dy[row], dx[column + 1] - 1,
				dy[row + 1] - 1);
			view->DrawBitmap(bitmap, source, target);
		}
	}
	view->PopState();
}


static void
draw_art(BView* view, const Art& art, const BRect& rect, float border)
{
	draw_art(view, art, rect, border, border, border, border);
}


//!	Draws a picture as it is, in the middle of rect.
static void
draw_centred(BView* view, const Art& art, const BRect& rect)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL)
		return;
	const BPoint where(floorf((rect.left + rect.right + 1 - art.width) / 2),
		floorf((rect.top + rect.bottom + 1 - art.height) / 2));
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->DrawBitmap(bitmap, where);
	view->PopState();
}


//!	Repeats a picture over rect, from its top left corner.
static void
draw_tiled(BView* view, const Art& art, const BRect& rect)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL || !rect.IsValid())
		return;
	view->PushState();
	view->ClipToRect(rect);
	for (float y = rect.top; y <= rect.bottom; y += art.height) {
		for (float x = rect.left; x <= rect.right; x += art.width)
			view->DrawBitmap(bitmap, BPoint(x, y));
	}
	view->PopState();
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED | BControlLook::B_CLICKED)) != 0
		&& (flags & BControlLook::B_DISABLED) == 0;
}


//!	A button's picture for its state.
static const Art&
button_art(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kButtonButtonIns;
	return is_pressed(flags) ? kButtonButtonPressed : kButtonButton;
}


static void
fill(BView* view, const BRect& rect, const rgb_color& color)
{
	if (!rect.IsValid())
		return;
	view->SetHighColor(color);
	view->FillRect(rect);
}


//	#pragma mark -


AmigaOS3ControlLook::AmigaOS3ControlLook()
{
}


AmigaOS3ControlLook::~AmigaOS3ControlLook()
{
}


//	#pragma mark - buttons


/*!	The button's picture, stretched; the default button has a black line round
	it, a pixel away.
*/
void
AmigaOS3ControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 4 : 2;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);
	fill(view, rect, background);
	BRect shape(rect);
	if (isDefault) {
		view->SetHighColor(kBlack);
		view->StrokeRect(shape);
		shape.InsetBy(2, 2);
	}
	draw_art(view, button_art(flags), shape, 4);
	view->PopState();

	rect.InsetBy(inset, inset);
}


void
AmigaOS3ControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	// Tracker's column headers, and the Deskbar's items in a menu, are drawn
	// as a button's background, and get a button's picture
	if ((view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0)
		|| dynamic_cast<BMenu*>(view) != NULL) {
		if (ShouldDraw(view, rect, updateRect))
			draw_art(view, button_art(flags), rect, 2);
		return;
	}

	// the frame drew the picture, face and all
	rect.InsetBy(1, 1);
}


void
AmigaOS3ControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);
	draw_centred(view, is_pressed(flags) ? kArrowArrowDownActiveCombo
		: kArrowArrowDownCombo, BRect(area.right - 12, area.top, area.right - 2,
		area.bottom));
}


void
AmigaOS3ControlLook::DrawLabel(BView* view, const char* label,
	const BBitmap* icon, BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, const BAlignment& alignment,
	const rgb_color* textColor)
{
	ForwardingControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);
}


//	#pragma mark - menus


//!	The menu bar's bevelled picture.
void
AmigaOS3ControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		view->PushState();
		fill(view, rect, base);
		view->PopState();
		return;
	}

	draw_art(view, kMenubarMenubar, rect, 3);
}


//!	A button, with a groove and the combo box arrow at its right end.
void
AmigaOS3ControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill(view, rect, background);
		draw_art(view, button_art(flags & ~(B_ACTIVATED | B_CLICKED)), rect, 4);
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


void
AmigaOS3ControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	// redraws the button inside the field's menu bar, which Haiku clears first
	draw_art(view, button_art(flags & ~(B_ACTIVATED | B_CLICKED)),
		rect.InsetByCopy(-2, -2), 4);

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 3;
		// a groove, then the arrow
		fill(view, BRect(indicator.left, rect.top + 1, indicator.left,
			rect.bottom - 1), kBlack);
		fill(view, BRect(indicator.left + 1, rect.top + 1, indicator.left + 1,
			rect.bottom - 1), kWhite);
		draw_centred(view, (flags & (B_ACTIVATED | B_CLICKED)) != 0
			? kArrowArrowDownActiveCombo : kArrowArrowDownCombo,
			BRect(indicator.left + 2, rect.top, indicator.right, rect.bottom));
		rect.right = indicator.left - 1;
	}

	view->PopState();
}


void
AmigaOS3ControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


//!	The menu bar's bevelled picture behind the items.
void
AmigaOS3ControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	draw_art(view, kMenubarMenubar, rect, 3);

	if ((borders & B_LEFT_BORDER) != 0)
		rect.left += 2;
	if ((borders & B_RIGHT_BORDER) != 0)
		rect.right -= 2;
	if ((borders & B_TOP_BORDER) != 0)
		rect.top += 2;
	if ((borders & B_BOTTOM_BORDER) != 0)
		rect.bottom -= 2;
}


//!	A selected item is the blue menu item picture.
void
AmigaOS3ControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	if (selected)
		draw_art(view, kMenubarMenuitem, rect, 3);
	else {
		view->PushState();
		fill(view, rect, base);
		view->PopState();
	}
}


//	#pragma mark - status bars


//!	The sunken trough, and in it the blue bar, each its picture stretched.
void
AmigaOS3ControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	draw_art(view, kProgressbarTroughH, rect, 3);
	if (progressPosition > rect.left + 2) {
		BRect bar(rect.InsetByCopy(2, 2));
		bar.right = std::min(progressPosition, bar.right);
		if (bar.IsValid())
			draw_art(view, kProgressbarProgressbarHoriz, bar, 3);
	}
}


//	#pragma mark - check boxes and radio buttons


void
AmigaOS3ControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0
		? kCBoxCBoxPressed : kCBoxCBox, r);
}


void
AmigaOS3ControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, (flags & B_ACTIVATED) != 0 ? kRadioRadioPressed
		: kRadioRadio, r);
}


//	#pragma mark - scroll bars


void
AmigaOS3ControlLook::DrawScrollBarBorder(BView* view, BRect rect,
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


//!	The arrow's picture, stretched to the box, the clicked one when held.
void
AmigaOS3ControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Art* art;
	switch (direction) {
		case B_UP_ARROW:
			art = down ? &kScrollbarArrowUpClicked : &kScrollbarArrowUp;
			break;
		case B_DOWN_ARROW:
			art = down ? &kScrollbarArrowDownClicked : &kScrollbarArrowDown;
			break;
		case B_LEFT_ARROW:
			art = down ? &kScrollbarArrowLeftClicked : &kScrollbarArrowLeft;
			break;
		default:
			art = down ? &kScrollbarArrowRightClicked : &kScrollbarArrowRight;
			break;
	}
	draw_art(view, *art, rect, 3);
}


void
AmigaOS3ControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


//!	The dotted pattern, and the trough's frame over it.
void
AmigaOS3ControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	draw_tiled(view, kScrollbarTroughDots, rect);
	if (orientation == B_VERTICAL)
		draw_art(view, kScrollbarTroughV, rect, 4, 4, 3, 3);
	else
		draw_art(view, kScrollbarTroughH, rect, 3, 3, 4, 4);
}


//!	The raised thumb, its grip in the middle. A bar with nothing to scroll has none.
void
AmigaOS3ControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	BRect track(rect);
	DrawScrollBarBackground(view, track, updateRect, base, flags, orientation);
	if ((flags & B_DISABLED) != 0)
		return;

	if (orientation == B_VERTICAL) {
		draw_art(view, kScrollbarScrollbarVertical, rect, 4);
		if (rect.Height() > kScrollbarThumbV.height + 4)
			draw_centred(view, kScrollbarThumbV, rect);
	} else {
		draw_art(view, kScrollbarScrollbarHorizontal, rect, 4);
		if (rect.Width() > kScrollbarThumbH.width + 4)
			draw_centred(view, kScrollbarThumbH, rect);
	}
}


float
AmigaOS3ControlLook::GetScrollBarWidth(orientation orientation)
{
	return 16;
}


//	#pragma mark - sliders


void
AmigaOS3ControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if (orientation == B_HORIZONTAL)
		draw_art(view, kScaleTroughHoriz, rect, 8, 8, 2, 2);
	else
		draw_art(view, kScaleTroughVert, rect, 2, 2, 8, 8);

	// an application's fill colour, when it has one; Haiku passes a grey tint
	// of the panel for the part with none
	const bool isGrey = fillColor.red == fillColor.green
		&& fillColor.green == fillColor.blue;
	if (!isGrey && (flags & B_DISABLED) == 0) {
		view->PushState();
		fill(view, rect.InsetByCopy(2, 2), fillColor);
		view->PopState();
	}
}


void
AmigaOS3ControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if (orientation == B_HORIZONTAL) {
		draw_art(view, kScaleSliderHoriz, rect, 9, 9, 4, 4);
		draw_centred(view, kScaleThumbH, rect);
	} else {
		draw_art(view, kScaleSliderVert, rect, 4, 4, 9, 9);
		draw_centred(view, kScaleThumbV, rect);
	}
}


void
AmigaOS3ControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	DrawSliderThumb(view, rect, updateRect, base, flags, orientation);
}


//	#pragma mark - text controls


void
AmigaOS3ControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect))
		draw_art(view, kEntryEntry, rect, 2);
	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


//!	The notebook's raised frame round the group.
void
AmigaOS3ControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		ForwardingControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	draw_art(view, kNotebookNotebook, rect, 8);
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


//!	A white line along the page's edge that the tabs sit on.
void
AmigaOS3ControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, base);
	if (borderStyle != B_NO_BORDER)
		fill(view, page_edge(rect, side), kWhite);
	view->PopState();
}


//!	Where a tab's label goes: inside its bevel.
static void
inset_tab(BRect& rect, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			rect.InsetBy(4, 0);
			rect.top += 1;
			rect.bottom -= 3;
			break;
		case BTabView::kLeftSide:
			rect.InsetBy(0, 4);
			rect.left += 3;
			rect.right -= 1;
			break;
		case BTabView::kRightSide:
			rect.InsetBy(0, 4);
			rect.left += 1;
			rect.right -= 3;
			break;
		default:
			rect.InsetBy(4, 0);
			rect.top += 3;
			rect.bottom -= 1;
			break;
	}
}


//!	The tab's picture for its side, the chosen one with its bigger bevel.
static void
draw_tab(BView* view, const BRect& rect, uint32 side, bool chosen)
{
	const Art* art;
	switch (side) {
		case BTabView::kBottomSide:
			art = chosen ? &kNotebookTabBottomActive : &kNotebookTabBottom;
			break;
		case BTabView::kLeftSide:
			art = chosen ? &kNotebookTabLeftActive : &kNotebookTabLeft;
			break;
		case BTabView::kRightSide:
			art = chosen ? &kNotebookTabRightActive : &kNotebookTabRight;
			break;
		default:
			art = chosen ? &kNotebookTabTopActive : &kNotebookTabTop;
			break;
	}
	BRect shape(rect);
	// one behind the chosen tab is lower, and sits on the page's edge
	if (!chosen) {
		switch (side) {
			case BTabView::kBottomSide:
				shape.bottom -= 2;
				break;
			case BTabView::kLeftSide:
				shape.left += 2;
				break;
			case BTabView::kRightSide:
				shape.right -= 2;
				break;
			default:
				shape.top += 2;
				break;
		}
	}
	draw_art(view, *art, shape, chosen ? 8 : 6);
}


void
AmigaOS3ControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill(view, rect, base);
		draw_tab(view, rect, side, true);
		view->PopState();
	}
	inset_tab(rect, side);
}


void
AmigaOS3ControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill(view, rect, base);
		draw_tab(view, rect, side, false);
		fill(view, page_edge(rect, side), kWhite);
		view->PopState();
	}
	inset_tab(rect, side);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::AmigaOS3ControlLook();
}
