/* Typed binding to the OS dialog manager, used the way the OS builds its own Handheld Setup dialog. */
#ifndef NTHEME_DIALOG_H
#define NTHEME_DIALOG_H

#include <stdbool.h>
#include <stdint.h>
#include "hooks.h"

typedef void *dialog_t;
typedef void *component_t;
typedef void (*dialog_callback)(component_t source, void *context, int event);

enum panel_layout { PANEL_ROWS, PANEL_BUTTONS };

void dialog_bind(const struct os_table *os);

/* Creation. Every call returns NULL on failure. */
dialog_t dialog_new(const uint16_t *title);
component_t dialog_content(dialog_t dialog);
component_t dialog_panel(component_t parent, enum panel_layout layout);
component_t dialog_label(component_t panel, const uint16_t *text);
component_t dialog_combo(component_t panel, const uint16_t *const *items);   /* NULL-terminated */
component_t dialog_checkbox(component_t panel, const uint16_t *text);
component_t dialog_button(component_t panel, const uint16_t *text, void *context, dialog_callback callback);
/* The OS pairs each label with its combo and OK with Cancel; a layout hint, kept exactly as observed. */
bool dialog_pair(component_t panel, component_t first, component_t second);
void dialog_set_default_button(dialog_t dialog, component_t button);
void dialog_set_escape_button(dialog_t dialog, component_t button);

/* Lifetime. */
void dialog_run(dialog_t dialog);   /* modal; returns after dialog_close() */
void dialog_close(dialog_t dialog);
void dialog_delete(dialog_t dialog);
dialog_t dialog_of(component_t component);

/* Values. */
int dialog_combo_get(component_t combo);
void dialog_combo_set(component_t combo, int index);
bool dialog_checkbox_get(component_t checkbox);
void dialog_checkbox_set(component_t checkbox, bool checked);

/* Callbacks fire for several events; act only on these. */
bool dialog_is_activation(int event);

#endif
