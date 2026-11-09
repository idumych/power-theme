/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See QNXControlLook.h. Colours and sizes were measured from screenshots of
 * QNX 6.2.1's Photon microGUI with its standard colours, on toastytech.com.
 */


#include "QNXControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Button.h>
#include <InterfaceDefs.h>
#include <GradientLinear.h>
#include <Menu.h>
#include <MenuBar.h>
#include <Rect.h>
#include <AffineTransform.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);
// the dark grey outline round nearly everything
static const rgb_color kOutline = GREY(0x4b);
static const rgb_color kDisabledOutline = GREY(0x9a);
// the panel round a button or field, sunk a little: darker along its top and
// left, lighter along its bottom and right
static const rgb_color kSinkOuter = GREY(0xc0);
static const rgb_color kSinkInner = GREY(0xb1);
static const rgb_color kSinkLight = GREY(0xdd);
// a button's face, shaded from its top to its bottom, and its lines
static const rgb_color kFaceTop = GREY(0xeb);
static const rgb_color kFaceBottom = GREY(0xc6);
static const rgb_color kFaceShade = GREY(0xb0);
static const rgb_color kPressedTop = GREY(0xb8);
static const rgb_color kPressedBottom = GREY(0xd4);
// a field's inside, and the line along its top and left
static const rgb_color kFieldShade = GREY(0xc0);
// check boxes and radio buttons
static const rgb_color kBoxEdge = GREY(0x3b);
static const rgb_color kBoxOuter = GREY(0xad);
static const rgb_color kBoxCorner = GREY(0xcc);
static const rgb_color kMark = GREY(0x18);
static const rgb_color kMarkSoft = GREY(0x99);
static const rgb_color kBallTop = GREY(0xe0);
static const rgb_color kBallBottom = GREY(0xc2);
static const rgb_color kDot = GREY(0x4a);
// scroll bars
static const rgb_color kTrack = GREY(0xb5);
static const rgb_color kTrackShade = GREY(0xa1);
static const rgb_color kThumbFace = GREY(0xd5);
static const rgb_color kArrow = GREY(0x33);
static const rgb_color kArrowDisabled = GREY(0x85);
// menus: their face, the bands of the items, and the line inside the edge
static const rgb_color kMenuLight = GREY(0xf1);
static const rgb_color kItemBand = GREY(0xcc);
// a menu bar, shaded from its top to its bottom
static const rgb_color kBarTop = GREY(0xcd);
static const rgb_color kBarBottom = GREY(0xdb);
// grooves round groups and along a slider
static const rgb_color kEtchDark = GREY(0xc4);
static const rgb_color kEtchLight = GREY(0xec);
static const rgb_color kGrooveDark = GREY(0x8c);
static const rgb_color kGrooveFace = GREY(0xac);
static const rgb_color kSliderTop = GREY(0xed);
static const rgb_color kSliderBottom = GREY(0xc4);
// tabs: the chosen one's highlight, and the others' face and highlight
static const rgb_color kTabLight = GREY(0xec);
static const rgb_color kTabBack = GREY(0xc7);
static const rgb_color kTabBackLight = GREY(0xdb);
static const rgb_color kTabBackText = GREY(0x63);

// the groove before a pop-up's arrow, a check box's light edge and its
// inside held down, and a radio button's edge
static const rgb_color kGroove = GREY(0xa2);
static const rgb_color kBoxLight = GREY(0xf4);
static const rgb_color kBoxDown = GREY(0xd8);
static const rgb_color kBallEdge = GREY(0x3c);

static const float kIndicatorWidth = 20;


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	rgb_color color)
{
	if (left > right || top > bottom)
		return;
	view->SetHighColor(color);
	view->FillRect(BRect(left, top, right, bottom));
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


//!	\a rect shaded from \a top to \a bottom, along \a orientation.
static void
fill_shade(BView* view, const BRect& rect, rgb_color top, rgb_color bottom,
	orientation orientation = B_HORIZONTAL)
{
	if (!rect.IsValid())
		return;
	if (orientation == B_HORIZONTAL) {
		const float span = std::max(1.0f, rect.Height());
		for (float y = rect.top; y <= rect.bottom; y++) {
			fill_line(view, rect.left, y, rect.right, y,
				mix(top, bottom, (y - rect.top) / span));
		}
	} else {
		const float span = std::max(1.0f, rect.Width());
		for (float x = rect.left; x <= rect.right; x++) {
			fill_line(view, x, rect.top, x, rect.bottom,
				mix(top, bottom, (x - rect.left) / span));
		}
	}
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED | BControlLook::B_CLICKED)) != 0
		&& (flags & BControlLook::B_DISABLED) == 0;
}


