/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See NotebookControlLook.h. The pictures, in NotebookControlArt.h, are from
 * ../art. The skin has pictures only for buttons, check boxes and radio
 * buttons; the other controls take the button's pencil outline, so that they
 * look drawn on the same paper.
 */


#include "NotebookControlLook.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <new>
#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Locker.h>
#include <Menu.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>

#include "NotebookControlArt.h"


namespace BPrivate {


static const rgb_color kPaper = { 255, 255, 255, 255 };
// the pencil, as the darkest grey of the buttons' outline
static const rgb_color kPencil = { 66, 66, 66, 255 };
static const rgb_color kLightPencil = { 156, 156, 156, 255 };
// the rule under the menu bar, from the skin's menu.bmp
static const rgb_color kMenuRule = { 17, 5, 96, 255 };
// the colour menus select in, from notebook.uis
static const rgb_color kMenuSelection = { 232, 232, 232, 255 };
// how much of a button's picture is its outline, kept as it is when stretched
static const float kEdge = 2;
static const float kIndicatorWidth = 16;


//!	A picture as a bitmap, made once and kept.
static BBitmap*
bitmap_for(const Art& art)
{
	static std::map<const uint32*, BBitmap*> sBitmaps;
	static BLocker sLock("notebook art");
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
	along and, with face, its middle across, without smoothing.
*/
static void
draw_stretched(BView* view, const Art& art, BRect rect, float border,
	bool face = true)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL || !rect.IsValid())
		return;

	const float w = art.width;
	const float h = art.height;
	const float across = std::min(border, floorf((rect.Width() + 1) / 2));
	const float down = std::min(border, floorf((rect.Height() + 1) / 2));

	const float sx[4] = { 0, across, w - across, w };
	const float sy[4] = { 0, down, h - down, h };
	const float dx[4] = { rect.left, rect.left + across,
		rect.right + 1 - across, rect.right + 1 };
	const float dy[4] = { rect.top, rect.top + down, rect.bottom + 1 - down,
		rect.bottom + 1 };

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	for (int32 row = 0; row < 3; row++) {
		for (int32 column = 0; column < 3; column++) {
			if (!face && row == 1 && column == 1)
				continue;
			if (sx[column + 1] <= sx[column] || sy[row + 1] <= sy[row]
				|| dx[column + 1] <= dx[column] || dy[row + 1] <= dy[row])
				continue;
			view->DrawBitmap(bitmap, BRect(sx[column], sy[row],
				sx[column + 1] - 1, sy[row + 1] - 1), BRect(dx[column],
				dy[row], dx[column + 1] - 1, dy[row + 1] - 1));
		}
	}
	view->PopState();
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


static void
fill(BView* view, const BRect& rect, const rgb_color& color)
{
	if (!rect.IsValid())
		return;
	view->SetHighColor(color);
	view->FillRect(rect);
}


//!	The pencil outline of a button round rect, with white paper inside if face.
static void
pencil(BView* view, const BRect& rect, uint32 flags, bool face = true)
{
	draw_stretched(view, (flags & BControlLook::B_DISABLED) != 0
		? kButtonDisabled : kButton, rect, kEdge, face);
}


//!	A button's picture for its state.
static const Art&
button_art(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kButtonDisabled;
	if ((flags & (BControlLook::B_ACTIVATED | BControlLook::B_PARTIALLY_ACTIVATED
			| BControlLook::B_CLICKED)) != 0)
		return kButtonPressed;
	if ((flags & BControlLook::B_DEFAULT_BUTTON) != 0)
		return kButtonDefault;
	if ((flags & BControlLook::B_FOCUSED) != 0)
		return kButtonFocused;
	return kButton;
}


//!	A small pencilled arrowhead pointing direction, in the middle of rect.
static void
draw_arrow(BView* view, const BRect& rect, int32 direction, bool disabled)
{
	const float size = std::max(2.0f,
		floorf(std::min(rect.Width(), rect.Height()) / 4));
	const BPoint middle(floorf((rect.left + rect.right) / 2) + 0.5f,
		floorf((rect.top + rect.bottom) / 2) + 0.5f);
	BPoint a, b, c;
	switch (direction) {
		case BControlLook::B_UP_ARROW:
			a = BPoint(middle.x - size, middle.y + size / 2);
			b = BPoint(middle.x + size, middle.y + size / 2);
			c = BPoint(middle.x, middle.y - size / 2);
			break;
		case BControlLook::B_LEFT_ARROW:
			a = BPoint(middle.x + size / 2, middle.y - size);
			b = BPoint(middle.x + size / 2, middle.y + size);
			c = BPoint(middle.x - size / 2, middle.y);
			break;
		case BControlLook::B_RIGHT_ARROW:
			a = BPoint(middle.x - size / 2, middle.y - size);
			b = BPoint(middle.x - size / 2, middle.y + size);
			c = BPoint(middle.x + size / 2, middle.y);
			break;
		default:
			a = BPoint(middle.x - size, middle.y - size / 2);
			b = BPoint(middle.x + size, middle.y - size / 2);
			c = BPoint(middle.x, middle.y + size / 2);
			break;
	}
	view->PushState();
	view->SetHighColor(disabled ? kLightPencil : kPencil);
	view->FillTriangle(a, b, c);
	view->PopState();
}


//	#pragma mark -


NotebookControlLook::NotebookControlLook()
{
}


NotebookControlLook::~NotebookControlLook()
{
}


//	#pragma mark - buttons


//!	The button's picture, stretched, face and all.
void
NotebookControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill(view, rect, background);
		draw_stretched(view, button_art(flags), rect, kEdge);
		view->PopState();
	}
	rect.InsetBy(kEdge, kEdge);
}


