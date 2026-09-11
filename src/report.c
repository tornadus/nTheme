#include <os.h>
#include <stdarg.h>
#include "report.h"
#include "version.h"

static void show(const char *title, const char *format, va_list arguments)
{
	char message[240];
	vsnprintf(message, sizeof message, format, arguments);
	show_msgbox(title, message);
}

void report_error(const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	show("nTheme " NTHEME_VERSION " - error", format, arguments);
	va_end(arguments);
}

void report_info(const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	show("nTheme " NTHEME_VERSION, format, arguments);
	va_end(arguments);
}
