/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * See ExposeControlLook.h. Colours and sizes were measured from the artwork.
 */


#include "ExposeControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Bitmap.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>

#include "ExposeMarks.h"


namespace BPrivate {


static const rgb_color kWhite = { 255, 255, 255, 255 };
static const rgb_color kBlack = { 0, 0, 0, 255 };


#define GREY(level) { level, level, level, 255 }


/*!	What differs between the two Kvantum themes that's only a colour. */
struct Palette {
	rgb_color	buttonOutline;
	rgb_color	editLine;
	rgb_color	editFocusLine;
	rgb_color	menuLine;
	rgb_color	groupLine;
	rgb_color	headerFill;
	rgb_color	headerLine;
	rgb_color	arrow;
	rgb_color	scrollEdge;
	rgb_color	thumbLight;
	rgb_color	thumbDark;
	rgb_color	thumbHoverLight;
	rgb_color	thumbHoverDark;
	rgb_color	troughLight;
	rgb_color	troughDark;
	rgb_color	tabPageLine;
	rgb_color	activeTabLine;
	rgb_color	inactiveTabLine;
	rgb_color	sliderLine;
	rgb_color	sliderFill;
};


static const Palette kLightPalette = {
	GREY(0x70), GREY(0x9e), { 0xf7, 0xba, 0x63, 255 }, GREY(0x66), GREY(0x9e),
	GREY(0xdb), GREY(0xb6), kBlack, GREY(0x9b), GREY(0xf6), GREY(0xd6),
	GREY(0xf7), GREY(0xde), GREY(0xf8), GREY(0xe9), GREY(0x9d),
	{ 0xde, 0x79, 0x21, 255 }, GREY(0xad), GREY(0xad), GREY(0xd1)
};

static const Palette kDarkPalette = {
	GREY(0x1b), { 0x7b, 0x82, 0x88, 255 }, { 0x98, 0xa0, 0xa8, 255 },
	{ 0x50, 0x51, 0x4f, 255 }, { 0x50, 0x51, 0x4f, 255 },
	{ 0x2d, 0x2e, 0x2c, 255 }, { 0x5d, 0x5e, 0x5d, 255 },
	{ 0xd0, 0xd1, 0xce, 255 }, GREY(0x33), GREY(0x68), GREY(0x3a), GREY(0x74),
	GREY(0x40), GREY(0x68), GREY(0x3a), { 0x6e, 0x6f, 0x6d, 255 },
	{ 0xa8, 0xa9, 0xa6, 255 }, { 0x5d, 0x5e, 0x5d, 255 }, kBlack, GREY(0x33)
};


// Expose Dark: the white rim outside a button's outline, its shading, and the
// amber of a default button
static const rgb_color kRim = GREY(0xf0);
static const rgb_color kInnerTop = GREY(0xe0);
static const rgb_color kInnerBottom = GREY(0xae);
static const rgb_color kInnerHoverTop = GREY(0xf9);
static const rgb_color kInnerHoverBottom = GREY(0xc0);
static const rgb_color kInnerPressed = GREY(0xae);
static const rgb_color kPressedFill = GREY(0xbe);
static const rgb_color kAmberOuterTop = { 0xf7, 0xeb, 0xa5, 255 };
static const rgb_color kAmberOuterBottom = { 0xde, 0x79, 0x21, 255 };
static const rgb_color kAmberInnerTop = { 0xef, 0xcf, 0x84, 255 };
static const rgb_color kAmberInnerBottom = { 0xf7, 0xba, 0x63, 255 };
static const rgb_color kLightSelected = GREY(0xbd);
static const rgb_color kBarEdge = { 0x76, 0xb6, 0x76, 255 };
static const rgb_color kTabTopAmber = { 0xf7, 0xba, 0x63, 255 };
static const rgb_color kHeaderHighlight = GREY(0xe8);
static const rgb_color kMenuBarLine = GREY(0xb6);
static const rgb_color kMenuBarHighlight = GREY(0xed);

// Expose Air Dark: a button's pale line, a pressed one, and what's selected
static const rgb_color kLineTop = GREY(0x8b);
static const rgb_color kLineBottom = GREY(0x5c);
static const rgb_color kDefaultLine = GREY(0x58);
static const rgb_color kSlateOutline = { 0x20, 0x20, 0x22, 255 };
static const rgb_color kSlateLine = { 0x83, 0x89, 0x92, 255 };
static const rgb_color kSlateTop = { 0x66, 0x6d, 0x78, 255 };
static const rgb_color kSlateBottom = { 0x76, 0x7f, 0x8c, 255 };
static const rgb_color kSelectedTop = { 0x75, 0x7d, 0x8a, 255 };
static const rgb_color kSelectedBottom = { 0x67, 0x6f, 0x7a, 255 };
static const rgb_color kGutter = { 0x18, 0x19, 0x16, 255 };
static const rgb_color kGutterEdge = { 0x14, 0x14, 0x12, 255 };
static const rgb_color kBarLine = { 0x92, 0x98, 0xa3, 255 };
static const rgb_color kThumbOutline = GREY(0x27);
static const rgb_color kThumbLine = GREY(0x9e);
static const rgb_color kThumb = GREY(0x87);
static const rgb_color kThumbHover = GREY(0x96);
static const rgb_color kActiveTab = { 0x46, 0x47, 0x44, 255 };
static const rgb_color kInactiveTab = { 0x32, 0x33, 0x31, 255 };

static const float kButtonRadius = 3;

// what a pop-up button or menu field keeps for its arrow, as Haiku's do
static const float kIndicatorWidth = 11;

// which of the bitmaps a mark is: ten for each theme, and the light thumb's two
enum {
	kMarkCheckOff = 0,
	kMarkCheckMixed = 4,
	kMarkCheckOn = 2,
	kMarkRadioOff = 6,
	kMarkRadioOn = 8,
	kMarkLightOffset = 10,
	kMarkThumb = 20
};


static bool
is_light()
{
	return ui_color(B_PANEL_BACKGROUND_COLOR).Brightness() >= 128;
}


static const Palette&
palette()
{
	return is_light() ? kLightPalette : kDarkPalette;
}


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


static rgb_color
grey(uint8 level)
{
	rgb_color color = { level, level, level, 255 };
	return color;
}


/*!	A rounded rectangle set row by row, so that its corners are crisp steps with
	no colour fringes.
*/
static void
fill_rounded(BView* view, BRect rect, float radius, const rgb_color& color)
{
	if (!rect.IsValid())
		return;

	view->SetHighColor(color);
	const int32 rows = (int32)std::min(radius, floorf((rect.Height() + 1) / 2));
	for (int32 row = 0; row < rows; row++) {
		const float distance = radius - row - 0.5f;
		const float inset = floorf(radius
			- sqrtf(std::max(radius * radius - distance * distance, 0.0f))
			+ 0.5f);
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


/*!	Fills \a rect from one colour to another across its height, or its width. */
static void
fill_ramp(BView* view, const BRect& rect, const rgb_color& from,
	const rgb_color& to, orientation direction = B_VERTICAL)
{
	if (!rect.IsValid())
		return;

	BGradientLinear gradient;
	gradient.AddColor(from, 0);
	gradient.AddColor(to, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(direction == B_VERTICAL ? rect.LeftBottom() : rect.RightTop());
	view->FillRect(rect, gradient);
}


/*!	A ring with a ramp down it, with cut corners. What's inside is left for
	whatever draws next.
*/
static void
ramp_ring(BView* view, BRect rect, const rgb_color& top, const rgb_color& bottom)
{
	if (!rect.IsValid())
		return;

	BGradientLinear gradient;
	gradient.AddColor(top, 0);
	gradient.AddColor(bottom, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(BRect(rect.left + 1, rect.top, rect.right - 1, rect.top),
		gradient);
	view->FillRect(BRect(rect.left + 1, rect.bottom, rect.right - 1, rect.bottom),
		gradient);
	view->FillRect(BRect(rect.left, rect.top + 1, rect.left, rect.bottom - 1),
		gradient);
	view->FillRect(BRect(rect.right, rect.top + 1, rect.right, rect.bottom - 1),
		gradient);
}


/*!	Air Dark's glass: the upper half's ramp, then a hard step down to the lower
	half's. Hovering lifts both.
*/
static void
fill_glass(BView* view, const BRect& rect, bool hover, bool disabled)
{
	uint8 upperTop = hover ? 0x5e : 0x54;
	uint8 upperBottom = hover ? 0x4d : 0x44;
	uint8 lowerTop = hover ? 0x24 : 0x20;
	uint8 lowerBottom = hover ? 0x31 : 0x2c;
	if (disabled) {
		upperTop = 0x3c;
		upperBottom = 0x38;
		lowerTop = 0x2c;
		lowerBottom = 0x30;
	}

	const float middle = floorf((rect.top + rect.bottom) / 2);
	fill_ramp(view, BRect(rect.left, rect.top, rect.right, middle),
		grey(upperTop), grey(upperBottom));
	fill_ramp(view, BRect(rect.left, middle + 1, rect.right, rect.bottom),
		grey(lowerTop), grey(lowerBottom));
}


/*!	Expose Dark's button: white for its first rows, shading down to grey. */
static void
fill_soft(BView* view, const BRect& rect, bool hover, bool disabled)
{
	uint8 top = hover ? 0xff : 0xfc;
	uint8 bottom = hover ? 0xd0 : 0xbb;
	if (disabled) {
		top = 0xe6;
		bottom = 0xd2;
	}

	BGradientLinear gradient;
	gradient.AddColor(grey(top), 0);
	gradient.AddColor(grey(top), 40);
	gradient.AddColor(grey(bottom), 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED)) != 0;
}


/*!	A small arrow in rows of solid pixels, pointing down. */
static void
draw_down_arrow(BView* view, const BRect& area, const rgb_color& color)
{
	const float rows = 4;
	const float centre = floorf((area.left + area.right) / 2);
	const float top = floorf((area.top + area.bottom + 1 - rows) / 2);
	view->SetHighColor(color);
	for (int32 row = 0; row < rows; row++) {
		const float half = rows - 1 - row;
		view->FillRect(BRect(centre - half, top + row, centre + half + 1,
			top + row));
	}
}



/*!	Draws a tab, or the line the tabs sit on, on another side of a tab view as
	it is drawn on the top: into a bitmap of the top side's shape, whose pixels
	are then copied into place flipped for the bottom side and turned for the
	left and right. Inset() turns the top side's insets back for the label.
*/
class TabSide {
public:
	TabSide(const BRect& rect, uint32 side)
		:
		fSide(side),
		fWidth((int32)rect.IntegerWidth() + 1),
		fHeight((int32)rect.IntegerHeight() + 1),
		fBitmap(NULL),
		fView(NULL)
	{
		const bool turned = side == BTabView::kLeftSide
			|| side == BTabView::kRightSide;
		fRect = turned ? BRect(0, 0, fHeight - 1, fWidth - 1)
			: BRect(0, 0, fWidth - 1, fHeight - 1);
		fStart = fRect;
		fBitmap = new(std::nothrow) BBitmap(fRect, B_RGBA32, true);
		if (fBitmap == NULL || fBitmap->InitCheck() != B_OK)
			return;
		memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
		fView = new BView(fRect, "tab", 0, B_WILL_DRAW);
		fBitmap->AddChild(fView);
		fBitmap->Lock();
	}

	~TabSide()
	{
		if (fView != NULL)
			fBitmap->Unlock();
		delete fBitmap;
	}

	bool IsValid() const
	{
		return fView != NULL;
	}

	BView* View() const
	{
		return fView;
	}

	BRect& Rect()
	{
		return fRect;
	}

	//!	Copies what was drawn into view, at rect, turned the right way.
	void Draw(BView* view, const BRect& rect)
	{
		fView->Sync();
		BBitmap out(BRect(0, 0, fWidth - 1, fHeight - 1), B_RGBA32);
		if (out.InitCheck() != B_OK)
			return;
		const uint32* source = (const uint32*)fBitmap->Bits();
		const int32 sourceRow = fBitmap->BytesPerRow() / 4;
		uint32* target = (uint32*)out.Bits();
		const int32 targetRow = out.BytesPerRow() / 4;
		for (int32 y = 0; y < fHeight; y++) {
			for (int32 x = 0; x < fWidth; x++) {
				int32 sx, sy;
				switch (fSide) {
					case BTabView::kBottomSide:
						sx = x;
						sy = fHeight - 1 - y;
						break;
					case BTabView::kLeftSide:
						sx = y;
						sy = x;
						break;
					default:
						sx = y;
						sy = fWidth - 1 - x;
						break;
				}
				target[y * targetRow + x] = source[sy * sourceRow + sx];
			}
		}
		view->PushState();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->DrawBitmap(&out, rect.LeftTop());
		view->PopState();
	}

	void Inset(BRect& rect) const
	{
		const float left = fRect.left - fStart.left;
		const float top = fRect.top - fStart.top;
		const float right = fStart.right - fRect.right;
		const float bottom = fStart.bottom - fRect.bottom;
		switch (fSide) {
			case BTabView::kBottomSide:
				rect.left += left;
				rect.right -= right;
				rect.top += bottom;
				rect.bottom -= top;
				break;
			case BTabView::kLeftSide:
				rect.left += top;
				rect.right -= bottom;
				rect.top += left;
				rect.bottom -= right;
				break;
			default:
				rect.right -= top;
				rect.left += bottom;
				rect.top += left;
				rect.bottom -= right;
				break;
		}
	}

private:
	uint32		fSide;
	int32		fWidth;
	int32		fHeight;
	BBitmap*	fBitmap;
	BView*		fView;
	BRect		fRect;
	BRect		fStart;
};

//	#pragma mark -


ExposeControlLook::ExposeControlLook()
{
	memset(fMarks, 0, sizeof(fMarks));
}


ExposeControlLook::~ExposeControlLook()
{
	for (size_t i = 0; i < sizeof(fMarks) / sizeof(fMarks[0]); i++)
		delete fMarks[i];
}


//	#pragma mark - buttons


void
ExposeControlLook::DrawButtonFrame(BView* view, BRect& rect,
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
		shape.left -= kButtonRadius;
	if ((borders & B_RIGHT_BORDER) == 0)
		shape.right += kButtonRadius;
	if ((borders & B_TOP_BORDER) == 0)
		shape.top -= kButtonRadius;
	if ((borders & B_BOTTOM_BORDER) == 0)
		shape.bottom += kButtonRadius;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool pressed = is_pressed(flags);

	if (is_light()) {
		// the rim and the outline; the amber of a default button goes inside them
		fill_rounded(view, shape, kButtonRadius + 1, kRim);
		fill_rounded(view, shape.InsetByCopy(1, 1), kButtonRadius,
			disabled ? mix(kLightPalette.buttonOutline, background, 0.5f)
				: kLightPalette.buttonOutline);
		fill_rounded(view, shape.InsetByCopy(2, 2), kButtonRadius - 1,
			background);
		if (isDefault) {
			ramp_ring(view, shape.InsetByCopy(2, 2), kAmberOuterTop,
				kAmberOuterBottom);
			ramp_ring(view, shape.InsetByCopy(3, 3), kAmberInnerTop,
				kAmberInnerBottom);
			ramp_ring(view, shape.InsetByCopy(4, 4), kAmberInnerTop,
				kAmberInnerBottom);
		}
	} else {
		if (isDefault) {
			fill_rounded(view, shape, kButtonRadius + 2, kDefaultLine);
			fill_rounded(view, shape.InsetByCopy(1, 1), kButtonRadius + 1,
				background);
			fill_rounded(view, shape.InsetByCopy(2, 2), kButtonRadius + 1,
				kDefaultLine);
			shape.InsetBy(3, 3);
		}

		fill_rounded(view, shape, kButtonRadius, pressed ? kSlateOutline
			: disabled ? mix(kDarkPalette.buttonOutline, background, 0.5f)
				: kDarkPalette.buttonOutline);

		rgb_color top = pressed ? kSlateLine : kLineTop;
		rgb_color bottom = pressed ? kSlateLine : kLineBottom;
		if (disabled) {
			top = mix(top, background, 0.6f);
			bottom = mix(bottom, background, 0.6f);
		}
		ramp_ring(view, shape.InsetByCopy(1, 1), top, bottom);
	}

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
ExposeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(1, 1);
		return;
	}

	const Palette& colors = palette();

	view->PushState();
	view->ClipToRect(rect);

	// Tracker's column headers are drawn as a button's background; the view's
	// name is the only thing that tells them from any other button
	if (view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0) {
		view->SetHighColor((flags & B_ACTIVATED) != 0
			? mix(colors.headerFill, is_light() ? kBlack : kWhite, 0.1f)
			: colors.headerFill);
		view->FillRect(rect);
		if (is_light()) {
			view->SetHighColor(kHeaderHighlight);
			view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
		}
		view->SetHighColor(colors.headerLine);
		view->FillRect(BRect(rect.left, rect.bottom + 1, rect.right,
			rect.bottom + 1));
		view->PopState();
		return;
	}

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool hover = (flags & (B_HOVER | B_FOCUSED)) != 0;

	if (is_light()) {
		// its first pixel in is a line of its own, darkening towards the bottom
		if (is_pressed(flags)) {
			view->SetHighColor(kInnerPressed);
			view->FillRect(rect);
			view->SetHighColor(kPressedFill);
			view->FillRect(rect.InsetByCopy(1, 1));
		} else if ((flags & B_DEFAULT_BUTTON) != 0) {
			fill_soft(view, rect, hover, disabled);
		} else {
			ramp_ring(view, rect, hover ? kInnerHoverTop : kInnerTop,
				hover ? kInnerHoverBottom : kInnerBottom);
			fill_soft(view, rect.InsetByCopy(1, 1), hover, disabled);
		}
	} else {
		if (is_pressed(flags))
			fill_ramp(view, rect, kSlateTop, kSlateBottom);
		else
			fill_glass(view, rect, hover, disabled);
	}

	view->PopState();

	rect.InsetBy(1, 1);
}


void
ExposeControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_down_arrow(view, BRect(area.right - kIndicatorWidth, area.top,
		area.right, area.bottom), palette().arrow);
	view->PopState();
}


//	#pragma mark - menus


void
ExposeControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// Tracker's item count uses this too, and has no gloss in the artwork
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->FillRect(rect);
		view->PopState();
		return;
	}

