#include "glass.h"
#include "theme.h"

/* Never produced by the flip or by any stock entry. */
#define GLASS_RGB 0x010203

void glass_apply(const struct os_table *os, bool on)
{
	if (!on)
		return;
	const struct theme_ids *ids = &os->ids;
	volatile unsigned *table = (volatile unsigned *)os->color_table;
	table[ids->home_list_background] = GLASS_RGB;
	table[ids->home_doc_name_background] = GLASS_RGB;
	/* Gray text is hard to read through glass; use the section-title white. */
	table[ids->home_list_text] = theme_color(ids->home_section_title);
	table[ids->home_doc_name_text] = theme_color(ids->home_section_title);
}

void glass_on_color(Gc gc, unsigned rgb)
{
	gui_gc_setAlpha(gc, rgb == GLASS_RGB ? GC_A_HALF : GC_A_OFF);
}
