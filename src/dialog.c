#include <string.h>
#include "dialog.h"

/* Literals the OS passes; meaning unknown, values verified osid 48. */
enum {
	DIALOG_FLAGS = 0x11941,
	LAYOUT_ROWS = 0x128e9,
	LAYOUT_BUTTONS = 0x128ed,
	LAYOUT_OK = 0x124f9,
	PAIR_OK = 0x124f9,       /* 0x124f8 is the failure code */
	LABEL_ALIGN = 0x130b1,
	COMBO_ALIGN = 0x130b2,
	ROW_ALIGN = 0x1349b,
	COMBO_KIND = 0x13882,
	CHECKBOX_KIND = 0x153d9,
	CHECKED = 0x13c69,
	UNCHECKED = 0x13c6a,
	RUN = 1,
	CLOSE = 0
};

struct dialog_options {
	const char *name;
	unsigned flags;
};

typedef void *(*new_fn)(void *parent, const uint16_t *title, struct dialog_options *options);
typedef void *(*unary_fn)(void *);
typedef void *(*panel_fn)(void *parent, int flags);
typedef unsigned (*layout_fn)(void *panel, unsigned layout);
typedef void *(*label_fn)(void *panel, const uint16_t *text, int, int, int, unsigned, unsigned);
typedef void *(*combo_fn)(void *panel, unsigned kind, const uint16_t *const *items, unsigned, unsigned, int, int);
typedef void *(*checkbox_fn)(void *panel, unsigned kind, const uint16_t *text);
typedef unsigned (*pair_fn)(void *panel, void *first, void *second);
typedef void *(*button_fn)(void *panel, const uint16_t *text, void *context, dialog_callback callback);
typedef void (*dialog_button_fn)(void *dialog, void *button);
typedef void (*run_fn)(void *dialog, int run);
typedef int (*get_fn)(void *);
typedef void (*set_fn)(void *, int);

static const struct os_table *os;
static const struct dialog_addresses *at;

void dialog_bind(const struct os_table *t)
{
	os = t;
	at = &t->dialog;
}

dialog_t dialog_new(const uint16_t *title)
{
	static struct dialog_options options;
	memset(&options, 0, sizeof options);
	options.name = "DLG AFW - nTheme";
	options.flags = DIALOG_FLAGS;
	return ((new_fn)at->dialog_new)(NULL, title, &options);
}

component_t dialog_content(dialog_t dialog)
{
	return ((unary_fn)at->dialog_content)(dialog);
}

component_t dialog_panel(component_t parent, enum panel_layout layout)
{
	component_t panel = ((panel_fn)at->panel_new)(parent, 0);
	if (!panel)
		return NULL;
	unsigned id = layout == PANEL_ROWS ? LAYOUT_ROWS : LAYOUT_BUTTONS;
	return ((layout_fn)at->set_layout)(panel, id) == LAYOUT_OK ? panel : NULL;
}

component_t dialog_label(component_t panel, const uint16_t *text)
{
	return ((label_fn)at->add_label)(panel, text, 0, 0, 0, LABEL_ALIGN, ROW_ALIGN);
}

component_t dialog_combo(component_t panel, const uint16_t *const *items)
{
	return ((combo_fn)at->add_combo)(panel, COMBO_KIND, items, COMBO_ALIGN, ROW_ALIGN, 0, 0);
}

component_t dialog_checkbox(component_t panel, const uint16_t *text)
{
	return ((checkbox_fn)at->add_checkbox)(panel, CHECKBOX_KIND, text);
}

component_t dialog_button(component_t panel, const uint16_t *text, void *context, dialog_callback callback)
{
	return ((button_fn)at->add_button)(panel, text, context, callback);
}

bool dialog_pair(component_t panel, component_t first, component_t second)
{
	return ((pair_fn)at->pair_components)(panel, first, second) == PAIR_OK;
}

void dialog_set_default_button(dialog_t dialog, component_t button)
{
	((dialog_button_fn)at->set_default_button)(dialog, button);
}

void dialog_set_escape_button(dialog_t dialog, component_t button)
{
	((dialog_button_fn)at->set_escape_button)(dialog, button);
}

void dialog_run(dialog_t dialog)
{
	((run_fn)at->run_or_close)(dialog, RUN);
}

void dialog_close(dialog_t dialog)
{
	((run_fn)at->run_or_close)(dialog, CLOSE);
}

void dialog_delete(dialog_t dialog)
{
	((unary_fn)at->dialog_delete)(dialog);
}

dialog_t dialog_of(component_t component)
{
	return ((unary_fn)at->component_dialog)(component);
}

int dialog_combo_get(component_t combo)
{
	return ((get_fn)at->combo_get)(combo);
}

void dialog_combo_set(component_t combo, int index)
{
	((set_fn)at->combo_set)(combo, index);
}

bool dialog_checkbox_get(component_t checkbox)
{
	return ((get_fn)at->checkbox_get)(checkbox) == CHECKED;
}

void dialog_checkbox_set(component_t checkbox, bool checked)
{
	((set_fn)at->checkbox_set)(checkbox, checked ? CHECKED : UNCHECKED);
}

bool dialog_is_activation(int event)
{
	return event == (int)os->event_enter || event == (int)os->event_click;
}
