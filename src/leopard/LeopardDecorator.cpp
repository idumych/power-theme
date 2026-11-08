/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See LeopardDecorator.h. Colours and sizes were measured from screenshots of
 * Mac OS X 10.5's windows.
 */


#include "LeopardDecorator.h"

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

#include "LeopardButtons.h"


#define GREY(level) { level, level, level, 255 }

// The title bar, top to bottom: a light top row, a shading and a dark line over
// the content. An inactive window's is lighter all through.
static const uint8 kBarTop = 0xdc;
static const uint8 kBarShadeTop = 0xc4;
static const uint8 kBarShadeBottom = 0x97;
static const uint8 kBarLine = 0x40;
static const uint8 kInactiveBarTop = 0xf6;
static const uint8 kInactiveBarShadeTop = 0xea;
static const uint8 kInactiveBarShadeBottom = 0xd1;
static const uint8 kInactiveBarLine = 0x8c;

// the line round the content, where Leopard has its shadow's darkest edge
static const rgb_color kEdge = GREY(0x76);
static const rgb_color kInactiveEdge = GREY(0xa4);
// the title's light edge below it
static const rgb_color kEmboss = GREY(0xdc);
static const rgb_color kInactiveEmboss = GREY(0xf8);

static const rgb_color kCornerFace = GREY(0xe8);
static const rgb_color kGripDark = GREY(0x8e);
static const rgb_color kGripLight = GREY(0xff);
static const rgb_color kCornerLine = GREY(0xa5);

static const float kTitleHeight = 22;
static const float kSmallTitleHeight = 16;
static const float kTitleGap = 8;
static const float kResizeCornerLength = 14;
static const float kBorderResizeLength = 22;

// how far each of a top corner's first rows is cut in, without a compositor
static const int32 kCorner[] = { 3, 1, 1 };
static const int32 kCornerRows = sizeof(kCorner) / sizeof(kCorner[0]);


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


//	#pragma mark - LeopardDecorAddOn


LeopardDecorAddOn::LeopardDecorAddOn(image_id id, const char* name)
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
LeopardDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) LeopardDecorator(settings, rect, desktop, fBeOSTabs);
}


//	#pragma mark - LeopardDecorator


LeopardDecorator::LeopardDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs),
	fGraphite(false)
{
	UpdateColors(settings);
}


LeopardDecorator::~LeopardDecorator()
{
}


void
LeopardDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
LeopardDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
LeopardDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
LeopardDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

	// Graphite's highlight is a greyish blue; Blue's is a strong one
	const rgb_color highlight = settings.UIColor(B_CONTROL_HIGHLIGHT_COLOR);
	const int32 high = std::max(highlight.red,
		std::max(highlight.green, highlight.blue));
	const int32 low = std::min(highlight.red,
		std::min(highlight.green, highlight.blue));
	fGraphite = high == 0 || (high - low) * 100 / high < 40;
}


//	#pragma mark - Protected methods


void
LeopardDecorator::_DoLayout()
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
LeopardDecorator::_DoOutlineLayout()
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


/*!	A line round the content. A full title bar draws the top one, as its dark
	bottom row.
*/
void
LeopardDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const rgb_color edge = IsFocus(fTopTab) ? kEdge : kInactiveEdge;
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
LeopardDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


void
LeopardDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	// the shading, row by row, a little darker at its two ends so that a bar
	// over a light window still has an edge
	const BRect rows = t & invalid;
	for (float y = rows.top; y <= rows.bottom; y++) {
		const rgb_color color = _BarColor(tab, y);
		fDrawingEngine->StrokeLine(BPoint(rows.left, y), BPoint(rows.right, y),
			color);
		if (y == t.top || y == t.bottom)
			continue;
		rgb_color end = color;
		end.red = end.green = end.blue = (uint8)(color.red * 0.86f);
		if (rows.left == t.left)
			fDrawingEngine->StrokeLine(BPoint(t.left, y), BPoint(t.left, y), end);
		if (rows.right == t.right) {
			fDrawingEngine->StrokeLine(BPoint(t.right, y), BPoint(t.right, y),
				end);
		}
	}

	// between two tabs of a stack
	if (t.left > fTitleBarRect.left) {
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
			BPoint(t.left, t.bottom - 1), grey(IsFocus(tab) ? 0x80 : 0xb0));
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);

