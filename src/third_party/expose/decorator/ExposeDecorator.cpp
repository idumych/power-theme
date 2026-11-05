/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * See ExposeDecorator.h. Colours and sizes were measured from the artwork, and
 * layout values come from its settings (art/exposedarkrc), at a 12 point title.
 */


#include "ExposeDecorator.h"

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

#include "ExposeButtons.h"


static const rgb_color kOutline = { 0x2b, 0x2b, 0x2b, 255 };
static const rgb_color kLine = { 0x85, 0x85, 0x85, 255 };
static const rgb_color kInactiveLine = { 0x66, 0x66, 0x66, 255 };
static const rgb_color kFace = { 0x2a, 0x2a, 0x2a, 255 };
static const rgb_color kDarkLine = { 0x17, 0x17, 0x17, 255 };
static const rgb_color kTitleShadow = { 0, 0, 0, 255 };

// the title bar's two halves, and the inactive bar's single ramp
static const uint8 kUpperTop = 0x6f;
static const uint8 kUpperBottom = 0x56;
static const uint8 kLowerTop = 0x3d;
static const uint8 kLowerBottom = 0x2a;
static const uint8 kInactiveTop = 0x27;
static const uint8 kInactiveBottom = 0x4b;
// how much of the inactive bar is flat before its ramp starts
static const float kInactiveFlat = 0.14f;

// a corner, as in the artwork: what its first rows are cut in by, and then its
// outline ('O') and pale line ('L') on whatever is inside them
static const int32 kCorner[] = { 5, 3, 1, 1 };
static const int32 kCornerRows = sizeof(kCorner) / sizeof(kCorner[0]);
static const char* const kCornerLines[] = {
	"     O",
	"   OLL",
	" OLL  ",
	" OL   ",
	"OLL   ",
	"OL    ",
};
static const int32 kCornerSize = 6;

static const float kBorder = 5;
static const float kSmallBorder = 3;
static const float kTitleHeight = 32;
static const float kSmallTitleHeight = 22;
static const float kButtonGap = 4;
static const float kButtonEdge = 8;
static const float kTitleLeft = 12;
static const float kShadowOffset = 2;

static const float kResizeCornerLength = 14;
static const float kBorderResizeLength = 22;


static rgb_color
grey(float value)
{
	const uint8 level = (uint8)roundf(value);
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


//	#pragma mark - ExposeDecorAddOn


ExposeDecorAddOn::ExposeDecorAddOn(image_id id, const char* name)
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
ExposeDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) ExposeDecorator(settings, rect, desktop, fBeOSTabs);
}


//	#pragma mark - ExposeDecorator


ExposeDecorator::ExposeDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


ExposeDecorator::~ExposeDecorator()
{
}


void
ExposeDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
ExposeDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
ExposeDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
ExposeDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
}


//	#pragma mark - Protected methods


