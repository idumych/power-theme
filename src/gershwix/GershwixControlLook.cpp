/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See GershwixControlLook.h. Colours and sizes were measured from Gershwix 2.5's
 * pictures of its controls.
 */


#include "GershwixControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }
#define RGB(hex) { (uint8)((hex) >> 16), (uint8)((hex) >> 8), (uint8)(hex), 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);


/*!	An accent's shades: a thumb's face, its lit and shaded edges, the light and
	dark lines of its ridges, and a progress bar's rows from top to bottom,
	shaded like a tube.
*/
struct Accent {
	rgb_color	face;
	rgb_color	light;
	rgb_color	dark;
	rgb_color	ridgeLight;
	rgb_color	ridge;
	rgb_color	ridgeDark;
	rgb_color	tube[12];
};

static const int32 kTubeRows = 12;

// Gershwix's seven, keyed by their thumb's face, which their themes give as the
// scroll bar thumb colour
static const Accent kAccents[] = {
	// Ivy
	{ RGB(0x6a9c67), RGB(0x82be7e), RGB(0x517f4e),
		RGB(0xa3cf9f), RGB(0x8dbf89), RGB(0x41663e),
		{ RGB(0x41663e), RGB(0x527d4e), RGB(0x6a9a66), RGB(0x8dbf89),
			RGB(0xa3cf9f), RGB(0xcccccc), RGB(0xa3cf9f), RGB(0x8dbf89),
			RGB(0x6a9a66), RGB(0x527d4e), RGB(0x41663e), RGB(0x1e401b) } },
	// French Blue
	{ RGB(0x8786be), RGB(0xa5a4e8), RGB(0x68679b),
		RGB(0xccccff), RGB(0xb0b0ea), RGB(0x51517c),
		{ RGB(0x51517c), RGB(0x666699), RGB(0x8585bc), RGB(0xb0b0ea),
			RGB(0xccccff), RGB(0xffffff), RGB(0xccccff), RGB(0xb0b0ea),
			RGB(0x8585bc), RGB(0x666699), RGB(0x51517c), RGB(0x26264d) } },
	// Poppy
	{ RGB(0xff8704), RGB(0xffa23e), RGB(0xce6c00),
		RGB(0xffb770), RGB(0xffa449), RGB(0xa45200),
		{ RGB(0xa45200), RGB(0xcc6600), RGB(0xff8001), RGB(0xffa449),
			RGB(0xffb770), RGB(0xe6e6e6), RGB(0xffb770), RGB(0xffa449),
			RGB(0xff8001), RGB(0xcc6600), RGB(0xa45200), RGB(0x5c2e00) } },
	// Plum
	{ RGB(0xbe86bd), RGB(0xe8a4e7), RGB(0x9b679a),
		RGB(0xffccff), RGB(0xeab0ea), RGB(0x7c517c),
		{ RGB(0x7c517c), RGB(0x996699), RGB(0xbc85bc), RGB(0xeab0ea),
			RGB(0xffccff), RGB(0xffffff), RGB(0xffccff), RGB(0xeab0ea),
			RGB(0xbc85bc), RGB(0x996699), RGB(0x7c517c), RGB(0x4d264d) } },
	// Crimson
	{ RGB(0xe7605d), RGB(0xff8f8d), RGB(0xc14340),
		RGB(0xffcccc), RGB(0xff9b9b), RGB(0x9b3131),
		{ RGB(0x9b3131), RGB(0xbe4040), RGB(0xe45c5c), RGB(0xff9b9b),
			RGB(0xffcccc), RGB(0xffffff), RGB(0xffcccc), RGB(0xff9b9b),
			RGB(0xe45c5c), RGB(0xbe4040), RGB(0x9b3131), RGB(0x690909) } },
	// Sunny
	{ RGB(0xffe473), RGB(0xffeea9), RGB(0xedcc54),
		RGB(0xfff7d9), RGB(0xffefb4), RGB(0xcaaf4d),
		{ RGB(0xcaaf4d), RGB(0xe9c854), RGB(0xffe071), RGB(0xffefb4),
			RGB(0xfff7d9), RGB(0xffffff), RGB(0xfff7d9), RGB(0xffefb4),
			RGB(0xffe071), RGB(0xe9c854), RGB(0xcaaf4d), RGB(0xa38a31) } },
	// Silver
	{ RGB(0xa2a2a2), RGB(0xc6c6c6), RGB(0x818181),
		RGB(0xe5e5e5), RGB(0xcdcdcd), RGB(0x666666),
		{ RGB(0x666666), RGB(0x7f7f7f), RGB(0xa0a0a0), RGB(0xcdcdcd),
			RGB(0xe5e5e5), RGB(0xffffff), RGB(0xe5e5e5), RGB(0xcdcdcd),
			RGB(0xa0a0a0), RGB(0x7f7f7f), RGB(0x666666), RGB(0x393939) } },
};
static const int32 kAccentCount = sizeof(kAccents) / sizeof(kAccents[0]);


