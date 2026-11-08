/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See AmigaOS3Decorator.h. The pictures are lordwolfchild's public domain
 * Amiga3.x theme for xfwm4 (AmigaOS3WindowArt.h, from ../art/xfwm4), laid out as
 * its themerc says: the title at the left, close at the left end, and zoom and
 * depth at the right.
 */


#include "AmigaOS3Decorator.h"

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

#include "AmigaOS3WindowArt.h"


// the bar's pieces, left to right: the frame's corner, the close gadget, the
// title's left edge (a groove after the close gadget), the title, a dark line
// before the zoom and depth gadgets, and the frame's other corner
static const float kTitleHeight = 16;
static const float kCloseLeft = 4;
static const float kTitleLeft = 30;
static const float kRightEnd = 6;
static const float kGadgetWidth = 23;
// the frame, and its bottom corners, which are the resize handles
static const float kBorder = 4;
static const float kCorner = 25;
static const float kBorderResizeLength = 25;
// the control look's scroll bars, whose corner the resize grip fills
static const float kScrollBarWidth = 16;
// the title's white text sits a row lower than the middle, as themerc has it
static const float kTitleOffset = 1;


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - AmigaOS3DecorAddOn


AmigaOS3DecorAddOn::AmigaOS3DecorAddOn(image_id id, const char* name)
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
AmigaOS3DecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) AmigaOS3Decorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - AmigaOS3Decorator


AmigaOS3Decorator::AmigaOS3Decorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


AmigaOS3Decorator::~AmigaOS3Decorator()
{
}


void
AmigaOS3Decorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
AmigaOS3Decorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
AmigaOS3Decorator::RegionAt(BPoint where, int32& tabIndex) const
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
AmigaOS3Decorator::UpdateColors(DesktopSettings& settings)
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
AmigaOS3Decorator::_DoLayout()
{
	fBorderWidth = 0;

	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = (int32)kBorder;
			break;
	}

	if (fBorderWidth > 0) {
		const float w = fBorderWidth;
		fLeftBorder.Set(fFrame.left - w, fFrame.top, fFrame.left - 1,
			fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top, fFrame.right + w,
			fFrame.bottom);
		fTopBorder.Set(fFrame.left - w, fFrame.top - w, fFrame.right + w,
			fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - w, fFrame.bottom + 1, fFrame.right + w,
			fFrame.bottom + w);
	} else {
		fLeftBorder.Set(0, 0, -1, -1);
		fRightBorder.Set(0, 0, -1, -1);
		fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(0, 0, -1, -1);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	if (_HasResizeCorner()) {
		// the frame's bottom right corner
		fResizeRect.Set(fBorderRect.right - kCorner + 1,
			fBorderRect.bottom - kCorner + 1, fBorderRect.right,
			fBorderRect.bottom);
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
AmigaOS3Decorator::_DoOutlineLayout()
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


/*!	The frame, four pixels wide: its sides and bottom from their pictures,
	repeated along them, and its bottom corners, the resize handles, over them.
	A full title bar draws its top.
*/
void
AmigaOS3Decorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const bool focus = IsFocus(fTopTab);
	if (!_HasFullTitleBar())
		_Repeat(focus ? kBottomActive : kBottomInactive, fTopBorder, true, rect);
	_Repeat(focus ? kLeftActive : kLeftInactive, fLeftBorder, false, rect);
	_Repeat(focus ? kRightActive : kRightInactive, fRightBorder, false, rect);
	_Repeat(focus ? kBottomActive : kBottomInactive, fBottomBorder, true, rect);

	if (fTopTab->look != B_BORDERED_WINDOW_LOOK) {
		const BRect& r = fBorderRect;
		_Draw(focus ? kBottomLeftActive : kBottomLeftInactive,
			BPoint(r.left, r.bottom - kCorner + 1), rect);
		_Draw(focus ? kBottomRightActive : kBottomRightInactive,
			BPoint(r.right - kCorner + 1, r.bottom - kCorner + 1), rect);
	}
}


void
AmigaOS3Decorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar from its pieces: the frame's corner and the close gadget at its left,
	the groove before the title, the title on the bar's colour, a dark line, the
	zoom and depth gadgets and the frame's other corner at its right.
*/
void
AmigaOS3Decorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const float line = t.right - kRightEnd - 2 * kGadgetWidth;

	_Draw(focus ? kTopLeftActive : kTopLeftInactive, t.LeftTop(), invalid);
	_Draw(focus ? kTitle2Active : kTitle2Inactive,
		BPoint(t.left + kTitleLeft - 12, t.top), invalid);
	_Repeat(focus ? kTitle3Active : kTitle3Inactive,
		BRect(t.left + kTitleLeft, t.top, line - 1, t.bottom), true, invalid);
	_Draw(focus ? kTitle4Active : kTitle4Inactive, BPoint(line, t.top),
		invalid);
	_Draw(focus ? kTopRightActive : kTopRightInactive,
		BPoint(t.right - kRightEnd + 1, t.top), invalid);
	// where a window lacks a gadget, the bar goes on
	if (!tab->closeRect.IsValid()) {
		_Repeat(focus ? kTitle3Active : kTitle3Inactive,
			BRect(t.left + kCloseLeft, t.top, t.left + kTitleLeft - 13,
				t.bottom), true, invalid);
	}
	if (!tab->zoomRect.IsValid() || !tab->minimizeRect.IsValid()) {
		const BRect gadgets(line + 1, t.top, t.right - kRightEnd, t.bottom);
		_Repeat(focus ? kTitle3Active : kTitle3Inactive, gadgets, true, invalid);
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


/*!	Where the title goes: at the left of the bar, after the groove, a row lower
	than the middle.
*/
BRect
AmigaOS3Decorator::_TitleRect(const Decorator::Tab* tab) const
{
	const BRect& t = tab->tabRect;
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	const float width = fDrawState.Font().StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength);

	const float x = t.left + kTitleLeft;
	const float baseline = floorf(t.top + (kTitleHeight + fontHeight.ascent
		- fontHeight.descent) / 2 + kTitleOffset);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


void
AmigaOS3Decorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
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
AmigaOS3Decorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
AmigaOS3Decorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, !IsFocus(tab) ? kCloseInactive
		: tab->closePressed ? kClosePressed : kCloseActive);
}


