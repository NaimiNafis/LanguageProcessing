#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#define ERROR -1
#define NORMAL 0

// External variables
extern int token;
extern int indentation_level;

// Parsing functions
int parse_program(void);
int parse_block(void);
int parse_variable_declaration(void);
int parse_variable_names(void);
int parse_type(void);
int parse_array_type(void);
int parse_compound_statement(void);
int parse_statement(void);
int parse_assignment_statement(void);
int parse_condition_statement(void);
int parse_iteration_statement(void);
int parse_exit_statement(void);
int parse_expression(void);
int parse_simple_expression(void);
int parse_term(void);
int parse_factor(void);

// Pretty-printing
void pretty_print(void);
void expect(int expected, const char *error_message);
void print_token(void);

#endif
