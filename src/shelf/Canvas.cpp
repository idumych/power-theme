/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 */


#include "Canvas.h"

#include <View.h>
#include <image.h>

#include <algorithm>
#include <math.h>
#include <string.h>


rgb_color
with_alpha(rgb_color color, uint8 alpha)
{
	color.alpha = alpha;
	return color;
}


rgb_color
make_rgba(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	rgb_color color = { red, green, blue, alpha };
	return color;
}


bool
desktop_composited()
{
	static int composited = -1;
	if (composited >= 0)
		return composited != 0;

	// looked up, so that this runs on systems without it too
	composited = 0;
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		const char* leaf = strrchr(info.name, '/');
		if (strcmp(leaf != NULL ? leaf + 1 : info.name, "libbe.so") != 0)
			continue;
		bool (*isComposited)();
		if (get_image_symbol(info.id, "_Z21is_composited_desktopv",
				B_SYMBOL_TYPE_TEXT, (void**)&isComposited) == B_OK)
			composited = isComposited() ? 1 : 0;
		break;
	}
	return composited != 0;
}


static inline uint8
clamp_byte(float value)
{
	return (uint8)std::min(255.0f, std::max(0.0f, value + 0.5f));
}


static inline rgb_color
mix(rgb_color a, rgb_color b, float t)
{
	rgb_color color = { clamp_byte(a.red + (b.red - a.red) * t),
		clamp_byte(a.green + (b.green - a.green) * t),
		clamp_byte(a.blue + (b.blue - a.blue) * t),
		clamp_byte(a.alpha + (b.alpha - a.alpha) * t) };
	return color;
}


Canvas::Canvas()
	:
	fBitmap(NULL),
	fWidth(0),
	fHeight(0),
	fText(NULL),
	fTextView(NULL)
{
}


Canvas::~Canvas()
{
	delete fBitmap;
	delete fText;
}


bool
Canvas::SetSize(int32 width, int32 height)
{
	width = std::max((int32)1, width);
	height = std::max((int32)1, height);
	if (fBitmap == NULL || width != fWidth || height != fHeight) {
		delete fBitmap;
		fBitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
		if (fBitmap->InitCheck() != B_OK) {
			delete fBitmap;
			fBitmap = NULL;
			fWidth = fHeight = 0;
			return false;
		}
		fWidth = width;
		fHeight = height;
	}
	Clear();
	return true;
}


void
Canvas::Clear()
{
	if (fBitmap != NULL)
		memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
}


inline uint8*
Canvas::_Pixel(int32 x, int32 y) const
{
	return (uint8*)fBitmap->Bits() + y * fBitmap->BytesPerRow() + x * 4;
}


void
Canvas::Blend(int32 x, int32 y, rgb_color color, uint8 cover)
{
	if (fBitmap == NULL || x < 0 || y < 0 || x >= fWidth || y >= fHeight)
		return;
	const uint32 alpha = color.alpha * cover / 255;
	if (alpha == 0)
		return;
	uint8* pixel = _Pixel(x, y);
	const uint32 below = pixel[3];
	if (alpha == 255 || below == 0) {
		pixel[0] = color.blue;
		pixel[1] = color.green;
		pixel[2] = color.red;
		pixel[3] = alpha;
		return;
	}
	// over, with colours that aren't premultiplied
	const uint32 rest = below * (255 - alpha) / 255;
	const uint32 total = alpha + rest;
	pixel[0] = (color.blue * alpha + pixel[0] * rest) / total;
	pixel[1] = (color.green * alpha + pixel[1] * rest) / total;
	pixel[2] = (color.red * alpha + pixel[2] * rest) / total;
	pixel[3] = total;
}


