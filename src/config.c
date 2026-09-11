#include <os.h>
#include "config.h"
#include "options.h"
#include "version.h"

#define FILE_MAX 4096
#define CONFIG_LINE_MAX 128

static const char *const keys[] = { "theme", "accent", "wallpaper", "wallpaper_file", "wallpaper_scale", "glass" };
#define KEY_COUNT ((int)(sizeof keys / sizeof keys[0]))

static const char template[] =
	"# nTheme " NTHEME_VERSION ". Edit by hand or use Home > 5 > Style.\n"
	"theme=light\n"
	"accent=blue\n"
	"wallpaper=off\n"
	"wallpaper_file=\n"
	"wallpaper_scale=fit\n"
	"glass=off\n"
	"# Home screen title: remove the # for a custom title; leave it empty for no title.\n"
	"# title=\n";

const char *config_path(void)
{
	static char path[128];
	snprintf(path, sizeof path, "%sndless/theme.cfg.tns", get_documents_dir());
	return path;
}

void config_defaults(struct config *config)
{
	memset(config, 0, sizeof *config);
	config->theme = THEME_LIGHT;
	config->accent = ACCENT_DEFAULT;
	config->wallpaper_scale = IMAGE_FIT;
	config->title_mode = TITLE_DEFAULT;
}

static char *trim(char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	char *end = s + strlen(s);
	while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
		*--end = 0;
	return s;
}

/* Splits "key=value" in place. False for comments, blank lines and lines without '='. */
static bool split(char *line, char **key, char **value)
{
	char *equals = strchr(line, '=');
	line = trim(line);
	if (!*line || *line == '#' || !equals)
		return false;
	*equals = 0;
	*key = trim(line);
	*value = trim(equals + 1);
	return true;
}

static void set_value(struct config *config, const char *key, const char *value)
{
	int index;
	if (strcmp(key, "theme") == 0 && (index = option_index(&OPTIONS_THEME, value)) >= 0)
		config->theme = index;
	else if (strcmp(key, "accent") == 0 && (index = option_index(&OPTIONS_ACCENT, value)) >= 0)
		config->accent = index;
	else if (strcmp(key, "wallpaper") == 0 && (index = option_index(&OPTIONS_SWITCH, value)) >= 0)
		config->wallpaper = index;
	else if (strcmp(key, "wallpaper_file") == 0)
		snprintf(config->wallpaper_file, sizeof config->wallpaper_file, "%s", value);
	else if (strcmp(key, "wallpaper_scale") == 0 && (index = option_index(&OPTIONS_SCALE, value)) >= 0)
		config->wallpaper_scale = index;
	else if (strcmp(key, "glass") == 0 && (index = option_index(&OPTIONS_SWITCH, value)) >= 0)
		config->glass = index;
	else if (strcmp(key, "title") == 0) {
		config->title_mode = *value ? TITLE_TEXT : TITLE_BLANK;
		snprintf(config->title, sizeof config->title, "%s", value);
	}
}

static const char *value_of(const struct config *config, const char *key)
{
	if (strcmp(key, "theme") == 0)
		return OPTIONS_THEME.names[config->theme];
	if (strcmp(key, "accent") == 0)
		return OPTIONS_ACCENT.names[config->accent];
	if (strcmp(key, "wallpaper") == 0)
		return OPTIONS_SWITCH.names[config->wallpaper];
	if (strcmp(key, "wallpaper_file") == 0)
		return config->wallpaper_file;
	if (strcmp(key, "wallpaper_scale") == 0)
		return OPTIONS_SCALE.names[config->wallpaper_scale];
	if (strcmp(key, "glass") == 0)
		return OPTIONS_SWITCH.names[config->glass];
	return NULL;
}

/* Whole file as a string, or NULL. */
static char *read_file(char *buffer, int capacity)
{
	FILE *file = fopen(config_path(), "rb");
	if (!file)
		return NULL;
	int n = fread(buffer, 1, capacity - 1, file);
	fclose(file);
	if (n < 0)
		return NULL;
	buffer[n] = 0;
	return buffer;
}

/* Next line of `text`, copied and NUL-terminated (without its newline); NULL at the end. */
static const char *next_line(const char *text, char *line, int capacity)
{
	if (!*text)
		return NULL;
	const char *end = strchr(text, '\n');
	int length = end ? end - text : (int)strlen(text);
	if (length >= capacity)
		length = capacity - 1;
	memcpy(line, text, length);
	line[length] = 0;
	return end ? end + 1 : text + strlen(text);
}

bool config_load(struct config *config)
{
	static char buffer[FILE_MAX];
	config_defaults(config);
	const char *text = read_file(buffer, sizeof buffer);
	if (!text)
		return false;
	char line[CONFIG_LINE_MAX], *key, *value;
	while ((text = next_line(text, line, sizeof line)))
		if (split(line, &key, &value))
			set_value(config, key, value);
	return true;
}

struct writer {
	char *out;
	int capacity, length;
	bool written[KEY_COUNT];
};

static void emit(struct writer *w, const char *text)
{
	int n = snprintf(w->out + w->length, w->capacity - w->length, "%s", text);
	if (n > 0)
		w->length += n < w->capacity - w->length ? n : w->capacity - w->length - 1;
}

static void emit_key(struct writer *w, const struct config *config, int key)
{
	char line[CONFIG_LINE_MAX];
	snprintf(line, sizeof line, "%s=%s\n", keys[key], value_of(config, keys[key]));
	emit(w, line);
	w->written[key] = true;
}

static int key_index(const char *key)
{
	for (int i = 0; i < KEY_COUNT; i++)
		if (strcmp(keys[i], key) == 0)
			return i;
	return -1;
}

/* Copies `old` line by line, replacing the value of each known key the first time it appears. */
static void rewrite(struct writer *w, const struct config *config, const char *old)
{
	char line[CONFIG_LINE_MAX], parsed[CONFIG_LINE_MAX], *key, *value;
	while ((old = next_line(old, line, sizeof line))) {
		strcpy(parsed, line);
		int index = split(parsed, &key, &value) ? key_index(key) : -1;
		if (index >= 0 && !w->written[index]) {
			emit_key(w, config, index);
		} else {
			emit(w, line);
			emit(w, "\n");
		}
	}
	for (int i = 0; i < KEY_COUNT; i++)
		if (!w->written[i])
			emit_key(w, config, i);
}

bool config_save(const struct config *config)
{
	static char old[FILE_MAX], fresh[FILE_MAX + CONFIG_LINE_MAX * KEY_COUNT];
	struct writer w = { fresh, sizeof fresh, 0, { false } };
	rewrite(&w, config, read_file(old, sizeof old) ? old : template);
	FILE *file = fopen(config_path(), "wb");
	if (!file)
		return false;
	bool ok = fwrite(fresh, 1, w.length, file) == (size_t)w.length;
	fclose(file);
	return ok;
}
