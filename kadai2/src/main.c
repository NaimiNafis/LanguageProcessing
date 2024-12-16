#include <stdarg.h> 
#include "scan.h"
#include "parser.h"
#include "pretty.h"
#include "debug.h" 

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./pp <filename.mpl> [--debug]\n");
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = 1;
    }

    if (init_scan(argv[1]) < 0) {
        return 1;
    }

    pretty_print_program();  // This now handles both parsing and printing
    end_scan();

    return 0;
}