/*!	A bevel's greys: its outline and the outline's rounded corners, and inside
	it a lit top and left, a face, a shaded line along the right and bottom and
	a darker edge outside that.
*/
struct Bevel {
	uint8	outline;
	uint8	corner;
	uint8	light;
	uint8	face;
	uint8	shade;
	uint8	edge;
};

static const Bevel kButton = { 0x80, 0x99, 0xff, 0xeb, 0xcc, 0xb2 };
static const Bevel kButtonPressed = { 0x80, 0x99, 0xe5, 0xcd, 0xb3, 0x99 };
static const Bevel kButtonDisabled = { 0xb0, 0xcc, 0xdd, 0xdd, 0xdd, 0xdd };

// check boxes: an outline, and a face lit from the top left
struct Box {
	uint8	outline;
	uint8	light;
	uint8	face;
	uint8	shade;
	uint8	corner;
	uint8	mark;
};

static const Box kCheckBox = { 0x66, 0xff, 0xeb, 0xb3, 0xee, 0x2f };
static const Box kCheckBoxPressed = { 0x66, 0xcc, 0xb9, 0x90, 0xb9, 0x2f };
static const Box kCheckBoxDisabled = { 0xa6, 0xf0, 0xe3, 0xd0, 0xe8, 0x9a };

// a radio button is a ball lit from the top, its rows top to bottom
static const uint8 kBall[] = { 0xe4, 0xf0, 0xee, 0xed, 0xeb, 0xd8, 0xc1, 0xac,
	0x9d, 0x8d };
static const uint8 kBallPressed[] = { 0xa0, 0xa8, 0xa7, 0xa6, 0xa5, 0x98, 0x87,
	0x79, 0x6e, 0x63 };
static const uint8 kBallDisabled[] = { 0xf2, 0xf6, 0xf5, 0xf5, 0xf4, 0xeb, 0xe0,
	0xd6, 0xce, 0xc7 };
static const int32 kBallRows = sizeof(kBall) / sizeof(kBall[0]);

// the scroll bar's track, across it from its left or top
static const uint8 kTrack[] = { 0xb2, 0xba, 0xc2, 0xcc };
static const uint8 kTrackFar[] = { 0xcc, 0xd9, 0xe6, 0xb2 };
static const rgb_color kScrollBorder = GREY(0xb2);
static const rgb_color kArrowLight = GREY(0xff);
static const rgb_color kArrowFace = GREY(0xe5);
static const rgb_color kArrowPressedLight = GREY(0xc3);
static const rgb_color kArrowPressedFace = GREY(0xa9);
static const rgb_color kArrowPressedShade = GREY(0x76);

// a slider's groove, top to bottom, and the shadow under it
static const uint8 kGroove[] = { 0x56, 0x76, 0x92, 0xa6, 0xb0 };
static const int32 kGrooveRows = sizeof(kGroove) / sizeof(kGroove[0]);

// a progress bar's sunken track, top to bottom
static const rgb_color kWellLine = GREY(0x9a);
static const uint8 kWellTop[] = { 0xb4, 0xbd };
static const uint8 kWellFace = 0xcd;
static const uint8 kWellBottom[] = { 0xe5, 0xff };

// text fields: a sunken frame of two lines
static const rgb_color kFieldOuterTop = GREY(0x7c);
static const rgb_color kFieldOuterSide = GREY(0xc3);
static const rgb_color kFieldOuterBottom = GREY(0xdd);
static const rgb_color kFieldInnerTop = GREY(0xc3);
static const rgb_color kFieldInnerSide = GREY(0xef);
static const rgb_color kFieldInnerBottom = GREY(0xfe);

static const rgb_color kGroupLine = GREY(0x8a);
static const rgb_color kMenuEdge = GREY(0x99);
static const rgb_color kMenuBarLine = GREY(0xa6);

// Tracker's column headers
static const rgb_color kHeaderLine = GREY(0xa5);
static const rgb_color kHeaderLight = GREY(0xff);
static const rgb_color kHeaderFace = GREY(0xe7);
static const rgb_color kHeaderShade = GREY(0xc1);

// a tab behind the chosen one: darker and flat
static const uint8 kBackTabFace = 0xb9;
static const uint8 kBackTabLine = 0x82;

static const float kIndicatorWidth = 18;


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


