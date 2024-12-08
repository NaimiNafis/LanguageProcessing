#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"
#include "debug_pretty.h"

/*
 * This version introduces a context stack and a unified indentation calculation approach.
 * It removes the scattered logic of directly manipulating current_indent and replaces it
 * with contexts that store the indentation "type" and base level. The indentation rules are
 * all computed in one place, making it easier to tweak and maintain formatting.
 *
 * IMPORTANT: 
 * - You will need to adjust indentation rules in compute_indent_level() to match exactly 
 *   your desired formatting.
 * - The code below is a template. Integrate it with your environment, test, and refine.
 * - Not all special cases may be fully handled. Use this as a starting point.
 */

// For indentation
#define INDENT_SPACES 4
#define MAX_CONTEXTS  100

// Context types define what kind of block we are in
typedef enum {
    CTX_GLOBAL,      // Top-level (under program)
    CTX_PROCEDURE,   // Inside a procedure declaration
    CTX_VAR_BLOCK,   // Inside a var block
    CTX_BEGIN_BLOCK, // A begin-end block
    CTX_IF_THEN,     // An if-then block
    CTX_ELSE_BLOCK,  // An else block
    CTX_WHILE_DO     // A while-do block
} ContextType;

typedef struct {
    ContextType type;
    int base_indent_level; // Logical indentation level (not directly spaces)
    // Add flags if needed, for example to differentiate between var under program vs procedure
    int is_var_under_program;
    int is_var_under_procedure;
} Context;

// Global state
static Context context_stack[MAX_CONTEXTS];
static int context_top = -1;

static int need_space = 0;
static int last_printed_newline = 1; 
static int prev_token = 0, curr_token = 0, next_token = 0;
static int num_attr;
extern char string_attr[]; // provided by scan.c
extern char *tokenstr[];   // provided by token.c

// Forward declarations
static void push_context(ContextType type, int base_indent, int var_prog, int var_proc);
static void pop_context(void);
static ContextType current_context_type(void);
static int compute_indent_level(void);
static void print_indent(void);
static void print_newline(void);
static void print_token(const char *text);
static void update_token_history(int new_token);
static void handle_var_context_on_entry(void);
static void handle_var_context_on_exit(void);


// Initialize the pretty printer
void init_pretty_printer(void) {
    context_top = -1;
    // Start at global context, indent=0
    push_context(CTX_GLOBAL, 0, 0, 0);
    need_space = 0;
    last_printed_newline = 1;
    prev_token = curr_token = next_token = 0;
}

// Push a new context on the stack
static void push_context(ContextType type, int base_indent, int var_prog, int var_proc) {
    if (context_top < MAX_CONTEXTS - 1) {
        context_top++;
        context_stack[context_top].type = type;
        context_stack[context_top].base_indent_level = base_indent;
        context_stack[context_top].is_var_under_program = var_prog;
        context_stack[context_top].is_var_under_procedure = var_proc;
    }
}

// Pop the current context
static void pop_context(void) {
    if (context_top >= 0) {
        context_top--;
    }
}

// Get the current context type
static ContextType current_context_type(void) {
    if (context_top >= 0) {
        return context_stack[context_top].type;
    }
    return CTX_GLOBAL;
}

// Compute the actual indentation level in spaces
// Adjust this logic to achieve desired indentation rules.
static int compute_indent_level(void) {
    int indent = 0;

    // Walk through stack and determine indentation based on context order
    // The highest level context (top of stack) usually determines how much to indent.
    // You can refine these rules as needed.

    // Example heuristic:
    // CTX_GLOBAL: base indentation = 0
    // CTX_PROCEDURE: procedure header lines at base_indent (often 1)
    // CTX_VAR_BLOCK under program: var line at level 1, variables at level 2
    // CTX_VAR_BLOCK under procedure: var line at level 2, variables at level 3
    // CTX_BEGIN_BLOCK: usually one more level than parent
    // CTX_IF_THEN, CTX_ELSE_BLOCK, CTX_WHILE_DO: usually indent one more level for their statements

    // In this simplified approach, we use the base_indent_level stored in the context.
    // You can store the indentation logic in the push_context calls or compute differently.
    if (context_top >= 0) {
        indent = context_stack[context_top].base_indent_level;
    }

    return indent * INDENT_SPACES;
}

