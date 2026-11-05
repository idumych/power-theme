/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * See PlatinumControlLook.h.
 */


#include "PlatinumControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Button.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);

// a button, from the outline in
static const rgb_color kFace = GREY(0xdd);
static const rgb_color kSoftOutline = GREY(0x22);
static const rgb_color kShade = GREY(0x77);
static const rgb_color kSoftShade = GREY(0xaa);
static const rgb_color kPressedFace = GREY(0x66);
static const rgb_color kPressedLine = GREY(0x55);
static const rgb_color kPressedLight = GREY(0x88);
static const rgb_color kDisabledOutline = GREY(0x99);

static const rgb_color kLavenderLight = { 0xcc, 0xcc, 0xff, 255 };
static const rgb_color kLavender = { 0x99, 0x99, 0xff, 255 };
static const rgb_color kLavenderShade = { 0x66, 0x66, 0xcc, 255 };
static const rgb_color kDeepBlue = { 0x33, 0x33, 0x99, 255 };

static const rgb_color kTrough = GREY(0xaa);
static const rgb_color kTroughShade = GREY(0x88);
static const rgb_color kGutter = GREY(0xbb);
static const rgb_color kFieldShade = GREY(0x88);
static const rgb_color kEtch = { 0xf9, 0xf9, 0xfe, 255 };
static const rgb_color kInactiveTab = GREY(0xcc);
static const rgb_color kHeader = { 0xd5, 0xd6, 0xd5, 255 };
static const rgb_color kHeaderShade = GREY(0xa9);
static const rgb_color kBarDark = { 0x38, 0x38, 0x9b, 255 };
static const rgb_color kBarLight = { 0xe9, 0xe9, 0xec, 255 };

static const float kButtonRadius = 2;
static const float kIndicatorWidth = 11;
static const float kTabSlope = 3;


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


/*!	A rounded rectangle set row by row, so that its corners are crisp steps. */
static void
fill_rounded(BView* view, BRect rect, float radius, const rgb_color& color)
{
	if (!rect.IsValid())
		return;

	view->SetHighColor(color);
	const int32 rows = (int32)std::min(radius, floorf((rect.Height() + 1) / 2));
	for (int32 row = 0; row < rows; row++) {
		const float inset = rows - row;
		view->FillRect(BRect(rect.left + inset, rect.top + row,
			rect.right - inset, rect.top + row));
		view->FillRect(BRect(rect.left + inset, rect.bottom - row,
			rect.right - inset, rect.bottom - row));
	}
	if (rect.top + rows <= rect.bottom - rows) {
		view->FillRect(BRect(rect.left, rect.top + rows, rect.right,
			rect.bottom - rows));
	}
}


/*!	The light along a rectangle's top and left, and the shade along its bottom and
	right, so that it stands up from what's behind it.
*/
static void
bevel(BView* view, const BRect& rect, const rgb_color& light,
	const rgb_color& shade)
{
	view->SetHighColor(light);
	view->FillRect(BRect(rect.left, rect.top, rect.right - 1, rect.top));
	view->FillRect(BRect(rect.left, rect.top, rect.left, rect.bottom - 1));
	view->SetHighColor(shade);
	view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	view->FillRect(BRect(rect.right, rect.top, rect.right, rect.bottom));
}


/*!	Two small arrows, one above the other: a Platinum pop-up menu's. */
static void
draw_updown_arrows(BView* view, const BRect& area, const rgb_color& color)
{
	const float centre = floorf((area.left + area.right) / 2);
	const float middle = floorf((area.top + area.bottom) / 2);
	view->SetHighColor(color);
	for (int32 row = 0; row < 3; row++) {
		const float half = row;
		view->FillRect(BRect(centre - half, middle - 4 + row, centre + half + 1,
			middle - 4 + row));
		view->FillRect(BRect(centre - half, middle + 5 - row, centre + half + 1,
			middle + 5 - row));
	}
}


