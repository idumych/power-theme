/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See SumaControlLook.h. Colours and sizes were measured from the pictures of the
 * SuMa 1 GUI kit (2006).
 */


#include "SumaControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <Button.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }
#define RGB(hex) { (uint8)((hex) >> 16), (uint8)((hex) >> 8), (uint8)(hex), 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);


/*!	An accent's shades: its flat colour, as a menu selection; a scroll thumb's
	stripe and its dark edges; a progress bar's top and bottom halves; and the
	colour that sets it off, for tabs and pressed scroll arrows, light at its
	top, in its middle, light again at its bottom, and as its outline.
*/
struct Accent {
	rgb_color	face;
	rgb_color	stripe;
	rgb_color	stripeEdge;
	rgb_color	barTop;
	rgb_color	barBottom;
	rgb_color	otherTop;
	rgb_color	otherMiddle;
	rgb_color	otherBottom;
	rgb_color	otherLine;
};

// the kit's three versions, keyed by their flat colour, which their themes give
// as the highlight colour
static const Accent kAccents[] = {
	// Orange, set off by blue
	{ RGB(0xfda31c), RGB(0xf89320), RGB(0x7e5016), RGB(0xf2b97e),
		RGB(0xf5972e), RGB(0xe2ecf9), RGB(0x92beea), RGB(0xb6edff),
		RGB(0x0023b2) },
	// Blue, set off by gold
	{ RGB(0x7ebaff), RGB(0x60b2ff), RGB(0x3c5d8a), RGB(0xa4c7ff),
		RGB(0x6bb3ff), RGB(0xeeebd9), RGB(0xc9bb6d), RGB(0xffddb4),
		RGB(0x6e6400) },
	// Green, set off by purple
	{ RGB(0x96c800), RGB(0x97ba00), RGB(0x4d6200), RGB(0xb1d060),
		RGB(0x97bb00), RGB(0xf1e7fa), RGB(0xd0adf0), RGB(0xe3deff),
		RGB(0x96008f) },
};
static const int32 kAccentCount = sizeof(kAccents) / sizeof(kAccents[0]);

// black glass, top to bottom: a light top edge, a grey upper half, and a lower
// half from black, lightening towards the bottom
static const rgb_color kGlassEdge = GREY(0xa5);
static const rgb_color kGlassTop = GREY(0x5a);
static const rgb_color kGlassMiddle = GREY(0x40);
static const rgb_color kGlassLow = GREY(0x06);
static const rgb_color kGlassBottom = GREY(0x39);

// a scroll bar's light track, across it
static const rgb_color kTrackEdge = GREY(0xb9);
static const rgb_color kTrackLight = GREY(0xee);
static const rgb_color kTrackFar = GREY(0xd5);
static const rgb_color kTrackLine = GREY(0xa0);
// a scroll arrow's box and triangle
static const rgb_color kArrowLight = GREY(0xfa);
static const rgb_color kArrowShade = GREY(0xdd);
static const rgb_color kArrowMark = GREY(0x43);

// a slider's groove, top to bottom
static const rgb_color kGrooveTop = GREY(0x56);
static const rgb_color kGrooveBottom = GREY(0xb0);

// a text field's frame
static const rgb_color kFieldOuter = GREY(0xc3);
static const rgb_color kFieldInner = GREY(0xf0);

static const rgb_color kGroupDark = GREY(0xc0);
static const rgb_color kInactiveTab = GREY(0xb8);
static const rgb_color kInactiveTabLine = GREY(0x8c);

static const float kIndicatorWidth = 20;
static const float kTabRadius = 4;


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


static bool
same_color(rgb_color a, rgb_color b)
{
	return a.red == b.red && a.green == b.green && a.blue == b.blue;
}


