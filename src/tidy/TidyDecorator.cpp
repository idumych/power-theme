/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See TidyDecorator.h. Colours and sizes were measured from the mockups.
 */


#include "TidyDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Region.h>
#include <WindowPrivate.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"


// how far each of a corner's first rows is cut in; TidySquareDecorator.cpp cuts
// none of them
static const int32 kCorner[] = { 3, 2, 1 };
#ifdef TIDY_SQUARE_CORNERS
static const int32 kCornerRows = 0;
#else
static const int32 kCornerRows = sizeof(kCorner) / sizeof(kCorner[0]);
#endif

static const float kResizeCornerLength = 14;
static const float kBorderResizeLength = 22;

// A titled window's top: the outline, and four rows that make its menu bar as
// tall as the mockups', which BMenuBar's own height isn't. TidyControlLook moves
// the bar's text up to stay in the middle of them.
static const float kThinTop = 5;

// the room in a lone tab on each side of its title, from the tab's end to the
// title: the button's offset and width, and a gap
static const float kTabSideRoom = 8 + 15 + 28;


static rgb_color
mix(rgb_color a, rgb_color b, float amount)
{
	rgb_color result;
	result.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	result.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	result.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	result.alpha = 255;
	return result;
}


static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kWhite = { 255, 255, 255, 255 };


static bool
is_dark(rgb_color color)
{
	return color.Brightness() < 128;
}


//	#pragma mark - TidyDecorAddOn


TidyDecorAddOn::TidyDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
TidyDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) TidyDecorator(settings, rect, desktop);
}


//	#pragma mark - TidyDecorator


TidyDecorator::TidyDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
	:
	TabDecorator(settings, rect, desktop)
{
	_UpdateMenuBarHeight(settings);
}


TidyDecorator::~TidyDecorator()
{
}


void
TidyDecorator::_DoLayout()
{
	bool hasTab = false;
	float border = 0;

	switch ((int)fTopTab->look) {
		case B_MODAL_WINDOW_LOOK:
			border = 5;
			break;

		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			hasTab = true;
			border = 5;
			break;

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			hasTab = true;
			border = 3;
			break;

		case B_BORDERED_WINDOW_LOOK:
			border = 1;
			break;
	}

	const float scale = _Scale();
	fBorderWidth = border > 1 ? int32(border * scale) : int32(border);
	fBorderResizeLength = kBorderResizeLength * scale;
	// TabDecorator's footprint gives document windows the content's corner
	// over fResizeKnobSize - fBorderWidth (+ 1) pixels
	fResizeKnobSize = roundf(kResizeCornerLength * scale) + fBorderWidth;

	if (fBorderWidth > 0) {
		// the left and right borders don't include the corners
		fLeftBorder.Set(fFrame.left - fBorderWidth, fFrame.top,
			fFrame.left - 1, fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top,
			fFrame.right + fBorderWidth, fFrame.bottom);
		const float top = _HasThinTop() ? kThinTop : fBorderWidth;
		fTopBorder.Set(fFrame.left - fBorderWidth, fFrame.top - top,
			fFrame.right + fBorderWidth, fFrame.top - 1);
		fBottomBorder.Set(fFrame.left - fBorderWidth, fFrame.bottom + 1,
			fFrame.right + fBorderWidth, fFrame.bottom + fBorderWidth);
	} else {
		fLeftBorder.Set(0.0, 0.0, -1.0, -1.0);
		fRightBorder.Set(0.0, 0.0, -1.0, -1.0);
		fTopBorder.Set(0.0, 0.0, -1.0, -1.0);
		fBottomBorder.Set(0.0, 0.0, -1.0, -1.0);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	if (fBorderWidth > 1) {
		fResizeRect.Set(fBottomBorder.right - fResizeKnobSize,
			fBottomBorder.bottom - fResizeKnobSize, fBottomBorder.right,
			fBottomBorder.bottom);
	} else
		fResizeRect.Set(0, 0, -1, -1);

	if (hasTab) {
		_DoTabLayout();
		return;
	}

	for (int32 i = 0; i < fTabList.CountItems(); i++)
		fTabList.ItemAt(i)->tabRect.Set(0.0, 0.0, -1.0, -1.0);
	fTabsRegion.MakeEmpty();
	fTitleBarRect.Set(0.0, 0.0, -1.0, -1.0);
}


/*!	TabDecorator lays the tabs out, sliding and stacking them; a titled window's
	tab is then made Tidy's height and set on the thin top. Later layouts by
	TabDecorator only move tabs sideways, or place the buttons from the tab's top,
	so the height stays.
*/
void
TidyDecorator::_DoTabLayout()
{
	TabDecorator::_DoTabLayout();

	if (!_HasThinTop())
		return;

	const float bottom = fTopBorder.top - 1;
	const float top = bottom - _TabHeight() + 1;

	fTitleBarRect.Set(0.0, 0.0, -1.0, -1.0);
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float delta = top - tab->tabRect.top;
		tab->tabRect.top = top;
		tab->tabRect.bottom = bottom;
		tab->closeRect.OffsetBy(0, delta);
		tab->zoomRect.OffsetBy(0, delta);

		// a lone tab has room to spare, with its title in the middle
		if (fTabList.CountItems() == 1) {
			const float wanted = tab->tabRect.left
				+ roundf(2 * kTabSideRoom * _Scale())
				+ ceilf(fDrawingEngine->StringWidth(Title(tab),
					strlen(Title(tab)), fDrawState.Font()));
			const float right = std::min(fRightBorder.right, wanted);
			tab->zoomRect.OffsetBy(right - tab->tabRect.right, 0);
			tab->tabRect.right = right;
		}

		fTitleBarRect = fTitleBarRect.IsValid()
			? fTitleBarRect | tab->tabRect : tab->tabRect;
	}
	_CalculateTabsRegion();
}


