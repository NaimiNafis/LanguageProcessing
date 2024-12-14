#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parser.h"
#include "scan.h"
#include "token.h"
#include "debug.h"
#include "cross_referencer.h"

// Define a constant for EOF
#define EOF_TOKEN -1
#define MAX_ERRORS 5
#define SYNC_TOKENS_COUNT 6

// Forward declarations for parsing functions
static int parse_variable_declaration(void);
static int parse_name_list(void);
static int parse_type(void);
static int parse_procedure(void);
static int parse_procedure_declaration(void);
static int parse_parameter_list(void);
static int parse_statement_list(void);
static int parse_statement(void);
static int parse_if_statement(void);
static int parse_while_statement(void);
static int parse_procedure_call(void);
static int parse_read_statement(void);
static int parse_write_statement(void);
static int parse_variable(void);
static int parse_expression(void);
static int parse_term(void);
static int parse_factor(void);
static int parse_comparison(void);
static int parse_condition(void);
static int parse_assignment(void);
static int match(int expected_token);
static int parse_block(void);
void parse_error(const char* message);

// Add these at the top with other forward declarations
static int check_recursive_call(const char* proc_name);
static int check_malformed_declaration(int start_line);

// Parser state
static Parser parser;

void init_parser(void) {
    parser.current_token = scan();
    parser.line_number = get_linenum();
    parser.first_error_line = 0;
    parser.previous_token = 0;
    parser.previous_previous_token = 0;  // Initialize new field
}

// Main program parsing
int parse_program(void) {
    parser.first_error_line = 0;
    
    // Set up error handling
    int error_line = setjmp(parser.error_jmp);
    if (error_line != 0) {
        return error_line;
    }

    // Check for program keyword
    if (parser.current_token != TPROGRAM) {
        parse_error("program expected");
    }
    match(TPROGRAM);

    // Check for program name (must be identifier)
    if (parser.current_token != TNAME) {
        parse_error("program name expected");
    }
    match(TNAME);

    // Check for semicolon
    if (parser.current_token != TSEMI) {
        parse_error("semicolon expected");
    }
    match(TSEMI);

    // Parse the rest of the program
    parse_block();

    // Check for ending dot
    if (parser.current_token != TDOT) {
        parse_error("period expected at end of program");
    }
    match(TDOT);

    return 0;
}