static rgb_color
outline_color(uint32 flags)
{
	return (flags & BControlLook::B_DISABLED) != 0 ? kDisabledOutline
		: kOutline;
}


/*!	A button's face, inside its outline: white along its top and left, shaded
	along its bottom and right, and between them shaded from light to darker.
	Pressed, the shade turns over and the lines change places.
*/
static void
draw_face(BView* view, const BRect& r, uint32 flags,
	orientation orientation = B_HORIZONTAL)
{
	if (!r.IsValid())
		return;
	const bool pressed = is_pressed(flags);
	fill_shade(view, r, pressed ? kPressedTop : kFaceTop,
		pressed ? kPressedBottom : kFaceBottom, orientation);
	fill_line(view, r.left, r.top, r.right - 1, r.top,
		pressed ? kFaceShade : kWhite);
	fill_line(view, r.left, r.top, r.left, r.bottom - 1,
		pressed ? kFaceShade : kWhite);
	fill_line(view, r.left + 1, r.bottom, r.right, r.bottom,
		pressed ? kWhite : kFaceShade);
	fill_line(view, r.right, r.top + 1, r.right, r.bottom,
		pressed ? kWhite : kFaceShade);
}


//!	The sunk panel round a button or field, a pixel wide.
static void
draw_sink(BView* view, const BRect& r)
{
	fill_line(view, r.left, r.top, r.right, r.top, kSinkInner);
	fill_line(view, r.left, r.top, r.left, r.bottom, kSinkInner);
	fill_line(view, r.left + 1, r.bottom, r.right, r.bottom, kSinkLight);
	fill_line(view, r.right, r.top + 1, r.right, r.bottom, kSinkLight);
}


/*!	A small solid triangle, its point towards \a direction, \a half wide
	across and \a tall along.
*/
static void
draw_triangle(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color, float half, float tall)
{
	const float x = floorf((rect.left + rect.right) / 2);
	const float y = floorf((rect.top + rect.bottom) / 2);
	view->PushState();
	view->SetHighColor(color);
	for (int32 i = 0; i < tall; i++) {
		const float reach = half - i * half / std::max(1.0f, tall - 1);
		const float w = roundf(reach);
		const float along = i - floorf(tall / 2);
		switch (direction) {
			case BControlLook::B_UP_ARROW:
				view->FillRect(BRect(x - w, y - along, x + w, y - along));
				break;
			case BControlLook::B_LEFT_ARROW:
				view->FillRect(BRect(x - along, y - w, x - along, y + w));
				break;
			case BControlLook::B_RIGHT_ARROW:
				view->FillRect(BRect(x + along, y - w, x + along, y + w));
				break;
			default:
				view->FillRect(BRect(x - w, y + along, x + w, y + along));
				break;
		}
	}
	view->PopState();
}


/*!	A scroll arrow as Photon drew it: a solid arrowhead on a short, wide stem,
	pointing \a direction.
*/
static void
draw_arrow(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color)
{
	const float x = floorf((rect.left + rect.right) / 2);
	const float y = floorf((rect.top + rect.bottom) / 2);
	view->PushState();
	view->SetHighColor(color);
	// the head, four rows from a point to seven wide, then a stem three wide
	// and two long
	for (int32 i = 0; i < 4; i++) {
		const float along = -3 + i;
		switch (direction) {
			case BControlLook::B_UP_ARROW:
				view->FillRect(BRect(x - i, y + along, x + i, y + along));
				break;
			case BControlLook::B_DOWN_ARROW:
				view->FillRect(BRect(x - i, y - along, x + i, y - along));
				break;
			case BControlLook::B_LEFT_ARROW:
				view->FillRect(BRect(x + along, y - i, x + along, y + i));
				break;
			default:
				view->FillRect(BRect(x - along, y - i, x - along, y + i));
				break;
		}
	}
	switch (direction) {
		case BControlLook::B_UP_ARROW:
			view->FillRect(BRect(x - 1, y + 1, x + 1, y + 2));
			break;
		case BControlLook::B_DOWN_ARROW:
			view->FillRect(BRect(x - 1, y - 2, x + 1, y - 1));
			break;
		case BControlLook::B_LEFT_ARROW:
			view->FillRect(BRect(x + 1, y - 1, x + 2, y + 1));
			break;
		default:
			view->FillRect(BRect(x - 2, y - 1, x - 1, y + 1));
			break;
	}
	view->PopState();
}


