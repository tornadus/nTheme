/* Home screen wallpaper: files under ndless/wallpapers, one decoded image kept for the painter hook. */
#ifndef NTHEME_WALLPAPER_H
#define NTHEME_WALLPAPER_H

#include <stdbool.h>
#include "image.h"

#define WALLPAPER_WIDTH 320
#define WALLPAPER_TOP 23   /* title bar height */
#define WALLPAPER_HEIGHT (240 - WALLPAPER_TOP)
#define WALLPAPER_NAME_MAX 64
#define WALLPAPER_MAX_FILES 32
/* Decoding needs about 4 bytes per pixel at peak. A CX II-T on OS 6.4 handed out 26 MB of heap in 256 KB
 * chunks (measured in Firebird, 2026-09-11), so 4 megapixels leaves the OS more than half of that. */
#define WALLPAPER_MAX_PIXELS 4000000

/* Creates ndless/wallpapers if it is missing, so there is a place to drop files. */
void wallpaper_create_folder(void);

/* Sorted *.tns names in the folder. Returns the count. */
int wallpaper_list(char names[][WALLPAPER_NAME_MAX], int max);

/* Replace the current image. Off or a missing file simply means no wallpaper; false = a real failure, see `error`.
 * With `defer` the decode waits for the first home screen paint instead: memory is too constrained while startup
 * programs run (only a few MB free), and a deferred failure is kept for wallpaper_take_deferred_error(). */
bool wallpaper_apply(bool on, const char *file, enum image_scale scale, unsigned background_rgb, bool defer,
                     char *error, int error_capacity);

/* The reason a deferred decode failed, once; NULL when there is none (yet). */
const char *wallpaper_take_deferred_error(void);

/* Hook side: TI.Image to draw, or NULL. Runs a deferred decode on first use. */
const void *wallpaper_image(void);

#endif
