/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See LeopardControlLook.h. Colours and sizes were measured from screenshots of
 * Mac OS X 10.5.
 */


#include "LeopardControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <Bitmap.h>
#include <string.h>

#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Polygon.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


#define GREY(level) { level, level, level, 255 }
#define RGB(r, g, b) { r, g, b, 255 }

static const rgb_color kBlack = GREY(0);
static const rgb_color kWhite = GREY(0xff);


/*!	A gel's colours: its outline, darker at the top, and its face, light in its
	upper half, then a darker band that brightens towards the bottom, where the
	light comes through it.
*/
struct Gel {
	rgb_color	outlineTop;
	rgb_color	outlineBottom;
	rgb_color	top;
	rgb_color	middleLight;
	rgb_color	middleDark;
	rgb_color	bottom;
};

static const Gel kWhiteGel = {
	GREY(0x55), GREY(0x70),
	GREY(0xff), GREY(0xef), GREY(0xdc), GREY(0xff)
};
static const Gel kBlueGel = {
	RGB(0x26, 0x30, 0x8a), RGB(0x55, 0x60, 0x78),
	RGB(0xcf, 0xdf, 0xf0), RGB(0x84, 0xb5, 0xe6), RGB(0x5d, 0x9e, 0xdf),
	RGB(0xc3, 0xfb, 0xff)
};
static const Gel kGraphiteGel = {
	RGB(0x3d, 0x44, 0x52), RGB(0x66, 0x6b, 0x74),
	RGB(0xe2, 0xe7, 0xee), RGB(0xa7, 0xb2, 0xc2), RGB(0x88, 0x95, 0xa9),
	RGB(0xe4, 0xeb, 0xf3)
};
static const Gel kDisabledGel = {
	GREY(0xa8), GREY(0xb4),
	GREY(0xfb), GREY(0xf3), GREY(0xea), GREY(0xfb)
};

// the menu selection, and the menu bar
static const rgb_color kMenuBlueTop = RGB(0x51, 0x70, 0xf6);
static const rgb_color kMenuBlueBottom = RGB(0x1a, 0x43, 0xf3);
static const rgb_color kMenuBlueLine = RGB(0x48, 0x67, 0xea);
static const rgb_color kMenuBlueBottomLine = RGB(0x0e, 0x37, 0xe7);
static const rgb_color kMenuGraphiteTop = RGB(0x7f, 0x8f, 0xaa);
static const rgb_color kMenuGraphiteBottom = RGB(0x4f, 0x60, 0x7f);
static const rgb_color kMenuBarTop = GREY(0xf8);
static const rgb_color kMenuBarShadeTop = GREY(0xef);
static const rgb_color kMenuBarShadeBottom = GREY(0xcc);
static const rgb_color kMenuBarLine = GREY(0x5f);
static const rgb_color kMenuEdge = GREY(0xc4);

// the scroll bar's track, across it from its left
static const rgb_color kTrackEdge = GREY(0xb8);
static const rgb_color kTrackShade = GREY(0xc4);
static const rgb_color kTrackLight = GREY(0xfb);
static const rgb_color kTrackFar = GREY(0xee);
static const rgb_color kScrollBorder = GREY(0xa5);
static const rgb_color kArrowFace = GREY(0xe1);
static const rgb_color kArrowLight = GREY(0xf6);
static const rgb_color kArrowShade = GREY(0xc8);
static const rgb_color kArrow = GREY(0x43);

static const rgb_color kGroove = GREY(0x7f);
static const rgb_color kGrooveDark = RGB(0x5d, 0x55, 0x54);
static const rgb_color kGrooveLight = GREY(0xa5);

static const rgb_color kFieldTop = GREY(0x8a);
static const rgb_color kFieldSide = GREY(0xbc);
static const rgb_color kFieldShadow = GREY(0xe4);
static const rgb_color kFocusOuter = RGB(0xa9, 0xc5, 0xe6);
static const rgb_color kFocusInner = RGB(0x5f, 0x90, 0xca);

