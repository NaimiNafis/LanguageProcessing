#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"

static int current_indent = 0;
static int need_space = 0;
static int begin_end_count = 0;
static int previous_token = 0;
static int in_procedure = 0;
static int in_procedure_body = 0;
static int in_nested_block = 0; 
extern char string_attr[];
extern int num_attr;

static void print_indent(void) {
    for (int i = 0; i < current_indent; i++) {
        printf(" ");
    }
}

void init_pretty_printer(void) {
    current_indent = 0;
    need_space = 0;
    previous_token = 0;
    in_procedure = 0;
    in_procedure_body = 0;
    in_nested_block = 0; 
}

void pretty_print_token(int token) {
    switch (token) {
        // Program structure
        case TPROGRAM:
            printf("program ");
            current_indent = 0;  // Global level, no indentation
            need_space = 0;
            break;
            
        case TPROCEDURE:
            printf("\n");
            current_indent = 4;  // Level 1 indentation
            print_indent();
            printf("procedure ");
            in_procedure = 1;
            need_space = 0;
            break;
            
        case TVAR:
            printf("\n");
            if (in_procedure_body) {
                current_indent = 12;  // Inside procedure body, Level 3 indentation (12 spaces)
            } else if (in_procedure) {
                current_indent = 8;   // Inside procedure declaration, Level 2 indentation (8 spaces)
            } else {
                current_indent = 4;   // Program-level var declaration, Level 1 indentation (4 spaces)
            }
            print_indent();
            printf("var\n");
            current_indent += 4;  // Add 4 spaces for the variables inside the var block
            print_indent();
            need_space = 0;
            break;

        // Compound statements
        case TBEGIN:
            printf("\n");
            if (in_procedure_body) {
                current_indent = 8;  // Inside procedure body, Level 2 indentation (8 spaces)
            } else if (in_procedure) {
                current_indent = 4;  // Inside procedure, Level 1 indentation (4 spaces)
            } else {
                current_indent = 0;  // Global block, no indentation
            }
            print_indent();
            printf("begin\n");
            current_indent += 4;   // Increase indentation for inner block
            print_indent();
            in_nested_block = 1;    // Mark that we're inside a nested block
            need_space = 0;
            break;

        case TEND:
            if (in_nested_block) {
                current_indent -= 4; // Decrease indentation after finishing a nested block
                in_nested_block = 0;  // Mark that we've exited the nested block
            }
            printf("\n");
            print_indent();
            printf("end");
            if (!in_procedure_body) {
                current_indent = 0;  // Reset indentation if we've exited a procedure or main body
            }
            need_space = 1;
            break;

        // Statements
        case TIF:
            printf("if ");
            need_space = 0;
            break;

        case TTHEN:
            printf(" then ");
            need_space = 0;
            break;

        case TELSE:
            printf("\n");
            print_indent();
            printf("else ");
            need_space = 0;
            break;

        case TWHILE:
            printf("while ");
            need_space = 0;
            break;

        case TDO:
            printf(" do");
            need_space = 0;
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
            if (in_procedure_body || begin_end_count > 0) {
                printf("\n");
                print_indent();
            }
            need_space = 0;
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
    previous_token = token;
}

void pretty_print_program(void) {
    int token;
    init_pretty_printer();
    while ((token = scan()) > 0) {
        pretty_print_token(token);
    }
}