//	#pragma mark -


QNXControlLook::QNXControlLook()
{
}


QNXControlLook::~QNXControlLook()
{
}


//	#pragma mark - buttons


/*!	The button's sunk panel and its dark outline; the background draws the
	face. A default button's outline is two pixels wide.
*/
void
QNXControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 4 : 3;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	BRect r(rect);
	fill_line(view, r.left, r.top, r.right, r.top, kSinkOuter);
	fill_line(view, r.left, r.top, r.left, r.bottom, kSinkOuter);
	fill_line(view, r.left + 1, r.bottom, r.right, r.bottom, background);
	fill_line(view, r.right, r.top + 1, r.right, r.bottom, background);
	r.InsetBy(1, 1);
	draw_sink(view, r);
	r.InsetBy(1, 1);
	view->SetHighColor(outline_color(flags));
	view->StrokeRect(r);
	if (isDefault)
		view->StrokeRect(r.InsetByCopy(1, 1));

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
QNXControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(1, 1);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	// Tracker's column headers are drawn as a button's background; the view's
	// name is the only thing that tells them from any other button
	if (view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0) {
		draw_face(view, BRect(rect.left, rect.top, rect.right,
			rect.bottom - 1), flags & ~(B_ACTIVATED | B_CLICKED));
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kOutline);
		view->PopState();
		return;
	}

	// a menu's items drawn as buttons, as Deskbar's are, have no frame
	if (dynamic_cast<BMenu*>(view) != NULL) {
		view->SetHighColor(kOutline);
		view->StrokeRect(rect);
		draw_face(view, rect.InsetByCopy(1, 1), flags);
		view->PopState();
		rect.InsetBy(2, 2);
		return;
	}

	draw_face(view, rect, flags, orientation);

	view->PopState();

	rect.InsetBy(1, 1);
}


void
QNXControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_triangle(view, BRect(area.right - 12, area.top, area.right - 2,
		area.bottom), B_DOWN_ARROW,
		(flags & B_DISABLED) != 0 ? kArrowDisabled : kBlack, 3, 4);
	view->PopState();
}


//!	A tab behind the chosen one has a grey label.
void
QNXControlLook::DrawLabel(BView* view, const char* label,
	const BBitmap* icon, BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, const BAlignment& alignment,
	const rgb_color* textColor)
{
	BTabView* tabView = dynamic_cast<BTabView*>(view);
	if (tabView != NULL && (flags & B_DISABLED) == 0) {
		// BTab draws its label in the tab view, turned for tabs on the left
		// or right, so its middle is the place to look
		const BPoint middle((rect.left + rect.right) / 2,
			(rect.top + rect.bottom) / 2);
		const BPoint where = view->Transform().Apply(middle);
		for (int32 i = 0; i < tabView->CountTabs(); i++) {
			if (i != tabView->Selection()
				&& tabView->TabFrame(i).Contains(where)) {
				HaikuControlLook::DrawLabel(view, label, icon, rect,
					updateRect, base, flags, alignment, &kTabBackText);
				return;
			}
		}
	}
	HaikuControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);
}


//	#pragma mark - menus


//!	A grey bar, white along its top and shaded slightly lighter downwards.
void
QNXControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		view->PopState();
		return;
	}

	fill_shade(view, rect, kBarTop, kBarBottom);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kWhite);
	if ((borders & B_BOTTOM_BORDER) != 0) {
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kEtchDark);
	}

	view->PopState();
}


/*!	The pop-up's sunk panel and dark outline. Haiku draws the field's
	background in its menu bar, a view two pixels inside this rectangle.
*/
void
QNXControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		draw_sink(view, rect);
		view->SetHighColor(outline_color(flags));
		view->StrokeRect(rect.InsetByCopy(1, 1));
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


