#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "scan.h"
#include "parser.h"
#include "cross_referencer.h"
#include "compiler.h"
#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

int main(int argc, char *argv[]) {
    // Validate input and handle debug mode
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./mpplc <filename.mpl> [--debug]\n");
        return 1;
    }

    // Convert input filename to its absolute path
    char fullpath[PATH_MAX];
    // Get absolute path even if input is already absolute
    if (argv[1][0] == '/') {
        // Input is already absolute path
        strncpy(fullpath, argv[1], PATH_MAX - 1);
        fullpath[PATH_MAX - 1] = '\0';
    } else {
        // Convert relative path to absolute path
        if (!realpath(argv[1], fullpath)) {
            fprintf(stderr, "Error: Invalid path %s\n", argv[1]);
            return 1;
        }
    }

    // Check file extension
    char *dot = strrchr(fullpath, '.');
    if (!dot || strcmp(dot, ".mpl") != 0) {
        fprintf(stderr, "Error: Input file must have .mpl extension\n");
        return 1;
    }

    // Create output filename (replace .mpl with .csl)
    char outfile[MAXSTRSIZE];
    strncpy(outfile, fullpath, dot - fullpath);
    outfile[dot - fullpath] = '\0';
    strcat(outfile, ".csl");

    // Open output file
    caslfp = fopen(outfile, "w");
    if (!caslfp) {
        fprintf(stderr, "Error: Cannot create output file %s\n", outfile);
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = 1;
    }

    // Initialize components
    if (init_scan(argv[1]) < 0) {
        fclose(caslfp);
        return 1;
    }
    init_parser();
    init_cross_referencer();

    // Execute parsing and code generation
    int parse_result = parse_program();
    
    // Only print cross reference if no errors
    if (parse_result == 0) {
        print_cross_reference_table();
    }

    // Cleanup
    end_scan();
    fclose(caslfp);

    return parse_result != 0;
}