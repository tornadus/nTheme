#include <os.h>
#include "settings.h"
#include "dialog.h"
#include "options.h"
#include "wallpaper.h"
#include "utf16.h"
#include "version.h"

#define NONE_LABEL "(none)"

/* make EXTRA_CFLAGS=-DNTHEME_TRACE: announce each dialog step on screen. */
#ifdef NTHEME_TRACE
#include "report.h"
#define TRACE(step, value) report_info("%s = %p", step, (void *)(value))
#else
#define TRACE(step, value) ((void)0)
#endif
#define OS_STRING_OK 0x92
#define OS_STRING_CANCEL 0x93

struct page {
	component_t theme, accent, use_wallpaper, wallpaper, scale, glass;
	char wallpapers[WALLPAPER_MAX_FILES][WALLPAPER_NAME_MAX];
	int wallpaper_count;
	struct config *config;
	bool accepted;
};

/* UTF-16 strings and item lists live here for the dialog's lifetime. */
static uint16_t text_pool[2048];
static const uint16_t *pointer_pool[WALLPAPER_MAX_FILES + 24];
static int text_used, pointers_used;
static const uint16_t empty[1];
static const char *failure;

/* Records the first failing step; returns NULL for use as the failed value. */
static component_t fail(const char *step)
{
	if (!failure)
		failure = step;
	return NULL;
}

static void reset_pools(void)
{
	text_used = pointers_used = 0;
}

static const uint16_t *text(const char *ascii)
{
	int room = (int)(sizeof text_pool / sizeof text_pool[0]) - text_used;
	if (room <= 1)
		return empty;
	uint16_t *s = text_pool + text_used;
	text_used += utf16_from_ascii(s, room, ascii) + 1;
	return s;
}

/* Localized OS text, as the OS's own dialogs use. */
static const uint16_t *os_string(int id)
{
	return (const uint16_t *)get_res_string(RES_SYST, id);
}

static const uint16_t *const *list_begin(void)
{
	return pointer_pool + pointers_used;
}

static void list_add(const char *ascii)
{
	if (pointers_used < (int)(sizeof pointer_pool / sizeof pointer_pool[0]) - 1)
		pointer_pool[pointers_used++] = text(ascii);
}

static void list_end(void)
{
	pointer_pool[pointers_used++] = NULL;
}

static const uint16_t *const *option_items(const struct option_set *set)
{
	const uint16_t *const *items = list_begin();
	for (int i = 0; i < set->count; i++)
		list_add(set->labels[i]);
	list_end();
	return items;
}

/* File names without their .tns suffix; "(none)" when the folder is empty. */
static const uint16_t *const *wallpaper_items(const struct page *page)
{
	const uint16_t *const *items = list_begin();
	if (page->wallpaper_count == 0)
		list_add(NONE_LABEL);
	for (int i = 0; i < page->wallpaper_count; i++) {
		char label[WALLPAPER_NAME_MAX];
		strcpy(label, page->wallpapers[i]);
		label[strlen(label) - strlen(".tns")] = 0;
		list_add(label);
	}
	list_end();
	return items;
}

static int wallpaper_index(const struct page *page)
{
	for (int i = 0; i < page->wallpaper_count; i++)
		if (strcmp(page->wallpapers[i], page->config->wallpaper_file) == 0)
			return i;
	return 0;
}

static component_t combo_row(component_t panel, const char *label, const uint16_t *const *items)
{
	component_t caption = dialog_label(panel, text(label));
	TRACE(label, caption);
	if (!caption)
		return fail("label");
	component_t combo = dialog_combo(panel, items);
	TRACE("combo", combo);
	if (!combo)
		return fail("combo");
	if (!dialog_pair(panel, caption, combo))
		return fail("pair label with combo");
	return combo;
}