//!	The depth gadget, which sends a window back on the Amiga; here, it minimizes.
void
AmigaOS3Decorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, !IsFocus(tab) ? kHideInactive
		: tab->minimizePressed ? kHidePressed : kHideActive);
}


void
AmigaOS3Decorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, !IsFocus(tab) ? kMaximizeInactive
		: tab->zoomPressed ? kMaximizePressed : kMaximizeActive);
}


void
AmigaOS3Decorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
AmigaOS3Decorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
AmigaOS3Decorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
AmigaOS3Decorator::_SetSettings(const BMessage& settings,
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
AmigaOS3Decorator::_UpdateFont(DesktopSettings& settings)
{
	// the titles are plain, as Topaz was, and a utility window's smaller
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	if (_IsSmall())
		font.SetSize(std::max(9.0f, roundf(font.Size() * 11 / 12)));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
AmigaOS3Decorator::_MoveBy(BPoint offset)
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
AmigaOS3Decorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
AmigaOS3Decorator::_AddTab(DesktopSettings& settings, int32 index,
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
AmigaOS3Decorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
AmigaOS3Decorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
AmigaOS3Decorator::_GetFootprint(BRegion* region)
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
AmigaOS3Decorator::_HasTitle() const
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
AmigaOS3Decorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its pieces and its title.
float
AmigaOS3Decorator::_TabWidth(const Decorator::Tab* tab) const
{
	const char* title = tab->title.String();
	return kTitleLeft + 8 + 1 + 2 * kGadgetWidth + kRightEnd
		+ ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
AmigaOS3Decorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
AmigaOS3Decorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


//!	The gadgets are pixels, so the bar keeps their height whatever the font.
float
AmigaOS3Decorator::_TitleHeight() const
{
	return kTitleHeight;
}


bool
AmigaOS3Decorator::_HasResizeCorner() const
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
AmigaOS3Decorator::_LayoutTabs()
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
	// the bar's last row, black, is the frame's top
	const float bottom = fBorderRect.top + fBorderWidth - 1;
	const float top = bottom - height + 1;
	if (fBeOSTabs && count == 1) {
		Decorator::Tab* tab = fTabList.ItemAt(0);
		const float tabWidth = std::min(_TabWidth(tab), width);
		left += std::min(tab->tabLocation, width - tabWidth);
		width = tabWidth;
	}
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

		// close after the frame's corner, and zoom and depth before the other
		const float gadgetsLeft = tabRight - kRightEnd - 2 * kGadgetWidth + 1;
		tab->closeRect = closable ? BRect(tabLeft + kCloseLeft, top,
			tabLeft + kCloseLeft + 13, top + kTitleHeight - 1)
			: BRect(0, 0, -1, -1);
		tab->zoomRect = zoomable ? BRect(gadgetsLeft, top,
			gadgetsLeft + kGadgetWidth - 1, top + kTitleHeight - 1)
			: BRect(0, 0, -1, -1);
		tab->minimizeRect = minimizable ? BRect(gadgetsLeft + kGadgetWidth, top,
			gadgetsLeft + 2 * kGadgetWidth - 1, top + kTitleHeight - 1)
			: BRect(0, 0, -1, -1);

		// the title from the groove to the dark line before the gadgets
		const float reserved = kTitleLeft;
		const float taken = kTitleLeft + 8 + 1 + 2 * kGadgetWidth + kRightEnd;
		const float room = (tabRight - tabLeft + 1) - taken;

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


/*!	The content's corner where the scroll bars meet, as wide as one: a raised
	grey square with the resize grip in it.
*/
void
AmigaOS3Decorator::_DrawResizeCorner()
{
	const BRect corner(fFrame.right - kScrollBarWidth + 1,
		fFrame.bottom - kScrollBarWidth + 1, fFrame.right, fFrame.bottom);
	const rgb_color face = { 0xb5, 0xb5, 0xb5, 255 };
	const rgb_color white = { 0xff, 0xff, 0xff, 255 };
	const rgb_color black = { 0x00, 0x00, 0x00, 255 };
	fDrawingEngine->FillRect(corner, face);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(), white);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(), white);
	fDrawingEngine->StrokeLine(corner.LeftBottom(), corner.RightBottom(), black);
	fDrawingEngine->StrokeLine(corner.RightTop(), corner.RightBottom(), black);
	_Draw(kResizer, BPoint(corner.right - kResizer.width,
		corner.bottom - kResizer.height), corner);
}


void
AmigaOS3Decorator::_DrawGadget(bool direct, BRect rect, const Art& art)
{
	if (!rect.IsValid())
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);
	_Draw(art, rect.LeftTop(), rect);
	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


/*!	Sets a picture's pixels at where, those inside clip, each row in runs of
	one colour; transparent pixels are left alone.
*/
void
AmigaOS3Decorator::_Draw(const Art& art, BPoint where, const BRect& clip)
{
	const BRect area(where.x, where.y, where.x + art.width - 1,
		where.y + art.height - 1);
	if (!area.Intersects(clip))
		return;

	for (int32 y = 0; y < art.height; y++) {
		const float rowY = where.y + y;
		if (rowY < clip.top || rowY > clip.bottom)
			continue;
		const uint32* row = art.pixels + y * art.width;
		int32 start = 0;
		for (int32 x = 1; x <= art.width; x++) {
			if (x < art.width && row[x] == row[start])
				continue;
			const uint32 pixel = row[start];
			if ((pixel >> 24) >= 128) {
				const rgb_color color = { (uint8)(pixel >> 16),
					(uint8)(pixel >> 8), (uint8)pixel, 255 };
				const BRect run = BRect(where.x + start, rowY, where.x + x - 1,
					rowY) & clip;
				if (run.IsValid())
					fDrawingEngine->FillRect(run, color);
			}
			start = x;
		}
	}
}


/*!	Fills area, as much of it as is inside clip, with a picture a pixel tall or
	wide repeated along it: across, its rows run along the area from its top,
	as along a bar; otherwise its columns run down it from its left.
*/
void
AmigaOS3Decorator::_Repeat(const Art& art, const BRect& area, bool across,
	const BRect& clip)
{
	if (!area.IsValid() || !clip.IsValid())
		return;

	const int32 count = across ? art.height : art.width;
	for (int32 i = 0; i < count; i++) {
		const uint32 pixel = art.pixels[i];
		const rgb_color color = { (uint8)(pixel >> 16), (uint8)(pixel >> 8),
			(uint8)pixel, 255 };
		const BRect line = (across
			? BRect(area.left, area.top + i, area.right, area.top + i)
			: BRect(area.left + i, area.top, area.left + i, area.bottom))
			& area & clip;
		if (line.IsValid())
			fDrawingEngine->FillRect(line, color);
	}
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) AmigaOS3DecorAddOn(id, name);
}
