/* theme.cfg.tns: key=value lines, # comments, hand-editable. */
#ifndef NTHEME_CONFIG_H
#define NTHEME_CONFIG_H

#include <stdbool.h>
#include "theme.h"
#include "image.h"
#include "title.h"
#include "wallpaper.h"

struct config {
	enum theme_mode theme;
	int accent;                                /* index into ACCENT_RGB */
	bool wallpaper;
	char wallpaper_file[WALLPAPER_NAME_MAX];
	enum image_scale wallpaper_scale;
	bool glass;
	enum title_mode title_mode;                /* only ever set by hand in the file */
	char title[TITLE_MAX + 1];
};

const char *config_path(void);
void config_defaults(struct config *config);

/* False = no readable file; `config` holds the defaults then. */
bool config_load(struct config *config);

/* Rewrites known keys in place and keeps every other line (comments, the title line) as it was. */
bool config_save(const struct config *config);

#endif
