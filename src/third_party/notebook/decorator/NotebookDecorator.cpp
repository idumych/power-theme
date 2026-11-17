/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See NotebookDecorator.h. The pictures are Hideaki Kamada's Notebook skin for
 * WindowBlinds (NotebookWindowArt.h, from ../art), placed as its notebook.uis
 * says.
 */


#include "NotebookDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Rect.h>
#include <View.h>

#include <WindowPrivate.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "PatternHandler.h"

#include "NotebookWindowArt.h"


// From notebook.uis. The title bar: its height, its left end and right end
// drawn as they are and the part between repeated, and its buttons, the close
// box from the left and the others from the right.
static const float kTitleHeight = 28;
static const float kTitleLeftEnd = 20;
static const float kTitleRightEnd = 40;
static const float kCloseLeft = 27;
static const float kMinimizeRight = 47;
static const float kZoomRight = 32;
static const float kButtonTop = 7;
static const float kButtonSize = 13;
// the title, in the middle, two pixels higher than the bar's
static const float kTitleShift = 2;
// the binding down the left: its top and bottom drawn as they are, the holes
// between repeated
static const float kLeftWidth = 20;
static const float kLeftTop = 18;
static const float kLeftBottom = 25;
// the rounded edge on the right
static const float kRightWidth = 7;
static const float kRightTop = 8;
static const float kRightBottom = 10;
// the pages' edge along the bottom
static const float kBottomHeight = 25;
// the rows of the title bar's top a window without one has, its first line
// and some paper
static const float kPlainTop = 3;
// how far along the bottom and right edges from the corner resizing both
static const float kResizeLength = 30;

// the paper, which most of each picture is
static const uint32 kPaper = 0xffffffff;


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


//	#pragma mark - NotebookDecorAddOn


NotebookDecorAddOn::NotebookDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
NotebookDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) NotebookDecorator(settings, rect, desktop);
}


//	#pragma mark - NotebookDecorator


NotebookDecorator::NotebookDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
	:
	Decorator(settings, rect, desktop)
{
	UpdateColors(settings);
}


NotebookDecorator::~NotebookDecorator()
{
}


void
NotebookDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
NotebookDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
NotebookDecorator::RegionAt(BPoint where, int32& tabIndex) const
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		if (fTabList.ItemAt(i)->minimizeRect.Contains(where)) {
			tabIndex = i;
			return REGION_MINIMIZE_BUTTON;
		}
	}

	// close, zoom and the tab itself
	Region region = Decorator::RegionAt(where, tabIndex);
	if (region != REGION_NONE)
		return region;

	const bool resizable = (fTopTab->flags & B_NOT_RESIZABLE) == 0
		&& fTopTab->look != B_BORDERED_WINDOW_LOOK;
	if (resizable && (fRightBorder.Contains(where)
			|| fBottomBorder.Contains(where))
		&& where.x > fBorderRect.right - kResizeLength
		&& where.y > fBorderRect.bottom - kResizeLength)
		return REGION_RIGHT_BOTTOM_CORNER;

	if (fLeftBorder.Contains(where))
		return REGION_LEFT_BORDER;
	if (fTopBorder.Contains(where))
		return REGION_TOP_BORDER;
	if (fRightBorder.Contains(where))
		return REGION_RIGHT_BORDER;
	if (fBottomBorder.Contains(where))
		return REGION_BOTTOM_BORDER;
	return REGION_NONE;
}


void
NotebookDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
}


//	#pragma mark - Protected methods


