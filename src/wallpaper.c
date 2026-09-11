#include <os.h>
#include <dirent.h>
#include <sys/stat.h>
#include "wallpaper.h"

#define FOLDER "ndless/wallpapers/"
#define ERROR_MAX 160
/* Paints the deferred decode may fail on for lack of memory before giving up; memory is back to normal
 * by the time the home screen paints, so the first try normally succeeds. */
#define DEFERRED_TRIES 3

struct request {
	char file[WALLPAPER_NAME_MAX];
	enum image_scale scale;
	unsigned background_rgb;
	int tries_left;   /* > 0: waiting for a home screen paint */
};

static struct ti_image *volatile current;
static struct request pending;
static char deferred_error[ERROR_MAX];

static const char *folder_path(void)
{
	static char path[128];
	snprintf(path, sizeof path, "%s" FOLDER, get_documents_dir());
	return path;
}

void wallpaper_create_folder(void)
{
	char path[128];
	snprintf(path, sizeof path, "%s", folder_path());
	path[strlen(path) - 1] = 0;   /* no trailing slash */
	mkdir(path, 0755);            /* already existing is fine */
}

static bool ends_with_ignoring_case(const char *name, const char *suffix)
{
	size_t n = strlen(name), s = strlen(suffix);
	return n >= s && strcasecmp(name + n - s, suffix) == 0;
}

/* Any .tns in the folder: the decoder recognizes PNG and JPEG by content, not by name. */
static bool is_wallpaper_file(const char *name)
{
	return ends_with_ignoring_case(name, ".tns");
}

static int compare_names(const void *a, const void *b)
{
	return strcmp(a, b);
}

int wallpaper_list(char names[][WALLPAPER_NAME_MAX], int max)
{
	DIR *directory = opendir(folder_path());
	if (!directory)
		return 0;
	int count = 0;
	struct dirent *entry;
	while (count < max && (entry = readdir(directory))) {
		if (!is_wallpaper_file(entry->d_name) || strlen(entry->d_name) >= WALLPAPER_NAME_MAX)
			continue;
		strcpy(names[count++], entry->d_name);
	}
	closedir(directory);
	qsort(names, count, WALLPAPER_NAME_MAX, compare_names);
	return count;
}

static bool file_exists(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file)
		return false;
	fclose(file);
	return true;
}

/* Decodes into `current`. A missing file counts as success with no image. */
static bool load(const struct request *request, bool *no_memory, char *error, int error_capacity)
{
	*no_memory = false;
	char path[128 + WALLPAPER_NAME_MAX];
	snprintf(path, sizeof path, "%s%s", folder_path(), request->file);
	if (!file_exists(path))
		return true;
	current = image_load(path, WALLPAPER_WIDTH, WALLPAPER_HEIGHT, request->scale, request->background_rgb,
	                     WALLPAPER_MAX_PIXELS, no_memory, error, error_capacity);
	return current != NULL;
}

bool wallpaper_apply(bool on, const char *file, enum image_scale scale, unsigned background_rgb, bool defer,
                     char *error, int error_capacity)
{
	struct ti_image *old = current;
	current = NULL;
	image_free(old);
	pending.tries_left = 0;
	deferred_error[0] = 0;
	if (!on || !file[0])
		return true;
	struct request request = { .scale = scale, .background_rgb = background_rgb };
	snprintf(request.file, sizeof request.file, "%s", file);
	if (defer) {
		request.tries_left = DEFERRED_TRIES;
		pending = request;
		return true;
	}
	bool no_memory;
	return load(&request, &no_memory, error, error_capacity);
}

const char *wallpaper_take_deferred_error(void)
{
	static char taken[ERROR_MAX];
	if (!deferred_error[0])
		return NULL;
	strcpy(taken, deferred_error);
	deferred_error[0] = 0;
	return taken;
}

const void *wallpaper_image(void)
{
	if (pending.tries_left > 0) {
		bool no_memory;
		char error[ERROR_MAX];
		if (load(&pending, &no_memory, error, sizeof error))
			pending.tries_left = 0;
		else if (!no_memory || --pending.tries_left == 0) {
			pending.tries_left = 0;
			snprintf(deferred_error, sizeof deferred_error, "%s", error);
		}
	}
	return current;
}