void
TidyDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size, float* _inset) const
{
	if (fTopTab->look != B_TITLED_WINDOW_LOOK
		&& fTopTab->look != B_DOCUMENT_WINDOW_LOOK) {
		TabDecorator::_GetButtonSizeAndOffset(tabRect, _offset, _size, _inset);
		return;
	}

	// TabDecorator's rects are this plus one wide
	const float size = roundf(15 * _Scale()) - 1;
	*_size = size;
	*_offset = floorf((_TabHeight() - size - 1) / 2);
	*_inset = floorf(fDrawState.Font().Size() / 6.0);
}


void
TidyDecorator::_GetFootprint(BRegion* region)
{
	TabDecorator::_GetFootprint(region);

	if (region == NULL || fBorderWidth < 3)
		return;

	const BRect& r = fBorderRect;
	_ExcludeCorner(region, r.left, r.bottom, 1, -1);
	_ExcludeCorner(region, r.right, r.bottom, -1, -1);
	if (_TopLeftIsRound())
		_ExcludeCorner(region, r.left, r.top, 1, 1);
	if (_TopRightIsRound())
		_ExcludeCorner(region, r.right, r.top, -1, 1);

	if (fTopTab->look == kLeftTitledWindowLook)
		return;

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		const BRect& tabRect = fTabList.ItemAt(i)->tabRect;
		if (!tabRect.IsValid())
			continue;
		_ExcludeCorner(region, tabRect.left, tabRect.top, 1, 1);
		_ExcludeCorner(region, tabRect.right, tabRect.top, -1, 1);
	}
}


void
TidyDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const bool focus = _TabFocus(fTopTab);
	const rgb_color outline = _Outline(focus);
	const BRect& r = fBorderRect;

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		fDrawingEngine->StrokeRect(r, outline);
		return;
	}

	const rgb_color face = focus ? fFocusFrameColor : fNonFocusFrameColor;

	// the sides and the bottom, and the top with a highlight along it
	if (fTopBorder.Height() > 0) {
		fDrawingEngine->FillRect(BRect(r.left + 1, r.top + 1, r.right - 1,
			fFrame.top - 1), face);
		if (_HasThinTop()) {
			fDrawingEngine->StrokeLine(BPoint(r.left + 1, r.top + 1),
				BPoint(r.right - 1, r.top + 1),
				mix(face, kWhite, is_dark(face) ? 0.3f : 0.4f));
		}
	}
	fDrawingEngine->FillRect(BRect(r.left + 1, fFrame.top, fFrame.left - 1,
		fFrame.bottom), face);
	fDrawingEngine->FillRect(BRect(fFrame.right + 1, fFrame.top, r.right - 1,
		fFrame.bottom), face);
	fDrawingEngine->FillRect(BRect(r.left + 1, fFrame.bottom + 1, r.right - 1,
		r.bottom - 1), face);

	// Document windows nearly always have a menu bar, and the sides carry on its
	// shading and its bottom line, so that it runs from edge to edge as in the
	// mockups. Other windows stay flat, as they are there.
	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK) {
		const float line = std::min(fFrame.bottom,
			fFrame.top + fMenuBarHeight);
		const bool dark = is_dark(face);
		const rgb_color top = dark ? mix(face, kWhite, 0.2f) : face;
		const rgb_color end = dark ? mix(face, kWhite, 0.06f)
			: mix(face, kBlack, 0.08f);
		BGradientLinear gradient;
		gradient.AddColor(top, 0);
		gradient.AddColor(end, 255);
		gradient.SetStart(BPoint(r.left, fFrame.top));
		gradient.SetEnd(BPoint(r.left, line - 1));
		fDrawingEngine->FillRect(BRect(r.left + 1, fFrame.top, fFrame.left - 1,
			line - 1), gradient);
		fDrawingEngine->FillRect(BRect(fFrame.right + 1, fFrame.top,
			r.right - 1, line - 1), gradient);

		const rgb_color edge = mix(face, kBlack, dark ? 0.4f : 0.2f);
		fDrawingEngine->StrokeLine(BPoint(r.left + 1, line),
			BPoint(fFrame.left - 1, line), edge);
		fDrawingEngine->StrokeLine(BPoint(fFrame.right + 1, line),
			BPoint(r.right - 1, line), edge);
	}

	// the outline; the cut corners clip its ends
	fDrawingEngine->StrokeLine(r.LeftTop(), r.RightTop(), outline);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), outline);
	fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(), outline);
	fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), outline);

	if (fBorderWidth >= 3) {
		_StrokeCorner(r.left, r.bottom, 1, -1, outline);
		_StrokeCorner(r.right, r.bottom, -1, -1, outline);
		if (_TopLeftIsRound())
			_StrokeCorner(r.left, r.top, 1, 1, outline);
		if (_TopRightIsRound())
			_StrokeCorner(r.right, r.top, -1, 1, outline);
	}

	_DrawResizeGrip(rect);
}


