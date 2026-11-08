/*
 * Copyright 2026, idumych.
 * Copyright (c) 2020 Arthur Choung (the gadgets' pixels and the palette, from
 * HOTDOG's objects/object-AmigaWindow.m).
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * See AmigaDecorator.h.
 */


#include "AmigaDecorator.h"

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


// Workbench 1.3's colours, as HOTDOG gives them: its blue, white, black and
// orange
static const rgb_color kBlue = { 0x00, 0x55, 0xaa, 255 };
static const rgb_color kWhite = { 0xff, 0xff, 0xff, 255 };
static const rgb_color kBlack = { 0x00, 0x00, 0x22, 255 };
static const rgb_color kOrange = { 0xff, 0x88, 0x00, 255 };

// The gadgets, a pixel per character: X blue, o white, . black, and a space
// for nothing. HOTDOG draws Workbench's at twice their size across and down,
// as a low resolution Amiga screen showed them. Pressed, white turns black,
// blue orange and black white.
static const char* const kCloseGadget[] = {
	"XXooooooooooooooooooooXX",
	"XXooooooooooooooooooooXX",
	"XXooXXXXXXXXXXXXXXXXooXX",
	"XXooXXXXXXXXXXXXXXXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXoooo....ooooXXooXX",
	"XXooXXoooo....ooooXXooXX",
	"XXooXXoooo....ooooXXooXX",
	"XXooXXoooo....ooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXooooooooooooXXooXX",
	"XXooXXXXXXXXXXXXXXXXooXX",
	"XXooXXXXXXXXXXXXXXXXooXX",
	"XXooooooooooooooooooooXX",
	"XXooooooooooooooooooooXX"
};
// sends the window back: a white window in front of a black one
static const char* const kLowerGadget[] = {
	"XXooooooooooooooooooooooXX",
	"XXooooooooooooooooooooooXX",
	"XXooXXXXXXXXXXXXXXooooooXX",
	"XXooXXXXXXXXXXXXXXooooooXX",
	"XXooXXooooooooooXXooooooXX",
	"XXooXXooooooooooXXooooooXX",
	"XXooXXoo..............ooXX",
	"XXooXXoo..............ooXX",
	"XXooXXoo..............ooXX",
	"XXooXXoo..............ooXX",
	"XXooXXoo..............ooXX",
	"XXooXXoo..............ooXX",
	"XXooXXXX..............ooXX",
	"XXooXXXX..............ooXX",
	"XXoooooo..............ooXX",
	"XXoooooo..............ooXX",
	"XXoooooo..............ooXX",
	"XXoooooo..............ooXX",
	"XXooooooooooooooooooooooXX",
	"XXooooooooooooooooooooooXX"
};
// brings the window to the front: a black window behind a white one
static const char* const kRaiseGadget[] = {
	"XXooooooooooooooooooooooXX",
	"XXooooooooooooooooooooooXX",
	"XXoo..............ooooooXX",
	"XXoo..............ooooooXX",
	"XXoo....XXXXXXXXXXXXXXooXX",
	"XXoo....XXXXXXXXXXXXXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXoo....XXooooooooooXXooXX",
	"XXooooooXXooooooooooXXooXX",
	"XXooooooXXooooooooooXXooXX",
	"XXooooooXXXXXXXXXXXXXXooXX",
	"XXooooooXXXXXXXXXXXXXXooXX",
	"XXooooooooooooooooooooooXX",
	"XXooooooooooooooooooooooXX"
};
static const char* const kResizeGadget[] = {
	"oooooooooooooooo",
	"oooooooooooooooo",
	"ooXXXXXXoooooooo",
	"ooXXXXXXoooooooo",
	"ooXXooXXoooooooo",
	"ooXXooXXoooooooo",
	"ooXXXXXXXXXXXXoo",
	"ooXXXXXXXXXXXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXooooXXoo",
	"ooooooXXXXXXXXoo",
	"ooooooXXXXXXXXoo",
	"oooooooooooooooo",
	"oooooooooooooooo"
};
static const int32 kGadgetRows = 20;
static const int32 kCloseWidth = 24;
static const int32 kDepthWidth = 26;
static const int32 kResizeWidth = 16;
static const int32 kResizeHeight = 18;

