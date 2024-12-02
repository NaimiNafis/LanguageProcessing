#ifndef PARSER_H
#define PARSER_H

#include "scan.h"
#include <setjmp.h>

#define NORMAL 0
#define ERROR 1

// Parser structure
typedef struct {
    const char *input;
    int current_token;
    int line_number;
    int first_error_line; 
    int previous_token;
    jmp_buf error_jmp; 
} Parser;

// Public interface
void init_parser(void);
int parse_program(void);

// Forward declarations for parsing functions
static int parse_variable_declaration(void);
static int parse_name_list(void);
static int parse_type(void);
static int parse_procedure(void);
static int parse_procedure_declaration(void);
static int parse_parameter_list(void);
static int parse_statement_list(void);
static int parse_statement(void);
static int parse_if_statement(void);
static int parse_while_statement(void);
static int parse_procedure_call(void);
static int parse_read_statement(void);
static int parse_write_statement(void);
static int parse_variable(void);
static int parse_expression(void);
static int parse_term(void);
static int parse_factor(void);
static int parse_comparison(void);
static int parse_condition(void);
static int match(int expected_token);
static int parse_block(void);
void parse_error(const char* message);

#endif