/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Turns pixel icons into a Haiku vector icon (HVIF, the "flat icon" format
 * of BEOS:ICON and META:ICON).
 *
 * Each image is one design size of the icon (16, 24, 32, 48 pixels...).
 * Its pixels become filled polygons, one shape per colour, laid exactly on
 * the pixel grid of the 64 unit icon canvas. Each size's shapes carry a
 * level of detail range, so the renderer draws the 16 pixel design at 16
 * pixels, the 32 pixel one at 32, and the nearest one scaled in between.
 *
 * HVIF holds at most 255 styles, paths and shapes. Images with more colours
 * than fit are reduced (median cut); if that isn't enough, the largest
 * designs are left out.
 *
 * No Haiku headers: this builds and can be tested anywhere.
 */
#ifndef HVIF_ENCODER_H
#define HVIF_ENCODER_H


#include <stdint.h>

#include <string>
#include <vector>


struct IconImage {
	int						size;
		// width and height in pixels
	std::vector<uint8_t>	rgba;
		// size * size pixels, red, green, blue and alpha bytes each,
		// not premultiplied
};


struct HVIFResult {
	std::vector<uint8_t>	data;
	std::vector<int>		sizes;
		// the design sizes it holds, smallest first
	int						colors;
		// colours used, after any reduction
	bool					reduced;
		// colours were merged to fit
};


bool encode_hvif(std::vector<IconImage> images, HVIFResult& result,
	std::string* error = NULL);


#endif	// HVIF_ENCODER_H
