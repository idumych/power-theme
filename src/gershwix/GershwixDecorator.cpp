/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See GershwixDecorator.h. Colours and sizes were measured from Gershwix 2.5's
 * pictures of its windows.
 */


#include "GershwixDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
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


// The title bar, top to bottom: a white row, a light one, then stripes of two
// greys, a lighter row and a dark line over the content. Its left end has a
// white edge and its right end a grey one.
static const uint8 kBarTop = 0xff;
static const uint8 kBarSecond = 0xe7;
static const uint8 kStripeLight = 0xe6;
static const uint8 kStripeDark = 0xd2;
static const uint8 kBarBottom = 0xcc;
static const uint8 kBarLine = 0xa6;
static const uint8 kBarCorner = 0xdd;
static const uint8 kBarRightShade = 0xd4;

// a box: a groove etched round it, a dark outline, and a face lit from its
// bottom right, a white corner at its top left and a dark edge along its bottom
// and right
static const uint8 kEtchDark = 0xa6;
static const uint8 kEtchLight = 0xff;
static const uint8 kEtchCorner = 0xe5;
static const uint8 kBoxLine = 0x66;
static const uint8 kFaceLight = 0xe7;
static const uint8 kFaceShadow = 0x9a;
static const uint8 kFaceSteps[] = { 0xad, 0xc1, 0xd4, 0xe7, 0xfa, 0xff };
static const int32 kFaceStepCount = sizeof(kFaceSteps) / sizeof(kFaceSteps[0]);
// how much darker a pressed box's face is
static const uint8 kPressed = 0x3c;
// an inactive box is a flat grey square, laid over the bar three quarters solid
static const uint8 kFlatLine = 0x48;
static const uint8 kFlatFace = 0x9b;
static const uint8 kFlatAlpha = 191;

static const rgb_color kCornerFace = { 0xeb, 0xeb, 0xeb, 255 };
static const rgb_color kCornerLine = { 0xa6, 0xa6, 0xa6, 255 };
static const rgb_color kGripDark = { 0x66, 0x66, 0x66, 255 };
static const rgb_color kGripLight = { 0xff, 0xff, 0xff, 255 };

// the outline's row and the bar's 22
static const float kTitleHeight = 23;
static const float kSmallTitleHeight = 17;
static const float kLargeBox = 13;
static const float kSmallBox = 10;
static const float kTitleGap = 8;
static const float kResizeCornerLength = 15;
static const float kBorderResizeLength = 22;


static rgb_color
grey(uint8 level)
{
	rgb_color color = { level, level, level, 255 };
	return color;
}


static uint8
blend(uint8 over, uint8 under, uint8 alpha)
{
	return (uint8)((over * alpha + under * (255 - alpha)) / 255);
}


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - GershwixDecorAddOn


GershwixDecorAddOn::GershwixDecorAddOn(image_id id, const char* name)
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
GershwixDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) GershwixDecorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - GershwixDecorator


GershwixDecorator::GershwixDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


GershwixDecorator::~GershwixDecorator()
{
}


void
GershwixDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
GershwixDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
GershwixDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
GershwixDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
	fFocusFrameColor = settings.UIColor(B_WINDOW_BORDER_COLOR);
	fNonFocusFrameColor = settings.UIColor(B_WINDOW_INACTIVE_BORDER_COLOR);
}


//	#pragma mark - Protected methods


void
GershwixDecorator::_DoLayout()
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
GershwixDecorator::_DoOutlineLayout()
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
GershwixDecorator::_DrawFrame(BRect rect)
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
GershwixDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar row by row: each row's grey across, then its ends, and the frame's
	line round its top and sides.
*/
void
GershwixDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const rgb_color edge = IsFocus(tab) ? fFocusFrameColor : fNonFocusFrameColor;
	const BRect rows = t & invalid;
	const float middle = floorf((t.left + t.right) / 2);
	for (float y = rows.top; y <= rows.bottom; y++) {
		if (y == t.top) {
			fDrawingEngine->StrokeLine(BPoint(rows.left, y),
				BPoint(rows.right, y), edge);
			continue;
		}
		fDrawingEngine->StrokeLine(BPoint(rows.left, y), BPoint(rows.right, y),
			grey(_BarLevel(tab, middle, y)));
		// the ends' first three columns, which differ from the row's middle
		for (int32 i = 1; i <= 2; i++) {
			const float left = t.left + i;
			const float right = t.right - i;
			if (left >= rows.left && left <= rows.right) {
				fDrawingEngine->StrokeLine(BPoint(left, y), BPoint(left, y),
					grey(_BarLevel(tab, left, y)));
			}
			if (right >= rows.left && right <= rows.right) {
				fDrawingEngine->StrokeLine(BPoint(right, y), BPoint(right, y),
					grey(_BarLevel(tab, right, y)));
			}
		}
		if (y == t.bottom)
			continue;
		if (rows.left == t.left)
			fDrawingEngine->StrokeLine(BPoint(t.left, y), BPoint(t.left, y), edge);
		if (rows.right == t.right) {
			fDrawingEngine->StrokeLine(BPoint(t.right, y), BPoint(t.right, y),
				edge);
		}
	}

	// between two tabs of a stack
	if (t.left > fTitleBarRect.left && invalid.left <= t.left) {
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
			BPoint(t.left, t.bottom - 1), grey(kBarLine));
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


