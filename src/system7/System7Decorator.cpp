/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See System7Decorator.h. Colours and sizes were measured from screenshots of
 * Mac OS 7.5.5 with its standard window colour, on toastytech.com.
 */


#include "System7Decorator.h"

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


// the window colour: the bar's light top and left, its shaded bottom and
// right, the boxes' dark edges, and the grow box's faces
static const rgb_color kLight = { 0xce, 0xce, 0xff, 255 };
static const rgb_color kShade = { 0xa5, 0xa5, 0xd6, 255 };
static const rgb_color kDark = { 0x31, 0x31, 0x63, 255 };
static const rgb_color kBoxFace = { 0x9c, 0x9c, 0x9c, 255 };
static const rgb_color kSmallSquareFace = { 0xbd, 0xbd, 0xbd, 255 };
// the bar's stripes, every other row
static const rgb_color kStripe = { 0x73, 0x73, 0x73, 255 };
static const rgb_color kCornerFace = { 0xef, 0xef, 0xef, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };

// the outline's row, the bar's own rows and the line over the content
static const float kTitleHeight = 19;
static const float kSmallTitleHeight = 15;
static const float kLargeBox = 11;
static const float kSmallBox = 9;
// the plain bar either side of the title
static const float kTitleGap = 7;
static const float kResizeCornerLength = 15;
static const float kBorderResizeLength = 22;


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - System7DecorAddOn


System7DecorAddOn::System7DecorAddOn(image_id id, const char* name)
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
System7DecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) System7Decorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - System7Decorator


System7Decorator::System7Decorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


System7Decorator::~System7Decorator()
{
}


void
System7Decorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
	_DrawShadow(updateRect);
}


void
System7Decorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
	_DrawShadow(BRect(-1e6, -1e6, 1e6, 1e6));
}


Decorator::Region
System7Decorator::RegionAt(BPoint where, int32& tabIndex) const
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
System7Decorator::UpdateColors(DesktopSettings& settings)
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
System7Decorator::_DoLayout()
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
System7Decorator::_DoOutlineLayout()
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
System7Decorator::_DrawFrame(BRect rect)
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
System7Decorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar: the frame's line round its top and sides and along its bottom,
	which is the line over the content. Active, it has a light edge along its
	top and left and a shaded one along its bottom and right, and stripes every
	other row, broken round the boxes and the title; inactive, it is plain.
*/
void
System7Decorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const rgb_color edge = focus ? fFocusFrameColor : fNonFocusFrameColor;

	fDrawingEngine->StrokeRect(t, edge);
	BRect inside = t.InsetByCopy(1, 1);
	if (!inside.IsValid())
		return;

	if (!focus) {
		fDrawingEngine->FillRect(inside & invalid, fNonFocusTabColor);
		_DrawTitle(tab, t);
		return;
	}

	fDrawingEngine->FillRect(inside & invalid, fFocusTabColor);
	fDrawingEngine->StrokeLine(inside.LeftTop(),
		BPoint(inside.right - 1, inside.top), kLight);
	fDrawingEngine->StrokeLine(inside.LeftTop(),
		BPoint(inside.left, inside.bottom - 1), kLight);
	fDrawingEngine->StrokeLine(BPoint(inside.right, inside.top),
		inside.RightBottom(), kShade);
	fDrawingEngine->StrokeLine(inside.LeftBottom(), inside.RightBottom(),
		kShade);

	// the stripes, from the third row inside the edges to as far from the
	// bottom, with a gap round each box and the title
	BRect title = _TitleRect(tab);
	title.InsetBy(-kTitleGap, 0);
	BRect gaps[3] = { tab->closeRect.InsetByCopy(-1, -1),
		tab->zoomRect.InsetByCopy(-1, -1), title };
	for (float y = t.top + 4; y <= t.bottom - 4; y += 2) {
		if (y < invalid.top || y > invalid.bottom)
			continue;
		float x = t.left + 2;
		const float end = t.right - 2;
		while (x <= end) {
			float stop = end;
			float next = end + 1;
			for (int32 i = 0; i < 3; i++) {
				const BRect& gap = gaps[i];
				if (!gap.IsValid() || y < gap.top || y > gap.bottom)
					continue;
				if (gap.right >= x && gap.left - 1 < stop) {
					stop = gap.left - 1;
					next = gap.right + 1;
				}
			}
			if (stop >= x) {
				fDrawingEngine->StrokeLine(BPoint(x, y), BPoint(stop, y),
					kStripe);
			}
			x = std::max(next, x + 1);
		}
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


/*!	Where the title goes: in the middle of the bar, but clear of the boxes, and
	with its baseline as far up as Chicago's was.
*/
BRect
System7Decorator::_TitleRect(const Decorator::Tab* tab) const
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
	// in the middle of the plain rows inside the edges
	const float top = t.top + 2;
	const float space = t.bottom - 2 - top;
	const float baseline = floorf(top + (space + fontHeight.ascent
		- fontHeight.descent) / 2 + 1);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


void
System7Decorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const bool focus = IsFocus(tab);
	const BRect title = _TitleRect(tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());
	fDrawingEngine->SetHighColor(focus ? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		BPoint(title.left, title.bottom));
	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
System7Decorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
System7Decorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->closePressed, GLYPH_CLOSE, !IsFocus(tab));
}