static const rgb_color kGroupTop = GREY(0xa8);
static const rgb_color kGroupSide = GREY(0xbe);
static const rgb_color kGroupShadow = GREY(0xd5);

static const rgb_color kHeaderTop = GREY(0xff);
static const rgb_color kHeaderBottom = GREY(0xe2);
static const rgb_color kHeaderLine = GREY(0xa7);
static const rgb_color kCheck = RGB(0x14, 0x1e, 0x2a);

static const float kIndicatorWidth = 16;
static const float kMenuFieldRadius = 4;


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


static rgb_color
with_alpha(rgb_color color, uint8 alpha)
{
	color.alpha = alpha;
	return color;
}


/*!	Whether the user chose Graphite: its highlight colour is a greyish blue, and
	Blue's a strong one.
*/
static bool
is_graphite()
{
	const rgb_color highlight = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	const int32 high = std::max(highlight.red,
		std::max(highlight.green, highlight.blue));
	const int32 low = std::min(highlight.red,
		std::min(highlight.green, highlight.blue));
	return high == 0 || (high - low) * 100 / high < 40;
}


static const Gel&
accent_gel()
{
	return is_graphite() ? kGraphiteGel : kBlueGel;
}


static Gel
pressed(const Gel& gel)
{
	Gel result = gel;
	result.top = mix(gel.top, kBlack, 0.14f);
	result.middleLight = mix(gel.middleLight, kBlack, 0.16f);
	result.middleDark = mix(gel.middleDark, kBlack, 0.18f);
	result.bottom = mix(gel.bottom, kBlack, 0.12f);
	return result;
}


/*!	Draws in B_OP_ALPHA, so that shapes blend their soft edges with what's
	behind them.
*/
static void
begin_alpha(BView* view)
{
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
}


static BGradientLinear
face_gradient(const Gel& gel, const BRect& rect, orientation orientation)
{
	BGradientLinear gradient;
	gradient.AddColor(gel.top, 0);
	gradient.AddColor(gel.middleLight, 120);
	gradient.AddColor(gel.middleDark, 132);
	gradient.AddColor(gel.bottom, 255);
	if (orientation == B_HORIZONTAL) {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
	} else {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.RightTop());
	}
	return gradient;
}


/*!	A gel shape: a soft shadow below it, its outline and its face. The rect is
	the outline's.
*/
static void
draw_gel(BView* view, BRect rect, float radius, const Gel& gel,
	orientation orientation = B_HORIZONTAL, bool shadow = true)
{
	if (!rect.IsValid())
		return;

	radius = std::min(radius, floorf((std::min(rect.Width(), rect.Height())
		+ 1) / 2));

	view->PushState();
	begin_alpha(view);

	if (shadow) {
		view->SetHighColor(with_alpha(kBlack, 34));
		view->FillRoundRect(rect.OffsetByCopy(0, 1), radius, radius);
	}

	BGradientLinear outline;
	outline.AddColor(gel.outlineTop, 0);
	outline.AddColor(gel.outlineBottom, 255);
	if (orientation == B_HORIZONTAL) {
		outline.SetStart(rect.LeftTop());
		outline.SetEnd(rect.LeftBottom());
	} else {
		outline.SetStart(rect.LeftTop());
		outline.SetEnd(rect.RightTop());
	}
	view->FillRoundRect(rect, radius, radius, outline);

	const BRect face = rect.InsetByCopy(1, 1);
	if (face.IsValid()) {
		view->FillRoundRect(face, std::max(0.0f, radius - 1),
			std::max(0.0f, radius - 1), face_gradient(gel, face, orientation));
	}

	view->PopState();
}


