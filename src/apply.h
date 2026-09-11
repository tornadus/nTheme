/* Push a configuration into the OS, in the order the pieces depend on each other. */
#ifndef NTHEME_APPLY_H
#define NTHEME_APPLY_H

#include <stdbool.h>
#include "config.h"
#include "hooks.h"

/* Reports any failure to the user itself. `defer_wallpaper` = decode the wallpaper at the first home screen
 * paint rather than now (see wallpaper_apply). */
void apply_config(const struct config *config, const struct os_table *os, bool defer_wallpaper);

#endif
