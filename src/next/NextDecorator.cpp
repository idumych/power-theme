/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See NextDecorator.h. Colours and sizes were measured from the pictures of
 * Swizcore Studio's NeXT 2.0 (2004).
 */


#include "NextDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <Path.h>
#include <Rect.h>
#include <View.h>

#include <WindowPrivate.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "PatternHandler.h"


// The title bar's edges: light along its top and left, grey along its bottom
// and right, inside the frame's line. Its fill is the window tab colour.
static const uint8 kBarLight = 0xc0;
static const uint8 kBarShadow = 0x7f;
static const uint8 kInactiveBarLight = 0xbe;
static const uint8 kInactiveBarShadow = 0x5b;

// a box: white top and left edges, a light grey face, a dark grey edge inside a
// black one along its bottom and right, and a black mark. An inactive window's
// is darker all over, and a pressed one is white with a black top and left.
static const uint8 kBoxLight = 0xff;
static const uint8 kBoxFace = 0xaa;
static const uint8 kBoxShadow = 0x55;
static const uint8 kBoxEdge = 0x00;
static const uint8 kDimLight = 0x7f;
static const uint8 kDimFace = 0x55;
static const uint8 kDimShadow = 0x2a;
static const uint8 kDimMarkEdge = 0x2a;

// the resize corner: a grey square with three ridges across it
static const rgb_color kCornerFace = { 0xc1, 0xc1, 0xc1, 255 };
static const rgb_color kRidgeLight = { 0xe4, 0xe5, 0xdf, 255 };
static const rgb_color kRidgeMiddle = { 0x7f, 0x80, 0x7a, 255 };
static const rgb_color kRidgeDark = { 0x5f, 0x5f, 0x5f, 255 };

// the outline's row, the bar's own rows, and the frame's line under it
static const float kTitleHeight = 23;
static const float kSmallTitleHeight = 19;
static const float kLargeBox = 15;
static const float kSmallBox = 13;
static const float kTitleGap = 8;
static const float kResizeCornerLength = 15;
static const float kBorderResizeLength = 22;


static rgb_color
grey(uint8 level)
{
	rgb_color color = { level, level, level, 255 };
	return color;
}


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - NextDecorAddOn


NextDecorAddOn::NextDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name),
	fBeOSTabs(false)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kBeOSTabsSetting) == B_OK) {
		fBeOSTabs = access(path.Path(), F_OK) == 0;
	}
}


Decorator*
NextDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) NextDecorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - NextDecorator


NextDecorator::NextDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


NextDecorator::~NextDecorator()
{
}


void
NextDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
NextDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
NextDecorator::RegionAt(BPoint where, int32& tabIndex) const
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

	if (_HasResizeCorner() && fResizeRect.Contains(where))
		return REGION_RIGHT_BOTTOM_CORNER;

	if (fLeftBorder.Contains(where))
		return REGION_LEFT_BORDER;
	if (fTopBorder.Contains(where))
		return REGION_TOP_BORDER;

	if (fRightBorder.Contains(where))
		region = REGION_RIGHT_BORDER;
	else if (fBottomBorder.Contains(where))
		region = REGION_BOTTOM_BORDER;
	else
		return REGION_NONE;

	if ((fTopTab->flags & B_NOT_RESIZABLE) == 0
		&& (fTopTab->look == B_TITLED_WINDOW_LOOK
			|| fTopTab->look == B_FLOATING_WINDOW_LOOK
			|| fTopTab->look == B_MODAL_WINDOW_LOOK
			|| fTopTab->look == kLeftTitledWindowLook)) {
		const float length = kBorderResizeLength * _Scale();
		BRect resizeRect(BPoint(fBottomBorder.right - length,
			fBottomBorder.bottom - length), fBottomBorder.RightBottom());
		if (resizeRect.Contains(where))
			return REGION_RIGHT_BOTTOM_CORNER;
	}

	return region;
}


