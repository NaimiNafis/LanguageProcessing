#include <stdarg.h> 
#include "scan.h"
#include "parser.h"
#include "cross_referencer.h"
#include "debug.h" 

int main(int argc, char *argv[]) {
    // Validate input and handle debug mode
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./pp <filename.mpl> [--debug]\n");
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = 1;
    }

    // Initialize components
    if (init_scan(argv[1]) < 0) return 1;
    init_parser();
    init_cross_referencer();

    // Execute parsing
    int parse_result = parse_program();
    end_scan();

    // Handle results
    if (parse_result != 0) return 1;
    print_cross_reference_table();

    return 0;
}