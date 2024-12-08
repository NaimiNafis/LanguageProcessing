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
 * - You need to verify and adjust indentation rules in compute_indent_level().
 * - Review how you push and pop contexts (especially for var, if/then/else, and while/do).
 * - Not all special cases are fully handled; this is a template.
 * - Add or refine logic as you discover patterns in your input grammar.
 */

// For indentation
#define INDENT_SPACES 4
#define MAX_CONTEXTS  100

// Context types define what kind of block we are in.
// You may need more context types if you discover more block-like constructs.
typedef enum {
    CTX_GLOBAL,      // Top-level (under program)
    CTX_PROCEDURE,   // Inside a procedure declaration
    CTX_VAR_BLOCK,   // Inside a var block
    CTX_VAR_DECL,    // Inside a var declaration
    CTX_BEGIN_BLOCK, // A begin-end block
    CTX_IF_THEN,     // An if-then block
    CTX_ELSE_BLOCK,  // An else block
    CTX_WHILE_DO     // A while-do block
} ContextType;

typedef struct {
    ContextType type;
    int base_indent_level; 
    int is_var_under_program;
    int is_var_under_procedure;
} Context;

// Global state
static Context context_stack[MAX_CONTEXTS];
static int context_top = -1;

static int need_space = 0;
static int last_printed_newline = 1; 
static int prev_token = 0, curr_token = 0, next_token = 0;
extern int num_attr;
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
// Adjust this logic to achieve your desired indentation rules.
static int compute_indent_level(void) {
    if (context_top < 0) return 0;

    ContextType curr_type = context_stack[context_top].type;
    ContextType parent_type = (context_top > 0) ? context_stack[context_top - 1].type : CTX_GLOBAL;
    int base_level = context_stack[context_top].base_indent_level;
    int is_var_prog = context_stack[context_top].is_var_under_program;
    int is_var_proc = context_stack[context_top].is_var_under_procedure;

    int indent = 0;
    switch (curr_type) {
        case CTX_GLOBAL:
            indent = 0;
            break;

        case CTX_PROCEDURE:
            indent = 1;
            break;

        case CTX_VAR_BLOCK:
            indent = base_level;  // 'var' aligned with base level
            break;

        case CTX_VAR_DECL:
            indent = base_level + 1;  // Variables indented under 'var'
            break;

        case CTX_BEGIN_BLOCK:
            // Begin block usually indents one level more than parent, but adjust as needed
            indent = (parent_type == CTX_GLOBAL) ? 1 : base_level;
            break;

        case CTX_IF_THEN:
        case CTX_ELSE_BLOCK:
        case CTX_WHILE_DO:
            // If-then, else, while-do often indent one level beyond their parent statement
            indent = base_level;
            break;
        
        default:
            indent = base_level + 1;
            break;
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
        print_indent();
        last_printed_newline = 0;
        need_space = 0;
    } else if (need_space) {
        printf(" ");
    }
    printf("%s", text);
    need_space = 1;
}

// Update token history to know prev/curr/next tokens
static void update_token_history(int new_token) {
    prev_token = curr_token;
    curr_token = next_token;
    next_token = new_token;
}

// Helper: when we enter a var block, decide indent rules based on current context
static void handle_var_context_on_entry(void) {
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
    if (current_context_type() == CTX_VAR_BLOCK) {
        if (curr_token == TBEGIN || curr_token == TPROCEDURE || curr_token == TEND) {
            pop_context();  // Pop CTX_VAR_BLOCK
        }
    }
}

