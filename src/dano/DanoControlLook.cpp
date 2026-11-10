/*
 * Copyright 2026, idumych.
 * Copyright 2026, Gonx contributors.
 * Distributed under the terms of the MIT License.
 *
 * Control look in the style of BeOS R5.1 "Dano". Colours and shapes were
 * measured from screenshots of a running system; see DanoControlLook.h. Those
 * were 16 bits per pixel, so white read as #F8FCF8 and is taken to be #FFFFFF.
 */


#include "DanoControlLook.h"

#include <algorithm>
#include <string.h>
#include <cmath>
#include <new>

#include <Bitmap.h>
#include <Button.h>
#include <Control.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>

#include "MenuMargins.h"
#include <Rect.h>
#include <TabView.h>
#include <View.h>


namespace BPrivate {


static const rgb_color kBlack = { 0, 0, 0, 255 };
static const rgb_color kWhite = { 255, 255, 255, 255 };

// buttons, check boxes and radio buttons
static const rgb_color kFace = { 240, 244, 240, 255 };
static const rgb_color kDisabledFace = { 224, 222, 224, 255 };
static const rgb_color kPressedFace = { 228, 228, 228, 255 };
static const rgb_color kDisabledLine = { 160, 156, 160, 255 };
static const rgb_color kCorner = { 184, 184, 184, 255 };
static const rgb_color kShadowNear = { 152, 152, 152, 255 };
static const rgb_color kShadowFar = { 184, 184, 184, 255 };
static const rgb_color kPressedLabel = { 112, 112, 112, 255 };

// the blue of default buttons, arrow tabs and marks, and its shades
static const rgb_color kAccent = { 116, 120, 188, 255 };
static const rgb_color kAccentNear = { 80, 84, 128, 255 };
static const rgb_color kAccentFar = { 96, 100, 160, 255 };

// text fields
static const rgb_color kFieldInner = { 176, 180, 176, 255 };

// slider grooves, scroll bars, frames and tabs
static const rgb_color kGroove = { 152, 152, 152, 255 };
static const rgb_color kGrooveEdge = { 112, 112, 112, 255 };
static const rgb_color kGrooveInner = { 128, 128, 128, 255 };
static const rgb_color kThumb = { 232, 236, 232, 255 };
static const rgb_color kTrough = { 200, 200, 200, 255 };
static const rgb_color kTroughEdge = { 184, 184, 184, 255 };
static const rgb_color kThumbEnd = { 96, 96, 96, 255 };
static const rgb_color kButtonEdge = { 192, 192, 192, 255 };
static const rgb_color kArrow = { 120, 120, 120, 255 };
static const rgb_color kArrowFill = { 232, 232, 232, 255 };
static const rgb_color kFrameOuter = { 184, 184, 184, 255 };
static const rgb_color kFrameInner = { 152, 152, 152, 255 };
static const rgb_color kTabDark = { 96, 96, 96, 255 };
static const rgb_color kGroup = { 160, 164, 160, 255 };

// status bars' empty end
static const rgb_color kStatusEmpty = { 152, 156, 152, 255 };
static const rgb_color kStatusEdge = { 104, 108, 104, 255 };
static const rgb_color kStatusInner = { 128, 132, 128, 255 };


// Marks, traced as pixels, so they keep their deliberate overflow and shading.
// The letters are colours; a space leaves what is there, and D is the control's
// background. They scale by whole pixels for larger fonts.
static const char* const kCheckOff[] = {
	" ########    ",
	"#cccccccc#   ",
	"#cccccccc#g  ",
	"#cccccccc#gh ",
	"#cccccccc#gh ",
	"#cccccccc#gh ",
	"#cccccccc#gh ",
	"#cccccccc#gh ",
	"#cccccccc#gh ",
	" ########ggh ",
	"  gggggggghh ",
	"   hhhhhhhh  ",
};
static const int32 kCheckOffRows = sizeof(kCheckOff) / sizeof(kCheckOff[0]);

// five rows above the box, and three columns left of it
static const char* const kCheckOn[] = {
	"           ##     ",
	"          #aa#    ",
	"         #aaa#g   ",
	"        #aaaa#gh  ",
	" ##    #aaaa#ggh  ",
	"#aa####aaaa#gghh  ",
	"#aaa#i#aaaa#gghh  ",
	"#aaaa#aaaa#gghh   ",
	" #aaaaaaa##ghh    ",
	" #aaaaaa#i#hh     ",
	" ##aaaa#ii#h      ",
	" ##aaa#iiD#       ",
	" #c###iiDD#       ",
	" #cciiiDDc#       ",
	"  ########        ",
};
static const int32 kCheckOnRows = sizeof(kCheckOn) / sizeof(kCheckOn[0]);

static const char* const kRadioOff[] = {
	"   #####     ",
	"  #ccccc#    ",
	" #ccccccc#   ",
	"#ccccccccc#  ",
	"#ccccccccc#g ",
	"#ccccccccc#gh",
	"#ccccccccc#gh",
	"#ccccccccc#gh",
	" #ccccccc#ggh",
	"  #ccccc#gghh",
	"   #####gghh ",
	"    ggggghh  ",
	"     hhhhh   ",
};
static const int32 kRadioOffRows = sizeof(kRadioOff) / sizeof(kRadioOff[0]);

static const char* const kRadioOn[] = {
	"   #####       ",
	"  #ccccc#      ",
	" #ccccccc#     ",
	"#ccc########   ",
	"#cc#aaaaaaaa#  ",
	"#cc#aaaaaaaaa# ",
	"#cc#aaaaaaaa#gg",
	"#ccc########ggh",
	" #ccciiii#ggghh",
	"  #cccDD#hhhhh ",
	"   #####       ",
};
static const int32 kRadioOnRows = sizeof(kRadioOn) / sizeof(kRadioOn[0]);

// the triangle under a slider's bar
static const char* const kTriangle[] = {
	"      #      ",
	"     CD#     ",
	"    CEED#    ",
	"   CEEEED#   ",
	"  CEEEEEED#  ",
	" CEssssssss# ",
	"zzzzzzzzzzzz#",
	"#############",
};
static const int32 kTriangleRows = sizeof(kTriangle) / sizeof(kTriangle[0]);


static rgb_color
mix(rgb_color a, rgb_color b, float amount)
{
	rgb_color result;
	result.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	result.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	result.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	result.alpha = a.alpha;
	return result;
}


static void
pixel(BView* view, float x, float y, const rgb_color& color)
{
	view->SetHighColor(color);
	view->FillRect(BRect(x, y, x, y));
}


/*!	A rectangle with its corners cut in crisp steps: \a cut pixels off the first
	row, one fewer off the next, and so on.
*/
static void
fill_cut(BView* view, BRect rect, int32 cut, const rgb_color& color)
{
	if (!rect.IsValid())
		return;

	view->SetHighColor(color);
	cut = std::min(cut, (int32)floorf((rect.Height() + 1) / 2));
	for (int32 row = 0; row < cut; row++) {
		const float inset = cut - row;
		view->FillRect(BRect(rect.left + inset, rect.top + row,
			rect.right - inset, rect.top + row));
		view->FillRect(BRect(rect.left + inset, rect.bottom - row,
			rect.right - inset, rect.bottom - row));
	}
	if (rect.top + cut <= rect.bottom - cut) {
		view->FillRect(BRect(rect.left, rect.top + cut, rect.right,
			rect.bottom - cut));
	}
}


/*!	Dano's control shape: a one pixel outline with corners cut by two, softened
	by a grey pixel in each step, around \a fill.
*/
static void
draw_plate(BView* view, BRect box, const rgb_color& outline,
	const rgb_color& fill, const rgb_color& corner)
{
	fill_cut(view, box, 2, outline);
	fill_cut(view, box.InsetByCopy(1, 1), 1, fill);

	pixel(view, box.left + 1, box.top, corner);
	pixel(view, box.left, box.top + 1, corner);
	pixel(view, box.right - 1, box.top, corner);
	pixel(view, box.right, box.top + 1, corner);
	pixel(view, box.left + 1, box.bottom, corner);
	pixel(view, box.left, box.bottom - 1, corner);
	pixel(view, box.right - 1, box.bottom, corner);
	pixel(view, box.right, box.bottom - 1, corner);
}


static void
draw_shadow(BView* view, BRect box, const rgb_color& near, const rgb_color& far)
{
	fill_cut(view, box.OffsetByCopy(2, 2), 2, far);
	fill_cut(view, box.OffsetByCopy(1, 1), 2, near);
}


static BRect
extend_past_open_sides(BRect rect, uint32 borders, float amount)
{
	if ((borders & BControlLook::B_LEFT_BORDER) == 0)
		rect.left -= amount;
	if ((borders & BControlLook::B_TOP_BORDER) == 0)
		rect.top -= amount;
	if ((borders & BControlLook::B_RIGHT_BORDER) == 0)
		rect.right += amount;
	if ((borders & BControlLook::B_BOTTOM_BORDER) == 0)
		rect.bottom += amount;
	return rect;
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


static bool
is_flat(uint32 flags)
{
	return (flags & BControlLook::B_FLAT) != 0
		&& (flags & (BControlLook::B_ACTIVATED
			| BControlLook::B_PARTIALLY_ACTIVATED)) == 0
		&& ((flags & (BControlLook::B_HOVER | BControlLook::B_FOCUSED)) == 0
			|| (flags & BControlLook::B_DISABLED) != 0);
}


static rgb_color
outline_color(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kDisabledLine;
	if ((flags & BControlLook::B_FOCUSED) != 0)
		return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	return kBlack;
}


static rgb_color
face_color(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kDisabledFace;
	if ((flags & (BControlLook::B_ACTIVATED
			| BControlLook::B_PARTIALLY_ACTIVATED)) != 0)
		return kPressedFace;
	return kFace;
}


/*!	Draws a table of marks with its top left at (\a x, \a y). A disabled mark is
	only its outline, so it shows the background through it.
*/
static void
draw_mark(BView* view, const char* const* rows, int32 count, float x, float y,
	int32 scale, const rgb_color& base, uint32 flags)
{
	const bool disabled = (flags & BControlLook::B_DISABLED) != 0;
	const rgb_color outline = outline_color(flags);

	for (int32 row = 0; row < count; row++) {
		for (int32 column = 0; rows[row][column] != '\0'; column++) {
			rgb_color color;
			switch (rows[row][column]) {
				case '#': color = outline; break;
				case 'a': color = disabled ? base : kAccent; break;
				case 'c': color = disabled ? base : kFace; break;
				case 'D': color = base; break;
				case 'i': color = disabled ? base
					: (rgb_color){ 168, 164, 168, 255 }; break;
				case 'g':
					if (disabled)
						continue;
					color = (rgb_color){ 144, 148, 144, 255 };
					break;
				case 'h':
					if (disabled)
						continue;
					color = (rgb_color){ 192, 192, 192, 255 };
					break;
				default:
					continue;
			}
			view->SetHighColor(color);
			view->FillRect(BRect(x + column * scale, y + row * scale,
				x + (column + 1) * scale - 1, y + (row + 1) * scale - 1));
		}
	}
}


/*!	How much to enlarge a mark drawn for a \a size pixels square box: whole
	pixels, so that it stays crisp.
*/
static int32
mark_scale(float size, float natural)
{
	return std::max((int32)1, (int32)floorf(size / natural + 0.25f));
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


DanoControlLook::DanoControlLook()
{
}


DanoControlLook::~DanoControlLook()
{
}


//	#pragma mark - buttons


void
DanoControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawDanoButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
DanoControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawDanoButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
DanoControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawDanoButtonFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
DanoControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawFace(view, rect, base, flags, borders);
}


void
DanoControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawFace(view, rect, base, flags, borders);
}


void
DanoControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawFace(view, rect, base, flags, borders);
}