/*!	A button's face, and at its right, past a groove, an arrow pointing down
	over a bar.
*/
void
QNXControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	draw_face(view, rect, flags & ~(B_ACTIVATED | B_CLICKED));

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 1;
		fill_line(view, indicator.left, rect.top + 1, indicator.left,
			rect.bottom - 1, kGroove);
		fill_line(view, indicator.left + 1, rect.top + 1, indicator.left + 1,
			rect.bottom - 1, kWhite);
		const rgb_color mark = (flags & B_DISABLED) != 0 ? kArrowDisabled
			: kBlack;
		BRect head(indicator.left + 2, rect.top, indicator.right, rect.bottom);
		head.OffsetBy(0, -1);
		draw_triangle(view, head, B_DOWN_ARROW, mark, 3, 4);
		const float x = floorf((head.left + head.right) / 2);
		const float y = floorf((head.top + head.bottom) / 2) + 4;
		fill_line(view, x - 3, y, x + 3, y, mark);
		rect.right = indicator.left - 1;
	}

	view->PopState();
}


void
QNXControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


/*!	A grey menu with a light line along its top and left; the window's
	border is the black line round it.
*/
void
QNXControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if ((borders & B_LEFT_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kMenuLight);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kMenuLight);
	view->PopState();

	if ((borders & B_LEFT_BORDER) != 0)
		rect.left++;
	if ((borders & B_TOP_BORDER) != 0)
		rect.top++;
}


/*!	Each item a darker band, apart from the next by a little of the menu's
	grey; the chosen one in the menu selection colour. In a menu bar, only the
	chosen item has a band.
*/
void
QNXControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	const bool inBar = dynamic_cast<BMenuBar*>(view) != NULL;
	if (!inBar)
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	BRect band(rect.InsetByCopy(inBar ? 0 : 1, 1));
	if (selected) {
		fill_line(view, band.left, band.top, band.right, band.bottom,
			ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
	} else if (!inBar)
		fill_line(view, band.left, band.top, band.right, band.bottom, kItemBand);
	view->PopState();
}


//	#pragma mark - status bars


//!	A dark outline round a grey track, shaded along its top, and the bar in it.
void
QNXControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(kOutline);
	view->StrokeRect(rect);
	const BRect well(rect.InsetByCopy(1, 1));
	fill_line(view, well.left, well.top, well.right, well.bottom, kTrack);
	fill_line(view, well.left, well.top, well.right, well.top, kTrackShade);
	fill_line(view, well.left, well.top, well.left, well.bottom, kTrackShade);
	if (progressPosition > well.left) {
		const BRect bar(well.left, well.top,
			std::min(progressPosition, well.right), well.bottom);
		fill_line(view, bar.left, bar.top, bar.right, bar.bottom, barColor);
		fill_line(view, bar.left, bar.top, bar.right, bar.top,
			tint_color(barColor, B_LIGHTEN_1_TINT));
	}
	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	A white box with a dark edge, sunk into the panel, its inside corners a
	little grey, and a cross when it's on. Held down, the inside is grey.
*/
void
QNXControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0 && !disabled;
	const rgb_color edge = disabled ? kDisabledOutline : kBoxEdge;
	const rgb_color mark = disabled ? kDisabledOutline : kMark;

	view->PushState();

	BRect r(rect);
	r.bottom = r.top + r.Width();
	fill_line(view, r.left, r.top, r.right, r.top, kBoxOuter);
	fill_line(view, r.left, r.top, r.left, r.bottom, kBoxOuter);
	fill_line(view, r.left + 1, r.bottom, r.right, r.bottom, kBoxLight);
	fill_line(view, r.right, r.top + 1, r.right, r.bottom, kBoxLight);
	BRect box(r.InsetByCopy(1, 1));
	view->SetHighColor(edge);
	view->StrokeRect(box);
	const BRect inside(box.InsetByCopy(1, 1));
	fill_line(view, inside.left, inside.top, inside.right, inside.bottom,
		down ? kBoxDown : kWhite);
	fill_line(view, inside.left, inside.top, inside.left, inside.top,
		kBoxCorner);
	fill_line(view, inside.right, inside.top, inside.right, inside.top,
		kBoxCorner);
	fill_line(view, inside.left, inside.bottom, inside.left, inside.bottom,
		kBoxCorner);
	fill_line(view, inside.right, inside.bottom, inside.right, inside.bottom,
		kBoxCorner);

	const BRect cross(inside.InsetByCopy(2, 2));
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf((cross.top + cross.bottom) / 2);
		fill_line(view, cross.left, middle, cross.right, middle + 1, mark);
	} else if ((flags & B_ACTIVATED) != 0 && cross.IsValid()) {
		// a pixel and a half wide: a dark line with a soft one beside it
		view->SetHighColor(kMarkSoft);
		view->StrokeLine(BPoint(cross.left + 1, cross.top),
			BPoint(cross.right, cross.bottom - 1));
		view->StrokeLine(BPoint(cross.right - 1, cross.top),
			BPoint(cross.left, cross.bottom - 1));
		view->SetHighColor(mark);
		view->StrokeLine(cross.LeftTop(), cross.RightBottom());
		view->StrokeLine(cross.RightTop(), cross.LeftBottom());
	}
	view->PopState();
}


