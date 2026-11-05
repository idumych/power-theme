/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * See PlatinumDecorator.h. Colours and sizes were measured from the artwork, and
 * layout values come from its settings (art/Platinumrc).
 */


#include "PlatinumDecorator.h"

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

#include "PlatinumButtons.h"


#define GREY(level) { level, level, level, 255 }

static const rgb_color kOutline = GREY(0x32);
static const rgb_color kLight = GREY(0xe3);
static const rgb_color kFace = GREY(0xc7);
static const rgb_color kDim = GREY(0x6d);
static const rgb_color kInactiveOutline = GREY(0x37);
static const rgb_color kInactiveFace = GREY(0xdd);
// the artwork's is black, three quarters opaque: a decorator can't blend, so this
// is what that comes to over a mid-toned desktop
static const rgb_color kShadow = GREY(0x20);
static const rgb_color kInactiveShadow = GREY(0x37);
static const rgb_color kCornerFace = GREY(0xdd);

// The frame's rings, from the outside in. The inside of the left and top is lit,
// and of the right and bottom in shade, so the content looks set in.
static const rgb_color kLeftRings[] = { kOutline, kLight, kFace, kFace, kFace,
	kDim, kOutline, kLight };
static const rgb_color kRightRings[] = { kOutline, kDim, kDim, kFace, kFace, kLight,
	kOutline, kDim };
static const rgb_color kBottomRings[] = { kOutline, kDim, kFace, kFace, kLight,
	kLight, kOutline, kDim };
static const rgb_color kInactiveLeftRings[] = { kInactiveOutline, kInactiveFace,
	kInactiveFace, kInactiveFace, kInactiveFace, kInactiveFace, kInactiveOutline,
	kInactiveFace };
static const rgb_color kInactiveRightRings[] = { kInactiveOutline, kInactiveOutline,
	kInactiveFace, kInactiveFace, kInactiveFace, kInactiveFace, kInactiveFace,
	kInactiveOutline };
static const int32 kRings = 8;

// floating windows have thin frames with no recess
static const rgb_color kSmallLeftRings[] = { kOutline, kLight, kFace, kOutline };
static const rgb_color kSmallRightRings[] = { kOutline, kDim, kFace, kOutline };
static const rgb_color kSmallInactiveRings[] = { kInactiveOutline, kInactiveFace,
	kInactiveFace, kInactiveOutline };
static const int32 kSmallRings = 4;

// the stripes, top to bottom
static const uint8 kStripes[] = { 0xd5, 0xb3, 0xd5, 0xb3, 0xb3, 0xd5, 0xb3, 0xd5,
	0xb3, 0xd5, 0xb3 };
static const int32 kStripeRows = sizeof(kStripes) / sizeof(kStripes[0]);

static const float kTitleHeight = 23;
static const float kSmallTitleHeight = 16;
static const float kButtonGap = 3;
static const float kTitleGap = 8;
// how much of the stripes a BeOS-style tab shows on each side of its title
static const float kTabStripes = 12;
static const float kResizeCornerLength = 14;
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


//	#pragma mark - PlatinumDecorAddOn


PlatinumDecorAddOn::PlatinumDecorAddOn(image_id id, const char* name)
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
PlatinumDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) PlatinumDecorator(settings, rect, desktop, fBeOSTabs);
}


//	#pragma mark - PlatinumDecorator


PlatinumDecorator::PlatinumDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


PlatinumDecorator::~PlatinumDecorator()
{
}


void
PlatinumDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
PlatinumDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
PlatinumDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
PlatinumDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
}


//	#pragma mark - Protected methods


