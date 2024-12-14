#ifndef PARSER_H
#define PARSER_H

#include <setjmp.h>
#include <stdbool.h>

// Parser result codes
#define ERROR 0
#define NORMAL 1

// Parser state tracking structure for maintaining parsing context
typedef struct {
    int current_token;
    int line_number;
    int first_error_line;
    int previous_token;
    int previous_previous_token;
    jmp_buf error_jmp;
} Parser;

// Core parser interface
void init_parser(void);
int parse_program(void);
void parse_error(const char* message);

#endif