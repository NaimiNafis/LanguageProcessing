#include <stdio.h>
#include <string.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"

#define INDENT_SIZE 4
#define MAX_INDENT 40

static int current_indent = 0;
static int need_space = 0;

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
    // Add space if needed from previous token
    if (need_space) {
        printf(" ");
        need_space = 0;
    }

    switch (token) {
        // Keywords that need space after
        case TPROGRAM:
            printf("program");
            need_space = 1;
            break;

        case TVAR:
        case TARRAY:
        case TOF:
        case TPROCEDURE:
        case TIF:
        case TTHEN:
        case TELSE:
        case TWHILE:
            printf("%s ", tokenstr[token]);
            break;

        // Block structure tokens
        case TBEGIN:
        case TDO:
            printf("%s", tokenstr[token]);
            print_newline_with_indent();
            increase_indent();
            break;

        case TEND:
            decrease_indent();
            print_newline_with_indent();
            printf("%s", tokenstr[token]);
            break;

        // Statement terminators
        case TSEMI:
            printf(";");
            print_newline_with_indent();
            need_space = 0;
            break;

        case TDOT:
            printf(".");
            need_space = 0;
            break;


        // Identifiers and literals
        case TNAME:
            if (need_space) {
                printf(" ");
            }
            printf("%s", string_attr);
            need_space = 1;
            break;

        // Numbers and strings
        case TNUMBER:
            if (need_space) printf(" ");
            printf("%d", num_attr);
            need_space = 1;
            break;

        case TSTRING:
            if (need_space) printf(" ");
            printf("'%s'", string_attr);
            need_space = 1;
            break;


        // Operators that need spaces around them
        case TPLUS: case TMINUS: case TSTAR: case TDIV:
        case TAND: case TOR: case TNOT:
        case TEQUAL: case TNOTEQ: case TLE: case TLEEQ:
        case TGR: case TGREQ:
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            break;

        // Delimiters
        case TLPAREN:
            printf("(");
            need_space = 0;
            break;
        case TRPAREN:
            printf(")");
            need_space = 1;
            break;
        case TLSQPAREN:
            printf("[");
            need_space = 0;
            break;
        case TRSQPAREN:
            printf("]");
            need_space = 1;
            break;
        case TASSIGN:
            printf(" := ");
            need_space = 0;
            break;
        case TCOMMA:
            printf(", ");
            need_space = 0;
            break;
        case TCOLON:
            printf(" : ");
            need_space = 0;
            break;
            
        default:
            if (need_space) {
                printf(" ");
            }
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