/*!	A ball shaded from light at its top left to darker at its bottom right,
	with a dark edge, and a dark dot in it when it's on.
*/
void
QNXControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0 && !disabled;

	BRect ball(rect);
	ball.bottom = ball.top + ball.Width();

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(kBoxLight);
	view->FillEllipse(ball.OffsetByCopy(1, 1));
	view->SetHighColor(disabled ? kDisabledOutline : kBallEdge);
	view->FillEllipse(ball);

	BGradientLinear gradient(ball.LeftTop(), ball.RightBottom());
	gradient.AddColor(down ? kBallBottom : kBallTop, 0);
	gradient.AddColor(down ? kBallTop : kBallBottom, 255);
	view->FillEllipse(ball.InsetByCopy(1, 1), gradient);

	if ((flags & B_ACTIVATED) != 0) {
		view->SetHighColor(disabled ? kDisabledOutline : kDot);
		const float inset = roundf((ball.Width() + 1) * 0.3f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
	}
	view->PopState();
}


//	#pragma mark - scroll bars


void
QNXControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kOutline);
	view->StrokeRect(rect);
	view->PopState();
}


/*!	A button's face with a dark arrow, grey when there is nothing to scroll,
	and a dark line between it and the track.
*/
void
QNXControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	switch (direction) {
		case B_UP_ARROW:
			fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
				kOutline);
			rect.bottom--;
			break;
		case B_DOWN_ARROW:
			fill_line(view, rect.left, rect.top, rect.right, rect.top, kOutline);
			rect.top++;
			break;
		case B_LEFT_ARROW:
			fill_line(view, rect.right, rect.top, rect.right, rect.bottom,
				kOutline);
			rect.right--;
			break;
		case B_RIGHT_ARROW:
			fill_line(view, rect.left, rect.top, rect.left, rect.bottom,
				kOutline);
			rect.left++;
			break;
	}
	draw_face(view, rect, down && !disabled ? B_ACTIVATED : 0);
	BRect glyph(rect);
	if (down && !disabled)
		glyph.OffsetBy(1, 1);
	draw_arrow(view, glyph, direction, disabled ? kArrowDisabled : kArrow);

	view->PopState();
}


void
QNXControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


//!	A grey track, shaded along its left or top.
void
QNXControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, kTrack);
	if (orientation == B_VERTICAL)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kTrackShade);
	else
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kTrackShade);
	view->PopState();
}


/*!	A light thumb, white along its top and left and shaded along its bottom
	and right, with dark lines across its ends. A bar with nothing to scroll
	has none.
*/
void
QNXControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	if ((flags & B_DISABLED) != 0) {
		BRect track(rect);
		DrawScrollBarBackground(view, track, updateRect, base, flags,
			orientation);
		view->PopState();
		return;
	}

	// the dark lines along the track's sides are the bar's border; the
	// thumb has its own along its ends
	BRect t(rect);
	if (orientation == B_VERTICAL) {
		t.InsetBy(-1, 0);
		fill_line(view, t.left, t.top, t.right, t.top, kOutline);
		fill_line(view, t.left, t.bottom, t.right, t.bottom, kOutline);
	} else {
		t.InsetBy(0, -1);
		fill_line(view, t.left, t.top, t.left, t.bottom, kOutline);
		fill_line(view, t.right, t.top, t.right, t.bottom, kOutline);
	}
	const BRect f(t.InsetByCopy(1, 1));
	fill_line(view, f.left, f.top, f.right, f.bottom, kThumbFace);
	fill_line(view, f.left, f.top, f.right - 1, f.top, kWhite);
	fill_line(view, f.left, f.top, f.left, f.bottom - 1, kWhite);
	fill_line(view, f.left + 1, f.bottom, f.right, f.bottom, kFaceShade);
	fill_line(view, f.right, f.top + 1, f.right, f.bottom, kFaceShade);

	view->PopState();
}


