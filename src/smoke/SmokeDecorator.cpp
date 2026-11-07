/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of ZETA's Smoke decorator. See
 * SmokeDecorator.h for what it looks like.
 *
 * The sizes and colours were measured on ZETA 1.21 screenshots, with the
 * "smoke" Theme Manager theme (window tab colour 656565, inactive 9D9D9D):
 *
 *   - the frame is four pixels: a nearly black line, a light one (the tab
 *     colour + 40), the tab colour, and a nearly black one; along the top,
 *     beside the tab, the third line is the tab's lightest shade
 *   - the tab is the text height + 7 pixels, of which the bottom three
 *     cover the frame's top: it has a black edge and a light one inside it
 *     (and a shadow on its left), and shades down from the tab colour to
 *     the tab colour + 27
 *   - a 6 x 6 white cross 9 pixels in from the tab's left end; a 9 x 8 pair
 *     of white squares, with a lighter shading, 5 pixels in from its right
 *   - the last 14 pixels of the bottom and right frame, round the corner,
 *     are lighter (the tab colour + 60); a document window has a grip there
 *     with six dots in a triangle
 *
 * Pressed buttons weren't captured: here the glyph moves down and right by
 * a pixel and takes its shading colour.
 */


#include "SmokeDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <GradientLinear.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>

#include <WindowPrivate.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


static const rgb_color kGlyphColor = { 245, 248, 248, 255 };

// at a 12 pt font
static const float kResizeCornerLength = 13.0;
static const float kBorderResizeLength = 22.0;

// 'X' the glyph colour, 's' its shading
static const char* const kCloseGlyph[] = {
	"X....X",
	".X..X.",
	"..XX..",
	"..XX..",
	".X..X.",
	"X....X"
};
static const int32 kCloseWidth = 6;
static const int32 kCloseHeight = 6;

static const char* const kZoomGlyph[] = {
	"XXXXXX...",
	"XssssX...",
	"X.XXXXXX.",
	"X.X..XsXs",
	"XXXXXXsXs",
	".sXsss.Xs",
	"..XXXXXXs",
	"...ssssss"
};
static const int32 kZoomWidth = 9;
static const int32 kZoomHeight = 8;


/*!	\a color lightened or darkened by \a delta. Lightening adds to each
	channel; darkening scales the colour down by as much as \a delta takes
	from its brightness, so a grey loses \a delta from each channel (as
	measured on Smoke) and a bright colour such as orange darkens to a dark
	shade of itself rather than to a saturated red.
*/
static rgb_color
shift(rgb_color color, int32 delta)
{
	if (delta < 0) {
		const int32 brightness = (color.red * 299 + color.green * 587
			+ color.blue * 114) / 1000;
		const float factor = brightness > 0
			? std::max(0.0f, (float)(brightness + delta) / brightness) : 0.0f;
		rgb_color result;
		result.red = (uint8)roundf(color.red * factor);
		result.green = (uint8)roundf(color.green * factor);
		result.blue = (uint8)roundf(color.blue * factor);
		result.alpha = 255;
		return result;
	}

	rgb_color result;
	result.red = (uint8)std::clamp((int32)color.red + delta, (int32)0,
		(int32)255);
	result.green = (uint8)std::clamp((int32)color.green + delta, (int32)0,
		(int32)255);
	result.blue = (uint8)std::clamp((int32)color.blue + delta, (int32)0,
		(int32)255);
	result.alpha = 255;
	return result;
}


static rgb_color
mix(rgb_color a, rgb_color b)
{
	rgb_color result;
	result.red = (uint8)(((int32)a.red + b.red) / 2);
	result.green = (uint8)(((int32)a.green + b.green) / 2);
	result.blue = (uint8)(((int32)a.blue + b.blue) / 2);
	result.alpha = 255;
	return result;
}


//	#pragma mark - SmokeDecorAddOn


SmokeDecorAddOn::SmokeDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
SmokeDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) SmokeDecorator(settings, rect, desktop);
}


//	#pragma mark - SmokeDecorator


