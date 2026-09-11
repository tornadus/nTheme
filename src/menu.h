/* Adds an nTheme entry to the Home > 5 (Settings & Status) menu. */
#ifndef NTHEME_MENU_H
#define NTHEME_MENU_H

#include <stdbool.h>
#include "hooks.h"

/* False = out of memory. */
bool menu_install(const struct os_table *os, void (*on_select)(void));

#endif
