/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Control look in the style of ZETA's controls. See SmokeControlLook.h.
 *
 * Measured on ZETA 1.21 screenshots with the "smoke" theme (panel B4B4B4,
 * controls ABABAB, control border black, control highlight 7BAB94, menu
 * selection FABA86):
 *
 *   - buttons and check boxes: a black outline with rounded corners (5 and
 *     3 pixels), a face shading down from the control colour + 10 to the
 *     control colour - 19, and a one pixel shadow (panel - 33) below and
 *     right of it
 *   - the tick: a green (control highlight) mark outlined in black, that
 *     reaches up out of the box, with a soft shadow
 *   - text fields: the document colour in a rounded black outline, with a
 *     shadow line (document colour - 41) inside its top and left
 *   - menu fields: a rounded black outline round a face shading from the
 *     menu colour + 8 to - 20, and a pop-up section in the menu selection
 *     colour, cut off by a black line, with a white arrow. ZETA puts that
 *     section at the left end; here it is at the right, where Haiku's menu
 *     fields keep room for it.
 *   - scroll bars: a light track shading across the bar (panel + 36 to
 *     + 69), arrow buttons and thumb with a dark outline (panel - 116;
 *     - 60 when disabled), a light line along their left (or top) edge and
 *     a face shading across from panel - 20 to + 4; double chevrons as
 *     arrows; no thumb when the bar can't scroll
 *   - tabs: a strip in the control colour with a black outline; the
 *     selected tab rounded and outlined in black, shading from panel + 23
 *     down to the panel colour, open to the pane; the others flat, with a
 *     shadow line on their left
 *   - menus: a light line inside the window's black edge; the open menu bar
 *     title a sunken rounded box with white text
 *   - group boxes: a single rounded line (panel - 48)
 *
 * Radio buttons, pressed buttons and selected menu items weren't captured
 * and are drawn to match the rest.
 */


#include "SmokeControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>

#include <Bitmap.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <Rect.h>
#include <ScrollView.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


static const float kButtonRadius = 5;
static const float kCheckBoxRadius = 3;
static const float kFieldRadius = 5;
static const float kTabRadius = 5;
static const float kSliderRadius = 4;

// The tick, 14 x 11 pixels over a 12 pixel box, from 2 pixels above it:
// each row's runs of the mark's colour, as first and last column; its
// outline is everything next to them (not diagonally).
static const int8 kTickRuns[11][4] = {
	{ 12, 13, -1, -1 },
	{ 11, 13, -1, -1 },
	{ 10, 13, -1, -1 },
	{ 9, 12, -1, -1 },
	{ 2, 3, 8, 11 },
	{ 1, 3, 7, 10 },
	{ 1, 4, 6, 9 },
	{ 2, 8, -1, -1 },
	{ 2, 7, -1, -1 },
	{ 3, 6, -1, -1 },
	{ 3, 5, -1, -1 }
};
static const int32 kTickTop = -2;
static const int32 kTickWidth = 15;
static const int32 kTickHeight = 11;

// a scroll bar arrow pointing up: two chevrons, 5 x 6 pixels
static const char* const kDoubleArrow[] = {
	"..X..",
	".X.X.",
	"X...X",
	"..X..",
	".X.X.",
	"X...X"
};


static rgb_color
shift(const rgb_color& color, int32 delta)
{
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


//! \a a and \a b mixed, \a amount (0 to 1) of the way to \a b.
static rgb_color
mix(const rgb_color& a, const rgb_color& b, float amount)
{
	rgb_color result;
	result.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	result.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	result.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	result.alpha = 255;
	return result;
}


static rgb_color
panel_color()
{
	return ui_color(B_PANEL_BACKGROUND_COLOR);
}


/*!	\a color, or the view's low colour (failing that, the panel colour)
	where a control passes its transparent view colour.
*/
static rgb_color
solid(const rgb_color& color, BView* view = NULL)
{
	if (color != B_TRANSPARENT_COLOR && color.alpha != 0)
		return color;
	if (view != NULL && view->LowColor() != B_TRANSPARENT_COLOR
		&& view->LowColor().alpha != 0)
		return view->LowColor();
	return panel_color();
}


static bool
is_flat(uint32 flags)
{
	return (flags & BControlLook::B_FLAT) != 0
		&& (flags & (BControlLook::B_ACTIVATED
			| BControlLook::B_PARTIALLY_ACTIVATED)) == 0
		&& ((flags & (BControlLook::B_HOVER | BControlLook::B_FOCUSED)) == 0
			|| (flags & BControlLook::B_DISABLED) != 0);
}


static void
inset_by_borders(BRect& rect, float inset, uint32 borders)
{
	if ((borders & BControlLook::B_LEFT_BORDER) != 0)
		rect.left += inset;
	if ((borders & BControlLook::B_TOP_BORDER) != 0)
		rect.top += inset;
	if ((borders & BControlLook::B_RIGHT_BORDER) != 0)
		rect.right -= inset;
	if ((borders & BControlLook::B_BOTTOM_BORDER) != 0)
		rect.bottom -= inset;
}


//! A control's outline: black, grey when disabled, or the focus colour.
static rgb_color
outline_color(uint32 flags, bool showFocus = true)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return shift(panel_color(), -49);
	if (showFocus && (flags & BControlLook::B_FOCUSED) != 0)
		return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	return ui_color(B_CONTROL_BORDER_COLOR);
}


static rgb_color
shadow_color(const rgb_color& background, uint32 flags)
{
	return shift(background, (flags & BControlLook::B_DISABLED) != 0
		? -9 : -33);
}


