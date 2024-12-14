#include <stdarg.h> 
#include "scan.h"
#include "parser.h"
#include "cross_referencer.h"
#include "debug.h" 

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./pp <filename.mpl> [--debug]\n");
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = 1;
    }

    debug_printf("Initializing scanner...\n");
    if (init_scan(argv[1]) < 0) {
        return 1;
    }

    debug_printf("Initializing parser...\n");
    init_parser();

    debug_printf("Starting parsing...\n");
    int parse_result = parse_program();
    debug_printf("Parsing completed with result: %d\n", parse_result);
    end_scan();

    if (parse_result != 0) {
        // If parsing failed, exit with an error code
        return 1;
    }

    debug_printf("Reinitializing scanner for cross-referencing...\n");
    if (init_scan(argv[1]) < 0) {
        return 1;
    }
    print_cross_reference_table();
    end_scan();

    return 0;  // Successful execution
}