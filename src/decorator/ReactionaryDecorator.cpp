/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator after the Reactionary ME theme's Windows ME decoration.
 * See ReactionaryDecorator.h for what it looks like.
 *
 * Like Gonx, this derives from Decorator rather than TabDecorator, whose
 * private button layout would move the buttons back to the tab's ends; every
 * change goes through _DoLayout().
 *
 * Sizes are those of the classic look at a 12 pt font: a 4 pixel frame, an
 * 18 pixel title bar with a 1 pixel line under it, and 16 x 14 pixel
 * buttons. They grow with the window title font.
 */


#include "ReactionaryDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <GradientLinear.h>
#include <Rect.h>
#include <View.h>

#include <WindowPrivate.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "PatternHandler.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


// The decoration's colours (windowsME-aurorae/decoration.svg): a C0C0C0
// frame, lit from the top left, and title bar gradients ending in light blue
// (active) or light grey (inactive). The buttons are the colour scheme's
// D4D0C8 button face.
static const rgb_color kButtonFace = { 212, 208, 200, 255 };
static const rgb_color kLight = { 255, 255, 255, 255 };
static const rgb_color kShadow = { 128, 128, 128, 255 };
static const rgb_color kDarkShadow = { 64, 64, 64, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kActiveGradientEnd = { 166, 202, 240, 255 };
static const rgb_color kInactiveGradientEnd = { 192, 192, 192, 255 };

static const float kResizeKnobSize = 18.0;


//! \a color with \a amount (0 to 1) of \a other mixed in.
static rgb_color
mix_color(const rgb_color& color, const rgb_color& other, float amount)
{
	rgb_color mixed = color;
	mixed.red = (uint8)(color.red + (other.red - color.red) * amount);
	mixed.green = (uint8)(color.green + (other.green - color.green) * amount);
	mixed.blue = (uint8)(color.blue + (other.blue - color.blue) * amount);
	return mixed;
}


// Button glyphs, from the decoration's 16 x 14 button bitmaps.
static const char* const kCloseGlyph[] = {
	"................",
	"................",
	"................",
	"....XX....XX....",
	".....XX..XX.....",
	"......XXXX......",
	".......XX.......",
	"......XXXX......",
	".....XX..XX.....",
	"....XX....XX....",
	"................",
	"................",
	"................",
	"................"
};

static const char* const kMinimizeGlyph[] = {
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"................",
	"....XXXXXX......",
	"....XXXXXX......",
	"................",
	"................",
	"................"
};

static const char* const kZoomGlyph[] = {
	"................",
	"................",
	"...XXXXXXXXX....",
	"...XXXXXXXXX....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...X.......X....",
	"...XXXXXXXXX....",
	"................",
	"................",
	"................"
};

static const int32 kGlyphWidth = 16;
static const int32 kGlyphHeight = 14;


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


//	#pragma mark - ReactionaryDecorAddOn


ReactionaryDecorAddOn::ReactionaryDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
ReactionaryDecorAddOn::_AllocateDecorator(DesktopSettings& settings,
	BRect rect, Desktop* desktop)
{
	return new (std::nothrow) ReactionaryDecorator(settings, rect, desktop);
}


//	#pragma mark - ReactionaryDecorator


ReactionaryDecorator::ReactionaryDecorator(DesktopSettings& settings,
	BRect rect, Desktop* desktop)
	:
	Decorator(settings, rect, desktop)
{
	STRACE(("ReactionaryDecorator: frame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
	UpdateColors(settings);
}


ReactionaryDecorator::~ReactionaryDecorator()
{
}


void
ReactionaryDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
ReactionaryDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
ReactionaryDecorator::RegionAt(BPoint where, int32& tabIndex) const
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		if (fTabList.ItemAt(i)->minimizeRect.Contains(where)) {
			tabIndex = i;
			return REGION_MINIMIZE_BUTTON;
		}
	}

	// close, zoom and the title bar
	Region region = Decorator::RegionAt(where, tabIndex);
	if (region != REGION_NONE)
		return region;

	if (_HasResizeGrip() && fResizeRect.Contains(where))
		return REGION_RIGHT_BOTTOM_CORNER;

	if (fBorderWidth <= 0 || !fBorderRect.Contains(where)
		|| fFrame.Contains(where))
		return REGION_NONE;

	const bool onLeft = where.x < fFrame.left;
	const bool onRight = where.x > fFrame.right;
	const bool onTop = where.y < fBorderRect.top + fBorderWidth;
	const bool onBottom = where.y > fFrame.bottom;

	if (!onLeft && !onRight && !onTop && !onBottom) {
		// the line under the title bar moves the window, like the title
		if (!_HasTab())
			return REGION_NONE;
		tabIndex = 0;
		return REGION_TAB;
	}

	// Every edge resizes, and the ends of each edge resize from the corner,
	// as on Windows. The window behaviour ignores this for windows that
	// can't be resized.
	const float corner = _Geometry().cornerLength;
	const bool left = where.x < fBorderRect.left + corner;
	const bool right = where.x > fBorderRect.right - corner;
	const bool top = where.y < fBorderRect.top + corner;
	const bool bottom = where.y > fBorderRect.bottom - corner;

	if (left && top)
		return REGION_LEFT_TOP_CORNER;
	if (right && top)
		return REGION_RIGHT_TOP_CORNER;
	if (left && bottom)
		return REGION_LEFT_BOTTOM_CORNER;
	if (right && bottom)
		return REGION_RIGHT_BOTTOM_CORNER;

	if (onLeft)
		return REGION_LEFT_BORDER;
	if (onRight)
		return REGION_RIGHT_BORDER;
	if (onTop)
		return REGION_TOP_BORDER;
	return REGION_BOTTOM_BORDER;
}


void
ReactionaryDecorator::UpdateColors(DesktopSettings& settings)
{
	fFocusTabColor = settings.UIColor(B_WINDOW_TAB_COLOR);
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTabColor = settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

	fFrameFace = settings.UIColor(B_WINDOW_BORDER_COLOR);
	if (fFrameFace.Brightness() < 128) {
		fFrameLight = mix_color(fFrameFace, kLight, 0.25f);
		fFrameShadow = mix_color(fFrameFace, kBlack, 0.4f);
		fFrameDarkShadow = kBlack;
		fFrameEdge = mix_color(fFrameFace, kLight, 0.12f);
		fInactiveGradientEnd = mix_color(fFrameFace, kLight, 0.3f);
	} else {
		fFrameLight = kLight;
		fFrameShadow = kShadow;
		fFrameDarkShadow = kDarkShadow;
		fFrameEdge = kButtonFace;
		fInactiveGradientEnd = kInactiveGradientEnd;
	}
}


//	#pragma mark - Protected methods


void
ReactionaryDecorator::_DoLayout()
{
	const Geometry geometry = _Geometry();

	float top = 0;
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			fBorderWidth = (int32)geometry.border;
			// the title bar and the line under it
			top = geometry.border + geometry.titleHeight + 1;
			break;

		case B_MODAL_WINDOW_LOOK:
			fBorderWidth = (int32)geometry.border;
			top = fBorderWidth;
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			top = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	if (fBorderWidth > 0) {
		// the left and right borders don't include the corners
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
		const float knob = roundf(kResizeKnobSize
			* std::max(fDrawState.Font().Size() / 12.0f, 1.0f));
		fResizeRect.Set(fBottomBorder.right - knob,
			fBottomBorder.bottom - knob, fBottomBorder.right,
			fBottomBorder.bottom);
	} else
		fResizeRect.Set(0, 0, -1, -1);

	if (_HasTab()) {
		_LayoutTabs(geometry);
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
ReactionaryDecorator::_DoOutlineLayout()
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


/*!	The classic window frame: a light grey line and a white one along the
	top and left, a mid and a dark grey one along the bottom and right, and
	the frame colour inside them, including the line under the title bar.
*/
void
ReactionaryDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		// Menus and tool tips. A menu's raised edge is two pixels: this
		// outer one, and the inner one the control look draws.
		_DrawBevel(fBorderRect, fFrameEdge, fFrameDarkShadow);
		return;
	}

	BRect outer(fBorderRect);
	_DrawBevel(outer, fFrameFace, fFrameDarkShadow);
	outer.InsetBy(1, 1);
	_DrawBevel(outer, fFrameLight, fFrameShadow);
	outer.InsetBy(1, 1);

	BRegion face(outer);
	face.Exclude(fFrame);
	if (fTitleBarRect.IsValid())
		face.Exclude(fTitleBarRect);
	fDrawingEngine->FillRegion(face, fFrameFace);

	if (_HasResizeGrip())
		_DrawResizeGrip(rect);
}


void
ReactionaryDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	A tab is its share of the title bar, shading from left to right. Tabs of
	a stack are kept apart by a line of the frame colour.
*/
void
ReactionaryDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& tabRect = tab->tabRect;
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	const bool focus = IsFocus(tab);
	BRect bar(tabRect);
	if (tab != fTabList.ItemAt(0)) {
		fDrawingEngine->StrokeLine(bar.LeftTop(), bar.LeftBottom(),
			fFrameFace);
		bar.left++;
	}

	BGradientLinear gradient;
	gradient.AddColor(focus ? fFocusTabColor : fNonFocusTabColor, 0);
	gradient.AddColor(focus ? kActiveGradientEnd : fInactiveGradientEnd, 255);
	gradient.SetStart(bar.LeftTop());
	gradient.SetEnd(bar.RightTop());
	fDrawingEngine->FillRect(bar & invalid, gradient);

	_DrawTitle(tab, bar);
	_DrawButtons(tab, invalid);
}