static void
fill_gradient(BView* view, BRect rect, float radius, const rgb_color& start,
	const rgb_color& end, bool vertical = true)
{
	if (!rect.IsValid())
		return;

	BGradientLinear gradient;
	gradient.AddColor(start, 0);
	gradient.AddColor(end, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(vertical ? rect.LeftBottom() : rect.RightTop());
	if (radius > 0)
		view->FillRoundRect(rect, radius, radius, gradient);
	else
		view->FillRect(rect, gradient);
}


/*!	A control's face: shading down from \a base + 10 to \a base - 19; when
	pressed, darker and the other way round; when disabled, nearly flat.
*/
static void
fill_face(BView* view, BRect rect, float radius, const rgb_color& base,
	uint32 flags, bool vertical = true)
{
	if ((flags & BControlLook::B_DISABLED) != 0) {
		const rgb_color panel = panel_color();
		fill_gradient(view, rect, radius, panel, shift(panel, -8), vertical);
	} else if ((flags & (BControlLook::B_ACTIVATED
			| BControlLook::B_CLICKED)) != 0) {
		fill_gradient(view, rect, radius, shift(base, -26), shift(base, 2),
			vertical);
	} else {
		fill_gradient(view, rect, radius, shift(base, 10), shift(base, -19),
			vertical);
	}
}


static void
fill_pixel(BView* view, float x, float y, float size)
{
	view->FillRect(BRect(x, y, x + size - 1, y + size - 1));
}


/*!	A rounded box with a shadow: \a rect is cleared to \a background, the
	box takes all but its last column and row, and the shadow those.
	Returns the inside of the box's outline.
*/
static BRect
draw_shadowed_outline(BView* view, const BRect& rect, float radius,
	const rgb_color& background, const rgb_color& outline, uint32 flags)
{
	BRect box(rect.left, rect.top, rect.right - 1, rect.bottom - 1);

	view->SetHighColor(background);
	view->FillRect(rect);
	view->SetHighColor(shadow_color(background, flags));
	view->FillRoundRect(box.OffsetByCopy(1, 1), radius, radius);
	view->SetHighColor(outline);
	view->FillRoundRect(box, radius, radius);

	return box.InsetByCopy(1, 1);
}


/*!	A scroll bar's thumb or arrow button: a dark outline (lighter when
	disabled), a light line along the edge across the bar's top or left,
	and a face shading across the bar; pressed, the shading turns round.
*/
static void
draw_scroll_box(BView* view, const BRect& rect, const rgb_color& face,
	bool disabled, bool pressed, orientation orientation)
{
	const bool vertical = orientation == B_VERTICAL;

	view->SetHighColor(shift(face, disabled ? -60 : -116));
	view->StrokeRect(rect);
	BRect inside(rect.InsetByCopy(1, 1));
	if (!inside.IsValid())
		return;

	fill_gradient(view, inside, 0, shift(face, pressed ? 4 : -20),
		shift(face, pressed ? -20 : 4), !vertical);
	view->SetHighColor(shift(face, pressed ? -30 : 12));
	if (vertical)
		view->StrokeLine(inside.LeftTop(), inside.LeftBottom());
	else
		view->StrokeLine(inside.LeftTop(), inside.RightTop());
}


//	#pragma mark -


SmokeControlLook::SmokeControlLook()
{
}


SmokeControlLook::~SmokeControlLook()
{
}


//	#pragma mark - buttons


void
SmokeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
SmokeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
SmokeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
SmokeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
SmokeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
SmokeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, false);
}


void
SmokeControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


void
SmokeControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, float, const rgb_color& base,
	uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


void
SmokeControlLook::DrawButtonWithPopUpBackground(BView* view,
	BRect& rect, const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders, orientation)
{
	_DrawButtonBackground(view, rect, updateRect, base, flags, borders, true);
}


//	#pragma mark - menu fields and menus


void
SmokeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const BRect inside = draw_shadowed_outline(view, rect, kFieldRadius,
		solid(background, view), outline_color(flags), flags);
	// the face, for where the menu bar doesn't cover it
	fill_gradient(view, inside, kFieldRadius - 1, shift(base, 8),
		shift(base, -20));
	view->PopState();

	rect.InsetBy(2, 2);
}


void
SmokeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
SmokeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
SmokeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
SmokeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
SmokeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackground(view, rect, updateRect, base, popupIndicator,
		flags);
}


void
SmokeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_gradient(view, rect, 0, shift(base, 8), shift(base, -20));
	view->PopState();
	inset_by_borders(rect, 1, borders);
}


/*!	A menu bar shades down from the menu colour + 5 to - 12, over a darker
	line along its bottom.
*/
void
SmokeControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	BRect face(rect);
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->SetHighColor(shift(base, -27));
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
		face.bottom--;
	}
	fill_gradient(view, face, 0, shift(base, 5), shift(base, -12));
	view->PopState();
	inset_by_borders(rect, 1, borders);
}


/*!	A menu: a light line along its top and left, inside the black edge
	the window decorator draws round it.
*/
void
SmokeControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const rgb_color light = shift(base, 32);
	view->BeginLineArray(4);
	if ((borders & B_TOP_BORDER) != 0)
		view->AddLine(rect.LeftTop(), rect.RightTop(), light);
	if ((borders & B_LEFT_BORDER) != 0)
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), light);
	if ((borders & B_RIGHT_BORDER) != 0)
		view->AddLine(rect.RightTop(), rect.RightBottom(), base);
	if ((borders & B_BOTTOM_BORDER) != 0)
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), base);
	view->EndLineArray();
	inset_by_borders(rect, 1, borders);
	view->SetHighColor(base);
	view->FillRect(rect);
	view->PopState();
}