SmokeDecorator::SmokeDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
	:
	TabDecorator(settings, rect, desktop)
{
	STRACE(("SmokeDecorator: frame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


SmokeDecorator::~SmokeDecorator()
{
}


//	#pragma mark - Protected methods


void
SmokeDecorator::_DoLayout()
{
	bool hasTab = false;
	float border = 0;

	switch ((int)fTopTab->look) {
		case B_MODAL_WINDOW_LOOK:
			border = 4;
			break;

		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			hasTab = true;
			border = 4;
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
		fTopBorder.Set(fFrame.left - fBorderWidth, fFrame.top - fBorderWidth,
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


/*!	TabDecorator lays the tabs out, sliding and stacking them; a titled
	window's tab is then made the height Smoke's is, and its buttons moved
	with its top. Later layouts by TabDecorator only move tabs sideways, or
	place the buttons from the tab's top, so the height stays.
*/
void
SmokeDecorator::_DoTabLayout()
{
	TabDecorator::_DoTabLayout();

	if (!_HasTab() || fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook)
		return;

	const float height = _TextHeight() + roundf(7 * _Scale());
	const float top = _SkirtBottom() - height + 1;

	fTitleBarRect.Set(0.0, 0.0, -1.0, -1.0);
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float delta = top - tab->tabRect.top;
		tab->tabRect.top = top;
		tab->closeRect.OffsetBy(0, delta);
		tab->zoomRect.OffsetBy(0, delta);
		fTitleBarRect = fTitleBarRect.IsValid()
			? fTitleBarRect | tab->tabRect : tab->tabRect;
	}
	_CalculateTabsRegion();
}


void
SmokeDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const Palette palette = _Palette(_TabFocus(fTopTab));

	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			_StrokeFrame(fBorderRect, palette, fBorderWidth);
			if (_HasTab() && fTopTab->look != B_FLOATING_WINDOW_LOOK
				&& fTopTab->look != kLeftTitledWindowLook) {
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					_DrawTabBody(fTabList.ItemAt(i), true);
			}
			_DrawResizeCorner(rect);
#ifdef SMOKE_ROUNDED_CORNERS
			_StrokeRoundedFrame();
#endif
			break;

		case B_BORDERED_WINDOW_LOOK:
			fDrawingEngine->StrokeRect(fBorderRect, palette.outer);
			break;
	}
}


void
SmokeDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& tabRect = tab->tabRect;
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	_DrawTabBody(tab, false);
#ifdef SMOKE_ROUNDED_CORNERS
	_StrokeRoundedTab(tab);
#endif
	_DrawTitle(tab, tabRect);
	_DrawButtons(tab, invalid);
}


void
SmokeDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
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
		// a little closer to the cross than TabDecorator's text offset
		const float gap = std::max(4.0f, tab->textOffset - 4 * _Scale());
		position.x = tab->closeRect.IsValid()
			? tab->closeRect.right + gap : tabRect.left + tab->textOffset;
		const float bottom = tab->look == B_FLOATING_WINDOW_LOOK
			? tabRect.bottom : _SkirtBottom();
		position.y = floorf((tabRect.top + 2 + bottom + 1 + fontHeight.ascent
			- fontHeight.descent) / 2);
	} else {
		position.x = floorf((tabRect.left + 2 + tabRect.right + 1
			+ fontHeight.ascent - fontHeight.descent) / 2);
		position.y = tab->zoomRect.IsValid()
			? tab->zoomRect.top - tab->textOffset
			: tabRect.bottom - tab->textOffset;
	}

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		position);
	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
SmokeDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const Palette palette = _Palette(_TabFocus(tab));
	const float pixel = std::max(1.0f, floorf(_Scale()));
	BPoint origin;
	if (tab->look != kLeftTitledWindowLook) {
		origin.x = tab->tabRect.left + roundf(9 * _Scale());
		const float bottom = tab->look == B_FLOATING_WINDOW_LOOK
			? tab->tabRect.bottom : _SkirtBottom();
		origin.y = floorf((tab->tabRect.top + bottom + 1) / 2
			- kCloseHeight * pixel / 2) + 1;
	} else {
		origin.x = floorf((tab->tabRect.left + tab->tabRect.right + 1) / 2
			- kCloseWidth * pixel / 2) + 1;
		origin.y = rect.top + floorf((rect.Height() + 1
			- kCloseHeight * pixel) / 2);
	}

	// the tab's shading under the button, then the glyph
	_FillTabShading(tab, rect);
	_DrawGlyph(kCloseGlyph, kCloseWidth, kCloseHeight, origin, palette,
		tab->closePressed);

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