void
NotebookDecorator::_DoLayout()
{
	fBorderWidth = 0;

	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			// the narrowest side, for what asks for one width
			fBorderWidth = (int32)kRightWidth;
			break;
		case B_BORDERED_WINDOW_LOOK:
			// a pencil line, as the Deskbar and the like need no binding
			fBorderWidth = 1;
			break;
	}

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		fLeftBorder.Set(fFrame.left - 1, fFrame.top, fFrame.left - 1,
			fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top, fFrame.right + 1,
			fFrame.bottom);
		fTopBorder.Set(fFrame.left - 1, fFrame.top - 1, fFrame.right + 1,
			fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - 1, fFrame.bottom + 1,
			fFrame.right + 1, fFrame.bottom + 1);
		fBorderRect = fFrame.InsetByCopy(-1, -1);
	} else if (fBorderWidth > 0) {
		const float top = _HasTitle() ? 0 : kPlainTop;
		fLeftBorder.Set(fFrame.left - kLeftWidth, fFrame.top,
			fFrame.left - 1, fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top,
			fFrame.right + kRightWidth, fFrame.bottom);
		if (top > 0) {
			fTopBorder.Set(fFrame.left - kLeftWidth, fFrame.top - top,
				fFrame.right + kRightWidth, fFrame.top - 1);
		} else
			fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(fFrame.left - kLeftWidth, fFrame.bottom + 1,
			fFrame.right + kRightWidth, fFrame.bottom + kBottomHeight);
		fBorderRect.Set(fLeftBorder.left, fFrame.top - top, fRightBorder.right,
			fBottomBorder.bottom);
	} else {
		fLeftBorder.Set(0, 0, -1, -1);
		fRightBorder.Set(0, 0, -1, -1);
		fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(0, 0, -1, -1);
		fBorderRect.Set(0, 0, -1, -1);
	}

	// the frame's edges resize; there's no grip in the content's corner
	fResizeRect.Set(0, 0, -1, -1);

	if (_HasTitle()) {
		_LayoutTabs();
		return;
	}

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		tab->tabRect.Set(0, 0, -1, -1);
		tab->closeRect.Set(0, 0, -1, -1);
		tab->zoomRect.Set(0, 0, -1, -1);
		tab->minimizeRect.Set(0, 0, -1, -1);
	}
	fTitleBarRect.Set(0, 0, -1, -1);
}


void
NotebookDecorator::_DoOutlineLayout()
{
	fOutlineBorderWidth = 1;

	fLeftOutlineBorder.Set(fFrame.left - fOutlineBorderWidth, fFrame.top,
		fFrame.left - 1, fFrame.bottom);
	fRightOutlineBorder.Set(fFrame.right + 1, fFrame.top,
		fFrame.right + fOutlineBorderWidth, fFrame.bottom);
	fTopOutlineBorder.Set(fFrame.left - fOutlineBorderWidth,
		fFrame.top - fOutlineBorderWidth, fFrame.right + fOutlineBorderWidth,
		fFrame.top - 1);
	fBottomOutlineBorder.Set(fFrame.left - fOutlineBorderWidth,
		fFrame.bottom + 1, fFrame.right + fOutlineBorderWidth,
		fFrame.bottom + fOutlineBorderWidth);

	fOutlineBorderRect = BRect(fTopOutlineBorder.LeftTop(),
		fBottomOutlineBorder.RightBottom());
}


//!	The binding, the right edge, the bottom, and a top for a window without a bar.
void
NotebookDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		const rgb_color pencil = { 115, 115, 115, 255 };
		fDrawingEngine->StrokeRect(fBorderRect, pencil);
		return;
	}

	std::vector<Piece> pieces;
	_FramePieces(pieces, IsFocus(fTopTab));
	for (size_t i = 0; i < pieces.size(); i++)
		_Draw(pieces[i], rect);
}


void
NotebookDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	A tab's part of the title bar: the bar's picture, its left end at the bar's
	left and its right end at the bar's right, so that a stack's tabs make one
	bar, each scribbled on unless it has the focus.