static void
draw_arrow(BView* view, const BRect& rect, int32 direction, const rgb_color& color)
{
	view->SetHighColor(color);
	const float centreX = floorf((rect.left + rect.right) / 2);
	const float centreY = floorf((rect.top + rect.bottom) / 2);
	for (int32 step = 0; step < 4; step++) {
		const float at = step - 2;
		const float half = step;
		switch (direction) {
			case BControlLook::B_UP_ARROW:
				view->FillRect(BRect(centreX - half, centreY + at,
					centreX + half + 1, centreY + at));
				break;
			case BControlLook::B_DOWN_ARROW:
				view->FillRect(BRect(centreX - half, centreY - at + 1,
					centreX + half + 1, centreY - at + 1));
				break;
			case BControlLook::B_LEFT_ARROW:
				view->FillRect(BRect(centreX + at, centreY - half,
					centreX + at, centreY + half + 1));
				break;
			case BControlLook::B_RIGHT_ARROW:
				view->FillRect(BRect(centreX - at + 1, centreY - half,
					centreX - at + 1, centreY + half + 1));
				break;
		}
	}
}


//	#pragma mark -


PlatinumControlLook::PlatinumControlLook()
{
}


PlatinumControlLook::~PlatinumControlLook()
{
}


//	#pragma mark - buttons


void
PlatinumControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 5 : 2;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	view->PushState();
	view->ClipToRect(rect);

	// a frame that's been split carries its shape on past what's open
	BRect shape(rect);
	if ((borders & B_LEFT_BORDER) == 0)
		shape.left -= kButtonRadius + 1;
	if ((borders & B_RIGHT_BORDER) == 0)
		shape.right += kButtonRadius + 1;
	if ((borders & B_TOP_BORDER) == 0)
		shape.top -= kButtonRadius + 1;
	if ((borders & B_BOTTOM_BORDER) == 0)
		shape.bottom += kButtonRadius + 1;

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color outline = disabled ? kDisabledOutline : kSoftOutline;

	// a default button's thick ring, with the panel showing inside it
	if (isDefault) {
		fill_rounded(view, shape, kButtonRadius + 2, outline);
		fill_rounded(view, shape.InsetByCopy(2, 2), kButtonRadius, background);
		shape.InsetBy(3, 3);
	}

	fill_rounded(view, shape, kButtonRadius, outline);

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
PlatinumControlLook::DrawButtonBackground(BView* view, BRect& rect,
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
		view->SetHighColor((flags & B_ACTIVATED) != 0
			? mix(kHeader, kBlack, 0.1f) : kHeader);
		view->FillRect(rect);
		view->SetHighColor(kWhite);
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
		view->SetHighColor(kHeaderShade);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
		view->SetHighColor(kBlack);
		view->FillRect(BRect(rect.left, rect.bottom + 1, rect.right,
			rect.bottom + 1));
		view->PopState();
		return;
	}

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool pressed = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;

	if (pressed) {
		view->SetHighColor(kPressedFace);
		view->FillRect(rect);
		bevel(view, rect, kPressedLine, kPressedLight);
	} else if (disabled) {
		view->SetHighColor(kFace);
		view->FillRect(rect);
	} else {
		view->SetHighColor(kFace);
		view->FillRect(rect);
		BRect ring(rect);
		bevel(view, ring, kWhite, kShade);
		ring.InsetBy(1, 1);
		if (ring.IsValid())
			bevel(view, ring, kFace, kSoftShade);
	}

	view->PopState();

	rect.InsetBy(1, 1);
}


void
PlatinumControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_updown_arrows(view, BRect(area.right - kIndicatorWidth, area.top,
		area.right, area.bottom), (flags & B_DISABLED) != 0
			? kDisabledOutline : kBlack);
	view->PopState();
}


/*!	A pressed button goes dark, so its label goes white. */
void
PlatinumControlLook::DrawLabel(BView* view, const char* label,
	const rgb_color& base, uint32 flags, const BPoint& where,
	const rgb_color* textColor)
{
	if (textColor == NULL && (flags & B_ACTIVATED) != 0
		&& dynamic_cast<BButton*>(view) != NULL) {
		HaikuControlLook::DrawLabel(view, label, base, flags, where, &kWhite);
		return;
	}
	HaikuControlLook::DrawLabel(view, label, base, flags, where, textColor);
}


//	#pragma mark - menus


void
PlatinumControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(rect);

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() == NULL || strcmp(view->Name(), "CountVw") != 0) {
		view->SetHighColor(kWhite);
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
		view->SetHighColor(kFieldShade);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	}
	view->PopState();
}


void
PlatinumControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	DrawButtonFrame(view, rect, updateRect, base, background,
		flags & ~B_DEFAULT_BUTTON, borders);
}