static void print_indent(void) {
    int spaces = compute_indent_level();
    for (int i = 0; i < spaces; i++) {
        printf(" ");
    }
}

static void print_newline(void) {
    printf("\n");
    last_printed_newline = 1;
}

// print_token prints a token with proper spacing and indentation
static void print_token(const char *text) {
    if (last_printed_newline) {
        // At the start of a line, print indentation first
        print_indent();
    } else {
        // If continuing the same line, print a space if needed
        if (need_space) printf(" ");
    }
    printf("%s", text);
    need_space = 1;             // Next token may need space before it
    last_printed_newline = 0;   // We are now mid-line
}

// Update token history to know prev/curr/next tokens
static void update_token_history(int new_token) {
    prev_token = curr_token;
    curr_token = next_token;
    next_token = new_token;
}

// Helper: when we enter a var block, decide indent rules based on current context
static void handle_var_context_on_entry(void) {
    // Check if we are under program or procedure
    // CTX_GLOBAL means we are at program level, CTX_PROCEDURE means at procedure level
    ContextType ctype = current_context_type();
    if (ctype == CTX_GLOBAL) {
        // var under program: var line at level 1
        push_context(CTX_VAR_BLOCK, 1, 1, 0);
    } else if (ctype == CTX_PROCEDURE) {
        // var under procedure: var line at level 2
        push_context(CTX_VAR_BLOCK, 2, 0, 1);
    } else {
        // Nested var block? Adjust as needed. For now, treat like global
        push_context(CTX_VAR_BLOCK, 1, 1, 0);
    }
}

// Helper: When exiting var block, pop the context
static void handle_var_context_on_exit(void) {
    // If we are in a var block, pop it
    if (current_context_type() == CTX_VAR_BLOCK) {
        pop_context();
    }
}

