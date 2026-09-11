/* Decode a PNG/JPEG file into a TI.Image (the OS's native RGB565 image struct) of a fixed size. */
#ifndef NTHEME_IMAGE_H
#define NTHEME_IMAGE_H

#include <stdbool.h>

enum image_scale {   /* order matches OPTIONS_SCALE */
	IMAGE_CROP,      /* 1:1 pixels, centered, edges cut */
	IMAGE_FIT,       /* whole picture visible, bars in the background color */
	IMAGE_FILL       /* stretched to the full size */
};

struct ti_image {
	unsigned width, height, zero, pitch;
	unsigned short bits_per_pixel, planes;
	unsigned short pixels[];
};

/* NULL on failure, with a user-readable reason in `error`. `*no_memory` tells a memory shortage (worth retrying
 * once the OS has freed some) apart from a bad file. max_pixels 0 = no limit. */
struct ti_image *image_load(const char *path, unsigned width, unsigned height, enum image_scale scale,
                            unsigned background_rgb, unsigned max_pixels, bool *no_memory,
                            char *error, int error_capacity);

void image_free(struct ti_image *image);

#endif
