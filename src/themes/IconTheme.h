/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Freedesktop icon themes (specifications.freedesktop.org/icon-theme-spec):
 * a folder in an "icons" data folder with an index.theme naming the theme
 * and its folders, each holding PNG or SVG icons of one size.
 *
 * Images() collects an icon's designs at the sizes Haiku draws icons at,
 * looking in the themes it inherits from too, for the HVIF encoder.
 */
#ifndef ICON_THEME_H
#define ICON_THEME_H


#include <ObjectList.h>
#include <String.h>
#include <StringList.h>

#include <vector>

#include "HVIFEncoder.h"


class BBitmap;


class IconTheme {
public:
								IconTheme();

			status_t			Load(const char* folder);

			const BString&		Name() const { return fName; }
				// as index.theme names it
			const BString&		ID() const { return fID; }
				// the folder's name, as themes refer to it
			const BString&		Comment() const { return fComment; }
			const BString&		Folder() const { return fFolder; }

			// The designs of the first of \a names this theme (or one it
			// inherits from) has, at or near 16, 24, 32 and 48 pixels.
			// Returns false if it has none of them.
			bool				Images(const BStringList& names,
									std::vector<IconImage>& images,
									BString* found = NULL) const;

			// one image of \a name at \a size pixels, for previews
			BBitmap*			Bitmap(const char* name, int size) const;

private:
	struct IconFolder {
		BString		path;
		int			size;
		int			minSize;
		int			maxSize;
		bool		scalable;
	};

			bool				_Images(const char* name,
									std::vector<IconImage>& images,
									BStringList& visited) const;
			bool				_Find(const char* name, int size,
									BString& path, int& actualSize) const;

			BString				fName;
			BString				fID;
			BString				fComment;
			BString				fFolder;
			BStringList			fInherits;
			std::vector<IconFolder> fFolders;
};


typedef BObjectList<IconTheme, true> IconThemeList;


// every installed icon theme, sorted by name
void			find_icon_themes(IconThemeList& themes);

// the installed icon theme whose folder is called \a id, or NULL
IconTheme*		load_icon_theme(const char* id);

// loads a PNG or SVG file at \a size pixels (SVG is drawn at that size;
// a PNG keeps its own size, which is returned in \a image.size)
bool			load_icon_image(const char* path, int size, IconImage& image);


#endif	// ICON_THEME_H