static int parse_block(void) {
    debug_printf("\n=== Entering parse_block ===\n");
    debug_printf("Current token: %d\n", parser.current_token);
    debug_printf("Previous token: %d\n", parser.previous_token);
    debug_printf("Previous-previous token: %d\n", parser.previous_previous_token);

    // Keep existing debug print
    debug_printf("Entering parse_block\n");

    // Existing VAR and PROCEDURE handling stays exactly the same
    while (parser.current_token == TVAR || parser.current_token == TPROCEDURE) {
        if (parser.current_token == TVAR) {
            match(TVAR);

            // Process variable declarations
            while (parser.current_token == TNAME) {
                // Store the actual line number when we see the variable name
                int var_declaration_line = get_linenum();
                struct VarList {
                    char *name;
                    int line;  // Add line number to the structure
                    struct VarList *next;
                } *head = NULL;

                // First variable
                char *var_name = strdup(string_attr);
                struct VarList *new_var = malloc(sizeof(struct VarList));
                new_var->name = var_name;
                new_var->line = var_declaration_line;  // Store the line number
                new_var->next = head;
                head = new_var;

                match(TNAME);

                // Additional variables after commas
                while (parser.current_token == TCOMMA) {
                    match(TCOMMA);
                    var_declaration_line = get_linenum();  // Get line for each variable
                    var_name = strdup(string_attr);
                    new_var = malloc(sizeof(struct VarList));
                    new_var->name = var_name;
                    new_var->line = var_declaration_line;  // Store the line number
                    new_var->next = head;
                    head = new_var;
                    match(TNAME);
                }

                // ...process type declaration...
                match(TCOLON);
                int var_type = parser.current_token;
                parse_type();

                // Now add all variables with their individual line numbers
                while (head != NULL) {
                    struct VarList *current = head;
                    add_symbol(current->name, var_type, current->line, 1);  // Use stored line number
                    head = head->next;
                    free(current->name);
                    free(current);
                }

                match(TSEMI);
            }
        }
        
        if (parser.current_token == TPROCEDURE) {
            if (parse_procedure() == ERROR) {
                parse_error("Failed to parse procedure declaration");
                return ERROR;
            }
        }
    }

    // Add new debug print before BEGIN
    debug_printf("Before BEGIN match in parse_block\n");
    debug_printf("Current token: %d\n", parser.current_token);

    if (match(TBEGIN) == ERROR) {
        parse_error("Expected 'begin'");
        return ERROR;
    }
    
    // Add new check here for optional semicolon after BEGIN
    if (parser.current_token == TSEMI) {
        if (match(TSEMI) == ERROR) return ERROR;
    }
    
    parser.previous_token = TBEGIN;

    if (parse_statement_list() == ERROR) {
        parse_error("Failed to parse statement list");
        return ERROR;
    }

    debug_printf("Before END match in parse_block\n");
    
    if (match(TEND) == ERROR) {
        parse_error("Expected 'end'");
        return ERROR;
    }

    // Keep existing exit debug print
    debug_printf("Exiting parse_block with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
    debug_printf("=== Exiting parse_block ===\n\n");
    return NORMAL;
}

// Also modify parse_statement_list to not treat the semicolon after BEGIN as an error
static int parse_statement_list(void) {
    debug_printf("Entering parse_statement_list with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);

    // Change this line
    int in_procedure = (get_current_procedure() != NULL);
    
    int statement_count = 0;

    while (parser.current_token != TEND && 
           parser.current_token != TELSE && 
           parser.current_token != -1) {
           
        // Skip any extra semicolons
        while (parser.current_token == TSEMI) {
            if (match(TSEMI) == ERROR) return ERROR;
        }
        
        int start_line = parser.line_number;
        int statement_type = parser.current_token;
        
        if (parser.current_token != TEND) {
            if (parse_statement() == ERROR) return ERROR;
            statement_count++;
        }

        // Semicolon handling
        if (parser.current_token != TEND && parser.current_token != TELSE) {
            if (statement_type == TIF) {
                // Special handling for if statements
                // Only require semicolon if not the last statement in a block
                if (parser.current_token != TEND && 
                    !(in_procedure && parser.current_token == TEND)) {
                    if (parser.current_token != TSEMI) {
                        parse_error("Missing semicolon between statements");
                        return ERROR;
                    }
                    if (match(TSEMI) == ERROR) return ERROR;
                }
            } else {
                // Normal statement handling
                if (parser.current_token != TSEMI && 
                    !(parser.current_token == TEND && statement_count > 0)) {
                    parse_error("Missing semicolon between statements");
                    return ERROR;
                }
                if (parser.current_token == TSEMI) {
                    if (match(TSEMI) == ERROR) return ERROR;
                }
            }
        }
    }
    return NORMAL;
}

static int parse_variable_declaration(void) {
    if (parser.current_token != TVAR) {
        parse_error("Expected 'var' keyword");
        return ERROR;
    }
    
    // Store the line number at the start of variable declaration
    int var_section_line = get_linenum();
    
    match(TVAR);

    do {
        struct VarList {
            char *name;
            struct VarList *next;
        } *head = NULL;
        
        // Process first variable
        if (parser.current_token != TNAME) {
            parse_error("Variable name expected");
            return ERROR;
        }
        
        // Store first variable
        char *var_name = strdup(string_attr);
        struct VarList *new_var = malloc(sizeof(struct VarList));
        new_var->name = var_name;
        new_var->next = head;
        head = new_var;
        
        match(TNAME);

        // Process additional variables
        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            if (parser.current_token != TNAME) {
                parse_error("Variable name expected after comma");
                return ERROR;
            }
            var_name = strdup(string_attr);
            new_var = malloc(sizeof(struct VarList));
            new_var->name = var_name;
            new_var->next = head;
            head = new_var;
            match(TNAME);
        }

        if (match(TCOLON) == ERROR) return ERROR;
        
        // Get variable type
        int var_type = parser.current_token;
        if (parse_type() == ERROR) return ERROR;
        
        // Add all variables using the stored line number
        while (head != NULL) {
            struct VarList *current = head;
            add_symbol(current->name, var_type, var_section_line, 1);
            head = head->next;
            free(current->name);
            free(current);
        }

        if (match(TSEMI) == ERROR) return ERROR;

        // Add check for malformed declarations
        if (get_linenum() - var_section_line > 1) {
            set_error_state();
            parse_error("Variable declaration must be on a single line");
            return ERROR;
        }

    } while (parser.current_token == TNAME);

    return NORMAL;
}

static int check_malformed_declaration(int start_line) {
    return (get_linenum() - start_line > 1);
}

static int parse_name_list(void) {
    match(TNAME);
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        match(TNAME);
    }
}