#ifdef LEOPARD_ROUNDED_CORNERS
	_StrokeRoundedFrame();
#endif
}


void
LeopardDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& t = tab->tabRect;
	const bool focus = IsFocus(tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	const float width = fDrawingEngine->StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength, fDrawState.Font());

	// in the middle of the bar, but clear of the buttons
	float left = t.left + tab->textOffset;
	float right = t.right - tab->textOffset;
	if (fBeOSTabs && tab->zoomRect.IsValid())
		right = tab->zoomRect.left - kTitleGap;

	BPoint position;
	position.x = floorf((t.left + t.right + 1 - width) / 2);
	position.x = std::max(left, std::min(position.x, right - width));
	// above the dark bottom row
	const float space = t.Height();
	position.y = floorf(t.top + (space + fontHeight.ascent - fontHeight.descent)
		/ 2);

	fDrawingEngine->SetHighColor(focus ? kEmboss : kInactiveEmboss);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position + BPoint(0, 1));
	fDrawingEngine->SetHighColor(focus ? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
LeopardDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
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
LeopardDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!IsFocus(tab))
		_DrawButton(tab, direct, rect, kInactiveLarge, kInactiveSmall);
	else if (tab->closePressed && fGraphite) {
		_DrawButton(tab, direct, rect, kGraphiteClosePressedLarge,
			kGraphiteClosePressedSmall);
	} else if (tab->closePressed)
		_DrawButton(tab, direct, rect, kClosePressedLarge, kClosePressedSmall);
	else if (fGraphite)
		_DrawButton(tab, direct, rect, kGraphiteLarge, kGraphiteSmall);
	else
		_DrawButton(tab, direct, rect, kCloseLarge, kCloseSmall);
}


void
LeopardDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!IsFocus(tab))
		_DrawButton(tab, direct, rect, kInactiveLarge, kInactiveSmall);
	else if (tab->minimizePressed && fGraphite) {
		_DrawButton(tab, direct, rect, kGraphiteMinimizePressedLarge,
			kGraphiteMinimizePressedSmall);
	} else if (tab->minimizePressed) {
		_DrawButton(tab, direct, rect, kMinimizePressedLarge,
			kMinimizePressedSmall);
	} else if (fGraphite)
		_DrawButton(tab, direct, rect, kGraphiteLarge, kGraphiteSmall);
	else
		_DrawButton(tab, direct, rect, kMinimizeLarge, kMinimizeSmall);
}


void
LeopardDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!IsFocus(tab))
		_DrawButton(tab, direct, rect, kInactiveLarge, kInactiveSmall);
	else if (tab->zoomPressed && fGraphite) {
		_DrawButton(tab, direct, rect, kGraphiteZoomPressedLarge,
			kGraphiteZoomPressedSmall);
	} else if (tab->zoomPressed)
		_DrawButton(tab, direct, rect, kZoomPressedLarge, kZoomPressedSmall);
	else if (fGraphite)
		_DrawButton(tab, direct, rect, kGraphiteLarge, kGraphiteSmall);
	else
		_DrawButton(tab, direct, rect, kZoomLarge, kZoomSmall);
}


void
LeopardDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
LeopardDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
LeopardDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
LeopardDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
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
LeopardDecorator::_UpdateFont(DesktopSettings& settings)
{
	// Leopard's titles aren't bold, and a utility window's are smaller
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	if (_IsSmall())
		font.SetSize(std::max(9.0f, roundf(font.Size() * 11 / 12)));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
LeopardDecorator::_MoveBy(BPoint offset)
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
LeopardDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
LeopardDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
LeopardDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
LeopardDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
LeopardDecorator::_GetFootprint(BRegion* region)
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

	if (_RoundedByCompositor())
		return;

	// the bar's top corners, or a BeOS-style tab's
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		const BRect& t = fTabList.ItemAt(i)->tabRect;
		if (!t.IsValid())
			continue;
		if (t.left == fTitleBarRect.left)
			_ExcludeCorner(region, t.left, t.top, 1);
		if (t.right == fTitleBarRect.right)
			_ExcludeCorner(region, t.right, t.top, -1);
	}
}


