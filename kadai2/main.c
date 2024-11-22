#include "scan.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Missing input file.\n");
        return EXIT_FAILURE;
    }

    if (init_scan(argv[1]) < 0) {
        fprintf(stderr, "Error: Cannot open input file.\n");
        return EXIT_FAILURE;
    }

    token = scan();  // Initialize the first token

    if (parse_program() == ERROR) {
        fprintf(stderr, "Parsing failed.\n");
        end_scan();
        return EXIT_FAILURE;
    }

    end_scan();  // Clean up resources
    return EXIT_SUCCESS;
}
