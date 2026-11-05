/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See TidyControlLook.h. Colours and sizes were measured from the mockups.
 */


#include "TidyControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <string.h>

#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <Window.h>

#include <MenuPrivate.h>
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kWhite = { 255, 255, 255, 255 };

// column headers
static const rgb_color kHeaderLight = { 244, 250, 252, 255 };
static const rgb_color kHeaderLineLight = { 199, 230, 240, 255 };
static const rgb_color kHeaderDark = { 37, 38, 49, 255 };
static const rgb_color kHeaderLineDark = { 53, 55, 72, 255 };

static const float kTabRadius = 4.0f;
static const float kMenuTextShift = 2.0f;

// a scroll bar's arrow, traced from the mockups, pointing up: its softened edges
// are the letters, and it is mirrored and turned for the other directions
static const char* const kArrow[] = {
	"        -       ",
	"      +oo+      ",
	"     +o--++     ",
	"    +o----++-   ",
	"   ++------+o-  ",
	" -++--------+o- ",
	" o+----------+o ",
	"-o------------++",
	" o++++++++++++o ",
	"  ++++++++++++  ",
};
static const int32 kArrowRows = sizeof(kArrow) / sizeof(kArrow[0]);
static const int32 kArrowColumns = 16;


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


static bool
is_dark(const rgb_color& color)
{
	return color.Brightness() < 128;
}


/*!	A colour moved towards whichever of black and white stands out against
	\a color.
*/
static rgb_color
contrast(const rgb_color& color, float amount)
{
	return mix(color, is_dark(color) ? kWhite : kBlack, amount);
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


/*!	What a scroll bar is drawn on. Not the base it's given, which BScrollBar sets
	to the thumb's colour.
*/
static rgb_color
_ScrollBarColor(orientation orientation)
{
	return ui_color(orientation == B_VERTICAL ? B_DOCUMENT_BACKGROUND_COLOR
		: B_PANEL_BACKGROUND_COLOR);
}


/*!	A scroll bar's thumb takes a little over half its width, in the middle. */
static BRect
_ThumbRect(BRect rect, orientation orientation)
{
	if (orientation == B_VERTICAL) {
		const float width = ceilf((rect.Width() + 1) * 0.57f);
		rect.left += floorf((rect.Width() + 1 - width) / 2);
		rect.right = rect.left + width - 1;
	} else {
		const float height = ceilf((rect.Height() + 1) * 0.57f);
		rect.top += floorf((rect.Height() + 1 - height) / 2);
		rect.bottom = rect.top + height - 1;
	}
	return rect;
}


/*!	Tracker keeps its column headers beside a window's vertical scroll bar, where
	the mockups have them run across it. This is how far down \a view it should
	draw them, in its coordinates, or a negative number if there are none.
*/
static float
_HeaderBottom(BView* view)
{
	if (view->Window() == NULL || view->Name() == NULL
		|| strcmp(view->Name(), "VScrollBar") != 0) {
		return -1;
	}
	BView* header = view->Window()->FindView("TitleView");
	if (header == NULL || header->IsHidden())
		return -1;

	const BRect frame = view->ConvertFromScreen(
		header->ConvertToScreen(header->Bounds()));
	if (fabsf(frame.top - view->Bounds().top) > 3)
		return -1;
	return frame.bottom;
}


/*!	A groove is sunk into what's around it: always darker, by more on dark. */
static rgb_color
_GrooveColor(const rgb_color& background, float amount)
{
	return mix(background, kBlack, is_dark(background) ? amount * 2.5f : amount);
}


/*!	The soft line along a vertical scroll bar's far side. Its own edge is hidden
	in Tracker's windows, so this goes on what's inside that.
*/
static void
_StrokeFarEdge(BView* view, const BRect& rect, orientation orientation)
{
	if (orientation != B_VERTICAL)
		return;
	view->SetHighColor(contrast(_ScrollBarColor(orientation), 0.22f));
	view->FillRect(BRect(rect.right, rect.top, rect.right, rect.bottom));
}


static void
_FillHeader(BView* view, BRect rect, float bottom)
{
	const bool dark = is_dark(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.bottom = std::min(rect.bottom, bottom);
	if (!rect.IsValid())
		return;
	view->SetHighColor(dark ? kHeaderDark : kHeaderLight);
	view->FillRect(rect);
	if (rect.bottom == bottom) {
		view->SetHighColor(dark ? kHeaderLineDark : kHeaderLineLight);
		view->FillRect(BRect(rect.left, bottom, rect.right, bottom));
	}
}


//	#pragma mark -


TidyControlLook::TidyControlLook()
{
}


TidyControlLook::~TidyControlLook()
{
}


//	#pragma mark - menu bars and column headers


void
TidyControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Tracker's item count uses this too, and is flat in the mockups
	if (view->Name() != NULL && strcmp(view->Name(), "CountVw") == 0) {
		view->PushState();
		view->SetHighColor(base);
		view->FillRect(rect);
		view->PopState();
		return;
	}

	view->PushState();

	const bool dark = is_dark(base);
	const rgb_color top = dark ? mix(base, kWhite, 0.2f) : base;
	const rgb_color bottom = dark ? mix(base, kWhite, 0.06f)
		: mix(base, kBlack, 0.08f);

	// menus draw their items in the view's colours afterwards: leave them
	BGradientLinear gradient;
	gradient.AddColor(top, 0);
	gradient.AddColor(bottom, 255);
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	view->FillRect(rect, gradient);

	// Its highlight and first rows are in the window's frame just above (see
	// TidyDecorator), so its text belongs higher up to be in the middle of them.
	// A menu item reads its margins as it draws, and their sum, so the bar's
	// size, doesn't change.
	BMenuBar* menuBar = dynamic_cast<BMenuBar*>(view);
	BWindow* window = view->Window();
	if (menuBar != NULL && window != NULL && (window->Look() == B_TITLED_WINDOW_LOOK
			|| window->Look() == B_DOCUMENT_WINDOW_LOOK)) {
		BPrivate::MenuPrivate menu(menuBar);
		float left, top, right, bottom;
		menu.GetItemMargins(&left, &top, &right, &bottom);
		if (top >= bottom && top >= kMenuTextShift)
			menu.SetItemMargins(left, top - kMenuTextShift, right,
				bottom + kMenuTextShift);
	}

	view->PopState();
}


void
TidyControlLook::DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, border_style borderStyle, uint32 flags,
	uint32 borders)
{
	// Tracker's folder icon sits at the end of its menu bar, as a view of its own
	if (dynamic_cast<BMenuBar*>(view) == NULL && (view->Name() == NULL
			|| strcmp(view->Name(), "DraggableContainerIcon") != 0)) {
		HaikuControlLook::DrawBorder(view, rect, updateRect, base, borderStyle,
			flags, borders);
		return;
	}

	// a menu bar's edge, in the colour TidyDecorator carries on beside it
	const rgb_color line = mix(base, kBlack, is_dark(base) ? 0.4f : 0.2f);
	_DrawFrame(view, rect, line, line, line, line, borders);
}