float
QNXControlLook::GetScrollBarWidth(orientation orientation)
{
	return 17;
}


//	#pragma mark - sliders


/*!	A thin groove: a dark line, a darker one and grey inside, with a light
	line along its bottom.
*/
void
QNXControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 2;
		bar.bottom = bar.top + 4;
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 2;
		bar.right = bar.left + 4;
	}
	// an application's fill colour, when it has one; Haiku passes a grey tint
	// of the panel for the part with none
	const bool isGrey = fillColor.red == fillColor.green
		&& fillColor.green == fillColor.blue;
	fill_line(view, bar.left, bar.top, bar.right, bar.bottom,
		!isGrey && (flags & B_DISABLED) == 0 ? fillColor : kGrooveFace);
	if (orientation == B_HORIZONTAL) {
		fill_line(view, bar.left, bar.top, bar.right, bar.top, kGrooveDark);
		fill_line(view, bar.left, bar.bottom - 1, bar.right, bar.bottom - 1,
			outline_color(flags));
		fill_line(view, bar.left, bar.bottom, bar.right, bar.bottom, kEtchLight);
	} else {
		fill_line(view, bar.left, bar.top, bar.left, bar.bottom, kGrooveDark);
		fill_line(view, bar.right - 1, bar.top, bar.right - 1, bar.bottom,
			outline_color(flags));
		fill_line(view, bar.right, bar.top, bar.right, bar.bottom, kEtchLight);
	}

	view->PopState();
}


//!	A small button: a dark outline round a face shaded from light to darker.
void
QNXControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(outline_color(flags));
	view->StrokeRect(rect);
	const BRect face(rect.InsetByCopy(1, 1));
	fill_shade(view, face, kSliderTop, kSliderBottom,
		orientation == B_HORIZONTAL ? B_HORIZONTAL : B_VERTICAL);
	fill_line(view, face.left, face.top, face.left, face.bottom, kWhite);
	fill_line(view, face.left, face.top, face.right, face.top, kWhite);
	view->PopState();
}


//!	The thumb, as Photon's sliders had no pointer.
void
QNXControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	DrawSliderThumb(view, rect, updateRect, base, flags, orientation);
}


//	#pragma mark - text controls


/*!	The field's sunk panel, its dark outline, and a grey line inside along its
	top and left; with the focus, that line is the keyboard navigation colour.
*/
void
QNXControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(3, 3);
		return;
	}

	view->PushState();
	const bool disabled = (flags & B_DISABLED) != 0;
	const bool focused = (flags & B_FOCUSED) != 0 && !disabled;
	draw_sink(view, rect);
	view->SetHighColor(outline_color(flags));
	view->StrokeRect(rect.InsetByCopy(1, 1));
	const BRect inside(rect.InsetByCopy(2, 2));
	const rgb_color line = focused ? ui_color(B_KEYBOARD_NAVIGATION_COLOR)
		: kFieldShade;
	if (focused) {
		view->SetHighColor(line);
		view->StrokeRect(inside);
	} else {
		fill_line(view, inside.left, inside.top, inside.right, inside.top, line);
		fill_line(view, inside.left, inside.top, inside.left, inside.bottom,
			line);
		fill_line(view, inside.left + 1, inside.bottom, inside.right,
			inside.bottom, ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fill_line(view, inside.right, inside.top + 1, inside.right,
			inside.bottom, ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	}
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - groups


//!	An etched line round the group: grey, with a light line beside it.
void
QNXControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kEtchLight);
	view->StrokeRect(BRect(rect.left + 1, rect.top + 1, rect.right,
		rect.bottom));
	view->SetHighColor(kEtchDark);
	view->StrokeRect(BRect(rect.left, rect.top, rect.right - 1,
		rect.bottom - 1));
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - tabs


//!	The page's edge on the tabs' side, as the rectangle's row or column.
static BRect
page_edge(const BRect& r, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			return BRect(r.left, r.top, r.right, r.top);
		case BTabView::kLeftSide:
			return BRect(r.right, r.top, r.right, r.bottom);
		case BTabView::kRightSide:
			return BRect(r.left, r.top, r.left, r.bottom);
		default:
			return BRect(r.left, r.bottom, r.right, r.bottom);
	}
}