/*!	The accent for a face colour: one of Gershwix's, or one shaded the way they
	are.
*/
static Accent
accent_for(rgb_color face)
{
	for (int32 i = 0; i < kAccentCount; i++) {
		if (same_color(kAccents[i].face, face))
			return kAccents[i];
	}

	Accent accent;
	accent.face = face;
	accent.light = mix(face, kWhite, 0.25f);
	accent.dark = mix(face, kBlack, 0.2f);
	accent.ridgeLight = mix(face, kWhite, 0.55f);
	accent.ridge = mix(face, kWhite, 0.4f);
	accent.ridgeDark = mix(face, kBlack, 0.4f);
	const rgb_color tube[kTubeRows] = {
		accent.ridgeDark, mix(face, kBlack, 0.2f), face, accent.ridge,
		accent.ridgeLight, mix(face, kWhite, 0.85f), accent.ridgeLight,
		accent.ridge, face, mix(face, kBlack, 0.2f), accent.ridgeDark,
		mix(face, kBlack, 0.65f)
	};
	for (int32 i = 0; i < kTubeRows; i++)
		accent.tube[i] = tube[i];
	return accent;
}


static Accent
theme_accent()
{
	return accent_for(ui_color(B_SCROLL_BAR_THUMB_COLOR));
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


static void
fill_line(BView* view, float left, float top, float right, float bottom,
	uint8 level)
{
	fill_line(view, left, top, right, bottom, grey(level));
}


/*!	A bevel's inside: a lit top two rows and left two columns, a shaded column
	and row along the right and bottom, and an edge outside those. Rounded ends
	put the outline's corner grey in their corners.
*/
static void
draw_bevel_face(BView* view, BRect r, const Bevel& bevel, bool roundLeft = true,
	bool roundRight = true)
{
	if (r.Width() < 3 || r.Height() < 3)
		return;

	const float l = r.left, t = r.top, rr = r.right, b = r.bottom;
	fill_line(view, l, t, rr, b, bevel.face);
	fill_line(view, l, t, rr, t, bevel.light);
	fill_line(view, l, t + 1, rr - 2, t + 1, bevel.light);
	fill_line(view, rr - 1, t + 1, rr - 1, t + 1, bevel.face);
	fill_line(view, l, t + 1, l + 1, b - 1, bevel.light);
	fill_line(view, rr - 1, t + 2, rr - 1, b - 1, bevel.shade);
	fill_line(view, rr, t + 1, rr, b - 1, bevel.edge);
	fill_line(view, l + 2, b - 1, rr - 1, b - 1, bevel.shade);
	fill_line(view, l + 1, b - 1, l + 1, b - 1, bevel.face);
	fill_line(view, l, b, rr, b, bevel.edge);
	if (roundLeft) {
		fill_line(view, l, t, l, t, bevel.corner);
		fill_line(view, l, b, l, b, bevel.corner);
	}
	if (roundRight) {
		fill_line(view, rr, t, rr, t, bevel.corner);
		fill_line(view, rr, b, rr, b, bevel.corner);
	}
}


/*!	A bevelled button: an outline whose corners are cut and softened, round its
	inside. A square end has a plain outline, for where two buttons butt.
*/
static void
draw_bevel(BView* view, BRect r, const Bevel& bevel, bool roundLeft = true,
	bool roundRight = true)
{
	const float l = r.left, t = r.top, rr = r.right, b = r.bottom;
	const float lineLeft = roundLeft ? l + 2 : l;
	const float lineRight = roundRight ? rr - 2 : rr;
	fill_line(view, lineLeft, t, lineRight, t, bevel.outline);
	fill_line(view, lineLeft, b, lineRight, b, bevel.outline);
	fill_line(view, l, roundLeft ? t + 2 : t, l, roundLeft ? b - 2 : b,
		bevel.outline);
	fill_line(view, rr, roundRight ? t + 2 : t, rr, roundRight ? b - 2 : b,
		bevel.outline);
	if (roundLeft) {
		fill_line(view, l + 1, t, l + 1, t, bevel.corner);
		fill_line(view, l, t + 1, l, t + 1, bevel.corner);
		fill_line(view, l + 1, b, l + 1, b, bevel.corner);
		fill_line(view, l, b - 1, l, b - 1, bevel.corner);
	}
	if (roundRight) {
		fill_line(view, rr - 1, t, rr - 1, t, bevel.corner);
		fill_line(view, rr, t + 1, rr, t + 1, bevel.corner);
		fill_line(view, rr - 1, b, rr - 1, b, bevel.corner);
		fill_line(view, rr, b - 1, rr, b - 1, bevel.corner);
	}
	draw_bevel_face(view, r.InsetByCopy(1, 1), bevel, roundLeft, roundRight);
}


static const Bevel&
button_bevel(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kButtonDisabled;
	if ((flags & (BControlLook::B_ACTIVATED | BControlLook::B_PARTIALLY_ACTIVATED
			| BControlLook::B_CLICKED)) != 0)
		return kButtonPressed;
	return kButton;
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
		case BControlLook::B_DOWN_ARROW:
			points[0] = BPoint(x - half, y - base);
			points[1] = BPoint(x + half, y - base);
			points[2] = BPoint(x, y + base);
			break;
		case BControlLook::B_LEFT_ARROW:
			points[0] = BPoint(x + base, y - half);
			points[1] = BPoint(x + base, y + half);
			points[2] = BPoint(x - base, y);
			break;
		default:
			points[0] = BPoint(x - base, y - half);
			points[1] = BPoint(x - base, y + half);
			points[2] = BPoint(x + base, y);
			break;
	}
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(color);
	view->FillTriangle(points[0], points[1], points[2]);
	view->PopState();
}