	if (is_light()) {
		// the same grey as a window, with the artwork's two lines under it
		view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->FillRect(rect);
		view->SetHighColor(kMenuBarHighlight);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
		view->SetHighColor(kMenuBarLine);
		view->FillRect(BRect(rect.left, rect.bottom - 1, rect.right,
			rect.bottom - 1));
		view->PopState();
		return;
	}

	// black glass, from the artwork's stops
	BGradientLinear gradient;
	const rgb_color top = { 0x49, 0x4c, 0x47, 255 };
	const rgb_color fold = { 0x24, 0x26, 0x23, 255 };
	const rgb_color deep = { 0x0a, 0x0d, 0x0a, 255 };
	const rgb_color bottom = { 0x15, 0x16, 0x13, 255 };
	gradient.AddColor(top, 0);
	gradient.AddColor(fold, 112);
	gradient.AddColor(deep, 143);
	gradient.AddColor(bottom, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);

	view->PopState();
}


void
ExposeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!is_light()) {
		DrawButtonFrame(view, rect, updateRect, base, background, flags, borders);
		return;
	}

	// like a text field
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->SetHighColor(background);
		view->StrokeRect(rect);
		view->SetHighColor((flags & B_FOCUSED) != 0
			? kLightPalette.editFocusLine : kLightPalette.editLine);
		view->StrokeRect(rect.InsetByCopy(1, 1));
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


