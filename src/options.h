/* User-visible choices: the config spelling, the dialog label, and what they mean. */
#ifndef NTHEME_OPTIONS_H
#define NTHEME_OPTIONS_H

struct option_set {
	const char *const *names;    /* as written in theme.cfg */
	const char *const *labels;   /* as shown in the dialog */
	int count;
};

extern const struct option_set OPTIONS_THEME;    /* index = enum theme_mode */
extern const struct option_set OPTIONS_ACCENT;   /* index into ACCENT_RGB */
extern const struct option_set OPTIONS_SCALE;    /* index = enum image_scale */
extern const struct option_set OPTIONS_SWITCH;   /* off, on */

extern const unsigned ACCENT_RGB[];
#define ACCENT_DEFAULT 3   /* blue, the stock color */

/* -1 when unknown. */
int option_index(const struct option_set *set, const char *name);

#endif