void
DanoControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	DrawButtonWithPopUpBackground(view, rect, updateRect, 0, 0, 0, 0, base,
		flags, borders, orientation);
}


void
DanoControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	DrawButtonWithPopUpBackground(view, rect, updateRect, 0, 0, 0, 0, base,
		flags, borders, orientation);
}


void
DanoControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, uint32 flags, uint32 borders,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const BRect face(rect);
	_DrawFace(view, rect, base, flags, borders);

	// same split as Haiku's look, so labels stay where they were
	rect.right -= ComposeSpacing(B_USE_ITEM_SPACING);

	view->PushState();
	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect segment(rect.right + 1, face.top, face.right, face.bottom);
	view->SetHighColor(disabled ? face_color(flags) : kAccent);
	view->FillRect(segment);
	view->SetHighColor(outline_color(flags));
	view->FillRect(BRect(segment.left, segment.top, segment.left,
		segment.bottom));
	BRect arrow(segment);
	arrow.left++;
	_DrawTriangle(view, arrow, B_DOWN_ARROW,
		disabled ? kDisabledLine : kWhite, disabled ? kDisabledLine : kWhite);
	view->PopState();
}


//	#pragma mark - menu fields and menus


void
DanoControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	// The box is centred on the menu bar inside, at Dano's size; the field draws
	// the part of it outside the bar, the bar the rest (see
	// DrawMenuFieldBackground()).
	if (!ShouldDraw(view, rect, updateRect))
		return;
	if ((flags & B_BLEND_FRAME) == 0) {
		view->PushState();
		view->SetHighColor(background);
		view->FillRect(rect);
		view->PopState();
	}

	BMenuField* field = dynamic_cast<BMenuField*>(view);
	BMenuBar* menuBar = field != NULL ? field->MenuBar() : NULL;
	if (menuBar != NULL) {
		// BMenuField draws this frame two pixels around its menu bar
		_DrawMenuFieldBox(view, menuBar, rect.InsetByCopy(2, 2), background,
			flags);
	}

	inset_by_borders(rect, 2, borders);
}