// the bar: white, with two blue stripes four rows deep across its middle, and
// plain ends where the gadgets sit
static const int32 kStripes[] = { 4, 12 };
static const int32 kStripeRows = 4;
static const float kBarLeft = 30;
static const float kBarRight = 56;
// an inactive window's stripes are dotted with white, every fourth pixel,
// staggered on alternate pairs of rows
static const int32 kDotSpacing = 4;

static const float kTitleHeight = 20;
static const float kBorder = 2;
static const float kBorderResizeLength = 22;


static const rgb_color&
gadget_color(char c, bool pressed, bool& draw)
{
	draw = true;
	switch (c) {
		case 'X':
			return pressed ? kOrange : kBlue;
		case 'o':
			return pressed ? kBlack : kWhite;
		case '.':
			return pressed ? kWhite : kBlack;
		default:
			draw = false;
			return kWhite;
	}
}


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - AmigaDecorAddOn


AmigaDecorAddOn::AmigaDecorAddOn(image_id id, const char* name)
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
AmigaDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) AmigaDecorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - AmigaDecorator


AmigaDecorator::AmigaDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


AmigaDecorator::~AmigaDecorator()
{
}


void
AmigaDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
AmigaDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
AmigaDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
AmigaDecorator::UpdateColors(DesktopSettings& settings)
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
AmigaDecorator::_DoLayout()
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
		// the resize gadget, over the frame's corner and the content's
		fResizeRect.Set(fBorderRect.right - kResizeWidth + 1,
			fBorderRect.bottom - kResizeHeight + 1, fBorderRect.right,
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
AmigaDecorator::_DoOutlineLayout()
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


/*!	A white frame two pixels wide round the content. A full title bar draws
	the top of it.
*/
void
AmigaDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const rgb_color edge = IsFocus(fTopTab)
		? fFocusFrameColor : fNonFocusFrameColor;
	if (!_HasFullTitleBar())
		fDrawingEngine->FillRect(fTopBorder & rect, edge);
	fDrawingEngine->FillRect(fLeftBorder & rect, edge);
	fDrawingEngine->FillRect(fRightBorder & rect, edge);
	fDrawingEngine->FillRect(fBottomBorder & rect, edge);

	if (_HasResizeCorner())
		_DrawResizeCorner();
}


void
AmigaDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar: white, with two blue stripes across its middle between its plain
	ends, the title in blue on a white patch at the left after the close
	gadget, and the gadgets. An inactive window's stripes are dotted with
	white.
*/
void
AmigaDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	fDrawingEngine->FillRect(t & invalid,
		focus ? fFocusTabColor : fNonFocusTabColor);

	const float from = t.left + kBarLeft;
	const float to = t.right - kBarRight;
	for (int32 i = 0; i < 2; i++) {
		const BRect stripe(from, t.top + kStripes[i], to,
			t.top + kStripes[i] + kStripeRows - 1);
		if (stripe.IsValid())
			fDrawingEngine->FillRect(stripe & invalid, kBlue);
	}

	_DrawTitle(tab, t);

	if (!focus) {
		// white dots over the stripes, from the title to the depth gadgets
		const float dotsTo = tab->minimizeRect.IsValid()
			? tab->minimizeRect.left - 2 : to;
		for (int32 i = 0; i < 2; i++) {
			for (int32 row = 0; row < kStripeRows; row++) {
				const float y = t.top + kStripes[i] + row;
				const float offset = row < kStripeRows / 2 ? 0 : 2;
				for (float x = from + offset; x <= dotsTo; x += kDotSpacing) {
					fDrawingEngine->FillRect(BRect(x, y, x, y) & invalid,
						kWhite);
				}
			}
		}
	}

	_DrawButtons(tab, invalid);
}


/*!	Where the title goes: at the left, after the close gadget, as Workbench
	put it, with its baseline as far down as Topaz's was.
*/
BRect
AmigaDecorator::_TitleRect(const Decorator::Tab* tab) const
{
	const BRect& t = tab->tabRect;
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	const float width = fDrawState.Font().StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength);

	const float x = t.left + kBarLeft;
	const float baseline = floorf(t.top + (kTitleHeight + fontHeight.ascent
		- fontHeight.descent) / 2);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