void
PlatinumControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	view->SetHighColor(kFace);
	view->FillRect(rect);
	bevel(view, rect, disabled ? kFace : kWhite, disabled ? kFace : kShade);

	if (popupIndicator) {
		// the arrows' own lavender box
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth - 3;
		view->SetHighColor(disabled ? kDisabledOutline : kSoftOutline);
		view->FillRect(BRect(indicator.left, rect.top, indicator.left,
			rect.bottom));
		indicator.left++;
		view->SetHighColor(disabled ? kFace : kLavender);
		view->FillRect(indicator);
		if (!disabled)
			bevel(view, indicator, kLavenderLight, kLavenderShade);
		draw_updown_arrows(view, indicator,
			disabled ? kDisabledOutline : kBlack);
		rect.right = indicator.left - 2;
	}

	view->PopState();
}


void
PlatinumControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kFace);
	view->FillRect(rect);
	if ((flags & B_DISABLED) == 0)
		bevel(view, rect, kWhite, kShade);
	view->PopState();
}


void
PlatinumControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	view->SetHighColor(kBlack);
	if ((borders & B_LEFT_BORDER) != 0)
		view->FillRect(BRect(rect.left, rect.top, rect.left, rect.bottom));
	if ((borders & B_RIGHT_BORDER) != 0)
		view->FillRect(BRect(rect.right, rect.top, rect.right, rect.bottom));
	if ((borders & B_TOP_BORDER) != 0)
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
	if ((borders & B_BOTTOM_BORDER) != 0)
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	view->PopState();

	if ((borders & B_LEFT_BORDER) != 0)
		rect.left++;
	if ((borders & B_RIGHT_BORDER) != 0)
		rect.right--;
	if ((borders & B_TOP_BORDER) != 0)
		rect.top++;
	if ((borders & B_BOTTOM_BORDER) != 0)
		rect.bottom--;
}


void
PlatinumControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor((flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0 ? kDeepBlue : base);
	view->FillRect(rect);
	view->PopState();
}


//	#pragma mark - status bars


void
PlatinumControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	view->SetHighColor(kBlack);
	view->StrokeRect(rect);
	BRect inside = rect.InsetByCopy(1, 1);
	view->SetHighColor(kGutter);
	view->FillRect(inside);

	if (progressPosition > rect.left + 1) {
		BRect bar(inside);
		bar.right = std::min(progressPosition - 1, inside.right);
		if (bar.IsValid()) {
			// dark at the edges and pale along a band in the middle
			const bool themed = barColor == ui_color(B_STATUS_BAR_COLOR);
			const rgb_color dark = themed ? kBarDark : mix(barColor, kBlack, 0.3f);
			const rgb_color light = themed ? kBarLight : mix(barColor, kWhite, 0.7f);
			BGradientLinear gradient;
			gradient.AddColor(dark, 0);
			gradient.AddColor(light, 128);
			gradient.AddColor(dark, 255);
			gradient.SetStart(bar.LeftTop());
			gradient.SetEnd(bar.LeftBottom());
			view->FillRect(bar, gradient);
		}
	}

	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


void
PlatinumControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool pressed = (flags & B_CLICKED) != 0;
	const rgb_color line = disabled ? kDisabledOutline : kBlack;

	view->PushState();

	view->SetHighColor(line);
	view->StrokeRect(rect);
	BRect inside = rect.InsetByCopy(1, 1);
	view->SetHighColor(pressed ? kPressedLight : disabled ? kFace : kFace);
	view->FillRect(inside);
	if (!disabled && !pressed)
		bevel(view, inside, kWhite, kSoftShade);

	view->SetHighColor(line);
	const float size = rect.Width() + 1;
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		// a square in the middle
		view->FillRect(rect.InsetByCopy(roundf(size / 4), roundf(size / 4)));
	} else if ((flags & B_ACTIVATED) != 0) {
		// a tick, two pixels thick, reaching a little past the box's top
		view->SetPenSize(2);
		view->SetFlags(view->Flags());
		const float left = rect.left + roundf(size * 0.22f);
		const float right = rect.right - roundf(size * 0.16f);
		const float low = rect.bottom - roundf(size * 0.24f);
		const float mid = left + roundf((right - left) * 0.34f);
		view->StrokeLine(BPoint(left, low - (mid - left)), BPoint(mid, low));
		view->StrokeLine(BPoint(mid, low), BPoint(right, low - (right - mid)));
	}

	view->PopState();
}


