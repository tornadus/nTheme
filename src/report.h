/* The only place nTheme talks to the user. */
#ifndef NTHEME_REPORT_H
#define NTHEME_REPORT_H

void report_error(const char *format, ...);
void report_info(const char *format, ...);

#endif