static int parse_type(void) {
    if (parser.current_token == TINTEGER || parser.current_token == TBOOLEAN || 
        parser.current_token == TCHAR) {
        int type = parser.current_token;
        match(parser.current_token);
        return type;
    }
    else if (parser.current_token == TARRAY) {
        match(TARRAY);
        
        if (match(TLSQPAREN) == ERROR) {
            parse_error("Expected '[' after ARRAY");
            return ERROR;
        }

        // Store array size before matching the number token
        int array_size = num_attr;  // Get size from scanner's num_attr

        if (parser.current_token != TNUMBER) {
            parse_error("Expected number for array size");
            return ERROR;
        }
        match(TNUMBER);

        // Rest of array parsing
        if (match(TRSQPAREN) == ERROR) return ERROR;
        if (match(TOF) == ERROR) return ERROR;

        int base_type = parser.current_token;
        if (base_type != TINTEGER && base_type != TBOOLEAN && base_type != TCHAR) {
            parse_error("Expected valid base type after OF");
            return ERROR;
        }
        match(parser.current_token);
        
        // Set array info with the stored size
        set_array_info(array_size, base_type);
        return TARRAY;
    }

    parse_error("Invalid type");
    return ERROR;
}

static int parse_parameter_list(void) {
    parse_name_list();
    match(TCOLON);
    parse_type();
    while (parser.current_token == TSEMI) {
        match(TSEMI);
        parse_name_list();
        match(TCOLON);
        parse_type();
    }
}