void
NextDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
	fFocusFrameColor = settings.UIColor(B_WINDOW_BORDER_COLOR);
	fNonFocusFrameColor = settings.UIColor(B_WINDOW_INACTIVE_BORDER_COLOR);
	fFocusTabColor = settings.UIColor(B_WINDOW_TAB_COLOR);
	fNonFocusTabColor = settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
}


//	#pragma mark - Protected methods


void
NextDecorator::_DoLayout()
{
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	if (fBorderWidth > 0) {
		fLeftBorder.Set(fFrame.left - 1, fFrame.top, fFrame.left - 1,
			fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top, fFrame.right + 1,
			fFrame.bottom);
		fTopBorder.Set(fFrame.left - 1, fFrame.top - 1, fFrame.right + 1,
			fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - 1, fFrame.bottom + 1, fFrame.right + 1,
			fFrame.bottom + 1);
	} else {
		fLeftBorder.Set(0, 0, -1, -1);
		fRightBorder.Set(0, 0, -1, -1);
		fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(0, 0, -1, -1);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	if (_HasResizeCorner()) {
		// the content's corner, as far in as a scroll bar's width
		const float knob = roundf(kResizeCornerLength * _Scale()) + fBorderWidth;
		fResizeRect.Set(fBottomBorder.right - knob,
			fBottomBorder.bottom - knob, fBottomBorder.right,
			fBottomBorder.bottom);
	} else
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
NextDecorator::_DoOutlineLayout()
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


/*!	A dark line round the content. A full title bar draws the top one, as its
	bottom row.
*/
void
NextDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const rgb_color edge = IsFocus(fTopTab)
		? fFocusFrameColor : fNonFocusFrameColor;
	const BRect& r = fBorderRect;

	if (!_HasFullTitleBar())
		fDrawingEngine->StrokeLine(r.LeftTop(), r.RightTop(), edge);
	fDrawingEngine->StrokeLine(BPoint(r.left, r.top + 1), r.LeftBottom(), edge);
	fDrawingEngine->StrokeLine(BPoint(r.right, r.top + 1), r.RightBottom(),
		edge);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), edge);

	if (_HasResizeCorner())
		_DrawResizeCorner();
}


void
NextDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar: the frame's line round its top and sides and along its bottom,
	which is the line over the content, a light edge inside along its top and
	left, a grey one along its bottom and right, and the fill between.
*/
void
NextDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const rgb_color edge = focus ? fFocusFrameColor : fNonFocusFrameColor;
	const rgb_color light = grey(focus ? kBarLight : kInactiveBarLight);
	const rgb_color shadow = grey(focus ? kBarShadow : kInactiveBarShadow);

	fDrawingEngine->StrokeRect(t, edge);
	BRect inside = t.InsetByCopy(1, 1);
	if (inside.IsValid()) {
		fDrawingEngine->StrokeLine(inside.LeftTop(), inside.RightTop(), light);
		fDrawingEngine->StrokeLine(inside.LeftTop(), inside.LeftBottom(), light);
		fDrawingEngine->StrokeLine(BPoint(inside.left + 1, inside.bottom),
			inside.RightBottom(), shadow);
		fDrawingEngine->StrokeLine(BPoint(inside.right, inside.top + 1),
			inside.RightBottom(), shadow);
		inside.InsetBy(1, 1);
		if (inside.IsValid()) {
			fDrawingEngine->FillRect(inside & invalid,
				focus ? fFocusTabColor : fNonFocusTabColor);
		}
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


void
NextDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& t = tab->tabRect;
	const bool focus = IsFocus(tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	const float width = fDrawingEngine->StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength, fDrawState.Font());

	// in the middle of the bar, but clear of the boxes
	float left = t.left + tab->textOffset;
	float right = t.right - tab->textOffset;
	if (fBeOSTabs && tab->zoomRect.IsValid())
		right = tab->zoomRect.left - kTitleGap;

	BPoint position;
	position.x = floorf((t.left + t.right + 1 - width) / 2);
	position.x = std::max(left, std::min(position.x, right - width));
	// in the middle of the fill, between the light and grey edges
	const float top = t.top + 2;
	const float space = t.bottom - 2 - top;
	position.y = floorf(top + (space + fontHeight.ascent - fontHeight.descent)
		/ 2 + 1);

	fDrawingEngine->SetHighColor(focus ? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
NextDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
NextDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->closePressed, GLYPH_CLOSE, !IsFocus(tab));
}


