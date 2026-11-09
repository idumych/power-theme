/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See QNXDecorator.h. Colours and sizes were measured from screenshots of
 * QNX 6.2.1's Photon microGUI with its standard colours, on toastytech.com.
 */


#include "QNXDecorator.h"

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


#define GREY(level) { level, level, level, 255 }

static const rgb_color kBlack = GREY(0);
// the frame: the dark line outside the ridge, the ridge's light, face and
// shaded lines, and the dark line round the content
static const rgb_color kOuter = GREY(0x3f);
static const rgb_color kRidgeLight = GREY(0xff);
static const rgb_color kRidgeFace = GREY(0xd9);
static const rgb_color kRidgeShade = GREY(0x9d);
static const rgb_color kInner = GREY(0x4b);
// the panel of boxes at the bar's right end, and the boxes on it
static const rgb_color kPanelShade = GREY(0xa6);
static const rgb_color kPanelTop = GREY(0xc5);
static const rgb_color kPanelBottom = GREY(0xe4);
static const rgb_color kWellDark = GREY(0xba);
static const rgb_color kWellLight = GREY(0xf6);
static const rgb_color kBoxEdge = GREY(0x3e);
static const rgb_color kGroove = GREY(0x90);
static const rgb_color kPanelFoot = GREY(0xd8);
// the window menu box's bars
static const rgb_color kMenuEdge = GREY(0x48);
static const rgb_color kMenuLight = GREY(0xe3);
static const rgb_color kMenuFace = GREY(0xb7);
static const rgb_color kMenuDark = GREY(0x80);
// a scroll bar's corner
static const rgb_color kCorner = GREY(0xd8);

static const float kBorder = 6;
// the bar's two outer lines and its nineteen rows
static const float kTitleHeight = 21;
static const float kBox = 13;
static const float kResizeCornerLength = 15;
static const float kBorderResizeLength = 22;


static void
offset_rect(BRect& rect, BPoint offset)
{
	if (rect.IsValid())
		rect.OffsetBy(offset);
}


//!	\a color, each of its channels moved by \a delta.
static rgb_color
shift(rgb_color color, int delta)
{
	color.red = (uint8)std::max(0, std::min(255, color.red + delta));
	color.green = (uint8)std::max(0, std::min(255, color.green + delta));
	color.blue = (uint8)std::max(0, std::min(255, color.blue + delta));
	return color;
}


static rgb_color
mix(rgb_color a, rgb_color b, float amount)
{
	rgb_color color;
	color.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	color.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	color.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	color.alpha = 255;
	return color;
}


// Power Theme's setting, in the user's settings: there if tabs are wanted
static const char* kBeOSTabsSetting = "PowerTheme/beos tabs";


//	#pragma mark - QNXDecorAddOn


QNXDecorAddOn::QNXDecorAddOn(image_id id, const char* name)
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
QNXDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow) QNXDecorator(settings, rect, desktop,
		fBeOSTabs);
}


//	#pragma mark - QNXDecorator


QNXDecorator::QNXDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop, bool beosTabs)
	:
	Decorator(settings, rect, desktop),
	fBeOSTabs(beosTabs)
{
	UpdateColors(settings);
}


QNXDecorator::~QNXDecorator()
{
}


void
QNXDecorator::Draw(BRect updateRect)
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(updateRect & fOutlineBorderRect);

	_DrawTabs(updateRect & fTitleBarRect);
}


void
QNXDecorator::Draw()
{
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);

	if (IsOutlineResizing())
		_DrawOutlineFrame(fOutlineBorderRect);

	_DrawTabs(fTitleBarRect);
}


Decorator::Region
QNXDecorator::RegionAt(BPoint where, int32& tabIndex) const
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
		BRect resizeRect(BPoint(fBottomBorder.right - kBorderResizeLength,
			fBottomBorder.bottom - kBorderResizeLength),
			fBottomBorder.RightBottom());
		if (resizeRect.Contains(where))
			return REGION_RIGHT_BOTTOM_CORNER;
	}

	return region;
}