void
DanoControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
DanoControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	DrawMenuFieldFrame(view, rect, updateRect, base, background, flags,
		borders);
}


void
DanoControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	DrawMenuFieldBackground(view, rect, updateRect, 0, 0, 0, 0, base,
		popupIndicator, flags);
}


void
DanoControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	DrawMenuFieldBackground(view, rect, updateRect, 0, 0, 0, 0, base,
		popupIndicator, flags);
}


void
DanoControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float,
	const rgb_color& base, bool popupIndicator, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Dano puts the arrow on the left. Haiku's menu field reserves that room on
	// the right, as item margins; swap them so the text moves over. A menu item
	// reads its margins as it draws, and their sum, so the field's size, is
	// unchanged.
	BMenuBar* menuBar = dynamic_cast<BMenuBar*>(view);
	if (menuBar != NULL && popupIndicator) {
		MenuMargins menu(menuBar);
		float left, top, right, bottom;
		menu.GetItemMargins(&left, &top, &right, &bottom);
		if (right > left)
			menu.SetItemMargins(right, top, left, bottom);
	}

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (view->Parent() != NULL
		&& view->Parent()->ViewColor() != B_TRANSPARENT_COLOR)
		background = view->Parent()->ViewColor();

	view->PushState();
	view->SetHighColor(background);
	view->FillRect(rect);
	view->PopState();

	if (menuBar == NULL) {
		_DrawFace(view, rect, base, flags & ~B_ACTIVATED, B_ALL_BORDERS);
		return;
	}

	rect = _DrawMenuFieldBox(view, menuBar, rect, background, flags);
}


/*!	Dano's menu field: a box with the panel's colour inside, a soft shadow, and a
	blue tab with a white arrow on the left, ending a little past the item's
	text. Haiku draws a menu field's frame in the field around its menu bar and
	its background in the bar; both call this with the bar's rect in their own
	coordinates, and each draws the part that falls in its view. Returns what is
	right of the tab.
*/
BRect
DanoControlLook::_DrawMenuFieldBox(BView* view, BMenuBar* menuBar, BRect bar,
	const rgb_color& background, uint32 flags)
{
	// where the item's text ends, and whether it has an arrow: the difference
	// between the item margins
	float right = bar.right;
	if (BMenuItem* item = menuBar->ItemAt(0)) {
		right = std::min(bar.right,
			bar.left + floorf(item->Frame().right) + 3);
	}
	MenuMargins menu(menuBar);
	float left, top, padRight, bottom;
	menu.GetItemMargins(&left, &top, &padRight, &bottom);
	const bool arrow = fabsf(left - padRight) > 0;

	font_height fontHeight;
	menuBar->GetFontHeight(&fontHeight);
	const float height = std::max(
		ceilf(fontHeight.ascent + fontHeight.descent) + 2, 19.0f);
	const float middle = floorf((bar.top + bar.bottom + 1) / 2);
	const BRect box(bar.left, middle - floorf(height / 2), right - 1,
		middle - floorf(height / 2) + height - 1);

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();
	if (!disabled)
		draw_shadow(view, box, kShadowNear, kShadowFar);
	draw_plate(view, box, outline_color(flags & ~B_ACTIVATED), background,
		disabled ? mix(kDisabledLine, background, 0.5f) : kCorner);

	BRect face = box.InsetByCopy(1, 1);
	if (arrow) {
		const float tabWidth = 9;
		BRect tab(face.left, face.top, face.left + tabWidth - 1, face.bottom);

		// its outer corners follow the box's
		view->SetHighColor(disabled ? background : kAccent);
		view->FillRect(BRect(tab.left + 1, tab.top, tab.right, tab.bottom));
		view->FillRect(BRect(tab.left, tab.top + 1, tab.left, tab.bottom - 1));

		view->SetHighColor(outline_color(flags & ~B_ACTIVATED));
		view->FillRect(BRect(tab.right + 1, box.top, tab.right + 1,
			box.bottom));

		// a small solid arrow pointing right
		const rgb_color color = disabled ? kDisabledLine : kWhite;
		const float x = tab.left + 3;
		const float y = floorf((tab.top + tab.bottom) / 2);
		view->SetHighColor(color);
		view->FillRect(BRect(x, y - 2, x, y + 2));
		view->FillRect(BRect(x + 1, y - 1, x + 1, y + 1));
		view->FillRect(BRect(x + 2, y, x + 2, y));

		face.left = tab.right + 2;
	}
	view->PopState();

	return face;
}


