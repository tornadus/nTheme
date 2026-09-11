/* Per-calculator/OS address table. Every OS-specific number nTheme touches lives in hooks.c. */
#ifndef NTHEME_HOOKS_H
#define NTHEME_HOOKS_H

/* Eight bytes a hook overwrites, and the two instructions expected there before patching. */
struct hook_site {
	unsigned address;
	unsigned word0, word1;
};

/* Theme color table facts: indices and the lists dark mode needs. */
struct theme_ids {
	unsigned home_background;          /* HomeScreenBackground */
	unsigned home_section_title;       /* white section titles; glass copies this onto list text */
	unsigned home_list_background;     /* HomeScreenUnselectedOptionBackground */
	unsigned home_list_text;           /* HomeScreenTextWithoutFocusEnabled */
	unsigned home_doc_name_background; /* HomeScreenAppDocNameBackground */
	unsigned home_doc_name_text;       /* HomeScreenAppDocNameForeground */
	unsigned keep_first, keep_last;    /* inclusive index range left alone by dark mode */
	const unsigned short *keep;        /* single entries left alone by dark mode */
	unsigned keep_count;
	const unsigned short *dim;         /* light saturated backgrounds dark mode dims instead of flips */
	unsigned dim_count;
	const unsigned short *accent_text; /* white text drawn on the accent color */
	unsigned accent_text_count;
	unsigned stock_accent;             /* the color every accent entry has in a stock table */
	unsigned plot_grid_index;          /* graph palette entry holding the grid color */
};

/* OS dialog manager entry points, called exactly the way the OS's own Handheld Setup dialog calls them. */
struct dialog_addresses {
	unsigned dialog_new;          /* (parent, utf16 title, options) */
	unsigned dialog_content;      /* (dialog) -> content panel */
	unsigned panel_new;           /* (parent, 0) */
	unsigned set_layout;          /* (panel, layout id) */
	unsigned add_label;           /* (panel, utf16, 0, 0, 0, align, align) */
	unsigned add_combo;           /* (panel, kind, utf16 *items, align, align, 0, 0) */
	unsigned add_checkbox;        /* (panel, kind, utf16) */
	unsigned pair_components;     /* (panel, first, second): label with its combo, OK with Cancel */
	unsigned add_button;          /* (panel, utf16, context, callback) */
	unsigned set_default_button;  /* (dialog, button) */
	unsigned set_escape_button;   /* (dialog, button) */
	unsigned run_or_close;        /* (dialog, 1 = run modal, 0 = close) */
	unsigned component_dialog;    /* (component) -> dialog */
	unsigned dialog_delete;       /* (dialog) */
	unsigned combo_get;           /* (combo) -> index */
	unsigned combo_set;           /* (combo, index) */
	unsigned checkbox_get;        /* (checkbox) -> state id */
	unsigned checkbox_set;        /* (checkbox, state id) */
};

struct os_table {
	unsigned osid;                 /* Ndless OS index, nl_osid() */
	unsigned hardware_subtype;     /* nl_hwsubtype(): 2 = CX II family */
	unsigned fingerprint_address;  /* word Ndless itself identifies the OS by */
	unsigned fingerprint;
	const char *model;
	const char *os_version;

	unsigned color_table;          /* u32 0x00RRGGBB entries */
	unsigned color_count;
	unsigned editor_config[3];     /* static 2D math editor default structs */
	unsigned plot_palette;         /* graph palette bank */

	unsigned menu_literal;         /* literal-pool word holding the Home > 5 menu table pointer */
	unsigned menu_base;            /* its stock value */
	unsigned menu_context_count;   /* pointers before the first record */
	unsigned menu_label_string;    /* syst string id of the nTheme entry */
	unsigned event_enter, event_click;   /* activation events for menu and dialog callbacks */

	struct hook_site set_color_driver;   /* driver setColorRGB: every OS color set passes here */
	unsigned set_color_wrapper_return;   /* return address inside the gui_gc_setColorRGB wrapper */
	struct hook_site home_background;    /* home painter, right after its background fill; r4 = gc */
	struct hook_site home_title;         /* home painter, right before its title drawString; r1 = text, r2 = x, r4 = gc */
	unsigned home_title_font;
	unsigned os_code_begin, os_code_end; /* colors set by callers outside this range are never touched */

	struct theme_ids ids;
	struct dialog_addresses dialog;
};

const struct os_table *os_table_lookup(unsigned osid);

/* Ndless syscall missing from the SDK headers. */
unsigned nl_osid(void);

#endif
