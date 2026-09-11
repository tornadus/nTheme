#include <os.h>
#include <hook.h>
#include "patch.h"
#include "theme.h"
#include "glass.h"
#include "wallpaper.h"
#include "title.h"

#define TRAMPOLINE 0xE51FF004   /* ldr pc,[pc,#-4]: what an installed Ndless hook reads back as */
#define SCREEN_WIDTH 320

/* HOOK_SAVED_REGS layout: r0..r12, lr, then the hooked function's own stack. */
#define REG_LR 13
#define STACK_WORD(n) (14 + (n))
#define GC_REGISTER 4   /* both home painter sites keep the gc in r4 (verified osid 48) */

static const struct os_table *os;   /* hooks take no arguments */

static unsigned pack_rgb(const unsigned *regs)
{
	return ((regs[1] & 0xff) << 16) | ((regs[2] & 0xff) << 8) | (regs[3] & 0xff);
}

static void unpack_rgb(unsigned *regs, unsigned rgb)
{
	regs[1] = rgb >> 16;
	regs[2] = (rgb >> 8) & 0xff;
	regs[3] = rgb & 0xff;
}

/* The gui_gc_setColorRGB wrapper pushes {r4,lr} before dispatching to the driver,
 * so its caller sits one word up the stack; a direct vtable call has the caller in lr. */
static bool called_from_os(const unsigned *regs)
{
	unsigned caller = regs[REG_LR] == os->set_color_wrapper_return ? regs[STACK_WORD(1)] : regs[REG_LR];
	return caller >= os->os_code_begin && caller < os->os_code_end;
}

/* Driver setColorRGB(gc, r, g, b). Ndless programs drawing through the OS keep their colors. */
HOOK_DEFINE(set_color_hook) {
	unsigned *regs = HOOK_SAVED_REGS(set_color_hook);
	if (called_from_os(regs)) {
		unsigned rgb = theme_remap(pack_rgb(regs));
		glass_on_color((Gc)regs[0], rgb);
		unpack_rgb(regs, rgb);
	}
	HOOK_RESTORE_RETURN(set_color_hook);
}

/* Home painter, right after it filled the background. */
HOOK_DEFINE(home_background_hook) {
	const void *image = wallpaper_image();
	if (image)
		gui_gc_drawImage((Gc)HOOK_SAVED_REGS(home_background_hook)[GC_REGISTER], (char *)image, 0, WALLPAPER_TOP);
	HOOK_RESTORE_RETURN(home_background_hook);
}

/* Home painter, right before it draws the title text: r1 = utf16, r2 = x. */
HOOK_DEFINE(home_title_hook) {
	unsigned *regs = HOOK_SAVED_REGS(home_title_hook);
	int length;
	const uint16_t *text = title_text(&length);
	if (text) {
		Gc gc = (Gc)regs[GC_REGISTER];
		int width = gui_gc_getStringWidth(gc, os->home_title_font, (char *)text, 0, length);
		regs[1] = (unsigned)text;
		regs[2] = (unsigned)((SCREEN_WIDTH - width) / 2);
	}
	HOOK_RESTORE_RETURN(home_title_hook);
}

static int sites(const struct os_table *t, const struct hook_site *out[])
{
	out[0] = &t->set_color_driver;
	out[1] = &t->home_background;
	out[2] = &t->home_title;
	return 3;
}

enum patch_state patch_probe(const struct os_table *t)
{
	const struct hook_site *site[3];
	int count = sites(t, site), pristine = 0, resident = 0;
	for (int i = 0; i < count; i++) {
		const unsigned *code = (const unsigned *)site[i]->address;
		if (code[0] == TRAMPOLINE)
			resident++;
		else if (code[0] == site[i]->word0 && code[1] == site[i]->word1)
			pristine++;
	}
	if (*(unsigned *)t->menu_literal == t->menu_base)
		pristine++;
	else
		resident++;
	count++;
	if (pristine == count)
		return PATCH_PRISTINE;
	if (resident == count)
		return PATCH_RESIDENT;
	return PATCH_UNEXPECTED;
}

void patch_install(const struct os_table *t)
{
	os = t;
	HOOK_INSTALL(t->set_color_driver.address, set_color_hook);
	HOOK_INSTALL(t->home_background.address, home_background_hook);
	HOOK_INSTALL(t->home_title.address, home_title_hook);
}