void
DanoControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;
	_DrawFace(view, rect, base, flags, borders);
}


void
DanoControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// a highlight along the top and left; two shades along the bottom
	const rgb_color light = mix(base, kWhite, 0.6f);
	_DrawFrame(view, rect, light, light, base, kFrameInner, borders);

	view->PushState();
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->SetHighColor(kFrameOuter);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	}

	// menus draw their items in the view's colours afterwards: leave them
	BRect fill(rect);
	if ((borders & B_BOTTOM_BORDER) != 0)
		fill.bottom--;
	view->SetHighColor(base);
	view->FillRect(fill);
	view->PopState();
}


void
DanoControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// a white edge along the top and left, inside the window's own border
	_DrawFrame(view, rect, kFace, kFace, base, base, borders);

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);
	view->PopState();
}


void
DanoControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// BMenuItem draws its label in the view's high colour right after this, so
	// it must be left as it was
	view->PushState();

	// a blue bar with a dark outline and softly cut corners
	fill_cut(view, rect, 1, mix(base, kBlack, 0.6f));
	fill_cut(view, rect.InsetByCopy(1, 1), 0, base);

	view->PopState();

	rect.InsetBy(1, 1);
}


//	#pragma mark - text, check boxes, radio buttons, groups


void
DanoControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	const BRect outer(rect);

	rgb_color outline = outline_color(flags);
	if (!disabled && (flags & B_INVALID) != 0)
		outline = ui_color(B_FAILURE_COLOR);

	view->PushState();

	// the black line, then a grey one along the top and left that the white
	// field shows inside; the right and bottom go straight to the field
	_DrawFrame(view, rect, outline, outline, outline, outline, borders);
	const rgb_color inner = disabled ? base : kFieldInner;
	const rgb_color field = disabled ? base
		: ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	_DrawFrame(view, rect, inner, inner, field, field, borders);

	// softened corners
	if (borders == B_ALL_BORDERS) {
		const rgb_color corner = disabled ? mix(kDisabledLine, base, 0.5f)
			: kCorner;
		pixel(view, outer.left, outer.top, base);
		pixel(view, outer.right, outer.top, base);
		pixel(view, outer.left, outer.bottom, base);
		pixel(view, outer.right, outer.bottom, base);
		pixel(view, outer.left + 1, outer.top, corner);
		pixel(view, outer.left, outer.top + 1, corner);
		pixel(view, outer.right - 1, outer.top, corner);
		pixel(view, outer.right, outer.top + 1, corner);
		pixel(view, outer.left + 1, outer.bottom, corner);
		pixel(view, outer.left, outer.bottom - 1, corner);
		pixel(view, outer.right - 1, outer.bottom, corner);
		pixel(view, outer.right, outer.bottom - 1, corner);
		pixel(view, outer.left + 1, outer.top + 1, outline);
		pixel(view, outer.right - 1, outer.top + 1, outline);
		pixel(view, outer.left + 1, outer.bottom - 1, outline);
		pixel(view, outer.right - 1, outer.bottom - 1, outline);
	}

	view->PopState();
}


void
DanoControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const int32 scale = mark_scale(rect.Width() + 1, 10);
	const float left = rect.left + floorf((rect.Width() + 1 - 10 * scale) / 2);
	const float top = rect.top + floorf((rect.Height() + 1 - 10 * scale) / 2);

	if ((flags & B_ACTIVATED) != 0) {
		draw_mark(view, kCheckOn, kCheckOnRows, left - 3 * scale,
			top - 5 * scale, scale, base, flags);
	} else {
		draw_mark(view, kCheckOff, kCheckOffRows, left, top, scale, base, flags);
		if ((flags & B_PARTIALLY_ACTIVATED) != 0) {
			view->SetHighColor(outline_color(flags));
			view->FillRect(BRect(left + 2 * scale, top + 4 * scale,
				left + 8 * scale - 1, top + 6 * scale - 1));
		}
	}

	view->PopState();
}


void
DanoControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const int32 scale = mark_scale(rect.Width() + 1, 11);
	const float left = rect.left + floorf((rect.Width() + 1 - 11 * scale) / 2);
	const float top = rect.top + floorf((rect.Height() + 1 - 11 * scale) / 2);

	if ((flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0)
		draw_mark(view, kRadioOn, kRadioOnRows, left, top, scale, base, flags);
	else
		draw_mark(view, kRadioOff, kRadioOffRows, left, top, scale, base, flags);

	view->PopState();
}


void
DanoControlLook::DrawGroupFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 borders)
{
	if (dynamic_cast<BTabView*>(view) != NULL) {
		// a tab's page continues its tab: white on the top and left, dark on the
		// right and bottom
		_DrawFrame(view, rect, kWhite, kWhite, kTabDark, kTabDark, borders);
		return;
	}
	if (borders != B_ALL_BORDERS) {
		ForwardingControlLook::DrawGroupFrame(view, rect, updateRect, base,
			borders);
		return;
	}

	// One grey line with stepped corners, and a thick band along the top that
	// the label interrupts; BBox draws that over it afterwards.
	view->PushState();
	fill_cut(view, rect, 2, kGroup);
	fill_cut(view, rect.InsetByCopy(1, 1), 1, base);

	BRect band(rect.left, rect.top - 2, rect.right, rect.top + 2);
	if (band.top < view->Bounds().top)
		band.top = view->Bounds().top;
	view->SetHighColor(kGroup);
	view->FillRect(BRect(band.left + 2, band.top, band.right - 2, band.bottom));
	view->FillRect(BRect(band.left + 1, band.top + 1, band.left + 1,
		band.bottom));
	view->FillRect(BRect(band.right - 1, band.top + 1, band.right - 1,
		band.bottom));
	view->FillRect(BRect(band.left, band.top + 2, band.left, band.bottom));
	view->FillRect(BRect(band.right, band.top + 2, band.right, band.bottom));
	view->PopState();

	rect.InsetBy(3, 3);
}


