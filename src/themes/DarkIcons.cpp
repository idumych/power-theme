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
#include <set>


namespace {


enum {
	STYLE_SOLID = 1,
	STYLE_GRADIENT = 2,
	STYLE_SOLID_NO_ALPHA = 3,
	STYLE_GRAY = 4,
	STYLE_GRAY_NO_ALPHA = 5,

	GRADIENT_TRANSFORM = 1 << 1,
	GRADIENT_NO_ALPHA = 1 << 2,
	GRADIENT_GRAYS = 1 << 4,

	PATH_COMMANDS = 1 << 2,
	PATH_NO_CURVES = 1 << 3,

	SHAPE_PATH_SOURCE = 10,
	SHAPE_TRANSFORM = 1 << 1,
	SHAPE_LOD_SCALE = 1 << 3,
	SHAPE_TRANSFORMERS = 1 << 4,
	SHAPE_TRANSLATION = 1 << 5,

	TRANSFORMER_AFFINE = 20,
	TRANSFORMER_CONTOUR = 21,
	TRANSFORMER_PERSPECTIVE = 22,
	TRANSFORMER_STROKE = 23
};

// the outline: a stroke of width 2 on the 64 unit grid, half of it showing,
// with round caps and joins
const uint8_t kOutlineStroke[] = { TRANSFORMER_STROKE, 128 + 2, 0x11, 4 };

const int kMaxItems = 255;


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

	void SkipCoord()
	{
		if ((Byte() & 128) != 0)
			Skip(1);
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


struct Style {
	// whether the outline should go around what uses it
	bool	solid;
	// nothing but soft shadows
	bool	shadow;
};


/*!	Copies the styles to \a out in their dark colours, each with all its colour
	values, and returns what is needed to know about them.
*/
bool
copy_styles(Reader& in, std::vector<uint8_t>& out, std::vector<Style>& styles)
{
	const int count = in.Byte();
	out.push_back((uint8_t)count);
	for (int i = 0; i < count && !in.Failed(); i++) {
		std::vector<Color> colors;
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
				colors.push_back(color);
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
					colors.push_back(color);
					add_color(out, dark_color(color));
				}
				break;
			}
			default:
				return false;
		}

		Style style = { false, !colors.empty() };
		int maxAlpha = 0;
		for (size_t c = 0; c < colors.size(); c++) {
			maxAlpha = std::max(maxAlpha, (int)colors[c].alpha);
			style.shadow = style.shadow && is_shadow(dark_color(colors[c]));
		}
		style.solid = maxAlpha >= 200 && !style.shadow;
		styles.push_back(style);
	}
	return !in.Failed();
}


void
skip_paths(Reader& in)
{
	const int count = in.Byte();
	for (int i = 0; i < count && !in.Failed(); i++) {
		const uint8_t flags = in.Byte();
		const int points = in.Byte();
		if ((flags & PATH_NO_CURVES) != 0) {
			for (int p = 0; p < points * 2 && !in.Failed(); p++)
				in.SkipCoord();
		} else if ((flags & PATH_COMMANDS) != 0) {
			std::vector<uint8_t> commands((points + 3) / 4);
			for (size_t c = 0; c < commands.size(); c++)
				commands[c] = in.Byte();
			for (int p = 0; p < points && !in.Failed(); p++) {
				static const int kCoords[] = { 1, 1, 2, 6 };
				const int command = (commands[p / 4] >> ((p % 4) * 2)) & 3;
				for (int c = 0; c < kCoords[command]; c++)
					in.SkipCoord();
			}
		} else {
			for (int p = 0; p < points * 6 && !in.Failed(); p++)
				in.SkipCoord();
		}
	}
}


struct Shape {
	size_t	start;
	size_t	end;
	int		style;
	size_t	flagsAt;
	uint8_t	flags;
	// where the number of transformers is, or 0 without any
	size_t	transformersAt;
};