/*!	Ridges across a thumb, each a light line with a dark one under it shifted a
	pixel along, centred on the thumb.
*/
static void
draw_ridges(BView* view, const BRect& thumb, const Accent& accent, int32 count,
	float length, orientation orientation)
{
	const float span = 2 * count;
	if (orientation == B_VERTICAL) {
		const float x = floorf((thumb.left + thumb.right - length) / 2);
		float y = floorf((thumb.top + thumb.bottom - span) / 2) + 1;
		for (int32 i = 0; i < count; i++, y += 2) {
			fill_line(view, x, y, x, y, accent.ridgeLight);
			fill_line(view, x + 1, y, x + length - 1, y, accent.ridge);
			fill_line(view, x + 1, y + 1, x + length, y + 1, accent.ridgeDark);
		}
	} else {
		const float y = floorf((thumb.top + thumb.bottom - length) / 2);
		float x = floorf((thumb.left + thumb.right - span) / 2) + 1;
		for (int32 i = 0; i < count; i++, x += 2) {
			fill_line(view, x, y, x, y, accent.ridgeLight);
			fill_line(view, x, y + 1, x, y + length - 1, accent.ridge);
			fill_line(view, x + 1, y + 1, x + 1, y + length, accent.ridgeDark);
		}
	}
}


//	#pragma mark -


GershwixControlLook::GershwixControlLook()
{
}


GershwixControlLook::~GershwixControlLook()
{
}


//	#pragma mark - buttons


/*!	The whole bevel, so that its face fills the gap Haiku leaves inside the
	frame; the background draws the face again. A default button has a dark ring
	round it, as Platinum's does.
*/
void
GershwixControlLook::DrawButtonFrame(BView* view, BRect& rect,
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

	BRect shape(rect);
	if (isDefault) {
		const bool disabled = (flags & B_DISABLED) != 0;
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(disabled ? grey(0xb0) : grey(0x55));
		view->FillRoundRect(shape, 4, 4);
		view->SetHighColor(disabled ? grey(0xdd) : grey(0xa6));
		view->FillRoundRect(shape.InsetByCopy(1, 1), 3, 3);
		view->SetHighColor(background);
		view->FillRect(shape.InsetByCopy(3, 3));
		view->SetDrawingMode(B_OP_COPY);
		shape.InsetBy(3, 3);
	}

	draw_bevel(view, shape, button_bevel(flags),
		(borders & B_LEFT_BORDER) != 0, (borders & B_RIGHT_BORDER) != 0);

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
GershwixControlLook::DrawButtonBackground(BView* view, BRect& rect,
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
		const bool active = (flags & B_ACTIVATED) != 0;
		const float darker = active ? 0.1f : 0;
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			mix(kHeaderFace, kBlack, darker));
		fill_line(view, rect.left, rect.top, rect.right, rect.top,
			mix(kHeaderLight, kBlack, darker));
		fill_line(view, rect.left, rect.bottom - 1, rect.right, rect.bottom - 1,
			mix(kHeaderShade, kBlack, darker));
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kHeaderLine);
		view->PopState();
		return;
	}

	// the inside the frame drew, as it drew it
	draw_bevel_face(view, rect.InsetByCopy(-1, -1), button_bevel(flags),
		(borders & B_LEFT_BORDER) != 0, (borders & B_RIGHT_BORDER) != 0);

	view->PopState();

	rect.InsetBy(1, 1);
}


void
GershwixControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	BRect area(rect);
	DrawButtonBackground(view, rect, updateRect, base, flags, borders,
		orientation);

	view->PushState();
	view->ClipToRect(area);
	draw_triangle(view, BRect(area.right - 12, area.top, area.right - 2,
		area.bottom), B_DOWN_ARROW, (flags & B_DISABLED) != 0
			? grey(0xa6) : kBlack, 3.5f, 4);
	view->PopState();
}


//	#pragma mark - menus


void
GershwixControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
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

	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	fill_line(view, rect.left, rect.top, rect.right, rect.top, kWhite);
	if ((borders & B_BOTTOM_BORDER) != 0) {
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kMenuBarLine);
	}

	view->PopState();
}