void
Canvas::FillRect(BRect rect, rgb_color color)
{
	const int32 left = std::max((int32)0, (int32)floorf(rect.left));
	const int32 top = std::max((int32)0, (int32)floorf(rect.top));
	const int32 right = std::min(fWidth - 1, (int32)floorf(rect.right));
	const int32 bottom = std::min(fHeight - 1, (int32)floorf(rect.bottom));
	for (int32 y = top; y <= bottom; y++) {
		for (int32 x = left; x <= right; x++)
			Blend(x, y, color);
	}
}


void
Canvas::FillGradient(BRect rect, rgb_color top, rgb_color bottom)
{
	const int32 first = (int32)floorf(rect.top);
	const int32 last = (int32)floorf(rect.bottom);
	for (int32 y = first; y <= last; y++) {
		const float t = last > first ? (float)(y - first) / (last - first) : 0;
		FillRect(BRect(rect.left, y, rect.right, y), mix(top, bottom, t));
	}
}


void
Canvas::FillRoundRect(BRect rect, float radius, rgb_color color)
{
	radius = std::min(radius, std::min(rect.Width(), rect.Height()) / 2);
	const int32 left = std::max((int32)0, (int32)floorf(rect.left));
	const int32 top = std::max((int32)0, (int32)floorf(rect.top));
	const int32 right = std::min(fWidth - 1, (int32)ceilf(rect.right));
	const int32 bottom = std::min(fHeight - 1, (int32)ceilf(rect.bottom));
	// the corners' centres, with the edges at the pixels' outer sides
	const float innerLeft = rect.left + radius;
	const float innerRight = rect.right + 1 - radius;
	const float innerTop = rect.top + radius;
	const float innerBottom = rect.bottom + 1 - radius;
	for (int32 y = top; y <= bottom; y++) {
		const float cy = y + 0.5f;
		for (int32 x = left; x <= right; x++) {
			const float cx = x + 0.5f;
			const float dx = cx < innerLeft ? innerLeft - cx
				: cx > innerRight ? cx - innerRight : 0;
			const float dy = cy < innerTop ? innerTop - cy
				: cy > innerBottom ? cy - innerBottom : 0;
			float cover = 1;
			if (dx > 0 && dy > 0)
				cover = radius + 0.5f - sqrtf(dx * dx + dy * dy);
			else if (dx > 0)
				cover = radius + 0.5f - dx;
			else if (dy > 0)
				cover = radius + 0.5f - dy;
			if (cover > 0)
				Blend(x, y, color, clamp_byte(std::min(1.0f, cover) * 255));
		}
	}
}


void
Canvas::FillCircle(BPoint center, float radius, rgb_color inner,
	rgb_color outer)
{
	const int32 left = (int32)floorf(center.x - radius);
	const int32 right = (int32)ceilf(center.x + radius);
	const int32 top = (int32)floorf(center.y - radius);
	const int32 bottom = (int32)ceilf(center.y + radius);
	for (int32 y = top; y <= bottom; y++) {
		for (int32 x = left; x <= right; x++) {
			const float dx = x + 0.5f - center.x;
			const float dy = y + 0.5f - center.y;
			const float distance = sqrtf(dx * dx + dy * dy);
			const float cover = std::min(1.0f, radius + 0.5f - distance);
			if (cover <= 0)
				continue;
			Blend(x, y, mix(inner, outer, std::min(1.0f, distance / radius)),
				clamp_byte(cover * 255));
		}
	}
}


void
Canvas::FillTrapezoid(float top, float bottom, float backLeft,
	float backRight, float frontLeft, float frontRight, rgb_color topColor,
	rgb_color bottomColor)
{
	const int32 first = std::max((int32)0, (int32)floorf(top));
	const int32 last = std::min(fHeight - 1, (int32)ceilf(bottom) - 1);
	for (int32 y = first; y <= last; y++) {
		const float t = bottom > top
			? std::min(1.0f, std::max(0.0f, (y + 0.5f - top) / (bottom - top)))
			: 0;
		const float left = backLeft + (frontLeft - backLeft) * t;
		const float right = backRight + (frontRight - backRight) * t;
		const rgb_color color = mix(topColor, bottomColor, t);
		// the rows' top and bottom pixels may be partly covered
		float rowCover = 1;
		if (y < top)
			rowCover = y + 1 - top;
		else if (y + 1 > bottom)
			rowCover = bottom - y;
		const int32 from = std::max((int32)0, (int32)floorf(left));
		const int32 to = std::min(fWidth - 1, (int32)ceilf(right) - 1);
		for (int32 x = from; x <= to; x++) {
			float cover = rowCover;
			if (x < left)
				cover *= x + 1 - left;
			if (x + 1 > right)
				cover *= right - x;
			if (cover > 0)
				Blend(x, y, color, clamp_byte(std::min(1.0f, cover) * 255));
		}
	}
}


