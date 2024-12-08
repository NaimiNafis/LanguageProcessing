#include <stdio.h>
#include <stdarg.h>
#include "debug_pretty.h"

int debug_mode_pretty = 0;  // 0 off 1 on

void debug_pretty_printf(const char *format, ...) {
    if (debug_mode_pretty) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}