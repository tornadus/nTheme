/* Push a configuration into the OS, in the order the pieces depend on each other. */
#ifndef NTHEME_APPLY_H
#define NTHEME_APPLY_H

#include "config.h"
#include "hooks.h"

/* Reports any failure to the user itself. */
void apply_config(const struct config *config, const struct os_table *os);

#endif