void
System7Decorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->minimizePressed, GLYPH_MINIMIZE,
		!IsFocus(tab));
}


void
System7Decorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->zoomPressed, GLYPH_ZOOM, !IsFocus(tab));
}


void
System7Decorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
System7Decorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
System7Decorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
System7Decorator::_SetSettings(const BMessage& settings,
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
System7Decorator::_UpdateFont(DesktopSettings& settings)
{
	// the titles are bold, as Chicago looked, and a utility window's smaller
	ServerFont font;
	settings.GetDefaultBoldFont(font);
	if (_IsSmall())
		font.SetSize(std::max(9.0f, roundf(font.Size() * 11 / 12)));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
System7Decorator::_MoveBy(BPoint offset)
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
System7Decorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
System7Decorator::_AddTab(DesktopSettings& settings, int32 index,
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
System7Decorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
System7Decorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
System7Decorator::_GetFootprint(BRegion* region)
{
	if (region == NULL || fTopTab->look == B_NO_BORDER_WINDOW_LOOK
		|| fBorderWidth <= 0)
		return;

	region->Include(fTopBorder);
	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fBottomBorder);

	BRegion shadow;
	_GetShadow(&shadow);
	region->Include(&shadow);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;

	if (fTitleBarRect.IsValid())
		region->Include(fTitleBarRect);
	if (_HasResizeCorner())
		region->Include(fResizeRect);
}


/*!	The drop shadow: a line one pixel to the right of and below the frame and
	each tab, starting a pixel down and in, as a window's in System 7 had.
*/
void
System7Decorator::_GetShadow(BRegion* region) const
{
	if (fBorderWidth <= 0 || !fBorderRect.IsValid())
		return;

	const BRect& r = fBorderRect;
	region->Include(BRect(r.right + 1, r.top + 1, r.right + 1, r.bottom + 1));
	region->Include(BRect(r.left + 1, r.bottom + 1, r.right + 1, r.bottom + 1));
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		const BRect& t = fTabList.ItemAt(i)->tabRect;
		if (t.IsValid() && t.right >= r.right)
			region->Include(BRect(t.right + 1, t.top + 1, t.right + 1, t.bottom));
		else if (t.IsValid())
			region->Include(BRect(t.right + 1, t.top + 1, t.right + 1,
				t.bottom - 1));
	}
}


void
System7Decorator::_DrawShadow(BRect invalid)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	BRegion shadow;
	_GetShadow(&shadow);
	BRegion clip(invalid);
	shadow.IntersectWith(&clip);
	const rgb_color color = IsFocus(fTopTab) ? fFocusFrameColor
		: fNonFocusFrameColor;
	for (int32 i = 0; i < shadow.CountRects(); i++)
		fDrawingEngine->FillRect(shadow.RectAt(i), color);
}


//	#pragma mark - Private methods


bool
System7Decorator::_HasTitle() const
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
System7Decorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its title and its two boxes.
float
System7Decorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float size = _ButtonSize();
	const float edge = _ButtonEdge();

	float width = 2 * edge + 2 * size + 2 * kTitleGap + 2 * 4;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
System7Decorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
System7Decorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


float
System7Decorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


float
System7Decorator::_ButtonSize() const
{
	return _IsSmall() ? kSmallBox : kLargeBox;
}


//!	How far the first box is from the bar's left end, the frame's line included.
float
System7Decorator::_ButtonEdge() const
{
	return _IsSmall() ? 7 : 9;
}


float
System7Decorator::_ButtonGap() const
{
	return 0;
}


float
System7Decorator::_GlyphSpan() const
{
	return 0;
}


/*!	Where a title bar's boxes go: close at its left end (slot 0) and zoom at
	its right (slot 1), as far in from each end.
*/
BRect
System7Decorator::_ButtonSlot(const Decorator::Tab* tab, int32 slot) const
{
	const BRect& t = tab->tabRect;
	const float size = _ButtonSize();
	// in the middle of the rows inside the light and shaded edges
	const float top = t.top + 2 + floorf((t.Height() + 1 - 4 - size) / 2);
	const float left = slot == 0 ? t.left + _ButtonEdge()
		: t.right - _ButtonEdge() - size + 1;
	return BRect(left, top, left + size - 1, top + size - 1);
}


bool
System7Decorator::_HasResizeCorner() const
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
System7Decorator::_LayoutTabs()
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

		// no minimize box: System 7.5 had none, and a double click on the bar
		// minimizes, as one rolled a window up then
		(void)minimizable;
		tab->closeRect = closable ? _ButtonSlot(tab, 0) : BRect(0, 0, -1, -1);
		tab->minimizeRect.Set(0, 0, -1, -1);
		tab->zoomRect = zoomable ? _ButtonSlot(tab, 1) : BRect(0, 0, -1, -1);

		// the title is kept clear of the boxes on both sides, so that it stays
		// in the middle, with stripes between
		float reserved = edge;
		if (closable || zoomable)
			reserved = edge + size + kTitleGap + 4;

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