//!	The accent for a highlight colour: one of the kit's, or one shaded like them.
static Accent
accent_for(rgb_color face)
{
	for (int32 i = 0; i < kAccentCount; i++) {
		if (same_color(kAccents[i].face, face))
			return kAccents[i];
	}

	Accent accent;
	accent.face = face;
	accent.stripe = mix(face, kBlack, 0.05f);
	accent.stripeEdge = mix(face, kBlack, 0.5f);
	accent.barTop = mix(face, kWhite, 0.4f);
	accent.barBottom = mix(face, kBlack, 0.05f);
	// set off by the colour opposite it
	const rgb_color other = { (uint8)(255 - face.red), (uint8)(255 - face.green),
		(uint8)(255 - face.blue), 255 };
	accent.otherTop = mix(other, kWhite, 0.8f);
	accent.otherMiddle = mix(other, kWhite, 0.35f);
	accent.otherBottom = mix(other, kWhite, 0.65f);
	accent.otherLine = mix(other, kBlack, 0.4f);
	return accent;
}


static Accent
theme_accent()
{
	return accent_for(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
}


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	rgb_color color)
{
	if (left > right || top > bottom)
		return;
	view->SetHighColor(color);
	view->FillRect(BRect(left, top, right, bottom));
}


static bool
is_pressed(uint32 flags)
{
	return (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_PARTIALLY_ACTIVATED | BControlLook::B_CLICKED)) != 0
		&& (flags & BControlLook::B_DISABLED) == 0;
}


/*!	A shape of black glass with a black outline: a round rectangle, or a pill
	or a circle when the radius is half its height. Pressed, a glow in the
	accent rises from its bottom; disabled, it is a lighter grey.
*/
static void
draw_glass(BView* view, BRect r, float radius, bool pressed, bool disabled,
	const Accent& accent)
{
	if (!r.IsValid())
		return;

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
	const float lighter = disabled ? 0.55f : 0;
	view->SetHighColor(mix(kBlack, panel, lighter));
	view->FillRoundRect(r, radius, radius);

	const BRect inside(r.InsetByCopy(1, 1));
	BGradientLinear gradient;
	gradient.AddColor(mix(kGlassEdge, panel, lighter), 0);
	gradient.AddColor(mix(kGlassTop, panel, lighter), 20);
	gradient.AddColor(mix(kGlassMiddle, panel, lighter), 122);
	gradient.AddColor(mix(kGlassLow, panel, lighter), 128);
	if (pressed) {
		gradient.AddColor(mix(accent.face, kBlack, 0.85f), 170);
		gradient.AddColor(mix(accent.face, kBlack, 0.35f), 225);
		gradient.AddColor(mix(accent.face, kWhite, 0.55f), 255);
	} else
		gradient.AddColor(mix(kGlassBottom, panel, lighter), 255);
	gradient.SetStart(inside.LeftTop());
	gradient.SetEnd(inside.LeftBottom());
	view->FillRoundRect(inside, std::max(0.0f, radius - 1),
		std::max(0.0f, radius - 1), gradient);

	view->PopState();
}


/*!	A small solid triangle, its point towards direction, half wide across and
	tall along.
*/
static void
draw_triangle(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color, float half, float tall)
{
	const float x = floorf((rect.left + rect.right) / 2) + 0.5f;
	const float y = floorf((rect.top + rect.bottom) / 2) + 0.5f;
	const float base = tall / 2;
	BPoint points[3];
	switch (direction) {
		case BControlLook::B_UP_ARROW:
			points[0] = BPoint(x - half, y + base);
			points[1] = BPoint(x + half, y + base);
			points[2] = BPoint(x, y - base);
			break;
		case BControlLook::B_LEFT_ARROW:
			points[0] = BPoint(x + base, y - half);
			points[1] = BPoint(x + base, y + half);
			points[2] = BPoint(x - base, y);
			break;
		case BControlLook::B_RIGHT_ARROW:
			points[0] = BPoint(x - base, y - half);
			points[1] = BPoint(x - base, y + half);
			points[2] = BPoint(x + base, y);
			break;
		default:
			points[0] = BPoint(x - half, y - base);
			points[1] = BPoint(x + half, y - base);
			points[2] = BPoint(x, y + base);
			break;
	}
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(color);
	view->FillTriangle(points[0], points[1], points[2]);
	view->PopState();
}


