#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "scan.h"
#include "parser.h"
#include "cross_referencer.h"
#include "compiler.h"
#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Platform-independent path separator
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

char* get_absolute_path(const char* path) {
    char* abs_path = (char*)malloc(PATH_MAX);
    if (!abs_path) return NULL;

    #ifdef _WIN32
    if (_fullpath(abs_path, path, PATH_MAX) == NULL) {
        free(abs_path);
        return NULL;
    }
    #else
    if (realpath(path, abs_path) == NULL) {
        free(abs_path);
        return NULL;
    }
    #endif

    return abs_path;
}

int main(int argc, char *argv[]) {
    // Validate input and handle debug mode
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./mpplc <filename.mpl> [--debug]\n");
        return 1;
    }

    // Get absolute path
    char* fullpath = get_absolute_path(argv[1]);
    if (!fullpath) {
        fprintf(stderr, "Error: Invalid path %s\n", argv[1]);
        return 1;
    }

    // Check file extension
    char *dot = strrchr(fullpath, '.');
    if (!dot || strcmp(dot, ".mpl") != 0) {
        fprintf(stderr, "Error: Input file must have .mpl extension\n");
        free(fullpath);
        return 1;
    }

    // Create output filename (replace .mpl with .csl)
    char *outfile = (char*)malloc(strlen(fullpath) + 1);
    if (!outfile) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(fullpath);
        return 1;
    }
    
    strcpy(outfile, fullpath);
    strcpy(outfile + (dot - fullpath), ".csl");

    // Open output file
    caslfp = fopen(outfile, "w");
    if (!caslfp) {
        fprintf(stderr, "Error: Cannot create output file %s\n", outfile);
        free(fullpath);
        free(outfile);
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug_mode = 1;
    }

    // Initialize components
    if (init_scan(argv[1]) < 0) {
        fclose(caslfp);
        free(fullpath);
        free(outfile);
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
    free(fullpath);
    free(outfile);

    return parse_result != 0;
}