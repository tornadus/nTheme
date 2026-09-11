/* Translucent home-screen strips: theme entries set to a sentinel color draw with the OS's 50% blend. */
#ifndef NTHEME_GLASS_H
#define NTHEME_GLASS_H

#include <stdbool.h>
#include <ngc.h>
#include "hooks.h"

/* Post-pass over the table; call after theme_apply(). Off leaves the table as theme_apply() wrote it. */
void glass_apply(const struct os_table *os, bool on);

/* Hook side: set the blend mode for the color about to be used. */
void glass_on_color(Gc gc, unsigned rgb);

#endif
