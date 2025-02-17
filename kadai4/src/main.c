#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "scan.h"
#include "parser.h"
#include "cross_referencer.h"
#include "compiler.h"

// Global debug flags
int debug_scanner = 0;
int debug_parser = 1;
int debug_cross_referencer = 1;
int debug_pretty = 0;
int debug_compiler = 1;
int debug_codegen = 1;

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
    char *outfile = (char*)malloc(strlen(fullpath) + 5); // +5 for .csl and null terminator
    if (!outfile) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(fullpath);
        return 1;
    }
    
    strcpy(outfile, fullpath);
    char *ext = strrchr(outfile, '.');
    if (ext) *ext = '\0';
    strcat(outfile, ".csl");

    // Create directories if they don't exist
    char *last_slash = strrchr(outfile, PATH_SEPARATOR);
    if (last_slash) {
        *last_slash = '\0';
        #ifdef _WIN32
        _mkdir(outfile);
        #else
        mkdir(outfile, 0777);
        #endif
        *last_slash = PATH_SEPARATOR;
    }

    // Open output file
    caslfp = fopen(outfile, "w");
    if (!caslfp) {
        fprintf(stderr, "Error: Cannot create output file %s\n", outfile);
        free(fullpath);
        free(outfile);
        return 1;
    }

    // Process command line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--debug-scan") == 0) {
            debug_scanner = 1;
        } else if (strcmp(argv[i], "--debug-parse") == 0) {
            debug_parser = 1;
        } else if (strcmp(argv[i], "--debug-xref") == 0) {
            debug_cross_referencer = 1;
        } else if (strcmp(argv[i], "--debug-compile") == 0) {
            debug_compiler = 1;
        } else if (strcmp(argv[i], "--debug-pretty") == 0) {
            debug_pretty = 1;
        } else if (strcmp(argv[i], "--debug-codegen") == 0) {
            debug_codegen = 1;
        } else if (strcmp(argv[i], "--debug-all") == 0) {
            debug_scanner = debug_parser = debug_cross_referencer = 
            debug_pretty = debug_compiler = debug_codegen = 1;
        }
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
    
    // Log error to CASL file if parsing fails
    if (parse_result != 0) {
        fprintf(caslfp, "/* Compilation failed: no valid CASL code generated. */\n");
    }
    else {
        // Only print cross reference if no errors
        print_cross_reference_table();
    }

    // Cleanup
    end_scan();
    fclose(caslfp);
    free(fullpath);
    free(outfile);

    return parse_result != 0;
}