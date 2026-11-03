/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * icon-test: checks the HVIF encoder against Haiku's own icon renderer.
 *
 *   icon-test <output folder> <16px.png> [<32px.png> ...]
 *
 * The PNGs are the design sizes of one icon. They are encoded, the result
 * is rendered back with BIconUtils at each design size and compared with
 * the source pixel by pixel (both laid over white), and written out as
 * icon.hvif and render-<size>.png (also at 24, 40 and 64 pixels).
 */


#include <stdio.h>

#include <Application.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <IconUtils.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include "../src/themes/HVIFEncoder.h"


static bool
to_image(BBitmap* bitmap, IconImage& image)
{
	BBitmap rgba(bitmap->Bounds(), B_RGBA32);
	if (rgba.ImportBits(bitmap) != B_OK)
		return false;
	const int width = rgba.Bounds().IntegerWidth() + 1;
	const int height = rgba.Bounds().IntegerHeight() + 1;
	if (width != height)
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


static void
save_png(BBitmap* bitmap, const char* path)
{
	BBitmapStream stream(bitmap);
	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	BTranslatorRoster::Default()->Translate(&stream, NULL, NULL, &file,
		B_PNG_FORMAT);
	BBitmap* detached;
	stream.DetachBitmap(&detached);
}


static double
over_white(uint8 value, uint8 alpha)
{
	return (value * alpha + 255.0 * (255 - alpha)) / 255.0;
}


int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: icon-test <output folder> <png>...\n");
		return 2;
	}

	// bitmaps need a connection to app_server
	BApplication app("application/x-vnd.Reactionary-IconTest");

	std::vector<IconImage> images;
	for (int i = 2; i < argc; i++) {
		BBitmap* bitmap = BTranslationUtils::GetBitmapFile(argv[i]);
		IconImage image;
		if (bitmap == NULL || !to_image(bitmap, image)) {
			fprintf(stderr, "%s: can't read a square image\n", argv[i]);
			return 1;
		}
		delete bitmap;
		images.push_back(image);
	}

	HVIFResult result;
	std::string error;
	if (!encode_hvif(images, result, &error)) {
		fprintf(stderr, "encoding failed: %s\n", error.c_str());
		return 1;
	}
	printf("%zu bytes, %d colours%s, sizes:", result.data.size(),
		result.colors, result.reduced ? " (reduced)" : "");
	for (size_t i = 0; i < result.sizes.size(); i++)
		printf(" %d", result.sizes[i]);
	printf("\n");

	BPath out(argv[1]);
	BPath hvif(out.Path(), "icon.hvif");
	BFile file(hvif.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	file.Write(result.data.data(), result.data.size());

	std::vector<int> sizes = { 16, 24, 32, 40, 48, 64 };
	int failures = 0;
	for (size_t s = 0; s < sizes.size(); s++) {
		const int size = sizes[s];
		BBitmap rendered(BRect(0, 0, size - 1, size - 1), B_RGBA32);
		memset(rendered.Bits(), 0, rendered.BitsLength());
		status_t status = BIconUtils::GetVectorIcon(result.data.data(),
			result.data.size(), &rendered);
		if (status != B_OK) {
			printf("%d: render failed: %s\n", size, strerror(status));
			failures++;
			continue;
		}

		char name[64];
		snprintf(name, sizeof(name), "render-%d.png", size);
		BPath path(out.Path(), name);
		save_png(&rendered, path.Path());

		// compare with the source design of this size, if there is one
		for (size_t i = 0; i < images.size(); i++) {
			if (images[i].size != size)
				continue;
			bool encoded = false;
			for (size_t k = 0; k < result.sizes.size(); k++)
				encoded |= result.sizes[k] == size;
			double total = 0;
			double worst = 0;
			int bad = 0;
			for (int y = 0; y < size; y++) {
				const uint8* row = (const uint8*)rendered.Bits()
					+ y * rendered.BytesPerRow();
				for (int x = 0; x < size; x++) {
					const uint8* source = &images[i].rgba[(y * size + x) * 4];
					const uint8* pixel = &row[x * 4];
					for (int c = 0; c < 3; c++) {
						const double a = over_white(source[c], source[3]);
						const double b = over_white(pixel[2 - c], pixel[3]);
						const double difference = fabs(a - b);
						total += difference;
						worst = std::max(worst, difference);
						if (difference > 48)
							bad++;
					}
				}
			}
			printf("%d: mean difference %.2f, worst %.0f, %d channels off "
				"by more than 48%s\n", size, total / (size * size * 3), worst,
				bad, encoded ? "" : " (design left out)");
		}
	}
	return failures > 0 ? 1 : 0;
}