*/
void
NotebookDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	std::vector<Piece> pieces;
	_BarPieces(pieces, t, IsFocus(tab));
	for (size_t i = 0; i < pieces.size(); i++)
		_Draw(pieces[i], invalid);

	// a line between a stack's tabs, as the bar's top line
	if (fTabList.CountItems() > 1 && t.right < fTitleBarRect.right) {
		const rgb_color line = { 140, 140, 140, 255 };
		fDrawingEngine->StrokeLine(BPoint(t.right, t.top + 3),
			BPoint(t.right, t.bottom - 6), line);
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


//!	Where the title goes: in the middle of the bar, clear of the buttons.
BRect
NotebookDecorator::_TitleRect(const Decorator::Tab* tab) const
{
	const BRect& t = tab->tabRect;
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	const float width = fDrawState.Font().StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength);

	const float left = t.left + tab->textOffset;
	const float right = t.right - tab->textOffset;
	float x = floorf((t.left + t.right + 1 - width) / 2);
	x = std::max(left, std::min(x, right - width));
	const float middle = t.top + (kTitleHeight - 4) / 2 - kTitleShift + 2;
	const float baseline = floorf(middle + (fontHeight.ascent
		- fontHeight.descent) / 2 + 0.5f);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


void
NotebookDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect title = _TitleRect(tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());
	fDrawingEngine->SetHighColor(IsFocus(tab) ? fFocusTextColor
		: fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		BPoint(title.left, title.bottom));
	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
NotebookDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
NotebookDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, kClose, kClosePressed, kCloseInactive,
		tab->closePressed);
}


void
NotebookDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, kMin, kMinPressed, kMinInactive,
		tab->minimizePressed);
}


void
NotebookDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, kMax, kMaxPressed, kMaxInactive,
		tab->zoomPressed);
}


void
NotebookDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
	BRegion* updateRegion)
{
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion == NULL)
		return;

	BRegion footprint;
	_GetFootprint(&footprint);
	updateRegion->Include(&footprint);
}


void
NotebookDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
NotebookDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	// the title bar always spans the window
	return false;
}


bool
NotebookDecorator::_SetSettings(const BMessage& settings,
	BRegion* updateRegion)
{
	// the bar keeps the place of the tab it may become under another border
	float location;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		if (settings.FindFloat("tab location", i, &location) != B_OK)
			break;
		fTabList.ItemAt(i)->tabLocation = std::max(0.0f, floorf(location));
	}
	return false;
}


void
NotebookDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	settings.GetDefaultBoldFont(font);
	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
NotebookDecorator::_MoveBy(BPoint offset)
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		offset_rect(tab->tabRect, offset);
		offset_rect(tab->closeRect, offset);
		offset_rect(tab->zoomRect, offset);
		offset_rect(tab->minimizeRect, offset);
	}

	fFrame.OffsetBy(offset);
	offset_rect(fTitleBarRect, offset);
	offset_rect(fResizeRect, offset);
	offset_rect(fBorderRect, offset);
	offset_rect(fLeftBorder, offset);
	offset_rect(fRightBorder, offset);
	offset_rect(fTopBorder, offset);
	offset_rect(fBottomBorder, offset);
}


void
NotebookDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// the pictures repeat from the top left, so the bottom and right move
	if (dirty != NULL) {
		BRegion before;
		_GetFootprint(&before);
		dirty->Include(&before);
	}

	fFrame.right += offset.x;
	fFrame.bottom += offset.y;
	_DoLayout();

	if (dirty != NULL) {
		BRegion after;
		_GetFootprint(&after);
		dirty->Include(&after);
	}
}


bool
NotebookDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);

	_UpdateFont(settings);
	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
NotebookDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);

	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
NotebookDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);

	_DoLayout();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


//!	The frame and the bar, without the binding's holes and the rounded corners.
void
NotebookDecorator::_GetFootprint(BRegion* region)
{
	if (region == NULL || fTopTab->look == B_NO_BORDER_WINDOW_LOOK
		|| fBorderWidth <= 0)
		return;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		region->Include(fTopBorder);
		region->Include(fLeftBorder);
		region->Include(fRightBorder);
		region->Include(fBottomBorder);
		return;
	}

	std::vector<Piece> pieces;
	_FramePieces(pieces, true);
	for (size_t i = 0; i < pieces.size(); i++)
		_AddOpaque(region, pieces[i]);

	if (fTitleBarRect.IsValid())
		region->Include(fTitleBarRect);
}


//	#pragma mark - Private methods


bool
NotebookDecorator::_HasTitle() const
{
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			return true;
	}
	return false;
}