static void show_values(const struct page *page)
{
	const struct config *c = page->config;
	dialog_combo_set(page->theme, c->theme);
	dialog_combo_set(page->accent, c->accent);
	dialog_checkbox_set(page->use_wallpaper, c->wallpaper);
	dialog_combo_set(page->wallpaper, wallpaper_index(page));
	dialog_combo_set(page->scale, c->wallpaper_scale);
	dialog_checkbox_set(page->glass, c->glass);
}

static void read_values(struct page *page)
{
	struct config *c = page->config;
	c->theme = dialog_combo_get(page->theme);
	c->accent = dialog_combo_get(page->accent);
	c->wallpaper = dialog_checkbox_get(page->use_wallpaper);
	int file = dialog_combo_get(page->wallpaper);
	if (page->wallpaper_count > 0)
		strcpy(c->wallpaper_file, page->wallpapers[file]);
	c->wallpaper_scale = dialog_combo_get(page->scale);
	c->glass = dialog_checkbox_get(page->glass);
}

static void on_ok(component_t button, void *context, int event)
{
	struct page *page = context;
	if (!dialog_is_activation(event))
		return;
	read_values(page);
	page->accepted = true;
	dialog_close(dialog_of(button));
}

static void on_cancel(component_t button, void *context, int event)
{
	(void)context;
	if (dialog_is_activation(event))
		dialog_close(dialog_of(button));
}

static component_t checkbox_row(component_t panel, const char *label)
{
	component_t box = dialog_checkbox(panel, text(label));
	return box ? box : fail("checkbox");
}

static bool build(dialog_t dialog, struct page *page)
{
	component_t content = dialog_content(dialog);
	TRACE("content", content);
	if (!content)
		return fail("content panel");
	component_t rows = dialog_panel(content, PANEL_ROWS);
	TRACE("rows", rows);
	if (!rows)
		return fail("rows panel");
	page->theme = combo_row(rows, "Theme", option_items(&OPTIONS_THEME));
	page->accent = combo_row(rows, "Accent", option_items(&OPTIONS_ACCENT));
	page->use_wallpaper = checkbox_row(rows, "Use wallpaper");
	page->wallpaper = combo_row(rows, "Wallpaper", wallpaper_items(page));
	page->scale = combo_row(rows, "Scaling", option_items(&OPTIONS_SCALE));
	page->glass = checkbox_row(rows, "Glass home screen");
	component_t buttons = dialog_panel(content, PANEL_BUTTONS);
	TRACE("buttons", buttons);
	if (!buttons)
		return fail("button panel");
	component_t ok = dialog_button(buttons, os_string(OS_STRING_OK), page, on_ok);
	component_t cancel = dialog_button(buttons, os_string(OS_STRING_CANCEL), page, on_cancel);
	if (!ok || !cancel)
		return fail("button");
	if (!dialog_pair(buttons, ok, cancel))
		return fail("pair buttons");
	if (failure)
		return false;
	dialog_set_default_button(dialog, ok);
	dialog_set_escape_button(dialog, cancel);
	show_values(page);
	return true;
}

enum settings_result settings_run(struct config *config)
{
	struct page page;
	memset(&page, 0, sizeof page);
	page.config = config;
	page.wallpaper_count = wallpaper_list(page.wallpapers, WALLPAPER_MAX_FILES);
	reset_pools();
	failure = NULL;
	dialog_t dialog = dialog_new(text("nTheme " NTHEME_VERSION));
	TRACE("dialog", dialog);
	if (!dialog) {
		fail("dialog");
		return SETTINGS_FAILED;
	}
	bool built = build(dialog, &page);
	TRACE("built", built);
	if (built)
		dialog_run(dialog);
	TRACE("run done", 1);
	dialog_delete(dialog);
	TRACE("deleted", 1);
	if (!built)
		return SETTINGS_FAILED;
	return page.accepted ? SETTINGS_ACCEPTED : SETTINGS_CANCELED;
}

const char *settings_failure(void)
{
	return failure;
}
