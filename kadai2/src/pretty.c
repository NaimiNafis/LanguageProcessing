#include <stdio.h>
#include <string.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"

#define INDENT_SIZE 4
#define MAX_INDENT 40

static int current_indent = 0;
static int need_space = 0;
extern Token current_token;

// Function declarations
static void pretty_expression(void);
static void pretty_statement(void);
static void pretty_term(void);
static void pretty_factor(void);
static int get_token(void);

// Helper functions
static void print_indent(void) {
    for (int i = 0; i < current_indent; i++) {
        printf(" ");
    }
}

static void increase_indent(void) {
    current_indent += INDENT_SIZE;
    if (current_indent > MAX_INDENT) {
        current_indent = MAX_INDENT;
    }
}

static void decrease_indent(void) {
    current_indent -= INDENT_SIZE;
    if (current_indent < 0) {
        current_indent = 0;
    }
}

static void print_newline_with_indent(void) {
    printf("\n");
    print_indent();
}

void init_pretty_printer(void) {
    current_indent = 0;
    need_space = 0;
}

// Main pretty print function
void pretty_print_token(int token) {
    switch (token) {
        // Program structure
        case TPROGRAM:
            printf("program");
            need_space = 1;
            break;
            
        case TVAR:
            print_newline_with_indent();
            printf("var");
            print_newline_with_indent();
            increase_indent();
            need_space = 0;
            break;

        case TBEGIN:
            if (current_indent > 0) decrease_indent();
            print_newline_with_indent();
            printf("begin");
            print_newline_with_indent();
            increase_indent();
            need_space = 0;
            break;

        case TEND:
            decrease_indent();
            print_newline_with_indent();
            printf("end");
            need_space = 0;
            break;

        case TSEMI:
            printf(";");
            print_newline_with_indent();
            need_space = 0;
            break;

        case TDOT:
            printf(".");
            need_space = 0;
            break;

        case TNAME:
            if (need_space) printf(" ");
            printf("%s", string_attr);
            need_space = 1;
            break;

        case TNUMBER:
            if (need_space) printf(" ");
            printf("%d", num_attr);
            need_space = 1;
            break;

        default:
            if (need_space) printf(" ");
            printf("%s", tokenstr[token]);
            need_space = 1;
            break;
    }
}

void pretty_print_program(void) {
    int token;
    while ((token = scan()) >= 0) {
        pretty_print_token(token);
    }
}

void pretty_if_statement(void) {
    if (need_space) printf(" ");
    printf("if ");
    need_space = 0;
    
    pretty_expression();
    
    printf(" then ");
    need_space = 0;
    
    pretty_statement();
    
    if (current_token.kind == TELSE) {
        printf(" else ");
        get_token();
        pretty_statement();
    }
}

void pretty_while_statement(void) {
    if (need_space) printf(" ");
    printf("while ");
    need_space = 0;
    
    pretty_expression();
    
    printf(" do ");
    need_space = 0;
    
    pretty_statement();
}

static void pretty_expression(void) {
    pretty_term();
    while (current_token.kind == TPLUS || current_token.kind == TMINUS) {
        pretty_print_token(current_token.kind);
        get_token();
        pretty_term();
    }
}

static void pretty_statement(void) {
    switch (current_token.kind) {
        case TIF:
            pretty_if_statement();
            break;
        case TWHILE:
            pretty_while_statement();
            break;
        case TBEGIN:
            pretty_print_token(TBEGIN);
            get_token();
            while (current_token.kind != TEND) {
                pretty_statement();
                if (current_token.kind == TSEMI) {
                    pretty_print_token(TSEMI);
                    get_token();
                }
            }
            pretty_print_token(TEND);
            get_token();
            break;
        default:
            pretty_expression();
            break;
    }
}

static void pretty_term(void) {
    pretty_factor();
    while (current_token.kind == TSTAR || current_token.kind == TDIV) {
        pretty_print_token(current_token.kind);
        get_token();
        pretty_factor();
    }
}

static void pretty_factor(void) {
    switch (current_token.kind) {
        case TLPAREN:
            pretty_print_token(TLPAREN);
            get_token();
            pretty_expression();
            if (current_token.kind == TRPAREN) {
                pretty_print_token(TRPAREN);
                get_token();
            }
            break;
        case TNAME:
        case TNUMBER:
            pretty_print_token(current_token.kind);
            get_token();
            break;
        default:
            break;
    }
}

static int get_token(void) {
    int token = scan();
    current_token.kind = token;
    return token;
}