void
SmokeControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if (dynamic_cast<BMenuBar*>(view) != NULL
		&& dynamic_cast<BMenuField*>(view->Parent()) == NULL) {
		// An open menu bar title is a sunken box in the bar, shading up
		// from darker than the bar, with its label in the selected text
		// colour. The item draws its label in the view's colours right
		// after this.
		const rgb_color menu = ui_color(B_MENU_BACKGROUND_COLOR);
		const rgb_color top = shift(menu, -18);
		const rgb_color bottom = shift(menu, 14);
		BRect box(rect.left, rect.top + 1, rect.right, rect.bottom - 1);
		view->SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
		view->FillRoundRect(box, kFieldRadius, kFieldRadius);
		fill_gradient(view, box.InsetByCopy(1, 1), kFieldRadius - 1, top,
			bottom);
		view->SetLowColor(mix(top, bottom, 0.5f));
		view->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
		return;
	}

	// a rounded bar in the selection colour, lighter at the top
	view->PushState();
	BRect bar(rect);
	const BRect bounds(view->Bounds());
	bar.left = std::max(bar.left, bounds.left + 1);
	bar.right = std::min(bar.right, bounds.right - 1);
	view->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
	view->FillRect(bar);
	fill_gradient(view, bar, 3, mix(base, make_color(255, 255, 255), 0.15f),
		shift(base, -24));
	view->PopState();
}


//	#pragma mark - status bars, check boxes, radio buttons


void
SmokeControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// a sunken field with a shaded bar in it
	view->PushState();
	const rgb_color behind = solid(view->LowColor(), view);
	view->SetHighColor(behind);
	view->FillRect(rect);
	view->SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	view->FillRoundRect(rect, kCheckBoxRadius, kCheckBoxRadius);
	BRect inner(rect.InsetByCopy(1, 1));
	fill_gradient(view, inner, kCheckBoxRadius - 1, shift(base, -16),
		shift(base, 8));

	if (inner.IsValid() && progressPosition > inner.left) {
		view->ClipToRect(BRect(inner.left, inner.top,
			std::min(inner.right, progressPosition - 1), inner.bottom));
		fill_gradient(view, inner, kCheckBoxRadius - 1,
			mix(barColor, make_color(255, 255, 255), 0.3f),
			shift(barColor, -30));
	}
	view->PopState();

	rect = inner;
}


void
SmokeControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect box(floorf(rect.left), floorf(rect.top), floorf(rect.right),
		floorf(rect.bottom));
	const rgb_color behind = solid(view->LowColor(), view);

	view->PushState();
	view->SetHighColor(shadow_color(behind, flags));
	view->FillRoundRect(box.OffsetByCopy(1, 1), kCheckBoxRadius,
		kCheckBoxRadius);
	view->SetHighColor(outline_color(flags));
	view->FillRoundRect(box, kCheckBoxRadius, kCheckBoxRadius);
	fill_face(view, box.InsetByCopy(1, 1), kCheckBoxRadius - 1, base,
		flags & ~B_ACTIVATED);

	if ((flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0) {
		const float size = std::min(box.Width(), box.Height()) + 1;
		const float pixel = std::max(1.0f, floorf(size / 12));

		bool mark[kTickHeight][kTickWidth] = {};
		if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
			// a bar across the middle
			for (int32 y = 6; y < 8; y++) {
				for (int32 x = 2; x < 10; x++)
					mark[y][x] = true;
			}
		} else {
			for (int32 y = 0; y < kTickHeight; y++) {
				for (int32 run = 0; run < 4; run += 2) {
					if (kTickRuns[y][run] < 0)
						continue;
					for (int32 x = kTickRuns[y][run];
							x <= kTickRuns[y][run + 1]; x++)
						mark[y][x] = true;
				}
			}
		}

		const rgb_color fill = disabled
			? shift(panel_color(), -24) : ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		const rgb_color edge = outline_color(flags, false);
		const BPoint origin(box.left, box.top + kTickTop * pixel);

		// the shadow, then the outline and the mark
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(0, 0, 0, disabled ? 24 : 64);
		for (int32 y = -1; y <= kTickHeight; y++) {
			for (int32 x = -1; x <= kTickWidth; x++) {
				bool any = false;
				for (int32 dy = -1; dy <= 1 && !any; dy++) {
					for (int32 dx = -1; dx <= 1 && !any; dx++) {
						const int32 my = y + dy;
						const int32 mx = x + dx;
						if (abs(dx) + abs(dy) <= 1 && my >= 0
							&& my < kTickHeight && mx >= 0 && mx < kTickWidth)
							any = mark[my][mx];
					}
				}
				if (any) {
					fill_pixel(view, origin.x + (x + 2) * pixel,
						origin.y + (y + 1) * pixel, pixel);
				}
			}
		}
		view->SetDrawingMode(B_OP_COPY);

		for (int32 y = -1; y <= kTickHeight; y++) {
			for (int32 x = -1; x <= kTickWidth; x++) {
				const bool inside = y >= 0 && y < kTickHeight && x >= 0
					&& x < kTickWidth && mark[y][x];
				bool next = false;
				if (!inside) {
					const int32 offsets[4][2]
						= { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
					for (int32 i = 0; i < 4 && !next; i++) {
						const int32 my = y + offsets[i][1];
						const int32 mx = x + offsets[i][0];
						next = my >= 0 && my < kTickHeight && mx >= 0
							&& mx < kTickWidth && mark[my][mx];
					}
				}
				if (!inside && !next)
					continue;
				view->SetHighColor(inside ? fill : edge);
				fill_pixel(view, origin.x + x * pixel, origin.y + y * pixel,
					pixel);
			}
		}
	}
	view->PopState();

	rect = box.InsetByCopy(2, 2);
}


