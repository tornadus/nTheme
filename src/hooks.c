/* One row per supported calculator/OS pair. Add rows from utils/hookfinder.py output; never edit addresses elsewhere. */
#include <stddef.h>
#include "hooks.h"

/* Dark mode keeps: title bars (3, 5, 133..135), doc browser bars (87, 223, 226, 231), white text on the accent. */
static const unsigned short keep_6_4_cx2t[] = {
	3, 5, 133, 134, 135, 87, 223, 226, 231,
	64, 86, 88, 124, 185, 191, 221, 238, 298, 328, 345, 361
};
static const unsigned short dim_6_4_cx2t[] = { 127, 129, 147, 375 };
static const unsigned short accent_text_6_4_cx2t[] = {
	104, 64, 86, 88, 124, 185, 191, 221, 238, 298, 328, 345, 361
};

static const struct os_table tables[] = {
	{
		.osid = 48, .hardware_subtype = 2,
		.fingerprint_address = 0x10000020, .fingerprint = 0x1042a600,
		.model = "CX II-T", .os_version = "6.4.0.74",
		.color_table = 0x10b7a660, .color_count = 454,
		.editor_config = { 0x10bad8c8, 0x10bad9d4, 0x10badae0 },
		.plot_palette = 0x10b72118,
		.menu_literal = 0x10a38188, .menu_base = 0x10c3b4a0, .menu_context_count = 9,
		.menu_label_string = 0x3f8, /* "Style" */
		.event_enter = 0x14c0b, .event_click = 0x14c0d,
		.set_color_driver = { 0x100291bc, 0xE92D4038, 0xE590C030 }, /* push {r3,r4,r5,lr}; ldr ip,[r0,#0x30] */
		.set_color_wrapper_return = 0x1004d260,
		.home_background = { 0x10315a28, 0xE3A0C004, 0xE1A00004 },  /* mov ip,#4; mov r0,r4 */
		.home_title = { 0x103159f0, 0xE280300B, 0xE1A00004 },       /* add r3,r0,#0xb; mov r0,r4 */
		.home_title_font = 0x10b,
		.os_code_begin = 0x10000000, .os_code_end = 0x10C00000,
		.ids = {
			.home_background = 99, .home_section_title = 101,
			.home_list_background = 103, .home_list_text = 105,
			.home_doc_name_background = 109, .home_doc_name_text = 110,
			.keep_first = 94, .keep_last = 110,
			.keep = keep_6_4_cx2t, .keep_count = sizeof keep_6_4_cx2t / sizeof keep_6_4_cx2t[0],
			.dim = dim_6_4_cx2t, .dim_count = sizeof dim_6_4_cx2t / sizeof dim_6_4_cx2t[0],
			.accent_text = accent_text_6_4_cx2t,
			.accent_text_count = sizeof accent_text_6_4_cx2t / sizeof accent_text_6_4_cx2t[0],
			.stock_accent = 0x2478CF, .plot_grid_index = 9,
		},
		.dialog = {
			.dialog_new = 0x1025e374, .dialog_content = 0x1025e2b4,
			.panel_new = 0x1025e2b0, .set_layout = 0x1025e458,
			.add_label = 0x1025e370, .add_combo = 0x1025e3c4, .add_checkbox = 0x1025e404,
			.pair_components = 0x1025e498, .add_button = 0x10070524,
			.set_default_button = 0x1025e47c, .set_escape_button = 0x1025e46c,
			.run_or_close = 0x1025e450, .component_dialog = 0x1025e38c, .dialog_delete = 0x1025e49c,
			.combo_get = 0x1025e440, .combo_set = 0x1025e48c,
			.checkbox_get = 0x1025e4a8, .checkbox_set = 0x1025e1ec,
		},
	},
};

const struct os_table *os_table_lookup(unsigned osid)
{
	for (unsigned i = 0; i < sizeof tables / sizeof tables[0]; i++)
		if (tables[i].osid == osid)
			return &tables[i];
	return NULL;
}