bool
read_shapes(Reader& in, std::vector<Shape>& shapes)
{
	const int count = in.Byte();
	for (int i = 0; i < count && !in.Failed(); i++) {
		Shape shape;
		shape.start = in.At();
		if (in.Byte() != SHAPE_PATH_SOURCE)
			return false;
		shape.style = in.Byte();
		in.Skip(in.Byte());
		shape.flagsAt = in.At();
		shape.flags = in.Byte();
		shape.transformersAt = 0;
		if ((shape.flags & SHAPE_TRANSFORM) != 0)
			in.Skip(18);
		else if ((shape.flags & SHAPE_TRANSLATION) != 0) {
			in.SkipCoord();
			in.SkipCoord();
		}
		if ((shape.flags & SHAPE_LOD_SCALE) != 0)
			in.Skip(2);
		if ((shape.flags & SHAPE_TRANSFORMERS) != 0) {
			shape.transformersAt = in.At();
			const int transformers = in.Byte();
			for (int t = 0; t < transformers && !in.Failed(); t++) {
				switch (in.Byte()) {
					case TRANSFORMER_AFFINE:
						in.Skip(24);
						break;
					case TRANSFORMER_CONTOUR:
					case TRANSFORMER_STROKE:
						in.Skip(3);
						break;
					case TRANSFORMER_PERSPECTIVE:
						in.Skip(27);
						break;
					default:
						return false;
				}
			}
		}
		shape.end = in.At();
		shapes.push_back(shape);
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

	std::vector<uint8_t> styleBytes;
	std::vector<Style> styles;
	if (!copy_styles(in, styleBytes, styles))
		return false;

	const size_t pathsAt = in.At();
	skip_paths(in);
	const size_t shapesAt = in.At();

	std::vector<Shape> shapes;
	if (in.Failed() || !read_shapes(in, shapes)
		|| styles.size() >= (size_t)kMaxItems) {
		// the colours alone, if a later version has more than can be read here
		if (in.Failed())
			return false;
		dark.assign(data, data + 4);
		dark.insert(dark.end(), styleBytes.begin(), styleBytes.end());
		dark.insert(dark.end(), data + pathsAt, data + size);
		return true;
	}

	// a copy of each solid shape, stroked in the outline's style
	std::vector<std::vector<uint8_t> > outlines;
	std::set<std::vector<uint8_t> > seen;
	for (size_t i = 0; i < shapes.size(); i++) {
		const Shape& shape = shapes[i];
		if (shape.style >= (int)styles.size() || !styles[shape.style].solid)
			continue;

		std::vector<uint8_t> copy(data + shape.start, data + shape.end);
		copy[1] = (uint8_t)styles.size();
		if (shape.transformersAt != 0)
			copy[shape.transformersAt - shape.start]++;
		else {
			copy[shape.flagsAt - shape.start]
				= shape.flags | SHAPE_TRANSFORMERS;
			copy.push_back(1);
		}
		copy.insert(copy.end(), kOutlineStroke,
			kOutlineStroke + sizeof(kOutlineStroke));
		if (seen.insert(copy).second)
			outlines.push_back(copy);
	}

	// There's only so much room; the topmost shapes matter most.
	const size_t room = kMaxItems - shapes.size();
	if (outlines.size() > room)
		outlines.erase(outlines.begin(), outlines.end() - room);

	dark.assign(data, data + 4);
	dark.push_back((uint8_t)(styles.size() + 1));
	dark.insert(dark.end(), styleBytes.begin() + 1, styleBytes.end());
	const Color white = { 255, 255, 255, 255 };
	dark.push_back(STYLE_SOLID);
	add_color(dark, white);
	dark.insert(dark.end(), data + pathsAt, data + shapesAt);

	// behind everything but the shadows
	size_t lead = 0;
	while (lead < shapes.size() && shapes[lead].style < (int)styles.size()
		&& styles[shapes[lead].style].shadow) {
		lead++;
	}
	dark.push_back((uint8_t)(shapes.size() + outlines.size()));
	for (size_t i = 0; i < lead; i++) {
		dark.insert(dark.end(), data + shapes[i].start,
			data + shapes[i].end);
	}
	for (size_t i = 0; i < outlines.size(); i++)
		dark.insert(dark.end(), outlines[i].begin(), outlines[i].end());
	for (size_t i = lead; i < shapes.size(); i++) {
		dark.insert(dark.end(), data + shapes[i].start,
			data + shapes[i].end);
	}
	dark.insert(dark.end(), data + in.At(), data + size);
	return true;
}
