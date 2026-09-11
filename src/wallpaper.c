#include <os.h>
#include <dirent.h>
#include <sys/stat.h>
#include "wallpaper.h"

#define FOLDER "ndless/wallpapers/"

static struct ti_image *volatile current;

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

bool wallpaper_apply(bool on, const char *file, enum image_scale scale, unsigned background_rgb,
                     char *error, int error_capacity)
{
	struct ti_image *old = current;
	current = NULL;
	image_free(old);
	if (!on || !file[0])
		return true;
	char path[128 + WALLPAPER_NAME_MAX];
	snprintf(path, sizeof path, "%s%s", folder_path(), file);
	if (!file_exists(path))
		return true;
	current = image_load(path, WALLPAPER_WIDTH, WALLPAPER_HEIGHT, scale, background_rgb, WALLPAPER_MAX_PIXELS,
	                     error, error_capacity);
	return current != NULL;
}

const void *wallpaper_image(void)
{
	return current;
}