void
PlatinumDecorator::_DoLayout()
{
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
			fBorderWidth = kRings;
			break;

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			fBorderWidth = kSmallRings;
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	if (fBorderWidth > 0) {
		// A window with a full title bar has only its last row for a top border. Those
		// with frames have a shadow as well, one wider on the right and bottom.
		const float shadow = fBorderWidth > 1 ? 1 : 0;
		const float top = _HasFullTitleBar() ? 1 : fBorderWidth;
		fLeftBorder.Set(fFrame.left - fBorderWidth, fFrame.top,
			fFrame.left - 1, fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top,
			fFrame.right + fBorderWidth + shadow, fFrame.bottom);
		fTopBorder.Set(fFrame.left - fBorderWidth, fFrame.top - top,
			fFrame.right + fBorderWidth + shadow, fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - fBorderWidth, fFrame.bottom + 1,
			fFrame.right + fBorderWidth + shadow,
			fFrame.bottom + fBorderWidth + shadow);
	} else {
		fLeftBorder.Set(0, 0, -1, -1);
		fRightBorder.Set(0, 0, -1, -1);
		fTopBorder.Set(0, 0, -1, -1);
		fBottomBorder.Set(0, 0, -1, -1);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	if (fBorderWidth > 1) {
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
PlatinumDecorator::_DoOutlineLayout()
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


/*!	Draws the rings of the frame from the outside in, each stopping where the next
	side's ring takes over. The title bar draws what's beside itself.
*/
void
PlatinumDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		fDrawingEngine->StrokeRect(fBorderRect, kOutline);
		return;
	}

	const bool focus = IsFocus(fTopTab);
	const bool small = fBorderWidth == kSmallRings;
	const rgb_color* left = small
		? (focus ? kSmallLeftRings : kSmallInactiveRings)
		: (focus ? kLeftRings : kInactiveLeftRings);
	const rgb_color* right = small
		? (focus ? kSmallRightRings : kSmallInactiveRings)
		: (focus ? kRightRings : kInactiveRightRings);
	const rgb_color* bottom = small
		? (focus ? kSmallRightRings : kSmallInactiveRings)
		: (focus ? kBottomRings : kInactiveRightRings);

	// the frame, without the shadow on its right and bottom
	const BRect r(fBorderRect.left, fBorderRect.top, fBorderRect.right - 1,
		fBorderRect.bottom - 1);
	const float top = _HasFullTitleBar() ? fFrame.top : r.top;

	for (int32 i = 0; i < fBorderWidth; i++) {
		const float bottomY = r.bottom - i;
		const float rowTop = _HasFullTitleBar() ? top : top + i;
		if (!_HasFullTitleBar()) {
			fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
				BPoint(r.right - i, r.top + i), left[i]);
		}
		fDrawingEngine->StrokeLine(BPoint(r.left + i, rowTop),
			BPoint(r.left + i, bottomY), left[i]);
		fDrawingEngine->StrokeLine(BPoint(r.right - i, rowTop),
			BPoint(r.right - i, bottomY), right[i]);
		fDrawingEngine->StrokeLine(BPoint(r.left + i, bottomY),
			BPoint(r.right - i, bottomY), bottom[i]);
	}

	// the shadow, stepped in at each end
	const rgb_color shadow = focus ? kShadow : kInactiveShadow;
	const float shadowTop = _HasFullTitleBar()
		? std::max(fTitleBarRect.top + 1, rect.top) : r.top + 1;
	fDrawingEngine->StrokeLine(BPoint(fBorderRect.right, shadowTop),
		BPoint(fBorderRect.right, fBorderRect.bottom), shadow);
	fDrawingEngine->StrokeLine(BPoint(fBorderRect.left + 1, fBorderRect.bottom),
		BPoint(fBorderRect.right, fBorderRect.bottom), shadow);

	if (_HasResizeCorner())
		_DrawResizeCorner();
}


void
PlatinumDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