//	#pragma mark - scroll bars


void
DanoControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style borderStyle, uint32 flags, uint32 borders)
{
	BRect outer(rect);
	ForwardingControlLook::DrawScrollViewFrame(view, outer, updateRect,
		verticalScrollBarFrame, horizontalScrollBarFrame, base, borderStyle,
		flags, borders);
	BRect result(outer);

	if (borderStyle != B_FANCY_BORDER) {
		rect = result;
		return;
	}

	// Haiku's look has laid out and painted what goes inside; give it Dano's
	// sunken edge: two lines dark along the top and left, a white one outside
	// a dark one along the bottom and right.
	outer = rect;
	view->PushState();
	_DrawFrame(view, outer, kFrameOuter, kFrameOuter, kWhite, kWhite, borders);
	if ((flags & B_FOCUSED) == 0) {
		_DrawFrame(view, outer, kFrameInner, kFrameInner, kFrameInner,
			kFrameInner, borders);
	}
	view->PopState();

	rect = result;
}


void
DanoControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& text,
	uint32 flags, int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color face = down ? kTrough : base;

	BRect inner(rect);
	if (down) {
		_DrawFrame(view, inner, kButtonEdge, kButtonEdge, kWhite, kWhite);
	} else {
		_DrawFrame(view, inner, disabled ? base : kWhite,
			disabled ? base : kWhite, kButtonEdge, kButtonEdge);
	}
	view->SetHighColor(face);
	view->FillRect(inner);

	_DrawTriangle(view, inner, direction,
		disabled ? mix(kArrow, base, 0.6f) : kArrow,
		disabled ? base : kArrowFill);

	view->PopState();
}


void
DanoControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base,
	uint32 flags, orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags,
		orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags,
		orientation);

	// the thumb's shadow falls on the part of the trough after it
	if ((flags & B_DISABLED) == 0 && rect2.IsValid()) {
		view->PushState();
		view->SetHighColor(kThumbEnd);
		if (orientation == B_HORIZONTAL) {
			view->FillRect(BRect(rect2.left, rect2.top, rect2.left,
				rect2.bottom));
		} else {
			view->FillRect(BRect(rect2.left, rect2.top, rect2.right,
				rect2.top));
		}
		view->PopState();
	}
}


void
DanoControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);
	view->SetDrawingMode(B_OP_COPY);

	if ((flags & B_DISABLED) != 0) {
		view->SetHighColor(mix(kTrough, base, 0.5f));
		view->FillRect(rect);
		view->PopState();
		return;
	}

	view->SetHighColor(kTrough);
	view->FillRect(rect);

	// sunken: darker along the side the light comes from
	view->SetHighColor(kTroughEdge);
	if (orientation == B_HORIZONTAL) {
		view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top + 1));
		view->SetHighColor(base);
		view->FillRect(BRect(rect.left, rect.bottom, rect.right, rect.bottom));
	} else {
		view->FillRect(BRect(rect.left, rect.top, rect.left + 1, rect.bottom));
		view->SetHighColor(base);
		view->FillRect(BRect(rect.right, rect.top, rect.right, rect.bottom));
	}

	view->PopState();
}


void
DanoControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->ClipToRect(rect);

	if ((flags & B_DISABLED) != 0) {
		view->SetHighColor(mix(kTrough, base, 0.5f));
		view->FillRect(rect);
		view->PopState();
		return;
	}

	BRect inner(rect);
	_DrawFrame(view, inner, kWhite, kWhite, kTroughEdge, kTroughEdge);
	view->SetHighColor(base);
	view->FillRect(inner);

	// three small raised bars across the middle, whatever the knob style
	const float along = orientation == B_HORIZONTAL ? inner.Width() : inner.Height();
	const float across = orientation == B_HORIZONTAL ? inner.Height() : inner.Width();
	if (along >= 14 && across >= 5) {
		const BPoint center(floorf((inner.left + inner.right) / 2),
			floorf((inner.top + inner.bottom) / 2));
		const float reach = floorf(std::min(across, 12.0f) / 2) - 2;
		for (int32 i = -1; i <= 1; i++) {
			BRect bar;
			if (orientation == B_HORIZONTAL) {
				const float x = center.x + i * 4;
				bar.Set(x - 1, center.y - reach, x + 1, center.y + reach);
			} else {
				const float y = center.y + i * 4;
				bar.Set(center.x - reach, y - 1, center.x + reach, y + 1);
			}
			_DrawFrame(view, bar, kWhite, kWhite, kTroughEdge, kTroughEdge);
		}
	}

	view->PopState();
}


void
DanoControlLook::DrawArrowShape(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 direction,
	uint32 flags, float tint)
{
	if (direction > B_DOWN_ARROW) {
		ForwardingControlLook::DrawArrowShape(view, rect, updateRect, base,
			direction, flags, tint);
		return;
	}
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool disabled = (flags & B_DISABLED) != 0;
	_DrawTriangle(view, rect, direction,
		disabled ? mix(kArrow, base, 0.6f) : kArrow,
		disabled ? base : kArrowFill);
}


//	#pragma mark - sliders


rgb_color
DanoControlLook::SliderBarColor(const rgb_color& base)
{
	return kGroove;
}


void
DanoControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color leftFillColor,
	rgb_color rightFillColor, float sliderScale, uint32 flags,
	orientation orientation)
{
	if (leftFillColor == rightFillColor) {
		DrawSliderBar(view, rect, updateRect, base, leftFillColor, flags,
			orientation);
		return;
	}

	// each side through the one-colour version, clipped wide enough for the
	// thicker bar it draws
	BRect leftSide(rect);
	BRect rightSide(rect);
	if (orientation == B_HORIZONTAL) {
		leftSide.InsetBy(0, -2);
		rightSide.InsetBy(0, -2);
		const float position = floorf(rect.left + 2
			+ (rect.Width() - 2) * sliderScale);
		leftSide.right = position - 1;
		rightSide.left = position;
	} else {
		leftSide.InsetBy(-2, 0);
		rightSide.InsetBy(-2, 0);
		const float position = floorf(rect.top + 2
			+ (rect.Height() - 2) * (1.0f - sliderScale));
		leftSide.top = position;
		rightSide.bottom = position - 1;
	}

	view->PushState();
	view->ClipToRect(leftSide);
	DrawSliderBar(view, rect, updateRect, base, leftFillColor, flags,
		orientation);
	view->PopState();

	view->PushState();
	view->ClipToRect(rightSide);
	DrawSliderBar(view, rect, updateRect, base, rightFillColor, flags,
		orientation);
	view->PopState();
}


