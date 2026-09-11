#include "utf16.h"

int utf16_from_ascii(uint16_t *destination, int capacity, const char *source)
{
	int length = 0;
	while (source[length] && length < capacity - 1) {
		destination[length] = (unsigned char)source[length];
		length++;
	}
	destination[length] = 0;
	return length;
}