// Modify the statement case for TNAME to use assignment parsing
static int parse_statement(void) {
    debug_printf("Entering parse_statement with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
                
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file in statement");
        return ERROR;
    }

    switch (parser.current_token) {
        case TNAME:
            if (parse_assignment() == ERROR) return ERROR;
            return NORMAL;
            
        case TIF:
            if (parse_if_statement() == ERROR) return ERROR;
            return NORMAL;
            
        case TWHILE:
            if (parse_while_statement() == ERROR) return ERROR;
            return NORMAL;
            
        case TBREAK:
            if (match(TBREAK) == ERROR) return ERROR;
            return NORMAL;
            
        case TCALL:
            if (parse_procedure_call() == ERROR) return ERROR;
            return NORMAL;
            
        case TRETURN:
            if (match(TRETURN) == ERROR) return ERROR;
            return NORMAL;
            
        case TREAD:
        case TREADLN:
            debug_printf("Matched TREAD or TREADLN with token: %d at line: %d\n", 
                        parser.current_token, parser.line_number);
            if (parse_read_statement() == ERROR) return ERROR;
            return NORMAL;
            
        case TWRITE:
        case TWRITELN:
            if (parse_write_statement() == ERROR) return ERROR;
            return NORMAL;
            
        case TBEGIN:
            if (match(TBEGIN) == ERROR) return ERROR;
            if (parse_statement_list() == ERROR) return ERROR;
            if (match(TEND) == ERROR) return ERROR;
            return NORMAL;
            
        case TEND:
            return NORMAL;
            
        default:
            parse_error("Invalid statement");
            return ERROR;
    }

    debug_printf("Exiting parse_statement with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
    return NORMAL;
}

static int parse_if_statement(void) {
    debug_printf("Entering parse_if_statement\n");
    
    if (match(TIF) == ERROR) return ERROR;
    
    if (parse_condition() == ERROR) return ERROR;
    
    if (match(TTHEN) == ERROR) return ERROR;

    // Parse then-clause
    if (parser.current_token == TBEGIN) {
        if (parse_block() == ERROR) return ERROR;
    } else {
        if (parse_statement() == ERROR) return ERROR;
    }

    // Handle optional else-clause
    if (parser.current_token == TELSE) {
        if (match(TELSE) == ERROR) return ERROR;
        
        if (parser.current_token == TBEGIN) {
            if (parse_block() == ERROR) return ERROR;
        } else {
            if (parse_statement() == ERROR) return ERROR;
        }
    }

    // Special handling for if statements
    // Don't require semicolon if this is the last statement in a block
    if (parser.current_token == TEND) {
        debug_printf("If statement followed by END - no semicolon required\n");
        return NORMAL;
    }

    debug_printf("Exiting parse_if_statement successfully\n");
    return NORMAL;
}

static int parse_while_statement(void) {
    debug_printf("\n=== Entering parse_while_statement ===\n");
    debug_printf("Token state: current=%d, prev=%d, prev_prev=%d\n",
                parser.current_token, parser.previous_token, 
                parser.previous_previous_token);
    
    if (match(TWHILE) == ERROR) return ERROR;
    
    if (parse_condition() == ERROR) return ERROR;
    
    if (match(TDO) == ERROR) return ERROR;
    debug_printf("After DO: token=%d, prev=%d, prev_prev=%d\n",
                parser.current_token, parser.previous_token,
                parser.previous_previous_token);
    
    // Save token state before BEGIN
    int saved_prev = parser.previous_token;
    int saved_prev_prev = parser.previous_previous_token;
    parser.previous_token = TDO;
    
    if (parser.current_token == TBEGIN) {
        debug_printf("Found BEGIN after DO, tokens: current=%d, prev=%d\n",
                    parser.current_token, parser.previous_token);
        
        // No need to update token state here
        if (parse_block() == ERROR) return ERROR;
    } else {
        if (parse_statement() == ERROR) return ERROR;
    }
    
    // Restore token state after block/statement
    parser.previous_token = saved_prev;
    parser.previous_previous_token = saved_prev_prev;
    
    debug_printf("=== Exiting parse_while_statement ===\n\n");
    return NORMAL;
}

static int parse_procedure(void) {
    debug_printf("Entering parse_procedure\n");
    
    if (match(TPROCEDURE) == ERROR) {
        parse_error("Expected 'procedure' keyword");
        return ERROR;
    }

    // Store procedure name before matching
    char *proc_name = strdup(string_attr);
    int def_line = get_linenum();

    if (parser.current_token != TNAME) {
        parse_error("Procedure name expected");
        free(proc_name);
        return ERROR;
    }
    
    // Add procedure definition to symbol table
    add_symbol(proc_name, TPROCEDURE, def_line, 1);
    
    // Enter procedure scope
    enter_procedure(proc_name);
    free(proc_name);
    match(TNAME);

    // Handle parameter list
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        
        do {
            // Store all parameter names in this group
            int param_count = 0;
            char *param_names[10];  // Reasonable limit for parameters in one group
            
            // Parse first parameter name
            if (parser.current_token != TNAME) {
                parse_error("Parameter name expected");
                return ERROR;
            }
            param_names[param_count++] = strdup(string_attr);
            match(TNAME);

            // Parse additional parameter names in this group
            while (parser.current_token == TCOMMA) {
                match(TCOMMA);
                if (parser.current_token != TNAME) {
                    parse_error("Parameter name expected after comma");
                    return ERROR;
                }
                param_names[param_count++] = strdup(string_attr);
                match(TNAME);
            }

            if (match(TCOLON) == ERROR) return ERROR;

            // Store parameter type
            int param_type = parser.current_token;
            if (parse_type() == ERROR) return ERROR;
            
            // Add parameter type to procedure for each parameter in this group
            for (int i = 0; i < param_count; i++) {
                add_procedure_parameter(param_type);
                // Add parameter as a scoped variable
                add_symbol(param_names[i], param_type, def_line, 1);
                free(param_names[i]);
            }

        } while (parser.current_token == TSEMI && match(TSEMI) == NORMAL);

        if (match(TRPAREN) == ERROR) return ERROR;
    }

    // Parse procedure body
    if (match(TSEMI) == ERROR) {
        parse_error("Expected semicolon after procedure header");
        return ERROR;
    }

    if (parse_block() == ERROR) {
        parse_error("Invalid procedure body");
        return ERROR;
    }

    if (match(TSEMI) == ERROR) {
        parse_error("Expected semicolon after procedure body");
        return ERROR;
    }

    // Exit procedure scope
    exit_procedure();

    debug_printf("Exiting parse_procedure successfully\n");
    return NORMAL;
}

static int check_recursive_call(const char* proc_name) {
    const char* current_proc = get_current_procedure();
    if (current_proc && strcmp(proc_name, current_proc) == 0) {
        set_error_state();
        return 1;
    }
    return 0;
}