void
ExposeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool light = is_light();

	view->PushState();
	view->ClipToRect(rect);

	if (light) {
		view->SetHighColor(disabled ? grey(0xee) : kWhite);
		view->FillRect(rect);
	} else
		fill_glass(view, rect, (flags & B_HOVER) != 0, disabled);

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth - 3;
		if (light) {
			// a small button of its own
			fill_soft(view, indicator, (flags & B_HOVER) != 0, disabled);
			view->SetHighColor(kLightPalette.editLine);
			view->FillRect(BRect(indicator.left, rect.top, indicator.left,
				rect.bottom));
		} else {
			view->SetHighColor(kDarkPalette.buttonOutline);
			view->FillRect(BRect(indicator.left, rect.top, indicator.left,
				rect.bottom));
		}
		indicator.left++;
		const rgb_color& arrow = palette().arrow;
		draw_down_arrow(view, indicator, disabled
			? mix(arrow, ui_color(B_PANEL_BACKGROUND_COLOR), 0.5f) : arrow);
		rect.right = indicator.left - 2;
	}

	view->PopState();
}


void
ExposeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	if (is_light()) {
		view->SetHighColor((flags & B_DISABLED) != 0 ? grey(0xee) : kWhite);
		view->FillRect(rect);
	} else
		fill_glass(view, rect, (flags & B_HOVER) != 0, (flags & B_DISABLED) != 0);
	view->PopState();
}