void
ExposeDecorator::_DoLayout()
{
	const float scale = _Scale();

	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
			fBorderWidth = (int32)roundf(kBorder * scale);
			break;

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			fBorderWidth = (int32)roundf(kSmallBorder * scale);
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	if (fBorderWidth > 0) {
		// A full title bar sits straight on the content: a window with one has
		// only the bar's last row for a top border.
		const float top = _HasFullTitleBar() ? 1 : fBorderWidth;
		fLeftBorder.Set(fFrame.left - fBorderWidth, fFrame.top,
			fFrame.left - 1, fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top,
			fFrame.right + fBorderWidth, fFrame.bottom);
		fTopBorder.Set(fFrame.left - fBorderWidth, fFrame.top - top,
			fFrame.right + fBorderWidth, fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - fBorderWidth, fFrame.bottom + 1,
			fFrame.right + fBorderWidth, fFrame.bottom + fBorderWidth);
	} else {
		fLeftBorder.Set(0, 0, -1, -1);
		fRightBorder.Set(0, 0, -1, -1);
		fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(0, 0, -1, -1);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	if (fBorderWidth > 1) {
		// the content's corner, as far in as a scroll bar's width
		const float knob = roundf(kResizeCornerLength * scale) + fBorderWidth;
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
ExposeDecorator::_DoOutlineLayout()
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


void
ExposeDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const BRect& r = fBorderRect;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		fDrawingEngine->StrokeRect(r, kOutline);
		return;
	}

	const bool focus = IsFocus(fTopTab);
	const rgb_color line = focus ? kLine : kInactiveLine;

	if (!_HasFullTitleBar())
		fDrawingEngine->FillRect(fTopBorder, kFace);
	fDrawingEngine->FillRect(fLeftBorder, kFace);
	fDrawingEngine->FillRect(fRightBorder, kFace);
	fDrawingEngine->FillRect(fBottomBorder, kFace);

	// the dark line that runs round a focused frame's bottom
	if (focus && fBorderWidth >= 5) {
		fDrawingEngine->StrokeLine(BPoint(fFrame.left, fFrame.bottom + 2),
			BPoint(fFrame.right, fFrame.bottom + 2), kDarkLine);
	}

	// A full title bar draws these beside itself. Its last row is the top border.
	const float top = _HasFullTitleBar() ? fFrame.top : r.top;
	fDrawingEngine->StrokeLine(BPoint(r.left + 1, top),
		BPoint(r.left + 1, r.bottom - 1), line);
	fDrawingEngine->StrokeLine(BPoint(r.right - 1, top),
		BPoint(r.right - 1, r.bottom - 1), line);
	fDrawingEngine->StrokeLine(BPoint(r.left + 1, r.bottom - 1),
		BPoint(r.right - 1, r.bottom - 1), line);
	if (!_HasFullTitleBar()) {
		fDrawingEngine->StrokeLine(BPoint(r.left + 1, r.top + 1),
			BPoint(r.right - 1, r.top + 1), line);
	}

	fDrawingEngine->StrokeLine(BPoint(r.left, top), r.LeftBottom(), kOutline);
	fDrawingEngine->StrokeLine(BPoint(r.right, top), r.RightBottom(), kOutline);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), kOutline);
	if (!_HasFullTitleBar())
		fDrawingEngine->StrokeLine(r.LeftTop(), r.RightTop(), kOutline);

	_DrawCorner(r.left, r.bottom, 1, -1, line);
	_DrawCorner(r.right, r.bottom, -1, -1, line);
	if (!_HasFullTitleBar()) {
		if (_TopLeftIsRound())
			_DrawCorner(r.left, r.top, 1, 1, line);
		if (_TopRightIsRound())
			_DrawCorner(r.right, r.top, -1, 1, line);
	}

	if (_HasResizeCorner())
		_DrawResizeCorner();
}


void
ExposeDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


void
ExposeDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const rgb_color line = focus ? kLine : kInactiveLine;
	const float height = t.Height() + 1;

	const float first = std::max(t.top, floorf(invalid.top));
	const float last = std::min(t.bottom, ceilf(invalid.bottom));
	for (float y = first; y <= last; y++) {
		fDrawingEngine->FillRect(BRect(t.left, y, t.right, y),
			_TitleColorAt(focus, y - t.top, height));
	}

	// the ends of the bar, or of a BeOS-style tab or stack
	if (t.left == fTitleBarRect.left) {
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
			BPoint(t.left, t.bottom), kOutline);
		fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 2),
			BPoint(t.left + 1, t.bottom), line);
		_DrawCorner(t.left, t.top, 1, 1, line);
	} else {
		// between two tabs of a stack
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
			BPoint(t.left, t.bottom), kDarkLine);
	}
	if (t.right == fTitleBarRect.right) {
		fDrawingEngine->StrokeLine(BPoint(t.right, t.top + 1),
			BPoint(t.right, t.bottom), kOutline);
		fDrawingEngine->StrokeLine(BPoint(t.right - 1, t.top + 2),
			BPoint(t.right - 1, t.bottom), line);
		_DrawCorner(t.right, t.top, -1, 1, line);
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


void
ExposeDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& t = tab->tabRect;

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint position;
	position.x = t.left + tab->textOffset;
	position.y = floorf((t.top + t.bottom + 1 + fontHeight.ascent
		- fontHeight.descent) / 2);

	if (!_IsSmall()) {
		fDrawingEngine->SetHighColor(kTitleShadow);
		fDrawingEngine->DrawString(tab->truncatedTitle,
			tab->truncatedTitleLength,
			position + BPoint(kShadowOffset, kShadowOffset));
	}

	fDrawingEngine->SetHighColor(IsFocus(tab)
		? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
ExposeDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
}


void
ExposeDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->closePressed)
		_DrawButton(tab, direct, rect, kClosePressedLarge, kClosePressedSmall);
	else if (IsFocus(tab))
		_DrawButton(tab, direct, rect, kCloseActiveLarge, kCloseActiveSmall);
	else
		_DrawButton(tab, direct, rect, kCloseInactiveLarge, kCloseInactiveSmall);
}