static int parse_procedure_call(void) {
    if (match(TCALL) == ERROR) return ERROR;

    // Store procedure name before matching
    char *proc_name = strdup(string_attr);
    int call_line = get_linenum();
    
    // Check for recursive call before doing anything else
    if (check_recursive_call(proc_name)) {
        free(proc_name);
        parse_error("Direct recursive call detected");
        return ERROR;
    }
    
    if (match(TNAME) == ERROR) {
        free(proc_name);
        return ERROR;
    }

    // Add procedure reference
    if (check_recursive_call(proc_name)) {
        free(proc_name);
        parse_error("Direct recursive call detected");
        return ERROR;
    }
    add_symbol(proc_name, TPROCEDURE, call_line, 0);
    free(proc_name);
    
    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        if (parse_expression() == ERROR) return ERROR;
        
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) return ERROR;
            if (parse_expression() == ERROR) return ERROR;
        }
        
        if (match(TRPAREN) == ERROR) return ERROR;
    }
    return NORMAL;
}

static int parse_read_statement(void) {
    int is_readln = (parser.current_token == TREADLN);
    if (match(parser.current_token) == ERROR) return ERROR;

    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        
        // Must have at least one variable
        if (parse_variable() == ERROR) return ERROR;

        // Handle multiple variables
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) return ERROR;
            if (parse_variable() == ERROR) return ERROR;
        }

        if (match(TRPAREN) == ERROR) return ERROR;
    }
    return NORMAL;
}

static int parse_write_statement(void) {
    debug_printf("Entering parse_write_statement with token: %d\n", parser.current_token);
    
    // Handle both write and writeln
    int is_writeln = (parser.current_token == TWRITELN);
    if (match(parser.current_token) == ERROR) return ERROR;

    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        
        // Parse first output specification
        if (parser.current_token == TSTRING) {
            if (match(TSTRING) == ERROR) return ERROR;
            if (parser.current_token == TCOLON) {
                if (match(TCOLON) == ERROR) return ERROR;
                if (parser.current_token != TNUMBER) {
                    parse_error("Number expected after colon");
                    return ERROR;
                }
                if (match(TNUMBER) == ERROR) return ERROR;
            }
        } else {
            if (parse_expression() == ERROR) return ERROR;
            if (parser.current_token == TCOLON) {
                if (match(TCOLON) == ERROR) return ERROR;
                if (parser.current_token != TNUMBER) {
                    parse_error("Number expected after colon");
                    return ERROR;
                }
                if (match(TNUMBER) == ERROR) return ERROR;
            }
        }

        // Handle multiple output specifications
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) return ERROR;
            if (parser.current_token == TSTRING) {
                if (match(TSTRING) == ERROR) return ERROR;
                if (parser.current_token == TCOLON) {
                    if (match(TCOLON) == ERROR) return ERROR;
                    if (parser.current_token != TNUMBER) {
                        parse_error("Number expected after colon");
                        return ERROR;
                    }
                    if (match(TNUMBER) == ERROR) return ERROR;
                }
            } else {
                if (parse_expression() == ERROR) return ERROR;
                if (parser.current_token == TCOLON) {
                    if (match(TCOLON) == ERROR) return ERROR;
                    if (parser.current_token != TNUMBER) {
                        parse_error("Number expected after colon");
                        return ERROR;
                    }
                    if (match(TNUMBER) == ERROR) return ERROR;
                }
            }
        }

        if (match(TRPAREN) == ERROR) return ERROR;
    }

    debug_printf("Checking for semicolon after writeln, current token: %d\n", parser.current_token);
    
    // Let parse_statement_list handle semicolons
    return NORMAL;
} 

static int parse_variable(void) {
    char *var_name = strdup(string_attr);
    int current_line = get_linenum();
    debug_printf("Processing variable reference: %s at line %d\n", var_name, current_line);
    
    if (match(TNAME) == ERROR) {
        free(var_name);
        return ERROR;
    }
    
    // Try to add reference - it will handle scoping internally
    add_symbol(var_name, TINTEGER, current_line, 0);
    
    // Store the name before freeing it
    char *stored_name = strdup(var_name);
    free(var_name);
    
    // Handle array indexing
    if (parser.current_token == TLSQPAREN) {
        if (match(TLSQPAREN) == ERROR) {
            free(stored_name);
            return ERROR;
        }
        if (parse_expression() == ERROR) {
            free(stored_name);
            return ERROR;
        }
        if (match(TRSQPAREN) == ERROR) {
            free(stored_name);
            return ERROR;
        }
    }
    
    free(stored_name);
    return NORMAL;
}

