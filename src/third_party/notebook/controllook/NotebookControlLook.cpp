/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See NotebookControlLook.h. The pictures, in NotebookControlArt.h, are from
 * ../art, and buttons are stretched as notebook.uis has them.
 */


#include "NotebookControlLook.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <new>
#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Locker.h>
#include <Menu.h>
#include <Rect.h>
#include <View.h>

#include "NotebookControlArt.h"


namespace BPrivate {


// the rule under the menu bar, from the skin's menu.bmp
static const rgb_color kMenuRule = { 17, 5, 96, 255 };
// how much of a button's picture is its edge, kept as it is when stretched
static const float kButtonEdge = 2;


//!	A picture as a bitmap, made once and kept.
static BBitmap*
bitmap_for(const Art& art)
{
	static std::map<const uint32*, BBitmap*> sBitmaps;
	static BLocker sLock("notebook art");
	BAutolock locker(sLock);

	std::map<const uint32*, BBitmap*>::iterator found = sBitmaps.find(art.pixels);
	if (found != sBitmaps.end())
		return found->second;

	BBitmap* bitmap = new(std::nothrow) BBitmap(
		BRect(0, 0, art.width - 1, art.height - 1), B_RGBA32);
	if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return NULL;
	}
	// B_RGBA32 is the pixels' ARGB as a little endian word
	const int32 rowBytes = bitmap->BytesPerRow();
	for (int32 y = 0; y < art.height; y++) {
		memcpy((uint8*)bitmap->Bits() + y * rowBytes,
			art.pixels + y * art.width, art.width * 4);
	}
	sBitmaps[art.pixels] = bitmap;
	return bitmap;
}


/*!	Draws a picture over rect, its corners as they are, its edges stretched
	along and its middle across, without smoothing.
*/
static void
draw_stretched(BView* view, const Art& art, BRect rect, float border)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL || !rect.IsValid())
		return;

	const float w = art.width;
	const float h = art.height;
	const float across = std::min(border, floorf((rect.Width() + 1) / 2));
	const float down = std::min(border, floorf((rect.Height() + 1) / 2));

	const float sx[4] = { 0, across, w - across, w };
	const float sy[4] = { 0, down, h - down, h };
	const float dx[4] = { rect.left, rect.left + across,
		rect.right + 1 - across, rect.right + 1 };
	const float dy[4] = { rect.top, rect.top + down, rect.bottom + 1 - down,
		rect.bottom + 1 };

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	for (int32 row = 0; row < 3; row++) {
		for (int32 column = 0; column < 3; column++) {
			if (sx[column + 1] <= sx[column] || sy[row + 1] <= sy[row]
				|| dx[column + 1] <= dx[column] || dy[row + 1] <= dy[row])
				continue;
			view->DrawBitmap(bitmap, BRect(sx[column], sy[row],
				sx[column + 1] - 1, sy[row + 1] - 1), BRect(dx[column],
				dy[row], dx[column + 1] - 1, dy[row + 1] - 1));
		}
	}
	view->PopState();
}


//!	Draws a picture as it is, in the middle of rect.
static void
draw_centred(BView* view, const Art& art, const BRect& rect)
{
	BBitmap* bitmap = bitmap_for(art);
	if (bitmap == NULL)
		return;
	const BPoint where(floorf((rect.left + rect.right + 1 - art.width) / 2),
		floorf((rect.top + rect.bottom + 1 - art.height) / 2));
	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->DrawBitmap(bitmap, where);
	view->PopState();
}


//!	A button's picture for its state.
static const Art&
button_art(uint32 flags)
{
	if ((flags & BControlLook::B_DISABLED) != 0)
		return kButtonDisabled;
	if ((flags & (BControlLook::B_ACTIVATED | BControlLook::B_PARTIALLY_ACTIVATED
			| BControlLook::B_CLICKED)) != 0)
		return kButtonPressed;
	if ((flags & BControlLook::B_DEFAULT_BUTTON) != 0)
		return kButtonDefault;
	if ((flags & BControlLook::B_FOCUSED) != 0)
		return kButtonFocused;
	return kButton;
}


//	#pragma mark -


NotebookControlLook::NotebookControlLook()
{
}


NotebookControlLook::~NotebookControlLook()
{
}


//	#pragma mark - buttons


//!	The button's picture, stretched, face and all.
void
NotebookControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (ShouldDraw(view, rect, updateRect)) {
		view->PushState();
		view->ClipToRect(rect);
		view->SetHighColor(background);
		view->FillRect(rect);
		draw_stretched(view, button_art(flags), rect, kButtonEdge);
		view->PopState();
	}
	rect.InsetBy(kButtonEdge, kButtonEdge);
}


void
NotebookControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	// Tracker's column headers, and the Deskbar's items in a menu, are drawn
	// as a button's background alone, and get a button's picture
	if ((view->Name() != NULL && strcmp(view->Name(), "TitleView") == 0)
		|| dynamic_cast<BMenu*>(view) != NULL) {
		if (ShouldDraw(view, rect, updateRect))
			draw_stretched(view, button_art(flags), rect, kButtonEdge);
		return;
	}

	// the frame drew the picture's face
	rect.InsetBy(1, 1);
}


//	#pragma mark - menus


//!	Paper with a navy rule along its bottom, as under a notebook's heading.
void
NotebookControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();
	view->SetHighColor(base);
	view->FillRect(rect);
	// Tracker's item count uses this too, and shouldn't look like a menu bar
	if (view->Name() == NULL || strcmp(view->Name(), "CountVw") != 0) {
		view->SetHighColor(kMenuRule);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}
	view->PopState();
}


//	#pragma mark - check boxes and radio buttons


void
NotebookControlLook::DrawCheckBox(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool on = (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, disabled ? (on ? kCheckBoxOnDisabled : kCheckBoxDisabled)
		: (on ? kCheckBoxOn : kCheckBox), r);
}


void
NotebookControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	const bool on = (flags & B_ACTIVATED) != 0;
	const bool disabled = (flags & B_DISABLED) != 0;
	BRect r(rect);
	r.bottom = r.top + r.Width();
	draw_centred(view, disabled ? (on ? kRadioOnDisabled : kRadioDisabled)
		: (on ? kRadioOn : kRadio), r);
}


}	// namespace BPrivate


extern "C" BControlLook*
instantiate_control_look(image_id id)
{
	return new (std::nothrow) BPrivate::NotebookControlLook();
}