void
ExposeControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	view->SetHighColor(palette().menuLine);
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
ExposeControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	if ((flags & (B_ACTIVATED | B_HOVER)) != 0 && (flags & B_DISABLED) == 0) {
		if (is_light()) {
			view->SetHighColor(kLightSelected);
			view->FillRect(rect);
		} else
			fill_ramp(view, rect, kSelectedTop, kSelectedBottom);
	} else {
		view->SetHighColor(base);
		view->FillRect(rect);
	}
	view->PopState();
}


//	#pragma mark - status bars


void
ExposeControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const bool light = is_light();
	const bool themed = barColor == ui_color(B_STATUS_BAR_COLOR);
	BRect inside = rect.InsetByCopy(1, 1);

	view->SetHighColor(light ? kLightPalette.editLine : kSlateOutline);
	view->StrokeRect(rect);
	view->SetHighColor(light ? kWhite : kGutter);
	view->FillRect(inside);
	if (!light) {
		view->SetHighColor(kGutterEdge);
		view->FillRect(BRect(inside.left, inside.top, inside.right, inside.top));
	}

	if (progressPosition > rect.left + 1) {
		BRect bar(inside);
		bar.right = std::min(progressPosition - 1, inside.right);
		if (bar.IsValid()) {
			if (light) {
				// shaded from both edges to the middle
				BGradientLinear gradient;
				gradient.AddColor(mix(barColor, kWhite, 0.3f), 0);
				gradient.AddColor(barColor, 128);
				gradient.AddColor(mix(barColor, kWhite, 0.3f), 255);
				gradient.SetStart(bar.LeftTop());
				gradient.SetEnd(bar.LeftBottom());
				view->FillRect(bar, gradient);
				view->SetHighColor(themed ? kBarEdge : mix(barColor, kBlack, 0.2f));
				view->StrokeRect(bar);
			} else {
				fill_ramp(view, bar, mix(barColor, kWhite, 0.06f),
					mix(barColor, kBlack, 0.06f));
				view->SetHighColor(themed ? kBarLine
					: mix(barColor, kWhite, 0.25f));
				view->FillRect(BRect(bar.left, bar.top, bar.right, bar.top));
			}
		}
	}

	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	One of the artwork's marks, made into a bitmap the first time it's used, and
	scaled to the control's size.
