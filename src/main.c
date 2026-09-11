/* nTheme: resident theme manager for TI-Nspire CX II (Ndless).
 * Run once (or from ndless/startup): checks the calculator, hooks the OS, applies theme.cfg.tns,
 * and adds Home > 5 > Style, which opens the settings dialog. */
#include <os.h>
#include "hooks.h"
#include "patch.h"
#include "theme.h"
#include "menu.h"
#include "dialog.h"
#include "settings.h"
#include "config.h"
#include "wallpaper.h"
#include "apply.h"
#include "report.h"

static const struct os_table *os;
static struct config current;

static void open_settings(void)
{
	struct config edited = current;
	switch (settings_run(&edited)) {
	case SETTINGS_FAILED:
		report_error("The OS could not build the settings dialog (%s).", settings_failure());
		return;
	case SETTINGS_CANCELED:
		return;
	case SETTINGS_ACCEPTED:
		break;
	}
	current = edited;
	if (!config_save(&current))
		report_error("%s: could not write.", config_path());
	apply_config(&current, os);
}

/* nl_osid is an extension syscall; nl_hassyscall() cannot see those, so gate on the revision. */
#define NDLESS_REVISION_REQUIRED 2022

static const struct os_table *supported_os(void)
{
	if (nl_ndless_rev() < NDLESS_REVISION_REQUIRED) {
		report_error("nTheme needs Ndless r%u or newer (this is r%u).", NDLESS_REVISION_REQUIRED, nl_ndless_rev());
		return NULL;
	}
	unsigned osid = nl_osid();
	const struct os_table *table = os_table_lookup(osid);
	if (!table) {
		report_error("Unsupported calculator or OS (Ndless OS index %u). This build supports the CX II-T on OS 6.4.0.74.", osid);
		return NULL;
	}
	if (nl_hwsubtype() != table->hardware_subtype
	    || *(volatile unsigned *)table->fingerprint_address != table->fingerprint) {
		report_error("This is not a %s running OS %s.", table->model, table->os_version);
		return NULL;
	}
	return table;
}

static bool os_untouched(void)
{
	switch (patch_probe(os)) {
	case PATCH_RESIDENT:
		report_error("nTheme is already resident. Open Home > 5 > Style to change settings.");
		return false;
	case PATCH_UNEXPECTED:
		report_error("Another program has patched the OS where nTheme hooks it. Reboot, then run nTheme first.");
		return false;
	case PATCH_PRISTINE:
		break;
	}
	if (!theme_is_stock(os)) {
		report_error("The OS theme is not in its stock state. Reboot first.");
		return false;
	}
	return true;
}

static void load_config(void)
{
	if (config_load(&current))
		return;
	if (!config_save(&current))
		report_error("%s: could not create.", config_path());
}

int main(void)
{
	os = supported_os();
	if (!os || !os_untouched())
		return 1;
	if (!theme_snapshot(os) || !menu_install(os, open_settings)) {
		report_error("Out of memory.");
		return 1;
	}
	dialog_bind(os);
	patch_install(os);
	wallpaper_create_folder();
	load_config();
	apply_config(&current, os);
	if (!nl_isstartup()) {
		open_settings();
		report_info("nTheme stays active until the next reboot. Copy nTheme.tns into ndless/startup to run it with every Ndless install.");
	}
	nl_set_resident();
	_exit(0);
}
