#include "scan.h"
#include "parser.h"
#include "pretty.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./pp <filename.mpl>\n");
        return 1;
    }

    printf("Initializing scanner...\n");
    if (init_scan(argv[1]) < 0) {
        return 1;
    }

    printf("Initializing parser...\n");
    init_parser();
    printf("Initializing pretty printer...\n");
    init_pretty_printer();

    printf("Starting parsing...\n");
    int parse_result = parse_program();
    if (parse_result == 0) {  // Check for successful parsing
        printf("Parsing successful. Cleaning up scanner...\n");
        end_scan();
        
        // Reinitialize scanner for pretty printing
        printf("Reinitializing scanner for pretty printing...\n");
        if (init_scan(argv[1]) < 0) {
            return 1;
        }
        printf("Starting pretty printing...\n");
        pretty_print_program();
        printf("Cleaning up scanner after pretty printing...\n");
        end_scan();
        return 0;
    }

    printf("Parsing failed. Cleaning up scanner...\n");
    end_scan();
    return 1;
}