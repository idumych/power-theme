/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Dark versions of Haiku's vector icons. See DarkIcons.h.
 *
 * An icon is the magic "ncif", then lists of styles, paths and shapes, each
 * with a one byte count (src/libs/icon/flat_icon in Haiku's source).
 */


#include "DarkIcons.h"

#include <math.h>

#include <algorithm>


namespace {


enum {
	STYLE_SOLID = 1,
	STYLE_GRADIENT = 2,
	STYLE_SOLID_NO_ALPHA = 3,
	STYLE_GRAY = 4,
	STYLE_GRAY_NO_ALPHA = 5,

	GRADIENT_TRANSFORM = 1 << 1,
	GRADIENT_NO_ALPHA = 1 << 2,
	GRADIENT_GRAYS = 1 << 4
};

struct Color {
	uint8_t	red;
	uint8_t	green;
	uint8_t	blue;
	uint8_t	alpha;
};

typedef uint32_t Ramp[5];

// dark to highlight, for lightness 0, 1/4, 1/2, 3/4 and 1
const Ramp kNeutral	= { 0x000000, 0x242428, 0x66686c, 0xaeb0b4, 0xe6e8ea };
const Ramp kRed		= { 0x300000, 0x7a0606, 0xc41414, 0xee4030, 0xff8470 };
const Ramp kOrange	= { 0x2a0e00, 0x6e2e00, 0xb05208, 0xe07c18, 0xf8ac4c };
const Ramp kYellow	= { 0x382800, 0x7c5c00, 0xbc8e00, 0xe8b818, 0xfbdc62 };
const Ramp kGreen	= { 0x002a0c, 0x045c20, 0x0c9438, 0x30c058, 0x80e490 };
const Ramp kCyan	= { 0x002c34, 0x006070, 0x0098ac, 0x20c4d4, 0x7ce8ee };
const Ramp kBlue	= { 0x00183e, 0x043e80, 0x0a6cc4, 0x2c98ea, 0x7cc4f6 };
const Ramp kPurple	= { 0x1c0640, 0x401088, 0x6c28c8, 0x9450ea, 0xc090f8 };
const Ramp kPink	= { 0x340420, 0x7a0c50, 0xc0208c, 0xe850b4, 0xf898d4 };

// how much further apart shadows and highlights are than a straight ramp's
const double kContrast = 0.6;


bool
is_shadow(const Color& color)
{
	return color.alpha < 200
		&& std::max(std::max(color.red, color.green), color.blue) < 40;
}


//! The place of \a lightness on \a ramp, in \a out[3].
void
on_ramp(const Ramp& ramp, double lightness, double out[3])
{
	const double curve = lightness * lightness * (3 - 2 * lightness);
	lightness += (curve - lightness) * kContrast;

	int from = 3;
	for (int i = 0; i < 4; i++) {
		if (lightness <= (i + 1) / 4.0) {
			from = i;
			break;
		}
	}
	const double t = lightness <= 1
		? (lightness - from / 4.0) / 0.25 : 1;
	for (int i = 0; i < 3; i++) {
		const int shift = 16 - i * 8;
		const int a = (ramp[from] >> shift) & 0xff;
		const int b = (ramp[from + 1] >> shift) & 0xff;
		out[i] = (double)lrint(a + (b - a) * t);
	}
}


Color
dark_color(const Color& color)
{
	if (is_shadow(color))
		return color;

	const double r = color.red / 255.0;
	const double g = color.green / 255.0;
	const double b = color.blue / 255.0;
	const double high = std::max(std::max(r, g), b);
	const double low = std::min(std::min(r, g), b);
	const double lightness = (high + low) / 2;
	double hue = 0;
	double saturation = 0;
	if (high != low) {
		const double range = high - low;
		saturation = lightness <= 0.5
			? range / (high + low) : range / (2 - high - low);
		const double rc = (high - r) / range;
		const double gc = (high - g) / range;
		const double bc = (high - b) / range;
		if (r == high)
			hue = bc - gc;
		else if (g == high)
			hue = 2 + rc - bc;
		else
			hue = 4 + gc - rc;
		hue = fmod(hue / 6, 1.0);
		if (hue < 0)
			hue += 1;
	}
	const double degrees = hue * 360;

	double out[3];
	if (saturation < 0.14 || lightness < 0.04 || lightness > 0.985)
		on_ramp(kNeutral, lightness, out);
	else {
		const Ramp* ramp;
		if (degrees < 15 || degrees >= 345)
			ramp = &kRed;
		else if (degrees < 42)
			ramp = &kOrange;
		else if (degrees < 70)
			ramp = &kYellow;
		else if (degrees < 165)
			ramp = &kGreen;
		else if (degrees < 200)
			ramp = &kCyan;
		else if (degrees < 255)
			ramp = &kBlue;
		else if (degrees < 300)
			ramp = &kPurple;
		else
			ramp = &kPink;
		on_ramp(*ramp, lightness, out);

		// greyish colours stay greyish
		const double weight = std::min(1.0, saturation / 0.30);
		double neutral[3];
		on_ramp(kNeutral, lightness, neutral);
		for (int i = 0; i < 3; i++)
			out[i] = (double)lrint(neutral[i] + (out[i] - neutral[i]) * weight);
	}

	const Color dark = { (uint8_t)out[0], (uint8_t)out[1], (uint8_t)out[2],
		color.alpha };
	return dark;
}


//! Reads what it's told to, and remembers if there wasn't that much.
class Reader {
public:
	Reader(const uint8_t* data, size_t size)
		:
		fData(data),
		fSize(size),
		fAt(0),
		fFailed(false)
	{
	}

	bool	Failed() const { return fFailed; }
	size_t	At() const { return fAt; }

	uint8_t Byte()
	{
		if (fAt >= fSize) {
			fFailed = true;
			return 0;
		}
		return fData[fAt++];
	}

	void Skip(size_t count)
	{
		if (count > fSize - fAt) {
			fFailed = true;
			fAt = fSize;
		} else
			fAt += count;
	}

private:
	const uint8_t*	fData;
	size_t			fSize;
	size_t			fAt;
	bool			fFailed;
};


Color
read_color(Reader& in, bool gray, bool alpha)
{
	Color color;
	color.red = in.Byte();
	color.green = gray ? color.red : in.Byte();
	color.blue = gray ? color.red : in.Byte();
	color.alpha = alpha ? in.Byte() : 255;
	return color;
}


void
add_color(std::vector<uint8_t>& out, const Color& color)
{
	out.push_back(color.red);
	out.push_back(color.green);
	out.push_back(color.blue);
	out.push_back(color.alpha);
}


/*!	Copies the styles to \a out in their dark colours, each with all its colour
	values.
*/
bool
copy_styles(Reader& in, std::vector<uint8_t>& out)
{
	const int count = in.Byte();
	out.push_back((uint8_t)count);
	for (int i = 0; i < count && !in.Failed(); i++) {
		const uint8_t type = in.Byte();
		switch (type) {
			case STYLE_SOLID:
			case STYLE_SOLID_NO_ALPHA:
			case STYLE_GRAY:
			case STYLE_GRAY_NO_ALPHA:
			{
				const Color color = read_color(in,
					type == STYLE_GRAY || type == STYLE_GRAY_NO_ALPHA,
					type == STYLE_SOLID || type == STYLE_GRAY);
				out.push_back(STYLE_SOLID);
				add_color(out, dark_color(color));
				break;
			}
			case STYLE_GRADIENT:
			{
				const uint8_t kind = in.Byte();
				const uint8_t flags = in.Byte();
				const uint8_t stops = in.Byte();
				out.push_back(STYLE_GRADIENT);
				out.push_back(kind);
				out.push_back(flags & ~(GRADIENT_NO_ALPHA | GRADIENT_GRAYS));
				out.push_back(stops);
				if ((flags & GRADIENT_TRANSFORM) != 0) {
					for (int b = 0; b < 18; b++)
						out.push_back(in.Byte());
				}
				for (int s = 0; s < stops; s++) {
					out.push_back(in.Byte());
					const Color color = read_color(in,
						(flags & GRADIENT_GRAYS) != 0,
						(flags & GRADIENT_NO_ALPHA) == 0);
					add_color(out, dark_color(color));
				}
				break;
			}
			default:
				return false;
		}
	}
	return !in.Failed();
}


}	// namespace


bool
make_dark_icon(const uint8_t* data, size_t size, std::vector<uint8_t>& dark)
{
	dark.clear();
	if (size < 7 || data[0] != 'n' || data[1] != 'c' || data[2] != 'i'
		|| data[3] != 'f') {
		return false;
	}

	Reader in(data, size);
	in.Skip(4);

	std::vector<uint8_t> styles;
	if (!copy_styles(in, styles))
		return false;

	// the paths and shapes stay as they are
	dark.assign(data, data + 4);
	dark.insert(dark.end(), styles.begin(), styles.end());
	dark.insert(dark.end(), data + in.At(), data + size);
	return true;
}