void
Canvas::DrawBitmap(const BBitmap* bitmap, BRect dest, uint8 alpha,
	bool mirror, float fade, float clipTop, float clipBottom)
{
	if (bitmap == NULL || fBitmap == NULL || dest.Width() <= 0
		|| bitmap->ColorSpace() != B_RGBA32)
		return;
	const BRect bounds = bitmap->Bounds();
	const int32 sourceWidth = bounds.IntegerWidth() + 1;
	const int32 sourceHeight = bounds.IntegerHeight() + 1;
	const uint8* bits = (const uint8*)bitmap->Bits();
	const int32 bpr = bitmap->BytesPerRow();
	// how many source pixels one pixel here covers
	const float scaleX = sourceWidth / (dest.Width() + 1);
	const float scaleY = sourceHeight / (dest.Height() + 1);

	const int32 left = (int32)floorf(dest.left);
	const int32 top = (int32)floorf(dest.top);
	const int32 width = (int32)roundf(dest.Width() + 1);
	const int32 height = (int32)roundf(dest.Height() + 1);
	for (int32 row = 0; row < height; row++) {
		const int32 y = top + row;
		if (y < 0 || y >= fHeight || y < clipTop || y > clipBottom)
			continue;
		uint32 rowAlpha = alpha;
		if (mirror) {
			if (fade <= 0 || row >= fade)
				break;
			rowAlpha = (uint32)(alpha * (1 - row / fade));
		}
		const int32 sourceRow = mirror ? height - 1 - row : row;
		// the source rows this one covers, averaged
		const float fromY = sourceRow * scaleY;
		const float toY = std::max(fromY + 1, (sourceRow + 1) * scaleY);
		const int32 y0 = std::min(sourceHeight - 1, (int32)fromY);
		const int32 y1 = std::min(sourceHeight, (int32)ceilf(toY));
		for (int32 column = 0; column < width; column++) {
			const int32 x = left + column;
			if (x < 0 || x >= fWidth)
				continue;
			// premultiplied, so that clear pixels don't darken the edges
			float b = 0, g = 0, r = 0, a = 0, weight = 0;
			if (scaleX < 1) {
				// bigger than drawn: between the four nearest pixels
				const float sx = std::max(0.0f, (column + 0.5f) * scaleX - 0.5f);
				const float sy = std::max(0.0f,
					(sourceRow + 0.5f) * scaleY - 0.5f);
				const int32 ix = std::min(sourceWidth - 1, (int32)sx);
				const int32 iy = std::min(sourceHeight - 1, (int32)sy);
				const float fx = sx - ix;
				const float fy = sy - iy;
				for (int32 k = 0; k < 4; k++) {
					const int32 px = std::min(sourceWidth - 1, ix + (k & 1));
					const int32 py = std::min(sourceHeight - 1, iy + (k >> 1));
					const float w = ((k & 1) ? fx : 1 - fx)
						* ((k >> 1) ? fy : 1 - fy);
					const uint8* p = bits + py * bpr + px * 4;
					b += p[0] * p[3] * w;
					g += p[1] * p[3] * w;
					r += p[2] * p[3] * w;
					a += p[3] * w;
					weight += w;
				}
			} else {
				// smaller: the average of the pixels it covers
				const float fromX = column * scaleX;
				const float toX = std::max(fromX + 1, (column + 1) * scaleX);
				const int32 x0 = std::min(sourceWidth - 1, (int32)fromX);
				const int32 x1 = std::min(sourceWidth, (int32)ceilf(toX));
				for (int32 sy = y0; sy < y1; sy++) {
					const uint8* p = bits + sy * bpr + x0 * 4;
					for (int32 sx = x0; sx < x1; sx++, p += 4) {
						b += p[0] * p[3];
						g += p[1] * p[3];
						r += p[2] * p[3];
						a += p[3];
						weight++;
					}
				}
			}
			if (a <= 0 || weight <= 0)
				continue;
			rgb_color color = { (uint8)std::min(255.0f, r / a),
				(uint8)std::min(255.0f, g / a), (uint8)std::min(255.0f, b / a),
				(uint8)std::min(255.0f, a / weight + 0.5f) };
			Blend(x, y, color, rowAlpha);
		}
	}
}