*/
void
ExposeControlLook::_DrawMark(BView* view, const BRect& rect,
	const uint32* pixels, int32 index, bool disabled)
{
	const int32 slot = index * 2 + (disabled ? 1 : 0);
	if (fMarks[slot] == NULL) {
		BBitmap* bitmap = new (std::nothrow) BBitmap(
			BRect(0, 0, kMarkSize - 1, kMarkSize - 1), B_RGBA32);
		if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
			delete bitmap;
			return;
		}
		uint8* row = (uint8*)bitmap->Bits();
		for (int32 y = 0; y < kMarkSize; y++) {
			uint32* out = (uint32*)row;
			for (int32 x = 0; x < kMarkSize; x++) {
				uint32 pixel = pixels[y * kMarkSize + x];
				if (disabled)
					pixel = (pixel & 0x00ffffff) | (((pixel >> 24) / 2) << 24);
				// on a little-endian CPU, this is B_RGBA32's B, G, R, A
				out[x] = pixel;
			}
			row += bitmap->BytesPerRow();
		}
		fMarks[slot] = bitmap;
	}

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->DrawBitmap(fMarks[slot], fMarks[slot]->Bounds(), rect,
		B_FILTER_BITMAP_BILINEAR);
	view->PopState();
}


void
ExposeControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool hover = (flags & (B_HOVER | B_FOCUSED | B_CLICKED)) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	const bool light = is_light();
	const int32 offset = (light ? kMarkLightOffset : 0) + (hover ? 1 : 0);

	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		_DrawMark(view, rect, light
				? (hover ? kCheckMixedHoverLight : kCheckMixedLight)
				: (hover ? kCheckMixedHoverDark : kCheckMixedDark),
			kMarkCheckMixed + offset, disabled);
	} else if ((flags & B_ACTIVATED) != 0) {
		_DrawMark(view, rect, light
				? (hover ? kCheckOnHoverLight : kCheckOnLight)
				: (hover ? kCheckOnHoverDark : kCheckOnDark),
			kMarkCheckOn + offset, disabled);
	} else {
		_DrawMark(view, rect, light
				? (hover ? kCheckOffHoverLight : kCheckOffLight)
				: (hover ? kCheckOffHoverDark : kCheckOffDark),
			kMarkCheckOff + offset, disabled);
	}
}


