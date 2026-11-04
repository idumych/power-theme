/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * A dark version of a Haiku vector icon (HVIF). Its shapes stay as they are.
 * Each colour moves onto a chosen ramp for its colour family, darker than
 * Haiku's but keeping deep shadows and bright highlights, and a thin soft white
 * outline goes behind the shapes so that the icon stands out on a dark
 * background. Soft black shadows stay.
 *
 * This knows nothing of Haiku, so it can be tested anywhere.
 */
#ifndef DARK_ICONS_H
#define DARK_ICONS_H


#include <stddef.h>
#include <stdint.h>

#include <vector>


// False if \a data isn't an icon that can be read, leaving \a dark empty.
bool	make_dark_icon(const uint8_t* data, size_t size,
			std::vector<uint8_t>& dark);


#endif	// DARK_ICONS_H