/*!	Lays the tabs out side by side across the bar, which spans the window from
	the binding to the right edge, each with its own buttons where the skin has
	them and its title in its middle.
*/
void
NotebookDecorator::_LayoutTabs()
{
	const int32 count = fTabList.CountItems();
	if (count == 0) {
		fTitleBarRect.Set(0, 0, -1, -1);
		return;
	}

	const ServerFont& font = fDrawState.Font();
	const float left = fBorderRect.left;
	const float width = fBorderRect.Width() + 1;
	const float bottom = fFrame.top - 1;
	const float top = bottom - kTitleHeight + 1;
	// the title is kept as far from both ends, so that it stays in the middle
	const float reserved = kMinimizeRight + 4;
	const float minTitle = 32;

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab->tabRect.Set(tabLeft, top, tabRight, bottom);

		const float buttonTop = top + kButtonTop;
		const float buttonBottom = buttonTop + kButtonSize - 1;
		if ((tab->flags & B_NOT_CLOSABLE) == 0) {
			tab->closeRect.Set(tabLeft + kCloseLeft, buttonTop,
				tabLeft + kCloseLeft + kButtonSize - 1, buttonBottom);
		} else
			tab->closeRect.Set(0, 0, -1, -1);
		if ((tab->flags & B_NOT_MINIMIZABLE) == 0) {
			tab->minimizeRect.Set(tabRight + 1 - kMinimizeRight, buttonTop,
				tabRight - kMinimizeRight + kButtonSize, buttonBottom);
		} else
			tab->minimizeRect.Set(0, 0, -1, -1);
		if ((tab->flags & B_NOT_ZOOMABLE) == 0) {
			tab->zoomRect.Set(tabRight + 1 - kZoomRight, buttonTop,
				tabRight - kZoomRight + kButtonSize, buttonBottom);
		} else
			tab->zoomRect.Set(0, 0, -1, -1);

		tab->tabOffset = (uint32)(tabLeft - fBorderRect.left);
		tab->textOffset = reserved;
		tab->minTabSize = 0;
		const char* title = Title(tab);
		tab->maxTabSize = 2 * reserved
			+ ceilf(font.StringWidth(title, strlen(title)));
		minimum += 2 * reserved + minTitle;

		tab->truncatedTitle = title;
		font.TruncateString(&tab->truncatedTitle,
			count > 1 ? B_TRUNCATE_END : B_TRUNCATE_MIDDLE,
			std::max(0.0f, tabRight - tabLeft + 1 - 2 * reserved));
		tab->truncatedTitleLength = tab->truncatedTitle.Length();
	}

	// Decorator::GetSizeLimits() keeps the window at least this wide
	fTabList.ItemAt(0)->minTabSize = minimum;

	fTitleBarRect.Set(left, top, left + width - 1, bottom);
}


/*!	The frame's pieces: the binding between the bar and the bottom, the right
	edge as far down, the pages' edge across the bottom, and for a window
	without a bar the top of the bar's picture.
*/
void
NotebookDecorator::_FramePieces(std::vector<Piece>& pieces, bool focus) const
{
	if (fTopBorder.IsValid())
		_BarPieces(pieces, fTopBorder, focus);

	const Art& left = focus ? kLeftActive : kLeftInactive;
	const BRect& l = fLeftBorder;
	if (l.IsValid()) {
		const float topEnd = std::min(l.top + kLeftTop - 1, l.bottom);
		const float bottomStart = std::max(l.bottom - kLeftBottom + 1,
			topEnd + 1);
		pieces.push_back(Piece{ &left,
			BRect(0, 0, kLeftWidth - 1, kLeftTop - 1), l.LeftTop(),
			BRect(l.left, l.top, l.right, topEnd) });
		_Repeat(pieces, left, BRect(0, kLeftTop, kLeftWidth - 1,
				left.height - kLeftBottom - 1),
			BRect(l.left, topEnd + 1, l.right, bottomStart - 1), l);
		pieces.push_back(Piece{ &left,
			BRect(0, left.height - kLeftBottom, kLeftWidth - 1,
				left.height - 1),
			BPoint(l.left, l.bottom - kLeftBottom + 1),
			BRect(l.left, bottomStart, l.right, l.bottom) });
	}

	const Art& right = focus ? kRightActive : kRightInactive;
	const BRect& r = fRightBorder;
	if (r.IsValid()) {
		const float topEnd = std::min(r.top + kRightTop - 1, r.bottom);
		const float bottomStart = std::max(r.bottom - kRightBottom + 1,
			topEnd + 1);
		pieces.push_back(Piece{ &right,
			BRect(0, 0, kRightWidth - 1, kRightTop - 1), r.LeftTop(),
			BRect(r.left, r.top, r.right, topEnd) });
		_Repeat(pieces, right, BRect(0, kRightTop, kRightWidth - 1,
				right.height - kRightBottom - 1),
			BRect(r.left, topEnd + 1, r.right, bottomStart - 1), r);
		pieces.push_back(Piece{ &right,
			BRect(0, right.height - kRightBottom, kRightWidth - 1,
				right.height - 1),
			BPoint(r.left, r.bottom - kRightBottom + 1),
			BRect(r.left, bottomStart, r.right, r.bottom) });
	}

	const Art& bottom = focus ? kBottomActive : kBottomInactive;
	if (fBottomBorder.IsValid()) {
		_Repeat(pieces, bottom, BRect(0, 0, bottom.width - 1,
			bottom.height - 1), fBottomBorder, fBottomBorder);
	}
}