void
DanoControlLook::DrawSliderBar(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, rgb_color fillColor,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// BSlider asks for a thinner bar than Dano's; its thumb covers the extra
	if (orientation == B_HORIZONTAL)
		rect.InsetBy(0, -2);
	else
		rect.InsetBy(-2, 0);

	const bool disabled = (flags & B_DISABLED) != 0;

	view->PushState();

	draw_plate(view, rect, disabled ? kDisabledLine : kBlack,
		disabled ? base : fillColor, disabled ? base : kShadowFar);

	// shaded inside along the top and left, as if sunken
	if (!disabled && fillColor == kGroove) {
		const BRect inner = rect.InsetByCopy(1, 1);
		view->SetHighColor(kGrooveEdge);
		view->FillRect(BRect(inner.left + 1, inner.top, inner.right - 1,
			inner.top));
		view->FillRect(BRect(inner.left, inner.top + 1, inner.left,
			inner.bottom - 1));
		view->SetHighColor(kGrooveInner);
		view->FillRect(BRect(inner.left + 2, inner.top + 1, inner.right - 1,
			inner.top + 1));
		view->FillRect(BRect(inner.left + 1, inner.top + 2, inner.left + 1,
			inner.bottom - 1));
	}

	view->PopState();
}


void
DanoControlLook::DrawSliderHashMarks(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, int32 count,
	hash_mark_location location, uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect) || count < 2
		|| location == B_HASH_MARKS_NONE) {
		return;
	}

	// small dots inside the bar, which BSlider centres this rect on
	view->PushState();
	view->SetHighColor((flags & B_DISABLED) != 0 ? kDisabledLine
		: (rgb_color){ 88, 92, 88, 255 });
	if (orientation == B_HORIZONTAL) {
		const float y = floorf((rect.top + rect.bottom) / 2);
		const float step = (rect.Width() - 2) / (count - 1);
		for (int32 i = 0; i < count; i++) {
			const float x = floorf(rect.left + 1 + i * step);
			view->FillRect(BRect(x, y, x, y + 1));
		}
	} else {
		const float x = floorf((rect.left + rect.right) / 2);
		const float step = (rect.Height() - 2) / (count - 1);
		for (int32 i = 0; i < count; i++) {
			const float y = floorf(rect.top + 1 + i * step);
			view->FillRect(BRect(x, y, x + 1, y));
		}
	}
	view->PopState();
}


void
DanoControlLook::DrawSliderThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	const bool disabled = (flags & B_DISABLED) != 0;

	// room for the shadow on the right and bottom
	BRect knob(rect);
	knob.right -= 2;
	knob.bottom -= 2;
	if (orientation == B_HORIZONTAL) {
		const float width = std::min(knob.Width(), 10.0f);
		knob.left += floorf((knob.Width() - width) / 2);
		knob.right = knob.left + width;
	} else {
		const float height = std::min(knob.Height(), 10.0f);
		knob.top += floorf((knob.Height() - height) / 2);
		knob.bottom = knob.top + height;
	}

	if (!disabled)
		draw_shadow(view, knob, kShadowNear, kShadowFar);
	draw_plate(view, knob, outline_color(flags & ~B_ACTIVATED),
		disabled ? base : kThumb, disabled ? base : kCorner);

	// a short dash in the middle
	const float x = floorf((knob.left + knob.right) / 2);
	const float y = floorf((knob.top + knob.bottom) / 2);
	view->SetHighColor(disabled ? kDisabledLine : kBlack);
	if (orientation == B_HORIZONTAL)
		view->FillRect(BRect(x, y, x, y + 1));
	else
		view->FillRect(BRect(x, y, x + 1, y));

	view->PopState();

	rect = knob.InsetByCopy(2, 2);
}


void
DanoControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawSliderTriangle(view, rect, updateRect, base, kThumb, flags,
		orientation);
}


void
DanoControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	if (orientation != B_HORIZONTAL) {
		ForwardingControlLook::DrawSliderTriangle(view, rect, updateRect, base, fill,
			flags, orientation);
		return;
	}

	const bool disabled = (flags & B_DISABLED) != 0;
	const float left = floorf((rect.left + rect.right) / 2) - 6;
	const float top = rect.bottom - kTriangleRows + 1;

	view->PushState();
	for (int32 row = 0; row < kTriangleRows; row++) {
		for (int32 column = 0; kTriangle[row][column] != '\0'; column++) {
			rgb_color color;
			switch (kTriangle[row][column]) {
				case '#': color = outline_color(flags & ~B_ACTIVATED); break;
				case 'C': color = (rgb_color){ 120, 120, 120, 255 }; break;
				case 'D': color = (rgb_color){ 200, 200, 200, 255 }; break;
				case 'E': color = (rgb_color){ 224, 228, 224, 255 }; break;
				case 's': color = kFieldInner; break;
				case 'z': color = (rgb_color){ 96, 100, 96, 255 }; break;
				default:
					continue;
			}
			if (disabled)
				color = mix(color, base, 0.6f);
			pixel(view, left + column, top + row, color);
		}
	}
	view->PopState();
}


//	#pragma mark - status bars


