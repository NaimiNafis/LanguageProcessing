#ifndef PARSER_H
#define PARSER_H

#include <setjmp.h>
#include <stdbool.h>

#define ERROR 0
#define NORMAL 1

typedef struct {
    int current_token;
    int line_number;
    int first_error_line;
    int previous_token;
    int previous_previous_token;
    jmp_buf error_jmp;
} Parser;

// Public interface
void init_parser(void);
void parse_error(const char* message);
int parse_program(void);

#endif