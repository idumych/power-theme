/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Pixel icons to HVIF. See HVIFEncoder.h.
 *
 * The format, as read by Haiku's FlatIconImporter (src/libs/icon): the
 * magic 'ficn', then styles, paths and shapes, each list preceded by a one
 * byte count. A path here is a closed polygon without curves; a shape draws
 * its paths with one style, and is visible between two scales of the 64
 * unit canvas (its level of detail).
 *
 * The outline of each colour is traced along pixel edges, clockwise, so
 * holes run the other way and the non-zero fill rule leaves them empty.
 * To spend fewer paths, the outlines of one colour are joined into one
 * polygon through a "hub": the polygon runs round an outline, back to the
 * hub, out to the next outline and so on. Each connecting line is drawn
 * once each way and cancels out. An outline with more corners than a path
 * holds is cut in two along a chord, drawn once in each half.
 */


#include "HVIFEncoder.h"

#include <algorithm>
#include <cmath>
#include <map>


namespace {


const int kMaxItems = 255;
	// styles, paths and shapes in one icon
const int kMaxPathPoints = 255;
const int kMinColorLimit = 12;
	// below this, a design is left out rather than reduced further
const uint8_t kAlphaThreshold = 24;
	// pixels more transparent than this are left out
const uint8_t kOpaqueThreshold = 240;
	// and more opaque ones are drawn opaque


struct Point {
	int		x;
	int		y;

	bool operator==(const Point& other) const
		{ return x == other.x && y == other.y; }
};

typedef std::vector<Point> Polygon;


struct Design {
	int						size;
	std::vector<int>		pixels;
		// palette index per pixel, -1 for transparent
	std::vector<uint32_t>	palette;
		// packed red, green, blue, alpha
	bool					reduced;
};


struct ShapeData {
	int						design;
	int						style;
	std::vector<Polygon>	paths;
};


uint32_t
pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (uint32_t)r << 24 | (uint32_t)g << 16 | (uint32_t)b << 8 | a;
}


uint8_t
channel(uint32_t color, int which)
{
	return (color >> (24 - 8 * which)) & 0xff;
}


//	#pragma mark - colour reduction


struct Entry {
	uint32_t	color;
	int			count;
};


/*!	Median cut: the box of colours with the widest spread (weighted by how
	many pixels it covers) is split at the median of that channel, until
	there are \a limit boxes. Each box becomes its pixel weighted average.
*/
std::map<uint32_t, uint32_t>
reduce_colors(const std::map<uint32_t, int>& histogram, int limit)
{
	std::vector<std::vector<Entry> > boxes(1);
	for (std::map<uint32_t, int>::const_iterator it = histogram.begin();
			it != histogram.end(); ++it) {
		Entry entry = { it->first, it->second };
		boxes[0].push_back(entry);
	}

	while ((int)boxes.size() < limit) {
		int best = -1;
		int bestChannel = 0;
		double bestScore = 0;
		for (size_t i = 0; i < boxes.size(); i++) {
			if (boxes[i].size() < 2)
				continue;
			int pixels = 0;
			for (size_t e = 0; e < boxes[i].size(); e++)
				pixels += boxes[i][e].count;
			for (int c = 0; c < 4; c++) {
				int low = 255, high = 0;
				for (size_t e = 0; e < boxes[i].size(); e++) {
					int value = channel(boxes[i][e].color, c);
					low = std::min(low, value);
					high = std::max(high, value);
				}
				double score = (double)(high - low) * sqrt((double)pixels);
				if (score > bestScore) {
					bestScore = score;
					best = (int)i;
					bestChannel = c;
				}
			}
		}
		if (best < 0)
			break;

		std::vector<Entry>& box = boxes[best];
		const int c = bestChannel;
		std::sort(box.begin(), box.end(),
			[c](const Entry& a, const Entry& b) {
				return channel(a.color, c) < channel(b.color, c);
			});

		int total = 0;
		for (size_t e = 0; e < box.size(); e++)
			total += box[e].count;
		int running = 0;
		size_t split = 1;
		for (size_t e = 0; e + 1 < box.size(); e++) {
			running += box[e].count;
			split = e + 1;
			if (running * 2 >= total)
				break;
		}

		std::vector<Entry> upper(box.begin() + split, box.end());
		box.resize(split);
		boxes.push_back(upper);
	}

	std::map<uint32_t, uint32_t> mapping;
	for (size_t i = 0; i < boxes.size(); i++) {
		double sum[4] = { 0, 0, 0, 0 };
		double weight = 0;
		for (size_t e = 0; e < boxes[i].size(); e++) {
			for (int c = 0; c < 4; c++)
				sum[c] += channel(boxes[i][e].color, c) * boxes[i][e].count;
			weight += boxes[i][e].count;
		}
		uint8_t average[4];
		for (int c = 0; c < 4; c++)
			average[c] = (uint8_t)lround(sum[c] / weight);
		const uint32_t color = pack(average[0], average[1], average[2],
			average[3]);
		for (size_t e = 0; e < boxes[i].size(); e++)
			mapping[boxes[i][e].color] = color;
	}
	return mapping;
}