//!	A glossy fill of the colour that sets the accent off, top to bottom.
static BGradientLinear
other_gradient(const Accent& accent, const BRect& r)
{
	BGradientLinear gradient;
	gradient.AddColor(accent.otherTop, 0);
	gradient.AddColor(accent.otherMiddle, 120);
	gradient.AddColor(mix(accent.otherMiddle, kBlack, 0.12f), 128);
	gradient.AddColor(accent.otherBottom, 255);
	gradient.SetStart(r.LeftTop());
	gradient.SetEnd(r.LeftBottom());
	return gradient;
}


//	#pragma mark -


SumaControlLook::SumaControlLook()
{
}


SumaControlLook::~SumaControlLook()
{
}


//	#pragma mark - buttons


/*!	A pill of black glass. A default button has a ring in the accent round it;
	pressed, the accent glows up from the button's bottom.
*/
void
SumaControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	const float inset = isDefault ? 4 : 2;

	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(inset, inset);
		return;
	}

	const Accent accent = theme_accent();
	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, background);

	BRect shape(rect);
	if (isDefault) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		const float radius = floorf((shape.Height() + 1) / 2);
		view->SetHighColor(disabled ? mix(accent.face, background, 0.6f)
			: accent.face);
		view->FillRoundRect(shape, radius, radius);
		shape.InsetBy(2, 2);
	}
	const float radius = std::min(floorf((shape.Height() + 1) / 2), 12.0f);
	draw_glass(view, shape, radius, is_pressed(flags), disabled, accent);

	view->PopState();

	rect.InsetBy(inset, inset);
}


/*!	The glass is all drawn by the frame; this keeps Haiku's inset. The
	Deskbar draws its items with this, in a menu, and they get the menu bar's
	black glass, lighter when one is open.
*/
void
SumaControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (dynamic_cast<BMenu*>(view) != NULL) {
		if (!ShouldDraw(view, rect, updateRect))
			return;
		view->PushState();
		const bool open = (flags & B_ACTIVATED) != 0;
		BGradientLinear gradient;
		gradient.AddColor(grey(open ? 0xb0 : 0x92), 0);
		gradient.AddColor(grey(open ? 0x88 : 0x66), 14);
		gradient.AddColor(grey(open ? 0x6a : 0x49), 125);
		gradient.AddColor(grey(open ? 0x30 : 0x0d), 132);
		gradient.AddColor(grey(open ? 0x48 : 0x25), 255);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
		view->FillRect(rect, gradient);
		view->PopState();
		return;
	}

	// Tracker's column headers are drawn as a button's background; the view's
	// name is the only thing that tells them from any other button
	if (view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0) {
		if (!ShouldDraw(view, rect, updateRect))
			return;
		view->PushState();
		const bool active = (flags & B_ACTIVATED) != 0;
		BGradientLinear gradient;
		gradient.AddColor(active ? grey(0xe8) : kWhite, 0);
		gradient.AddColor(active ? grey(0xd0) : grey(0xe8), 255);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
		view->FillRect(rect, gradient);
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kTrackLine);
		view->PopState();
		return;
	}

	rect.InsetBy(1, 1);
}


void
SumaControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
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
		(flags & B_DISABLED) != 0 ? grey(0xa0) : kWhite, 3.5f, 4);
	view->PopState();
}


//!	A button's label is white on its black glass.
void
SumaControlLook::DrawLabel(BView* view, const char* label,
	const BBitmap* icon, BRect rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, const BAlignment& alignment,
	const rgb_color* textColor)
{
	// BButton passes the control text colour; anything else is its own
	const bool standard = textColor == NULL
		|| same_color(*textColor, ui_color(B_CONTROL_TEXT_COLOR));
	if (standard && dynamic_cast<BButton*>(view) != NULL) {
		const rgb_color color = (flags & B_DISABLED) != 0 ? grey(0xd8) : kWhite;
		HaikuControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
			flags, alignment, &color);
		return;
	}
	HaikuControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);
}


void
SumaControlLook::DrawLabel(BView* view, const char* label, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	const BAlignment& alignment, const rgb_color* textColor)
{
	DrawLabel(view, label, NULL, rect, updateRect, base, flags, alignment,
		textColor);
}


//	#pragma mark - menus


