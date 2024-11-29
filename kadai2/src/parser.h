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
} Parser;

// Public interface
void init_parser(void);
int parse_program(void);
void parse_error(const char* message);
static void parse_name_list(void);
static void parse_type(void);
static void parse_parameter_list(void);
static void parse_assignment(void);
static void parse_if_statement(void);
static void parse_while_statement(void);
static void parse_procedure_call(void);
static void parse_read_statement(void);
static void parse_write_statement(void);
static void parse_variable(void);
static void parse_expression(void);

#endif