void
QNXDecorator::UpdateColors(DesktopSettings& settings)
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
QNXDecorator::_DoLayout()
{
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			fBorderWidth = (int32)kBorder;
			break;

		// menus and the like: a black line
		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	if (fBorderWidth > 0) {
		const float w = fBorderWidth;
		// a full title bar takes the place of the frame's top, all but the
		// dark line over the content
		const float top = _HasFullTitleBar() ? 1 : w;
		fLeftBorder.Set(fFrame.left - w, fFrame.top, fFrame.left - 1,
			fFrame.bottom);
		fRightBorder.Set(fFrame.right + 1, fFrame.top, fFrame.right + w,
			fFrame.bottom);
		fTopBorder.Set(fFrame.left - w, fFrame.top - top, fFrame.right + w,
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
		// the content's corner, as far in as a scroll bar's width
		const float knob = kResizeCornerLength + fBorderWidth;
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
QNXDecorator::_DoOutlineLayout()
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


/*!	The frame: a black line outside, a dark grey one inside it, a ridge three
	pixels wide lit along each side's top or left, and a dark line round the
	content. A full title bar draws the outer lines along the top itself.
*/
void
QNXDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK || fBorderWidth <= 0
		|| !rect.IsValid())
		return;

	const BRect& r = fBorderRect;
	if (fTopTab->look == B_BORDERED_WINDOW_LOOK) {
		fDrawingEngine->StrokeRect(r, kBlack);
		return;
	}

	const bool fullBar = _HasFullTitleBar();
	const float l = r.left, t = r.top, rt = r.right, b = r.bottom;

	// the ridge's face, then its lines
	fDrawingEngine->FillRect(BRect(l + 2, t, l + 4, b - 2) & rect, kRidgeFace);
	fDrawingEngine->FillRect(BRect(rt - 4, t, rt - 2, b - 2) & rect,
		kRidgeFace);
	fDrawingEngine->FillRect(BRect(l + 2, b - 4, rt - 2, b - 2) & rect,
		kRidgeFace);
	if (!fullBar) {
		fDrawingEngine->FillRect(BRect(l + 2, t + 2, rt - 2, t + 4) & rect,
			kRidgeFace);
	}

	const float ridgeTop = fullBar ? t : t + 2;
	// left: lit on its outer side, shaded on its inner
	fDrawingEngine->StrokeLine(BPoint(l + 2, ridgeTop), BPoint(l + 2, b - 3),
		kRidgeLight);
	fDrawingEngine->StrokeLine(BPoint(l + 4, fullBar ? t : t + 4),
		BPoint(l + 4, b - 5), kRidgeShade);
	// right: lit on its inner side, shaded on its outer
	fDrawingEngine->StrokeLine(BPoint(rt - 4, fullBar ? t : t + 4),
		BPoint(rt - 4, b - 5), kRidgeLight);
	fDrawingEngine->StrokeLine(BPoint(rt - 2, ridgeTop), BPoint(rt - 2, b - 2),
		kRidgeShade);
	// bottom: lit along its top, shaded along its bottom
	fDrawingEngine->StrokeLine(BPoint(l + 4, b - 4), BPoint(rt - 4, b - 4),
		kRidgeLight);
	fDrawingEngine->StrokeLine(BPoint(l + 3, b - 2), BPoint(rt - 2, b - 2),
		kRidgeShade);
	if (!fullBar) {
		fDrawingEngine->StrokeLine(BPoint(l + 2, t + 2), BPoint(rt - 3, t + 2),
			kRidgeLight);
		fDrawingEngine->StrokeLine(BPoint(l + 4, t + 4), BPoint(rt - 5, t + 4),
			kRidgeShade);
	}

	// the outer lines
	fDrawingEngine->StrokeLine(BPoint(l, t), BPoint(l, b), kBlack);
	fDrawingEngine->StrokeLine(BPoint(rt, t), BPoint(rt, b), kBlack);
	fDrawingEngine->StrokeLine(BPoint(l, b), BPoint(rt, b), kBlack);
	fDrawingEngine->StrokeLine(BPoint(l + 1, t), BPoint(l + 1, b - 1), kOuter);
	fDrawingEngine->StrokeLine(BPoint(rt - 1, t), BPoint(rt - 1, b - 1),
		kOuter);
	fDrawingEngine->StrokeLine(BPoint(l + 1, b - 1), BPoint(rt - 1, b - 1),
		kOuter);
	if (!fullBar) {
		fDrawingEngine->StrokeLine(BPoint(l, t), BPoint(rt, t), kBlack);
		fDrawingEngine->StrokeLine(BPoint(l + 1, t + 1), BPoint(rt - 1, t + 1),
			kOuter);
	}

	// the line round the content
	fDrawingEngine->StrokeRect(BRect(l + 5, fullBar ? t : t + 5, rt - 5,
		b - 5), kInner);

	if (_HasResizeCorner() || ((fTopTab->flags & B_NOT_RESIZABLE) == 0
			&& fTopTab->look != B_BORDERED_WINDOW_LOOK))
		_DrawResizeCorner();
}


void
QNXDecorator::_DrawOutlineFrame(BRect rect)
{
	drawing_mode oldMode;

	fDrawingEngine->SetDrawingMode(B_OP_ALPHA, oldMode);
	fDrawingEngine->SetPattern(B_MIXED_COLORS);
	fDrawingEngine->StrokeRect(rect);

	fDrawingEngine->SetDrawingMode(oldMode);
}


/*!	The bar: black and dark grey lines along its top and ends, then the blue,
	with a light line along its top and left, a dark one under that and along
	its bottom and right, and between them a shade from light to dark; the
	window menu box sunk into its left end, the title in the middle, and the
	grey panel of boxes at its right end.
*/
void
QNXDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	const BRect& t = tab->tabRect;
	if (!t.IsValid() || !invalid.Intersects(t))
		return;

	const bool focus = IsFocus(tab);
	const rgb_color face = focus ? fFocusTabColor : fNonFocusTabColor;
	const rgb_color light = shift(face, 0x32);
	const rgb_color dark = shift(face, -0x32);
	const rgb_color shadeTop = shift(face, 0x0a);
	const rgb_color shadeBottom = shift(face, -0x15);

	fDrawingEngine->StrokeLine(t.LeftTop(), t.RightTop(), kBlack);
	fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 1),
		BPoint(t.right - 1, t.top + 1), kOuter);
	fDrawingEngine->StrokeLine(BPoint(t.left, t.top + 1), t.LeftBottom(),
		kBlack);
	fDrawingEngine->StrokeLine(BPoint(t.left + 1, t.top + 2),
		BPoint(t.left + 1, t.bottom), kOuter);
	fDrawingEngine->StrokeLine(BPoint(t.right, t.top + 1), t.RightBottom(),
		kBlack);
	fDrawingEngine->StrokeLine(BPoint(t.right - 1, t.top + 2),
		BPoint(t.right - 1, t.bottom), kOuter);

	const float gadgets = _GadgetsWidth(tab);
	// the blue, from inside the outer lines to the panel
	const float left = t.left + 2;
	const float right = gadgets > 0 ? t.right - gadgets - 1 : t.right - 2;
	if (right > left) {
		const float y = t.top + 2;
		fDrawingEngine->StrokeLine(BPoint(left, y), BPoint(right, y), light);
		fDrawingEngine->StrokeLine(BPoint(left, y + 1), BPoint(right, y + 1),
			face);
		fDrawingEngine->StrokeLine(BPoint(left, y + 2), BPoint(right, y + 2),
			dark);
		fDrawingEngine->StrokeLine(BPoint(left, y + 3), BPoint(right, y + 3),
			face);
		const float first = y + 4;
		const float last = t.bottom - 1;
		for (float row = first; row <= last; row++) {
			if (row < invalid.top || row > invalid.bottom)
				continue;
			const float amount = last > first ? (row - first) / (last - first)
				: 0;
			fDrawingEngine->StrokeLine(BPoint(left, row), BPoint(right, row),
				mix(shadeTop, shadeBottom, amount));
		}
		fDrawingEngine->StrokeLine(BPoint(left, t.bottom),
			BPoint(right, t.bottom), dark);
		fDrawingEngine->StrokeLine(BPoint(left, y), BPoint(left, t.bottom - 1),
			light);
		fDrawingEngine->StrokeLine(BPoint(right, y + 1),
			BPoint(right, t.bottom), dark);
	}

	_DrawMenuBox(tab, face);
	_DrawTitle(tab, t);
	if (gadgets > 0)
		_DrawGadgets(tab, invalid);
}