Design
make_design(const IconImage& image, int limit)
{
	Design design;
	design.size = image.size;
	design.reduced = false;

	const int count = image.size * image.size;
	std::vector<uint32_t> colors(count);
	std::map<uint32_t, int> histogram;
	for (int i = 0; i < count; i++) {
		const uint8_t* p = &image.rgba[i * 4];
		if (p[3] < kAlphaThreshold) {
			colors[i] = 0;
			continue;
		}
		uint8_t alpha = p[3] >= kOpaqueThreshold ? 255 : p[3];
		colors[i] = pack(p[0], p[1], p[2], alpha);
		histogram[colors[i]]++;
	}

	std::map<uint32_t, uint32_t> mapping;
	if ((int)histogram.size() > limit) {
		mapping = reduce_colors(histogram, limit);
		design.reduced = true;
	}

	std::map<uint32_t, int> indexes;
	design.pixels.assign(count, -1);
	for (int i = 0; i < count; i++) {
		if (image.rgba[i * 4 + 3] < kAlphaThreshold)
			continue;
		uint32_t color = colors[i];
		if (design.reduced)
			color = mapping[color];
		std::map<uint32_t, int>::iterator found = indexes.find(color);
		if (found == indexes.end()) {
			found = indexes.insert(std::make_pair(color,
				(int)design.palette.size())).first;
			design.palette.push_back(color);
		}
		design.pixels[i] = found->second;
	}
	return design;
}


//	#pragma mark - outlines