/*!	A bevel, with a second one butted against its right end holding two arrows,
	and returns the first one's rectangle.
*/
static BRect
draw_menu_field(BView* view, const BRect& shape, uint32 flags,
	bool popupIndicator)
{
	const bool disabled = (flags & BControlLook::B_DISABLED) != 0;
	const bool open = (flags & (BControlLook::B_ACTIVATED
		| BControlLook::B_CLICKED)) != 0;
	const Bevel& bevel = disabled ? kButtonDisabled
		: open ? kButtonPressed : kButton;

	if (!popupIndicator) {
		draw_bevel(view, shape, bevel);
		return shape;
	}

	BRect indicator(shape);
	indicator.left = indicator.right - kIndicatorWidth + 1;
	BRect label(shape);
	label.right = indicator.left - 1;
	draw_bevel(view, label, disabled ? kButtonDisabled : kButton, true, false);
	draw_bevel(view, indicator, bevel, false, true);

	const rgb_color arrows = disabled ? grey(0xa6) : kBlack;
	const float middle = floorf((indicator.top + indicator.bottom) / 2);
	BRect arrow(indicator.left + 1, middle - 6, indicator.right - 1, middle - 1);
	draw_triangle(view, arrow, BControlLook::B_UP_ARROW, arrows, 3.5f, 4);
	arrow.OffsetBy(0, 7);
	draw_triangle(view, arrow, BControlLook::B_DOWN_ARROW, arrows, 3.5f, 4);
	return label;
}


/*!	The whole field. Haiku draws the background in the field's menu bar, a view
	two pixels inside this rectangle, so the bevels' outer lines have to be drawn
	here; the background draws the rest again.
*/
void
GershwixControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		draw_menu_field(view, rect, flags, true);
		view->PopState();
	}
	// Haiku's inset, which the label is laid out in
	rect.InsetBy(2, 2);
}


void
GershwixControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// the bevel goes back out to where the frame would have been
	const BRect shape(rect.InsetByCopy(-2, -2));
	view->ClipToRect(shape);
	const BRect label = draw_menu_field(view, shape, flags, popupIndicator);

	view->PopState();

	if (popupIndicator)
		rect.right = label.right - 2;
}


void
GershwixControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


void
GershwixControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if ((borders & B_LEFT_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.left, rect.bottom, kMenuEdge);
	if ((borders & B_RIGHT_BORDER) != 0)
		fill_line(view, rect.right, rect.top, rect.right, rect.bottom, kMenuEdge);
	if ((borders & B_TOP_BORDER) != 0)
		fill_line(view, rect.left, rect.top, rect.right, rect.top, kMenuEdge);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom, kMenuEdge);
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


//!	A selected item, in a menu or the menu bar, is the accent's dark shade, flat.
void
GershwixControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
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


/*!	A sunken track, and in it a bar in the accent's shades, lit along its
	middle like a tube.
*/
void
GershwixControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// the track, a line round a face darker at its top
	view->SetHighColor(kWellLine);
	view->StrokeRect(rect);
	const BRect well(rect.InsetByCopy(1, 1));
	fill_line(view, well.left, well.top, well.right, well.bottom, kWellFace);
	for (int32 i = 0; i < 2; i++) {
		fill_line(view, well.left, well.top + i, well.right, well.top + i,
			kWellTop[i]);
		fill_line(view, well.left + i, well.top, well.left + i, well.bottom,
			kWellTop[i]);
		fill_line(view, well.left, well.bottom - 1 + i, well.right,
			well.bottom - 1 + i, kWellBottom[i]);
		fill_line(view, well.right - 1 + i, well.top + 2, well.right - 1 + i,
			well.bottom, kWellBottom[i]);
	}

	if (progressPosition > well.left) {
		BRect bar(well);
		bar.right = std::min(progressPosition, well.right);
		// the theme's own colour has Gershwix's shades; an application's own
		// is shaded the same way
		const Accent accent = same_color(barColor, ui_color(B_STATUS_BAR_COLOR))
			? theme_accent() : accent_for(barColor);
		const float height = bar.Height() + 1;
		for (float y = bar.top; y <= bar.bottom; y++) {
			const int32 row = std::min(kTubeRows - 1,
				(int32)((y - bar.top) * kTubeRows / height));
			fill_line(view, bar.left, y, bar.right, y, accent.tube[row]);
		}
		fill_line(view, bar.right, bar.top, bar.right, bar.bottom,
			accent.ridgeDark);
	}

	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


/*!	A square with a dark outline and a face lit from its top left. A tick
	reaches up out of its top right.
*/
void
GershwixControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0;
	const Box& box = disabled ? kCheckBoxDisabled
		: down ? kCheckBoxPressed : kCheckBox;

	view->PushState();

	BRect r(rect);
	r.bottom = r.top + r.Width();
	view->SetHighColor(grey(box.outline));
	view->StrokeRect(r);
	const BRect f(r.InsetByCopy(1, 1));
	fill_line(view, f.left, f.top, f.right, f.bottom, box.face);
	fill_line(view, f.left, f.top, f.right - 1, f.top, box.light);
	fill_line(view, f.left, f.top, f.left, f.bottom - 1, box.light);
	fill_line(view, f.right, f.top + 1, f.right, f.bottom, box.shade);
	fill_line(view, f.left + 1, f.bottom, f.right, f.bottom, box.shade);
	fill_line(view, f.right, f.top, f.right, f.top, box.corner);
	fill_line(view, f.left, f.bottom, f.left, f.bottom, box.corner);

	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(grey(box.mark));
	const float size = r.Width() + 1;
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf((r.top + r.bottom) / 2);
		view->FillRect(BRect(r.left + roundf(size * 0.25f), middle,
			r.right - roundf(size * 0.25f), middle + 1));
	} else if ((flags & B_ACTIVATED) != 0) {
		view->SetPenSize(std::max(2.0f, size / 6));
		view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
		const BPoint start(r.left + size * 0.27f, r.top + size * 0.33f);
		const BPoint low(r.left + size * 0.5f, r.top + size * 0.72f);
		const BPoint end(r.left + size * 0.86f, r.top - size * 0.02f);
		view->StrokeLine(start, low);
		view->StrokeLine(low, end);
	}
	view->PopState();
}