static void
draw_gel_ellipse(BView* view, BRect rect, const Gel& gel, bool shadow = true)
{
	view->PushState();
	begin_alpha(view);

	if (shadow) {
		view->SetHighColor(with_alpha(kBlack, 34));
		view->FillEllipse(rect.OffsetByCopy(0, 1));
	}

	BGradientLinear outline;
	outline.AddColor(gel.outlineTop, 0);
	outline.AddColor(gel.outlineBottom, 255);
	outline.SetStart(rect.LeftTop());
	outline.SetEnd(rect.LeftBottom());
	view->FillEllipse(rect, outline);

	const BRect face = rect.InsetByCopy(1, 1);
	view->FillEllipse(face, face_gradient(gel, face, B_HORIZONTAL));

	view->PopState();
}


/*!	A small solid triangle, its point towards direction. */
static void
draw_triangle(BView* view, const BRect& rect, int32 direction,
	const rgb_color& color, float size = 3)
{
	const float x = floorf((rect.left + rect.right) / 2) + 0.5f;
	const float y = floorf((rect.top + rect.bottom) / 2) + 0.5f;
	BPoint points[3];
	switch (direction) {
		case BControlLook::B_UP_ARROW:
			points[0] = BPoint(x - size - 0.5f, y + size / 2 + 0.5f);
			points[1] = BPoint(x + size + 0.5f, y + size / 2 + 0.5f);
			points[2] = BPoint(x, y - size / 2 - 1);
			break;
		case BControlLook::B_DOWN_ARROW:
			points[0] = BPoint(x - size - 0.5f, y - size / 2 - 0.5f);
			points[1] = BPoint(x + size + 0.5f, y - size / 2 - 0.5f);
			points[2] = BPoint(x, y + size / 2 + 1);
			break;
		case BControlLook::B_LEFT_ARROW:
			points[0] = BPoint(x + size / 2 + 0.5f, y - size - 0.5f);
			points[1] = BPoint(x + size / 2 + 0.5f, y + size + 0.5f);
			points[2] = BPoint(x - size / 2 - 1, y);
			break;
		default:
			points[0] = BPoint(x - size / 2 - 0.5f, y - size - 0.5f);
			points[1] = BPoint(x - size / 2 - 0.5f, y + size + 0.5f);
			points[2] = BPoint(x + size / 2 + 1, y);
			break;
	}
	view->PushState();
	begin_alpha(view);
	view->SetHighColor(color);
	view->FillTriangle(points[0], points[1], points[2]);
	view->PopState();
}


//! A push button is a pill, its ends half circles.
static float
button_radius(const BRect& rect)
{
	return floorf((rect.Height() + 1) / 2);
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


LeopardControlLook::LeopardControlLook()
{
}


LeopardControlLook::~LeopardControlLook()
{
}


//	#pragma mark - buttons


/*!	The whole gel, so that the face fills the gap Haiku leaves inside the frame;
	the background draws the face again. A default button gets the same size as
	the others, in the accent colour, as Leopard's does.
*/
void
LeopardControlLook::DrawButtonFrame(BView* view, BRect& rect,
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
	if (isDefault)
		shape.InsetBy(3, 3);
	// room for the shadow below
	shape.bottom--;

	// a frame that's been split carries its shape on past what's open
	const float radius = button_radius(shape);
	if ((borders & B_LEFT_BORDER) == 0)
		shape.left -= radius + 1;
	if ((borders & B_RIGHT_BORDER) == 0)
		shape.right += radius + 1;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;
	Gel gel = disabled ? kDisabledGel
		: (isDefault || down) ? accent_gel() : kWhiteGel;
	if (down && !disabled)
		gel = pressed(gel);
	draw_gel(view, shape, radius, gel);

	if ((flags & B_FOCUSED) != 0 && !disabled) {
		view->PushState();
		begin_alpha(view);
		view->SetHighColor(with_alpha(kFocusInner, 200));
		view->SetPenSize(1);
		view->StrokeRoundRect(shape, radius, radius);
		view->PopState();
	}

	view->PopState();

	rect.InsetBy(inset, inset);
}


void
LeopardControlLook::DrawButtonBackground(BView* view, BRect& rect,
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
		BGradientLinear gradient;
		const bool active = (flags & B_ACTIVATED) != 0;
		gradient.AddColor(active ? mix(kHeaderTop, kBlack, 0.08f) : kHeaderTop,
			0);
		gradient.AddColor(active ? mix(kHeaderBottom, kBlack, 0.08f)
			: kHeaderBottom, 255);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
		view->FillRect(rect, gradient);
		view->SetHighColor(kHeaderLine);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
		view->PopState();
		return;
	}

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool down = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;
	Gel gel = disabled ? kDisabledGel
		: (isDefault || down) ? accent_gel() : kWhiteGel;
	if (down && !disabled)
		gel = pressed(gel);

	// the face inside the outline the frame drew, as it drew it
	BRect face(rect.InsetByCopy(-1, -1));
	face.bottom = std::min(face.bottom, rect.bottom);
	const float radius = std::max(0.0f, button_radius(face) - 1);
	BRect shape(face);
	if ((borders & B_LEFT_BORDER) == 0)
		shape.left -= radius + 1;
	if ((borders & B_RIGHT_BORDER) == 0)
		shape.right += radius + 1;

	begin_alpha(view);
	view->FillRoundRect(shape, radius, radius,
		face_gradient(gel, shape, orientation));

	view->PopState();

	rect.InsetBy(1, 1);
}