/*!	The title bar's picture over the whole bar, as much of it as is in clip:
	its ends at the bar's ends and its middle repeated between them. A window
	without a bar gets its top rows, from the frame's top border.
*/
void
NotebookDecorator::_BarPieces(std::vector<Piece>& pieces, const BRect& clip,
	bool focus) const
{
	const Art& art = focus ? kTopActive : kTopInactive;
	const BRect bar = fTitleBarRect.IsValid() ? fTitleBarRect : fTopBorder;
	if (!bar.IsValid())
		return;
	const float height = std::min(bar.Height(), kTitleHeight - 1);

	const float leftEnd = std::min(bar.left + kTitleLeftEnd - 1, bar.right);
	const float rightStart = std::max(bar.right - kTitleRightEnd + 1,
		leftEnd + 1);
	pieces.push_back(Piece{ &art, BRect(0, 0, kTitleLeftEnd - 1, height),
		bar.LeftTop(), BRect(bar.left, bar.top, leftEnd, bar.bottom) & clip });
	_Repeat(pieces, art, BRect(kTitleLeftEnd, 0,
			art.width - kTitleRightEnd - 1, height),
		BRect(leftEnd + 1, bar.top, rightStart - 1, bar.bottom), clip);
	pieces.push_back(Piece{ &art,
		BRect(art.width - kTitleRightEnd, 0, art.width - 1, height),
		BPoint(bar.right - kTitleRightEnd + 1, bar.top),
		BRect(rightStart, bar.top, bar.right, bar.bottom) & clip });
}


//!	Copies of a part of a picture over area, from its top left, inside clip.
void
NotebookDecorator::_Repeat(std::vector<Piece>& pieces, const Art& art,
	BRect source, const BRect& area, const BRect& clip)
{
	const BRect inside = area & clip;
	if (!inside.IsValid() || !source.IsValid())
		return;
	const float width = source.Width() + 1;
	const float height = source.Height() + 1;
	for (float y = area.top; y <= area.bottom; y += height) {
		for (float x = area.left; x <= area.right; x += width)
			pieces.push_back(Piece{ &art, source, BPoint(x, y), inside });
	}
}