void
ExposeDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->zoomPressed)
		_DrawButton(tab, direct, rect, kZoomPressedLarge, kZoomPressedSmall);
	else if (IsFocus(tab))
		_DrawButton(tab, direct, rect, kZoomActiveLarge, kZoomActiveSmall);
	else
		_DrawButton(tab, direct, rect, kZoomInactiveLarge, kZoomInactiveSmall);
}


void
ExposeDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->minimizePressed) {
		_DrawButton(tab, direct, rect, kMinimizePressedLarge,
			kMinimizePressedSmall);
	} else if (IsFocus(tab)) {
		_DrawButton(tab, direct, rect, kMinimizeActiveLarge,
			kMinimizeActiveSmall);
	} else {
		_DrawButton(tab, direct, rect, kMinimizeInactiveLarge,
			kMinimizeInactiveSmall);
	}
}


void
ExposeDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
ExposeDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
ExposeDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
ExposeDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
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
ExposeDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (_IsSmall())
		settings.GetDefaultPlainFont(font);
	else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
ExposeDecorator::_MoveBy(BPoint offset)
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
ExposeDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
ExposeDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
ExposeDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
ExposeDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
ExposeDecorator::_GetFootprint(BRegion* region)
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

	region->Include(fTopBorder);
	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fBottomBorder);
	if (fTitleBarRect.IsValid())
		region->Include(fTitleBarRect);
	if (_HasResizeCorner())
		region->Include(fResizeRect);

	const BRect outer = _OuterRect();
	if (_HasFullTitleBar()) {
		_ExcludeCorner(region, outer.left, outer.top, 1, 1);
		_ExcludeCorner(region, outer.right, outer.top, -1, 1);
	} else {
		if (_TopLeftIsRound())
			_ExcludeCorner(region, fBorderRect.left, fBorderRect.top, 1, 1);
		if (_TopRightIsRound())
			_ExcludeCorner(region, fBorderRect.right, fBorderRect.top, -1, 1);
		if (fTitleBarRect.IsValid()) {
			_ExcludeCorner(region, fTitleBarRect.left, fTitleBarRect.top, 1, 1);
			_ExcludeCorner(region, fTitleBarRect.right, fTitleBarRect.top, -1, 1);
		}
	}
	_ExcludeCorner(region, outer.left, outer.bottom, 1, -1);
	_ExcludeCorner(region, outer.right, outer.bottom, -1, -1);
}


//	#pragma mark - Private methods


bool
ExposeDecorator::_HasTitle() const
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
ExposeDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	A corner of a frame with a top edge is square where a tab meets it.
bool
ExposeDecorator::_TopLeftIsRound() const
{
	return !fTitleBarRect.IsValid() || fTitleBarRect.left > fBorderRect.left;
}


bool
ExposeDecorator::_TopRightIsRound() const
{
	return !fTitleBarRect.IsValid() || fTitleBarRect.right < fBorderRect.right;
}


//!	How wide a BeOS-style tab has to be, for its title and buttons.
float
ExposeDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float scale = _Scale();
	const float size = _IsSmall() ? kSmallButton : kLargeButton;
	const float edge = _IsSmall() ? 4 : roundf(kButtonEdge * scale);
	const float gap = _IsSmall() ? 4 : roundf(kTitleLeft * scale);

	float width = 2 * edge + 2 * gap;
	if ((tab->flags & B_NOT_CLOSABLE) == 0)
		width += size;
	if ((tab->flags & B_NOT_ZOOMABLE) == 0)
		width += size;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
ExposeDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
ExposeDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / 12.0f, 1.0f);
}


float
ExposeDecorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


bool
ExposeDecorator::_HasResizeCorner() const
{
	return fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0;
}


BRect
ExposeDecorator::_OuterRect() const
{
	return fTitleBarRect.IsValid() ? fBorderRect | fTitleBarRect : fBorderRect;
}


/*!	Lays the tabs out side by side across the whole window, each with its own
	buttons at its right end, in the artwork's order: minimize, zoom, close.

	A BeOS-style tab sits on the frame, over its top edge so that they join up, and
	has close on its left and zoom on its right. A lone one is as wide as its title
	and can slide; a stack's share the window's width.
*/
void
ExposeDecorator::_LayoutTabs()
{
	const int32 count = fTabList.CountItems();
	if (count == 0) {
		fTitleBarRect.Set(0, 0, -1, -1);
		return;
	}

	const ServerFont& font = fDrawState.Font();
	const float scale = _Scale();
	float left = fFrame.left - fBorderWidth;
	float width = fFrame.Width() + 1 + 2 * fBorderWidth;
	const float height = _TitleHeight();
	const float bottom = fBeOSTabs ? fBorderRect.top + 1 : fFrame.top - 1;
	const float top = bottom - height + 1;
	if (fBeOSTabs && count == 1) {
		Decorator::Tab* tab = fTabList.ItemAt(0);
		const float tabWidth = std::min(_TabWidth(tab), width);
		left += std::min(tab->tabLocation, width - tabWidth);
		width = tabWidth;
	}
	const float size = _IsSmall() ? kSmallButton : kLargeButton;
	const float gap = _IsSmall() ? 2 : roundf(kButtonGap * scale);
	const float edge = _IsSmall() ? 4 : roundf(kButtonEdge * scale);
	const float textOffset = _IsSmall() ? 8 : roundf(kTitleLeft * scale);
	// the room a tab needs apart from its title
	const float minTitle = roundf(32 * scale);

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab->tabRect.Set(tabLeft, top, tabRight, bottom);

		BRect* rects[3] = { &tab->closeRect, &tab->zoomRect, &tab->minimizeRect };
		const uint32 hidden[3] = { B_NOT_CLOSABLE, B_NOT_ZOOMABLE,
			B_NOT_MINIMIZABLE };
		// a BeOS-style tab doesn't reach the two rows it has over the frame
		const float buttonTop = top + floorf((height - (fBeOSTabs ? 2 : 0) - size) / 2);
		float right = tabRight - edge;
		float buttons = 0;
		float titleLeft = tabLeft + textOffset;
		for (int32 b = 0; b < 3; b++) {
			if ((tab->flags & hidden[b]) != 0
				|| (fBeOSTabs && hidden[b] == B_NOT_MINIMIZABLE)) {
				rects[b]->Set(0, 0, -1, -1);
				continue;
			}
			if (fBeOSTabs && hidden[b] == B_NOT_CLOSABLE) {
				rects[b]->Set(tabLeft + edge, buttonTop, tabLeft + edge + size - 1,
					buttonTop + size - 1);
				titleLeft = rects[b]->right + 1 + (_IsSmall() ? 4 : gap + 2);
			} else {
				rects[b]->Set(right - size + 1, buttonTop, right,
					buttonTop + size - 1);
				right -= size + gap;
			}
			buttons += size + gap;
		}

		tab->tabOffset = (uint32)(tabLeft - fBorderRect.left);
		tab->textOffset = titleLeft - tabLeft;
		if (fBeOSTabs && count > 1)
			tab->tabLocation = 0;
		tab->minTabSize = 0;
		const char* title = Title(tab);
		tab->maxTabSize = textOffset + edge + buttons
			+ ceilf(font.StringWidth(title, strlen(title)));
		minimum += tab->textOffset + edge + buttons + minTitle;

		tab->truncatedTitle = title;
		font.TruncateString(&tab->truncatedTitle,
			count > 1 ? B_TRUNCATE_END : B_TRUNCATE_MIDDLE,
			std::max(0.0f, right - titleLeft - gap));
		tab->truncatedTitleLength = tab->truncatedTitle.Length();
	}

	// Decorator::GetSizeLimits() keeps the window at least this wide
	fTabList.ItemAt(0)->minTabSize = minimum;

	fTitleBarRect.Set(left, top, left + width - 1, bottom);
}