void
LeopardControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
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
			? grey(0xa8) : kBlack, 2.5f);
	view->PopState();
}


//	#pragma mark - menus


void
LeopardControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		view->SetHighColor(base);
		view->FillRect(rect);
		view->PopState();
		return;
	}

	BGradientLinear gradient;
	gradient.AddColor(kMenuBarShadeTop, 0);
	gradient.AddColor(kMenuBarShadeBottom, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);

	view->SetHighColor(kMenuBarTop);
	view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->SetHighColor(kMenuBarLine);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	}

	view->PopState();
}


void
LeopardControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	// the field's background draws the whole gel; this keeps Haiku's inset,
	// which the label is laid out in
	rect.InsetBy(2, 2);
}


/*!	A white gel, with a blue one at its right end holding two arrows. */
void
LeopardControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();

	// the gel goes back out to where the frame would have been
	BRect shape(rect.InsetByCopy(-2, -2));
	shape.bottom--;
	view->ClipToRect(shape.InsetByCopy(0, -1));
	draw_gel(view, shape, kMenuFieldRadius, disabled ? kDisabledGel : kWhiteGel);

	if (popupIndicator) {
		BRect indicator(shape);
		indicator.left = indicator.right - kIndicatorWidth;
		if (!disabled) {
			view->PushState();
			view->ClipToRect(indicator);
			const bool open = (flags & (B_ACTIVATED | B_CLICKED)) != 0;
			const Gel& accent = accent_gel();
			draw_gel(view, shape, kMenuFieldRadius,
				open ? pressed(accent) : accent, B_HORIZONTAL, false);
			view->PopState();
		}
		view->PushState();
		view->SetHighColor(disabled ? grey(0xb4) : grey(0x5a));
		view->FillRect(BRect(indicator.left, shape.top + 1, indicator.left,
			shape.bottom - 1));
		view->PopState();

		const rgb_color arrows = disabled ? grey(0xa8) : kBlack;
		const float middle = floorf((indicator.top + indicator.bottom) / 2);
		BRect arrow(indicator.left + 1, middle - 6, indicator.right, middle - 1);
		draw_triangle(view, arrow, B_UP_ARROW, arrows, 2.5f);
		arrow.OffsetBy(0, 7);
		draw_triangle(view, arrow, B_DOWN_ARROW, arrows, 2.5f);

		rect.right = indicator.left - 2;
	}

	view->PopState();
}


void
LeopardControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldBackground(view, rect, updateRect, base, false, flags);
}


void
LeopardControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);

	view->SetHighColor(kMenuEdge);
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