//!	A ball with a dark edge, lit from the top. A dark dot in it when it's on.
void
GershwixControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & B_CLICKED) != 0;
	const uint8* rows = disabled ? kBallDisabled : down ? kBallPressed : kBall;

	BRect ball(rect);
	ball.bottom = ball.top + ball.Width();

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	view->SetHighColor(grey(disabled ? 0xa6 : down ? 0x48 : 0x66));
	view->FillEllipse(ball);

	const BRect face(ball.InsetByCopy(1, 1));
	BGradientLinear gradient;
	for (int32 i = 0; i < kBallRows; i++)
		gradient.AddColor(grey(rows[i]), i * 255 / (kBallRows - 1));
	gradient.SetStart(face.LeftTop());
	gradient.SetEnd(face.LeftBottom());
	view->FillEllipse(face, gradient);

	if ((flags & B_ACTIVATED) != 0) {
		view->SetHighColor(grey(disabled ? 0x9a : 0x2f));
		const float inset = roundf((ball.Width() + 1) * 0.29f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
	}
	view->PopState();
}


//	#pragma mark - scroll bars


void
GershwixControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetHighColor(kScrollBorder);
	view->StrokeRect(rect);
	view->PopState();
}


//!	A light face with a white edge at its top left, and a black arrow.
void
GershwixControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
		down ? kArrowPressedFace : kArrowFace);
	fill_line(view, rect.left, rect.top, rect.right, rect.top,
		down ? kArrowPressedLight : kArrowLight);
	fill_line(view, rect.left, rect.top, rect.left, rect.bottom,
		down ? kArrowPressedLight : kArrowLight);
	if (down) {
		fill_line(view, rect.right, rect.top, rect.right, rect.bottom,
			kArrowPressedShade);
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kArrowPressedShade);
	}

	// the line between it and the track
	switch (direction) {
		case B_UP_ARROW:
			fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
				kScrollBorder);
			break;
		case B_DOWN_ARROW:
			fill_line(view, rect.left, rect.top, rect.right, rect.top,
				kScrollBorder);
			break;
		case B_LEFT_ARROW:
			fill_line(view, rect.right, rect.top, rect.right, rect.bottom,
				kScrollBorder);
			break;
		case B_RIGHT_ARROW:
			fill_line(view, rect.left, rect.top, rect.left, rect.bottom,
				kScrollBorder);
			break;
	}

	draw_triangle(view, rect, direction, disabled ? grey(0x99) : kBlack, 3.5f,
		6);

	view->PopState();
}


void
GershwixControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


/*!	A track shaded across, darkest along the side the light comes from, with a
	lighter line along the far side.
*/
void
GershwixControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const int32 count = sizeof(kTrack) / sizeof(kTrack[0]);
	if (orientation == B_VERTICAL) {
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			kTrack[count - 1]);
		for (int32 i = 0; i < count - 1; i++) {
			fill_line(view, rect.left + i, rect.top, rect.left + i, rect.bottom,
				kTrack[i]);
			fill_line(view, rect.right - (count - 2) + i, rect.top,
				rect.right - (count - 2) + i, rect.bottom, kTrackFar[i + 1]);
		}
	} else {
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom,
			kTrack[count - 1]);
		for (int32 i = 0; i < count - 1; i++) {
			fill_line(view, rect.left, rect.top + i, rect.right, rect.top + i,
				kTrack[i]);
			fill_line(view, rect.left, rect.bottom - (count - 2) + i, rect.right,
				rect.bottom - (count - 2) + i, kTrackFar[i + 1]);
		}
	}

	view->PopState();
}