void
Canvas::DrawSmooth(const BBitmap* bitmap, float left, float top, float size,
	uint8 alpha, bool mirror, float fade, float clipTop, float clipBottom)
{
	if (bitmap == NULL || fBitmap == NULL || size <= 0
		|| bitmap->ColorSpace() != B_RGBA32)
		return;
	const int32 sourceSize = bitmap->Bounds().IntegerWidth() + 1;
	const uint8* bits = (const uint8*)bitmap->Bits();
	const int32 bpr = bitmap->BytesPerRow();
	const float scale = sourceSize / size;
	const float right = left + size;
	const float bottom = top + size;

	const int32 firstY = std::max((int32)floorf(top),
		std::max((int32)0, (int32)ceilf(clipTop)));
	const int32 lastY = std::min((int32)ceilf(bottom) - 1,
		std::min(fHeight - 1, (int32)floorf(clipBottom)));
	const int32 firstX = std::max((int32)0, (int32)floorf(left));
	const int32 lastX = std::min(fWidth - 1, (int32)ceilf(right) - 1);
	for (int32 y = firstY; y <= lastY; y++) {
		// how far down it this row is, and how much of the row it covers
		const float down = y + 0.5f - top;
		float rowCover = std::min((float)y + 1, bottom) - std::max((float)y, top);
		if (mirror) {
			if (fade <= 0 || down >= fade)
				break;
			rowCover *= 1 - down / fade;
		}
		if (rowCover <= 0)
			continue;
		float v = (mirror ? size - down : down) * scale - 0.5f;
		v = std::min(std::max(v, 0.0f), sourceSize - 1.0f);
		const int32 v0 = (int32)v;
		const int32 v1 = std::min(sourceSize - 1, v0 + 1);
		const float fv = v - v0;
		const uint8* row0 = bits + v0 * bpr;
		const uint8* row1 = bits + v1 * bpr;
		for (int32 x = firstX; x <= lastX; x++) {
			const float cover = rowCover * (std::min((float)x + 1, right)
				- std::max((float)x, left));
			if (cover <= 0)
				continue;
			float u = (x + 0.5f - left) * scale - 0.5f;
			u = std::min(std::max(u, 0.0f), sourceSize - 1.0f);
			const int32 u0 = (int32)u;
			const int32 u1 = std::min(sourceSize - 1, u0 + 1);
			const float fu = u - u0;
			// premultiplied, so that clear pixels don't darken the edges
			const uint8* p[4] = { row0 + u0 * 4, row0 + u1 * 4, row1 + u0 * 4,
				row1 + u1 * 4 };
			const float w[4] = { (1 - fu) * (1 - fv), fu * (1 - fv),
				(1 - fu) * fv, fu * fv };
			float b = 0, g = 0, r = 0, a = 0;
			for (int32 k = 0; k < 4; k++) {
				const float weight = w[k] * p[k][3];
				b += p[k][0] * weight;
				g += p[k][1] * weight;
				r += p[k][2] * weight;
				a += weight;
			}
			if (a <= 0)
				continue;
			rgb_color color = { (uint8)std::min(255.0f, r / a + 0.5f),
				(uint8)std::min(255.0f, g / a + 0.5f),
				(uint8)std::min(255.0f, b / a + 0.5f),
				(uint8)std::min(255.0f, a + 0.5f) };
			Blend(x, y, color, (uint8)std::min(255.0f, alpha * cover + 0.5f));
		}
	}
}


