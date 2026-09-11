#include "apply.h"
#include "theme.h"
#include "glass.h"
#include "wallpaper.h"
#include "title.h"
#include "options.h"
#include "report.h"

void apply_config(const struct config *config, const struct os_table *os)
{
	theme_apply(config->theme, ACCENT_RGB[config->accent]);
	glass_apply(os, config->glass);   /* after the theme: dark mode must not flip the glass sentinel */
	char error[160];
	if (!wallpaper_apply(config->wallpaper, config->wallpaper_file, config->wallpaper_scale,
	                     theme_color(os->ids.home_background), error, sizeof error))
		report_error("%s", error);
	title_set(config->title_mode, config->title);
}
