#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"

#define INDENT 4

static int current_indent = 0;
static int need_space = 0;
static int begin_end_count = 0;
static int previous_token = 0;
static int while_do_block = 0;
extern char string_attr[];

static void print_indent(void) {
    for (int i = 0; i < current_indent; i++) {
        printf(" ");
    }
}

void init_pretty_printer(void) {
    current_indent = 0;
    need_space = 0;
    begin_end_count = 0;
    previous_token = 0; 
}

void pretty_print_token(int token) {
    switch (token) {
        case TPROGRAM:
            printf("program ");
            need_space = 0;
            break;
            
        case TVAR:
            current_indent = 4;
            print_indent();
            printf("var\n"); // No initial newline
            current_indent = 8;
            print_indent();
            need_space = 0;
            break;
        
        case TPROCEDURE:
            current_indent = current_indent + INDENT;
            print_indent();
            printf("procedure ");  // Add 4 spaces before procedure
            need_space = 0;
            break;

        case TBEGIN:
            if (begin_end_count == 0) {  // Main program begin
                printf("begin\n");
                current_indent = current_indent + INDENT;
            } else if (previous_token == TDO){  // Nested begin if before is TDO
                while_do_block = 1;
                current_indent = current_indent + INDENT;
                print_indent();
                printf("begin\n");
                current_indent = current_indent + INDENT;
            } else if (begin_end_count > 0) {   // Nested begin normal
                print_indent();
                printf("begin\n");
                current_indent = current_indent + INDENT;
            }
            print_indent();
            begin_end_count++;
            need_space = 0;
            break;

        case TEND:
            begin_end_count--;
            if (begin_end_count == 0) {
                current_indent = current_indent - INDENT;
                printf("\n");
                print_indent();
                printf("end");
            } else if (while_do_block == 1){
                while_do_block = 0;
                current_indent = current_indent - INDENT;
                printf("\n");
                print_indent();
                printf("end");
                current_indent = current_indent - INDENT; // Reset to -4 for main block
            } else {
                current_indent = current_indent - INDENT;
                printf("\n");
                print_indent();
                printf("end");
            }
            need_space = 1;
            break;

        case TSEMI:
            printf(";");
            if (begin_end_count == 0 && current_indent == 8) {
                current_indent = 0;
                printf("\n");
            } else {
                printf("\n");
                print_indent();  // This will use current_indent for proper nesting
            }
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

        case TSTRING:
            printf("'%s'", string_attr);
            need_space = 1;
            break;

        case TNUMBER:
            printf(" %d", num_attr);
            need_space = 1;
            break;

        case TWHILE:
            printf("while ");
            need_space = 0;
            break;

        case TDO:
            printf(" do\n");
            need_space = 0;
            break;

        case TCOMMA:
            printf(" ,");
            need_space = 1;
            break;

        case TLPAREN:
            printf(" ( ");
            need_space = 0;
            break;

        case TRPAREN:
            printf(" )");
            need_space = 1;
            break;

        case TPLUS:
        case TMINUS:
            printf(" %s", tokenstr[token]);  // No space before +/-
            need_space = 1;  // Space after operator
            break;

        case TASSIGN:
            printf(" :=");
            need_space = 1;
            break;
        
        case TGR:
            printf(" >");
            need_space = 1;
            break;

        case TLE:
            printf(" <");
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