//!	A bar of black glass, so that it matches the dark menus' white text.
void
SumaControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			ui_color(B_PANEL_BACKGROUND_COLOR));
		view->PopState();
		return;
	}

	BGradientLinear gradient;
	gradient.AddColor(grey(0x92), 0);
	gradient.AddColor(grey(0x66), 14);
	gradient.AddColor(grey(0x49), 125);
	gradient.AddColor(grey(0x0d), 132);
	gradient.AddColor(grey(0x25), 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kBlack);

	view->PopState();
}


/*!	A pill of black glass. Haiku draws the field's background in its menu bar,
	a view two pixels inside this rectangle, so the glass is drawn here.
*/
void
SumaControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			background);
		draw_glass(view, rect, 6, false, (flags & B_DISABLED) != 0,
			theme_accent());
		view->PopState();
	}
	rect.InsetBy(2, 2);
}


/*!	The glass again inside the frame, as Haiku clears the menu bar first, with
	a white triangle at its right end.
*/
void
SumaControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	draw_glass(view, rect.InsetByCopy(-2, -2), 6,
		(flags & (B_ACTIVATED | B_CLICKED)) != 0, (flags & B_DISABLED) != 0,
		theme_accent());

	if (popupIndicator) {
		BRect indicator(rect);
		indicator.left = indicator.right - kIndicatorWidth + 3;
		draw_triangle(view, indicator, B_DOWN_ARROW,
			(flags & B_DISABLED) != 0 ? grey(0xa0) : kWhite, 3.5f, 4);
		rect.right = indicator.left - 1;
	}

	view->PopState();
}


void
SumaControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


//!	A dark menu with a black line round it.
void
SumaControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if ((borders & B_LEFT_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kBlack);
	if ((borders & B_RIGHT_BORDER) != 0)
		fill_line(view, rect.right, rect.top, rect.right, rect.bottom, kBlack);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kBlack);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kBlack);
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


//!	A selected item, in a menu or the menu bar, is the accent, flat.
void
SumaControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		selected ? ui_color(B_MENU_SELECTED_BACKGROUND_COLOR) : base);
	view->PopState();
}


//	#pragma mark - status bars


/*!	A track of black glass, and in it a glossy bar in the status bar colour:
	lighter in its top half.
*/
void
SumaControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const Accent accent = same_color(barColor, ui_color(B_STATUS_BAR_COLOR))
		? theme_accent() : accent_for(barColor);

	view->PushState();
	draw_glass(view, rect, 3, false, false, accent);

	if (progressPosition > rect.left + 1) {
		BRect bar(rect.InsetByCopy(1, 1));
		bar.right = std::min(progressPosition, bar.right);
		BGradientLinear gradient;
		gradient.AddColor(mix(accent.barTop, kWhite, 0.2f), 0);
		gradient.AddColor(accent.barTop, 120);
		gradient.AddColor(accent.barBottom, 132);
		gradient.AddColor(mix(accent.barBottom, kWhite, 0.15f), 255);
		gradient.SetStart(bar.LeftTop());
		gradient.SetEnd(bar.LeftBottom());
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->FillRoundRect(bar, 2, 2, gradient);
	}
	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	A square of black glass with rounded corners, and an accent tick when it's
	on, a bar when it's partly on. Held down, the accent glows from its bottom.
*/
void
SumaControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const Accent accent = theme_accent();
	const rgb_color mark = disabled ? mix(accent.face, base, 0.55f)
		: accent.face;

	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_glass(view, r, 2, (flags & B_CLICKED) != 0 && !disabled, disabled,
		accent);

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(mark);
	const float size = r.Width() + 1;
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf((r.top + r.bottom) / 2);
		view->FillRect(BRect(r.left + roundf(size * 0.25f), middle - 1,
			r.right - roundf(size * 0.25f), middle));
	} else if ((flags & B_ACTIVATED) != 0) {
		// the tick reaches up out of the box's top right, as the kit's did
		view->SetPenSize(std::max(2.0f, size / 6));
		view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
		const BPoint start(r.left + size * 0.25f, r.top + size * 0.5f);
		const BPoint low(r.left + size * 0.45f, r.top + size * 0.75f);
		const BPoint end(r.left + size * 0.88f, r.top - size * 0.05f);
		view->StrokeLine(start, low);
		view->StrokeLine(low, end);
	}
	view->PopState();
}