void
SmokeDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	if (!rect.IsValid() || rect.Width() < 1)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const Palette palette = _Palette(_TabFocus(tab));
	const float pixel = std::max(1.0f, floorf(_Scale()));
	BPoint origin;
	if (tab->look != kLeftTitledWindowLook) {
		origin.x = tab->tabRect.right - roundf(5 * _Scale())
			- kZoomWidth * pixel + 1;
		const float bottom = tab->look == B_FLOATING_WINDOW_LOOK
			? tab->tabRect.bottom : _SkirtBottom();
		origin.y = floorf((tab->tabRect.top + bottom + 1) / 2
			- kZoomHeight * pixel / 2) + 1;
	} else {
		origin.x = floorf((tab->tabRect.left + tab->tabRect.right + 1) / 2
			- kZoomWidth * pixel / 2) + 1;
		origin.y = rect.top + floorf((rect.Height() + 1
			- kZoomHeight * pixel) / 2);
	}

	_FillTabShading(tab, rect);
	_DrawGlyph(kZoomGlyph, kZoomWidth, kZoomHeight, origin, palette,
		tab->zoomPressed);

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


void
SmokeDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	// Smoke has no minimize button
}


/*!	A titled window's buttons are squares, \a offset in from the tab's top
	and ends, reaching as far down as the tab does over the frame (less the
	offset), whatever the tab's height before _DoTabLayout() shortens it.
*/
void
SmokeDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size, float* _inset) const
{
	if (fTopTab->look != B_TITLED_WINDOW_LOOK
		&& fTopTab->look != B_DOCUMENT_WINDOW_LOOK) {
		TabDecorator::_GetButtonSizeAndOffset(tabRect, _offset, _size,
			_inset);
		return;
	}

	const float scale = _Scale();
	const float height = _TextHeight() + roundf(7 * scale);
	*_offset = roundf(4 * scale);
	*_inset = floorf(fDrawState.Font().Size() / 6.0);
	*_size = height - 2 * *_offset - 1;
}


//	#pragma mark - Private methods


bool
SmokeDecorator::_HasTab() const
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
SmokeDecorator::_HasResizeCorner() const
{
	if ((fTopTab->flags & B_NOT_RESIZABLE) != 0 || fBorderWidth < 3)
		return false;

	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
			return true;
	}
	return false;
}


float
SmokeDecorator::_Scale() const
{
	return std::max(fDrawState.Font().Size() / 12.0f, 1.0f);
}


float
SmokeDecorator::_TextHeight() const
{
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	return ceilf(fontHeight.ascent + fontHeight.descent);
}


/*!	The last row of a titled window's tab, over the frame's top: all but
	the frame's inner line.
*/
float
SmokeDecorator::_SkirtBottom() const
{
	return fTopBorder.top + fBorderWidth - 2;
}


/*!	The content's bottom right corner, where TabDecorator puts a document
	window's resize knob; the frame round it is drawn lighter.
*/
BRect
SmokeDecorator::_CornerRect() const
{
	const float length = fResizeKnobSize - fBorderWidth;
	return BRect(fFrame.right - length, fFrame.bottom - length, fFrame.right,
		fFrame.bottom);
}


SmokeDecorator::Palette
SmokeDecorator::_Palette(bool focus) const
{
	const rgb_color base = focus ? fFocusTabColor : fNonFocusTabColor;

	Palette palette;
	palette.outer = shift(base, -98);
	palette.light = shift(base, 39);
	palette.face = shift(base, 3);
	palette.inner = palette.outer;
	palette.gradientTop = shift(base, -1);
	palette.gradientBottom = shift(base, 27);
	palette.glyph = kGlyphColor;
	palette.glyphShade = mix(kGlyphColor, shift(base, 10));
	return palette;
}


/*!	The lighter colours round the resize corner: the frame's lines, the
	grip's shading (gradientTop to gradientBottom), and its dots (glyph for
	the light half, glyphShade for the dark one).
*/
SmokeDecorator::Palette
SmokeDecorator::_CornerPalette(bool focus) const
{
	const rgb_color base = focus ? fFocusTabColor : fNonFocusTabColor;

	Palette palette;
	palette.outer = shift(base, -45);
	palette.light = shift(base, 91);
	palette.face = shift(base, 60);
	palette.inner = shift(base, -37);
	palette.gradientTop = shift(base, 59);
	palette.gradientBottom = shift(base, 83);
	palette.glyph = shift(base, 115);
	palette.glyphShade = palette.outer;
	return palette;
}


