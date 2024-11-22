#include <stdio.h>
#include "scan.h"

// External keyword list and token string array
struct KEY key[KEYWORDSIZE] = {
    {"and", TAND},         {"array", TARRAY},     {"begin", TBEGIN},
    {"boolean", TBOOLEAN}, {"break", TBREAK},     {"call", TCALL},
    {"char", TCHAR},       {"div", TDIV},         {"do", TDO},
    {"else", TELSE},       {"end", TEND},         {"false", TFALSE},
    {"if", TIF},           {"integer", TINTEGER}, {"not", TNOT},
    {"of", TOF},           {"or", TOR},           {"procedure", TPROCEDURE},
    {"program", TPROGRAM}, {"read", TREAD},       {"readln", TREADLN},
    {"return", TRETURN},   {"then", TTHEN},       {"true", TTRUE},
    {"var", TVAR},         {"while", TWHILE},     {"write", TWRITE},
    {"writeln", TWRITELN}
};

// Function prototypes
int parse_program(void);   // Parsing logic
void pretty_print(void);   // Pretty-printing logic

int main(int argc, char *argv[]) {
    // Check for input file argument
    if (argc < 2) {
        fprintf(stderr, "Error: File name is not provided.\n");
        return 1;
    }

    // Initialize the scanner with the input file
    if (init_scan(argv[1]) < 0) {
        fprintf(stderr, "Error: Cannot open input file '%s'.\n", argv[1]);
        return 1;
    }

    // Parse the program
    int parse_result = parse_program();

    // Handle the outcome of parsing
    if (parse_result == 0) {
        printf("Parsing successful. Pretty-printing program...\n");
        pretty_print();
    } else {
        fprintf(stderr, "Parsing failed. Check the errors above.\n");
    }

    // Cleanup resources
    end_scan();
    return 0;
}
