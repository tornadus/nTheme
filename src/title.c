#include <stdbool.h>
#include "title.h"
#include "utf16.h"

static uint16_t text[TITLE_MAX + 1];
static int text_length;
static volatile bool custom;

void title_set(enum title_mode mode, const char *ascii)
{
	custom = false;
	text_length = mode == TITLE_TEXT ? utf16_from_ascii(text, TITLE_MAX + 1, ascii) : 0;
	text[text_length] = 0;
	custom = mode != TITLE_DEFAULT;
}

const uint16_t *title_text(int *length)
{
	if (!custom)
		return 0;
	*length = text_length;
	return text;
}
