#include <string.h>
#include "options.h"

#define COUNT(a) ((int)(sizeof (a) / sizeof (a)[0]))

static const char *const theme_names[] = { "light", "dark" };
static const char *const theme_labels[] = { "Light", "Dark (experimental)" };

static const char *const accent_names[] = { "red", "orange", "yellow", "blue", "green", "purple", "pink" };
static const char *const accent_labels[] = { "Red", "Orange", "Yellow", "Blue", "Green", "Purple", "Pink" };
const unsigned ACCENT_RGB[] = { 0xD32F2F, 0xF7821B, 0xF2B705, 0x2478CF, 0x2E9E44, 0x7E57C2, 0xE0559A };

static const char *const scale_names[] = { "crop", "fit", "fill" };
static const char *const scale_labels[] = { "Crop", "Fit", "Fill" };

static const char *const switch_names[] = { "off", "on" };
static const char *const switch_labels[] = { "Off", "On" };

const struct option_set OPTIONS_THEME = { theme_names, theme_labels, COUNT(theme_names) };
const struct option_set OPTIONS_ACCENT = { accent_names, accent_labels, COUNT(accent_names) };
const struct option_set OPTIONS_SCALE = { scale_names, scale_labels, COUNT(scale_names) };
const struct option_set OPTIONS_SWITCH = { switch_names, switch_labels, COUNT(switch_names) };

int option_index(const struct option_set *set, const char *name)
{
	for (int i = 0; i < set->count; i++)
		if (strcmp(set->names[i], name) == 0)
			return i;
	return -1;
}