void
ReactionaryDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const Geometry geometry = _Geometry();
	const BRect& tabRect = tab->tabRect;

	// the title stops short of the leftmost button
	float right = tabRect.right - geometry.buttonMargin;
	if (tab->minimizeRect.IsValid())
		right = tab->minimizeRect.left;
	else if (tab->zoomRect.IsValid())
		right = tab->zoomRect.left;
	else if (tab->closeRect.IsValid())
		right = tab->closeRect.left;
	right -= geometry.titleInset;

	const float left = tabRect.left + geometry.titleInset;

	tab->truncatedTitle = Title(tab);
	fDrawState.Font().TruncateString(&tab->truncatedTitle, B_TRUNCATE_END,
		std::max(0.0f, right - left));
	tab->truncatedTitleLength = tab->truncatedTitle.Length();

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint position(left, floorf((tabRect.top + tabRect.bottom + 1
		+ fontHeight.ascent - fontHeight.descent) / 2));

	fDrawingEngine->SetFont(fDrawState.Font());
	fDrawingEngine->SetHighColor(IsFocus(tab)
		? fFocusTextColor : fNonFocusTextColor);
	drawing_mode oldMode;
	fDrawingEngine->SetDrawingMode(B_OP_OVER, oldMode);
	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);
	fDrawingEngine->SetDrawingMode(oldMode);
}