/*!	Turns a point given for a tab on top, \a along its row from its start and
	\a across from the page towards its far edge, into the view's coordinates
	for a tab on \a side of \a rect.
*/
static BPoint
tab_point(const BRect& rect, uint32 side, float along, float across)
{
	switch (side) {
		case BTabView::kBottomSide:
			return BPoint(rect.left + along, rect.top + across);
		case BTabView::kLeftSide:
			return BPoint(rect.right - across, rect.top + along);
		case BTabView::kRightSide:
			return BPoint(rect.left + across, rect.top + along);
		default:
			return BPoint(rect.left + along, rect.bottom - across);
	}
}


/*!	A tab as Photon drew it: a dark outline, straight up at its start with the
	corner there cut by a pixel, along its far edge, and slanting back down to
	the page at its end, a pixel for each row, so the slant is as wide as the
	tab is deep; a light line inside along its start, its far edge and its
	slant. Behind the chosen tab, a tab is as deep but darker, and sits on the
	page's line.
*/
static void
draw_tab(BView* view, const BRect& rect, uint32 side, bool open)
{
	const bool across = side == BTabView::kLeftSide
		|| side == BTabView::kRightSide;
	const float length = across ? rect.Height() + 1 : rect.Width() + 1;
	// from the page's line to the far edge
	const float top = across ? rect.Width() : rect.Height();
	const float slant = std::max(0.0f, std::min(top, length - 4));
	const float end = length - 1 - slant;
	const rgb_color face = open ? ui_color(B_PANEL_BACKGROUND_COLOR)
		: kTabBack;
	const rgb_color light = open ? kTabLight : kTabBackLight;
	auto at = [&](float along, float out) {
		return tab_point(rect, side, along, out);
	};

	// pixel by pixel, so that the slant stays as crisp as Photon's: a run
	// along the tab, \a out from the page, from \a from to \a to
	auto run = [&](float out, float from, float to, rgb_color color) {
		if (from > to)
			return;
		const BPoint a = at(from, out), b = at(to, out);
		fill_line(view, std::min(a.x, b.x), std::min(a.y, b.y),
			std::max(a.x, b.x), std::max(a.y, b.y), color);
	};

	view->PushState();
	view->ClipToRect(rect);
	for (float out = 0; out <= top; out++) {
		// how far along the slant is at this row
		const float reach = end + (top - out);
		if (out == top) {
			run(out, 1, end, kOutline);
			continue;
		}
		run(out, 0, 0, kOutline);
		if (out == top - 1) {
			run(out, 1, 1, kOutline);
			run(out, 2, reach - 1, light);
		} else {
			run(out, 1, 1, light);
			run(out, 2, reach - 2, face);
			run(out, reach - 1, reach - 1, light);
		}
		run(out, reach, reach, kOutline);
	}
	view->PopState();

	if (!open) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom, kOutline);
	}
}


//!	Where a tab's label goes: inside its outline, clear of its slant.
static void
inset_tab(BRect& rect, uint32 side)
{
	// the label may run under the slant's upper half, as Photon's did
	const float slant = floorf(((side == BTabView::kLeftSide
		|| side == BTabView::kRightSide) ? rect.Width() : rect.Height()) / 2);
	switch (side) {
		case BTabView::kBottomSide:
			rect.top += 1;
			rect.bottom -= 2;
			rect.left += 3;
			rect.right -= slant - 2;
			break;
		case BTabView::kLeftSide:
			rect.left += 2;
			rect.right -= 1;
			rect.top += 3;
			rect.bottom -= slant - 2;
			break;
		case BTabView::kRightSide:
			rect.left += 1;
			rect.right -= 2;
			rect.top += 3;
			rect.bottom -= slant - 2;
			break;
		default:
			rect.top += 2;
			rect.bottom -= 1;
			rect.left += 3;
			rect.right -= slant - 2;
			break;
	}
}


//!	A dark line along the page's edge that the tabs sit on.
void
QNXControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if (borderStyle != B_NO_BORDER) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom, kOutline);
	}
	view->PopState();
}


void
QNXControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, true);
		view->PopState();
	}

	inset_tab(rect, side);
}


void
QNXControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, false);
		view->PopState();
	}

	inset_tab(rect, side);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::QNXControlLook();
}