void
NotebookControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	// Tracker's column headers, and the Deskbar's items in a menu, are drawn
	// as a button's background alone, and get a button's picture
	if ((view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0)
		|| dynamic_cast<BMenu*>(view) != NULL) {
		if (ShouldDraw(view, rect, updateRect))
			draw_stretched(view, button_art(flags), rect, kEdge);
		return;
	}

	// the frame drew the picture's face
	rect.InsetBy(1, 1);
}


void
NotebookControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);
	BRect indicator(rect);
	indicator.left = indicator.right - kIndicatorWidth + 1;
	draw_arrow(view, indicator, B_DOWN_ARROW, (flags & B_DISABLED) != 0);
	rect.right = indicator.left - 1;
}


//	#pragma mark - menus


//!	Paper with a navy rule along its bottom, as under a notebook's heading.
void
NotebookControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() == NULL || strcmp(view->Name(), "CountVw") != 0) {
		view->SetHighColor(kMenuRule);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}
	view->PopState();
}


//!	A button's outline, with a pencilled arrowhead at its right end.
void
NotebookControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill(view, rect, background);
		pencil(view, rect, flags);
		view->PopState();
	}
	rect.InsetBy(kEdge, kEdge);
}


void
NotebookControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 1;
		view->SetHighColor(kLightPencil);
		view->StrokeLine(BPoint(indicator.left, rect.top + 2),
			BPoint(indicator.left, rect.bottom - 2));
		draw_arrow(view, indicator.OffsetByCopy(1, 0), B_DOWN_ARROW,
			(flags & B_DISABLED) != 0);
		rect.right = indicator.left - 1;
	}
	view->PopState();
}


void
NotebookControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


//!	A sheet of paper with a pencil line round it.
void
NotebookControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	view->SetHighColor(kPencil);
	if ((borders & B_LEFT_BORDER) != 0) {
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		rect.left++;
	}
	if ((borders & B_TOP_BORDER) != 0) {
		view->StrokeLine(rect.LeftTop(), rect.RightTop());
		rect.top++;
	}
	if ((borders & B_RIGHT_BORDER) != 0) {
		view->StrokeLine(rect.RightTop(), rect.RightBottom());
		rect.right--;
	}
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
		rect.bottom--;
	}
	view->PopState();
}


//!	The skin's light grey under a selected item, plain paper otherwise.
void
NotebookControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	view->PushState();
	fill(view, rect, selected ? kMenuSelection : kPaper);
	view->PopState();
}


//	#pragma mark - status bars


//!	A pencilled box, coloured in as far as the bar has got.
void
NotebookControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	pencil(view, rect, 0);
	BRect bar(rect.InsetByCopy(kEdge + 1, kEdge + 1));
	bar.right = std::min(progressPosition, bar.right);
	fill(view, bar, barColor);
	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