/*static*/ BBitmap*
Canvas::Scaled(const BBitmap* bitmap, int32 size)
{
	if (bitmap == NULL)
		return NULL;
	Canvas canvas;
	if (!canvas.SetSize(size, size))
		return NULL;
	canvas.DrawBitmap(bitmap, BRect(0, 0, size - 1, size - 1));
	BBitmap* scaled = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGBA32);
	if (scaled->InitCheck() != B_OK) {
		delete scaled;
		return NULL;
	}
	for (int32 y = 0; y < size; y++) {
		memcpy((uint8*)scaled->Bits() + y * scaled->BytesPerRow(),
			(const uint8*)canvas.fBitmap->Bits()
				+ y * canvas.fBitmap->BytesPerRow(), size * 4);
	}
	return scaled;
}


bool
Canvas::_TextBuffer(int32 width, int32 height)
{
	if (fText != NULL && fText->Bounds().IntegerWidth() + 1 >= width
		&& fText->Bounds().IntegerHeight() + 1 >= height)
		return true;
	delete fText;
	fText = new BBitmap(BRect(0, 0, std::max((int32)256, width) - 1,
		std::max((int32)32, height) - 1), B_RGB32, true);
	if (fText->InitCheck() != B_OK) {
		delete fText;
		fText = NULL;
		return false;
	}
	fTextView = new BView(fText->Bounds(), "text", B_FOLLOW_NONE, 0);
	fText->AddChild(fTextView);
	return true;
}


void
Canvas::DrawString(const char* text, const BFont& font, BPoint where,
	rgb_color color)
{
	if (fBitmap == NULL || text == NULL || text[0] == '\0')
		return;
	BFont grey(font);
	// coverage only: no subpixel colours
	grey.SetFlags(B_FORCE_ANTIALIASING);
	font_height height;
	grey.GetHeight(&height);
	const int32 width = (int32)ceilf(grey.StringWidth(text)) + 4;
	const int32 ascent = (int32)ceilf(height.ascent) + 1;
	const int32 rows = ascent + (int32)ceilf(height.descent) + 2;
	if (!_TextBuffer(width, rows) || !fText->Lock())
		return;
	fTextView->SetHighColor(0, 0, 0);
	fTextView->FillRect(BRect(0, 0, width, rows));
	fTextView->SetFont(&grey);
	fTextView->SetHighColor(255, 255, 255);
	fTextView->SetLowColor(0, 0, 0);
	fTextView->SetDrawingMode(B_OP_OVER);
	fTextView->DrawString(text, BPoint(1, ascent));
	fTextView->Sync();
	fText->Unlock();

	// white on black is how much of each pixel the text covers
	const uint8* bits = (const uint8*)fText->Bits();
	const int32 bpr = fText->BytesPerRow();
	const int32 left = (int32)floorf(where.x) - 1;
	const int32 top = (int32)floorf(where.y) - ascent;
	for (int32 y = 0; y < rows; y++) {
		const uint8* p = bits + y * bpr;
		for (int32 x = 0; x < width; x++, p += 4) {
			if (p[1] != 0)
				Blend(left + x, top + y, color, p[1]);
		}
	}
}


void
Canvas::CopyTo(BView* view, BPoint where)
{
	if (fBitmap == NULL)
		return;
	view->PushState();
	view->SetDrawingMode(B_OP_COPY);
	view->DrawBitmap(fBitmap, where);
	view->PopState();
}
