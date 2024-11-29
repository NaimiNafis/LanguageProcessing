#ifndef PARSER_H
#define PARSER_H

#include "scan.h"

// Parser structure
typedef struct {
    const char *input;
    int position;
    int current_token;
    int line_number;
    int error_count;
    int first_error_line; 
    int previous_token;
} Parser;

// Public interface
void init_parser(void);
int parse_program(void);
void parse_error(const char* message);

// Forward declarations for parsing functions
static void parse_block(void);
static void parse_var_declarations(void);
static void parse_name_list(void);
static void parse_type(void);
static void parse_procedure(void);
static void parse_parameter_list(void);
static void parse_statement_list(void);
static void parse_statement(void);
static void parse_assignment(void);
static void parse_if_statement(void);
static void parse_while_statement(void);
static void parse_procedure_call(void);
static void parse_read_statement(void);
static void parse_write_statement(void);
static void parse_variable(void);
static void parse_expression(void);
static void parse_term(void);
static void parse_factor(void);
static void parse_comparison(void);
static void parse_condition(void);

#endif