void
SmokeControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect box(floorf(rect.left), floorf(rect.top), floorf(rect.right),
		floorf(rect.bottom));
	const rgb_color behind = solid(view->LowColor(), view);

	view->PushState();
	view->SetHighColor(shadow_color(behind, flags));
	view->FillEllipse(box.OffsetByCopy(1, 1));
	view->SetHighColor(outline_color(flags));
	view->FillEllipse(box);

	BRect inside(box.InsetByCopy(1, 1));
	BGradientLinear gradient;
	if (disabled) {
		gradient.AddColor(panel_color(), 0);
		gradient.AddColor(shift(panel_color(), -8), 255);
	} else if ((flags & B_CLICKED) != 0) {
		gradient.AddColor(shift(base, -26), 0);
		gradient.AddColor(shift(base, 2), 255);
	} else {
		gradient.AddColor(shift(base, 10), 0);
		gradient.AddColor(shift(base, -19), 255);
	}
	gradient.SetStart(inside.LeftTop());
	gradient.SetEnd(inside.LeftBottom());
	view->FillEllipse(inside, gradient);

	if ((flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0) {
		// a dot in the tick's colours
		const float inset = floorf((box.Width() + 1) / 4);
		BRect dot(box.InsetByCopy(inset, inset));
		view->SetHighColor(outline_color(flags, false));
		view->FillEllipse(dot);
		view->SetHighColor(disabled ? shift(panel_color(), -24)
			: ui_color(B_CONTROL_HIGHLIGHT_COLOR));
		view->FillEllipse(dot.InsetByCopy(1, 1));
	}
	view->PopState();

	rect = box.InsetByCopy(2, 2);
}


//	#pragma mark - scroll bars


/*!	A scroll bar's edge: the track's darker shade along its top and left,
	its lighter one along its bottom and right.
*/
void
SmokeControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const rgb_color dark = shift(base, 29);
	const rgb_color light = shift(base, 61);

	view->PushState();
	view->BeginLineArray(4);
	view->AddLine(rect.LeftTop(), rect.RightTop(), dark);
	view->AddLine(BPoint(rect.left, rect.top + 1), rect.LeftBottom(), dark);
	view->AddLine(BPoint(rect.right, rect.top + 1), rect.RightBottom(), light);
	view->AddLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom(),
		light);
	view->EndLineArray();
	view->PopState();
}


/*!	An arrow button: the thumb's box, with two white chevrons pointing
	\a direction. Pressed, its shading turns round and the chevrons move a
	pixel.
*/
void
SmokeControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	draw_scroll_box(view, rect, base, (flags & B_DISABLED) != 0, down,
		orientation);

	const float size = std::min(rect.Width(), rect.Height()) + 1;
	const float pixel = std::max(1.0f, floorf(size / 14));
	const bool horizontal = direction == B_LEFT_ARROW
		|| direction == B_RIGHT_ARROW;
	const float width = (horizontal ? 6 : 5) * pixel;
	const float height = (horizontal ? 5 : 6) * pixel;
	BPoint origin(
		rect.left + floorf((rect.Width() + 1 - width) / 2),
		rect.top + floorf((rect.Height() + 1 - height) / 2));
	if (down)
		origin += BPoint(pixel, pixel);

	view->SetHighColor(shift(base, 69));
	for (int32 row = 0; row < 6; row++) {
		for (int32 column = 0; column < 5; column++) {
			if (kDoubleArrow[row][column] != 'X')
				continue;
			// the up arrow, turned for the other directions
			float x = column;
			float y = row;
			switch (direction) {
				case B_DOWN_ARROW:
					y = 5 - row;
					break;
				case B_LEFT_ARROW:
					x = row;
					y = column;
					break;
				case B_RIGHT_ARROW:
					x = 5 - row;
					y = column;
					break;
			}
			fill_pixel(view, origin.x + x * pixel, origin.y + y * pixel,
				pixel);
		}
	}

	view->PopState();
}


void
SmokeControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base,
	uint32 flags, orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags,
		orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags,
		orientation);
}


//! The track: light, shading across the bar.
void
SmokeControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	BGradientLinear gradient;
	gradient.AddColor(shift(base, 36), 0);
	gradient.AddColor(shift(base, 69), 170);
	gradient.AddColor(shift(base, 68), 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(orientation == B_VERTICAL
		? rect.RightTop() : rect.LeftBottom());
	view->FillRect(rect, gradient);
	view->PopState();
}


/*!	The thumb, in the panel colour like the arrows (it is given the scroll
	bar thumb colour). A bar that can't scroll shows only its track.
*/
void
SmokeControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if ((flags & B_DISABLED) != 0) {
		DrawScrollBarBackground(view, rect, updateRect, panel_color(), flags,
			orientation);
		return;
	}

	view->PushState();
	draw_scroll_box(view, rect, panel_color(), false, false, orientation);
	view->PopState();
}


