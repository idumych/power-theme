#!/usr/bin/env python3
"""Make the HOTDOG Topaz fonts, regular and bold, for the Amiga theme.

    python3 -I tools/make-topaz-font.py

Reads src/third_party/amiga/topaz/topaz.txt (Topaz as Arthur Choung's HOTDOG
draws it, GPL 3 or later) and writes data/fonts/HOTDOGTopaz.ttf and
HOTDOGTopaz-Bold.ttf: TrueType fonts whose glyphs are squares, a pixel each, so
that at 16 points, a pixel to a unit square, they look as HOTDOG's bitmap text
does. The bold one is each glyph laid over itself a pixel to the right, as the
Amiga made bold text. No libraries are needed: the tables are written here.
"""

import os
import struct
import time

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SOURCE = os.path.join(ROOT, "src", "third_party", "amiga", "topaz", "topaz.txt")
OUT = os.path.join(ROOT, "data", "fonts")

ROWS = 16
COLUMNS = 8
PIXEL = 64                 # units to a pixel
UNITS = ROWS * PIXEL       # the em is a glyph's height
ADVANCE = COLUMNS * PIXEL


def read_glyphs():
	glyphs = {}
	code = None
	for line in open(SOURCE):
		line = line.rstrip("\n")
		if not line or line.startswith("#"):
			continue
		if line.isdigit():
			code = int(line)
			glyphs[code] = []
		else:
			glyphs[code].append(line)
	for code, rows in glyphs.items():
		rows += ["." * COLUMNS] * (ROWS - len(rows))
	return glyphs


def embolden(rows):
	out = []
	for row in rows:
		cells = [c == "b" for c in row]
		out.append("".join("b" if cells[x] or (x > 0 and cells[x - 1]) else "."
			for x in range(COLUMNS)))
	return out


def rectangles(rows, baseline):
	"""The glyph's pixels as rectangles, runs along each row merged with the
	same runs in the rows below."""
	runs = []
	for y, row in enumerate(rows):
		x = 0
		while x < COLUMNS:
			if row[x] != "b":
				x += 1
				continue
			start = x
			while x < COLUMNS and row[x] == "b":
				x += 1
			runs.append([start, x, y, y + 1])
	merged = []
	for run in runs:
		for m in merged:
			if m[0] == run[0] and m[1] == run[1] and m[3] == run[2]:
				m[3] = run[3]
				break
		else:
			merged.append(run)
	# in font units, y up from the baseline
	return [(x0 * PIXEL, (baseline - y1) * PIXEL, x1 * PIXEL,
		(baseline - y0) * PIXEL) for x0, x1, y0, y1 in merged]


def glyf_entry(rects):
	if not rects:
		return b"", (0, 0, 0, 0), 0
	xs = [r[0] for r in rects] + [r[2] for r in rects]
	ys = [r[1] for r in rects] + [r[3] for r in rects]
	box = (min(xs), min(ys), max(xs), max(ys))
	ends = []
	points = []
	for x0, y0, x1, y1 in rects:
		# clockwise, y up: bottom left, top left, top right, bottom right
		points += [(x0, y0), (x0, y1), (x1, y1), (x1, y0)]
		ends.append(len(points) - 1)
	data = struct.pack(">hhhhh", len(rects), *box)
	data += struct.pack(">%dH" % len(ends), *ends)
	data += struct.pack(">H", 0)  # no instructions
	data += bytes([0x01] * len(points))  # on curve, two-byte coordinates
	px = py = 0
	xd = b""
	yd = b""
	for x, y in points:
		xd += struct.pack(">h", x - px)
		yd += struct.pack(">h", y - py)
		px, py = x, y
	data += xd + yd
	if len(data) % 4:
		data += b"\0" * (4 - len(data) % 4)
	return data, box, len(points)