//!	A ball of black glass, and an accent dot in it when it's on.
void
SumaControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const Accent accent = theme_accent();

	BRect ball(rect);
	ball.bottom = ball.top + ball.Width();
	draw_glass(view, ball, floorf((ball.Width() + 1) / 2),
		(flags & B_CLICKED) != 0 && !disabled, disabled, accent);

	if ((flags & B_ACTIVATED) != 0) {
		view->PushState();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(disabled ? mix(accent.face, base, 0.55f)
			: accent.face);
		const float inset = roundf((ball.Width() + 1) * 0.32f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
		view->PopState();
	}
}


//	#pragma mark - scroll bars


void
SumaControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kTrackLine);
	view->StrokeRect(rect);
	view->PopState();
}


/*!	A light box with a dark triangle; held down, it is glossy in the colour
	that sets the accent off.
*/
void
SumaControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	if (down && !disabled) {
		view->FillRect(rect, other_gradient(theme_accent(), rect));
	} else {
		BGradientLinear gradient;
		gradient.AddColor(kArrowLight, 0);
		gradient.AddColor(kArrowShade, 255);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(orientation == B_VERTICAL ? rect.RightTop()
			: rect.LeftBottom());
		view->FillRect(rect, gradient);
	}

	// the line between it and the track
	switch (direction) {
		case B_UP_ARROW:
			fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
				kTrackLine);
			break;
		case B_DOWN_ARROW:
			fill_line(view, rect.left, rect.top, rect.right, rect.top,
				kTrackLine);
			break;
		case B_LEFT_ARROW:
			fill_line(view, rect.right, rect.top, rect.right, rect.bottom,
				kTrackLine);
			break;
		case B_RIGHT_ARROW:
			fill_line(view, rect.left, rect.top, rect.left, rect.bottom,
				kTrackLine);
			break;
	}

	draw_triangle(view, rect, direction, disabled ? grey(0xa8) : kArrowMark,
		3.5f, 5);

	view->PopState();
}


void
SumaControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


//!	A light track, shaded across: grey at its edges, lightest past its middle.
void
SumaControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	BGradientLinear gradient;
	gradient.AddColor(kTrackEdge, 0);
	gradient.AddColor(kTrackLight, 190);
	gradient.AddColor(kTrackFar, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(orientation == B_VERTICAL ? rect.RightTop()
		: rect.LeftBottom());
	view->FillRect(rect, gradient);
	view->PopState();
}


/*!	A pill of black glass along the track, shaded across, with a stripe in the
	accent down its middle. A bar with nothing to scroll has none.
*/
void
SumaControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	BRect track(rect);
	DrawScrollBarBackground(view, track, updateRect, base, flags, orientation);
	if ((flags & B_DISABLED) != 0) {
		view->PopState();
		return;
	}

	const Accent accent = theme_accent();
	const bool vertical = orientation == B_VERTICAL;
	BRect t(rect.InsetByCopy(vertical ? 1 : 0, vertical ? 0 : 1));
	const float across = vertical ? t.Width() + 1 : t.Height() + 1;
	const float radius = floorf(across / 2);

	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(grey(0x0a));
	view->FillRoundRect(t, radius, radius);

	// lit along one side, dark in the middle, lighter at the far side
	const BRect inside(t.InsetByCopy(1, 1));
	BGradientLinear gradient;
	gradient.AddColor(grey(0x41), 0);
	gradient.AddColor(grey(0x30), 110);
	gradient.AddColor(grey(0x0f), 145);
	gradient.AddColor(grey(0x3e), 255);
	gradient.SetStart(inside.LeftTop());
	gradient.SetEnd(vertical ? inside.RightTop() : inside.LeftBottom());
	view->FillRoundRect(inside, std::max(0.0f, radius - 1),
		std::max(0.0f, radius - 1), gradient);

	// the stripe, a bright line between two dark ones, short of the ends
	const float middle = vertical ? floorf((t.left + t.right) / 2)
		: floorf((t.top + t.bottom) / 2);
	const float from = (vertical ? t.top : t.left) + radius;
	const float to = (vertical ? t.bottom : t.right) - radius;
	if (to > from) {
		for (int32 i = -1; i <= 1; i++) {
			const rgb_color color = i == 0 ? accent.stripe : accent.stripeEdge;
			if (vertical)
				fill_line(view, middle + i, from, middle + i, to, color);
			else
				fill_line(view, from, middle + i, to, middle + i, color);
		}
	}

	view->PopState();
}