bool
SmokeDecorator::_TabFocus(Decorator::Tab* tab) const
{
	return tab != NULL && (tab->buttonFocus || IsFocus(tab));
}


/*!	The frame's \a lines rings, from the outside in: the outer line, the
	light one, the face colour, and the inner line. Along the top, the face
	is the tab's lightest shade, as it is where the tab meets the frame.
*/
void
SmokeDecorator::_StrokeFrame(BRect rect, const Palette& palette, int32 lines)
{
	for (int32 i = 0; i < lines; i++) {
		rgb_color color;
		if (i == 0)
			color = palette.outer;
		else if (i == lines - 1)
			color = palette.inner;
		else if (i == 1 && lines >= 4)
			color = palette.light;
		else
			color = palette.face;

		rgb_color top = color;
		if (i > 1 && i < lines - 1 && _HasTab())
			top = palette.gradientBottom;

		const BRect ring = rect.InsetByCopy(i, i);
		fDrawingEngine->StrokeLine(ring.LeftTop(), ring.RightTop(), top);
		fDrawingEngine->StrokeLine(BPoint(ring.left, ring.top + 1),
			ring.LeftBottom(), color);
		fDrawingEngine->StrokeLine(BPoint(ring.right, ring.top + 1),
			ring.RightBottom(), color);
		fDrawingEngine->StrokeLine(ring.LeftBottom(), ring.RightBottom(),
			color);
	}
}


/*!	A tab: its edges (the outer line, a light one inside it, and on the
	left a shadow inside that) and its shading, down to _SkirtBottom() for a
	titled window. With \a skirtOnly, only the part over the frame's top is
	drawn, from _DrawFrame(): the frame's lines carry on under the tab's
	right end, and under its left one when it doesn't start at the
	window's left edge.
*/
void
SmokeDecorator::_DrawTabBody(Decorator::Tab* tab, bool skirtOnly)
{
	const BRect& tabRect = tab->tabRect;
	if (!tabRect.IsValid())
		return;

	const bool hasSkirt = tab->look == B_TITLED_WINDOW_LOOK
		|| tab->look == B_DOCUMENT_WINDOW_LOOK;
	if (skirtOnly && !hasSkirt)
		return;

	const Palette palette = _Palette(_TabFocus(tab));
	const float bottom = hasSkirt ? _SkirtBottom() : tabRect.bottom;

	BRect area(tabRect.left, tabRect.top, tabRect.right, bottom);
	if (skirtOnly)
		area.top = fTopBorder.top;

	if (tab->look == kLeftTitledWindowLook) {
		// sideways: the shading runs from left to right
		BGradientLinear gradient;
		gradient.AddColor(palette.gradientTop, 0);
		gradient.AddColor(palette.gradientBottom, 255);
		gradient.SetStart(BPoint(tabRect.left + 2, tabRect.top));
		gradient.SetEnd(BPoint(tabRect.right, tabRect.top));
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right, tabRect.bottom - 2), gradient);
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
			palette.outer);
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.LeftBottom(),
			palette.outer);
		fDrawingEngine->StrokeLine(tabRect.LeftBottom(), tabRect.RightBottom(),
			palette.outer);
		fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
			BPoint(tabRect.right, tabRect.top + 1), palette.light);
		fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
			BPoint(tabRect.left + 1, tabRect.bottom - 1), palette.light);
		fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1,
			tabRect.bottom - 1), BPoint(tabRect.right, tabRect.bottom - 1),
			palette.light);
		return;
	}

	BGradientLinear gradient;
	gradient.AddColor(palette.gradientTop, 0);
	gradient.AddColor(palette.gradientBottom, 255);
	gradient.SetStart(BPoint(tabRect.left, tabRect.top + 2));
	gradient.SetEnd(BPoint(tabRect.left, bottom));

	BGradientLinear shadow;
	shadow.AddColor(shift(palette.gradientTop, -20), 0);
	shadow.AddColor(shift(palette.gradientBottom, -20), 255);
	shadow.SetStart(BPoint(tabRect.left, tabRect.top + 2));
	shadow.SetEnd(BPoint(tabRect.left, bottom));

	const bool atLeftEdge = tabRect.left <= fBorderRect.left;
	const float fillTop = std::max(area.top, tabRect.top + 2);

	// the shading, and the shadow along the left
	BRect fill(tabRect.left + 3, fillTop, tabRect.right - 2, bottom);
	if (skirtOnly && !atLeftEdge)
		fill.left = tabRect.left + 2;
	if (fill.IsValid())
		fDrawingEngine->FillRect(fill, gradient);
	if (!skirtOnly || atLeftEdge) {
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, fillTop,
			tabRect.left + 2, bottom), shadow);
	}

	if (skirtOnly) {
		if (atLeftEdge) {
			fDrawingEngine->StrokeLine(BPoint(tabRect.left, area.top),
				BPoint(tabRect.left, bottom), palette.outer);
			fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, area.top),
				BPoint(tabRect.left + 1, bottom), palette.light);
		}
		// the tab's light edge on the right, down to the frame's light line
		fDrawingEngine->StrokeLine(BPoint(tabRect.right - 1, area.top),
			BPoint(tabRect.right - 1, std::min(bottom, fTopBorder.top + 1)),
			palette.light);
		return;
	}

	// the edges, down to the frame (the skirt's are drawn with the frame)
	const float edgeBottom = std::min(bottom, tabRect.bottom);
	fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
		palette.outer);
	fDrawingEngine->StrokeLine(tabRect.LeftTop(),
		BPoint(tabRect.left, edgeBottom), palette.outer);
	fDrawingEngine->StrokeLine(tabRect.RightTop(),
		BPoint(tabRect.right, edgeBottom), palette.outer);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.right - 1, tabRect.top + 1), palette.light);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.left + 1, edgeBottom), palette.light);
	fDrawingEngine->StrokeLine(BPoint(tabRect.right - 1, tabRect.top + 1),
		BPoint(tabRect.right - 1, edgeBottom), palette.light);

	if (!hasSkirt) {
		// a floating window's tab has no skirt; its bottom is the frame's
		fDrawingEngine->StrokeLine(BPoint(tabRect.left + 2, tabRect.top + 2),
			BPoint(tabRect.left + 2, tabRect.bottom), palette.face);
	}
}


