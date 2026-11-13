/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Canvas: a picture with an alpha channel drawn in memory, for windows that
 * are seen through on a composited desktop. app_server keeps a pixel's alpha
 * only when a B_RGBA32 bitmap is copied, so everything here, text included,
 * is drawn into one bitmap that is copied to the window at once.
 */
#ifndef POWER_CANVAS_H
#define POWER_CANVAS_H


#include <Bitmap.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include <Rect.h>


class BView;


class Canvas {
public:
								Canvas();
								~Canvas();

	// Makes it \a width by \a height, cleared.
			bool				SetSize(int32 width, int32 height);
			int32				Width() const { return fWidth; }
			int32				Height() const { return fHeight; }
			void				Clear();

	// Mixes \a color, at its alpha times \a cover, over what is there.
			void				Blend(int32 x, int32 y, rgb_color color,
									uint8 cover = 255);
			void				FillRect(BRect rect, rgb_color color);
			// from \a top's colour at the top to \a bottom's at the bottom
			void				FillGradient(BRect rect, rgb_color top,
									rgb_color bottom);
			void				FillRoundRect(BRect rect, float radius,
									rgb_color color);
			void				FillCircle(BPoint center, float radius,
									rgb_color inner, rgb_color outer);
			/*!	A shape whose edges run straight from (\a backLeft, \a top)
				to (\a frontLeft, \a bottom) and from (\a backRight, \a top)
				to (\a frontRight, \a bottom), shaded from \a top's colour to
				\a bottom's.
			*/
			void				FillTrapezoid(float top, float bottom,
									float backLeft, float backRight,
									float frontLeft, float frontRight,
									rgb_color topColor, rgb_color bottomColor);

	/*!	Draws \a bitmap (B_RGBA32) scaled into \a dest, at \a alpha. With
		\a mirror, it is turned upside down and fades out from \a alpha at
		its top to nothing \a fade pixels down. Only rows from \a clipTop to
		\a clipBottom are drawn.
	*/
			void				DrawBitmap(const BBitmap* bitmap, BRect dest,
									uint8 alpha = 255, bool mirror = false,
									float fade = 0, float clipTop = -1e6,
									float clipBottom = 1e6);
	/*!	Draws \a bitmap (B_RGBA32) as a square \a size pixels wide with its
		top left corner at (\a left, \a top), which needn't be whole pixels:
		it is sampled between pixels, and its edges partly cover the pixels
		they cross, so that it moves and grows smoothly. Meant for scaling
		down to no less than half, or up. \a mirror, \a fade and the clip
		rows are as for DrawBitmap(), the reflection's top at \a top.
	*/
			void				DrawSmooth(const BBitmap* bitmap, float left,
									float top, float size, uint8 alpha = 255,
									bool mirror = false, float fade = 0,
									float clipTop = -1e6,
									float clipBottom = 1e6);
	// \a bitmap (B_RGBA32) scaled to \a size by \a size, averaged
	static	BBitmap*			Scaled(const BBitmap* bitmap, int32 size);

	// \a text, its baseline's left end at \a where
			void				DrawString(const char* text,
									const BFont& font, BPoint where,
									rgb_color color);

	// Copies it to \a view, its top left corner at \a where.
			void				CopyTo(BView* view, BPoint where);

			const BBitmap*		Bitmap() const { return fBitmap; }

private:
			uint8*				_Pixel(int32 x, int32 y) const;
			bool				_TextBuffer(int32 width, int32 height);

			BBitmap*			fBitmap;
			int32				fWidth;
			int32				fHeight;
			BBitmap*			fText;
			BView*				fTextView;
};


// a colour with alpha \a alpha
rgb_color	with_alpha(rgb_color color, uint8 alpha);
rgb_color	make_rgba(uint8 red, uint8 green, uint8 blue, uint8 alpha = 255);
// whether windows can be seen through: is_composited_desktop(), where the
// system has it
bool		desktop_composited();
// B_TRANSPARENT_WINDOW, which older systems don't have
static const uint32 kTransparentWindow = 0x01000000;


#endif	// POWER_CANVAS_H
