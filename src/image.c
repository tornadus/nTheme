#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "third_party/stb_image.h"

#define CHANNELS 3

struct rgb_bitmap {
	unsigned char *pixels;   /* stb output, 3 bytes per pixel */
	unsigned width, height;
};

/* Where the source lands on the destination, and which part of the source is used. */
struct placement {
	unsigned dest_x, dest_y, dest_width, dest_height;
	unsigned source_x, source_y, source_width, source_height;
};

static const char *basename_of(const char *path)
{
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

static unsigned char *read_file(const char *path, long *size, char *error, int capacity)
{
	FILE *file = fopen(path, "rb");
	if (!file) {
		snprintf(error, capacity, "%s: cannot open", basename_of(path));
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);
	unsigned char *data = *size > 0 ? malloc(*size) : NULL;
	if (!data || fread(data, 1, *size, file) != (size_t)*size) {
		snprintf(error, capacity, "%s: cannot read (%s)", basename_of(path), data ? "short read" : "out of memory");
		free(data);
		fclose(file);
		return NULL;
	}
	fclose(file);
	return data;
}

static bool decode(const unsigned char *data, long size, unsigned max_pixels, const char *name,
                   struct rgb_bitmap *bitmap, char *error, int capacity)
{
	int width, height, channels;
	if (!stbi_info_from_memory(data, size, &width, &height, &channels)) {
		snprintf(error, capacity, "%s: not a PNG or JPEG file (%s)", name, stbi_failure_reason());
		return false;
	}
	if (max_pixels && (unsigned)width * (unsigned)height > max_pixels) {
		snprintf(error, capacity, "%s: %dx%d is over the %u megapixel limit", name, width, height, max_pixels / 1000000);
		return false;
	}
	bitmap->pixels = stbi_load_from_memory(data, size, &width, &height, &channels, CHANNELS);
	if (!bitmap->pixels) {
		snprintf(error, capacity, "%s: decoding failed (%s)", name, stbi_failure_reason());
		return false;
	}
	bitmap->width = width;
	bitmap->height = height;
	return true;
}

static unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

static struct placement place(const struct rgb_bitmap *source, unsigned width, unsigned height, enum image_scale scale)
{
	struct placement p = { 0, 0, width, height, 0, 0, source->width, source->height };
	switch (scale) {
	case IMAGE_CROP:
		p.source_width = p.dest_width = min(source->width, width);
		p.source_height = p.dest_height = min(source->height, height);
		p.source_x = (source->width - p.source_width) / 2;
		p.source_y = (source->height - p.source_height) / 2;
		break;
	case IMAGE_FIT:
		if (source->width * height >= source->height * width)
			p.dest_height = source->height * width / source->width;
		else
			p.dest_width = source->width * height / source->height;
		if (p.dest_width == 0) p.dest_width = 1;
		if (p.dest_height == 0) p.dest_height = 1;
		break;
	case IMAGE_FILL:
		break;
	}
	p.dest_x = (width - p.dest_width) / 2;
	p.dest_y = (height - p.dest_height) / 2;
	return p;
}

static unsigned short rgb565(unsigned r, unsigned g, unsigned b)
{
	return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/* Average of the source box that maps onto one destination pixel; a 1x1 box for 1:1 or upscaling. */
static unsigned short sample(const struct rgb_bitmap *source, const struct placement *p, unsigned x, unsigned y)
{
	unsigned x0 = p->source_x + x * p->source_width / p->dest_width;
	unsigned x1 = p->source_x + (x + 1) * p->source_width / p->dest_width;
	unsigned y0 = p->source_y + y * p->source_height / p->dest_height;
	unsigned y1 = p->source_y + (y + 1) * p->source_height / p->dest_height;
	if (x1 <= x0) x1 = x0 + 1;
	if (y1 <= y0) y1 = y0 + 1;
	unsigned r = 0, g = 0, b = 0, count = (x1 - x0) * (y1 - y0);
	for (unsigned sy = y0; sy < y1; sy++) {
		const unsigned char *row = source->pixels + (sy * source->width + x0) * CHANNELS;
		for (unsigned sx = x0; sx < x1; sx++, row += CHANNELS) {
			r += row[0];
			g += row[1];
			b += row[2];
		}
	}
	return rgb565(r / count, g / count, b / count);
}

static struct ti_image *compose(const struct rgb_bitmap *source, unsigned width, unsigned height,
                                enum image_scale scale, unsigned background_rgb)
{
	struct ti_image *image = malloc(sizeof *image + width * height * sizeof image->pixels[0]);
	if (!image)
		return NULL;
	image->width = width;
	image->height = height;
	image->zero = 0;
	image->pitch = width * sizeof image->pixels[0];
	image->bits_per_pixel = 16;
	image->planes = 1;
	unsigned short background = rgb565((background_rgb >> 16) & 255, (background_rgb >> 8) & 255, background_rgb & 255);
	for (unsigned i = 0; i < width * height; i++)
		image->pixels[i] = background;
	struct placement p = place(source, width, height, scale);
	for (unsigned y = 0; y < p.dest_height; y++)
		for (unsigned x = 0; x < p.dest_width; x++)
			image->pixels[(p.dest_y + y) * width + p.dest_x + x] = sample(source, &p, x, y);
	return image;
}

struct ti_image *image_load(const char *path, unsigned width, unsigned height, enum image_scale scale,
                            unsigned background_rgb, unsigned max_pixels, char *error, int error_capacity)
{
	long size;
	unsigned char *data = read_file(path, &size, error, error_capacity);
	if (!data)
		return NULL;
	struct rgb_bitmap bitmap;
	bool decoded = decode(data, size, max_pixels, basename_of(path), &bitmap, error, error_capacity);
	free(data);
	if (!decoded)
		return NULL;
	struct ti_image *image = compose(&bitmap, width, height, scale, background_rgb);
	stbi_image_free(bitmap.pixels);
	if (!image)
		snprintf(error, error_capacity, "%s: out of memory", basename_of(path));
	return image;
}

void image_free(struct ti_image *image)
{
	free(image);
}