void
SmokeControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& _base,
	border_style borderStyle, uint32 flags, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	BRect scrollCorner(rect.right, rect.bottom, rect.right, rect.bottom);
	if (horizontalScrollBarFrame.IsValid())
		scrollCorner.left = horizontalScrollBarFrame.right + 1;
	if (verticalScrollBarFrame.IsValid())
		scrollCorner.top = verticalScrollBarFrame.bottom + 1;

	view->PushState();
	if (borderStyle != B_NO_BORDER) {
		const rgb_color outline = shift(base, -58);
		view->BeginLineArray(4);
		if ((borders & B_TOP_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.RightTop(), outline);
		if ((borders & B_LEFT_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.LeftBottom(), outline);
		if ((borders & B_RIGHT_BORDER) != 0)
			view->AddLine(rect.RightTop(), rect.RightBottom(), outline);
		if ((borders & B_BOTTOM_BORDER) != 0)
			view->AddLine(rect.LeftBottom(), rect.RightBottom(), outline);
		view->EndLineArray();
		inset_by_borders(rect, 1, borders);

		if (borderStyle == B_FANCY_BORDER) {
			const rgb_color shade = shift(base, 21);
			const rgb_color light = shift(base, 69);
			view->BeginLineArray(4);
			if ((borders & B_TOP_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.RightTop(), shade);
			if ((borders & B_LEFT_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.LeftBottom(), shade);
			if ((borders & B_RIGHT_BORDER) != 0) {
				view->AddLine(BPoint(rect.right, rect.top + 1),
					rect.RightBottom(), light);
			}
			if ((borders & B_BOTTOM_BORDER) != 0) {
				view->AddLine(BPoint(rect.left + 1, rect.bottom),
					rect.RightBottom(), light);
			}
			view->EndLineArray();
			inset_by_borders(rect, 1, borders);
		}
	}

	if (horizontalScrollBarFrame.IsValid() && verticalScrollBarFrame.IsValid()
		&& scrollCorner.IsValid()) {
		view->SetHighColor(base);
		view->FillRect(scrollCorner & rect);
	}
	view->PopState();
}


void
SmokeControlLook::DrawArrowShape(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 direction,
	uint32 flags, float tint)
{
	ForwardingControlLook::DrawArrowShape(view, rect, updateRect, base, direction,
		flags, tint);
}


//	#pragma mark - sliders


rgb_color
SmokeControlLook::SliderBarColor(const rgb_color& base)
{
	return shift(panel_color(), -16);
}


void
SmokeControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color leftFillColor,
	rgb_color rightFillColor, float sliderScale, uint32 flags,
	orientation orientation)
{
	// splits the bar, drawing all of it clipped to each side
	ForwardingControlLook::DrawSliderBar(view, rect, updateRect, base,
		leftFillColor, rightFillColor, sliderScale, flags, orientation);
}


//! A rounded channel, outlined, shading across in \a fillColor.
void
SmokeControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(outline_color(flags, false));
	view->FillRoundRect(rect, kSliderRadius, kSliderRadius);
	fill_gradient(view, rect.InsetByCopy(1, 1), kSliderRadius - 1,
		shift(fillColor, -8), shift(fillColor, 8),
		orientation == B_HORIZONTAL);
	view->PopState();
}


/*!	The thumb: a rounded box like a button's, with a shadow and a grip of
	a dark and a light line down its middle.
*/
void
SmokeControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	BRect knob(floorf(rect.left), floorf(rect.top), floorf(rect.right) - 1,
		floorf(rect.bottom) - 1);
	const rgb_color behind = solid(view->LowColor(), view);
	const rgb_color face = panel_color();

	view->PushState();
	view->SetHighColor(shadow_color(behind, flags));
	view->FillRoundRect(knob.OffsetByCopy(1, 1), kSliderRadius,
		kSliderRadius);
	view->SetHighColor(outline_color(flags));
	view->FillRoundRect(knob, kSliderRadius, kSliderRadius);
	fill_gradient(view, knob.InsetByCopy(1, 1), kSliderRadius - 1, face,
		shift(face, -8));

	const bool horizontal = orientation == B_HORIZONTAL;
	const float length = floorf((horizontal ? knob.Height() : knob.Width())
		/ 3);
	const BPoint middle(floorf((knob.left + knob.right) / 2),
		floorf((knob.top + knob.bottom) / 2));
	view->BeginLineArray(2);
	if (horizontal) {
		view->AddLine(BPoint(middle.x, middle.y - length / 2),
			BPoint(middle.x, middle.y + length / 2),
			outline_color(flags, false));
		view->AddLine(BPoint(middle.x + 1, middle.y - length / 2 + 1),
			BPoint(middle.x + 1, middle.y + length / 2), shift(face, 44));
	} else {
		view->AddLine(BPoint(middle.x - length / 2, middle.y),
			BPoint(middle.x + length / 2, middle.y),
			outline_color(flags, false));
		view->AddLine(BPoint(middle.x - length / 2 + 1, middle.y + 1),
			BPoint(middle.x + length / 2, middle.y + 1), shift(face, 44));
	}
	view->EndLineArray();
	view->PopState();

	rect = knob;
}


void
SmokeControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	ForwardingControlLook::DrawSliderTriangle(view, rect, updateRect, base, flags,
		orientation);
}


void
SmokeControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	ForwardingControlLook::DrawSliderTriangle(view, rect, updateRect, base, fill,
		flags, orientation);
}


//	#pragma mark - tabs


