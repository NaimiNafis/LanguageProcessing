#include <stdio.h>
#include "parser.h"
#include "scan.h"
#include "pretty.h"
#include "token.h"
#include "debug.h" 

static int current_indent = 0;
static int need_space = 0;

void init_pretty_printer(void) {
    current_indent = 0;
    need_space = 0;
}

void print_indent(void) {
    for (int i = 0; i < current_indent * INDENT_SPACES; i++) {
        putchar(' ');
    }
}

void increase_indent(void) {
    current_indent++;
}

void decrease_indent(void) {
    if (current_indent > 0) current_indent--;
}

void pretty_print_token(int token) {
    if (need_space) {
        putchar(' ');
    }

    printf("%s", tokenstr[token]);

    switch (token) {
        case TBEGIN:
        case TDO:
            printf("\n");
            increase_indent();
            print_indent();
            break;

        case TEND:
            printf("\n");
            decrease_indent();
            print_indent();
            break;

        case TSEMI:
            printf("\n");
            print_indent();
            break;

        case TNAME:
        case TNUMBER:
        case TSTRING:
            need_space = 1;
            break;

        default:
            need_space = 0;
            break;
    }
}

void pretty_print_program(void) {
    int token;
    
    // Initialize the pretty printer
    init_pretty_printer();
    
    // Get and print tokens until we reach end of file
    while ((token = scan()) >= 0) {  // Changed from get_token() to scan()
        pretty_print_token(token);
    }
}