/*!	The outlines of the pixels of colour \a index, as corner points, each
	running clockwise with the pixels on its right.
*/
std::vector<Polygon>
trace(const Design& design, int index)
{
	const int size = design.size;
	const int stride = size + 1;
	static const int kDX[4] = { 1, 0, -1, 0 };
	static const int kDY[4] = { 0, 1, 0, -1 };
		// right, down, left, up

	struct Edge {
		int		from;
		int		direction;
		bool	used;
	};
	std::vector<Edge> edges;
	std::vector<std::vector<int> > outgoing(stride * stride);

	auto at = [&](int x, int y) {
		if (x < 0 || y < 0 || x >= size || y >= size)
			return -1;
		return design.pixels[y * size + x];
	};
	auto add = [&](int x, int y, int direction) {
		Edge edge = { y * stride + x, direction, false };
		outgoing[edge.from].push_back((int)edges.size());
		edges.push_back(edge);
	};

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			if (at(x, y) != index)
				continue;
			if (at(x, y - 1) != index)
				add(x, y, 0);
			if (at(x + 1, y) != index)
				add(x + 1, y, 1);
			if (at(x, y + 1) != index)
				add(x + 1, y + 1, 2);
			if (at(x - 1, y) != index)
				add(x, y + 1, 3);
		}
	}

	std::vector<Polygon> outlines;
	for (size_t start = 0; start < edges.size(); start++) {
		if (edges[start].used)
			continue;

		Polygon polygon;
		const int origin = edges[start].from;
		const int firstDirection = edges[start].direction;
		int current = (int)start;
		int direction = -1;
		while (true) {
			Edge& edge = edges[current];
			edge.used = true;
			if (edge.direction != direction) {
				Point corner = { edge.from % stride, edge.from / stride };
				polygon.push_back(corner);
				direction = edge.direction;
			}
			const int x = edge.from % stride + kDX[edge.direction];
			const int y = edge.from / stride + kDY[edge.direction];
			const int to = y * stride + x;
			if (to == origin)
				break;

			// Where two outlines touch at a corner, turn right, so each
			// keeps to its own pixels.
			int next = -1;
			static const int kTurns[3] = { 1, 0, 3 };
			for (int t = 0; t < 3 && next < 0; t++) {
				const int wanted = (edge.direction + kTurns[t]) % 4;
				for (size_t o = 0; o < outgoing[to].size(); o++) {
					const Edge& candidate = edges[outgoing[to][o]];
					if (!candidate.used && candidate.direction == wanted) {
						next = outgoing[to][o];
						break;
					}
				}
			}
			if (next < 0)
				break;
				// can't happen: every corner has as many edges in as out
			current = next;
		}

		// the starting corner is no corner if the outline arrives
		// straight
		if (direction == firstDirection && polygon.size() > 1)
			polygon.erase(polygon.begin());
		if (polygon.size() >= 3)
			outlines.push_back(polygon);
	}
	return outlines;
}


/*!	Cuts \a polygon into pieces of at most \a limit points along chords;
	each chord is drawn once in each piece, so they cancel.
*/
void
split_polygon(const Polygon& polygon, size_t limit, std::vector<Polygon>& out)
{
	if (polygon.size() <= limit) {
		out.push_back(polygon);
		return;
	}

	const size_t cut = limit - 1;
	Polygon first(polygon.begin(), polygon.begin() + cut + 1);
	Polygon rest(polygon.begin() + cut, polygon.end());
	rest.push_back(polygon[0]);
	out.push_back(first);
	split_polygon(rest, limit, out);
}


/*!	Joins the outlines of one colour into as few paths as fit. */
std::vector<Polygon>
join_outlines(const std::vector<Polygon>& outlines)
{
	std::vector<Polygon> pieces;
	for (size_t i = 0; i < outlines.size(); i++)
		split_polygon(outlines[i], kMaxPathPoints - 2, pieces);

	std::vector<Polygon> paths;
	Polygon current;
	for (size_t i = 0; i < pieces.size(); i++) {
		const Polygon& piece = pieces[i];
		if (current.empty()) {
			current = piece;
			continue;
		}
		if (current.size() + piece.size() + 2 > (size_t)kMaxPathPoints) {
			paths.push_back(current);
			current = piece;
			continue;
		}
		// back to the hub (closing what came before), out to the piece,
		// round it, and to its start again; the path's closing line then
		// returns to the hub
		current.push_back(current[0]);
		current.insert(current.end(), piece.begin(), piece.end());
		current.push_back(piece[0]);
	}
	if (!current.empty())
		paths.push_back(current);
	return paths;
}


//	#pragma mark - writing


void
write_coord(std::vector<uint8_t>& out, double coord)
{
	coord = std::max(-128.0, std::min(192.0, coord));
	const double rounded = floor(coord + 0.5);
	if (fabs(coord - rounded) < 1e-9 && rounded >= -32 && rounded <= 95) {
		out.push_back((uint8_t)(rounded + 32));
		return;
	}
	uint16_t value = (uint16_t)lround((coord + 128.0) * 102.0);
	value |= 0x8000;
	out.push_back(value >> 8);
	out.push_back(value & 0xff);
}


