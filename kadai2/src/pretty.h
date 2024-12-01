#ifndef PRETTY_H
#define PRETTY_H

#define INDENT_SPACES 4

void init_pretty_printer(void);
void pretty_print_token(int token);
void pretty_print_program(void);
void pretty_if_statement(void);
void pretty_while_statement(void);

#endif