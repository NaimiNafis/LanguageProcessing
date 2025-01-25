#ifndef PARSER_H
#define PARSER_H

#include <setjmp.h>
#include <stdbool.h>

#define ERROR 0
#define NORMAL 1
#define MAX_PARAMS 100  // Maximum number of parameters allowed in a procedure

// Symbol table entry structure
typedef struct {
    char* name;
    int type;
    int line_num;
    int array_size;  // For array types
    int is_array;    // Flag to indicate if it's an array
    int param_count;     // Added for procedure parameters
    int param_types[MAX_PARAMS]; // Added for procedure parameters
} SymbolEntry;

typedef struct {
    int current_token;
    int line_number;
    int first_error_line;
    int previous_token;
    int previous_previous_token;
    jmp_buf error_jmp;
} Parser;

// Add these declarations
extern int current_array_size;  // For array size tracking
extern FILE* caslfp;           // CASL output file pointer

// Public interface
void init_parser(void);
void parse_error(const char* message);
int parse_program(void);

// Symbol table functions
SymbolEntry* lookup_symbol(const char* name);
int is_current_procedure(const char* name);
int get_procedure_param_count(const char* name);
int* get_procedure_param_types(const char* name);

// Add these declarations
int p_ifst(void);
int p_term(void);
int p_exp(void);
int p_st(void);
int p_factor(void);

#endif