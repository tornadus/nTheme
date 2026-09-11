/* The nTheme settings page, a native OS dialog. */
#ifndef NTHEME_SETTINGS_H
#define NTHEME_SETTINGS_H

#include "config.h"

enum settings_result { SETTINGS_ACCEPTED, SETTINGS_CANCELED, SETTINGS_FAILED };

/* Modal. On SETTINGS_ACCEPTED, `config` holds the new choices. */
enum settings_result settings_run(struct config *config);

/* After SETTINGS_FAILED: the dialog step that failed. */
const char *settings_failure(void);

#endif
