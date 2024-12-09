#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"
#include "debug_pretty.h"

// For indentation
#define INDENT_SPACES 4
#define MAX_CONTEXTS  100

typedef enum {
    CTX_GLOBAL,
    CTX_PROCEDURE,
    CTX_VAR_BLOCK,
    CTX_VAR_DECL,
    CTX_BEGIN_BLOCK,
    CTX_IF_THEN,
    CTX_ELSE_BLOCK,
    CTX_WHILE_DO
} ContextType;

typedef struct {
    ContextType type;
    int base_indent_level; 
    int is_var_under_program;
    int is_var_under_procedure;
} Context;

static Context context_stack[MAX_CONTEXTS];
static int context_top = -1;

static int need_space = 0;
static int last_printed_newline = 1; 
static int prev_token = 0, curr_token = 0, next_token = 0;
static int in_procedure_header = 0;
extern int num_attr;
extern char string_attr[];
extern char *tokenstr[];

// Forward declarations
static void push_context(ContextType type, int base_indent, int var_prog, int var_proc);
static void pop_context(void);
static ContextType current_context_type(void);
static int compute_indent_level(void);
static void print_indent(void);
static void print_newline(void);
static void print_newline_if_needed(void);
static void print_token(const char *text);
static void update_token_history(int new_token);
static void handle_var_end_if_needed(void);
static int current_base_indent(void);

void init_pretty_printer(void) {
    debug_pretty_printf("Initializing pretty printer\n");
    context_top = -1;
    push_context(CTX_GLOBAL, 0, 0, 0);
    need_space = 0;
    last_printed_newline = 1;
    prev_token = curr_token = next_token = 0;
    in_procedure_header = 0;
    debug_pretty_printf("Initialization complete, global context pushed\n");
}

static void push_context(ContextType type, int base_indent, int var_prog, int var_proc) {
    if (context_top < MAX_CONTEXTS - 1) {
        context_top++;
        context_stack[context_top].type = type;
        context_stack[context_top].base_indent_level = base_indent;
        context_stack[context_top].is_var_under_program = var_prog;
        context_stack[context_top].is_var_under_procedure = var_proc;
        debug_pretty_printf("Pushed context: %d (base_indent=%d, var_prog=%d, var_proc=%d)\n", 
                            type, base_indent, var_prog, var_proc);
    } else {
        debug_pretty_printf("Error: Context stack overflow!\n");
    }
}

static void pop_context(void) {
    if (context_top >= 0) {
        debug_pretty_printf("Popping context: %d (base_indent=%d)\n",
               context_stack[context_top].type,
               context_stack[context_top].base_indent_level);
        context_top--;
    } else {
        debug_pretty_printf("Error: Pop on empty context stack!\n");
    }
}

static ContextType current_context_type(void) {
    if (context_top >= 0) {
        return context_stack[context_top].type;
    }
    return CTX_GLOBAL;
}

static int compute_indent_level(void) {
    if (context_top < 0) return 0;

    ContextType curr_type = context_stack[context_top].type;
    int base_level = context_stack[context_top].base_indent_level;

    if (curr_type == CTX_BEGIN_BLOCK) {
        if (curr_token == TBEGIN || curr_token == TEND) {
            return base_level * INDENT_SPACES;
        } else {
            return (base_level + 1) * INDENT_SPACES;
        }
    } else if (curr_type == CTX_IF_THEN || curr_type == CTX_ELSE_BLOCK) {
        // Contents of if/else blocks are indented one level deeper
        if (curr_token == TELSE) {
            return base_level * INDENT_SPACES;  // 'else' aligns with 'if'
        }
        return (base_level + 1) * INDENT_SPACES;
    } else if (curr_type == CTX_WHILE_DO) {
        if (curr_token == TBEGIN || curr_token == TEND) {
            return base_level * INDENT_SPACES;
        } else {
            return (base_level + 1) * INDENT_SPACES;
        }
    }

    return base_level * INDENT_SPACES;
}