/*!	A selected item, in the menu or the menu bar, is a vivid shaded blue. */
void
LeopardControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	const bool selected = (flags & (B_ACTIVATED | B_HOVER)) != 0
		&& (flags & B_DISABLED) == 0;
	if (!selected) {
		view->SetHighColor(base);
		view->FillRect(rect);
		view->PopState();
		return;
	}

	const bool graphite = is_graphite();
	const rgb_color top = graphite ? kMenuGraphiteTop : kMenuBlueTop;
	const rgb_color bottom = graphite ? kMenuGraphiteBottom : kMenuBlueBottom;
	BGradientLinear gradient;
	gradient.AddColor(top, 0);
	gradient.AddColor(bottom, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);
	view->SetHighColor(graphite ? mix(top, kBlack, 0.06f) : kMenuBlueLine);
	view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
	view->SetHighColor(graphite ? mix(bottom, kBlack, 0.1f)
		: kMenuBlueBottomLine);
	view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	view->PopState();
}


//	#pragma mark - status bars


/*!	A blue gel with light diagonal stripes, in a rounded light track. */
void
LeopardControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const float radius = std::min(4.0f, floorf(rect.Height() / 2));
	Gel track = kWhiteGel;
	track.outlineTop = grey(0x8c);
	track.outlineBottom = grey(0xa4);
	track.top = grey(0xe4);
	track.middleLight = grey(0xf0);
	track.middleDark = grey(0xf6);
	track.bottom = grey(0xff);
	BRect shape(rect);
	shape.bottom--;
	draw_gel(view, shape, radius, track);

	if (progressPosition > rect.left + 1) {
		BRect bar(shape);
		bar.right = std::min(progressPosition, shape.right);
		if (bar.Width() >= 2 * radius) {
			view->PushState();
			view->ClipToRect(bar);
			const bool themed = barColor == ui_color(B_STATUS_BAR_COLOR);
			Gel gel = accent_gel();
			if (!themed) {
				// an application's own colour, in the same gel
				gel.top = mix(barColor, kWhite, 0.7f);
				gel.middleLight = mix(barColor, kWhite, 0.35f);
				gel.middleDark = barColor;
				gel.bottom = mix(barColor, kWhite, 0.6f);
				gel.outlineTop = mix(barColor, kBlack, 0.55f);
				gel.outlineBottom = mix(barColor, kBlack, 0.4f);
			}
			draw_gel(view, shape, radius, gel, B_HORIZONTAL, false);

			// the stripes, leaning to the right
			begin_alpha(view);
			view->SetHighColor(with_alpha(kWhite, 56));
			const float height = bar.Height();
			for (float x = bar.left - height; x < bar.right; x += 16) {
				BPoint points[4] = {
					BPoint(x, bar.bottom), BPoint(x + 7, bar.bottom),
					BPoint(x + 7 + height, bar.top), BPoint(x + height, bar.top)
				};
				view->FillPolygon(points, 4);
			}
			view->PopState();
		}
	}

	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


void
LeopardControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool on = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;
	const bool down = (flags & B_CLICKED) != 0;

	Gel gel = disabled ? kDisabledGel : on ? accent_gel() : kWhiteGel;
	if (down && !disabled)
		gel = pressed(on ? gel : accent_gel());

	BRect box(rect);
	box.bottom--;
	draw_gel(view, box, 3, gel);

	view->PushState();
	begin_alpha(view);
	view->SetHighColor(disabled ? grey(0xa0) : kCheck);
	const float size = box.Width() + 1;
	if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
		const float middle = floorf((box.top + box.bottom) / 2);
		view->FillRect(BRect(box.left + roundf(size * 0.25f), middle - 1,
			box.right - roundf(size * 0.25f), middle + 1));
	} else if ((flags & B_ACTIVATED) != 0) {
		// a tick, reaching a little out past the box's top right
		view->SetPenSize(std::max(1.8f, size / 7));
		view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
		const BPoint start(box.left + size * 0.24f, box.top + size * 0.48f);
		const BPoint low(box.left + size * 0.43f, box.bottom - size * 0.22f);
		const BPoint end(box.right + size * 0.02f, box.top - size * 0.06f);
		view->StrokeLine(start, low);
		view->StrokeLine(low, end);
	}
	view->PopState();
}