void
ReactionaryDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
}


void
ReactionaryDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, tab->closePressed, kCloseGlyph);
}


void
ReactionaryDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct,
	BRect rect)
{
	_DrawButton(tab, direct, rect, tab->minimizePressed, kMinimizeGlyph);
}


void
ReactionaryDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, tab->zoomPressed, kZoomGlyph);
}


void
ReactionaryDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
	BRegion* updateRegion)
{
	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);

	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);
}


void
ReactionaryDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
ReactionaryDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	// the title bar always spans the window
	return false;
}


bool
ReactionaryDecorator::_SetSettings(const BMessage& settings,
	BRegion* updateRegion)
{
	return false;
}


void
ReactionaryDecorator::_UpdateFont(DesktopSettings& settings)
{
	// bold titles, as in the theme; tool windows get the plain font
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
ReactionaryDecorator::_MoveBy(BPoint offset)
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
ReactionaryDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// The title's gradient and the buttons move with the right edge, so
	// the whole title bar and the edges that moved are redrawn.
	if (dirty != NULL) {
		if (fTitleBarRect.IsValid())
			dirty->Include(fTitleBarRect);
		dirty->Include(fRightBorder);
		dirty->Include(fBottomBorder);
		if (fResizeRect.IsValid())
			dirty->Include(fResizeRect);
	}

	fFrame.right += offset.x;
	fFrame.bottom += offset.y;
	_DoLayout();

	if (dirty != NULL) {
		if (fTitleBarRect.IsValid())
			dirty->Include(fTitleBarRect);
		dirty->Include(fTopBorder);
		dirty->Include(fRightBorder);
		dirty->Include(fBottomBorder);
		if (fResizeRect.IsValid())
			dirty->Include(fResizeRect);
	}
}