void
TidyControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	// Tracker's column headers are drawn as a button's background; the view's
	// name is the only thing that tells them from any other button
	if (view->Name() == NULL || strcmp(view->Name(), "TitleView") != 0) {
		HaikuControlLook::DrawButtonBackground(view, rect, updateRect, base,
			flags, borders, orientation);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool dark = is_dark(ui_color(B_PANEL_BACKGROUND_COLOR));

	view->PushState();
	rgb_color fill = dark ? kHeaderDark : kHeaderLight;
	if ((flags & B_ACTIVATED) != 0)
		fill = contrast(fill, 0.1f);
	view->SetHighColor(fill);
	view->FillRect(rect);

	// over the line Tracker draws under it
	view->SetHighColor(dark ? kHeaderLineDark : kHeaderLineLight);
	view->FillRect(BRect(rect.left, rect.bottom + 1, rect.right,
		rect.bottom + 1));
	view->PopState();
}


//	#pragma mark - status bars


void
TidyControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const rgb_color line = contrast(base, 0.32f);
	view->SetHighColor(line);
	view->StrokeRect(rect);

	BRect inner = rect.InsetByCopy(1, 1);
	const float split = std::max(inner.left - 1,
		std::min(floorf(progressPosition), inner.right));

	if (split < inner.right) {
		// hatched for what's left
		BRect empty(split + 1, inner.top, inner.right, inner.bottom);
		const rgb_color light = contrast(base, 0.08f);
		const rgb_color stripe = contrast(base, 0.22f);
		view->SetHighColor(light);
		view->FillRect(empty);
		view->SetHighColor(stripe);
		for (float y = empty.top; y <= empty.bottom; y++) {
			for (float x = empty.left; x <= empty.right; x++) {
				if (((int32)(x + y)) % 6 < 2)
					view->FillRect(BRect(x, y, x, y));
			}
		}
	}

	if (split >= inner.left) {
		BRect full(inner.left, inner.top, split, inner.bottom);
		BGradientLinear gradient;
		gradient.AddColor(mix(barColor, kWhite, 0.25f), 0);
		gradient.AddColor(barColor, 255);
		gradient.SetStart(full.LeftTop());
		gradient.SetEnd(full.LeftBottom());
		view->FillRect(full, gradient);
		view->SetHighColor(mix(barColor, kBlack, 0.4f));
		view->FillRect(BRect(split, inner.top, split, inner.bottom));
	}

	view->PopState();
}


