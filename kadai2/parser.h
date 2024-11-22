#ifndef PARSER_H
#define PARSER_H

#define NORMAL 0
#define ERROR 1

int parse_program(void);
int parse_block(void);
int parse_variable_declaration(void);
int parse_statement(void);
int parse_expression(void);

// Declare missing function prototypes
int parse_assignment_statement(void);
int parse_if_statement(void);
int parse_while_statement(void);
int parse_write_statement(void);
int parse_read_statement(void);

#endif // PARSER_H