void
NextDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->minimizePressed, GLYPH_MINIMIZE,
		!IsFocus(tab));
}


void
NextDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->zoomPressed, GLYPH_ZOOM, !IsFocus(tab));
}


void
NextDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
NextDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
NextDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	// a title bar always spans the window, and so do a stack's tabs
	if (!fBeOSTabs || fTabList.CountItems() != 1 || !tab->tabRect.IsValid())
		return false;

	const float maxLocation = std::max(0.0f,
		fBorderRect.Width() - tab->tabRect.Width());
	location = std::max(0.0f, std::min(floorf(location), maxLocation));
	if (location == tab->tabLocation)
		return false;

	if (updateRegion != NULL) {
		updateRegion->Include(tab->tabRect);
		updateRegion->Include(fTopBorder);
	}
	tab->tabLocation = location;
	_LayoutTabs();
	if (updateRegion != NULL)
		updateRegion->Include(tab->tabRect);
	return true;
}


bool
NextDecorator::_SetSettings(const BMessage& settings,
	BRegion* updateRegion)
{
	bool modified = false;
	float location;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		if (settings.FindFloat("tab location", i, &location) != B_OK)
			break;
		// a title bar keeps the place of the tab it may become again
		if (!fBeOSTabs && fTabList.CountItems() == 1) {
			fTabList.ItemAt(i)->tabLocation = std::max(0.0f, floorf(location));
			continue;
		}
		modified |= SetTabLocation(i, location, false, updateRegion);
	}
	return modified;
}


void
NextDecorator::_UpdateFont(DesktopSettings& settings)
{
	// the titles are bold, and a utility window's are smaller
	ServerFont font;
	settings.GetDefaultBoldFont(font);
	if (_IsSmall())
		font.SetSize(std::max(9.0f, roundf(font.Size() * 11 / 12)));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
NextDecorator::_MoveBy(BPoint offset)
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
NextDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
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
NextDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
NextDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
NextDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);

	_DoLayout();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


void
NextDecorator::_GetFootprint(BRegion* region)
{
	if (region == NULL || fTopTab->look == B_NO_BORDER_WINDOW_LOOK
		|| fBorderWidth <= 0)
		return;

	region->Include(fTopBorder);
	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fBottomBorder);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;

	if (fTitleBarRect.IsValid())
		region->Include(fTitleBarRect);
	if (_HasResizeCorner())
		region->Include(fResizeRect);
}


//	#pragma mark - Private methods


bool
NextDecorator::_HasTitle() const
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


//!	Whether the title bar spans the window, rather than being a BeOS-style tab.
bool
NextDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its title and its two boxes.
float
NextDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float size = _ButtonSize();
	const float edge = _ButtonEdge();

	float width = 2 * edge + 2 * size + 4 * kTitleGap;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
NextDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
NextDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


float
NextDecorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


float
NextDecorator::_ButtonSize() const
{
	return _IsSmall() ? kSmallBox : kLargeBox;
}


//!	How far the first box is from the bar's left end, the frame's line included.
float
NextDecorator::_ButtonEdge() const
{
	return _IsSmall() ? 5 : 8;
}


float
NextDecorator::_ButtonGap() const
{
	return _IsSmall() ? 5 : 7;
}


//!	How wide a box's mark is: a glyph inside its edges.
float
NextDecorator::_GlyphSpan() const
{
	return _IsSmall() ? 6 : 10;
}


