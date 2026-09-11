#ifndef NTHEME_UTF16_H
#define NTHEME_UTF16_H

#include <stdint.h>

/* Copy ASCII into a NUL-terminated UTF-16LE buffer of `capacity` units. Returns the length written. */
int utf16_from_ascii(uint16_t *destination, int capacity, const char *source);

#endif