void
TidyDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& tabRect = tab->tabRect;
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	_DrawTabBody(tab);
	_DrawTitle(tab, tabRect);
	_DrawButtons(tab, invalid);
}


void
TidyDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(_TabFocus(tab)
		? fFocusTextColor : fNonFocusTextColor);
	fDrawingEngine->SetFont(fDrawState.Font());

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint position;
	if (tab->look != kLeftTitledWindowLook) {
		// in the middle of what is between the buttons
		const float width = fDrawingEngine->StringWidth(tab->truncatedTitle,
			tab->truncatedTitleLength, fDrawState.Font());
		const float left = tab->closeRect.IsValid()
			? tab->closeRect.right + 1 : tabRect.left;
		// (an inactive tab in a stack has an empty one at the origin)
		const float right = tab->zoomRect.Width() >= 1
			? tab->zoomRect.left : tabRect.right;
		position.x = floorf((left + right - width) / 2);
		position.y = floorf((tabRect.top + tabRect.bottom + 1 + fontHeight.ascent
			- fontHeight.descent) / 2);
	} else {
		position.x = floorf((tabRect.left + tabRect.right + 1
			+ fontHeight.ascent - fontHeight.descent) / 2);
		position.y = tab->zoomRect.IsValid()
			? tab->zoomRect.top - tab->textOffset
			: tabRect.bottom - tab->textOffset;
	}

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);
	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


/*!	A shaded box, softened at its corners, with a highlight under it. */
void
TidyDecorator::_DrawButtonBox(BRect box, const rgb_color& base, bool pressed)
{
	const rgb_color line = mix(base, kBlack, 0.3f);

	BGradientLinear gradient;
	gradient.AddColor(mix(base, kWhite, pressed ? 0.1f : 0.35f), 0);
	gradient.AddColor(mix(base, kBlack, pressed ? 0.12f : 0.04f), 255);
	gradient.SetStart(box.LeftTop());
	gradient.SetEnd(box.LeftBottom());
	fDrawingEngine->FillRect(box.InsetByCopy(1, 1), gradient);

	fDrawingEngine->StrokeLine(BPoint(box.left + 1, box.top + 1),
		BPoint(box.right - 1, box.top + 1),
		mix(base, kWhite, pressed ? 0.25f : 0.7f));

	fDrawingEngine->StrokeLine(BPoint(box.left + 1, box.top),
		BPoint(box.right - 1, box.top), line);
	fDrawingEngine->StrokeLine(BPoint(box.left + 1, box.bottom),
		BPoint(box.right - 1, box.bottom), line);
	fDrawingEngine->StrokeLine(BPoint(box.left, box.top + 1),
		BPoint(box.left, box.bottom - 1), line);
	fDrawingEngine->StrokeLine(BPoint(box.right, box.top + 1),
		BPoint(box.right, box.bottom - 1), line);

	const rgb_color corner = mix(base, kBlack, 0.12f);
	fDrawingEngine->FillRect(BRect(box.left, box.top, box.left, box.top), corner);
	fDrawingEngine->FillRect(BRect(box.right, box.top, box.right, box.top),
		corner);
	fDrawingEngine->FillRect(BRect(box.left, box.bottom, box.left, box.bottom),
		corner);
	fDrawingEngine->FillRect(BRect(box.right, box.bottom, box.right, box.bottom),
		corner);

	fDrawingEngine->StrokeLine(BPoint(box.left + 1, box.bottom + 1),
		BPoint(box.right - 1, box.bottom + 1), mix(base, kWhite, 0.35f));
}