//!	The title in blue on a white patch three pixels wider.
void
AmigaDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const bool focus = IsFocus(tab);
	const BRect title = _TitleRect(tab);
	const BRect& t = tab->tabRect;

	if (tab->truncatedTitleLength > 0) {
		fDrawingEngine->FillRect(BRect(title.left, t.top, title.right + 3,
			t.bottom), focus ? fFocusTabColor : fNonFocusTabColor);
	}
	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());
	fDrawingEngine->SetHighColor(focus ? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		BPoint(title.left, title.bottom));
	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
AmigaDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
AmigaDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, kCloseGadget, kCloseWidth, kGadgetRows,
		tab->closePressed);
}


//!	The depth gadget that sends a window back; here, it minimizes.
void
AmigaDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, kLowerGadget, kDepthWidth, kGadgetRows,
		tab->minimizePressed);
}


//!	The depth gadget that brings a window to the front; here, it zooms.
void
AmigaDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawGadget(direct, rect, kRaiseGadget, kDepthWidth, kGadgetRows,
		tab->zoomPressed);
}


void
AmigaDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
AmigaDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
AmigaDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
AmigaDecorator::_SetSettings(const BMessage& settings,
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
AmigaDecorator::_UpdateFont(DesktopSettings& settings)
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
AmigaDecorator::_MoveBy(BPoint offset)
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
AmigaDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
AmigaDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
AmigaDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
AmigaDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
AmigaDecorator::_GetFootprint(BRegion* region)
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
AmigaDecorator::_HasTitle() const
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
AmigaDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its plain ends and its title.
float
AmigaDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const char* title = tab->title.String();
	return kBarLeft + kBarRight + 3 + 8
		+ ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
AmigaDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
AmigaDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / (_IsSmall() ? 11.0f : 12.0f),
		1.0f);
}


//!	The gadgets are pixels, so the bar keeps their height whatever the font.
float
AmigaDecorator::_TitleHeight() const
{
	return kTitleHeight;
}


bool
AmigaDecorator::_HasResizeCorner() const
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
AmigaDecorator::_LayoutTabs()
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
	// over the frame's top, which is white like the bar's last rows
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

		// close four pixels in at the left, and the depth gadgets at the right,
		// their edges overlapping by two, as HOTDOG lays them out
		tab->closeRect = closable ? BRect(tabLeft + 4, top,
			tabLeft + 4 + kCloseWidth - 1, top + kGadgetRows - 1)
			: BRect(0, 0, -1, -1);
		const float raiseLeft = tabRight - 2 - kDepthWidth;
		tab->zoomRect = zoomable ? BRect(raiseLeft, top,
			raiseLeft + kDepthWidth - 1, top + kGadgetRows - 1)
			: BRect(0, 0, -1, -1);
		const float lowerLeft = raiseLeft - kDepthWidth + 2;
		tab->minimizeRect = minimizable ? BRect(lowerLeft, top,
			lowerLeft + kDepthWidth - 1, top + kGadgetRows - 1)
			: BRect(0, 0, -1, -1);

		// the title from the close gadget to the depth gadgets
		const float reserved = kBarLeft;
		const float room = (tabRight - tabLeft + 1) - kBarLeft - kBarRight - 3;
		const float taken = kBarLeft + kBarRight + 3;

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


//!	The resize gadget, over the frame's bottom right corner.
void
AmigaDecorator::_DrawResizeCorner()
{
	_DrawGadget(false, fResizeRect, kResizeGadget, kResizeWidth, kResizeHeight,
		false);
}


/*!	Sets a gadget's pixels, each row in runs of one colour, from its picture,
	in the pressed palette when it is held down.
*/
void
AmigaDecorator::_DrawGadget(bool direct, BRect rect, const char* const* rows,
	int32 width, int32 height, bool pressed)
{
	if (!rect.IsValid())
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	for (int32 y = 0; y < height; y++) {
		const char* row = rows[y];
		int32 start = 0;
		for (int32 x = 1; x <= width; x++) {
			if (x < width && row[x] == row[start])
				continue;
			bool draw;
			const rgb_color& color = gadget_color(row[start], pressed, draw);
			if (draw) {
				fDrawingEngine->FillRect(BRect(rect.left + start, rect.top + y,
					rect.left + x - 1, rect.top + y), color);
			}
			start = x;
		}
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) AmigaDecorAddOn(id, name);
}