static int parse_expression(void) {
    debug_printf("Entering parse_expression with token: %d\n", parser.current_token);
    
    if (parse_term() == ERROR) return ERROR;
    
    while (parser.current_token == TPLUS || 
           parser.current_token == TMINUS || 
           parser.current_token == TEQUAL) {  // Add TEQUAL here
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_term() == ERROR) return ERROR;
    }
    
    debug_printf("Exiting parse_expression\n");
    return NORMAL;
}

// Add this new function to handle assignment expressions
static int parse_assignment(void) {
    if (parse_variable() == ERROR) return ERROR;
    
    if (parser.current_token != TASSIGN) {
        parse_error("Expected ':=' in assignment");
        return ERROR;
    }
    if (match(TASSIGN) == ERROR) return ERROR;
    
    // Now parse the right side of the assignment which could be a comparison
    if (parse_expression() == ERROR) return ERROR;
    
    return NORMAL;
}

static int parse_term(void) {
    debug_printf("Entering parse_term with token: %d\n", parser.current_token);
    
    if (parse_factor() == ERROR) return ERROR;
    
    while (parser.current_token == TSTAR || parser.current_token == TDIV) {
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_factor() == ERROR) return ERROR;
    }
    
    debug_printf("Exiting parse_term\n");
    return NORMAL;
}

static int parse_factor(void) {
    debug_printf("Entering parse_factor with token: %d\n", parser.current_token);
    int type;
    
    switch (parser.current_token) {
        case TLPAREN:
            if (match(TLPAREN) == ERROR) return ERROR;
            if (parse_expression() == ERROR) return ERROR;
            if (match(TRPAREN) == ERROR) return ERROR;
            break;
            
        case TNOT:
            if (match(TNOT) == ERROR) return ERROR;
            if (parse_factor() == ERROR) return ERROR;
            break;
            
        case TMINUS:
            if (match(TMINUS) == ERROR) return ERROR;
            if (parse_factor() == ERROR) return ERROR;
            break;
            
        case TINTEGER:
        case TCHAR:
        case TBOOLEAN:
            type = parser.current_token;
            if (match(parser.current_token) == ERROR) return ERROR;
            if (match(TLPAREN) == ERROR) return ERROR;
            if (parse_expression() == ERROR) return ERROR;
            if (match(TRPAREN) == ERROR) return ERROR;
            break;
            
        case TNAME:
            if (parse_variable() == ERROR) return ERROR;
            // Handle function calls
            if (parser.current_token == TLPAREN) {
                if (match(TLPAREN) == ERROR) return ERROR;
                // Handle arguments if present
                if (parser.current_token != TRPAREN) {
                    if (parse_expression() == ERROR) return ERROR;
                    while (parser.current_token == TCOMMA) {
                        if (match(TCOMMA) == ERROR) return ERROR;
                        if (parse_expression() == ERROR) return ERROR;
                    }
                }
                if (match(TRPAREN) == ERROR) return ERROR;
            }
            break;
            
        case TNUMBER:
        case TSTRING:
        case TTRUE:
        case TFALSE:
            if (match(parser.current_token) == ERROR) return ERROR;
            break;
            
        default:
            parse_error("Invalid factor");
            return ERROR;
    }
    
    debug_printf("Exiting parse_factor\n");
    return NORMAL;
}