void
LeopardControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const bool on = (flags & B_ACTIVATED) != 0;
	const bool down = (flags & B_CLICKED) != 0;

	Gel gel = disabled ? kDisabledGel : on ? accent_gel() : kWhiteGel;
	if (down && !disabled)
		gel = pressed(on ? gel : accent_gel());

	BRect ball(rect);
	ball.bottom--;
	ball.right = ball.left + ball.Height();
	draw_gel_ellipse(view, ball, gel);

	if (on) {
		view->PushState();
		begin_alpha(view);
		view->SetHighColor(disabled ? grey(0xa0) : kCheck);
		const float inset = roundf((ball.Width() + 1) * 0.32f);
		view->FillEllipse(ball.InsetByCopy(inset, inset));
		view->PopState();
	}
}


//	#pragma mark - scroll bars


void
LeopardControlLook::DrawScrollBarBorder(BView* view, BRect rect,
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


void
LeopardControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	view->ClipToRect(rect);

	// a light shaded face across the bar, with a line between it and the track
	BGradientLinear gradient;
	const rgb_color light = down ? mix(kArrowLight, kBlack, 0.15f) : kArrowLight;
	const rgb_color face = down ? mix(kArrowFace, kBlack, 0.15f) : kArrowFace;
	gradient.AddColor(light, 0);
	gradient.AddColor(face, 128);
	gradient.AddColor(kArrowShade, 255);
	if (orientation == B_VERTICAL) {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.RightTop());
	} else {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
	}
	view->FillRect(rect, gradient);

	view->SetHighColor(grey(0x9e));
	switch (direction) {
		case B_UP_ARROW:
			view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
			break;
		case B_DOWN_ARROW:
			view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top));
			break;
		case B_LEFT_ARROW:
			view->FillRect(BRect(rect.right, rect.top, rect.right, rect.bottom));
			break;
		case B_RIGHT_ARROW:
			view->FillRect(BRect(rect.left, rect.top, rect.left, rect.bottom));
			break;
	}

	draw_triangle(view, rect, direction, disabled ? grey(0xa8) : kArrow, 3);

	view->PopState();
}


void
LeopardControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


/*!	A track shaded across, darkest along the side the light comes from. */
void
LeopardControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	BGradientLinear gradient;
	gradient.AddColor(kTrackEdge, 0);
	gradient.AddColor(kTrackShade, 24);
	gradient.AddColor(kTrackLight, 150);
	gradient.AddColor(kTrackFar, 255);
	if (orientation == B_VERTICAL) {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.RightTop());
	} else {
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());
	}
	view->FillRect(rect, gradient);

	view->PopState();
}


/*!	A blue capsule along the bar. A bar with nothing to scroll has none. */
void
LeopardControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();

	// the track runs on under it
	BRect track(rect);
	DrawScrollBarBackground(view, track, updateRect, base, flags, orientation);

	if (!disabled) {
		BRect thumb(rect);
		if (orientation == B_VERTICAL)
			thumb.InsetBy(1, 0);
		else
			thumb.InsetBy(0, 1);
		const float radius = floorf((orientation == B_VERTICAL
			? thumb.Width() : thumb.Height()) / 2) + 1;
		Gel gel = accent_gel();
		if ((flags & B_ACTIVATED) != 0)
			gel = pressed(gel);
		draw_gel(view, thumb, radius, gel,
			orientation == B_VERTICAL ? B_VERTICAL : B_HORIZONTAL, false);
	}

	view->PopState();
}


float
LeopardControlLook::GetScrollBarWidth(orientation orientation)
{
	return 15;
}


//	#pragma mark - sliders


