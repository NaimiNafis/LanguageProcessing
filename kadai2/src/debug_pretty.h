#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

extern int debug_mode;

void debug_pretty_printf(const char *format, ...);

#endif // DEBUG_H