static int parse_comparison(void) {
    debug_printf("Entering parse_comparison with token: %d\n", parser.current_token);
    
    // Keep existing parentheses handling
    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        if (parse_expression() == ERROR) return ERROR;
        
        // First check if we have a comparison operator after the expression
        if (parser.current_token == TEQUAL || 
            parser.current_token == TNOTEQ ||
            parser.current_token == TGR ||
            parser.current_token == TGREQ ||
            parser.current_token == TLE ||
            parser.current_token == TLEEQ) {
            if (match(parser.current_token) == ERROR) return ERROR;
            
            // Handle right side of comparison
            if (parser.current_token == TSTRING || parser.current_token == TCHAR) {
                if (match(parser.current_token) == ERROR) return ERROR;
            } else {
                if (parse_expression() == ERROR) return ERROR;
            }
            // Match closing paren after comparison is complete
            if (match(TRPAREN) == ERROR) return ERROR;
        } else {
            // No comparison operator found, just match closing paren
            if (match(TRPAREN) == ERROR) return ERROR;
            
            // Add support for operations after parenthesis
            if (parser.current_token == TSTAR || parser.current_token == TDIV) {
                if (match(parser.current_token) == ERROR) return ERROR;
                if (parse_expression() == ERROR) return ERROR;
            }
            
            // Now check for comparison operator
            if (parser.current_token == TEQUAL || 
                parser.current_token == TNOTEQ ||
                parser.current_token == TGR ||
                parser.current_token == TGREQ ||
                parser.current_token == TLE ||
                parser.current_token == TLEEQ) {
                if (match(parser.current_token) == ERROR) return ERROR;
                if (parse_expression() == ERROR) return ERROR;
            }
        }
    } else {
        // Keep existing non-parentheses handling
        if (parse_expression() == ERROR) return ERROR;
        
        if (parser.current_token == TEQUAL || 
            parser.current_token == TNOTEQ ||
            parser.current_token == TGR ||
            parser.current_token == TGREQ ||
            parser.current_token == TLE ||
            parser.current_token == TLEEQ) {
            if (match(parser.current_token) == ERROR) return ERROR;
            
            if (parser.current_token == TSTRING || parser.current_token == TCHAR) {
                if (match(parser.current_token) == ERROR) return ERROR;
            } else {
                if (parse_expression() == ERROR) return ERROR;
            }
        }
    }
    
    debug_printf("Exiting parse_comparison with token: %d\n", parser.current_token);
    return NORMAL;
}

static int parse_condition(void) {
    debug_printf("Entering parse_condition with token: %d\n", parser.current_token);

    // Parse first comparison
    if (parse_comparison() == ERROR) return ERROR;

    // Handle additional conditions with AND/OR
    while (parser.current_token == TOR || parser.current_token == TAND) {
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_comparison() == ERROR) return ERROR;
    }

    debug_printf("Exiting parse_condition with token: %d\n", parser.current_token);
    return NORMAL;
}

void parse_error(const char* message) {
    int current_line = get_linenum();
    
    // Only store the first error line number
    if (parser.first_error_line == 0) {
        parser.first_error_line = current_line;
        scanner.has_error = 1;  // Signal scanner to stop
        
        // Print error message immediately
        fprintf(stderr, "Syntax error at line %d: %s (token: %d)\n", 
                current_line, message, parser.current_token);
                
        // Jump to error handler with line number
        longjmp(parser.error_jmp, current_line);
    }
    
    // If we already have an error, just jump back
    longjmp(parser.error_jmp, parser.first_error_line);
}

static int match(int expected_token) {
    debug_printf("Matching token: %d, expected: %d at line: %d\n", 
                parser.current_token, expected_token, parser.line_number);
    
    if (parser.current_token != expected_token) {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), 
                "Expected token %d but found %d", 
                expected_token, parser.current_token);
        parse_error(error_msg);
        return ERROR;
    }
    
    // Update token history
    parser.previous_previous_token = parser.previous_token;
    parser.previous_token = parser.current_token;
    
    // Get next token
    int next_token = scan();
    parser.current_token = next_token;
    parser.line_number = get_linenum();
    
    debug_printf("After match: current=%d, prev=%d, prev_prev=%d\n",
                parser.current_token, parser.previous_token,
                parser.previous_previous_token);
    return NORMAL;
}

static int parse_procedure_declaration(void) {
    // Match PROCEDURE keyword
    if (parser.current_token != TPROCEDURE) {
        parse_error("Procedure keyword expected");
        return ERROR;
    }
    parser.current_token = scan();

    // Match procedure name
    if (parser.current_token != TNAME) {
        parse_error("Procedure name expected");
        return ERROR;
    }
    parser.current_token = scan();

    // Match semicolon
    if (parser.current_token != TSEMI) {
        parse_error("Semicolon expected");
        return ERROR;
    }
    parser.current_token = scan();

    // Parse block
    if (parse_block() == ERROR) return ERROR;

    // Match semicolon after block
    if (parser.current_token != TSEMI) {
        parse_error("Semicolon expected");
        return ERROR;
    }
    parser.current_token = scan();

    return NORMAL;
}