//	#pragma mark - Private methods


bool
LeopardDecorator::_HasTitle() const
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
LeopardDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


bool
LeopardDecorator::_RoundedByCompositor() const
{
#ifdef LEOPARD_ROUNDED_CORNERS
	return RoundedDecorator::IsActive(
		const_cast<LeopardDecorator*>(this)->GetDesktop());
#else
	return false;
#endif
}


//!	How wide a BeOS-style tab has to be: its title and its two buttons.
float
LeopardDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float size = _ButtonSize();
	const float edge = _ButtonEdge();

	float width = 2 * edge + 2 * size + 4 * kTitleGap;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
LeopardDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
LeopardDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


float
LeopardDecorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


float
LeopardDecorator::_ButtonSize() const
{
	return _IsSmall() ? kSmallButton : kLargeButton;
}


//!	How far the first button is from the bar's left end.
float
LeopardDecorator::_ButtonEdge() const
{
	return _IsSmall() ? 5 : 7;
}


float
LeopardDecorator::_ButtonGap() const
{
	return _IsSmall() ? 5 : 7;
}


/*!	Where a full title bar's buttons go: close, minimize and zoom, from the left.
	A window that lacks some of them still shows them there, disabled, as long as
	it has a close button.
*/
BRect
LeopardDecorator::_ButtonSlot(const Decorator::Tab* tab, int32 slot) const
{
	const BRect& t = tab->tabRect;
	const float size = _ButtonSize();
	const float top = t.top + floorf((t.Height() - size) / 2);
	const float left = t.left + _ButtonEdge() + slot * (size + _ButtonGap());
	return BRect(left, top, left + size - 1, top + size - 1);
}


bool
LeopardDecorator::_HasResizeCorner() const
{
	return fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0;
}


/*!	Lays the tabs out side by side across the whole window, each with its buttons
	at its left end and its title in its middle.

	A BeOS-style tab sits on the frame, over its top edge so that they join up,
	with close on the left, zoom on the right and no minimize button. A lone one is
	as wide as its title and can slide; a stack's share the window's width.
*/
void
LeopardDecorator::_LayoutTabs()
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

		// the title is kept clear of the buttons on both sides, so that it stays
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


//!	The colour of a bar's row: a light top, a shading, and a dark bottom row.
rgb_color
LeopardDecorator::_BarColor(const Decorator::Tab* tab, float y) const
{
	const BRect& t = tab->tabRect;
	const bool focus = IsFocus(const_cast<Decorator::Tab*>(tab));
	if (y <= t.top)
		return grey(focus ? kBarTop : kInactiveBarTop);
	if (y >= t.bottom)
		return grey(focus ? kBarLine : kInactiveBarLine);

	const float rows = std::max(1.0f, t.Height() - 2);
	const float f = std::min(1.0f, (y - t.top - 1) / rows);
	const float from = focus ? kBarShadeTop : kInactiveBarShadeTop;
	const float to = focus ? kBarShadeBottom : kInactiveBarShadeBottom;
	return grey((uint8)roundf(from + (to - from) * f));
}


void
LeopardDecorator::_ExcludeCorner(BRegion* region, float x, float y,
	int32 xDirection) const
{
	for (int32 row = 0; row < kCornerRows; row++) {
		const float end = x + (kCorner[row] - 1) * xDirection;
		region->Exclude(BRect(std::min(x, end), y + row, std::max(x, end),
			y + row));
	}
}


/*!	On what would be the content's corner: a light face and Leopard's grip of
	three diagonal ridges.
*/
void
LeopardDecorator::_DrawResizeCorner()
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
		const float offset = 3 + i * 4;
		fDrawingEngine->StrokeLine(BPoint(corner.right - offset, corner.bottom),
			BPoint(corner.right, corner.bottom - offset), kGripDark);
		fDrawingEngine->StrokeLine(
			BPoint(corner.right - offset + 1, corner.bottom),
			BPoint(corner.right, corner.bottom - offset + 1), kGripLight);
	}
}


