#include <os.h>
#include "menu.h"

/* Menu table: context-name pointers, then records {type, string id, callback, argument, resource} ending in type 0. */
#define RECORD_WORDS 5
#define RECORD_TEXT 1
#define RESOURCE_SYST 0x73797374

static const struct os_table *os;
static void (*select_handler)(void);

static void menu_callback(void *item, int argument, int event)
{
	(void)item;
	(void)argument;
	if (event == (int)os->event_enter || event == (int)os->event_click)
		select_handler();
}

static unsigned record_count(const unsigned *records)
{
	unsigned count = 0;
	while (records[count * RECORD_WORDS] != 0)
		count++;
	return count;
}

/* The OS reads the table through one literal-pool word; point it at a heap copy with one more record. */
bool menu_install(const struct os_table *t, void (*on_select)(void))
{
	os = t;
	select_handler = on_select;
	const unsigned *old = (const unsigned *)t->menu_base;
	unsigned count = record_count(old + t->menu_context_count);
	unsigned old_words = t->menu_context_count + count * RECORD_WORDS;
	unsigned *table = malloc((old_words + 2 * RECORD_WORDS) * sizeof *table);
	if (!table)
		return false;
	memcpy(table, old, old_words * sizeof *table);
	unsigned *entry = table + old_words;
	entry[0] = RECORD_TEXT;
	entry[1] = t->menu_label_string;
	entry[2] = (unsigned)menu_callback;
	entry[3] = 0;
	entry[4] = RESOURCE_SYST;
	memset(entry + RECORD_WORDS, 0, RECORD_WORDS * sizeof *table);
	*(volatile unsigned *)t->menu_literal = (unsigned)table;
	clear_cache();
	return true;
}
