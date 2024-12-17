#include <stdio.h>
#include "pretty.h"
#include "scan.h"
#include "token.h"
#include "parser.h"
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
static void print_context_stack(const char* action);
static const char* context_type_name(ContextType type);

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
        debug_pretty_printf("Pushed context: %s (base_indent=%d, var_prog=%d, var_proc=%d)\n", 
                            context_type_name(type), base_indent, var_prog, var_proc);
        print_context_stack("After push");
    } else {
        debug_pretty_printf("Error: Context stack overflow!\n");
    }
}

static const char* context_type_name(ContextType type) {
    switch(type) {
        case CTX_GLOBAL: return "GLOBAL";
        case CTX_PROCEDURE: return "PROCEDURE";
        case CTX_VAR_BLOCK: return "VAR_BLOCK";
        case CTX_VAR_DECL: return "VAR_DECL";
        case CTX_BEGIN_BLOCK: return "BEGIN_BLOCK";
        case CTX_IF_THEN: return "IF_THEN";
        case CTX_ELSE_BLOCK: return "ELSE_BLOCK";
        case CTX_WHILE_DO: return "WHILE_DO";
        default: return "UNKNOWN";
    }
}

static void pop_context(void) {
    if (context_top >= 0) {
        debug_pretty_printf("Popping context: %s (base_indent=%d)\n",
            context_type_name(context_stack[context_top].type),
            context_stack[context_top].base_indent_level);
        context_top--;
        print_context_stack("After pop");
    } else {
        debug_pretty_printf("Error: Attempted to pop from empty context stack!\n");
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
    } else if (need_space) {
        printf(" ");
    }

    if (curr_token == TSTRING) {
        printf("'");
        int str_len = 0;
        for (int i = 0; string_attr[i] != '\0'; i++) {
            str_len++;
        }
        debug_pretty_printf("String length: %d, next token: %d\n", str_len, next_token);

        // Print string content
        for (int i = 0; i < str_len; i++) {
            if (string_attr[i] == '\'') {
                printf("''");
            } else {
                printf("%c", string_attr[i]);
            }
        }
        printf("'");
        need_space = 1;

        // For multi-character strings:
        // Look ahead to check for format specifiers and skip them
        if (str_len > 1) {
            int format_specifier = 0;
            int temp_token = next_token;
            if (temp_token == TCOLON) {
                format_specifier = 1;
            }
            
            if (format_specifier) {
                debug_pretty_printf("Skipping format specifier for multi-char string\n");
                update_token_history(scan());  // Skip colon
                update_token_history(scan());  // Skip number
            }
        }
        return;  // Important: return here after handling string
    }

    if (curr_token == TCOLON && 
              (prev_token == TNUMBER || 
               (prev_token == TSTRING && strlen(string_attr) == 1))) {
        // Format specifier handling only for expressions and single-char strings
        printf(":");
        need_space = 1;
    } else if (curr_token == TNUMBER && prev_token == TCOLON) {
        printf(" %d", num_attr);  // Single space after colon before number
        need_space = 1;
    } else {
        printf("%s", text);
        need_space = 1;
    }
    
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
            
            // Get base indent level before popping
            int restore_indent = -1;
            ContextType curr_type = current_context_type();
            
            // Find the parent while-do or begin block's indent level to restore after
            for (int i = context_top - 1; i >= 0; i--) {
                if (context_stack[i].type == CTX_WHILE_DO || 
                    (context_stack[i].type == CTX_BEGIN_BLOCK && 
                     i > 0 && context_stack[i-1].type == CTX_WHILE_DO)) {
                    restore_indent = context_stack[i].base_indent_level;
                    break;
                }
            }

            if (curr_type == CTX_IF_THEN || curr_type == CTX_ELSE_BLOCK) {
                // Pop contexts until we find BEGIN_BLOCK
                while (curr_type != CTX_BEGIN_BLOCK && curr_type != CTX_GLOBAL) {
                    pop_context();
                    curr_type = current_context_type();
                }
                print_token("end");
                need_space = 0;
                
                if (curr_type == CTX_BEGIN_BLOCK) {
                    pop_context();
                    curr_type = current_context_type();
                    // If we found a while-do level to restore to, use it
                    if (restore_indent >= 0 && curr_type != CTX_GLOBAL) {
                        context_stack[context_top].base_indent_level = restore_indent;
                    }
                    if (curr_type == CTX_IF_THEN || 
                        curr_type == CTX_ELSE_BLOCK || 
                        curr_type == CTX_WHILE_DO || 
                        curr_type == CTX_PROCEDURE) {
                        pop_context();
                    }
                }
            } else {
                // Original handling for other contexts
                print_token("end");
                need_space = 0;
                
                while (curr_type != CTX_GLOBAL) {
                    if (curr_type == CTX_BEGIN_BLOCK) {
                        pop_context();
                        curr_type = current_context_type();
                        if (curr_type == CTX_IF_THEN || 
                            curr_type == CTX_ELSE_BLOCK || 
                            curr_type == CTX_WHILE_DO || 
                            curr_type == CTX_PROCEDURE) {
                            pop_context();
                        }
                        break;
                    }
                    pop_context();
                    curr_type = current_context_type();
                }
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
            {
                ContextType curr_type = current_context_type();
                int if_indent;
                int matched_if = 0;  // Flag to track if we found matching if

                // Search for matching IF_THEN context
                for (int i = context_top; i >= 0; i--) {
                    if (context_stack[i].type == CTX_IF_THEN) {
                        if_indent = context_stack[i].base_indent_level - 1;
                        matched_if = 1;
                        // Pop contexts until we reach this IF_THEN
                        while (context_top > i) {
                            pop_context();
                        }
                        pop_context();  // Pop the IF_THEN itself
                        break;
                    }
                }

                if (!matched_if) {
                    // Fallback if no matching if found
                    if_indent = current_base_indent();
                }

                // Push new ELSE_BLOCK with same indentation as its corresponding IF
                push_context(CTX_ELSE_BLOCK, if_indent + 1, 0, 0);
                print_newline_if_needed();
                print_token("else");
                need_space = 0;
                print_newline();
            }
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
            need_space = 0;
            print_token(";");
            // Pop all nested IF_THEN and ELSE_BLOCK contexts until we reach a BEGIN_BLOCK or WHILE_DO
            curr_type = current_context_type();
            while (curr_type == CTX_IF_THEN || curr_type == CTX_ELSE_BLOCK) {
                pop_context();
                curr_type = current_context_type();
                if (curr_type == CTX_BEGIN_BLOCK || curr_type == CTX_WHILE_DO) {
                    break;
                }
            }
            print_newline();
            need_space = 0;
            break;

        case TCOMMA:
            print_token(",");
            need_space = 1;
            break;

        case TCOLON:
            if (prev_token == TSTRING || prev_token == TNUMBER) {
                // Handle formatting specifier
                print_token(":");
                need_space = 1;
            } else {
                // Normal colon handling
                print_token(":");
                need_space = 1;
            }
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
            printf("%s", string_attr);  // Use string_attr instead of num_attr
            need_space = 1;
            last_printed_newline = 0;
            break;

        case TSTRING:
            if (need_space) printf(" ");
            printf("'");
            // Loop through string and double any single quotes
            for (int i = 0; string_attr[i] != '\0'; i++) {
                if (string_attr[i] == '\'') {
                    printf("''");  // Print doubled single quote
                } else {
                    printf("%c", string_attr[i]);
                }
            }
            printf("'");
            // Don't print formatting specifier for strings unless explicitly required
            if (curr_token == TCOLON && prev_token == TNUMBER) {
                printf(" : %d", num_attr);
            }
            need_space = 1;
            last_printed_newline = 0;
            break;

        case TBREAK:
            print_newline_if_needed();
            print_token("break");
            need_space = 0;
            break;

        case TRETURN:
            print_newline_if_needed();
            print_token("return");
            need_space = 0;
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
    init_parser();  // Initialize parser first
    
    // Parse the program first to validate syntax
    int error_line = parse_program();
    if (error_line != 0) {
        fprintf(stderr, "Syntax error detected at line %d. Cannot pretty print.\n", error_line);
        return;
    }
    
    // Reset scanner to start of file for pretty printing
    end_scan();
    init_scan(get_current_file());
    
    debug_pretty_printf("Scanning first token for pretty printing...\n");
    next_token = scan();
    if (next_token > 0) {
        curr_token = next_token;
        next_token = scan();
    }
    
    while (curr_token > 0) {
        debug_pretty_printf("Processing token %s (%d)\n", tokenstr[curr_token], curr_token);
        handle_var_end_if_needed();
        pretty_print_token(curr_token);
        update_token_history(scan());
    }
    debug_pretty_printf("Finished pretty_print_program\n");
}

static void print_context_stack(const char* action) {
    debug_pretty_printf("%s - Current context stack (from bottom to top):\n", action);
    if (context_top < 0) {
        debug_pretty_printf("  Stack is empty\n");
    } else {
        for (int i = 0; i <= context_top; i++) {
            debug_pretty_printf("  Level %d: %s (base_indent=%d)\n",
                i,
                context_type_name(context_stack[i].type),
                context_stack[i].base_indent_level);
        }
    }
    debug_pretty_printf("Stack trace end\n");
}