bool
ReactionaryDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);

	_UpdateFont(settings);
	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
ReactionaryDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);

	_DoLayout();
	_DoOutlineLayout();

	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
ReactionaryDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);

	_DoLayout();

	if (updateRegion != NULL && fTitleBarRect.IsValid())
		updateRegion->Include(fTitleBarRect);
	return true;
}


void
ReactionaryDecorator::_GetFootprint(BRegion* region)
{
	if (region == NULL || fTopTab->look == B_NO_BORDER_WINDOW_LOOK
		|| fBorderWidth <= 0)
		return;

	region->Include(fBorderRect);
	region->Exclude(fFrame);

	// the size grip covers the content's bottom right corner
	if (_HasResizeGrip())
		region->Include(fResizeRect);
}


//	#pragma mark - Private methods


bool
ReactionaryDecorator::_HasTab() const
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


bool
ReactionaryDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


bool
ReactionaryDecorator::_HasResizeGrip() const
{
	return fTopTab != NULL && fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0 && fResizeRect.IsValid();
}


ReactionaryDecorator::Geometry
ReactionaryDecorator::_Geometry() const
{
	const bool small = _IsSmall();
	const float scale = std::max(fDrawState.Font().Size() / 12.0f, 1.0f);

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	const float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	Geometry geometry;
	geometry.border = 2 + roundf(2 * scale);
	// 18 pixels, or 15 for tool windows, as on Windows, and more when the
	// font needs it
	geometry.titleHeight = std::max(small ? 15.0f : 18.0f,
		textHeight + (small ? 2 : 4));
	geometry.buttonHeight = geometry.titleHeight - 4;
	geometry.buttonWidth = geometry.buttonHeight + 2;
	geometry.buttonMargin = 2;
	geometry.closeGap = 2;
	geometry.titleInset = roundf(3 * scale);
	geometry.cornerLength = roundf(16 * scale);
	return geometry;
}


/*!	The title bar spans the window, between the frame's sides; tabs of a
	stack share it equally. Each tab has its buttons at its right end:
	minimize and zoom side by side, then a gap and close.
*/
void
ReactionaryDecorator::_LayoutTabs(const Geometry& geometry)
{
	const int32 count = fTabList.CountItems();
	if (count == 0) {
		fTitleBarRect.Set(0, 0, -1, -1);
		return;
	}

	fTitleBarRect.Set(fFrame.left, fFrame.top - 1 - geometry.titleHeight,
		fFrame.right, fFrame.top - 2);

	const float width = fTitleBarRect.Width() + 1;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		tab->tabRect = fTitleBarRect;
		tab->tabRect.left = fTitleBarRect.left + floorf(width * i / count);
		tab->tabRect.right = fTitleBarRect.left
			+ floorf(width * (i + 1) / count) - 1;
		tab->tabOffset = (uint32)(tab->tabRect.left - fTitleBarRect.left);
		tab->tabLocation = 0;

		BRect button(0, 0, geometry.buttonWidth - 1,
			geometry.buttonHeight - 1);
		button.OffsetTo(tab->tabRect.right - geometry.buttonMargin
			- geometry.buttonWidth + 1,
			tab->tabRect.top + floorf((geometry.titleHeight
				- geometry.buttonHeight) / 2));

		if ((tab->flags & B_NOT_CLOSABLE) == 0) {
			tab->closeRect = button;
			button.OffsetBy(-geometry.buttonWidth - geometry.closeGap, 0);
		} else
			tab->closeRect.Set(0, 0, -1, -1);

		if ((tab->flags & B_NOT_ZOOMABLE) == 0) {
			tab->zoomRect = button;
			button.OffsetBy(-geometry.buttonWidth, 0);
		} else
			tab->zoomRect.Set(0, 0, -1, -1);

		if ((tab->flags & B_NOT_MINIMIZABLE) == 0)
			tab->minimizeRect = button;
		else
			tab->minimizeRect.Set(0, 0, -1, -1);

		// buttons that don't fit are left out
		const float minimum = tab->tabRect.left + geometry.titleInset;
		if (tab->minimizeRect.IsValid() && tab->minimizeRect.left < minimum)
			tab->minimizeRect.Set(0, 0, -1, -1);
		if (tab->zoomRect.IsValid() && tab->zoomRect.left < minimum)
			tab->zoomRect.Set(0, 0, -1, -1);
		if (tab->closeRect.IsValid() && tab->closeRect.left < minimum)
			tab->closeRect.Set(0, 0, -1, -1);
	}
}