//!	How wide the panel of boxes is, from its dark line to the bar's end.
float
QNXDecorator::_GadgetsWidth(const Decorator::Tab* tab) const
{
	const BRect* boxes[3] = { &tab->minimizeRect, &tab->zoomRect,
		&tab->closeRect };
	for (int32 i = 0; i < 3; i++) {
		if (boxes[i]->IsValid())
			return tab->tabRect.right - boxes[i]->left + 6;
	}
	return 0;
}


/*!	The window menu box, sunk into the bar's left end: a grey box with a dark
	edge and three bars across it, each lit along its top.
*/
void
QNXDecorator::_DrawMenuBox(const Decorator::Tab* tab, rgb_color face)
{
	const BRect& t = tab->tabRect;
	const BRect well(t.left + 5, t.top + 6, t.left + 17, t.top + 17);
	if (well.right >= t.right - _GadgetsWidth(tab) - 2)
		return;

	fDrawingEngine->StrokeLine(well.LeftTop(), well.RightTop(),
		shift(face, -0x1e));
	fDrawingEngine->StrokeLine(well.LeftTop(), well.LeftBottom(),
		shift(face, -0x1e));
	fDrawingEngine->StrokeLine(well.LeftBottom(), well.RightBottom(),
		shift(face, 0x1e));
	fDrawingEngine->StrokeLine(BPoint(well.right, well.top + 1),
		well.RightBottom(), shift(face, 0x1e));

	const BRect box(well.left + 1, well.top + 1, well.right - 1,
		well.bottom - 1);
	fDrawingEngine->StrokeRect(box, kMenuEdge);
	for (int32 i = 0; i < 3; i++) {
		const float y = box.top + 1 + i * 3;
		if (y + 2 >= box.bottom + 1)
			break;
		fDrawingEngine->StrokeLine(BPoint(box.left + 1, y),
			BPoint(box.right - 1, y), kMenuLight);
		fDrawingEngine->StrokeLine(BPoint(box.left + 1, y + 1),
			BPoint(box.right - 1, y + 1), kMenuFace);
		if (y + 2 < box.bottom) {
			fDrawingEngine->StrokeLine(BPoint(box.left + 1, y + 2),
				BPoint(box.right - 1, y + 2), kMenuDark);
		}
	}
}


