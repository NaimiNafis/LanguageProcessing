#include <stdarg.h> 
#include "scan.h"
#include "parser.h"
// #include "pretty.h"
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
    debug_printf("Initializing pretty printer...\n");
    init_pretty_printer();

    debug_printf("Starting parsing...\n");
    int parse_result = parse_program();
    debug_printf("Parsing completed with result: %d\n", parse_result);
    end_scan();

    // Always attempt pretty printing, even if parsing found errors
    debug_printf("Reinitializing scanner for pretty printing...\n");
    if (init_scan(argv[1]) < 0) {
        return 1;
    }
    print_cross_reference_table();
    
    // debug_printf("Starting pretty printing...\n");
    // pretty_print_program();
    // debug_printf("Cleaning up scanner after pretty printing...\n");
    end_scan();

    // Return the parse result (0 for success, line number for error)
    return parse_result;
}