void
GershwixDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
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
	// in the middle of the stripes, below the outline and above the bottom rows
	const float top = t.top + 1;
	const float space = t.bottom - 2 - top;
	position.y = floorf(top + (space + fontHeight.ascent - fontHeight.descent)
		/ 2 + 1);

	fDrawingEngine->SetHighColor(focus ? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
GershwixDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
	_DrawDisabledSlots(tab, invalid);
}


void
GershwixDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->closePressed, GLYPH_NONE, !IsFocus(tab));
}


void
GershwixDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->minimizePressed, GLYPH_MINIMIZE,
		!IsFocus(tab));
}


void
GershwixDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->zoomPressed, GLYPH_ZOOM, !IsFocus(tab));
}


void
GershwixDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
GershwixDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
GershwixDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
GershwixDecorator::_SetSettings(const BMessage& settings,
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
GershwixDecorator::_UpdateFont(DesktopSettings& settings)
{
	// the titles aren't bold, and a utility window's are smaller
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	if (_IsSmall())
		font.SetSize(std::max(9.0f, roundf(font.Size() * 11 / 12)));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
GershwixDecorator::_MoveBy(BPoint offset)
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
GershwixDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
GershwixDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
GershwixDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
GershwixDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
GershwixDecorator::_GetFootprint(BRegion* region)
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
GershwixDecorator::_HasTitle() const
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
GershwixDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its title and its two boxes.
float
GershwixDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float size = _ButtonSize();
	const float edge = _ButtonEdge();

	float width = 2 * edge + 2 * size + 4 * kTitleGap;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
GershwixDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
GershwixDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


float
GershwixDecorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


float
GershwixDecorator::_ButtonSize() const
{
	return _IsSmall() ? kSmallBox : kLargeBox;
}


//!	How far the first box is from the bar's left end, the frame's line included.
float
GershwixDecorator::_ButtonEdge() const
{
	return _IsSmall() ? 6 : 9;
}


float
GershwixDecorator::_ButtonGap() const
{
	return _IsSmall() ? 5 : 8;
}


/*!	Where a full title bar's boxes go: close, minimize and zoom, from the left.
	A window that lacks some of them still shows them there, flat, as long as it
	has a close box.
*/
BRect
GershwixDecorator::_ButtonSlot(const Decorator::Tab* tab, int32 slot) const
{
	const BRect& t = tab->tabRect;
	const float size = _ButtonSize();
	const float top = t.top + 1 + ceilf((t.Height() - size) / 2);
	const float left = t.left + _ButtonEdge() + slot * (size + _ButtonGap());
	return BRect(left, top, left + size - 1, top + size - 1);
}


bool
GershwixDecorator::_HasResizeCorner() const
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
GershwixDecorator::_LayoutTabs()
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


/*!	The grey of the bar at a point inside the frame's line: a white top row and
	left edge, a grey right edge, stripes between, and two rows at the bottom,
	the last of them the line over the content. The stripes are counted from the
	bottom, so that a taller bar still ends the same way.
*/
uint8
GershwixDecorator::_BarLevel(const Decorator::Tab* tab, float x, float y) const
{
	const BRect& t = tab->tabRect;
	const int32 row = (int32)(y - t.top - 1);
	const int32 fromBottom = (int32)(t.bottom - y);
	const int32 column = (int32)(x - t.left);
	const int32 fromRight = (int32)(t.right - x);

	if (fromBottom == 0)
		return kBarLine;
	if (fromBottom == 1)
		return column == 1 ? kBarCorner : kBarBottom;
	if (fromRight == 1)
		return row == 0 ? kBarCorner : kBarBottom;
	if (row == 0 || column == 1)
		return kBarTop;
	if (fromRight == 2)
		return row == 1 ? kStripeDark : kBarRightShade;
	if (column == 2)
		return fromBottom == 2 ? kStripeDark : kBarSecond;
	if (row == 1)
		return kBarSecond;
	return fromBottom % 2 == 0 ? kStripeDark : kStripeLight;
}


/*!	On what would be the content's corner: a light face with a grey edge and
	three diagonal ridges.
*/
void
GershwixDecorator::_DrawResizeCorner()
{
	const float length = roundf(kResizeCornerLength * _Scale());
	const BRect corner(fFrame.right - length + 1, fFrame.bottom - length + 1,
		fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kCornerFace);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(), kCornerLine);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(),
		kCornerLine);

	if (!IsFocus(fTopTab))
		return;

	for (int32 i = 0; i < 3; i++) {
		const float offset = 4 + i * 3;
		fDrawingEngine->StrokeLine(BPoint(corner.right - offset, corner.bottom - 2),
			BPoint(corner.right - 2, corner.bottom - offset), kGripLight);
		fDrawingEngine->StrokeLine(
			BPoint(corner.right - offset + 1, corner.bottom - 2),
			BPoint(corner.right - 2, corner.bottom - offset + 1), kGripDark);
	}
}


