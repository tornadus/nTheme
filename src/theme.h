/* The OS theme color table: stock snapshot and the light/dark/accent pipeline. */
#ifndef NTHEME_THEME_H
#define NTHEME_THEME_H

#include <stdbool.h>
#include "hooks.h"

enum theme_mode { THEME_LIGHT, THEME_DARK };   /* order matches OPTIONS_THEME */

/* True while nothing has modified the table or the editor defaults. */
bool theme_is_stock(const struct os_table *os);

/* Copy the stock state; every later theme_apply() derives from it. False = out of memory. */
bool theme_snapshot(const struct os_table *os);

void theme_apply(enum theme_mode mode, unsigned accent_rgb);

/* Current table value. */
unsigned theme_color(unsigned id);

/* Hook side: rewrite a hard-coded OS color when dark mode is on. */
unsigned theme_remap(unsigned rgb);

#endif
