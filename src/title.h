/* Home screen title bar text. */
#ifndef NTHEME_TITLE_H
#define NTHEME_TITLE_H

#include <stdint.h>

enum title_mode {
	TITLE_DEFAULT,   /* the OS draws its own text */
	TITLE_BLANK,
	TITLE_TEXT
};

#define TITLE_MAX 31

void title_set(enum title_mode mode, const char *ascii);

/* Hook side: text to draw, or NULL for the OS default. */
const uint16_t *title_text(int *length);

#endif