/*!	On what would be the content's corner: a light square with a black line
	along its top and left, and in it two overlapping squares in the window
	colour, a small one in front of a larger one.
*/
void
System7Decorator::_DrawResizeCorner()
{
	const float length = roundf(kResizeCornerLength * _Scale());
	const BRect corner(fFrame.right - length + 1, fFrame.bottom - length + 1,
		fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kCornerFace);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(), kBlack);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(), kBlack);

	if (!IsFocus(fTopTab))
		return;

	_DrawSquare(BRect(corner.left + 4, corner.top + 4, corner.left + 12,
		corner.top + 12), kBoxFace);
	_DrawSquare(BRect(corner.left + 3, corner.top + 3, corner.left + 8,
		corner.top + 8), kSmallSquareFace);
}


//!	A square with a dark edge, lit inside along its top and left.
void
System7Decorator::_DrawSquare(BRect rect, rgb_color face)
{
	fDrawingEngine->StrokeRect(rect, kDark);
	fDrawingEngine->StrokeLine(BPoint(rect.left + 1, rect.top + 1),
		BPoint(rect.right - 1, rect.top + 1), kLight);
	fDrawingEngine->StrokeLine(BPoint(rect.left + 1, rect.top + 1),
		BPoint(rect.left + 1, rect.bottom - 1), kLight);
	fDrawingEngine->FillRect(BRect(rect.left + 2, rect.top + 2, rect.right - 1,
		rect.bottom - 1), face);
}


/*!	A box: sunk into the bar, its outer edge dark along its top and left and
	light along its bottom and right, and inside that a raised grey face, light
	along its top and left and dark along its bottom and right. The zoom box has
	a small square drawn in its face's top left corner. Pressed, the face is
	dark, with lines out from its middle like a burst.
*/
void
System7Decorator::_DrawBox(Decorator::Tab* tab, bool direct, BRect rect,
	bool pressed, glyph glyph, bool dim)
{
	if (!rect.IsValid() || rect.Width() < 3 || dim)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const float l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
	// the outer edge
	fDrawingEngine->StrokeLine(BPoint(l, t), BPoint(r, t), kDark);
	fDrawingEngine->StrokeLine(BPoint(l, t), BPoint(l, b), kDark);
	fDrawingEngine->StrokeLine(BPoint(r, t + 1), BPoint(r, b), kLight);
	fDrawingEngine->StrokeLine(BPoint(l + 1, b), BPoint(r, b), kLight);
	// the face's edges
	fDrawingEngine->StrokeLine(BPoint(l + 1, t + 1), BPoint(r - 1, t + 1),
		pressed ? kDark : kLight);
	fDrawingEngine->StrokeLine(BPoint(l + 1, t + 1), BPoint(l + 1, b - 1),
		pressed ? kDark : kLight);
	fDrawingEngine->StrokeLine(BPoint(r - 1, t + 2), BPoint(r - 1, b - 1),
		pressed ? kLight : kDark);
	fDrawingEngine->StrokeLine(BPoint(l + 2, b - 1), BPoint(r - 1, b - 1),
		pressed ? kLight : kDark);

	const BRect face(l + 2, t + 2, r - 2, b - 2);
	fDrawingEngine->FillRect(face, pressed ? kStripe : kBoxFace);

	if (pressed) {
		const BPoint middle(floorf((face.left + face.right) / 2),
			floorf((face.top + face.bottom) / 2));
		const float reach = floorf(face.Width() / 2);
		fDrawingEngine->StrokeLine(BPoint(middle.x - reach, middle.y),
			BPoint(middle.x + reach, middle.y), kBlack);
		fDrawingEngine->StrokeLine(BPoint(middle.x, middle.y - reach),
			BPoint(middle.x, middle.y + reach), kBlack);
		fDrawingEngine->StrokeLine(BPoint(middle.x - reach, middle.y - reach),
			BPoint(middle.x + reach, middle.y + reach), kBlack);
		fDrawingEngine->StrokeLine(BPoint(middle.x - reach, middle.y + reach),
			BPoint(middle.x + reach, middle.y - reach), kBlack);
	} else if (glyph == GLYPH_ZOOM) {
		// four sevenths of the way across and down
		const float corner = face.left + floorf((face.Width() + 1) * 4 / 7);
		fDrawingEngine->StrokeLine(BPoint(corner, face.top),
			BPoint(corner, corner - face.left + face.top), kDark);
		fDrawingEngine->StrokeLine(
			BPoint(face.left, corner - face.left + face.top),
			BPoint(corner, corner - face.left + face.top), kDark);
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) System7DecorAddOn(id, name);
}
