/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Freedesktop icon themes. See IconTheme.h.
 *
 * SVG icons are drawn with nanosvg (third_party/nanosvg), which knows no
 * filters: elements with one (usually a blurred shadow) are left out
 * rather than drawn unblurred.
 */


#include "IconTheme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <TranslationUtils.h>

#include "Theme.h"

// nanosvg is kept as it was released
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "third_party/nanosvg/nanosvg.h"
#include "third_party/nanosvg/nanosvgrast.h"
#pragma GCC diagnostic pop


static const int kDesignSizes[] = { 16, 24, 32, 48 };
static const int kDesignSizeCount = sizeof(kDesignSizes) / sizeof(int);
static const int kInheritDepth = 8;


//	#pragma mark - images


static bool
bitmap_to_image(BBitmap* bitmap, IconImage& image)
{
	BBitmap rgba(bitmap->Bounds(), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	if (rgba.InitCheck() != B_OK || rgba.ImportBits(bitmap) != B_OK)
		return false;
	const int width = rgba.Bounds().IntegerWidth() + 1;
	const int height = rgba.Bounds().IntegerHeight() + 1;
	if (width != height || width > 512)
		return false;

	image.size = width;
	image.rgba.resize(width * height * 4);
	for (int y = 0; y < height; y++) {
		const uint8* row = (const uint8*)rgba.Bits() + y * rgba.BytesPerRow();
		for (int x = 0; x < width; x++) {
			// B_RGBA32 is blue, green, red, alpha in memory
			uint8* out = &image.rgba[(y * width + x) * 4];
			out[0] = row[x * 4 + 2];
			out[1] = row[x * 4 + 1];
			out[2] = row[x * 4];
			out[3] = row[x * 4 + 3];
		}
	}
	return true;
}


/*!	Removes the elements that use a filter, which nanosvg would draw as if
	they had none. A group with a filter loses only the filter, as it may
	hold the icon itself.
*/
static void
strip_filters(BString& svg)
{
	int32 start = 0;
	while ((start = svg.FindFirst('<', start)) >= 0) {
		int32 end = svg.FindFirst('>', start);
		if (end < 0)
			break;
		BString tag;
		svg.CopyInto(tag, start, end - start + 1);
		if (tag.FindFirst("filter:url(") < 0
			&& tag.FindFirst("filter=\"url(") < 0) {
			start = end + 1;
			continue;
		}
		if (tag.EndsWith("/>")) {
			svg.Remove(start, end - start + 1);
			continue;
		}
		// an opening tag: drop just its filter
		BString cleaned(tag);
		cleaned.ReplaceAll("filter:url(", "x-filter:url(");
		cleaned.ReplaceAll("filter=\"url(", "x-filter=\"url(");
		svg.Remove(start, end - start + 1);
		svg.Insert(cleaned, start);
		start += cleaned.Length();
	}
}


static bool
load_svg(const char* path, int size, IconImage& image)
{
	BFile file(path, B_READ_ONLY);
	off_t fileSize;
	if (file.InitCheck() != B_OK || file.GetSize(&fileSize) != B_OK
		|| fileSize <= 0 || fileSize > 4 * 1024 * 1024)
		return false;

	BString text;
	char* buffer = text.LockBuffer(fileSize + 1);
	ssize_t bytesRead = file.Read(buffer, fileSize);
	text.UnlockBuffer(bytesRead > 0 ? bytesRead : 0);
	strip_filters(text);

	// nanosvg parses in place
	char* copy = strdup(text.String());
	if (copy == NULL)
		return false;
	NSVGimage* svg = nsvgParse(copy, "px", 96.0f);
	free(copy);
	if (svg == NULL)
		return false;

	bool result = false;
	NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
	if (rasterizer != NULL && svg->width > 0 && svg->height > 0) {
		const float scale = size / std::max(svg->width, svg->height);
		image.size = size;
		image.rgba.assign(size * size * 4, 0);
		nsvgRasterize(rasterizer, svg,
			(size - svg->width * scale) / 2, (size - svg->height * scale) / 2,
			scale, &image.rgba[0], size, size, size * 4);
		result = true;
	}
	nsvgDeleteRasterizer(rasterizer);
	nsvgDelete(svg);
	return result;
}


bool
load_icon_image(const char* path, int size, IconImage& image)
{
	BString name(path);
	if (name.IEndsWith(".svg"))
		return load_svg(path, size, image);

	BBitmap* bitmap = BTranslationUtils::GetBitmapFile(path);
	if (bitmap == NULL)
		return false;
	bool result = bitmap_to_image(bitmap, image);
	delete bitmap;
	return result;
}


//	#pragma mark - IconTheme


IconTheme::IconTheme()
{
}


status_t
IconTheme::Load(const char* folder)
{
	fFolder = folder;
	fID = BPath(folder).Leaf();
	fName = fID;

	BPath indexPath(folder, "index.theme");
	BFile file(indexPath.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	off_t size;
	if (file.GetSize(&size) != B_OK || size > 1024 * 1024)
		return B_BAD_DATA;

	BString text;
	char* buffer = text.LockBuffer(size + 1);
	ssize_t bytesRead = file.Read(buffer, size);
	text.UnlockBuffer(bytesRead > 0 ? bytesRead : 0);

	// sections, each a message of its keys
	BMessage sections;
	BMessage current;
	BString section;
	BStringList lines;
	text.Split("\n", false, lines);
	for (int32 i = 0; i <= lines.CountStrings(); i++) {
		BString line;
		if (i < lines.CountStrings())
			line = lines.StringAt(i);
		line.Trim();
		if (i == lines.CountStrings() || line.StartsWith("[")) {
			if (!section.IsEmpty())
				sections.AddMessage(section, &current);
			current.MakeEmpty();
			section = line;
			section.RemoveFirst("[");
			section.RemoveLast("]");
			continue;
		}
		int32 equals = line.FindFirst('=');
		if (equals <= 0 || line.StartsWith("#"))
			continue;
		BString key, value;
		line.CopyInto(key, 0, equals);
		line.CopyInto(value, equals + 1, line.Length() - equals - 1);
		key.Trim();
		value.Trim();
		if (!current.HasString(key))
			current.AddString(key, value);
	}

	BMessage main;
	if (sections.FindMessage("Icon Theme", &main) != B_OK)
		return B_BAD_DATA;

	fName = main.GetString("Name", fID);
	fComment = main.GetString("Comment", "");
	BString(main.GetString("Inherits", "")).Split(",", true, fInherits);
	for (int32 i = 0; i < fInherits.CountStrings(); i++) {
		BString inherit = fInherits.StringAt(i);
		inherit.Trim();
		fInherits.Replace(i, inherit);
	}

	BStringList directories;
	BString(main.GetString("Directories", "")).Split(",", true, directories);
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		BString directory = directories.StringAt(i);
		directory.Trim();
		BMessage info;
		if (directory.IsEmpty()
			|| sections.FindMessage(directory, &info) != B_OK)
			continue;
		if (atoi(info.GetString("Scale", "1")) != 1)
			continue;

		IconFolder folder;
		folder.path = BPath(fFolder, directory).Path();
		folder.size = atoi(info.GetString("Size", "0"));
		if (folder.size <= 0)
			continue;
		BString type = info.GetString("Type", "Threshold");
		folder.scalable = type == "Scalable";
		folder.minSize = atoi(info.GetString("MinSize", "0"));
		folder.maxSize = atoi(info.GetString("MaxSize", "0"));
		if (folder.minSize <= 0)
			folder.minSize = folder.size;
		if (folder.maxSize <= 0)
			folder.maxSize = folder.size;
		fFolders.push_back(folder);
	}

	return fFolders.empty() ? B_BAD_DATA : B_OK;
}


/*!	A file drawn at exactly \a size pixels: a PNG in a folder of that size,
	or an SVG in a scalable folder that covers it.
*/
bool
IconTheme::_Find(const char* name, int size, BString& path,
	int& actualSize) const
{
	for (size_t i = 0; i < fFolders.size(); i++) {
		const IconFolder& folder = fFolders[i];
		BString base(folder.path);
		base << "/" << name;

		if (folder.size == size) {
			BString png(base);
			png << ".png";
			if (BEntry(png).Exists()) {
				path = png;
				actualSize = size;
				return true;
			}
		}

		if (folder.size == size
			|| (folder.scalable && folder.minSize <= size
				&& size <= folder.maxSize)) {
			BString svg(base);
			svg << ".svg";
			if (BEntry(svg).Exists()) {
				path = svg;
				actualSize = size;
				return true;
			}
		}
	}
	return false;
}


bool
IconTheme::_Images(const char* name, std::vector<IconImage>& images,
	BStringList& visited) const
{
	if (visited.HasString(fID) || visited.CountStrings() > kInheritDepth)
		return false;
	visited.Add(fID);

	for (int32 i = 0; i < kDesignSizeCount; i++) {
		BString path;
		int size;
		IconImage image;
		if (_Find(name, kDesignSizes[i], path, size)
			&& load_icon_image(path, size, image)
			&& image.size == kDesignSizes[i])
			images.push_back(image);
	}

	if (images.empty()) {
		// No design at the usual sizes: take the one nearest 32 pixels,
		// at its own size, which can be scaled.
		int bestDistance = 1 << 30;
		BString bestPath;
		int bestSize = 0;
		for (size_t i = 0; i < fFolders.size(); i++) {
			const IconFolder& folder = fFolders[i];
			for (int e = 0; e < 2; e++) {
				BString path(folder.path);
				path << "/" << name << (e == 0 ? ".png" : ".svg");
				if (!BEntry(path).Exists())
					continue;
				int size = folder.size;
				if (e == 1 && folder.scalable)
					size = std::max(folder.minSize, std::min(32, folder.maxSize));
				const int distance = abs(size - 32);
				if (distance < bestDistance) {
					bestDistance = distance;
					bestPath = path;
					bestSize = size;
				}
			}
		}
		IconImage image;
		if (!bestPath.IsEmpty() && load_icon_image(bestPath, bestSize, image))
			images.push_back(image);
	}

	if (!images.empty())
		return true;

	BStringList inherits(fInherits);
	if (fID != "hicolor" && !inherits.HasString("hicolor"))
		inherits.Add("hicolor");
	for (int32 i = 0; i < inherits.CountStrings(); i++) {
		if (visited.HasString(inherits.StringAt(i)))
			continue;
		IconTheme* parent = load_icon_theme(inherits.StringAt(i));
		if (parent == NULL)
			continue;
		bool found = parent->_Images(name, images, visited);
		delete parent;
		if (found)
			return true;
	}
	return false;
}


bool
IconTheme::Images(const BStringList& names, std::vector<IconImage>& images,
	BString* found) const
{
	for (int32 i = 0; i < names.CountStrings(); i++) {
		BStringList visited;
		images.clear();
		if (_Images(names.StringAt(i), images, visited)) {
			if (found != NULL)
				*found = names.StringAt(i);
			return true;
		}
	}
	return false;
}


BBitmap*
IconTheme::Bitmap(const char* name, int size) const
{
	BStringList names;
	names.Add(name);
	std::vector<IconImage> images;
	if (!Images(names, images))
		return NULL;

	// the design nearest the size wanted
	const IconImage* best = &images[0];
	for (size_t i = 1; i < images.size(); i++) {
		if (abs(images[i].size - size) < abs(best->size - size))
			best = &images[i];
	}

	BBitmap* bitmap = new BBitmap(BRect(0, 0, best->size - 1, best->size - 1),
		B_RGBA32);
	if (bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return NULL;
	}
	for (int y = 0; y < best->size; y++) {
		uint8* row = (uint8*)bitmap->Bits() + y * bitmap->BytesPerRow();
		for (int x = 0; x < best->size; x++) {
			const uint8* in = &best->rgba[(y * best->size + x) * 4];
			row[x * 4] = in[2];
			row[x * 4 + 1] = in[1];
			row[x * 4 + 2] = in[0];
			row[x * 4 + 3] = in[3];
		}
	}
	return bitmap;
}


//	#pragma mark -


void
find_icon_themes(IconThemeList& themes)
{
	BStringList directories;
	data_directories(directories);

	BStringList seen;
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		BPath root(directories.StringAt(i), "icons");
		BDirectory directory(root.Path());
		BEntry entry;
		while (directory.GetNextEntry(&entry, true) == B_OK) {
			BPath path;
			if (!entry.IsDirectory() || entry.GetPath(&path) != B_OK
				|| seen.HasString(path.Leaf()))
				continue;
			IconTheme* theme = new IconTheme;
			if (theme->Load(path.Path()) != B_OK) {
				delete theme;
				continue;
			}
			seen.Add(path.Leaf());
			themes.AddItem(theme);
		}
	}

	themes.SortItems([](const IconTheme* a, const IconTheme* b) {
		return a->Name().ICompare(b->Name());
	});
}


IconTheme*
load_icon_theme(const char* id)
{
	BStringList directories;
	data_directories(directories);
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		BPath path(directories.StringAt(i), "icons");
		path.Append(id);
		IconTheme* theme = new IconTheme;
		if (theme->Load(path.Path()) == B_OK)
			return theme;
		delete theme;
	}
	return NULL;
}