/*!	Fills \a rect, inside the tab, with the tab's shading, as under a
	button before its glyph is drawn again.
*/
void
SmokeDecorator::_FillTabShading(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;
	const Palette palette = _Palette(_TabFocus(tab));
	const bool hasSkirt = tab->look == B_TITLED_WINDOW_LOOK
		|| tab->look == B_DOCUMENT_WINDOW_LOOK;
	const float bottom = hasSkirt ? _SkirtBottom() : tabRect.bottom;

	BGradientLinear gradient;
	gradient.AddColor(palette.gradientTop, 0);
	gradient.AddColor(palette.gradientBottom, 255);
	if (tab->look == kLeftTitledWindowLook) {
		gradient.SetStart(BPoint(tabRect.left + 2, tabRect.top));
		gradient.SetEnd(BPoint(tabRect.right, tabRect.top));
	} else {
		gradient.SetStart(BPoint(tabRect.left, tabRect.top + 2));
		gradient.SetEnd(BPoint(tabRect.left, bottom));
	}

	rect = rect & BRect(tabRect.left + 3, tabRect.top + 2, tabRect.right - 2,
		bottom);
	if (rect.IsValid())
		fDrawingEngine->FillRect(rect, gradient);
}


/*!	The lighter frame round the bottom right corner of a resizable window,
	and a document window's grip: an inner line along its top and left, a
	light line and a shadow inside them, a shading from left to right and
	six dots in a triangle, each a dark pixel with a light one below right.
*/
void
SmokeDecorator::_DrawResizeCorner(BRect invalid)
{
	if (!_HasResizeCorner())
		return;

	const BRect corner = _CornerRect();
	const BRect outer = fBorderRect;
	if (!invalid.Intersects(BRect(corner.left, corner.top, outer.right,
			outer.bottom)))
		return;

	const Palette palette = _CornerPalette(_TabFocus(fTopTab));
	const int32 lines = fBorderWidth;
	for (int32 i = 0; i < lines; i++) {
		rgb_color color;
		if (i == 0)
			color = palette.outer;
		else if (i == lines - 1)
			color = palette.inner;
		else if (i == 1 && lines >= 4)
			color = palette.light;
		else
			color = palette.face;

		fDrawingEngine->StrokeLine(BPoint(outer.right - i, corner.top),
			BPoint(outer.right - i, outer.bottom - i), color);
		fDrawingEngine->StrokeLine(BPoint(corner.left, outer.bottom - i),
			BPoint(outer.right - i, outer.bottom - i), color);
	}

	if (fTopTab->look != B_DOCUMENT_WINDOW_LOOK)
		return;

	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(),
		palette.inner);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(),
		palette.inner);
	fDrawingEngine->StrokeLine(BPoint(corner.left + 1, corner.top + 1),
		BPoint(corner.right, corner.top + 1), palette.light);
	fDrawingEngine->StrokeLine(BPoint(corner.left + 1, corner.top + 1),
		BPoint(corner.left + 1, corner.bottom), palette.light);
	fDrawingEngine->StrokeLine(BPoint(corner.left + 2, corner.top + 2),
		BPoint(corner.left + 2, corner.bottom), shift(palette.gradientTop, -15));

	BGradientLinear gradient;
	gradient.AddColor(palette.gradientTop, 0);
	gradient.AddColor(palette.gradientBottom, 255);
	gradient.SetStart(BPoint(corner.left + 3, corner.top));
	gradient.SetEnd(BPoint(corner.right, corner.top));
	fDrawingEngine->FillRect(BRect(corner.left + 3, corner.top + 2,
		corner.right, corner.bottom), gradient);

	// the dots, counted from the corner in steps of three
	const float scale = _Scale();
	const float pixel = std::max(1.0f, floorf(scale));
	const int32 dots[][2] = { { 1, 3 }, { 2, 2 }, { 1, 2 }, { 3, 1 },
		{ 2, 1 }, { 1, 1 } };
	for (size_t i = 0; i < sizeof(dots) / sizeof(dots[0]); i++) {
		const float x = corner.right - roundf((dots[i][0] * 3 - 1) * scale);
		const float y = corner.bottom - roundf((dots[i][1] * 3 - 1) * scale);
		fDrawingEngine->FillRect(BRect(x, y, x + pixel - 1, y + pixel - 1),
			palette.glyphShade);
		fDrawingEngine->FillRect(BRect(x + pixel, y + pixel,
			x + 2 * pixel - 1, y + 2 * pixel - 1), palette.glyph);
	}
}