/*!	A thin rounded groove, sunk in, in the middle of what it's given. */
void
LeopardControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	begin_alpha(view);

	BRect bar(rect);
	if (orientation == B_HORIZONTAL) {
		bar.top = floorf((rect.top + rect.bottom) / 2) - 2;
		bar.bottom = bar.top + 4;
	} else {
		bar.left = floorf((rect.left + rect.right) / 2) - 2;
		bar.right = bar.left + 4;
	}

	BGradientLinear gradient;
	gradient.AddColor(kGrooveDark, 0);
	gradient.AddColor(kGroove, 90);
	gradient.AddColor(kGrooveLight, 200);
	gradient.AddColor(kGroove, 255);
	if (orientation == B_HORIZONTAL) {
		gradient.SetStart(bar.LeftTop());
		gradient.SetEnd(bar.LeftBottom());
	} else {
		gradient.SetStart(bar.LeftTop());
		gradient.SetEnd(bar.RightTop());
	}
	view->FillRoundRect(bar, 2, 2, gradient);

	// an application's fill colour, when it has one
	if (fillColor != base && fillColor != ui_color(B_PANEL_BACKGROUND_COLOR)) {
		view->SetHighColor(with_alpha(fillColor, 160));
		view->FillRoundRect(bar.InsetByCopy(1, 1), 1, 1);
	}

	view->PopState();
}


/*!	A round gel knob in the accent colour. */
void
LeopardControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	Gel gel = disabled ? kDisabledGel : accent_gel();
	if ((flags & B_ACTIVATED) != 0 && !disabled)
		gel = pressed(gel);

	const float size = std::min(16.0f, std::min(rect.Width(), rect.Height()));
	const float x = floorf((rect.left + rect.right - size) / 2);
	const float y = floorf((rect.top + rect.bottom - size) / 2);
	draw_gel_ellipse(view, BRect(x, y, x + size - 1, y + size - 1), gel);
}


/*!	A gel pointer, its tip towards the slider's marks, as Leopard has where a
	slider has marks.
*/
void
LeopardControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	Gel gel = disabled ? kDisabledGel : accent_gel();
	if ((flags & B_ACTIVATED) != 0 && !disabled)
		gel = pressed(gel);

	view->PushState();
	begin_alpha(view);

	BRect r(rect);
	r.bottom--;
	BPoint points[5];
	if (orientation == B_HORIZONTAL) {
		const float tip = std::min(6.0f, floorf(r.Width() / 2));
		points[0] = BPoint(r.left + 2, r.top);
		points[1] = BPoint(r.right - 2, r.top);
		points[2] = BPoint(r.right, r.bottom - tip);
		points[3] = BPoint((r.left + r.right) / 2, r.bottom);
		points[4] = BPoint(r.left, r.bottom - tip);
	} else {
		const float tip = std::min(6.0f, floorf(r.Height() / 2));
		points[0] = BPoint(r.left, r.top + 2);
		points[1] = BPoint(r.left, r.bottom - 2);
		points[2] = BPoint(r.right - tip, r.bottom);
		points[3] = BPoint(r.right, (r.top + r.bottom) / 2);
		points[4] = BPoint(r.right - tip, r.top);
	}

	view->SetHighColor(with_alpha(kBlack, 34));
	BPolygon shadow(points, 5);
	shadow.MapTo(shadow.Frame(), shadow.Frame().OffsetByCopy(0, 1));
	view->FillPolygon(&shadow);

	BGradientLinear outline;
	outline.AddColor(gel.outlineTop, 0);
	outline.AddColor(gel.outlineBottom, 255);
	outline.SetStart(r.LeftTop());
	outline.SetEnd(r.LeftBottom());
	view->FillPolygon(points, 5, outline);

	// the face, a pixel in
	BPoint inner[5];
	const BPoint centre((r.left + r.right) / 2, (r.top + r.bottom) / 2);
	for (int32 i = 0; i < 5; i++) {
		BPoint toward = centre - points[i];
		const float length = sqrtf(toward.x * toward.x + toward.y * toward.y);
		inner[i] = length > 0 ? points[i] + BPoint(toward.x / length * 1.4f,
			toward.y / length * 1.4f) : points[i];
	}
	view->FillPolygon(inner, 5, face_gradient(gel, r, B_HORIZONTAL));

	view->PopState();
}


//	#pragma mark - text controls