// Main token printing function
void pretty_print_token(int token) {
    debug_pretty_printf("Token: %s, Prev: %s, CurrCtx: %d\n",
        tokenstr[token],
        tokenstr[prev_token],
        current_context_type());

    // NOTE ON POPPING CONTEXTS:
    // For blocks that end with 'end' (like begin-end, procedure-end, while-do-end),
    // you pop the context at 'end'.
    // For var blocks, you might pop when another block starts (e.g., 'procedure', 'begin', or 'end')
    // For if-then or else blocks without an explicit end, you might pop after the next semicolon or
    // when you know the controlled statement is finished.

    switch (token) {
        // Program structure
        case TPROGRAM:
            print_token("program ");
            need_space = 0; 
            break;

        case TPROCEDURE:
            print_newline();
            // Push procedure context here. It ends at 'end;' that corresponds to this procedure.
            push_context(CTX_PROCEDURE, 1, 0, 0);
            print_token("procedure");
            need_space = 1;
            break;

        case TVAR:
            print_newline();  // Move to a new line before 'var'
            print_token("var");
            push_context(CTX_VAR_BLOCK, compute_indent_level() / INDENT_SPACES, 0, 0);
            print_newline();        // Move to a new line after 'var'
            last_printed_newline = 1;  // Trigger indentation for the next line
            need_space = 0;
            break;

        // Begin-End Blocks
        case TBEGIN:
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                ContextType ctype = current_context_type();
                int new_indent = parent_indent;
                if (ctype == CTX_GLOBAL) {
                    // Program-level begin
                    new_indent = 0;
                } else if (ctype == CTX_PROCEDURE) {
                    // Procedure-level begin
                    new_indent = 1;
                } else {
                    // Nested begin block, one more level
                    new_indent = parent_indent + 1;
                }

                print_newline();
                // Push a begin block. This will be popped when we see an TEND token.
                push_context(CTX_BEGIN_BLOCK, new_indent, 0, 0);
                print_token("begin");
                print_newline();
            }
            need_space = 0;
            break;

        case TEND:
            // Pop the matching block.
            // If top is CTX_BEGIN_BLOCK, this end matches a begin-end.
            // If top is CTX_PROCEDURE and this end is at the end of a procedure, pop it.
            // If top is CTX_WHILE_DO, this end matches while-do-end.

            // In practice, after you know your grammar, you might check tokenstr[next_token]
            // or use a parser approach to see if this end corresponds to a procedure end or a block end.
            // For now, assume it's a begin-end block.
            if (current_context_type() == CTX_BEGIN_BLOCK) {
                pop_context();
            } else if (current_context_type() == CTX_PROCEDURE) {
                // If this 'end' corresponds to the end of a procedure,
                // you might check if next_token is TSEMI or TDOT to confirm.
                pop_context();
            } else if (current_context_type() == CTX_WHILE_DO) {
                pop_context();
            }

            print_newline();
            print_token("end");
            need_space = 0;
            break;

        // Statements
        case TIF:
            print_newline();
            // 'if' doesn't always create a block that ends with 'end'.
            // Instead, after 'then', we push CTX_IF_THEN. 
            // We might pop it after the statement following 'then' completes.
            print_token("if");
            break;

        case TTHEN:
            print_token("then");
            {
                // After then, we indent one more level.
                int parent_indent = context_stack[context_top].base_indent_level;
                // Push if-then block. We pop it once the 'then' statement is complete,
                // often after seeing a semicolon or next block start.
                push_context(CTX_IF_THEN, parent_indent + 1, 0, 0);
            }
            print_newline();
            need_space = 0;
            break;

        case TELSE:
            // For else, we first pop the if-then block context since else ends the then-part.
            if (current_context_type() == CTX_IF_THEN) {
                pop_context();
            }
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                // Push else block. Similar to if-then, we pop it after the else statement finishes.
                push_context(CTX_ELSE_BLOCK, parent_indent + 1, 0, 0);
                print_newline();
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
            print_token("do");
            {
                int parent_indent = context_stack[context_top].base_indent_level;
                // After do, push CTX_WHILE_DO. We'll pop it at the matching 'end' that closes this loop.
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
            printf(" %s ", tokenstr[token]);
            need_space = 0;
            last_printed_newline = 0;
            break;

        // Punctuation
        case TSEMI:
            printf(";");
            need_space = 0;
            if (current_context_type() == CTX_VAR_DECL) {
                // Check next token to decide whether to stay or pop context
                if (next_token == TNAME) {
                    // More variables to declare; stay in CTX_VAR_DECL
                    print_newline();
                    last_printed_newline = 1;
                } else {
                    // No more variables; pop CTX_VAR_DECL
                    pop_context();  // Pop CTX_VAR_DECL
                    print_newline();
                    last_printed_newline = 1;
                }
            } else {
                // Handle semicolon in other contexts
                print_newline();
                last_printed_newline = 1;
            }
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
            print_newline();
            print_token(tokenstr[token]);
            need_space = 1;
            break;

        // Values and identifiers
        case TNAME:
            if (current_context_type() == CTX_VAR_BLOCK) {
                // First variable name after 'var'
                if (prev_token == TVAR || prev_token == TSEMI) {
                    // Start variable declarations
                    push_context(CTX_VAR_DECL, context_stack[context_top].base_indent_level, 0, 0);
                    print_newline();
                    last_printed_newline = 1;
                }
            }
            if (need_space && !last_printed_newline) printf(" ");
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
        // Check if we need to exit the var context after processing the token
        handle_var_context_on_exit();
        update_token_history(scan());
    }
}

/*
 * COMMENTS AND FUTURE DIRECTIONS:
 * 
 * 1. Deciding When to Pop Context:
 *    - BEGIN-END: Pop on 'end' token.
 *    - PROCEDURE: Pop on the 'end' that terminates the procedure. 
 *      You may need to look ahead to confirm it's the procedure end, or rely on grammar rules.
 *    - WHILE-DO: Pop on 'end' that closes the while block.
 *    - VAR Block: Pop when you move out of var declarations (e.g., on 'begin', 'procedure', or 'end').
 *    - IF-THEN: Pop after the then-statement ends. If the statement is a single line ending in ';', pop after printing that semicolon.
 *      If it's a begin-end block, pop after the end of that block.
 *    - ELSE: Same logic as IF-THEN. Once else-statement finishes, pop CTX_ELSE_BLOCK.
 * 
 * 2. Look for patterns in the grammar:
 *    You may need to detect the end of a statement or var block more explicitly. 
 *    Sometimes this requires looking at next_token or using a small state machine.
 * 
 * 3. AST Approach:
 *    Another approach would be to build a parse tree first, then pretty-print from the AST, 
 *    which can make it much easier to know exactly when blocks start and end.
 * 
 * 4. Testing:
 *    Test with small programs and incrementally adjust the logic and indentation rules.
 * 
 */