void
ExposeControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool hover = (flags & (B_HOVER | B_FOCUSED | B_CLICKED)) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	const bool light = is_light();
	const int32 offset = (light ? kMarkLightOffset : 0) + (hover ? 1 : 0);

	if ((flags & B_ACTIVATED) != 0) {
		_DrawMark(view, rect, light
				? (hover ? kRadioOnHoverLight : kRadioOnLight)
				: (hover ? kRadioOnHoverDark : kRadioOnDark),
			kMarkRadioOn + offset, disabled);
	} else {
		_DrawMark(view, rect, light
				? (hover ? kRadioOffHoverLight : kRadioOffLight)
				: (hover ? kRadioOffHoverDark : kRadioOffDark),
			kMarkRadioOff + offset, disabled);
	}
}


//	#pragma mark - scroll bars


void
ExposeControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(palette().buttonOutline);
	view->StrokeRect(rect);
	view->PopState();
}


void
ExposeControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();
	view->ClipToRect(rect);

	const bool hover = (flags & B_HOVER) != 0 || down;
	const bool disabled = (flags & B_DISABLED) != 0;
	const enum orientation across
		= orientation == B_VERTICAL ? B_HORIZONTAL : B_VERTICAL;
	fill_ramp(view, rect, hover ? colors.thumbHoverLight : colors.thumbLight,
		hover ? colors.thumbHoverDark : colors.thumbDark, across);
	view->SetHighColor(colors.scrollEdge);
	view->StrokeRect(rect);

	// stepped, as in the artwork
	view->SetHighColor(disabled ? mix(colors.arrow, colors.thumbDark, 0.6f)
		: colors.arrow);
	const float centreX = floorf((rect.left + rect.right) / 2);
	const float centreY = floorf((rect.top + rect.bottom) / 2);
	const int32 steps = 4;
	for (int32 step = 0; step < steps; step++) {
		// how far from the arrow's tip, and how wide there
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

	view->PopState();
}