void
TidyDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const rgb_color base = _TabFocus(tab) ? fFocusTabColor : fNonFocusTabColor;
	_FillTabShading(tab, rect);
	_DrawButtonBox(rect, base, tab->closePressed);

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


void
TidyDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const rgb_color base = _TabFocus(tab) ? fFocusTabColor : fNonFocusTabColor;
	_FillTabShading(tab, rect);

	// a small box in front of a larger one's top left corner
	const float scale = rect.Width() / 14;
	const float large = roundf(12 * scale);
	const float small = roundf(9 * scale);
	_DrawButtonBox(BRect(rect.right - large + 1, rect.bottom - large + 1,
		rect.right, rect.bottom), base, tab->zoomPressed);
	_DrawButtonBox(BRect(rect.left, rect.top, rect.left + small - 1,
		rect.top + small - 1), base, tab->zoomPressed);

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


void
TidyDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	// BeOS-style tabs have no minimize button
}


//	#pragma mark - private


void
TidyDecorator::_FontsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	_UpdateMenuBarHeight(settings);
	TabDecorator::_FontsChanged(settings, updateRegion);
}


/*!	How far a menu bar's bottom line is from the top of the window, worked out
	as BMenuBar does: the menu font's height and margins of a sixth of the plain
	font's size, above and below. A menu bar's own font isn't the window title's.
*/
void
TidyDecorator::_UpdateMenuBarHeight(const DesktopSettings& settings)
{
	ServerFont plain;
	settings.GetDefaultPlainFont(plain);
	menu_info info;
	settings.GetMenuInfo(info);

	ServerFont font(plain);
	font.SetSize(info.font_size);
	font_height height;
	font.GetHeight(height);
	fMenuBarHeight = ceilf(height.ascent + height.descent
		+ plain.Size() / 3.0f);
}


bool
TidyDecorator::_HasThinTop() const
{
	return fTopTab->look == B_TITLED_WINDOW_LOOK
		|| fTopTab->look == B_DOCUMENT_WINDOW_LOOK;
}


float
TidyDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / 12.0f, 1.0f);
}


float
TidyDecorator::_TabHeight() const
{
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	return ceilf(fontHeight.ascent + fontHeight.descent) + roundf(14 * _Scale());
}


bool
TidyDecorator::_TabFocus(Decorator::Tab* tab) const
{
	return tab != NULL && (tab->buttonFocus || IsFocus(tab));
}


rgb_color
TidyDecorator::_Outline(bool focus) const
{
	const rgb_color face = focus ? fFocusFrameColor : fNonFocusFrameColor;
	return mix(face, kBlack, focus ? 0.8f : 0.55f);
}


void
TidyDecorator::_ExcludeCorner(BRegion* region, float x, float y,
	int32 xDirection, int32 yDirection) const
{
	for (int32 row = 0; row < kCornerRows; row++) {
		const float rowY = y + row * yDirection;
		const float end = x + (kCorner[row] - 1) * xDirection;
		region->Exclude(BRect(std::min(x, end), rowY, std::max(x, end), rowY));
	}
}


void
TidyDecorator::_StrokeCorner(float x, float y, int32 xDirection,
	int32 yDirection, const rgb_color& color)
{
	for (int32 row = 0; row < kCornerRows; row++) {
		const float rowX = x + kCorner[row] * xDirection;
		const float rowY = y + row * yDirection;
		fDrawingEngine->FillRect(BRect(rowX, rowY, rowX, rowY), color);
	}
}


/*!	A window's top corner is square where a tab meets it. */
bool
TidyDecorator::_TopLeftIsRound() const
{
	if (fTopTab->look == kLeftTitledWindowLook)
		return false;
	return !fTitleBarRect.IsValid() || fTitleBarRect.left > fBorderRect.left;
}


bool
TidyDecorator::_TopRightIsRound() const
{
	return !fTitleBarRect.IsValid() || fTitleBarRect.right < fBorderRect.right;
}