float
SumaControlLook::GetScrollBarWidth(orientation orientation)
{
	return 15;
}


//	#pragma mark - sliders


//!	A thin rounded groove, dark at its top, in the middle of what it's given.
void
SumaControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

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
	if (!isGrey && (flags & B_DISABLED) == 0) {
		view->SetHighColor(fillColor);
		view->FillRoundRect(bar, 2, 2);
	} else {
		BGradientLinear gradient;
		const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
		const float lighter = (flags & B_DISABLED) != 0 ? 0.5f : 0;
		gradient.AddColor(mix(kGrooveTop, panel, lighter), 0);
		gradient.AddColor(mix(kGrooveBottom, panel, lighter), 255);
		gradient.SetStart(bar.LeftTop());
		gradient.SetEnd(orientation == B_HORIZONTAL ? bar.LeftBottom()
			: bar.RightTop());
		view->FillRoundRect(bar, 2, 2, gradient);
	}
	view->PopState();
}


//!	A ball of black glass with an accent dot in its middle.
void
SumaControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const Accent accent = theme_accent();

	// as round as the narrower side allows, in the middle of the other
	const float size = std::min(rect.Width(), rect.Height());
	BRect ball(0, 0, size, size);
	ball.OffsetTo(floorf((rect.left + rect.right - size) / 2),
		floorf((rect.top + rect.bottom - size) / 2));
	draw_glass(view, ball, floorf((size + 1) / 2),
		(flags & B_ACTIVATED) != 0 && !disabled, disabled, accent);

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(disabled ? mix(accent.face, base, 0.55f) : accent.face);
	const float inset = roundf((size + 1) * 0.32f);
	view->FillEllipse(ball.InsetByCopy(inset, inset));
	view->PopState();
}


//!	A pointer of black glass, its tip towards the marks.
void
SumaControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	BRect r(rect);
	r.bottom--;
	BPoint points[5];
	if (orientation == B_HORIZONTAL) {
		const float tip = std::min(6.0f, floorf(r.Width() / 2));
		points[0] = BPoint(r.left, r.top);
		points[1] = BPoint(r.right, r.top);
		points[2] = BPoint(r.right, r.bottom - tip);
		points[3] = BPoint((r.left + r.right) / 2, r.bottom);
		points[4] = BPoint(r.left, r.bottom - tip);
	} else {
		const float tip = std::min(6.0f, floorf(r.Height() / 2));
		points[0] = BPoint(r.left, r.top);
		points[1] = BPoint(r.left, r.bottom);
		points[2] = BPoint(r.right - tip, r.bottom);
		points[3] = BPoint(r.right, (r.top + r.bottom) / 2);
		points[4] = BPoint(r.right - tip, r.top);
	}
	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
	BGradientLinear gradient;
	gradient.AddColor(mix(grey(0x7b), panel, disabled ? 0.55f : 0), 0);
	gradient.AddColor(mix(grey(0x5c), panel, disabled ? 0.55f : 0), 115);
	gradient.AddColor(mix(grey(0x0f), panel, disabled ? 0.55f : 0), 125);
	gradient.AddColor(mix(grey(0x52), panel, disabled ? 0.55f : 0), 255);
	gradient.SetStart(r.LeftTop());
	gradient.SetEnd(r.LeftBottom());
	view->FillPolygon(points, 5, gradient);
	view->SetHighColor(disabled ? mix(kBlack, panel, 0.55f) : kBlack);
	view->StrokePolygon(points, 5);
	view->PopState();
}