/*!	Sets a piece's pixels, those inside its clip and clip: each row's paper in
	runs, then the other colours over it in runs of one colour. Transparent
	pixels are left alone.
*/
void
NotebookDecorator::_Draw(const Piece& piece, const BRect& clip)
{
	const BRect area(piece.where.x, piece.where.y,
		piece.where.x + piece.source.Width(),
		piece.where.y + piece.source.Height());
	const BRect inside = area & piece.clip & clip;
	if (!inside.IsValid())
		return;

	const Art& art = *piece.art;
	const rgb_color paper = { (uint8)(kPaper >> 16), (uint8)(kPaper >> 8),
		(uint8)kPaper, 255 };
	const int32 firstRow = (int32)(inside.top - piece.where.y + piece.source.top);
	const int32 lastRow = (int32)(inside.bottom - piece.where.y
		+ piece.source.top);
	const int32 first = (int32)(inside.left - piece.where.x + piece.source.left);
	const int32 last = (int32)(inside.right - piece.where.x + piece.source.left);
	const float dx = piece.where.x - piece.source.left;
	const float dy = piece.where.y - piece.source.top;

	for (int32 y = firstRow; y <= lastRow; y++) {
		const uint32* row = art.pixels + y * art.width;
		// the paper under each run of opaque pixels
		int32 start = first;
		while (start <= last) {
			if ((row[start] >> 24) < 128) {
				start++;
				continue;
			}
			int32 end = start;
			while (end + 1 <= last && (row[end + 1] >> 24) >= 128)
				end++;
			fDrawingEngine->FillRect(BRect(start + dx, y + dy, end + dx,
				y + dy), paper);
			start = end + 1;
		}
		// the rest over it
		start = first;
		while (start <= last) {
			const uint32 pixel = row[start];
			int32 end = start;
			while (end + 1 <= last && row[end + 1] == pixel)
				end++;
			if (pixel != kPaper && (pixel >> 24) >= 128) {
				const rgb_color color = { (uint8)(pixel >> 16),
					(uint8)(pixel >> 8), (uint8)pixel, 255 };
				fDrawingEngine->FillRect(BRect(start + dx, y + dy, end + dx,
					y + dy), color);
			}
			start = end + 1;
		}
	}
}


/*!	Adds a piece's opaque pixels to region, in rectangles as tall as the rows
	that are alike allow.
*/
void
NotebookDecorator::_AddOpaque(BRegion* region, const Piece& piece)
{
	const BRect area(piece.where.x, piece.where.y,
		piece.where.x + piece.source.Width(),
		piece.where.y + piece.source.Height());
	const BRect inside = area & piece.clip;
	if (!inside.IsValid())
		return;

	const Art& art = *piece.art;
	const int32 first = (int32)(inside.left - piece.where.x + piece.source.left);
	const int32 last = (int32)(inside.right - piece.where.x + piece.source.left);
	const int32 firstRow = (int32)(inside.top - piece.where.y + piece.source.top);
	const int32 lastRow = (int32)(inside.bottom - piece.where.y
		+ piece.source.top);
	const float dx = piece.where.x - piece.source.left;
	const float dy = piece.where.y - piece.source.top;

	// whether two rows are opaque in the same places
	struct Alike {
		static bool Rows(const uint32* a, const uint32* b, int32 first,
			int32 last)
		{
			for (int32 x = first; x <= last; x++) {
				if (((a[x] >> 24) >= 128) != ((b[x] >> 24) >= 128))
					return false;
			}
			return true;
		}
	};

	int32 y = firstRow;
	while (y <= lastRow) {
		const uint32* row = art.pixels + y * art.width;
		int32 end = y;
		while (end + 1 <= lastRow && Alike::Rows(row,
				art.pixels + (end + 1) * art.width, first, last))
			end++;
		int32 start = first;
		while (start <= last) {
			if ((row[start] >> 24) < 128) {
				start++;
				continue;
			}
			int32 stop = start;
			while (stop + 1 <= last && (row[stop + 1] >> 24) >= 128)
				stop++;
			region->Include(BRect(start + dx, y + dy, stop + dx, end + dy));
			start = stop + 1;
		}
		y = end + 1;
	}
}


void
NotebookDecorator::_DrawButton(Decorator::Tab* tab, bool direct, BRect rect,
	const Art& normal, const Art& pressed, const Art& inactive, bool down)
{
	if (!rect.IsValid())
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);
	const Art& art = !tab->buttonFocus ? inactive : down ? pressed : normal;
	const Piece piece = { &art, BRect(0, 0, art.width - 1, art.height - 1),
		rect.LeftTop(), rect };
	_Draw(piece, rect);
	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) NotebookDecorAddOn(id, name);
}