/*!	Where the title goes: in the middle of the blue, clear of the menu box and
	the panel, with its baseline where Photon's was.
*/
BRect
QNXDecorator::_TitleRect(const Decorator::Tab* tab) const
{
	const BRect& t = tab->tabRect;
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);
	const float width = fDrawState.Font().StringWidth(tab->truncatedTitle,
		tab->truncatedTitleLength);

	const float left = t.left + tab->textOffset;
	const float right = t.right - _GadgetsWidth(tab) - 6;
	float x = floorf((left + right + 1 - width) / 2);
	x = std::max(left, std::min(x, right - width));
	// in the middle of the shaded rows
	const float top = t.top + 5;
	const float space = t.bottom - 1 - top;
	const float baseline = floorf(top + (space + fontHeight.ascent
		- fontHeight.descent) / 2 + 1);
	return BRect(x, baseline - ceilf(fontHeight.ascent), x + ceilf(width) - 1,
		baseline);
}


void
QNXDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
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


/*!	The grey panel at the bar's right end: a dark line and a shaded one along
	its left, shaded from darker at the top to lighter at the bottom, with the
	frame's ridge along its right, and a groove before the close box.
*/
void
QNXDecorator::_DrawGadgets(Decorator::Tab* tab, const BRect& invalid)
{
	const BRect& t = tab->tabRect;
	const float width = _GadgetsWidth(tab);
	const float left = t.right - width;
	const float top = t.top + 2;

	fDrawingEngine->StrokeLine(BPoint(left, top), BPoint(left, t.bottom),
		kOuter);
	fDrawingEngine->StrokeLine(BPoint(left + 1, top),
		BPoint(left + 1, t.bottom), kPanelShade);
	// white rows round a shade from darker at the top to lighter at the
	// bottom, a grey row and a dark one under them
	const float right = t.right - 4;
	fDrawingEngine->StrokeLine(BPoint(left + 2, top), BPoint(right, top),
		kRidgeLight);
	const float first = top + 1;
	const float last = t.bottom - 3;
	for (float row = first; row <= last; row++) {
		const float amount = last > first ? (row - first) / (last - first) : 0;
		fDrawingEngine->StrokeLine(BPoint(left + 2, row), BPoint(right, row),
			mix(kPanelTop, kPanelBottom, amount));
	}
	fDrawingEngine->StrokeLine(BPoint(left + 2, t.bottom - 2),
		BPoint(right, t.bottom - 2), kRidgeLight);
	fDrawingEngine->StrokeLine(BPoint(left + 2, t.bottom - 1),
		BPoint(right, t.bottom - 1), kPanelFoot);
	fDrawingEngine->StrokeLine(BPoint(left + 2, t.bottom),
		BPoint(right, t.bottom), kPanelShade);
	// the frame's ridge, on round the bar's end
	fDrawingEngine->StrokeLine(BPoint(t.right - 3, top),
		BPoint(t.right - 3, t.bottom), kRidgeFace);
	fDrawingEngine->StrokeLine(BPoint(t.right - 2, top),
		BPoint(t.right - 2, t.bottom), kRidgeShade);

	if (tab->closeRect.IsValid()
		&& (tab->zoomRect.IsValid() || tab->minimizeRect.IsValid())) {
		const float x = tab->closeRect.left - 4;
		fDrawingEngine->StrokeLine(BPoint(x, top), BPoint(x, t.bottom - 1),
			kGroove);
		fDrawingEngine->StrokeLine(BPoint(x + 1, top),
			BPoint(x + 1, t.bottom - 1), kRidgeLight);
	}

	_DrawButtons(tab, invalid);
}