/*!	The strip behind the tabs, in the control colour, outlined in black
	with rounded corners away from the pane; the pane's black edge runs
	along it, broken by the selected tab.
*/
void
SmokeControlLook::DrawTabFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, border_style borderStyle, uint32 side)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(solid(view->LowColor(), view));
	view->FillRect(rect);

	// the strip's box, run past the pane's edge so that only its outer
	// corners are rounded
	BRect strip(rect);
	switch (side) {
		default:
		case BTabView::kTopSide:
			strip.bottom += kTabRadius * 2;
			break;
		case BTabView::kBottomSide:
			strip.top -= kTabRadius * 2;
			break;
		case BTabView::kLeftSide:
			strip.right += kTabRadius * 2;
			break;
		case BTabView::kRightSide:
			strip.left -= kTabRadius * 2;
			break;
	}

	view->ClipToRect(rect);
	const rgb_color outline = ui_color(B_CONTROL_BORDER_COLOR);
	if (borderStyle != B_NO_BORDER) {
		view->SetHighColor(outline);
		view->FillRoundRect(strip, kTabRadius, kTabRadius);
		strip.InsetBy(1, 1);
	}
	view->SetHighColor(ui_color(B_CONTROL_BACKGROUND_COLOR));
	view->FillRoundRect(strip, kTabRadius - 1, kTabRadius - 1);

	// the pane's edge
	if (borderStyle != B_NO_BORDER) {
		view->SetHighColor(outline);
		switch (side) {
			default:
			case BTabView::kTopSide:
				view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
				break;
			case BTabView::kBottomSide:
				view->StrokeLine(rect.LeftTop(), rect.RightTop());
				break;
			case BTabView::kLeftSide:
				view->StrokeLine(rect.RightTop(), rect.RightBottom());
				break;
			case BTabView::kRightSide:
				view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
				break;
		}
	}
	view->PopState();
}


void
SmokeControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawTab(view, rect, base, flags, side, true);
}


void
SmokeControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawTab(view, rect, base, flags, side, false);
}


//	#pragma mark - borders


void
SmokeControlLook::DrawBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, border_style borderStyle,
	uint32 flags, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	if (borderStyle == B_NO_BORDER)
		return;

	view->PushState();
	if (dynamic_cast<BMenuBar*>(view) != NULL) {
		// menu bars have no edge of their own
		view->SetHighColor(base);
		view->StrokeRect(rect);
		inset_by_borders(rect, 1, borders);
	} else {
		const rgb_color outline = shift(base, -58);
		view->BeginLineArray(4);
		if ((borders & B_TOP_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.RightTop(), outline);
		if ((borders & B_LEFT_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.LeftBottom(), outline);
		if ((borders & B_RIGHT_BORDER) != 0)
			view->AddLine(rect.RightTop(), rect.RightBottom(), outline);
		if ((borders & B_BOTTOM_BORDER) != 0)
			view->AddLine(rect.LeftBottom(), rect.RightBottom(), outline);
		view->EndLineArray();
		inset_by_borders(rect, 1, borders);

		if (borderStyle == B_FANCY_BORDER) {
			const rgb_color shade = shift(base, 21);
			const rgb_color light = shift(base, 69);
			view->BeginLineArray(4);
			if ((borders & B_TOP_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.RightTop(), shade);
			if ((borders & B_LEFT_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.LeftBottom(), shade);
			if ((borders & B_RIGHT_BORDER) != 0)
				view->AddLine(rect.RightTop(), rect.RightBottom(), light);
			if ((borders & B_BOTTOM_BORDER) != 0)
				view->AddLine(rect.LeftBottom(), rect.RightBottom(), light);
			view->EndLineArray();
			inset_by_borders(rect, 1, borders);
		}
	}
	view->PopState();
}


void
SmokeControlLook::DrawRaisedBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	const rgb_color light = shift(solid(base, view), 32);
	const rgb_color dark = shift(solid(base, view), -48);

	view->PushState();
	view->BeginLineArray(4);
	if ((borders & B_TOP_BORDER) != 0)
		view->AddLine(rect.LeftTop(), rect.RightTop(), light);
	if ((borders & B_LEFT_BORDER) != 0)
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), light);
	if ((borders & B_RIGHT_BORDER) != 0)
		view->AddLine(rect.RightTop(), rect.RightBottom(), dark);
	if ((borders & B_BOTTOM_BORDER) != 0)
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), dark);
	view->EndLineArray();
	view->PopState();
	inset_by_borders(rect, 1, borders);
}