uint8_t
lod_byte(double size)
{
	// the renderer's scale is size / 64; the format stores scale * 63.75
	return (uint8_t)std::max(0L, std::min(255L,
		lround(size / 64.0 * 63.75)));
}


/*!	How many colours each design uses, counted as make_design() does. */
int
count_colors(const IconImage& image)
{
	std::map<uint32_t, int> histogram;
	for (size_t i = 0; i < image.rgba.size(); i += 4) {
		const uint8_t* p = &image.rgba[i];
		if (p[3] >= kAlphaThreshold)
			histogram[pack(p[0], p[1], p[2], p[3] >= kOpaqueThreshold ? 255 : p[3])]++;
	}
	return (int)histogram.size();
}


/*!	Shares \a budget colours between the designs, in proportion to the
	colours each uses, but at least kMinColorLimit each.
*/
std::vector<int>
color_limits(const std::vector<IconImage>& images, double budget)
{
	std::vector<int> counts;
	double total = 0;
	for (size_t i = 0; i < images.size(); i++) {
		counts.push_back(std::max(1, count_colors(images[i])));
		total += counts.back();
	}
	std::vector<int> limits;
	for (size_t i = 0; i < images.size(); i++) {
		limits.push_back(std::min(counts[i], std::max(kMinColorLimit,
			(int)floor(budget * counts[i] / total))));
	}
	return limits;
}


/*!	Traces every design with at most \a limits colours each. Returns false
	if the result doesn't fit the format.
*/
bool
build(const std::vector<IconImage>& images, const std::vector<int>& limits,
	std::vector<Design>& designs, std::vector<uint32_t>& styles,
	std::vector<ShapeData>& shapes, int& pathCount)
{
	designs.clear();
	styles.clear();
	shapes.clear();
	pathCount = 0;

	std::map<uint32_t, int> styleIndexes;
	for (size_t d = 0; d < images.size(); d++) {
		designs.push_back(make_design(images[d], limits[d]));
		const Design& design = designs.back();
		for (size_t p = 0; p < design.palette.size(); p++) {
			ShapeData shape;
			shape.design = (int)d;
			std::map<uint32_t, int>::iterator found
				= styleIndexes.find(design.palette[p]);
			if (found == styleIndexes.end()) {
				found = styleIndexes.insert(std::make_pair(design.palette[p],
					(int)styles.size())).first;
				styles.push_back(design.palette[p]);
			}
			shape.style = found->second;
			shape.paths = join_outlines(trace(design, (int)p));
			if (shape.paths.empty())
				continue;
			pathCount += (int)shape.paths.size();
			shapes.push_back(shape);
		}
	}

	return (int)styles.size() <= kMaxItems && (int)shapes.size() <= kMaxItems
		&& pathCount <= kMaxItems;
}