static int current_base_indent(void) {
    if (context_top < 0) return 0;
    return context_stack[context_top].base_indent_level;
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

static void print_newline_if_needed(void) {
    if (!last_printed_newline) {
        print_newline();
    }
}

static void print_token(const char *text) {
    if (last_printed_newline) {
        print_indent();
    } else {
        if (need_space) printf(" ");
    }
    printf("%s", text);
    need_space = 1;
    last_printed_newline = 0;
}

static void update_token_history(int new_token) {
    prev_token = curr_token;
    curr_token = next_token;
    next_token = new_token;
}

static void handle_var_end_if_needed(void) {
    ContextType ctype = current_context_type();
    if (ctype == CTX_VAR_DECL && (curr_token == TSEMI)) {
        pop_context(); 
        debug_pretty_printf("Ended var declaration line on semicolon\n");
    }
    if (current_context_type() == CTX_VAR_DECL && 
        (curr_token == TBEGIN || curr_token == TPROCEDURE || curr_token == TEND)) {
        pop_context(); 
        if (current_context_type() == CTX_VAR_BLOCK) {
            pop_context();
            debug_pretty_printf("Var block ended due to new block start/end\n");
        }
    } else if (current_context_type() == CTX_VAR_BLOCK &&
               (curr_token == TBEGIN || curr_token == TPROCEDURE || curr_token == TEND)) {
        pop_context(); 
        debug_pretty_printf("Var block ended due to new block start/end\n");
    }
}

void pretty_print_token(int token) {
    debug_pretty_printf("Token: %s (%d), Prev: %s (%d), CurrCtx: %d, InProcHeader: %d\n",
        tokenstr[token], token,
        tokenstr[prev_token], prev_token,
        current_context_type(),
        in_procedure_header);

    switch (token) {
        case TPROGRAM:
            print_token("program");
            need_space = 1; 
            break;

        case TPROCEDURE:
            handle_var_end_if_needed();
            print_newline_if_needed();
            // New procedure at indent=1 from global
            push_context(CTX_PROCEDURE, 1, 0, 0);
            in_procedure_header = 1;
            print_token("procedure");
            need_space = 1;
            break;

        case TVAR:
            handle_var_end_if_needed();
            {
                // Determine var indent:
                // If global: indent=1
                // If procedure header: indent=2
                // If after procedure ended (back to global): indent=1
                // If still in procedure body: indent=1 from procedure perspective since outside header
                ContextType ctype = current_context_type();
                int base_indent;
                if (ctype == CTX_GLOBAL) {
                    base_indent = 1;
                } else if (ctype == CTX_PROCEDURE && in_procedure_header == 1) {
                    base_indent = 2; 
                } else {
                    // If not in header now, treat as global-level var
                    base_indent = 1;
                }
                print_newline_if_needed();
                push_context(CTX_VAR_BLOCK, base_indent, (ctype == CTX_GLOBAL), (ctype == CTX_PROCEDURE && in_procedure_header == 1));
            }
            print_token("var");
            need_space = 0;
            print_newline();  
            break;

        case TNAME:
            if (current_context_type() == CTX_VAR_BLOCK) {
                // var decl line is one deeper than var block:
                // var block at base_indent, decl line at base_indent+1
                int var_base = current_base_indent();
                push_context(CTX_VAR_DECL, var_base+1, 0, 0);
                debug_pretty_printf("Started var declarations line\n");
                last_printed_newline = 1; 
            } else if (current_context_type() == CTX_VAR_DECL && prev_token == TSEMI) {
                print_newline_if_needed();
                last_printed_newline = 1;
            }
            print_token(string_attr); 
            need_space = 1;
            break;

        case TASSIGN:
            print_token(":=");
            need_space = 1;
            break;

        case TBEGIN:
            handle_var_end_if_needed();
            print_newline_if_needed();
            {
                ContextType ctype = current_context_type();
                int new_indent;

                // Program's main begin block
                if (ctype == CTX_GLOBAL || context_top == 0) {
                    new_indent = 0;  // Align with program
                } else if (ctype == CTX_PROCEDURE && in_procedure_header == 1) {
                    new_indent = 1;
                    in_procedure_header = 0;
                } else {
                    new_indent = current_base_indent() + 1;
                }

                push_context(CTX_BEGIN_BLOCK, new_indent, 0, 0);
                print_token("begin");
                print_newline();
                need_space = 0;
            }
            break;

        case TEND:
            print_newline_if_needed();
            print_token("end");
            need_space = 0;

            // Pop until we find the matching BEGIN_BLOCK
            while (context_top >= 0 && current_context_type() != CTX_BEGIN_BLOCK) {
                pop_context();
            }

            // Now pop the CTX_BEGIN_BLOCK if present
            if (current_context_type() == CTX_BEGIN_BLOCK) {
                pop_context();  
            }

            // If we ended a procedure or while/do block, pop those too
            if (current_context_type() == CTX_PROCEDURE) {
                pop_context();
                in_procedure_header = 0;
            } else if (current_context_type() == CTX_WHILE_DO) {
                pop_context();
            }
            break;
            
        case TIF:
            print_newline_if_needed();
            print_token("if");
            break;

        case TTHEN:
            print_token("then");
            {
                int parent_indent = current_base_indent();
                push_context(CTX_IF_THEN, parent_indent + 1, 0, 0);
            }
            print_newline();
            need_space = 0;
            break;

        case TELSE:
            if (current_context_type() == CTX_IF_THEN) {
                // Retrieve and store the base indentation level of the 'if' statement
                int if_indent = context_stack[context_top].base_indent_level;
                pop_context();  // Pop CTX_IF_THEN context
                // Push CTX_ELSE_BLOCK with the same base_indent as the 'if'
                push_context(CTX_ELSE_BLOCK, if_indent, 0, 0);
            } else { // outside if-then block context
                int if_indent = current_base_indent();  // Get current base indent
                push_context(CTX_ELSE_BLOCK, if_indent + 1, 0, 0);  // Increase indent level by 1
            }
            print_newline_if_needed();
            print_token("else");
            need_space = 0;
            print_newline();
            break;

        case TWHILE:
            print_newline_if_needed();
            print_token("while");
            need_space = 1;
            break;

        case TDO:
            print_token("do");
            {
                int parent_indent = current_base_indent();
                // while-do block at parent_indent+1
                push_context(CTX_WHILE_DO, parent_indent + 1, 0, 0);
            }
            print_newline();
            need_space = 0;
            break;

        case TCALL:
            print_newline_if_needed();
            print_token("call");
            need_space = 1;
            break;

        case TSEMI:
            printf(";");
            // Pop IF_THEN context if this semicolon ends an if statement
            if (current_context_type() == CTX_IF_THEN || 
                current_context_type() == CTX_ELSE_BLOCK) {
                pop_context();
            }
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

        // Operators with spacing
        case TPLUS:
        case TMINUS:
        case TSTAR:
        case TDIV:
        case TAND:
        case TOR:
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

        case TLPAREN:
            print_token("( ");
            need_space = 0;
            break;

        case TRPAREN:
            print_token(")");
            need_space = 1;
            break;

        case TLSQPAREN:
            print_token("[ ");
            need_space = 0;
            break;

        case TRSQPAREN:
            print_token("]");
            need_space = 1;
            break;

        case TREADLN:
        case TREAD:
        case TWRITE:
        case TWRITELN:
            print_newline_if_needed();
            print_token(tokenstr[token]);
            need_space = 1;
            break;

        case TNUMBER:
            if (need_space) printf(" ");
            printf("%d", num_attr);
            need_space = 1;
            last_printed_newline = 0;
            break;

        case TSTRING:
            if (need_space) printf(" ");
            printf("'");
            // Loop through string and double any single quotes
            for (int i = 0; string_attr[i] != '\0'; i++) {
                if (string_attr[i] == '\'') {
                    printf("''");  // Double the quote
                } else {
                    printf("%c", string_attr[i]);
                }
            }
            printf("'");
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

void pretty_print_program(void) {
    debug_pretty_printf("Starting pretty_print_program\n");
    init_pretty_printer();
    debug_pretty_printf("Scanning first token...\n");
    next_token = scan();
    if (next_token > 0) {
        curr_token = next_token;
        next_token = scan();
    }
    
    while (curr_token > 0) {
        debug_pretty_printf("Processing token %s (%d)\n", tokenstr[curr_token], curr_token);
        // Handle var block endings before printing token
        handle_var_end_if_needed();
        pretty_print_token(curr_token);
        update_token_history(scan());
    }
    debug_pretty_printf("Finished pretty_print_program\n");
}



/*
 * NOTES:
 * - The logic for var and var declarations is now simplified:
 *   - On 'var', push CTX_VAR_BLOCK.
 *   - On first TNAME after var, push CTX_VAR_DECL.
 *   - End of var line (TSEMI) pops CTX_VAR_DECL but stays in CTX_VAR_BLOCK if more vars follow.
 *   - Encountering TBEGIN, TPROCEDURE, or TEND pops CTX_VAR_DECL (if any) and CTX_VAR_BLOCK.
 * 
 * - debug_pretty_printf calls are included at critical steps to help diagnose the flow.
 *   Make sure you have debug_pretty_printf defined and working.
 * 
 * - Adjust indentation rules in compute_indent_level() as needed.
 * 
 * - Test and refine with various sample inputs.
 */


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