/*!	A group box's single rounded line, or a tab view pane's black edge.
	Both take three pixels, like Haiku's.
*/
void
SmokeControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, uint32 borders)
{
	const rgb_color base = solid(_base, view);
	const bool tabView = dynamic_cast<BTabView*>(view) != NULL;
	const rgb_color line = tabView
		? ui_color(B_CONTROL_BORDER_COLOR) : shift(base, -48);

	view->PushState();
	if (borders == B_ALL_BORDERS && !tabView) {
		view->SetHighColor(base);
		view->FillRect(rect);
		view->SetHighColor(line);
		view->StrokeRoundRect(rect, kFieldRadius, kFieldRadius);
		rect.InsetBy(3, 3);
	} else {
		view->BeginLineArray(4);
		if ((borders & B_TOP_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.RightTop(), line);
		if ((borders & B_LEFT_BORDER) != 0)
			view->AddLine(rect.LeftTop(), rect.LeftBottom(), line);
		if ((borders & B_RIGHT_BORDER) != 0)
			view->AddLine(rect.RightTop(), rect.RightBottom(), line);
		if ((borders & B_BOTTOM_BORDER) != 0)
			view->AddLine(rect.LeftBottom(), rect.RightBottom(), line);
		view->EndLineArray();
		inset_by_borders(rect, 1, borders);

		view->SetHighColor(base);
		for (int32 i = 0; i < 2; i++) {
			view->BeginLineArray(4);
			if ((borders & B_TOP_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.RightTop(), base);
			if ((borders & B_LEFT_BORDER) != 0)
				view->AddLine(rect.LeftTop(), rect.LeftBottom(), base);
			if ((borders & B_RIGHT_BORDER) != 0)
				view->AddLine(rect.RightTop(), rect.RightBottom(), base);
			if ((borders & B_BOTTOM_BORDER) != 0)
				view->AddLine(rect.LeftBottom(), rect.RightBottom(), base);
			view->EndLineArray();
			inset_by_borders(rect, 1, borders);
		}
	}
	view->PopState();
}


/*!	A text field: the document colour in a rounded outline, with a shadow
	line inside its top and left. The outline takes the focus colour when
	focused, and the failure colour when the text is invalid.
*/
void
SmokeControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& _base, uint32 flags,
	uint32 borders)
{
	const rgb_color base = solid(_base, view);
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	rgb_color outline = outline_color(flags);
	if (!disabled && (flags & B_INVALID) != 0)
		outline = ui_color(B_FAILURE_COLOR);
	const rgb_color field = disabled ? panel_color() : base;
	const rgb_color shade = shift(field, -41);

	view->PushState();
	view->SetHighColor(solid(view->LowColor(), view));
	view->FillRect(rect);
	view->SetHighColor(outline);
	view->FillRoundRect(rect, kFieldRadius, kFieldRadius);
	BRect inside(rect.InsetByCopy(1, 1));
	view->SetHighColor(field);
	view->FillRoundRect(inside, kFieldRadius - 1, kFieldRadius - 1);

	view->BeginLineArray(2);
	view->AddLine(BPoint(inside.left + 2, inside.top),
		BPoint(inside.right - 2, inside.top), shade);
	view->AddLine(BPoint(inside.left, inside.top + 2),
		BPoint(inside.left, inside.bottom - 2), shade);
	view->EndLineArray();
	view->PopState();

	inset_by_borders(rect, 2, borders);
}


//	#pragma mark - private


/*!	Push buttons: a rounded black outline with a shadow below and right of
	it. The default button has another outline round it, two pixels out.
	Tool bar buttons show nothing until hovered.
*/
void
SmokeControlLook::_DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!rect.IsValid())
		return;

	const rgb_color behind = solid(background, view);

	view->PushState();
	view->ClipToRect(rect);

	if (is_flat(flags)) {
		view->SetHighColor(behind);
		view->FillRect(rect);
		view->PopState();
		inset_by_borders(rect, 2, borders);
		return;
	}

	if ((flags & B_DEFAULT_BUTTON) != 0) {
		view->SetHighColor(behind);
		view->FillRect(rect);
		view->SetHighColor(outline_color(flags, false));
		view->StrokeRoundRect(rect, kButtonRadius + 2, kButtonRadius + 2);
		rect.InsetBy(3, 3);
	}

	const BRect inside = draw_shadowed_outline(view, rect, kButtonRadius,
		behind, outline_color(flags), flags);
	fill_face(view, inside, kButtonRadius - 1, base, flags);
	view->PopState();

	rect.InsetBy(2, 2);
}


/*!	The face, shading down; the frame drew it already, from a pixel
	further out, so it is filled again over the same span. A pop-up button
	has a small white arrow at its right end.
*/
void
SmokeControlLook::_DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, bool popUp)
{
	if (!rect.IsValid() || !ShouldDraw(view, rect, updateRect))
		return;

	const bool flat = is_flat(flags);
	const bool pressed = (flags & (B_ACTIVATED | B_CLICKED)) != 0 && !flat;

	view->PushState();
	if (flat) {
		view->SetHighColor(solid(view->LowColor(), view));
		view->FillRect(rect);
	} else {
		BRect inside(rect.left - 1, rect.top - 1, rect.right, rect.bottom);
		view->ClipToRect(rect);
		fill_face(view, inside, kButtonRadius - 1, base, flags);
	}

	inset_by_borders(rect, 1, borders);

	if (popUp) {
		const float width = ComposeSpacing(B_USE_ITEM_SPACING);
		BRect arrow(rect.right - width + 1, rect.top, rect.right - 1,
			rect.bottom);
		if (pressed)
			arrow.OffsetBy(1, 1);
		const float middle = floorf((arrow.left + arrow.right) / 2);
		const float top = floorf((arrow.top + arrow.bottom) / 2) - 1;
		view->SetHighColor((flags & B_DISABLED) != 0
			? shift(panel_color(), -40) : ui_color(B_CONTROL_TEXT_COLOR));
		for (int32 i = 0; i < 3; i++) {
			view->FillRect(BRect(middle - 2 + i, top + i, middle + 2 - i,
				top + i));
		}
		rect.right -= width;
	}
	view->PopState();

	if (pressed)
		rect.OffsetBy(1, 1);
}