void
PlatinumControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool pressed = (flags & B_CLICKED) != 0;
	const rgb_color line = disabled ? kDisabledOutline : kBlack;

	view->PushState();
	view->SetDrawingMode(B_OP_OVER);

	// a shaded ball
	BGradientLinear gradient;
	gradient.AddColor(pressed ? kPressedLight : kWhite, 0);
	gradient.AddColor(pressed ? kPressedFace : disabled ? kFace : kTrough, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	view->FillEllipse(rect, gradient);
	view->SetHighColor(line);
	view->StrokeEllipse(rect);

	if ((flags & B_ACTIVATED) != 0) {
		const float inset = roundf((rect.Width() + 1) * 0.3f);
		view->FillEllipse(rect.InsetByCopy(inset, inset));
	}

	view->PopState();
}


//	#pragma mark - scroll bars


void
PlatinumControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kBlack);
	view->StrokeRect(rect);
	view->PopState();
}


void
PlatinumControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	view->SetHighColor(down ? kPressedFace : kFace);
	view->FillRect(rect);
	if (down)
		bevel(view, rect, kPressedLine, kPressedLight);
	else if (!disabled)
		bevel(view, rect, kWhite, kGutter);

	draw_arrow(view, rect, direction, disabled ? kTrough
		: down ? kWhite : kBlack);

	view->PopState();
}


void
PlatinumControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


void
PlatinumControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;
	view->SetHighColor(disabled ? kFace : kTrough);
	view->FillRect(rect);

	if (!disabled) {
		// sunk in, with its shadow along the side the light comes from
		view->SetHighColor(kShade);
		if (orientation == B_VERTICAL)
			view->FillRect(BRect(rect.left, rect.top, rect.left, rect.bottom));
		else
			view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
		view->SetHighColor(kTroughShade);
		if (orientation == B_VERTICAL) {
			view->FillRect(BRect(rect.left + 1, rect.top, rect.left + 1,
				rect.bottom));
		} else {
			view->FillRect(BRect(rect.left, rect.top + 1, rect.right,
				rect.top + 1));
		}
	}

	view->PopState();
}


void
PlatinumControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool pressed = (flags & (B_ACTIVATED | B_HOVER)) != 0;

	view->SetHighColor(kBlack);
	view->StrokeRect(rect);
	BRect inside = rect.InsetByCopy(1, 1);
	view->SetHighColor(disabled ? kFace : pressed ? kLavenderShade : kLavender);
	view->FillRect(inside);

	if (!disabled) {
		bevel(view, inside, kLavenderLight, kLavenderShade);

		// its grip, across the way it moves
		const int32 lines = 4;
		const float gap = 2;
		if (orientation == B_VERTICAL) {
			const float middle = floorf((rect.top + rect.bottom) / 2);
			if (rect.Height() > lines * gap + 8) {
				for (int32 i = 0; i < lines; i++) {
					const float y = middle - (lines - 1) * gap / 2 + i * gap;
					view->SetHighColor(kDeepBlue);
					view->FillRect(BRect(inside.left + 2, y, inside.right - 2, y));
				}
			}
		} else {
			const float middle = floorf((rect.left + rect.right) / 2);
			if (rect.Width() > lines * gap + 8) {
				for (int32 i = 0; i < lines; i++) {
					const float x = middle - (lines - 1) * gap / 2 + i * gap;
					view->SetHighColor(kDeepBlue);
					view->FillRect(BRect(x, inside.top + 2, x, inside.bottom - 2));
				}
			}
		}
	}

	view->PopState();
}


//	#pragma mark - sliders


void
PlatinumControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// a sunk groove in the middle of what it's given
	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 1;
		bar.bottom = bar.top + 3;
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 1;
		bar.right = bar.left + 3;
	}

	bevel(view, bar, kShade, kWhite);
	view->SetHighColor(fillColor == base ? kTrough : fillColor);
	view->FillRect(bar.InsetByCopy(1, 1));

	view->PopState();
}