void
NotebookControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool on = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, disabled ? (on ? kCheckBoxOnDisabled : kCheckBoxDisabled)
		: (on ? kCheckBoxOn : kCheckBox), r);
}


void
NotebookControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool on = (flags & B_ACTIVATED) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, disabled ? (on ? kRadioOnDisabled : kRadioDisabled)
		: (on ? kRadioOn : kRadio), r);
}


//	#pragma mark - scroll bars


void
NotebookControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(kLightPencil);
	view->StrokeRect(rect);
	view->PopState();
}


//!	A pencilled box with an arrowhead, blue while it's held, as buttons are.
void
NotebookControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	draw_stretched(view, (flags & B_DISABLED) != 0 ? kButtonDisabled
		: down ? kButtonPressed : kButton, rect, kEdge);
	draw_arrow(view, rect, direction, (flags & B_DISABLED) != 0);
	view->PopState();
}


void
NotebookControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


//!	Plain paper.
void
NotebookControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	view->PopState();
}


//!	A pencilled button. A bar with nothing to scroll has none.
void
NotebookControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	view->PopState();
	if ((flags & B_DISABLED) != 0)
		return;
	draw_stretched(view, kButton, rect, kEdge);
}


/*!	A pencil line round the view, as round a field; a plain border is a
	single line.
*/
void
NotebookControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style borderStyle, uint32 flags, uint32 borders)
{
	DrawBorder(view, rect, updateRect, base, borderStyle, flags, borders);

	// the corner between the scroll bars
	if (verticalScrollBarFrame.IsValid() && horizontalScrollBarFrame.IsValid()) {
		BRect corner(verticalScrollBarFrame.left,
			horizontalScrollBarFrame.top, verticalScrollBarFrame.right,
			horizontalScrollBarFrame.bottom);
		view->PushState();
		fill(view, corner & rect, kPaper);
		view->PopState();
	}
}


//	#pragma mark - sliders


//!	A thin pencilled groove, coloured in where the application asks.
void
NotebookControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	pencil(view, rect, flags);
	// an application's fill colour, when it has one; Haiku passes a grey tint
	// of the panel for the part with none
	const bool isGrey = fillColor.red == fillColor.green
		&& fillColor.green == fillColor.blue;
	if (!isGrey && (flags & B_DISABLED) == 0)
		fill(view, rect.InsetByCopy(kEdge, kEdge), fillColor);
	view->PopState();
}


void
NotebookControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	draw_stretched(view, button_art(flags & ~B_FOCUSED), rect, kEdge);
	view->PopState();
}


void
NotebookControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	DrawSliderThumb(view, rect, updateRect, base, flags, orientation);
}


//	#pragma mark - text controls, borders and groups


void
NotebookControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		draw_stretched(view, (flags & B_DISABLED) != 0 ? kButtonDisabled
			: (flags & B_FOCUSED) != 0 ? kButtonFocused : kButton, rect, kEdge,
			false);
		view->PopState();
	}
	rect.InsetBy(kEdge, kEdge);
}


void
NotebookControlLook::DrawBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, border_style borderStyle,
	uint32 flags, uint32 borders)
{
	if (borderStyle == B_NO_BORDER)
		return;

	if (borderStyle == B_PLAIN_BORDER || borders != B_ALL_BORDERS) {
		if (ShouldDraw(view, rect, updateRect)) {
			view->PushState();
			view->SetHighColor(kLightPencil);
			if ((borders & B_LEFT_BORDER) != 0)
				view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
			if ((borders & B_TOP_BORDER) != 0)
				view->StrokeLine(rect.LeftTop(), rect.RightTop());
			if ((borders & B_RIGHT_BORDER) != 0)
				view->StrokeLine(rect.RightTop(), rect.RightBottom());
			if ((borders & B_BOTTOM_BORDER) != 0)
				view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
			view->PopState();
		}
		if ((borders & B_LEFT_BORDER) != 0)
			rect.left++;
		if ((borders & B_TOP_BORDER) != 0)
			rect.top++;
		if ((borders & B_RIGHT_BORDER) != 0)
			rect.right--;
		if ((borders & B_BOTTOM_BORDER) != 0)
			rect.bottom--;
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		pencil(view, rect, flags, false);
		view->PopState();
	}
	rect.InsetBy(kEdge, kEdge);
}


