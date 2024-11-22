#include "scan.h"

/* keyword list */
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
  {"writeln", TWRITELN}};

/* Token counter */
int numtoken[NUMOFTOKEN + 1];

/* string of each token */
char *tokenstr[NUMOFTOKEN + 1] = {
  "",        "NAME",    "program", "var",     "array",     "of",     
  "begin",   "end",     "if",      "then",    "else",      "procedure",
  "return",  "call",    "while",   "do",      "not",       "or",
  "div",     "and",     "char",    "integer", "boolean",   "readln",
  "writeln", "true",    "false",   "NUMBER",  "STRING",    "+",
  "-",       "*",       "=",       "<>",      "<",         "<=",
  ">",       ">=",      "(",       ")",       "[",         "]",
  ":=",      ".",       ",",       ":",       ";",         "read",   
  "write",   "break"};

#include "scan.h"

// Function prototypes
int parse_program(void);  // Parsing logic will be in scan.c
void pretty_print(void);  // Handles pretty printing

int main(int argc, char *argv[]) {
    if (argc < 2) {
        error("File name is not given.");
        return 1;
    }

    // Initialize the scanner with the input file
    if (init_scan(argv[1]) < 0) {
        error("Cannot open input file.");
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

    // Cleanup
    end_scan();
    return 0;
}

