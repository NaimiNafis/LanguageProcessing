#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"

// Constants for indentation levels
#define GLOBAL_LEVEL 0
#define PROCEDURE_LEVEL 4
#define NESTED_LEVEL 8
#define VAR_DECLARATION_LEVEL 4

static int current_indent = 0;
static int need_space = 0;
static int block_level = 0;
static int in_procedure = 0;
static int in_procedure_header = 0;
static int in_loop_or_if = 0;
static int last_printed_newline = 0;

extern char string_attr[];
extern int num_attr;

static void print_indent(void) {
    for (int i = 0; i < current_indent; i++) {
        printf(" ");
    }
}

static void print_newline_if_needed(void) {
    if (!last_printed_newline) {
        printf("\n");
        last_printed_newline = 1;
    }
}


void init_pretty_printer(void) {
    current_indent = GLOBAL_LEVEL;
    need_space = 0;
    block_level = 0;
    in_procedure = 0;
    in_procedure_header = 0;
    in_loop_or_if = 0;
    last_printed_newline = 0;
}


void pretty_print_token(int token) {
    switch (token) {
        // Program structure
        case TPROGRAM:
            current_indent = GLOBAL_LEVEL;
            printf("program ");
            need_space = 0;
            break;

        case TPROCEDURE:
            print_newline_if_needed();
            current_indent = PROCEDURE_LEVEL;
            print_indent();
            printf("procedure ");
            in_procedure = 1;
            in_procedure_header = 1;
            need_space = 0;
            last_printed_newline = 0;
            break;

        case TVAR:
            print_newline_if_needed();
            if (!in_procedure_header && current_indent == GLOBAL_LEVEL) {
                printf("    var\n");  // Direct indentation for global var
                current_indent = 4;
            } else {
                printf("\n");
                if (in_procedure_header) {
                    current_indent = PROCEDURE_LEVEL;
                } else if (in_procedure) {
                    current_indent = NESTED_LEVEL;
                } else {
                    current_indent = VAR_DECLARATION_LEVEL;
                }
                print_indent();
                printf("var\n");
            }
            current_indent += 4;
            print_indent();
            need_space = 0;
            last_printed_newline = 1;
            break;

        // compound statements
        case TBEGIN:
            print_newline_if_needed();
            if (in_loop_or_if) {
                current_indent += 4;
                print_indent();  // Use current indentation level
                printf("begin\n");
                current_indent += 4;  // Increase indent for block contents
            } else if (in_procedure) {
                current_indent = PROCEDURE_LEVEL;
                print_indent();
                printf("begin\n");
                current_indent += 4;
            } else {
                current_indent = GLOBAL_LEVEL;
                print_indent();
                printf("begin\n");
                current_indent += 4;
            }
            block_level++;
            print_indent();
            need_space = 0;
            last_printed_newline = 1;
            break;

        case TEND:
            block_level--;
            current_indent -= 4;
            printf("\n");
            print_indent();
            printf("end");
            if (block_level == 0) {
                current_indent = GLOBAL_LEVEL;
                in_procedure = 0;
                in_loop_or_if = 0;
            }
            need_space = 1;
            last_printed_newline = 0;
            break;

        // Statements
        case TIF:
            in_loop_or_if = 1;
            print_indent();
            printf("if ");
            need_space = 0;
            break;

        case TTHEN:
            printf(" then ");
            current_indent += 4;
            need_space = 0;
            break;

        case TELSE:
            current_indent -= 4;
            printf("\n");
            print_indent();
            printf("else ");
            current_indent += 4;
            need_space = 0;
            break;

        case TWHILE:
            in_loop_or_if = 1;
            print_indent();
            printf("while ");
            need_space = 0;
            break;

        case TDO:
            print_newline_if_needed();
            printf(" do");
            if (!in_loop_or_if) {
                current_indent += 4;
            }
            need_space = 0;
            last_printed_newline = 0;
            break;

        case TCALL:
            printf("call ");
            need_space = 0;
            break;

        // Operators
        case TASSIGN:
            printf(" := ");
            need_space = 0;
            break;

        case TPLUS:
        case TMINUS:
        case TSTAR:
        case TDIV:
        case TAND:
        case TOR:
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            break;

        case TEQUAL:
        case TNOTEQ:
        case TLE:
        case TLEEQ:
        case TGR:
        case TGREQ:
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            break;

        // Punctuation
        case TSEMI:
            printf(";");
            if (block_level > 0) {
                printf("\n");
                if (in_loop_or_if) {
                    current_indent = block_level * 4;
                }
                print_indent();
            } else if (in_procedure_header) {
                printf("\n");
                current_indent = PROCEDURE_LEVEL;
                print_indent();
                in_procedure_header = 0;
            } else {
                printf("\n");
                print_indent();
            }
            need_space = 0;
            last_printed_newline = 1;
            break;

        case TCOMMA:
            printf(" ,");
            need_space = 1;
            break;

        case TCOLON:
            printf(" :");
            need_space = 1;
            break;

        case TDOT:
            printf(".");
            need_space = 0;
            break;

        // Grouping symbols
        case TLPAREN:
            printf(" ( ");
            need_space = 0;
            break;

        case TRPAREN:
            printf(" )");
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

        // I/O statements
        case TREADLN:
        case TREAD:
            printf("%s", tokenstr[token]);
            need_space = 1;
            break;

        case TWRITELN:
        case TWRITE:
            printf("%s", tokenstr[token]);
            need_space = 1;
            break;

        // Values and identifiers
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

        case TSTRING:
            if (need_space) printf(" ");
            printf("'%s'", string_attr);
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
    init_pretty_printer();
    while ((token = scan()) > 0) {
        pretty_print_token(token);
    }
}