/*!	Where a full title bar's boxes go: close, minimize and zoom, from the left.
	A window that lacks some of them leaves their places empty.
*/
BRect
NextDecorator::_ButtonSlot(const Decorator::Tab* tab, int32 slot) const
{
	const BRect& t = tab->tabRect;
	const float size = _ButtonSize();
	// in the middle of the fill, inside the outline and the light and grey edges
	const float top = t.top + 2 + floorf((t.Height() + 1 - 4 - size) / 2);
	const float left = t.left + _ButtonEdge() + slot * (size + _ButtonGap());
	return BRect(left, top, left + size - 1, top + size - 1);
}


bool
NextDecorator::_HasResizeCorner() const
{
	return fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0;
}


/*!	Lays the tabs out side by side across the whole window, each with its boxes
	at its left end and its title in its middle.

	A BeOS-style tab sits on the frame, over its top line so that they join up,
	with close on the left, zoom on the right and no minimize box. A lone one is
	as wide as its title and can slide; a stack's share the window's width.
*/
void
NextDecorator::_LayoutTabs()
{
	const int32 count = fTabList.CountItems();
	if (count == 0) {
		fTitleBarRect.Set(0, 0, -1, -1);
		return;
	}

	const ServerFont& font = fDrawState.Font();
	const float scale = _Scale();
	float left = fBorderRect.left;
	float width = fBorderRect.Width() + 1;
	const float height = _TitleHeight();
	const float bottom = fBorderRect.top;
	const float top = bottom - height + 1;
	if (fBeOSTabs && count == 1) {
		Decorator::Tab* tab = fTabList.ItemAt(0);
		const float tabWidth = std::min(_TabWidth(tab), width);
		left += std::min(tab->tabLocation, width - tabWidth);
		width = tabWidth;
	}
	const float size = _ButtonSize();
	const float edge = _ButtonEdge();
	const float minTitle = roundf(32 * scale);

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab->tabRect.Set(tabLeft, top, tabRight, bottom);

		const bool closable = (tab->flags & B_NOT_CLOSABLE) == 0;
		const bool zoomable = (tab->flags & B_NOT_ZOOMABLE) == 0;
		const bool minimizable = (tab->flags & B_NOT_MINIMIZABLE) == 0;

		tab->closeRect = closable ? _ButtonSlot(tab, 0) : BRect(0, 0, -1, -1);
		tab->minimizeRect.Set(0, 0, -1, -1);
		tab->zoomRect.Set(0, 0, -1, -1);

		// the title is kept clear of the boxes on both sides, so that it stays
		// in the middle
		float reserved = edge;
		if (fBeOSTabs) {
			if (zoomable) {
				const BRect slot = _ButtonSlot(tab, 0);
				tab->zoomRect.Set(tabRight - edge - size + 1, slot.top,
					tabRight - edge, slot.bottom);
			}
			if (closable || zoomable)
				reserved = edge + size + kTitleGap;
		} else {
			if (minimizable)
				tab->minimizeRect = _ButtonSlot(tab, 1);
			if (zoomable)
				tab->zoomRect = _ButtonSlot(tab, 2);
			if (closable || minimizable || zoomable)
				reserved = _ButtonSlot(tab, 2).right + 1 - tabLeft + kTitleGap;
		}

		const float room = (tabRight - tabLeft + 1) - 2 * reserved;
		const float taken = 2 * reserved;

		tab->tabOffset = (uint32)(tabLeft - fBorderRect.left);
		tab->textOffset = reserved;
		if (fBeOSTabs && count > 1)
			tab->tabLocation = 0;
		tab->minTabSize = 0;
		const char* title = Title(tab);
		tab->maxTabSize = taken + ceilf(font.StringWidth(title, strlen(title)));
		minimum += taken + minTitle;

		tab->truncatedTitle = title;
		font.TruncateString(&tab->truncatedTitle,
			count > 1 ? B_TRUNCATE_END : B_TRUNCATE_MIDDLE,
			std::max(0.0f, room));
		tab->truncatedTitleLength = tab->truncatedTitle.Length();
	}

	// Decorator::GetSizeLimits() keeps the window at least this wide
	fTabList.ItemAt(0)->minTabSize = minimum;

	fTitleBarRect.Set(left, top, left + width - 1, bottom);
}