void
PlatinumDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const bool small = _IsSmall();
	const rgb_color face = focus ? kFace : kInactiveFace;
	const rgb_color outline = focus ? kOutline : kInactiveOutline;
	const rgb_color light = focus ? kLight : kInactiveFace;

	// the bar's end, without the shadow beside it
	const float barRight = fBeOSTabs ? fTitleBarRect.right : fBorderRect.right;
	const float rightEdge = barRight - 1;

	fDrawingEngine->FillRect(t, face);

	// the top; the rows at the bottom belong to the recess round the content
	fDrawingEngine->StrokeLine(t.LeftTop(), t.RightTop(), outline);
	fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
		BPoint(t.right, t.top + 1), light);

	if (t.left == fTitleBarRect.left) {
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top), t.LeftBottom(), outline);
		fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 1),
			BPoint(t.left + 1, t.bottom), light);
	} else {
		// between two tabs of a stack
		fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1),
			BPoint(t.left, t.bottom), outline);
	}

	const rgb_color* right = small
		? (focus ? kSmallRightRings : kSmallInactiveRings)
		: (focus ? kRightRings : kInactiveRightRings);
	if (t.right >= rightEdge) {
		// the frame's rings, and the shadow beyond them
		for (int32 i = 0; i < 3; i++) {
			fDrawingEngine->StrokeLine(BPoint(rightEdge - i, t.top + i),
				BPoint(rightEdge - i, t.bottom), right[i]);
		}
		fDrawingEngine->StrokeLine(BPoint(barRight, t.top + 1),
			BPoint(barRight, t.bottom), focus ? kShadow : kInactiveShadow);
	}

	if (fBeOSTabs) {
		// the frame below has its own recess
	} else if (!small) {
		// the recess runs along the bottom, inside the frame's face
		const float left = fFrame.left - 3;
		const float end = fFrame.right + 3;
		if (focus) {
			fDrawingEngine->StrokeLine(BPoint(left, t.bottom - 2),
				BPoint(end, t.bottom - 2), kDim);
			fDrawingEngine->StrokeLine(BPoint(left + 1, t.bottom - 1),
				BPoint(end - 1, t.bottom - 1), kOutline);
			fDrawingEngine->StrokeLine(BPoint(left + 2, t.bottom),
				BPoint(end - 2, t.bottom), kLight);
		} else {
			fDrawingEngine->StrokeLine(BPoint(left + 1, t.bottom - 1),
				BPoint(end - 1, t.bottom - 1), kInactiveOutline);
			fDrawingEngine->StrokeLine(BPoint(left + 1, t.bottom),
				BPoint(left + 1, t.bottom), kInactiveOutline);
			fDrawingEngine->StrokeLine(BPoint(end - 1, t.bottom),
				BPoint(end - 1, t.bottom), kInactiveOutline);
		}
	} else {
		fDrawingEngine->StrokeLine(BPoint(fFrame.left - 1, t.bottom),
			BPoint(fFrame.right + 1, t.bottom), outline);
	}

	if (focus) {
		// stripes between the buttons, with a gap for the title
		float left = t.left + (small ? 4 : 8);
		if (tab->closeRect.IsValid())
			left = tab->closeRect.right + kTitleGap;
		float right = t.right - (small ? 4 : 8);
		if (tab->minimizeRect.IsValid())
			right = tab->minimizeRect.left - kTitleGap;
		else if (tab->zoomRect.IsValid())
			right = tab->zoomRect.left - kTitleGap;

		const float width = fDrawingEngine->StringWidth(tab->truncatedTitle,
			tab->truncatedTitleLength, fDrawState.Font());
		const float middle = floorf((left + right) / 2);
		_DrawStripes(left, middle - ceilf(width / 2) - kTitleGap, t);
		_DrawStripes(middle + ceilf(width / 2) + kTitleGap, right, t);
	}

	_DrawTitle(tab, t);
	_DrawButtons(tab, invalid);
}


void
PlatinumDecorator::_DrawStripes(float left, float right, const BRect& tabRect)
{
	if (right <= left)
		return;

	// centred in the rows above the recess
	const int32 skip = _IsSmall() ? 3 : 0;
	const int32 rows = kStripeRows - 2 * skip;
	const float space = _TitleSpace(tabRect.Height() + 1);
	const float top = tabRect.top + floorf((space - rows) / 2);
	for (int32 row = 0; row < rows; row++) {
		const float y = top + row;
		fDrawingEngine->StrokeLine(BPoint(left, y), BPoint(right, y),
			grey(kStripes[row + skip]));
	}
}