/*!	The tab's shading: light at the top, the tab colour at the bottom, with a
	highlight along each end.
*/
void
TidyDecorator::_FillTabShading(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;
	const rgb_color base = _TabFocus(tab) ? fFocusTabColor : fNonFocusTabColor;

	BGradientLinear gradient;
	gradient.AddColor(mix(base, kWhite, 0.45f), 0);
	gradient.AddColor(base, 255);
	if (tab->look == kLeftTitledWindowLook) {
		gradient.SetStart(BPoint(tabRect.left + 1, tabRect.top));
		gradient.SetEnd(BPoint(tabRect.right, tabRect.top));
	} else {
		gradient.SetStart(BPoint(tabRect.left, tabRect.top + 2));
		gradient.SetEnd(BPoint(tabRect.left, tabRect.bottom - 1));
	}

	rect = rect & BRect(tabRect.left + 1, tabRect.top + 1, tabRect.right - 1,
		tabRect.bottom);
	if (rect.IsValid())
		fDrawingEngine->FillRect(rect, gradient);
}


void
TidyDecorator::_DrawTabBody(Decorator::Tab* tab)
{
	const BRect& t = tab->tabRect;
	const bool focus = _TabFocus(tab);
	const rgb_color outline = _Outline(focus);
	const rgb_color base = focus ? fFocusTabColor : fNonFocusTabColor;

	_FillTabShading(tab, t);

	if (tab->look == kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(t.LeftTop(), t.RightTop(), outline);
		fDrawingEngine->StrokeLine(t.LeftTop(), t.LeftBottom(), outline);
		fDrawingEngine->StrokeLine(t.LeftBottom(), t.RightBottom(), outline);
		return;
	}

	// highlights just inside its edges, brightest along the top
	fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 1),
		BPoint(t.left + 1, t.bottom), mix(base, kWhite, 0.45f));
	fDrawingEngine->StrokeLine(BPoint(t.right - 1, t.top + 1),
		BPoint(t.right - 1, t.bottom), mix(base, kWhite, 0.45f));
	fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 1),
		BPoint(t.right - 1, t.top + 1), mix(base, kWhite, 0.7f));
	if (_HasThinTop()) {
		fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.bottom),
			BPoint(t.right - 1, t.bottom), mix(base, kWhite, 0.35f));
	}

	fDrawingEngine->StrokeLine(t.LeftTop(), t.RightTop(), outline);
	fDrawingEngine->StrokeLine(t.LeftTop(), t.LeftBottom(), outline);
	fDrawingEngine->StrokeLine(t.RightTop(), t.RightBottom(), outline);
	_StrokeCorner(t.left, t.top, 1, 1, outline);
	_StrokeCorner(t.right, t.top, -1, 1, outline);
}


/*!	Six soft dots in a triangle, on what would be the content's corner. Each is
	three pixels square, darkest in the middle and lightest at its corners.
*/
void
TidyDecorator::_DrawResizeGrip(BRect invalid)
{
	if (fTopTab->look != B_DOCUMENT_WINDOW_LOOK
		|| (fTopTab->flags & B_NOT_RESIZABLE) != 0) {
		return;
	}

	const float length = fResizeKnobSize - fBorderWidth;
	const BRect corner(fFrame.right - length, fFrame.bottom - length,
		fFrame.right, fFrame.bottom);
	if (!invalid.Intersects(corner))
		return;

	const rgb_color face = _TabFocus(fTopTab) ? fFocusFrameColor
		: fNonFocusFrameColor;
	const rgb_color target = is_dark(face) ? kWhite : kBlack;
	const rgb_color edge = mix(face, target, 0.06f);
	const rgb_color side = mix(face, target, 0.12f);
	const rgb_color middle = mix(face, target, 0.24f);

	fDrawingEngine->FillRect(corner, face);

	static const int32 kDots[][2] = { { 0, 2 }, { 1, 1 }, { 0, 1 }, { 2, 0 },
		{ 1, 0 }, { 0, 0 } };
	for (size_t i = 0; i < sizeof(kDots) / sizeof(kDots[0]); i++) {
		const float x = fFrame.right - 3 - kDots[i][0] * 5;
		const float y = fFrame.bottom - 3 - kDots[i][1] * 5;
		fDrawingEngine->FillRect(BRect(x, y, x + 2, y + 2), edge);
		fDrawingEngine->FillRect(BRect(x + 1, y, x + 1, y + 2), side);
		fDrawingEngine->FillRect(BRect(x, y + 1, x + 2, y + 1), side);
		fDrawingEngine->FillRect(BRect(x + 1, y + 1, x + 1, y + 1), middle);
	}
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) TidyDecorAddOn(id, name);
}