void
write_icon(const std::vector<Design>& designs,
	const std::vector<uint32_t>& styles, const std::vector<ShapeData>& shapes,
	std::vector<uint8_t>& out)
{
	out.clear();
	// 'ficn', little endian
	out.push_back('n');
	out.push_back('c');
	out.push_back('i');
	out.push_back('f');

	out.push_back((uint8_t)styles.size());
	for (size_t i = 0; i < styles.size(); i++) {
		const uint8_t r = channel(styles[i], 0);
		const uint8_t g = channel(styles[i], 1);
		const uint8_t b = channel(styles[i], 2);
		const uint8_t a = channel(styles[i], 3);
		const bool gray = r == g && g == b;
		if (a == 255) {
			if (gray) {
				out.push_back(5);	// STYLE_TYPE_SOLID_GRAY_NO_ALPHA
				out.push_back(r);
			} else {
				out.push_back(3);	// STYLE_TYPE_SOLID_COLOR_NO_ALPHA
				out.push_back(r);
				out.push_back(g);
				out.push_back(b);
			}
		} else if (gray) {
			out.push_back(4);	// STYLE_TYPE_SOLID_GRAY
			out.push_back(r);
			out.push_back(a);
		} else {
			out.push_back(1);	// STYLE_TYPE_SOLID_COLOR
			out.push_back(r);
			out.push_back(g);
			out.push_back(b);
			out.push_back(a);
		}
	}

	int pathCount = 0;
	for (size_t s = 0; s < shapes.size(); s++)
		pathCount += (int)shapes[s].paths.size();
	out.push_back((uint8_t)pathCount);
	for (size_t s = 0; s < shapes.size(); s++) {
		const double unit = 64.0 / designs[shapes[s].design].size;
		for (size_t p = 0; p < shapes[s].paths.size(); p++) {
			const Polygon& path = shapes[s].paths[p];
			// PATH_FLAG_CLOSED | PATH_FLAG_NO_CURVES
			out.push_back((1 << 1) | (1 << 3));
			out.push_back((uint8_t)path.size());
			for (size_t i = 0; i < path.size(); i++) {
				write_coord(out, path[i].x * unit);
				write_coord(out, path[i].y * unit);
			}
		}
	}

	// Each design is drawn from halfway to the next smaller design's size
	// to halfway to the next larger one's; both ends are inclusive, so
	// neighbours share their boundary scale and no size falls between.
	out.push_back((uint8_t)shapes.size());
	int pathIndex = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		const int d = shapes[s].design;
		uint8_t minScale = 0;
		uint8_t maxScale = 255;
		if (d > 0)
			minScale = lod_byte((designs[d - 1].size + designs[d].size) / 2.0);
		if (d + 1 < (int)designs.size())
			maxScale = lod_byte((designs[d].size + designs[d + 1].size) / 2.0);

		out.push_back(10);	// SHAPE_TYPE_PATH_SOURCE
		out.push_back((uint8_t)shapes[s].style);
		out.push_back((uint8_t)shapes[s].paths.size());
		for (size_t p = 0; p < shapes[s].paths.size(); p++)
			out.push_back((uint8_t)pathIndex++);
		out.push_back(1 << 3);	// SHAPE_FLAG_LOD_SCALE
		out.push_back(minScale);
		out.push_back(maxScale);
	}
}


}	// namespace


bool
encode_hvif(std::vector<IconImage> images, HVIFResult& result,
	std::string* error)
{
	for (size_t i = 0; i < images.size(); i++) {
		if (images[i].size <= 0 || images[i].rgba.size()
				!= (size_t)images[i].size * images[i].size * 4) {
			images.erase(images.begin() + i);
			i--;
		}
	}
	std::sort(images.begin(), images.end(),
		[](const IconImage& a, const IconImage& b) { return a.size < b.size; });
	for (size_t i = 1; i < images.size(); i++) {
		if (images[i].size == images[i - 1].size) {
			images.erase(images.begin() + i);
			i--;
		}
	}
	if (images.empty()) {
		if (error != NULL)
			*error = "no usable images";
		return false;
	}

	std::vector<Design> designs;
	std::vector<uint32_t> styles;
	std::vector<ShapeData> shapes;
	int pathCount;

	double budget = kMaxItems;
	while (true) {
		const std::vector<int> limits = color_limits(images, budget);
		if (build(images, limits, designs, styles, shapes, pathCount))
			break;

		bool reducible = false;
		for (size_t i = 0; i < limits.size(); i++)
			reducible |= limits[i] > kMinColorLimit;
		if (reducible)
			budget *= 0.85;
		else if (images.size() > 1) {
			// the largest design is the one most worth dropping
			images.pop_back();
			budget = kMaxItems;
		} else {
			if (error != NULL)
				*error = "too detailed to fit a vector icon";
			return false;
		}
	}

	write_icon(designs, styles, shapes, result.data);
	result.sizes.clear();
	result.reduced = false;
	for (size_t d = 0; d < designs.size(); d++) {
		result.sizes.push_back(designs[d].size);
		result.reduced |= designs[d].reduced;
	}
	result.colors = (int)styles.size();
	return true;
}