//!	A pencil line round the group.
void
NotebookControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		DrawBorder(view, rect, updateRect, base, B_PLAIN_BORDER, 0, borders);
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		pencil(view, rect, 0, false);
		view->PopState();
	}
	rect.InsetBy(kEdge, kEdge);
}


//	#pragma mark - tabs


//!	The page's edge on the tabs' side, as the rectangle's two rows or columns.
static BRect
page_edge(const BRect& r, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			return BRect(r.left, r.top, r.right, r.top + 1);
		case BTabView::kLeftSide:
			return BRect(r.right - 1, r.top, r.right, r.bottom);
		case BTabView::kRightSide:
			return BRect(r.left, r.top, r.left + 1, r.bottom);
		default:
			return BRect(r.left, r.bottom - 1, r.right, r.bottom);
	}
}


//!	A pencil line along the page's edge, which the tabs stand on.
void
NotebookControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill(view, rect, kPaper);
	if (borderStyle != B_NO_BORDER) {
		const BRect edge = page_edge(rect, side);
		view->SetHighColor(kPencil);
		switch (side) {
			case BTabView::kBottomSide:
				view->StrokeLine(edge.LeftTop(), edge.RightTop());
				break;
			case BTabView::kLeftSide:
				view->StrokeLine(edge.RightTop(), edge.RightBottom());
				break;
			case BTabView::kRightSide:
				view->StrokeLine(edge.LeftTop(), edge.LeftBottom());
				break;
			default:
				view->StrokeLine(edge.LeftBottom(), edge.RightBottom());
				break;
		}
	}
	view->PopState();
}


//!	Where a tab's label goes: inside its outline.
static void
inset_tab(BRect& rect, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			rect.InsetBy(4, 0);
			rect.bottom -= 3;
			break;
		case BTabView::kLeftSide:
			rect.InsetBy(0, 4);
			rect.left += 3;
			break;
		case BTabView::kRightSide:
			rect.InsetBy(0, 4);
			rect.right -= 3;
			break;
		default:
			rect.InsetBy(4, 0);
			rect.top += 3;
			break;
	}
}


/*!	A tab: a button's pencil outline open on the page's side. The chosen one
	stands taller and runs into the page; the others stand on its edge.
*/
static void
draw_tab(BView* view, BRect rect, uint32 side, bool chosen, uint32 flags)
{
	BRect shape(rect);
	// past the page's edge, so that the outline is open there
	switch (side) {
		case BTabView::kBottomSide:
			shape.top -= kEdge;
			if (!chosen)
				shape.bottom -= 2;
			break;
		case BTabView::kLeftSide:
			shape.right += kEdge;
			if (!chosen)
				shape.left += 2;
			break;
		case BTabView::kRightSide:
			shape.left -= kEdge;
			if (!chosen)
				shape.right -= 2;
			break;
		default:
			shape.bottom += kEdge;
			if (!chosen)
				shape.top += 2;
			break;
	}

	view->PushState();
	view->ClipToRect(rect);
	fill(view, rect, kPaper);
	pencil(view, shape, flags);
	if (!chosen) {
		// the page's edge runs on under a tab behind
		const BRect edge = page_edge(rect, side);
		view->SetHighColor(kPencil);
		switch (side) {
			case BTabView::kBottomSide:
				view->StrokeLine(edge.LeftTop(), edge.RightTop());
				break;
			case BTabView::kLeftSide:
				view->StrokeLine(edge.RightTop(), edge.RightBottom());
				break;
			case BTabView::kRightSide:
				view->StrokeLine(edge.LeftTop(), edge.LeftBottom());
				break;
			default:
				view->StrokeLine(edge.LeftBottom(), edge.RightBottom());
				break;
		}
	}
	view->PopState();
}


void
NotebookControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect))
		draw_tab(view, rect, side, true, flags);
	inset_tab(rect, side);
}


void
NotebookControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect))
		draw_tab(view, rect, side, false, flags);
	inset_tab(rect, side);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::NotebookControlLook();
}