void
PlatinumDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& t = tab->tabRect;

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetFont(fDrawState.Font());
	fDrawingEngine->SetHighColor(IsFocus(tab)
		? fFocusTextColor : fNonFocusTextColor);

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	// in the middle of what's between the buttons, and above the recess
	float left = t.left;
	if (tab->closeRect.IsValid())
		left = tab->closeRect.right + kTitleGap;
	else
		left += _IsSmall() ? 4 : 8;
	float right = t.right - (_IsSmall() ? 4 : 8);
	if (tab->minimizeRect.IsValid())
		right = tab->minimizeRect.left - kTitleGap;
	else if (tab->zoomRect.IsValid())
		right = tab->zoomRect.left - kTitleGap;

	const float width = fDrawingEngine->StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength, fDrawState.Font());
	const float space = _TitleSpace(t.Height() + 1);

	BPoint position;
	position.x = floorf((left + right - width) / 2);
	position.y = floorf(t.top + (space + fontHeight.ascent - fontHeight.descent) / 2);

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
PlatinumDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	// an inactive window has none to see
	if (!IsFocus(tab))
		return;

	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
}


void
PlatinumDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->closePressed)
		_DrawButton(tab, direct, rect, kClosePressedLarge, kClosePressedSmall);
	else
		_DrawButton(tab, direct, rect, kCloseActiveLarge, kCloseActiveSmall);
}


void
PlatinumDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->zoomPressed)
		_DrawButton(tab, direct, rect, kZoomPressedLarge, kZoomPressedSmall);
	else
		_DrawButton(tab, direct, rect, kZoomActiveLarge, kZoomActiveSmall);
}


void
PlatinumDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (tab->minimizePressed) {
		_DrawButton(tab, direct, rect, kMinimizePressedLarge,
			kMinimizePressedSmall);
	} else {
		_DrawButton(tab, direct, rect, kMinimizeActiveLarge,
			kMinimizeActiveSmall);
	}
}


void
PlatinumDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
PlatinumDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
PlatinumDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
PlatinumDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
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
PlatinumDecorator::_UpdateFont(DesktopSettings& settings)
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
PlatinumDecorator::_MoveBy(BPoint offset)
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
PlatinumDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
PlatinumDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
PlatinumDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
PlatinumDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
PlatinumDecorator::_GetFootprint(BRegion* region)
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

	// the shadow is stepped in at the two corners it starts from, and a
	// BeOS-style tab has one of its own
	if (_HasFullTitleBar()) {
		region->Exclude(BRect(fBorderRect.right, fTitleBarRect.top,
			fBorderRect.right, fTitleBarRect.top));
	} else {
		if (_TopRightIsRound()) {
			region->Exclude(BRect(fBorderRect.right, fBorderRect.top,
				fBorderRect.right, fBorderRect.top));
		}
		if (fTitleBarRect.IsValid()) {
			region->Exclude(BRect(fTitleBarRect.right, fTitleBarRect.top,
				fTitleBarRect.right, fTitleBarRect.top));
		}
	}
	region->Exclude(BRect(fBorderRect.left, fBorderRect.bottom, fBorderRect.left,
		fBorderRect.bottom));
}


//	#pragma mark - Private methods


bool
PlatinumDecorator::_HasTitle() const
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
PlatinumDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	The frame's top right corner is covered where a tab reaches it.
bool
PlatinumDecorator::_TopRightIsRound() const
{
	return !fTitleBarRect.IsValid() || fTitleBarRect.right < fBorderRect.right;
}


//!	The rows of a bar that its title and buttons are centred in.
float
PlatinumDecorator::_TitleSpace(float height) const
{
	// a BeOS-style tab has two rows over the frame; a full bar has the recess
	if (fBeOSTabs)
		return height - 2;
	return height - (_IsSmall() ? 1 : 3);
}


//!	How wide a BeOS-style tab has to be: its title, buttons and some stripes.
float
PlatinumDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	const float size = _IsSmall() ? kSmallButton : kLargeButton;
	const float edge = _IsSmall() ? 4 : 8;

	float width = 2 * edge + 1 + 2 * kTitleGap + 2 * (kTabStripes + kTitleGap);
	if ((tab->flags & B_NOT_CLOSABLE) == 0)
		width += size;
	if ((tab->flags & B_NOT_ZOOMABLE) == 0)
		width += size;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
PlatinumDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
PlatinumDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / 12.0f, 1.0f);
}


float
PlatinumDecorator::_TitleHeight() const
{
	return roundf((_IsSmall() ? kSmallTitleHeight : kTitleHeight) * _Scale());
}