/*!	A thumb in the accent colour, lit along its top and left and shaded along
	its bottom and right, with six ridges across its middle. A bar with nothing
	to scroll has none.
*/
void
GershwixControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// the track runs on under it
	BRect track(rect);
	DrawScrollBarBackground(view, track, updateRect, base, flags, orientation);

	if ((flags & B_DISABLED) == 0) {
		BRect t(rect);
		if (orientation == B_VERTICAL)
			t.InsetBy(1, 0);
		else
			t.InsetBy(0, 1);

		Accent accent = theme_accent();
		if ((flags & B_ACTIVATED) != 0) {
			accent.face = accent.dark;
			accent.light = accent.face;
		}

		fill_line(view, t.left, t.top, t.right, t.bottom, accent.face);
		fill_line(view, t.left, t.top, t.right - 1, t.top, accent.light);
		fill_line(view, t.left, t.top, t.left, t.bottom - 1, accent.light);
		fill_line(view, t.left + 1, t.bottom, t.right, t.bottom, accent.dark);
		fill_line(view, t.right, t.top + 1, t.right, t.bottom, accent.dark);

		const float across = orientation == B_VERTICAL ? t.Width() : t.Height();
		const float along = orientation == B_VERTICAL ? t.Height() : t.Width();
		if (along >= 20)
			draw_ridges(view, t, accent, 6, floorf(across / 2), orientation);
	}

	view->PopState();
}


float
GershwixControlLook::GetScrollBarWidth(orientation orientation)
{
	return 15;
}


//	#pragma mark - sliders


//!	A thin rounded groove, darkest at its top, in the middle of what it's given.
void
GershwixControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 2;
		bar.bottom = bar.top + kGrooveRows - 1;
		for (int32 i = 0; i < kGrooveRows; i++) {
			const float inset = i == 0 || i == kGrooveRows - 1 ? 1 : 0;
			fill_line(view, bar.left + inset, bar.top + i, bar.right - inset,
				bar.top + i, kGroove[i]);
		}
		fill_line(view, bar.left + 1, bar.bottom + 1, bar.right - 1,
			bar.bottom + 1, mix(base, kBlack, 0.12f));
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 2;
		bar.right = bar.left + kGrooveRows - 1;
		for (int32 i = 0; i < kGrooveRows; i++) {
			const float inset = i == 0 || i == kGrooveRows - 1 ? 1 : 0;
			fill_line(view, bar.left + i, bar.top + inset, bar.left + i,
				bar.bottom - inset, kGroove[i]);
		}
		fill_line(view, bar.right + 1, bar.top + 1, bar.right + 1,
			bar.bottom - 1, mix(base, kBlack, 0.12f));
	}

	// an application's fill colour, when it has one
	if (fillColor != base && fillColor != ui_color(B_PANEL_BACKGROUND_COLOR)
		&& (flags & B_DISABLED) == 0) {
		view->SetHighColor(fillColor);
		view->FillRect(bar.InsetByCopy(1, 1));
	}

	view->PopState();
}


/*!	A knob in the accent colour, bevelled, with two ridges along the slider's
	direction across its middle.
*/
void
GershwixControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	Accent accent = theme_accent();
	if ((flags & B_DISABLED) != 0) {
		const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
		accent.face = mix(accent.face, panel, 0.6f);
		accent.light = mix(accent.light, panel, 0.6f);
		accent.dark = mix(accent.dark, panel, 0.6f);
		accent.ridgeLight = accent.ridge = accent.ridgeDark = accent.face;
	} else if ((flags & B_ACTIVATED) != 0) {
		accent.face = mix(accent.face, accent.dark, 0.5f);
		accent.light = mix(accent.light, accent.face, 0.5f);
	}
	const rgb_color midLight = mix(accent.light, accent.face, 0.5f);
	const rgb_color midDark = mix(accent.face, accent.dark, 0.5f);

	view->PushState();

	const BRect& r = rect;
	fill_line(view, r.left + 1, r.top, r.right - 1, r.bottom, accent.face);
	fill_line(view, r.left, r.top + 1, r.right, r.bottom - 1, accent.face);
	fill_line(view, r.left + 1, r.top, r.right - 1, r.top, accent.light);
	fill_line(view, r.left, r.top + 1, r.left, r.bottom - 1, accent.light);
	fill_line(view, r.left + 1, r.top + 1, r.right - 1, r.top + 1, midLight);
	fill_line(view, r.left + 1, r.top + 1, r.left + 1, r.bottom - 1, midLight);
	fill_line(view, r.left + 1, r.bottom, r.right - 1, r.bottom, accent.dark);
	fill_line(view, r.right, r.top + 1, r.right, r.bottom - 1, accent.dark);
	fill_line(view, r.left + 2, r.bottom - 1, r.right - 1, r.bottom - 1, midDark);
	fill_line(view, r.right - 1, r.top + 2, r.right - 1, r.bottom - 1, midDark);

	// two ridges, along the direction the knob moves in
	const float length = floorf((orientation == B_HORIZONTAL
		? r.Height() : r.Width()) / 2);
	draw_ridges(view, r, accent, 2, length,
		orientation == B_HORIZONTAL ? B_HORIZONTAL : B_VERTICAL);

	view->PopState();
}


