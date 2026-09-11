/* Code hooks: probe the sites, install once. */
#ifndef NTHEME_PATCH_H
#define NTHEME_PATCH_H

#include "hooks.h"

enum patch_state {
	PATCH_PRISTINE,    /* every site holds its expected code */
	PATCH_RESIDENT,    /* every site holds an nTheme hook already */
	PATCH_UNEXPECTED   /* something else patched the OS; do not touch it */
};

enum patch_state patch_probe(const struct os_table *os);

/* Only after patch_probe() returned PATCH_PRISTINE. */
void patch_install(const struct os_table *os);

#endif