//	#pragma mark - scroll bars


void
TidyControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style borderStyle, uint32 flags, uint32 borders)
{
	BRect result(rect);
	HaikuControlLook::DrawScrollViewFrame(view, result, updateRect,
		verticalScrollBarFrame, horizontalScrollBarFrame, base, borderStyle,
		flags, borders);

	if (borderStyle != B_FANCY_BORDER) {
		rect = result;
		return;
	}

	// a single line, with nothing round it to compete with the window's frame
	BRect outer(rect);
	view->PushState();
	_DrawFrame(view, outer, base, base, base, base, borders);
	if ((flags & B_FOCUSED) == 0) {
		const rgb_color line = contrast(base, 0.28f);
		_DrawFrame(view, outer, line, line, line, line, borders);
	}
	view->PopState();

	rect = result;
}


void
TidyControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// A vertical one is part of what it scrolls, with only a faint line where
	// they meet and a soft one along its far side. A horizontal one sits in the
	// strip along the window's bottom.
	const rgb_color fill = _ScrollBarColor(orientation);

	view->PushState();
	view->SetHighColor(fill);
	view->StrokeRect(rect);
	if (orientation == B_VERTICAL) {
		view->SetHighColor(contrast(fill, 0.06f));
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		_StrokeFarEdge(view, rect, orientation);

		const float header = _HeaderBottom(view);
		if (header >= 0)
			_FillHeader(view, rect, header);
	} else {
		view->SetHighColor(contrast(fill, 0.22f));
		view->StrokeLine(rect.LeftTop(), rect.RightTop());
	}
	view->PopState();
}