/*!	A thin line, darker along the top with a little shadow under it, or a blue
	ring when it has the focus.
*/
void
LeopardControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect)) {
		rect.InsetBy(2, 2);
		return;
	}

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect line = rect.InsetByCopy(1, 1);
	if ((flags & B_FOCUSED) != 0 && !disabled) {
		view->SetHighColor(kFocusOuter);
		view->StrokeRect(rect);
		view->SetHighColor(kFocusInner);
		view->StrokeRect(line);
	} else {
		view->SetHighColor(base);
		view->StrokeRect(rect);
		view->SetHighColor(disabled ? grey(0xc8) : kFieldSide);
		view->StrokeRect(line);
		view->SetHighColor(disabled ? grey(0xb8) : kFieldTop);
		view->FillRect(BRect(line.left, line.top, line.right, line.top));
		if (!disabled) {
			view->SetHighColor(kFieldShadow);
			view->FillRect(BRect(line.left + 1, line.top + 1, line.right - 1,
				line.top + 1));
		}
	}

	view->PopState();

	rect.InsetBy(2, 2);
}


//	#pragma mark - groups


/*!	A rounded line, a little darker along the top. */
void
LeopardControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		ForwardingControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	view->PushState();
	begin_alpha(view);
	BGradientLinear gradient;
	gradient.AddColor(kGroupTop, 0);
	gradient.AddColor(kGroupSide, 40);
	gradient.AddColor(kGroupSide, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRoundRect(rect, 4, 4, gradient);
	view->SetHighColor(kGroupShadow);
	view->FillRoundRect(rect.InsetByCopy(1, 1), 3, 3);
	view->SetHighColor(base);
	BRect inside(rect.InsetByCopy(1, 1));
	inside.top++;
	view->FillRoundRect(inside, 3, 3);
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - tabs


/*!	A line along the page's top that the segments sit on. */
void
LeopardControlLook::DrawTabFrame(BView* view, BRect& rect,
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
		view->SetHighColor(kGroupTop);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}
	view->PopState();
}


/*!	Tabs are a segmented control: one gel with a segment for each tab, rounded
	at its two ends, the chosen one in the accent colour.
*/
void
LeopardControlLook::_DrawSegment(BView* view, const BRect& rect, bool active,
	uint32 flags, int32 index, int32 first, int32 last)
{
	view->PushState();
	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(rect);

	// over the page's line, a little in from the tab's top
	BRect segment(rect.left, rect.top + 2, rect.right, rect.bottom - 1);
	view->ClipToRect(BRect(rect.left, rect.top, rect.right, rect.bottom));

	const float radius = 5;
	BRect shape(segment);
	if (index != first)
		shape.left -= radius + 1;
	if (index != last)
		shape.right += radius + 1;

	const bool disabled = (flags & B_DISABLED) != 0;
	const Gel& gel = disabled ? kDisabledGel : active ? accent_gel()
		: kWhiteGel;

	// the page's line, where it shows beside a rounded end
	view->SetHighColor(kGroupTop);
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());

	draw_gel(view, shape, radius, gel);

	// the line between two segments
	if (index != first) {
		view->SetHighColor(grey(0x70));
		view->FillRect(BRect(segment.left, segment.top + 1, segment.left,
			segment.bottom - 1));
	}

	view->PopState();
}


void
LeopardControlLook::DrawActiveTab(BView* view, BRect& rect,
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
	if (ShouldDraw(view, rect, updateRect))
		_DrawSegment(view, rect, true, flags, index, first, last);

	rect.top += 3;
	rect.bottom -= 2;
	rect.left += 4;
	rect.right -= 4;
}


void
LeopardControlLook::DrawInactiveTab(BView* view, BRect& rect,
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
	if (ShouldDraw(view, rect, updateRect))
		_DrawSegment(view, rect, false, flags, index, first, last);

	rect.top += 3;
	rect.bottom -= 2;
	rect.left += 4;
	rect.right -= 4;
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::LeopardControlLook();
}