bool
PlatinumDecorator::_HasResizeCorner() const
{
	return fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0;
}


/*!	Lays the tabs out side by side across the whole window, each with its close
	button at its left end and its others at its right.

	A BeOS-style tab sits on the frame, over its top edge so that they join up, and
	has no minimize button. A lone one is as wide as its title and can slide; a
	stack's share the window's width.
*/
void
PlatinumDecorator::_LayoutTabs()
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
	const float bottom = fBeOSTabs ? fBorderRect.top + 1 : fFrame.top - 1;
	const float top = bottom - height + 1;
	if (fBeOSTabs && count == 1) {
		Decorator::Tab* tab = fTabList.ItemAt(0);
		const float tabWidth = std::min(_TabWidth(tab), width);
		left += std::min(tab->tabLocation, width - tabWidth);
		width = tabWidth;
	}
	const float size = _IsSmall() ? kSmallButton : kLargeButton;
	const float edge = _IsSmall() ? 4 : 8;
	const float buttonTop = top + floorf((_TitleSpace(height) - size) / 2);
	const float minTitle = roundf(32 * scale);

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab->tabRect.Set(tabLeft, top, tabRight, bottom);

		float titleLeft = tabLeft + edge;
		if ((tab->flags & B_NOT_CLOSABLE) == 0) {
			tab->closeRect.Set(tabLeft + edge, buttonTop, tabLeft + edge + size - 1,
				buttonTop + size - 1);
			titleLeft = tab->closeRect.right + kTitleGap;
		} else
			tab->closeRect.Set(0, 0, -1, -1);

		float right = tabRight - edge - 1;
		if ((tab->flags & B_NOT_ZOOMABLE) == 0) {
			tab->zoomRect.Set(right - size + 1, buttonTop, right,
				buttonTop + size - 1);
			right -= size + kButtonGap;
		} else
			tab->zoomRect.Set(0, 0, -1, -1);
		if ((tab->flags & B_NOT_MINIMIZABLE) == 0 && !fBeOSTabs) {
			tab->minimizeRect.Set(right - size + 1, buttonTop, right,
				buttonTop + size - 1);
			right -= size + kButtonGap;
		} else
			tab->minimizeRect.Set(0, 0, -1, -1);

		const float room = right - kTitleGap - titleLeft;
		const float taken = (tabRight - tabLeft + 1) - room;

		tab->tabOffset = (uint32)(tabLeft - fBorderRect.left);
		tab->textOffset = titleLeft - tabLeft;
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


/*!	On what would be the content's corner: the frame's face, and a grip of dots. */
void
PlatinumDecorator::_DrawResizeCorner()
{
	const float length = roundf(kResizeCornerLength * _Scale());
	const BRect corner(fFrame.right - length + 1, fFrame.bottom - length + 1,
		fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kCornerFace);

	const bool focus = IsFocus(fTopTab);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(),
		focus ? kOutline : kInactiveOutline);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(),
		focus ? kOutline : kInactiveOutline);

	if (!focus)
		return;

	for (int32 row = 0; row < 4; row++) {
		for (int32 column = 0; column <= row; column++) {
			const float x = corner.right - 2 - column * 3;
			const float y = corner.bottom - 2 - (3 - row) * 3;
			fDrawingEngine->FillRect(BRect(x, y, x, y), kDim);
			fDrawingEngine->FillRect(BRect(x - 1, y - 1, x - 1, y - 1), kLight);
		}
	}
}


/*!	Sets a button's pixels by hand, each row in runs of one colour. A decorator
	can't blend, so its soft edges are mixed with the title bar's face here.
*/
void
PlatinumDecorator::_DrawButton(Decorator::Tab* tab, bool direct, BRect rect,
	const uint32* large, const uint32* small)
{
	if (!rect.IsValid() || rect.Width() < 1 || !IsFocus(tab))
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const int32 size = _IsSmall() ? kSmallButton : kLargeButton;
	const uint32* pixels = _IsSmall() ? small : large;
	const rgb_color under = kFace;

	for (int32 y = 0; y < size; y++) {
		const float rowY = rect.top + y;

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
	return new (std::nothrow) PlatinumDecorAddOn(id, name);
}