void
QNXDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
QNXDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->closePressed, GLYPH_CLOSE);
}


void
QNXDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->minimizePressed, GLYPH_MINIMIZE);
}


void
QNXDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBox(tab, direct, rect, tab->zoomPressed, GLYPH_ZOOM);
}


void
QNXDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
QNXDecorator::_SetFocus(Decorator::Tab* tab)
{
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
}


bool
QNXDecorator::_SetTabLocation(Decorator::Tab* tab, float location,
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
QNXDecorator::_SetSettings(const BMessage& settings,
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
QNXDecorator::_UpdateFont(DesktopSettings& settings)
{
	// Photon's titles are in its plain face, no larger than the bar has room
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	font.SetSize(std::min(font.Size(), 12.0f));

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
QNXDecorator::_MoveBy(BPoint offset)
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
QNXDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
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
QNXDecorator::_AddTab(DesktopSettings& settings, int32 index,
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
QNXDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
QNXDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
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
QNXDecorator::_GetFootprint(BRegion* region)
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
QNXDecorator::_HasTitle() const
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
QNXDecorator::_HasFullTitleBar() const
{
	return _HasTitle() && !fBeOSTabs;
}


//!	How wide a BeOS-style tab has to be: its menu box, title and panel.
float
QNXDecorator::_TabWidth(const Decorator::Tab* tab) const
{
	float width = 18 + 8 + 8;
	if ((tab->flags & B_NOT_CLOSABLE) == 0)
		width += 20;
	if ((tab->flags & B_NOT_ZOOMABLE) == 0)
		width += 16 + 10;
	if ((tab->flags & B_NOT_MINIMIZABLE) == 0)
		width += 16;
	width += 6;
	const char* title = tab->title.String();
	return width + ceilf(fDrawState.Font().StringWidth(title, strlen(title)));
}


bool
QNXDecorator::_IsSmall() const
{
	return fTopTab != NULL && (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook);
}


float
QNXDecorator::_Scale() const
{
	return 1;
}


float
QNXDecorator::_TitleHeight() const
{
	return kTitleHeight;
}


float
QNXDecorator::_ButtonSize() const
{
	return kBox;
}


float
QNXDecorator::_ButtonEdge() const
{
	return 8;
}


float
QNXDecorator::_ButtonGap() const
{
	return 3;
}


float
QNXDecorator::_GlyphSpan() const
{
	return 0;
}


//!	Box \a slot from the bar's right end: close, then zoom, then minimize.
BRect
QNXDecorator::_ButtonSlot(const Decorator::Tab* tab, int32 slot) const
{
	const BRect& t = tab->tabRect;
	const float top = t.top + 5;
	const float right = t.right - _ButtonEdge();
	return BRect(right - kBox + 1, top, right, top + kBox - 1);
}


bool
QNXDecorator::_HasResizeCorner() const
{
	return fTopTab->look == B_DOCUMENT_WINDOW_LOOK
		&& (fTopTab->flags & B_NOT_RESIZABLE) == 0;
}


/*!	Lays the tabs out side by side across the whole window, each with its menu
	box at its left end, its boxes at its right end and its title between.

	A BeOS-style tab sits on the frame, over its outer lines, and is as wide as
	its title; a stack's tabs share the window's width.
*/
void
QNXDecorator::_LayoutTabs()
{
	const int32 count = fTabList.CountItems();
	if (count == 0) {
		fTitleBarRect.Set(0, 0, -1, -1);
		return;
	}

	const ServerFont& font = fDrawState.Font();
	float left = fBorderRect.left;
	float width = fBorderRect.Width() + 1;
	const float height = _TitleHeight();
	// a full bar ends over the content's dark line; a tab, on the frame's
	// dark grey line
	const float bottom = fBeOSTabs ? fBorderRect.top + 1 : fBorderRect.top - 1;
	const float top = bottom - height + 1;
	if (fBeOSTabs && count == 1) {
		Decorator::Tab* tab = fTabList.ItemAt(0);
		const float tabWidth = std::min(_TabWidth(tab), width);
		left += std::min(tab->tabLocation, width - tabWidth);
		width = tabWidth;
	}
	const float minTitle = 32;

	float minimum = 0;
	for (int32 i = 0; i < count; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		const float tabLeft = left + floorf(width * i / count);
		const float tabRight = left + floorf(width * (i + 1) / count) - 1;
		tab->tabRect.Set(tabLeft, top, tabRight, bottom);

		const bool closable = (tab->flags & B_NOT_CLOSABLE) == 0;
		const bool zoomable = (tab->flags & B_NOT_ZOOMABLE) == 0;
		const bool minimizable = (tab->flags & B_NOT_MINIMIZABLE) == 0;

		// from the right: close, a groove, zoom, then minimize
		BRect slot = _ButtonSlot(tab, 0);
		tab->closeRect.Set(0, 0, -1, -1);
		tab->zoomRect.Set(0, 0, -1, -1);
		tab->minimizeRect.Set(0, 0, -1, -1);
		if (closable) {
			tab->closeRect = slot;
			slot.OffsetBy(-(kBox + (zoomable || minimizable ? 10 : 3)), 0);
		}
		if (zoomable) {
			tab->zoomRect = slot;
			slot.OffsetBy(-(kBox + _ButtonGap()), 0);
		}
		if (minimizable)
			tab->minimizeRect = slot;

		const float gadgets = _GadgetsWidth(tab);
		// the title keeps clear of the menu box, and the panel
		const float reserved = 18 + 6;
		const float room = (tabRight - tabLeft + 1) - reserved - gadgets - 6;
		const float taken = reserved + gadgets + 6;

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


/*!	The frame's bottom right corner, where it resizes the window: its ridge in
	the bar's blue for the last nine pixels each way, and on a document window
	the content's corner plain grey, where the scroll bars meet.
*/
void
QNXDecorator::_DrawResizeCorner()
{
	const rgb_color face = IsFocus(fTopTab) ? fFocusTabColor
		: fNonFocusTabColor;
	const BRect& r = fBorderRect;
	const float rt = r.right, b = r.bottom;

	// the right side's ridge and the bottom's, up and along nine pixels
	fDrawingEngine->FillRect(BRect(rt - 4, b - 9, rt - 2, b - 2), face);
	fDrawingEngine->FillRect(BRect(rt - 9, b - 4, rt - 2, b - 2), face);
	fDrawingEngine->StrokeLine(BPoint(rt - 9, b - 4), BPoint(rt - 5, b - 4),
		shift(face, -0x1c));
	fDrawingEngine->StrokeLine(BPoint(rt - 4, b - 9), BPoint(rt - 4, b - 4),
		shift(face, -0x1c));
	fDrawingEngine->StrokeLine(BPoint(rt - 9, b - 2), BPoint(rt - 2, b - 2),
		shift(face, -0x3c));
	fDrawingEngine->StrokeLine(BPoint(rt - 2, b - 9), BPoint(rt - 2, b - 2),
		shift(face, -0x3c));

	if (!_HasResizeCorner())
		return;
	const BRect corner(fFrame.right - kResizeCornerLength + 1,
		fFrame.bottom - kResizeCornerLength + 1, fFrame.right, fFrame.bottom);
	fDrawingEngine->FillRect(corner, kCorner);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.RightTop(), kInner);
	fDrawingEngine->StrokeLine(corner.LeftTop(), corner.LeftBottom(), kInner);
}


/*!	A box on the panel, in a well darker along its top and left: a dark edge
	round a small picture of a window, in greys, lit along its top and left.
	Minimize is a darker pane with a triangle pointing down, over a strip like
	a taskbar; maximize a window with its title bar; close a sunk square with
	a small raised one in it. Pressed, the picture is darker.
*/
void
QNXDecorator::_DrawBox(Decorator::Tab* tab, bool direct, BRect rect,
	bool pressed, glyph glyph)
{
	if (!rect.IsValid() || rect.Width() < 12)
		return;

	const bool copyToFront = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);

	const float l = rect.left, t = rect.top, r = rect.right, b = rect.bottom;
	const int dim = pressed ? -0x24 : 0;
	auto grey = [dim](int level) {
		const uint8 value = (uint8)std::max(0, std::min(255, level + dim));
		const rgb_color color = { value, value, value, 255 };
		return color;
	};
	auto line = [this](float x0, float y0, float x1, float y1, rgb_color c) {
		fDrawingEngine->StrokeLine(BPoint(x0, y0), BPoint(x1, y1), c);
	};

	// the well
	line(l - 1, t - 1, r, t - 1, kWellDark);
	line(l - 1, t - 1, l - 1, b + 1, kWellDark);
	line(l, b + 1, r + 1, b + 1, kWellLight);
	line(r + 1, t - 1, r + 1, b, kWellLight);
	fDrawingEngine->StrokeRect(rect, kBoxEdge);

	// a shaded face, inside the edge from l + 1 to r - 1 and t + 1 to b - 1:
	// from \a top to \a bottom, lit along its left and shaded along its right
	auto face = [&](float y0, float y1, int top, int bottom, int lit,
			int shade) {
		for (float y = y0; y <= y1; y++) {
			const float amount = y1 > y0 ? (y - y0) / (y1 - y0) : 0;
			const int level = (int)roundf(top + (bottom - top) * amount);
			line(l + 2, y, r - 2, y, grey(level));
			line(l + 1, y, l + 1, y, grey(level + lit));
			line(r - 1, y, r - 1, y, grey(level - shade));
		}
	};

	switch (glyph) {
		case GLYPH_MINIMIZE:
		{
			// the pane, its triangle, and the strip under a dark line
			line(l + 1, t + 1, r - 2, t + 1, grey(0xbf));
			line(r - 1, t + 1, r - 1, t + 1, grey(0x99));
			face(t + 2, t + 6, 0xa3, 0xa3, 0x1c, 0x21);
			line(l + 1, t + 7, l + 1, t + 7, grey(0x99));
			line(l + 2, t + 7, r - 1, t + 7, grey(0x82));
			for (int32 i = 0; i < 3; i++) {
				line(l + 4 + i, t + 3 + i, l + 8 - i, t + 3 + i,
					grey(0x51));
			}
			line(l + 1, t + 8, r - 1, t + 8, kBoxEdge);
			line(l + 1, t + 9, r - 1, t + 9, grey(0xec));
			line(l + 2, t + 10, r - 2, t + 10, grey(0xba));
			line(l + 1, t + 10, l + 1, t + 10, grey(0xd3));
			line(r - 1, t + 10, r - 1, t + 10, grey(0x96));
			line(l + 2, t + 11, r - 1, t + 11, grey(0x92));
			line(l + 1, t + 11, l + 1, t + 11, grey(0xa8));
			break;
		}
		case GLYPH_ZOOM:
		{
			// the title bar, then the window's shaded inside
			line(l + 1, t + 1, r - 2, t + 1, grey(0xec));
			line(r - 1, t + 1, r - 1, t + 1, grey(0xcf));
			line(l + 1, t + 2, l + 1, t + 2, grey(0xea));
			line(l + 2, t + 2, r - 2, t + 2, grey(0xde));
			line(r - 1, t + 2, r - 1, t + 2, grey(0xb2));
			line(l + 1, t + 3, l + 1, t + 3, grey(0xb7));
			line(l + 2, t + 3, r - 1, t + 3, grey(0xaf));
			line(l + 1, t + 4, l + 1, t + 4, grey(0xf2));
			line(l + 2, t + 4, r - 2, t + 4, grey(0xeb));
			line(r - 1, t + 4, r - 1, t + 4, grey(0xd8));
			face(t + 5, t + 10, 0xd1, 0xba, 0x12, 0x2a);
			line(l + 1, t + 11, l + 1, t + 11, grey(0xcf));
			line(l + 2, t + 11, r - 2, t + 11, grey(0x92));
			line(r - 1, t + 11, r - 1, t + 11, grey(0x9b));
			break;
		}
		case GLYPH_CLOSE:
		{
			// the face, then a sunk square holding a small raised one
			line(l + 1, t + 1, r - 2, t + 1, grey(0xec));
			line(r - 1, t + 1, r - 1, t + 1, grey(0xcf));
			face(t + 2, t + 10, 0xdf, 0xba, 0x0a, 0x2c);
			line(l + 1, t + 11, l + 1, t + 11, grey(0xa8));
			line(l + 2, t + 11, r - 1, t + 11, grey(0x92));
			const BRect well(l + 3, t + 3, l + 9, t + 9);
			line(well.left, well.top, well.right - 1, well.top, grey(0x92));
			line(well.left, well.top, well.left, well.bottom - 1, grey(0x92));
			line(well.right, well.top, well.right, well.bottom, grey(0xec));
			line(well.left, well.bottom, well.right, well.bottom, grey(0xec));
			const BRect square(well.left + 1, well.top + 1, well.right - 1,
				well.bottom - 1);
			fDrawingEngine->StrokeRect(square, kBoxEdge);
			line(square.left + 1, square.top + 1, square.right - 1,
				square.top + 1, grey(0xbf));
			line(square.left + 1, square.top + 1, square.left + 1,
				square.bottom - 1, grey(0xbf));
			line(square.left + 2, square.top + 2, square.left + 2,
				square.top + 2, grey(0xa4));
			line(square.right - 1, square.top + 1, square.right - 1,
				square.bottom - 1, grey(0x82));
			line(square.left + 1, square.bottom - 1, square.right - 1,
				square.bottom - 1, grey(0x82));
			break;
		}
		default:
			break;
	}

	fDrawingEngine->SetCopyToFrontEnabled(copyToFront);
}


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow) QNXDecorAddOn(id, name);
}