void
DanoControlLook::DrawStatusBar(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& barColor,
	float progressPosition)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	fill_cut(view, rect, 1, kBlack);
	pixel(view, rect.left, rect.top, base);
	pixel(view, rect.right, rect.top, base);
	pixel(view, rect.left, rect.bottom, base);
	pixel(view, rect.right, rect.bottom, base);

	BRect inner = rect.InsetByCopy(1, 1);
	const float split = std::max(inner.left - 1,
		std::min(floorf(progressPosition), inner.right));

	if (split >= inner.left) {
		BRect full(inner.left, inner.top, split, inner.bottom);
		_DrawFrame(view, full, mix(barColor, kBlack, 0.3f),
			mix(barColor, kBlack, 0.3f), barColor, barColor,
			B_LEFT_BORDER | B_TOP_BORDER);
		_DrawFrame(view, full, mix(barColor, kBlack, 0.15f),
			mix(barColor, kBlack, 0.15f), barColor, barColor,
			B_LEFT_BORDER | B_TOP_BORDER);
		view->SetHighColor(barColor);
		view->FillRect(full);
	}

	if (split < inner.right) {
		BRect empty(split + 1, inner.top, inner.right, inner.bottom);
		if (split >= inner.left) {
			view->SetHighColor(kBlack);
			view->FillRect(BRect(empty.left, empty.top, empty.left,
				empty.bottom));
			empty.left++;
		}
		_DrawFrame(view, empty, kStatusEdge, kStatusEdge, kStatusEmpty,
			kStatusEmpty, B_LEFT_BORDER | B_TOP_BORDER);
		_DrawFrame(view, empty, kStatusInner, kStatusInner, kStatusEmpty,
			kStatusEmpty, B_LEFT_BORDER | B_TOP_BORDER);
		view->SetHighColor(kStatusEmpty);
		view->FillRect(empty);
	}

	view->PopState();
}


//	#pragma mark - tabs


void
DanoControlLook::DrawTabFrame(BView* view, BRect& rect,
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

	// the strip behind the tabs, with the page's white top edge along it
	view->SetHighColor(base);
	view->FillRect(rect);

	if (borderStyle == B_NO_BORDER)
		return;

	view->SetHighColor(kWhite);
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
}


void
DanoControlLook::DrawActiveTab(BView* view, BRect& rect,
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
	_DrawTab(view, rect, base, flags, true);
}


void
DanoControlLook::DrawInactiveTab(BView* view, BRect& rect,
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
	_DrawTab(view, rect, base, flags, false);
}


//	#pragma mark - borders


void
DanoControlLook::DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, border_style borderStyle, uint32 flags,
	uint32 borders)
{
	if (dynamic_cast<BTabView*>(view) == NULL || borderStyle == B_NO_BORDER) {
		ForwardingControlLook::DrawBorder(view, rect, updateRect, base, borderStyle,
			flags, borders);
		return;
	}

	// a tab's page continues its tab: white on the top and left, dark on the
	// right and bottom, and the same room as Haiku's look takes
	_DrawFrame(view, rect, kWhite, kWhite, kTabDark, kTabDark, borders);
	if (borderStyle == B_FANCY_BORDER)
		_DrawFrame(view, rect, base, base, base, base, borders);
}


//	#pragma mark - labels


void
DanoControlLook::DrawLabel(BView* view, const char* label, const BBitmap* icon,
	BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags,
	const BAlignment& alignment, const rgb_color* textColor)
{
	if (textColor == NULL && (flags & B_ACTIVATED) != 0
		&& dynamic_cast<BButton*>(view) != NULL) {
		textColor = &kPressedLabel;
	}
	ForwardingControlLook::DrawLabel(view, label, icon, rect, updateRect, base,
		flags, alignment, textColor);
}


//	#pragma mark - private


/*!	Dano's button: a one pixel black outline with cut corners and a shadow two
	pixels wide on the bottom right. There are two pixels above the outline, to
	balance those below it. A default button sits on a black-edged blue plate
	that takes five pixels, and has its shadow in the plate's darker blues.
*/
void
DanoControlLook::_DrawDanoButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!rect.IsValid())
		return;

	if (is_flat(flags)) {
		_DrawFrame(view, rect, background, background, background,
			background, borders);
		_DrawFrame(view, rect, background, background, background,
			background, borders);
		return;
	}

	const bool blend = (flags & B_BLEND_FRAME) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;

	view->PushState();
	view->ClipToRect(rect);
	if (blend)
		view->SetDrawingMode(B_OP_ALPHA);
	else {
		view->SetHighColor(background);
		view->FillRect(rect);
	}

	_FitButtonHeight(view, rect, flags, borders);

	const rgb_color outline = outline_color(flags);
	const rgb_color face = face_color(flags);
	const rgb_color corner = disabled ? mix(kDisabledLine, background, 0.5f)
		: kCorner;

	if (isDefault) {
		const BRect plate = extend_past_open_sides(rect, borders, 8);
		fill_cut(view, plate, 2, disabled ? kDisabledLine : kBlack);
		fill_cut(view, plate.InsetByCopy(1, 1), 1,
			disabled ? mix(kAccent, background, 0.6f) : kAccent);

		BRect box(rect.left + 6, rect.top + 2, rect.right - 6, rect.bottom - 2);
		box = extend_past_open_sides(box, borders, 8);
		if (!disabled) {
			fill_cut(view, box.OffsetByCopy(2, 1), 2, kAccentFar);
			fill_cut(view, box.OffsetByCopy(1, 1), 2, kAccentNear);
		}
		draw_plate(view, box, outline, face, kAccentFar);
		inset_by_borders(rect, 5, borders);
	} else {
		BRect box(rect.left, rect.top + 2, rect.right - 2, rect.bottom - 2);
		box = extend_past_open_sides(box, borders, 8);
		if (!disabled)
			draw_shadow(view, box, kShadowNear, kShadowFar);
		draw_plate(view, box, outline, face, corner);

		// the same room for the label as Haiku's, centred in the outline
		rect = box;
		rect.left -= 1;
		rect.right += 1;
		inset_by_borders(rect, 2, borders);
		rect.top = std::min(rect.top, box.top + 1);
		rect.bottom = std::max(rect.bottom, box.bottom - 1);
	}

	view->PopState();
}