void
SmokeDecorator::_DrawGlyph(const char* const* glyph, int32 width,
	int32 height, BPoint origin, const Palette& palette, bool pressed)
{
	const float pixel = std::max(1.0f, floorf(_Scale()));
	if (pressed)
		origin += BPoint(pixel, pixel);

	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			const char c = glyph[y][x];
			if (c != 'X' && c != 's')
				continue;
			rgb_color color = c == 'X' ? palette.glyph : palette.glyphShade;
			if (pressed && c == 'X')
				color = palette.glyphShade;
			const float left = origin.x + x * pixel;
			const float top = origin.y + y * pixel;
			fDrawingEngine->FillRect(BRect(left, top, left + pixel - 1,
				top + pixel - 1), color);
		}
	}
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) SmokeDecorAddOn(id, name);
}


#ifdef SMOKE_ROUNDED_CORNERS
static const int32 kFrameCornerRadius = 6;
static const int32 kTabCornerRadius = 5;


void
SmokeDecorator::GetRoundedShape(std::vector<ShapeRect>& shape) const
{
	GetShape(shape);
	RoundedDecorator::RoundShape(shape, fTopTab->look, kFrameCornerRadius,
		kTabCornerRadius);
}


//! The compositor cuts off the corners, the border follows them.
void
SmokeDecorator::_StrokeRoundedFrame()
{
	if (!RoundedDecorator::IsActive(GetDesktop()))
		return;

	std::vector<ShapeRect> shape;
	GetRoundedShape(shape);
	const rgb_color outer = _Palette(_TabFocus(fTopTab)).outer;
	RoundedDecorator::StrokeCorners(fDrawingEngine, shape[0], outer, outer,
		outer, outer);
}


void
SmokeDecorator::_StrokeRoundedTab(Decorator::Tab* tab)
{
	if (!RoundedDecorator::IsActive(GetDesktop()))
		return;

	std::vector<ShapeRect> shape;
	GetRoundedShape(shape);
	for (size_t i = 1; i < shape.size(); i++) {
		if (shape[i].rect != tab->tabRect)
			continue;
		const rgb_color outer = _Palette(_TabFocus(tab)).outer;
		RoundedDecorator::StrokeCorners(fDrawingEngine, shape[i], outer, outer,
			outer, outer);
		break;
	}
}
#endif	// SMOKE_ROUNDED_CORNERS