/*!	Sets a box's pixels by hand, each row in runs of one grey: the groove round
	it, its outline and its face, with the minimize or zoom mark drawn in the
	outline's grey. An inactive window's box is a flat grey square over the bar.
*/
void
GershwixDecorator::_DrawBox(Decorator::Tab* tab, bool direct, BRect rect,
	bool pressed, glyph glyph, bool flat)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const int32 size = (int32)rect.Width() + 1;
	const int32 face = size - 4;
	// the marks: zoom's corner and the top of minimize's window
	const int32 mark = 2 + (face + 1) / 2;
	const int32 inset = face / 4;

	for (int32 y = 0; y < size; y++) {
		const float rowY = rect.top + y;
		int32 start = 0;
		uint8 runLevel = 0;
		for (int32 x = 0; x <= size; x++) {
			uint8 level = 0;
			if (x < size) {
				const uint8 bar = _BarLevel(tab, rect.left + x, rowY);
				const bool outline = x == 1 || y == 1 || x == size - 2
					|| y == size - 2;
				const bool inside = x > 1 && y > 1 && x < size - 2
					&& y < size - 2;
				if (flat) {
					if (inside)
						level = blend(kFlatFace, bar, kFlatAlpha);
					else if (outline && x > 0 && y > 0 && x < size - 1
						&& y < size - 1)
						level = blend(kFlatLine, bar, kFlatAlpha);
					else
						level = bar;
				} else if ((y == 0 && x == size - 1) || (x == 0 && y == size - 1))
					level = kEtchCorner;
				else if (y == 0 || x == 0)
					level = kEtchDark;
				else if (y == size - 1 || x == size - 1)
					level = kEtchLight;
				else if (outline)
					level = kBoxLine;
				else {
					const int32 fx = x - 2;
					const int32 fy = y - 2;
					if (fy == 0 && fx == 0)
						level = kBarTop;
					else if (fy == 0 || fx == 0)
						level = kFaceLight;
					else if (fx == face - 1 || fy == face - 1)
						level = kFaceShadow;
					else {
						// lit from the bottom right, in steps along the diagonal
						const int32 span = std::max(1, 2 * face - 8);
						const int32 step = std::min(kFaceStepCount - 1,
							(fx - 1 + fy - 1) * (kFaceStepCount - 1) / span);
						level = kFaceSteps[step];
					}
					if (pressed)
						level -= kPressed;

					if (glyph == GLYPH_ZOOM && ((x == mark && y <= mark)
							|| (y == mark && x <= mark)))
						level = kBoxLine;
					else if (glyph == GLYPH_MINIMIZE && y >= mark
						&& x >= 2 + inset && x <= size - 3 - inset
						&& (y == mark || x == 2 + inset
							|| x == size - 3 - inset))
						level = kBoxLine;
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


//!	A window with a close box shows the others it lacks, flat.
void
GershwixDecorator::_DrawDisabledSlots(Decorator::Tab* tab, const BRect& invalid)
{
	if (fBeOSTabs || !tab->closeRect.IsValid())
		return;

	const BRect minimize = _ButtonSlot(tab, 1);
	const BRect zoom = _ButtonSlot(tab, 2);
	if (!tab->minimizeRect.IsValid() && invalid.Intersects(minimize))
		_DrawBox(tab, false, minimize, false, GLYPH_NONE, true);
	if (!tab->zoomRect.IsValid() && invalid.Intersects(zoom))
		_DrawBox(tab, false, zoom, false, GLYPH_NONE, true);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) GershwixDecorAddOn(id, name);
}