//	#pragma mark - text controls


/*!	A soft grey frame, two lines deep; with the focus, both are the keyboard
	navigation colour.
*/
void
SumaControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();
	const bool disabled = (flags & B_DISABLED) != 0;
	if ((flags & B_FOCUSED) != 0 && !disabled) {
		const rgb_color focus = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		view->SetHighColor(focus);
		view->StrokeRect(rect);
		view->SetHighColor(mix(focus, kWhite, 0.5f));
		view->StrokeRect(rect.InsetByCopy(1, 1));
	} else {
		view->SetHighColor(disabled ? mix(kFieldOuter, base, 0.5f) : kFieldOuter);
		view->StrokeRect(rect);
		view->SetHighColor(kFieldInner);
		view->StrokeRect(rect.InsetByCopy(1, 1));
	}
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


//!	A light etched line round the group.
void
SumaControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kWhite);
	view->StrokeRect(BRect(rect.left + 1, rect.top + 1, rect.right,
		rect.bottom));
	view->SetHighColor(kGroupDark);
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


/*!	A tab with its corners rounded away from the page and open towards it:
	the chosen one glossy in the colour that sets the accent off, the others
	grey and pushed two pixels back.
*/
static void
draw_tab(BView* view, const BRect& rect, uint32 side, bool chosen,
	const Accent& accent)
{
	BRect r(rect);
	BRect shape(rect);
	const float back = chosen ? 0 : 2;
	switch (side) {
		case BTabView::kBottomSide:
			r.bottom -= back;
			shape = BRect(r.left, r.top - 4, r.right, r.bottom);
			break;
		case BTabView::kLeftSide:
			r.left += back;
			shape = BRect(r.left, r.top, r.right + 4, r.bottom);
			break;
		case BTabView::kRightSide:
			r.right -= back;
			shape = BRect(r.left - 4, r.top, r.right, r.bottom);
			break;
		default:
			r.top += back;
			shape = BRect(r.left, r.top, r.right, r.bottom + 4);
			break;
	}

	view->PushState();
	view->ClipToRect(rect);
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(chosen ? accent.otherLine : kInactiveTabLine);
	view->FillRoundRect(shape, kTabRadius, kTabRadius);
	const BRect inside(shape.InsetByCopy(1, 1));
	if (chosen) {
		view->FillRoundRect(inside, kTabRadius - 1, kTabRadius - 1,
			other_gradient(accent, inside));
	} else {
		view->SetHighColor(kInactiveTab);
		view->FillRoundRect(inside, kTabRadius - 1, kTabRadius - 1);
	}
	view->PopState();

	if (!chosen) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom,
			kInactiveTabLine);
	}
}


//!	Where a tab's label goes: inside its outline, a little away from the page.
static void
inset_tab(BRect& rect, uint32 side)
{
	switch (side) {
		case BTabView::kBottomSide:
			rect.top += 1;
			rect.bottom -= 3;
			rect.left += 4;
			rect.right -= 4;
			break;
		case BTabView::kLeftSide:
			rect.left += 3;
			rect.right -= 1;
			rect.top += 4;
			rect.bottom -= 4;
			break;
		case BTabView::kRightSide:
			rect.left += 1;
			rect.right -= 3;
			rect.top += 4;
			rect.bottom -= 4;
			break;
		default:
			rect.top += 3;
			rect.bottom -= 1;
			rect.left += 4;
			rect.right -= 4;
			break;
	}
}


//!	A grey line along the page's edge that the tabs sit on.
void
SumaControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if (borderStyle != B_NO_BORDER) {
		const BRect edge = page_edge(rect, side);
		fill_line(view, edge.left, edge.top, edge.right, edge.bottom,
			kInactiveTabLine);
	}
	view->PopState();
}


void
SumaControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, true, theme_accent());
		view->PopState();
	}

	inset_tab(rect, side);
}


void
SumaControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		draw_tab(view, rect, side, false, theme_accent());
		view->PopState();
	}

	inset_tab(rect, side);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::SumaControlLook();
}