/*!	A pointer in the accent colour, its tip towards the slider's marks, lit
	along its left.
*/
void
GershwixControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	Accent accent = theme_accent();
	if ((flags & B_DISABLED) != 0) {
		const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
		accent.face = mix(accent.face, panel, 0.6f);
		accent.light = mix(accent.light, panel, 0.6f);
		accent.dark = mix(accent.dark, panel, 0.6f);
	} else if ((flags & B_ACTIVATED) != 0)
		accent.face = mix(accent.face, accent.dark, 0.5f);

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

	view->SetHighColor(accent.dark);
	view->FillPolygon(points, 5);

	// the face a pixel in, lit along its top left
	BPoint inner[5];
	const BPoint centre((r.left + r.right) / 2, (r.top + r.bottom) / 2);
	for (int32 i = 0; i < 5; i++) {
		BPoint toward = centre - points[i];
		const float length = sqrtf(toward.x * toward.x + toward.y * toward.y);
		inner[i] = length > 0 ? points[i] + BPoint(toward.x / length * 1.4f,
			toward.y / length * 1.4f) : points[i];
	}
	BGradientLinear gradient;
	gradient.AddColor(accent.light, 0);
	gradient.AddColor(accent.face, 90);
	gradient.AddColor(accent.face, 255);
	gradient.SetStart(r.LeftTop());
	gradient.SetEnd(r.RightTop());
	view->FillPolygon(inner, 5, gradient);

	view->PopState();
}


//	#pragma mark - text controls


/*!	Sunk into the panel: a dark line along the top, lighter ones down the sides
	and a white one along the bottom, two deep. With the focus, two lines in the
	keyboard navigation colour.
*/
void
GershwixControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect outer(rect);
	const BRect inner(rect.InsetByCopy(1, 1));
	if ((flags & B_FOCUSED) != 0 && !disabled) {
		const rgb_color focus = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		view->SetHighColor(focus);
		view->StrokeRect(outer);
		view->SetHighColor(mix(focus, kWhite, 0.45f));
		view->StrokeRect(inner);
	} else {
		const float lighter = disabled ? 0.5f : 0;
		view->SetHighColor(mix(kFieldOuterSide, base, lighter));
		view->StrokeRect(outer);
		fill_line(view, outer.left, outer.top, outer.right, outer.top,
			mix(kFieldOuterTop, base, lighter));
		fill_line(view, outer.left + 1, outer.bottom, outer.right - 1,
			outer.bottom, mix(kFieldOuterBottom, base, lighter));
		view->SetHighColor(mix(kFieldInnerSide, base, lighter));
		view->StrokeRect(inner);
		fill_line(view, inner.left, inner.top, inner.right, inner.top,
			mix(kFieldInnerTop, base, lighter));
		fill_line(view, inner.left + 1, inner.bottom, inner.right - 1,
			inner.bottom, mix(kFieldInnerBottom, base, lighter));
	}

	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


//!	A thin dark line round the group.
void
GershwixControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	view->SetHighColor(kGroupLine);
	view->StrokeRect(rect);
	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - tabs


//!	A line along the page's top that the tabs sit on.
void
GershwixControlLook::DrawTabFrame(BView* view, BRect& rect,
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
	fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
	if (borderStyle != B_NO_BORDER) {
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kButton.outline);
	}
	view->PopState();
}


/*!	The chosen tab is a bevel with rounded top corners and no bottom, open to
	the page.
*/
void
GershwixControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawActiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		const Bevel& bevel = (flags & B_DISABLED) != 0 ? kButtonDisabled : kButton;
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		// the bottom runs on past the tab's edge, out of sight
		BRect shape(rect.left, rect.top, rect.right, rect.bottom + 4);
		draw_bevel(view, shape, bevel);
		// open to the page: the face goes down over the page's line
		fill_line(view, rect.left + 3, rect.bottom - 1, rect.right - 3,
			rect.bottom, bevel.face);
		view->PopState();
	}

	rect.top += 3;
	rect.bottom -= 1;
	rect.left += 4;
	rect.right -= 4;
}


//!	A tab behind the chosen one is lower, darker and flat, and sits on the line.
void
GershwixControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected, int32 first,
	int32 last)
{
	if (side != BTabView::kTopSide) {
		HaikuControlLook::DrawInactiveTab(view, rect, updateRect, base, flags,
			borders, side, index, selected, first, last);
		return;
	}

	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		fill_line(view, rect.left, rect.top, rect.right, rect.bottom, base);
		const Bevel flat = { kBackTabLine, mix(grey(kBackTabLine), base,
			0.4f).red, kBackTabFace, kBackTabFace, kBackTabFace, kBackTabFace };
		BRect shape(rect.left, rect.top + 2, rect.right, rect.bottom + 4);
		draw_bevel(view, shape, flat);
		fill_line(view, rect.left, rect.bottom, rect.right, rect.bottom,
			kButton.outline);
		view->PopState();
	}

	rect.top += 3;
	rect.bottom -= 1;
	rect.left += 4;
	rect.right -= 4;
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::GershwixControlLook();
}