/*!	The inside of a menu field: a face shading down, and the pop-up
	section, in the menu selection colour, past a black line at its right
	end, with a white arrow in it.
*/
void
SmokeControlLook::_DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	// the outline's inside, as the frame left it
	const BRect inside(rect.left - 1, rect.top - 1, rect.right, rect.bottom);

	view->PushState();
	view->ClipToRect(rect);
	if (disabled) {
		fill_gradient(view, inside, kFieldRadius - 1, panel_color(),
			shift(panel_color(), -8));
	} else {
		fill_gradient(view, inside, kFieldRadius - 1, shift(base, 8),
			shift(base, -20));
	}

	BRect text(rect);
	if (popupIndicator) {
		const float indicator = ComposeSpacing(B_USE_ITEM_SPACING);
		const float width = std::max(9.0f, indicator - 4);
		BRect section(rect.right - width + 1, inside.top, inside.right,
			inside.bottom);

		const rgb_color selected = disabled
			? shift(panel_color(), -16)
			: ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
		fill_gradient(view, section.InsetByCopy(0, 0), kFieldRadius - 1,
			mix(selected, make_color(255, 255, 255), 0.15f),
			shift(selected, -34));
		// square off the section's left corners again
		fill_gradient(view, BRect(section.left, section.top,
			section.left + kFieldRadius, section.bottom), 0,
			mix(selected, make_color(255, 255, 255), 0.15f),
			shift(selected, -34));

		view->SetHighColor(outline_color(flags, false));
		view->StrokeLine(BPoint(section.left - 1, inside.top),
			BPoint(section.left - 1, inside.bottom));

		// the arrow: 5 x 3 pixels, pointing down
		const float middle = floorf((section.left + section.right) / 2);
		const float top = floorf((section.top + section.bottom) / 2) - 1;
		view->SetHighColor(disabled ? shift(panel_color(), 24)
			: ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
		for (int32 i = 0; i < 3; i++) {
			view->FillRect(BRect(middle - 2 + i, top + i, middle + 2 - i,
				top + i));
		}
		text.right = std::min(section.left - 2, rect.right - indicator + 1);
	}
	view->PopState();

	text.InsetBy(1, 1);
	rect = text;
}


/*!	A tab, for any side of the tab view. Coordinates run along the tab's
	base (u) and from its outer edge in towards the pane (v). The selected
	tab is outlined and rounded, and runs into the pane over its edge; the
	others are the strip itself, with a shadow line before them.
*/
void
SmokeControlLook::_DrawTab(BView* view, BRect& rect,
	const rgb_color& base, uint32 flags, uint32 side, bool active)
{
	rect.left = floorf(rect.left);
	rect.right = floorf(rect.right);
	rect.top = floorf(rect.top);
	rect.bottom = floorf(rect.bottom);

	const bool vertical = side == BTabView::kLeftSide
		|| side == BTabView::kRightSide;
	const float length = vertical ? rect.Height() : rect.Width();
	const float depth = vertical ? rect.Width() : rect.Height();

	// a rect from tab coordinates
	auto area = [&](float u0, float v0, float u1, float v1) {
		BRect result;
		switch (side) {
			default:
			case BTabView::kTopSide:
				result.Set(rect.left + u0, rect.top + v0, rect.left + u1,
					rect.top + v1);
				break;
			case BTabView::kBottomSide:
				result.Set(rect.left + u0, rect.bottom - v1, rect.left + u1,
					rect.bottom - v0);
				break;
			case BTabView::kLeftSide:
				result.Set(rect.left + v0, rect.top + u0, rect.left + v1,
					rect.top + u1);
				break;
			case BTabView::kRightSide:
				result.Set(rect.right - v1, rect.top + u0, rect.right - v0,
					rect.top + u1);
				break;
		}
		return result;
	};

	const float top = 3;
	const rgb_color panel = panel_color();

	view->PushState();
	view->ClipToRect(rect);

	if (active) {
		// rounded at its outer corners only: the box runs on past the tab
		BRect box = area(0, top, length, depth + kTabRadius * 2);
		view->SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
		view->FillRoundRect(box, kTabRadius, kTabRadius);

		BRect inside = area(1, top + 1, length - 1, depth + kTabRadius * 2);
		BGradientLinear gradient;
		gradient.AddColor(shift(panel, 23), 0);
		gradient.AddColor(panel, 255);
		const BRect span = area(1, top + 1, length - 1, depth);
		switch (side) {
			default:
			case BTabView::kTopSide:
				gradient.SetStart(span.LeftTop());
				gradient.SetEnd(span.LeftBottom());
				break;
			case BTabView::kBottomSide:
				gradient.SetStart(span.LeftBottom());
				gradient.SetEnd(span.LeftTop());
				break;
			case BTabView::kLeftSide:
				gradient.SetStart(span.LeftTop());
				gradient.SetEnd(span.RightTop());
				break;
			case BTabView::kRightSide:
				gradient.SetStart(span.RightTop());
				gradient.SetEnd(span.LeftTop());
				break;
		}
		view->FillRoundRect(inside, kTabRadius - 1, kTabRadius - 1, gradient);
	} else {
		// a shadow line before the tab, curving in at its outer end
		const rgb_color shadow = shift(panel, -33);
		view->SetHighColor(shadow);
		view->FillRect(area(1, top + 3, 1, depth - 1));
		view->FillRect(area(2, top + 1, 2, top + 2));
		view->SetHighColor(mix(shadow, ui_color(B_CONTROL_BACKGROUND_COLOR),
			0.5f));
		view->FillRect(area(3, top, 4, top));
	}

	view->PopState();

	// the label's room
	rect = area(3, top + 1, length - 3, depth - 1);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::SmokeControlLook();
}