void
ExposeControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


void
ExposeControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();

	// the trough is the thumb's shading turned round
	const enum orientation across
		= orientation == B_VERTICAL ? B_HORIZONTAL : B_VERTICAL;
	if ((flags & B_DISABLED) != 0) {
		view->SetHighColor(colors.troughDark);
		view->FillRect(rect);
	} else
		fill_ramp(view, rect, colors.troughDark, colors.troughLight, across);

	view->PopState();
}


void
ExposeControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();
	view->ClipToRect(rect);

	const bool hover = (flags & (B_HOVER | B_ACTIVATED)) != 0;
	const enum orientation across
		= orientation == B_VERTICAL ? B_HORIZONTAL : B_VERTICAL;
	if ((flags & B_DISABLED) != 0) {
		view->SetHighColor(colors.thumbDark);
		view->FillRect(rect);
	} else {
		fill_ramp(view, rect, hover ? colors.thumbHoverLight : colors.thumbLight,
			hover ? colors.thumbHoverDark : colors.thumbDark, across);
	}

	view->SetHighColor(colors.scrollEdge);
	view->StrokeRect(rect);
	if (is_light()) {
		view->SetHighColor(kWhite);
		view->StrokeRect(rect.InsetByCopy(1, 1));
	}

	view->PopState();
}


//	#pragma mark - sliders


void
ExposeControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();

	// a thin groove in the middle of what it's given, three pixels across
	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 1;
		bar.bottom = bar.top + 2;
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 1;
		bar.right = bar.left + 2;
	}

	view->SetHighColor(colors.sliderLine);
	view->StrokeRect(bar);
	view->SetHighColor(fillColor == base ? colors.sliderFill : fillColor);
	view->FillRect(bar.InsetByCopy(1, 1));

	view->PopState();
}


