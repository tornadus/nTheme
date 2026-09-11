#include <stdlib.h>
#include "theme.h"
#include "dark.h"

/* 2D math editor default config: text, background, cursor, and two more colors. Verified osid 48. */
static const unsigned editor_fields[] = { 0x34, 0x38, 0x44, 0x84, 0xa0 };
#define EDITOR_FIELD_COUNT (sizeof editor_fields / sizeof editor_fields[0])
#define EDITOR_COUNT 3

#define DARK_GRID 0x505050
#define ACCENT_TEXT_ON_LIGHT 0x101010
#define LIGHT_ACCENT_LUMINANCE 170   /* above this, white text on the accent is unreadable */

static const struct os_table *os;
static unsigned *stock;
static unsigned stock_editor[EDITOR_COUNT][EDITOR_FIELD_COUNT];
static unsigned stock_grid;
static volatile bool dark;

static volatile unsigned *table(void)
{
	return (volatile unsigned *)os->color_table;
}

static volatile unsigned *editor_field(int editor, unsigned field)
{
	return (volatile unsigned *)(os->editor_config[editor] + editor_fields[field]);
}

static volatile unsigned *plot_grid(void)
{
	return (volatile unsigned *)os->plot_palette + os->ids.plot_grid_index;
}

bool theme_is_stock(const struct os_table *t)
{
	volatile unsigned *editor = (volatile unsigned *)t->editor_config[0];
	return ((volatile unsigned *)t->color_table)[t->ids.home_section_title] == 0xFFFFFF
	    && editor[editor_fields[0] / 4] == 1 && editor[editor_fields[1] / 4] == 0xFFFFFF;
}

bool theme_snapshot(const struct os_table *t)
{
	os = t;
	stock = malloc(os->color_count * sizeof *stock);
	if (!stock)
		return false;
	for (unsigned i = 0; i < os->color_count; i++)
		stock[i] = table()[i];
	for (int e = 0; e < EDITOR_COUNT; e++)
		for (unsigned f = 0; f < EDITOR_FIELD_COUNT; f++)
			stock_editor[e][f] = *editor_field(e, f);
	stock_grid = *plot_grid();
	return true;
}

static bool keep_in_dark(unsigned index)
{
	const struct theme_ids *ids = &os->ids;
	if (index >= ids->keep_first && index <= ids->keep_last)
		return true;
	for (unsigned k = 0; k < ids->keep_count; k++)
		if (ids->keep[k] == index)
			return true;
	return false;
}

static bool is_light(unsigned rgb)
{
	unsigned luminance = (299 * ((rgb >> 16) & 255) + 587 * ((rgb >> 8) & 255) + 114 * (rgb & 255)) / 1000;
	return luminance > LIGHT_ACCENT_LUMINANCE;
}

static void apply_table(bool dark_mode, unsigned accent_rgb)
{
	const struct theme_ids *ids = &os->ids;
	for (unsigned i = 0; i < os->color_count; i++) {
		unsigned v = stock[i];
		table()[i] = !dark_mode ? v : keep_in_dark(i) ? dark_nudge(v) : dark_flip(v);
	}
	if (dark_mode)
		for (unsigned k = 0; k < ids->dim_count; k++)
			table()[ids->dim[k]] = dark_dim(stock[ids->dim[k]]);
	for (unsigned i = 0; i < os->color_count; i++)
		if (stock[i] == ids->stock_accent)
			table()[i] = accent_rgb;
	if (is_light(accent_rgb))
		for (unsigned k = 0; k < ids->accent_text_count; k++)
			table()[ids->accent_text[k]] = ACCENT_TEXT_ON_LIGHT;
}

static void apply_editors(bool dark_mode)
{
	for (int e = 0; e < EDITOR_COUNT; e++)
		for (unsigned f = 0; f < EDITOR_FIELD_COUNT; f++) {
			unsigned v = stock_editor[e][f];
			if (dark_mode)
				v = dark_flip(v == 1 ? 0 : v);
			*editor_field(e, f) = v;
		}
}

void theme_apply(enum theme_mode mode, unsigned accent_rgb)
{
	bool dark_mode = mode == THEME_DARK;
	apply_table(dark_mode, accent_rgb);
	apply_editors(dark_mode);
	*plot_grid() = dark_mode ? DARK_GRID : stock_grid;
	dark = dark_mode;
}

unsigned theme_color(unsigned id)
{
	return table()[id];
}

unsigned theme_remap(unsigned rgb)
{
	return dark ? dark_remap(rgb) : rgb;
}