/*!	On what would be the content's corner: a grey square with three diagonal
	ridges.
*/
void
NextDecorator::_DrawResizeCorner()
{
	const float length = roundf(kResizeCornerLength * _Scale());
	const BRect corner(fFrame.right - length + 1, fFrame.bottom - length + 1,
		fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kCornerFace);

	if (!IsFocus(fTopTab))
		return;

	// each a light, a middle and a dark line from the bottom left up to the
	// right, inside a margin of two at the top left and one at the bottom right
	const rgb_color colors[] = { kRidgeLight, kRidgeMiddle, kRidgeDark };
	const int32 last = (int32)length - 2;
	for (int32 ridge = 0; ridge < 3; ridge++) {
		for (int32 k = 0; k < 3; k++) {
			const int32 sum = last + 2 + 4 * ridge + k;
			const int32 low = std::max((int32)2, sum - last);
			const int32 high = std::min(last, sum - 2);
			if (low > high)
				continue;
			fDrawingEngine->StrokeLine(
				BPoint(corner.left + low, corner.top + sum - low),
				BPoint(corner.left + high, corner.top + sum - high), colors[k]);
		}
	}
}


/*!	Sets a box's pixels by hand, each row in runs of one grey: its white top and
	left, its face, its grey and black bottom and right, and its mark: a cross for
	close, a triangle in the top left for minimize and a window for zoom. Pressed,
	it is white with black along its top and left; an inactive window's is darker.
*/
void
NextDecorator::_DrawBox(Decorator::Tab* tab, bool direct, BRect rect,
	bool pressed, glyph glyph, bool dim)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const int32 size = (int32)rect.Width() + 1;
	const int32 span = (int32)_GlyphSpan();
	// the mark's first and last rows and columns
	const int32 first = (size - span) / 2;
	const int32 last = first + span - 1;
	// how far down the triangle reaches: one row short of the mark's square
	const int32 corner = first + last - (_IsSmall() ? 0 : 1);

	const uint8 light = dim ? kDimLight : kBoxLight;
	const uint8 face = dim ? kDimFace : kBoxFace;
	const uint8 shadow = dim ? kDimShadow : kBoxShadow;
	const uint8 markEdge = dim ? kDimMarkEdge : kBoxShadow;

	for (int32 y = 0; y < size; y++) {
		const float rowY = rect.top + y;
		int32 start = 0;
		uint8 runLevel = 0;
		for (int32 x = 0; x <= size; x++) {
			uint8 level = 0;
			if (x < size) {
				if (pressed) {
					if (y == 0 || x == 0)
						level = x == size - 1 || y == size - 1 ? kBoxLight : kBoxEdge;
					else
						level = kBoxLight;
				} else if (y == size - 1 || x == size - 1)
					level = kBoxEdge;
				else if (y == 0 || x == 0)
					level = light;
				else if (y == size - 2 || x == size - 2)
					level = shadow;
				else
					level = face;

				const bool inMark = x >= first && x <= last && y >= first
					&& y <= last;
				if (inMark) {
					const uint8 mark = kBoxEdge;
					const uint8 soft = pressed ? kBoxShadow : markEdge;
					switch (glyph) {
						case GLYPH_CLOSE:
						{
							const int32 d1 = abs(x - y);
							const int32 d2 = abs(x + y - first - last);
							if (d1 == 0 || d2 == 0)
								level = mark;
							else if (d1 == 1 || d2 == 1)
								level = soft;
							break;
						}
						case GLYPH_MINIMIZE:
							if (x + y <= corner)
								level = mark;
							break;
						case GLYPH_ZOOM:
							if (y <= first + 2 || y == last || x == first
								|| x == last)
								level = mark;
							break;
						default:
							break;
					}
				}
			}
			if (x == 0)
				runLevel = level;
			if (x == size || level != runLevel) {
				fDrawingEngine->FillRect(BRect(rect.left + start, rowY,
					rect.left + x - 1, rowY), grey(runLevel));
				start = x;
				runLevel = level;
			}
		}
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) NextDecorAddOn(id, name);
}