/*!	One pixel wide frame round \a rect: \a topLeft along the top and left,
	\a bottomRight along the bottom and right, which own the corners they
	share.
*/
void
ReactionaryDecorator::_DrawBevel(BRect rect, rgb_color topLeft,
	rgb_color bottomRight)
{
	fDrawingEngine->StrokeLine(rect.LeftTop(),
		BPoint(rect.right - 1, rect.top), topLeft);
	fDrawingEngine->StrokeLine(rect.LeftTop(),
		BPoint(rect.left, rect.bottom - 1), topLeft);
	fDrawingEngine->StrokeLine(rect.RightTop(), rect.RightBottom(),
		bottomRight);
	fDrawingEngine->StrokeLine(rect.LeftBottom(), rect.RightBottom(),
		bottomRight);
}


/*!	A title bar button: the classic raised bevel on the button face, which
	turns over when pressed, moving the glyph one pixel down and right. The
	glyph is a 16 x 14 bitmap, drawn at whole multiples for large fonts.
*/
void
ReactionaryDecorator::_DrawButton(Decorator::Tab* tab, bool direct,
	BRect rect, bool pressed, const char* const* glyph)
{
	if (!rect.IsValid())
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	BRect bevel(rect);
	if (pressed) {
		_DrawBevel(bevel, kDarkShadow, kLight);
		bevel.InsetBy(1, 1);
		_DrawBevel(bevel, kShadow, kButtonFace);
	} else {
		_DrawBevel(bevel, kLight, kDarkShadow);
		bevel.InsetBy(1, 1);
		_DrawBevel(bevel, kButtonFace, kShadow);
	}
	bevel.InsetBy(1, 1);
	fDrawingEngine->FillRect(bevel, kButtonFace);

	const float width = rect.Width() + 1;
	const float height = rect.Height() + 1;
	const float pixel = std::max(1.0f, floorf(std::min(width / kGlyphWidth,
		height / kGlyphHeight)));
	BPoint origin(
		rect.left + floorf((width - kGlyphWidth * pixel) / 2),
		rect.top + floorf((height - kGlyphHeight * pixel) / 2));
	if (pressed)
		origin += BPoint(1, 1);

	for (int32 y = 0; y < kGlyphHeight; y++) {
		for (int32 x = 0; x < kGlyphWidth; x++) {
			if (glyph[y][x] != 'X')
				continue;
			const float left = origin.x + x * pixel;
			const float top = origin.y + y * pixel;
			BRect dot(left, top, left + pixel - 1, top + pixel - 1);
			if (bevel.Contains(dot))
				fDrawingEngine->FillRect(dot, kBlack);
		}
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


/*!	The size grip of a document window, in the content's bottom right
	corner where the scroll bars meet: three diagonal ridges, each a white
	line with two mid grey ones below it, on the button face.
*/
void
ReactionaryDecorator::_DrawResizeGrip(BRect invalid)
{
	BRect grip = fResizeRect & fFrame;
	if (!grip.IsValid() || !invalid.Intersects(grip))
		return;

	fDrawingEngine->FillRect(grip, fFrameEdge);

	const int32 size = (int32)std::min(grip.Width(), grip.Height()) + 1;
	for (int32 dy = 0; dy < size; dy++) {
		for (int32 dx = 0; dx < size; dx++) {
			// distance from the corner along the diagonal
			const int32 step = dx + dy;
			if (dx == 0 || dy == 0 || step < 3)
				continue;
			const int32 phase = (step - 3) % 4;
			if (step - 3 >= 12 || phase == 3)
				continue;
			const BPoint point(grip.right - dx + 1, grip.bottom - dy + 1);
			if (!grip.Contains(point))
				continue;
			// counted from the corner out: grey, grey, white, a gap
			fDrawingEngine->StrokePoint(point, phase == 2 ? fFrameLight : fFrameShadow);
		}
	}
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) ReactionaryDecorAddOn(id, name);
}