// Main token printing function
void pretty_print_token(int token) {
    debug_pretty_printf("Token: %s, Prev: %s, CurrCtx: %d\n",
        tokenstr[token],
        tokenstr[prev_token],
        current_context_type());

    // Before handling this token, consider if certain tokens (like TSEMI) ended a line, etc.
    switch (token) {
        // Program structure
        case TPROGRAM:
            // program at global, indent = 0
            print_token("program");
            need_space = 0; // expecting a program name next
            break;

        case TPROCEDURE:
            // procedure: push a context at level 1
            // print on a new line from the global or after var
            print_newline();
            push_context(CTX_PROCEDURE, 1, 0, 0);
            print_token("procedure");
            need_space = 1;
            break;

        case TVAR:
            // var keyword: depends if under program or procedure
            // handle_var_context_on_entry decides correct indentation
            print_newline();
            handle_var_context_on_entry();
            print_token("var");
            print_newline();
            need_space = 0;
            break;

        // Begin-End Blocks
        case TBEGIN:
            // Decide indentation based on context:
            // - In PROGRAM: begin at level 0
            // - In PROCEDURE: begin at level 1
            // - Nested blocks: begin one level more than parent
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                ContextType ctype = current_context_type();
                int new_indent = parent_indent;
                if (ctype == CTX_GLOBAL) {
                    new_indent = 0;  // Program-level begin
                } else if (ctype == CTX_PROCEDURE) {
                    new_indent = 1;  // Procedure-level begin
                } else {
                    // Nested begin block: increment parent by 1
                    new_indent = parent_indent + 1;
                }

                print_newline();
                push_context(CTX_BEGIN_BLOCK, new_indent, 0, 0);
                print_token("begin");
                print_newline();
            }
            need_space = 0;
            break;

        case TEND:
            // end must match the indentation of corresponding begin
            // Just pop the begin context
            if (current_context_type() == CTX_BEGIN_BLOCK) {
                pop_context();
            } else {
                // handle error or unexpected token scenario
            }
            print_newline();
            print_token("end");
            need_space = 0; // next might be ';'
            break;

        // Statements
        case TIF:
            // if: print on new line, same indent as parent, then after "then" we indent more
            print_newline();
            print_token("if");
            break;

        case TTHEN:
            // if and then on same line
            print_token("then");
            // after then, we indent one more level for the block
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                // Push if-then block with one more level
                push_context(CTX_IF_THEN, parent_indent + 1, 0, 0);
            }
            print_newline();
            need_space = 0;
            break;

        case TELSE:
            // else on new line at the same indent as if
            // pop if-then context first
            if (current_context_type() == CTX_IF_THEN) {
                pop_context();
            }
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                // else block with one more level
                print_newline();
                push_context(CTX_ELSE_BLOCK, parent_indent + 1, 0, 0);
                print_token("else");
                print_newline();
            }
            need_space = 0;
            break;

        case TWHILE:
            print_newline();
            print_token("while");
            need_space = 1;
            break;

        case TDO:
            // while and do on same line
            print_token("do");
            // after do, one more indent level
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                push_context(CTX_WHILE_DO, parent_indent + 1, 0, 0);
            }
            print_newline();
            need_space = 0;
            break;

        case TCALL:
            print_token("call");
            need_space = 1;
            break;

        // Operators
        case TASSIGN:
            print_token(":=");
            need_space = 1;
            break;

        case TPLUS:
        case TMINUS:
        case TSTAR:
        case TDIV:
        case TAND:
        case TOR:
            // Binary operators surrounded by spaces
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            last_printed_newline = 0;
            break;

        case TEQUAL:
        case TNOTEQ:
        case TLE:
        case TLEEQ:
        case TGR:
        case TGREQ:
            // Relational operators with spaces
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            last_printed_newline = 0;
            break;

        // Punctuation
        case TSEMI:
            // Semicolon ends the statement line
            printf(";");
            // Newline after semicolon, same indentation as the current block
            print_newline();
            need_space = 0;
            break;

        case TCOMMA:
            print_token(",");
            need_space = 1;
            break;

        case TCOLON:
            print_token(":");
            need_space = 1;
            break;

        case TDOT:
            print_token(".");
            need_space = 0;
            break;

        // Grouping symbols
        case TLPAREN:
            print_token("(");
            need_space = 0;
            break;

        case TRPAREN:
            print_token(")");
            need_space = 1;
            break;

        case TLSQPAREN:
            print_token("[");
            need_space = 0;
            break;

        case TRSQPAREN:
            print_token("]");
            need_space = 1;
            break;

        // I/O statements
        case TREADLN:
        case TREAD:
        case TWRITE:
        case TWRITELN:
            // These appear often at the start of a statement line
            // If needed, print a newline first
            print_newline();
            print_token(tokenstr[token]);
            need_space = 1;
            break;

        // Values and identifiers
        case TNAME:
            if (need_space) printf(" ");
            printf("%s", string_attr);
            need_space = 1;
            last_printed_newline = 0;
            break;

        case TNUMBER:
            if (need_space) printf(" ");
            printf("%d", num_attr);
            need_space = 1;
            last_printed_newline = 0;
            break;

        case TSTRING:
            if (need_space) printf(" ");
            printf("'%s'", string_attr);
            need_space = 1;
            last_printed_newline = 0;
            break;

        default:
            // For any other tokens not explicitly handled
            if (need_space) printf(" ");
            printf("%s", tokenstr[token]);
            need_space = 1;
            last_printed_newline = 0;
            break;
    }
}

// Main pretty-print function
void pretty_print_program(void) {
    init_pretty_printer();
    
    // Prime the token stream
    next_token = scan();
    if (next_token > 0) {
        curr_token = next_token;
        next_token = scan();
    }
    
    while (curr_token > 0) {
        pretty_print_token(curr_token);
        update_token_history(scan());
    }
}