/*!	BButton sizes itself for more than Dano's 21 pixel outline. When \a rect is
	the height BButton would choose on its own, the button is drawn at Dano's
	height, centred; BButton centres its label in whatever rect it gets back, so
	the label follows. Taller buttons (icons, explicit sizes) are left alone.
*/
void
DanoControlLook::_FitButtonHeight(BView* view, BRect& rect, uint32 flags,
	uint32 borders)
{
	if ((borders & (B_TOP_BORDER | B_BOTTOM_BORDER))
			!= (B_TOP_BORDER | B_BOTTOM_BORDER))
		return;

	font_height fontHeight;
	view->GetFontHeight(&fontHeight);
	const float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);
	const bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;

	// what BButton::_ValidatePreferredSize() would pick, give or take
	const float automatic = std::max(ceilf(textHeight * 1.8f),
		textHeight + (isDefault ? 18 : 12));
	// the outline, the two rows above and the shadow's two below
	const float wanted = std::max(textHeight + 5, 21.0f) + 4;

	const float height = rect.Height() + 1;
	if (height <= wanted || height > automatic + 1)
		return;

	const float extra = height - wanted;
	rect.top += floorf(extra / 2);
	rect.bottom -= ceilf(extra / 2);
}


/*!	A flat face inside the outline, which the frame has already drawn. */
void
DanoControlLook::_DrawFace(BView* view, BRect& rect, const rgb_color& base,
	uint32 flags, uint32 borders)
{
	if (!rect.IsValid())
		return;

	if (is_flat(flags)) {
		_DrawFrame(view, rect, base, base, base, base, borders);
		view->SetHighColor(base);
		view->FillRect(rect);
		return;
	}

	view->SetHighColor(face_color(flags));
	view->FillRect(rect);
	inset_by_borders(rect, 1, borders);
}


/*!	An outlined triangle centred in \a rect, set pixel by pixel so that it stays
	crisp and free of the colour fringes subpixel anti-aliasing would give it.
*/
void
DanoControlLook::_DrawTriangle(BView* view, BRect rect, uint32 direction,
	const rgb_color& outline, const rgb_color& fill)
{
	const bool horizontal = direction == B_LEFT_ARROW
		|| direction == B_RIGHT_ARROW;
	const float across = (horizontal ? rect.Height() : rect.Width()) + 1;
	const float along = (horizontal ? rect.Width() : rect.Height()) + 1;

	int32 length = (int32)std::min(across * 0.85f, along * 1.6f);
	if (length % 2 == 0)
		length--;
	length = std::max(length, (int32)5);
	const int32 depth = (int32)std::max(3.0f,
		std::min(roundf(length * 0.75f), along - 2));

	const int32 originAcross = (int32)floorf((horizontal
		? (rect.top + rect.bottom) : (rect.left + rect.right)) / 2)
		- length / 2;
	const int32 originAlong = (int32)floorf((horizontal
		? (rect.left + rect.right) : (rect.top + rect.bottom)) / 2)
		- depth / 2;

	// extent of each column across the base, from base to tip
	int32 extent[length];
	const float middle = (length - 1) / 2.0f;
	for (int32 k = 0; k < length; k++) {
		extent[k] = std::max((int32)1, (int32)roundf(depth
			* (1 - fabsf(k - middle) / (middle + 0.5f))));
	}

	view->PushState();
	view->SetDrawingMode(B_OP_COPY);

	for (int32 k = 0; k < length; k++) {
		for (int32 j = 0; j < extent[k]; j++) {
			const bool edge = j == 0 || j == extent[k] - 1 || k == 0
				|| k == length - 1 || extent[k - 1] <= j
				|| extent[k + 1] <= j;

			// j runs from the base towards the point
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
			pixel(view, x, y, edge ? outline : fill);
		}
	}

	view->PopState();
}


/*!	A tab sweeps out at the bottom: a white line up its left and along its top,
	a dark one down its right. All of them are the same height, and an active
	tab just leaves out the white line along its bottom, opening into the page.
*/
void
DanoControlLook::_DrawTab(BView* view, BRect& rect, const rgb_color& base,
	uint32 flags, bool active)
{
	rect.left = floorf(rect.left);
	rect.right = floorf(rect.right);
	rect.top = floorf(rect.top);
	rect.bottom = floorf(rect.bottom);

	view->PushState();
	view->ClipToRect(rect);

	view->SetHighColor(base);
	view->FillRect(rect);

	const bool disabled = (flags & B_DISABLED) != 0;
	const rgb_color light = disabled ? mix(kWhite, base, 0.5f) : kWhite;
	const rgb_color dark = disabled ? mix(kTabDark, base, 0.5f) : kTabDark;

	const float left = rect.left;
	const float right = rect.right;
	const float top = rect.top + 2;
	const float bottom = rect.bottom;

	// the page's top edge
	if (!active) {
		view->SetHighColor(light);
		view->FillRect(BRect(left, bottom, right, bottom));
	}

	// up the left side, along the top
	view->SetHighColor(light);
	view->FillRect(BRect(left, bottom, left + 1, bottom));
	view->FillRect(BRect(left + 2, bottom - 1, left + 2, bottom - 1));
	view->FillRect(BRect(left + 3, bottom - 3, left + 3, bottom - 2));
	view->FillRect(BRect(left + 4, top + 4, left + 4, bottom - 4));
	view->FillRect(BRect(left + 5, top + 2, left + 5, top + 3));
	view->FillRect(BRect(left + 6, top + 1, left + 7, top + 1));
	view->FillRect(BRect(left + 8, top, right - 8, top));

	// down the right side
	view->SetHighColor(light);
	view->FillRect(BRect(right - 7, top + 1, right - 6, top + 1));
	view->SetHighColor(dark);
	view->FillRect(BRect(right - 5, top + 2, right - 5, top + 3));
	view->FillRect(BRect(right - 4, top + 4, right - 4, bottom - 4));
	view->FillRect(BRect(right - 3, bottom - 3, right - 3, bottom - 2));
	view->FillRect(BRect(right - 2, bottom - 1, right - 2, bottom - 1));
	view->FillRect(BRect(right - 1, bottom, right, bottom));

	view->PopState();

	// the same room for the label as Haiku's look leaves
	rect.left += 3;
	rect.right -= 3;
	rect.top += active ? 3 : 2;
	rect.bottom -= active ? 1 : 2;
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::DanoControlLook();
}