/*!	An outlined triangle, set pixel by pixel. */
void
TidyControlLook::_DrawArrow(BView* view, BRect rect, int32 direction,
	const rgb_color& line, const rgb_color& fill)
{
	const bool horizontal = direction == B_LEFT_ARROW
		|| direction == B_RIGHT_ARROW;
	const float across = (horizontal ? rect.Height() : rect.Width()) + 1;
	const float along = (horizontal ? rect.Width() : rect.Height()) + 1;

	// the mockups' arrow if there's room; short of it, without its faint tip and
	// the faint row under its base
	if (across >= kArrowColumns && along >= kArrowRows - 2) {
		const int32 first = along >= kArrowRows ? 0 : 1;
		const int32 rows = kArrowRows - 2 * first;
		const float x0 = floorf((rect.left + rect.right + 1
			- (horizontal ? rows : kArrowColumns)) / 2);
		const float y0 = floorf((rect.top + rect.bottom + 1
			- (horizontal ? kArrowColumns : rows)) / 2);
		for (int32 row = 0; row < rows; row++) {
			const char* cells = kArrow[row + first];
			for (int32 column = 0; column < kArrowColumns; column++) {
				float x, y;
				switch (direction) {
					case B_LEFT_ARROW:
						x = x0 + row;
						y = y0 + column;
						break;
					case B_RIGHT_ARROW:
						x = x0 + rows - 1 - row;
						y = y0 + column;
						break;
					case B_DOWN_ARROW:
						x = x0 + column;
						y = y0 + rows - 1 - row;
						break;
					case B_UP_ARROW:
					default:
						x = x0 + column;
						y = y0 + row;
						break;
				}
				switch (cells[column]) {
					case 'o':
						view->SetHighColor(line);
						break;
					case '+':
						view->SetHighColor(mix(line, fill, 0.5f));
						break;
					case '-':
						view->SetHighColor(fill);
						break;
					default:
						continue;
				}
				view->FillRect(BRect(x, y, x, y));
			}
		}
		return;
	}

	int32 length = (int32)roundf(std::min(across, along) * 0.62f);
	if (length % 2 == 0)
		length--;
	length = std::max(length, (int32)5);
	const int32 depth = std::max((int32)3, (int32)roundf(length * 0.65f));

	const int32 originAcross = (int32)floorf((horizontal
		? (rect.top + rect.bottom) : (rect.left + rect.right)) / 2) - length / 2;
	const int32 originAlong = (int32)floorf((horizontal
		? (rect.left + rect.right) : (rect.top + rect.bottom)) / 2) - depth / 2;

	// how far each column reaches from the base towards the point
	int32 extent[length];
	const float middle = (length - 1) / 2.0f;
	for (int32 k = 0; k < length; k++) {
		extent[k] = std::max((int32)1, (int32)roundf(depth
			* (1 - fabsf(k - middle) / (middle + 0.5f))));
	}

	for (int32 k = 0; k < length; k++) {
		for (int32 j = 0; j < extent[k]; j++) {
			const bool edge = j == 0 || j == extent[k] - 1 || k == 0
				|| k == length - 1 || extent[k - 1] <= j || extent[k + 1] <= j;
			int32 x, y;
			const int32 a = originAcross + k;
			switch (direction) {
				case B_RIGHT_ARROW:
					x = originAlong + j;
					y = a;
					break;
				case B_LEFT_ARROW:
					x = originAlong + depth - 1 - j;
					y = a;
					break;
				case B_UP_ARROW:
					x = a;
					y = originAlong + depth - 1 - j;
					break;
				case B_DOWN_ARROW:
				default:
					x = a;
					y = originAlong + j;
					break;
			}
			view->SetHighColor(edge ? line : fill);
			view->FillRect(BRect(x, y, x, y));
		}
	}
}


void
TidyControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	const rgb_color background = _ScrollBarColor(orientation);
	view->SetHighColor(background);
	view->FillRect(rect);

	// the column headers run across the top of it
	if (direction == B_UP_ARROW) {
		const float header = _HeaderBottom(view);
		if (header >= 0) {
			_FillHeader(view, rect, header);
			rect.top = std::max(rect.top, header + 1);
		}
	}

	_StrokeFarEdge(view, rect, orientation);

	// no bezel, and nothing at all when there's nothing to scroll
	if ((flags & B_DISABLED) == 0) {
		// on dark, a solid shape rather than an outlined one
		const bool dark = is_dark(background);
		const rgb_color fill = dark
			? contrast(background, down ? 0.4f : 0.22f)
			: contrast(background, down ? 0.2f : 0.07f);
		const rgb_color line = dark ? fill
			: contrast(background, down ? 0.5f : 0.3f);
		_DrawArrow(view, rect, direction, line, fill);
	}

	view->PopState();
}


float
TidyControlLook::GetScrollBarWidth(orientation orientation)
{
	// wide beside the content, slimmer in the strip along the window's bottom
	const float width = orientation == B_VERTICAL ? 23 : 18;
	return std::max(14.0f, roundf(width * be_plain_font->Size() / 12));
}


void
TidyControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base,
	uint32 flags, orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);

	if ((flags & B_DISABLED) != 0)
		return;

	// The thumb's groove: a capsule the length of the bar, as wide as the thumb
	// and its shadow, that fades towards its end. It is drawn in the parts on
	// either side of the thumb.
	BRect whole(std::min(rect1.left, rect2.left), std::min(rect1.top, rect2.top),
		std::max(rect1.right, rect2.right), std::max(rect1.bottom, rect2.bottom));
	BRect groove(_ThumbRect(whole, orientation));
	if (orientation == B_VERTICAL) {
		groove.right++;
		groove.InsetBy(0, 2);
	} else {
		groove.bottom++;
		groove.InsetBy(2, 0);
	}
	const rgb_color background = _ScrollBarColor(orientation);

	BGradientLinear gradient;
	gradient.AddColor(_GrooveColor(background, 0.055f), 0);
	gradient.AddColor(_GrooveColor(background, 0.025f), 255);
	gradient.SetStart(groove.LeftTop());
	gradient.SetEnd(orientation == B_VERTICAL ? groove.LeftBottom()
		: groove.RightTop());

	const BRect* parts[] = { &rect1, &rect2 };
	for (int32 i = 0; i < 2; i++) {
		if (!parts[i]->IsValid() || !parts[i]->Intersects(updateRect))
			continue;
		view->PushState();
		view->ClipToRect(*parts[i]);
		fill_rounded(view, groove, 3, _GrooveColor(background, 0.07f));
		view->FillRect(BRect(groove.left + 1, groove.top + 1, groove.right - 1,
			groove.bottom - 1), gradient);
		view->PopState();
	}
}