/*!	The grey of a title bar's row: the outline, the pale line, then the body, and
	a dark line along the bottom.
*/
rgb_color
ExposeDecorator::_TitleColorAt(bool focus, float row, float height) const
{
	if (row <= 0)
		return kOutline;
	if (row == 1)
		return focus ? kLine : kInactiveLine;
	if (row >= height - 1)
		return kDarkLine;

	const float body = height - 3;
	const float at = row - 2;
	if (!focus) {
		const float flat = roundf(body * kInactiveFlat);
		if (at < flat)
			return grey(kInactiveTop);
		return grey(kInactiveTop + (kInactiveBottom - kInactiveTop)
			* (at - flat + 1) / (body - flat));
	}

	// the lower half is a row longer than the upper one, as in the artwork
	const float upper = floorf(body / 2);
	if (at < upper) {
		return grey(kUpperTop + (float(kUpperBottom) - kUpperTop)
			* at / std::max(upper - 1, 1.0f));
	}
	return grey(kLowerTop + (float(kLowerBottom) - kLowerTop)
		* (at - upper) / std::max(body - upper - 1, 1.0f));
}


void
ExposeDecorator::_ExcludeCorner(BRegion* region, float x, float y,
	int32 xDirection, int32 yDirection) const
{
	for (int32 row = 0; row < kCornerRows; row++) {
		const float rowY = y + row * yDirection;
		const float end = x + (kCorner[row] - 1) * xDirection;
		region->Exclude(BRect(std::min(x, end), rowY, std::max(x, end), rowY));
	}
}


void
ExposeDecorator::_DrawCorner(float x, float y, int32 xDirection,
	int32 yDirection, const rgb_color& line)
{
	for (int32 row = 0; row < kCornerSize; row++) {
		for (int32 column = 0; column < kCornerSize; column++) {
			const char mark = kCornerLines[row][column];
			if (mark == ' ')
				continue;
			const float px = x + column * xDirection;
			const float py = y + row * yDirection;
			fDrawingEngine->FillRect(BRect(px, py, px, py),
				mark == 'O' ? kOutline : line);
		}
	}
}


/*!	On what would be the content's corner: the frame's face, with a grip of three
	short diagonals.
*/
void
ExposeDecorator::_DrawResizeCorner()
{
	const float length = fResizeRect.Width() - fBorderWidth;
	const BRect corner(fFrame.right - length + 1, fFrame.bottom - length + 1,
		fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kFace);

	const rgb_color light = IsFocus(fTopTab) ? kLine : kInactiveLine;
	for (int32 i = 0; i < 3; i++) {
		const float reach = 3 + i * 4;
		const float right = fFrame.right - 2;
		const float bottom = fFrame.bottom - 2;
		fDrawingEngine->StrokeLine(BPoint(right - reach, bottom),
			BPoint(right, bottom - reach), light);
		fDrawingEngine->StrokeLine(BPoint(right - reach + 1, bottom),
			BPoint(right, bottom - reach + 1), kDarkLine);
	}
}


/*!	Sets a button's pixels by hand, each row in runs of one colour. A decorator
	can't blend, so its soft edges are mixed with the title bar's row here.
*/
void
ExposeDecorator::_DrawButton(Decorator::Tab* tab, bool direct, BRect rect,
	const uint32* large, const uint32* small)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const int32 size = _IsSmall() ? kSmallButton : kLargeButton;
	const uint32* pixels = _IsSmall() ? small : large;
	const bool focus = IsFocus(tab);
	const float height = tab->tabRect.Height() + 1;

	for (int32 y = 0; y < size; y++) {
		const float rowY = rect.top + y;
		const rgb_color under = _TitleColorAt(focus, rowY - tab->tabRect.top,
			height);

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


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) ExposeDecorAddOn(id, name);
}
