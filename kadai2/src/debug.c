#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

int debug_mode = 0;  // 0 off 1 on

void debug_printf(const char *format, ...) {
    if (debug_mode) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}