/*!	Sets a button's pixels by hand, each row in runs of one colour. A decorator
	can't blend, so its soft edges are mixed with the bar's row behind them here.
	A button's cell is a row taller than the button, for its light edge.
*/
void
LeopardDecorator::_DrawButton(Decorator::Tab* tab, bool direct, BRect rect,
	const uint32* large, const uint32* small)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const int32 size = _IsSmall() ? kSmallButton : kLargeButton;
	const uint32* pixels = _IsSmall() ? small : large;

	for (int32 y = 0; y <= size; y++) {
		const float rowY = rect.top + y;
		if (rowY >= tab->tabRect.bottom)
			break;
		const rgb_color under = _BarColor(tab, rowY);

		int32 start = 0;
		rgb_color runColor = under;
		for (int32 x = 0; x <= size; x++) {
			rgb_color color = under;
			if (x < size) {
				const uint32 pixel = pixels[y * size + x];
				const uint32 alpha = pixel >> 24;
				color.red = (uint8)((((pixel >> 16) & 0xff) * alpha
					+ under.red * (255 - alpha)) / 255);
				color.green = (uint8)((((pixel >> 8) & 0xff) * alpha
					+ under.green * (255 - alpha)) / 255);
				color.blue = (uint8)(((pixel & 0xff) * alpha
					+ under.blue * (255 - alpha)) / 255);
			}
			if (x == 0)
				runColor = color;
			if (x == size || color != runColor) {
				fDrawingEngine->FillRect(BRect(rect.left + start, rowY,
					rect.left + x - 1, rowY), runColor);
				start = x;
				runColor = color;
			}
		}
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


//!	A window with a close button shows the others it lacks, greyed out.
void
LeopardDecorator::_DrawDisabledSlots(Decorator::Tab* tab, const BRect& invalid)
{
	if (fBeOSTabs || !tab->closeRect.IsValid())
		return;

	const BRect minimize = _ButtonSlot(tab, 1);
	const BRect zoom = _ButtonSlot(tab, 2);
	if (!tab->minimizeRect.IsValid() && invalid.Intersects(minimize))
		_DrawButton(tab, false, minimize, kInactiveLarge, kInactiveSmall);
	if (!tab->zoomRect.IsValid() && invalid.Intersects(zoom))
		_DrawButton(tab, false, zoom, kInactiveLarge, kInactiveSmall);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) LeopardDecorAddOn(id, name);
}


#ifdef LEOPARD_ROUNDED_CORNERS
static const int32 kFrameCornerRadius = 5;
static const int32 kTabCornerRadius = 5;


void
LeopardDecorator::GetRoundedShape(std::vector<ShapeRect>& shape) const
{
	GetShape(shape);
	RoundedDecorator::RoundShape(shape, fTopTab->look, kFrameCornerRadius,
		kTabCornerRadius);
	// Leopard's windows are square at the bottom
	if (!shape.empty() && _HasTitle()) {
		shape[0].leftBottom = 0;
		shape[0].rightBottom = 0;
	}
}


//! The compositor cuts off the top corners, and the bar's shading follows them.
void
LeopardDecorator::_StrokeRoundedFrame()
{
	if (!RoundedDecorator::IsActive(GetDesktop()))
		return;

	std::vector<ShapeRect> shape;
	GetRoundedShape(shape);
	const bool focus = IsFocus(fTopTab);
	const rgb_color edge = grey(focus ? 0xa8 : 0xd8);
	for (size_t i = 1; i < shape.size(); i++) {
		RoundedDecorator::StrokeCorners(fDrawingEngine, shape[i], edge, edge,
			edge, edge);
	}
	if (!_HasFullTitleBar()) {
		const rgb_color frame = focus ? kEdge : kInactiveEdge;
		RoundedDecorator::StrokeCorners(fDrawingEngine, shape[0], frame, frame,
			frame, frame);
	}
}
#endif	// LEOPARD_ROUNDED_CORNERS