def checksum(data):
	data += b"\0" * (-len(data) % 4)
	return sum(struct.unpack(">%dL" % (len(data) // 4), data)) & 0xffffffff


def name_table(names):
	records = b""
	strings = b""
	for nid, text in names:
		encoded = text.encode("utf-16-be")
		records += struct.pack(">HHHHHH", 3, 1, 0x409, nid, len(encoded),
			len(strings))
		strings += encoded
	header = struct.pack(">HHH", 0, len(names), 6 + len(records))
	return header + records + strings


def cmap_table(codes):
	# format 4: one segment per run of codes, glyph i + 1 for code i
	segments = []
	for i, code in enumerate(codes):
		glyph = i + 1
		if segments and segments[-1][1] == code - 1 \
				and segments[-1][2] + (code - segments[-1][0]) == glyph:
			segments[-1][1] = code
		else:
			segments.append([code, code, glyph])
	segments.append([0xffff, 0xffff, 0])
	n = len(segments)
	search = 2 ** (n.bit_length() - 1) * 2
	ends = b"".join(struct.pack(">H", s[1]) for s in segments)
	starts = b"".join(struct.pack(">H", s[0]) for s in segments)
	deltas = b"".join(struct.pack(">H", (s[2] - s[0]) % 0x10000)
		for s in segments[:-1]) + struct.pack(">H", 1)
	offsets = b"\0\0" * n
	body = struct.pack(">HHHH", n * 2, search, search.bit_length() - 2,
		n * 2 - search) + ends + b"\0\0" + starts + deltas + offsets
	sub = struct.pack(">HHH", 4, 6 + len(body), 0) + body
	return struct.pack(">HHHHL", 0, 1, 3, 1, 12) + sub


def build(glyphs, bold, path):
	codes = sorted(glyphs)
	# the baseline: under H's last row
	h = glyphs[ord("H")]
	baseline = max(y for y, row in enumerate(h) if "b" in row) + 1
	ascent = baseline * PIXEL
	descent = (ROWS - baseline) * PIXEL

	glyf = b""
	loca = [0]
	boxes = []
	max_points = max_contours = 0
	order = [None] + codes  # .notdef first
	for code in order:
		rows = glyphs[code] if code is not None else ["." * COLUMNS] * ROWS
		if bold:
			rows = embolden(rows)
		rects = rectangles(rows, baseline)
		data, box, points = glyf_entry(rects)
		glyf += data
		loca.append(len(glyf))
		boxes.append(box)
		max_points = max(max_points, points)
		max_contours = max(max_contours, len(rects))
	n = len(order)
	real = [b for b in boxes if b != (0, 0, 0, 0)]
	xmin = min(b[0] for b in real)
	ymin = min(b[1] for b in real)
	xmax = max(b[2] for b in real)
	ymax = max(b[3] for b in real)

	now = int(time.mktime((2026, 11, 8, 0, 0, 0, 0, 0, 0))) + 2082844800
	style = "Bold" if bold else "Regular"
	family = "HOTDOG Topaz"
	tables = {}
	tables[b"head"] = struct.pack(">LLLLHHqqhhhhHHhhh", 0x00010000, 0x00010000,
		0, 0x5F0F3CF5, 0x000B, UNITS, now, now, xmin, ymin, xmax, ymax,
		1 if bold else 0, 8, 2, 1, 0)
	tables[b"hhea"] = struct.pack(">LhhhHhhhhhhhhhhhH", 0x00010000, ascent,
		-descent, 0, ADVANCE, 0, 0, xmax, 1, 0, 0, 0, 0, 0, 0, 0, n)
	tables[b"maxp"] = struct.pack(">LHHHHHHHHHHHHHH", 0x00010000, n,
		max_points, max_contours, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0)
	tables[b"OS/2"] = struct.pack(">HhHHHhhhhhhhhhhh10sLLLL4sHHHhhhHHLLhhHHH",
		4, ADVANCE, 700 if bold else 400, 5, 0,
		PIXEL * 6, PIXEL * 7, 0, PIXEL * 2, PIXEL * 6, PIXEL * 7, 0, PIXEL * 6,
		PIXEL, PIXEL * 4,
		0, bytes([2, 11, 6, 9, 3, 0, 0, 0, 0, 0]) if not bold
			else bytes([2, 11, 8, 9, 3, 0, 0, 0, 0, 0]),
		1, 0, 0, 0, b"HTDG",
		0x20 if bold else 0x40, codes[0], codes[-1],
		ascent, -descent, 0, ascent, descent, 1, 0,
		PIXEL * 5, PIXEL * 7, 0, 32, 1)
	tables[b"hmtx"] = b"".join(struct.pack(">Hh", ADVANCE, b[0]) for b in boxes)
	tables[b"cmap"] = cmap_table(codes)
	tables[b"loca"] = b"".join(struct.pack(">L", o) for o in loca)
	tables[b"glyf"] = glyf
	tables[b"name"] = name_table([
		(0, "Topaz as drawn by HOTDOG, Copyright (c) 2020 Arthur Choung; "
			"GNU GPL version 3 or later"),
		(1, family), (2, style), (3, "%s %s" % (family, style)),
		(4, "%s %s" % (family, style)), (5, "Version 1.0"),
		(6, "HOTDOGTopaz-%s" % style),
	])
	tables[b"post"] = struct.pack(">LLhhLLLLL", 0x00030000, 0, -PIXEL, PIXEL,
		1, 0, 0, 0, 0)

	tags = sorted(tables)
	count = len(tags)
	search = 2 ** (count.bit_length() - 1)
	header = struct.pack(">LHHHH", 0x00010000, count, search * 16,
		search.bit_length() - 1, count * 16 - search * 16)
	offset = 12 + 16 * count
	directory = b""
	body = b""
	for tag in tags:
		data = tables[tag]
		directory += struct.pack(">4sLLL", tag, checksum(data), offset + len(body),
			len(data))
		body += data + b"\0" * (-len(data) % 4)
	font = bytearray(header + directory + body)
	# the head table's checksum adjustment, over the whole file
	head_offset = 12 + 16 * count + sum(
		len(tables[t]) + (-len(tables[t]) % 4) for t in tags[:tags.index(b"head")])
	adjustment = (0xB1B0AFBA - checksum(bytes(font))) & 0xffffffff
	struct.pack_into(">L", font, head_offset + 8, adjustment)
	with open(path, "wb") as f:
		f.write(font)


def main():
	glyphs = read_glyphs()
	os.makedirs(OUT, exist_ok=True)
	build(glyphs, False, os.path.join(OUT, "HOTDOGTopaz.ttf"))
	build(glyphs, True, os.path.join(OUT, "HOTDOGTopaz-Bold.ttf"))


if __name__ == "__main__":
	main()