void
TidyControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetDrawingMode(B_OP_COPY);
	view->SetHighColor(_ScrollBarColor(orientation));
	view->FillRect(rect);
	_StrokeFarEdge(view, rect, orientation);
	view->PopState();
}


void
TidyControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	const rgb_color background = _ScrollBarColor(orientation);
	view->SetHighColor(background);
	view->FillRect(rect);

	_StrokeFarEdge(view, rect, orientation);

	if ((flags & B_DISABLED) == 0) {
		// the groove carries on behind it
		BRect groove(_ThumbRect(rect, orientation));
		if (orientation == B_VERTICAL)
			groove.right++;
		else
			groove.bottom++;
		view->SetHighColor(_GrooveColor(background, 0.07f));
		view->FillRect(groove);
		view->SetHighColor(_GrooveColor(background, 0.05f));
		view->FillRect(orientation == B_VERTICAL
			? groove.InsetByCopy(1, 0) : groove.InsetByCopy(0, 1));

		// a capsule, across the bar's middle, with a soft shadow on the groove
		BRect pill(_ThumbRect(rect, orientation));
		if (orientation == B_VERTICAL)
			pill.InsetBy(0, 3);
		else
			pill.InsetBy(3, 0);
		const float radius = std::min(3.0f, floorf((orientation == B_VERTICAL
			? pill.Width() : pill.Height()) / 2));
		fill_rounded(view, pill.OffsetByCopy(1, 1), radius,
			_GrooveColor(background, 0.07f));

		const bool dark = is_dark(background);
		const rgb_color line = dark ? mix(base, kBlack, 0.65f)
			: mix(base, kBlack, 0.34f);
		fill_rounded(view, pill, radius, line);
		fill_rounded(view, pill.InsetByCopy(1, 1), std::max(0.0f, radius - 1),
			base);
	}

	view->PopState();
}


//	#pragma mark - groups


void
TidyControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (borders != B_ALL_BORDERS) {
		HaikuControlLook::DrawGroupFrame(view, rect, updateRect, base, borders);
		return;
	}

	// one soft line with rounded corners
	view->PushState();
	fill_rounded(view, rect, 4, contrast(base, 0.25f));
	fill_rounded(view, rect.InsetByCopy(1, 1), 3, base);
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - tabs


void
TidyControlLook::DrawTabFrame(BView* view, BRect& rect,
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

	view->SetHighColor(base);
	view->FillRect(rect);

	if (borderStyle == B_NO_BORDER)
		return;

	view->SetHighColor(contrast(base, 0.25f));
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
}


void
TidyControlLook::DrawActiveTab(BView* view, BRect& rect,
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

	// raised, and joining what's under it
	BRect shape(rect);
	shape.bottom += kTabRadius + 2;
	fill_rounded(view, shape, kTabRadius, contrast(base, 0.3f));
	fill_rounded(view, shape.InsetByCopy(1, 1), kTabRadius - 1,
		is_dark(base) ? mix(base, kWhite, 0.08f) : mix(base, kWhite, 0.35f));

	view->PopState();

	rect.left += 3;
	rect.right -= 3;
	rect.top += 3;
	rect.bottom -= 1;
}


void
TidyControlLook::DrawInactiveTab(BView* view, BRect& rect,
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

	// only a shade lower than the page, with no edges of its own
	view->SetHighColor(base);
	view->FillRect(rect);
	BRect shape(rect);
	shape.top += 4;
	shape.bottom += kTabRadius + 2;
	fill_rounded(view, shape, kTabRadius - 1, contrast(base, 0.05f));
	view->SetHighColor(contrast(base, 0.25f));
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
	return new (std::nothrow) BPrivate::TidyControlLook();
}