void
ExposeControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool hover = (flags & (B_HOVER | B_FOCUSED)) != 0;

	if (is_light()) {
		// round, so it takes a square in the middle of what it's given
		const float size = std::min(rect.Width(), rect.Height());
		const float left = floorf((rect.left + rect.right - size) / 2);
		const float top = floorf((rect.top + rect.bottom - size) / 2);
		_DrawMark(view, BRect(left, top, left + size, top + size),
			hover ? kThumbHoverLight : kThumbLight, kMarkThumb + (hover ? 1 : 0),
			(flags & B_DISABLED) != 0);
		return;
	}

	view->PushState();
	fill_rounded(view, rect, 3, kThumbOutline);
	fill_rounded(view, rect.InsetByCopy(1, 1), 2, kThumbLine);
	fill_rounded(view, rect.InsetByCopy(3, 3), 1, hover ? kThumbHover : kThumb);
	view->PopState();
}


//	#pragma mark - text controls


void
ExposeControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	const Palette& colors = palette();

	view->PushState();

	// one thin line, with the panel showing round it to keep Haiku's insets
	rgb_color line = (flags & B_FOCUSED) != 0
		? colors.editFocusLine : colors.editLine;
	if ((flags & B_DISABLED) != 0)
		line = mix(line, base, 0.6f);

	view->SetHighColor(base);
	view->StrokeRect(rect);
	view->SetHighColor(line);
	view->StrokeRect(rect.InsetByCopy(1, 1));

	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


void
ExposeControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	fill_rounded(view, rect, 3, palette().groupLine);
	fill_rounded(view, rect.InsetByCopy(1, 1), 2, base);
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - tabs


void
ExposeControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (side != BTabView::kTopSide) {
		TabSide turned(rect, side);
		if (turned.IsValid()) {
			DrawTabFrame(turned.View(), turned.Rect(), turned.Rect(),
				base, flags, borders, borderStyle, BTabView::kTopSide);
			turned.Draw(view, rect);
			turned.Inset(rect);
		}
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	if (borderStyle != B_NO_BORDER) {
		view->SetHighColor(palette().tabPageLine);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}
	view->PopState();
}


void
ExposeControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		TabSide turned(rect, side);
		if (turned.IsValid()) {
			DrawActiveTab(turned.View(), turned.Rect(), turned.Rect(),
				base, flags, borders, BTabView::kTopSide, index, selected,
				first, last);
			turned.Draw(view, rect);
			turned.Inset(rect);
		}
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();
	if (is_light()) {
		// white, under an orange line and an amber one
		fill_ramp(view, rect, kWhite, grey(0xf3));
		view->SetHighColor(colors.tabPageLine);
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		view->StrokeLine(rect.RightTop(), rect.RightBottom());
		view->SetHighColor(colors.activeTabLine);
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
		view->SetHighColor(kTabTopAmber);
		view->FillRect(BRect(rect.left, rect.top + 1, rect.right, rect.top + 2));
	} else {
		view->SetHighColor(kActiveTab);
		view->FillRect(rect);
		view->SetHighColor(colors.activeTabLine);
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top + 1));
		view->SetHighColor(colors.tabPageLine);
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		view->StrokeLine(rect.RightTop(), rect.RightBottom());
	}
	view->PopState();

	rect.left += 3;
	rect.right -= 3;
	rect.top += 3;
	rect.bottom -= 1;
}


void
ExposeControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		TabSide turned(rect, side);
		if (turned.IsValid()) {
			DrawInactiveTab(turned.View(), turned.Rect(), turned.Rect(),
				base, flags, borders, BTabView::kTopSide, index, selected,
				first, last);
			turned.Draw(view, rect);
			turned.Inset(rect);
		}
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Palette& colors = palette();

	view->PushState();

	// a little lower than the active one, as in the artwork
	view->SetHighColor(base);
	view->FillRect(rect);
	BRect shape(rect);
	shape.top += 3;
	if (is_light()) {
		fill_ramp(view, shape, grey(0xf2), grey(0xcc));
		view->SetHighColor(colors.inactiveTabLine);
		view->StrokeLine(shape.LeftTop(), shape.LeftBottom());
		view->StrokeLine(shape.RightTop(), shape.RightBottom());
	} else {
		view->SetHighColor(kInactiveTab);
		view->FillRect(shape);
	}
	view->SetHighColor(colors.inactiveTabLine);
	view->FillRect(BRect(shape.left, shape.top, shape.right, shape.top));
	view->SetHighColor(colors.tabPageLine);
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
	return new (std::nothrow) BPrivate::ExposeControlLook();
}