/*!	A lavender pointer, its tip towards the slider's marks. */
void
PlatinumControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color line = disabled ? kDisabledOutline : kBlack;
	const rgb_color face = disabled ? kFace
		: (flags & (B_HOVER | B_FOCUSED)) != 0 ? kLavenderLight : kLavender;

	view->PushState();

	if (orientation == B_HORIZONTAL) {
		const float tip = std::min(floorf((rect.Width() + 1) / 2), 4.0f);
		const float shoulder = rect.bottom - tip;
		view->SetHighColor(line);
		view->FillRect(BRect(rect.left, rect.top, rect.right, shoulder));
		for (int32 row = 1; row <= tip; row++) {
			view->FillRect(BRect(rect.left + row, shoulder + row, rect.right - row,
				shoulder + row));
		}
		view->SetHighColor(face);
		view->FillRect(BRect(rect.left + 1, rect.top + 1, rect.right - 1,
			shoulder));
		for (int32 row = 1; row < tip; row++) {
			view->FillRect(BRect(rect.left + row + 1, shoulder + row,
				rect.right - row - 1, shoulder + row));
		}
		if (!disabled) {
			view->SetHighColor(kLavenderLight);
			view->FillRect(BRect(rect.left + 1, rect.top + 1, rect.right - 1,
				rect.top + 1));
			view->FillRect(BRect(rect.left + 1, rect.top + 1, rect.left + 1,
				shoulder));
		}
	} else {
		const float tip = std::min(floorf((rect.Height() + 1) / 2), 4.0f);
		const float shoulder = rect.right - tip;
		view->SetHighColor(line);
		view->FillRect(BRect(rect.left, rect.top, shoulder, rect.bottom));
		for (int32 column = 1; column <= tip; column++) {
			view->FillRect(BRect(shoulder + column, rect.top + column,
				shoulder + column, rect.bottom - column));
		}
		view->SetHighColor(face);
		view->FillRect(BRect(rect.left + 1, rect.top + 1, shoulder,
			rect.bottom - 1));
		for (int32 column = 1; column < tip; column++) {
			view->FillRect(BRect(shoulder + column, rect.top + column + 1,
				shoulder + column, rect.bottom - column - 1));
		}
	}

	view->PopState();
}


//	#pragma mark - text controls


void
PlatinumControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();

	// sunk in, and lavender when it has the focus
	if ((flags & B_FOCUSED) != 0) {
		view->SetHighColor(kLavenderShade);
		view->StrokeRect(rect);
	} else
		bevel(view, rect, kFieldShade, kWhite);
	view->SetHighColor((flags & B_DISABLED) != 0 ? kDisabledOutline : kBlack);
	view->StrokeRect(rect.InsetByCopy(1, 1));

	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


/*!	An etched line: dark, with a pale one inside it. */
void
PlatinumControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kFieldShade);
	view->StrokeRect(rect);
	view->SetHighColor(kEtch);
	view->StrokeRect(rect.InsetByCopy(1, 1));
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - tabs


void
PlatinumControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawTabFrame(view, rect, updateRect, base, flags,
			borders, borderStyle, side);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	if (borderStyle != B_NO_BORDER) {
		view->SetHighColor(kBlack);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}
	view->PopState();
}


/*!	A tab's shape, narrowing towards the top. */
static void
draw_tab(BView* view, const BRect& rect, float top, const rgb_color& fill)
{
	const float height = rect.bottom - top;
	view->SetHighColor(kBlack);
	view->FillRect(BRect(rect.left + kTabSlope + 1, top, rect.right - kTabSlope - 1,
		top));
	for (float y = top + 1; y <= rect.bottom; y++) {
		const float inset = kTabSlope - floorf((y - top) * kTabSlope / height);
		view->SetHighColor(kBlack);
		view->FillRect(BRect(rect.left + inset, y, rect.right - inset, y));
		view->SetHighColor(fill);
		view->FillRect(BRect(rect.left + inset + 1, y, rect.right - inset - 1, y));
	}
}


void
PlatinumControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawActiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(base);
	view->FillRect(rect);

	// joined to the page under it
	draw_tab(view, rect, rect.top, base);
	const float inner = rect.top + 1;
	view->SetHighColor(kWhite);
	view->FillRect(BRect(rect.left + kTabSlope + 1, inner, rect.right - kTabSlope - 1,
		inner));
	view->PopState();

	rect.left += 3;
	rect.right -= 3;
	rect.top += 3;
	rect.bottom -= 1;
}


void
PlatinumControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawInactiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	// a little lower than the active one, and behind the page's line
	view->SetHighColor(base);
	view->FillRect(rect);
	draw_tab(view, rect, rect.top + 2, kInactiveTab);
	view->SetHighColor(kBlack);
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());

	view->PopState();

	rect.left += 2;
	rect.right -= 2;
	rect.top += 4;
	rect.bottom -= 2;
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::PlatinumControlLook();
}
