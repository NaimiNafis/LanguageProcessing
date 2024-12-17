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

// Forward declarations for grammar rule functions
static int parse_block(void);
static int parse_variable_declaration_section(void);
static int parse_list_of_variable_names(void);
static int parse_type(void);
static int parse_standard_type(void);
static int parse_array_type(void);
static int parse_subprogram_declaration(void);
static int parse_formal_parameter_section(void);
static int parse_compound_statement(void);
static int parse_statement(void);
static int parse_assignment_statement(void);
static int parse_conditional_statement(void);
static int parse_iteration_statement(void);
static int parse_exit_statement(void);
static int parse_procedure_call_statement(void);
static int parse_return_statement(void);
static int parse_input_statement(void);
static int parse_output_statement(void);
static int parse_empty_statement(void);
static int parse_variable(void);
static int parse_expression(void);
static int parse_simple_expression(void);
static int parse_term(void);
static int parse_factor(void);
static int parse_output_format(void);
static int parse_list_of_expressions(void);
static int parse_condition(void);
static int parse_statement_list(void);
static int parse_left_hand_part(void);

// Helper functions
static int match(int expected_token);
static int is_standard_type(int token);
static int is_relational_operator(int token);
static int is_additive_operator(int token);
static int is_multiplicative_operator(int token);

// Parser state
static Parser parser;
static int in_while_loop = 0;

// Global variable to track format specifier presence
static int format_specifier_present = 0;

void init_parser(void) {
    parser.current_token = scan();
    parser.line_number = get_linenum();
    parser.first_error_line = 0;
    parser.previous_token = 0;
    parser.previous_previous_token = 0;
}

// Main program parsing
int parse_program(void) {
    parser.first_error_line = 0;  // Reset error state
    
    // Set up error handling
    int error_line = setjmp(parser.error_jmp);
    if (error_line != 0) {
        return error_line;
    }

    // Program structure according to grammar:
    // Program ::= "program" "name" ";" Block "."
    if (match(TPROGRAM) == ERROR) return ERROR;
    if (match(TNAME) == ERROR) return ERROR;
    if (match(TSEMI) == ERROR) return ERROR;
    if (parse_block() == ERROR) return ERROR;
    if (match(TDOT) == ERROR) return ERROR;

    return 0;  // Success
}

static int parse_block(void) {
    // Block ::= { Variable declaration section | Subprogram declaration } Compound statement
    
    // Handle variable declarations and subprograms
    while (parser.current_token == TVAR || parser.current_token == TPROCEDURE) {
        if (parser.current_token == TVAR) {
            if (parse_variable_declaration_section() == ERROR) return ERROR;
        } else {
            if (parse_subprogram_declaration() == ERROR) return ERROR;
        }
    }
    
    // Parse compound statement
    return parse_compound_statement();
}

static int parse_variable_declaration_section(void) {
    if (match(TVAR) == ERROR) return ERROR;
    
    do {
        int line_num;
        char* var_names[MAXSTRSIZE];  // Array to store variable names
        int var_line_nums[MAXSTRSIZE];  // Array to store line numbers
        int var_count = 0;
        
        // Store first variable name and its line number
        var_names[var_count] = strdup(string_attr);
        var_line_nums[var_count] = get_linenum();
        var_count++;
        
        if (match(TNAME) == ERROR) {
            for (int i = 0; i < var_count; i++) free(var_names[i]);
            return ERROR;
        }
        
        // Handle additional variable names
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) {
                for (int i = 0; i < var_count; i++) free(var_names[i]);
                return ERROR;
            }
            
            // Store additional variable name with its line number
            var_names[var_count] = strdup(string_attr);
            var_line_nums[var_count] = get_linenum();
            var_count++;
            
            if (match(TNAME) == ERROR) {
                for (int i = 0; i < var_count; i++) free(var_names[i]);
                return ERROR;
            }
        }
        
        if (match(TCOLON) == ERROR) {
            for (int i = 0; i < var_count; i++) free(var_names[i]);
            return ERROR;
        }
        
        // Save the type before parsing it
        int var_type = parser.current_token;
        if (parse_type() == ERROR) {
            for (int i = 0; i < var_count; i++) free(var_names[i]);
            return ERROR;
        }
        
        // Add all variables with their type and individual line numbers
        for (int i = 0; i < var_count; i++) {
            add_symbol(var_names[i], var_type, var_line_nums[i], 1);
            free(var_names[i]);
        }
        
        if (match(TSEMI) == ERROR) return ERROR;
    } while (parser.current_token == TNAME);
    
    return NORMAL;
}

static int parse_type(void) {
    if (is_standard_type(parser.current_token)) {
        return parse_standard_type();
    }
    return parse_array_type();
}

static int parse_standard_type(void) {
    switch (parser.current_token) {
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            return match(parser.current_token);
        default:
            parse_error("Expected standard type");
            return ERROR;
    }
}

static int parse_array_type(void) {
    if (match(TARRAY) == ERROR) return ERROR;
    if (match(TLSQPAREN) == ERROR) return ERROR;
    
    // Save array size before matching
    int size = num_attr;
    if (match(TNUMBER) == ERROR) return ERROR;
    if (match(TRSQPAREN) == ERROR) return ERROR;
    if (match(TOF) == ERROR) return ERROR;
    
    // Save base type
    int base_type = parser.current_token;
    int result = parse_standard_type();
    
    // Set array info for cross-referencer
    set_array_info(size, base_type);
    
    return result;
}

static int parse_expression(void) {
    // First simple expression
    if (parse_simple_expression() == ERROR) return ERROR;
    
    // Handle relational operators with left associativity
    if (is_relational_operator(parser.current_token)) {
        int op = parser.current_token;
        if (match(op) == ERROR) return ERROR;
        
        // Right operand
        if (parse_simple_expression() == ERROR) return ERROR;
        
        // Continue chaining for left associativity
        while (is_relational_operator(parser.current_token)) {
            op = parser.current_token;
            if (match(op) == ERROR) return ERROR;
            if (parse_simple_expression() == ERROR) return ERROR;
        }
    }
    return NORMAL;
}

static int parse_simple_expression(void) {
    if (parser.current_token == TPLUS || parser.current_token == TMINUS) {
        if (match(parser.current_token) == ERROR) return ERROR;
    }
    
    if (parse_term() == ERROR) return ERROR;
    
    while (is_additive_operator(parser.current_token)) {
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_term() == ERROR) return ERROR;
    }
    return NORMAL;
}

static int parse_term(void) {
    if (parse_factor() == ERROR) return ERROR;
    
    while (is_multiplicative_operator(parser.current_token)) {
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_factor() == ERROR) return ERROR;
    }
    return NORMAL;
}

static int parse_statement(void) {
    switch (parser.current_token) {
        case TNAME:
            return parse_assignment_statement();
        case TIF:
            return parse_conditional_statement();
        case TWHILE:
            return parse_iteration_statement();
        case TBREAK:
            return parse_exit_statement();
        case TCALL:
            return parse_procedure_call_statement();
        case TRETURN:
            return parse_return_statement();
        case TREAD:
        case TREADLN:
            return parse_input_statement();
        case TWRITE:
        case TWRITELN:
            return parse_output_statement();
        case TBEGIN:
            return parse_compound_statement();
        case TSEMI:
            return parse_empty_statement();
        default:
            parse_error("Invalid statement");
            return ERROR;
    }
}

static int parse_variable_declaration(void) {
    // Match VAR keyword
    if (parser.current_token != TVAR) {
        parse_error("Expected 'var' keyword");
        return ERROR;
    }
    match(TVAR);

    // Loop to handle multiple variable declarations
    do {
        // Parse first variable name
        if (parser.current_token != TNAME) {
            parse_error("Variable name expected");
            return ERROR;
        }
        match(TNAME);

        // Handle multiple variables separated by commas
        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            if (parser.current_token != TNAME) {
                parse_error("Variable name expected after comma");
                return ERROR;
            }
            match(TNAME);
            // Extra validation after each variable in the list
            if (parser.current_token != TCOMMA && parser.current_token != TCOLON) {
                parse_error("Expected comma or colon after variable name");
                return ERROR;
            }
        }

        // Stricter checking for colon
        if (parser.current_token != TCOLON) {
            parse_error("Expected ':' after variable names");
            return ERROR;
        }
        match(TCOLON);

        // More strict type checking
        if (parser.current_token != TINTEGER && 
            parser.current_token != TBOOLEAN && 
            parser.current_token != TCHAR && 
            parser.current_token != TARRAY) {
            parse_error("Invalid type declaration");
            return ERROR;
        }

        if (parse_type() == ERROR) {
            return ERROR;
        }

        if (parser.current_token != TSEMI) {
            parse_error("Expected semicolon after variable declaration");
            return ERROR;
        }
        match(TSEMI);

    } while (parser.current_token == TNAME);

    return NORMAL;
}

static int parse_name_list(void) {
    match(TNAME);
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        match(TNAME);
    }
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

static int parse_if_statement(void) {
    debug_printf("Entering parse_if_statement\n");
    
    /* Match 'if' keyword */
    if (match(TIF) == ERROR) {
        parse_error("Expected 'if' keyword");
        return ERROR;
    }

    /* Parse condition */
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file in condition");
        return ERROR;
    }
    if (parse_condition() == ERROR) {
        parse_error("Invalid condition in if statement");
        return ERROR;
    }

    /* Match 'then' keyword */
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file before 'then'");
        return ERROR;
    }
    if (match(TTHEN) == ERROR) {
        parse_error("Expected 'then' after condition"); 
        return ERROR;
    }

    /* Parse then-clause */
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file in then-clause");
        return ERROR;
    }
    if (parser.current_token == TBEGIN) {
        if (parse_block() == ERROR) {
            parse_error("Invalid block in then-clause");
            return ERROR;
        }
    } else {
        if (parse_statement() == ERROR) {
            parse_error("Invalid statement in then-clause");
            return ERROR;
        }
    }

    /* Handle optional else-clause */
    if (parser.current_token == TELSE) {
        if (match(TELSE) == ERROR) {
            parse_error("Error parsing else keyword");
            return ERROR;
        }
        
        if (parser.current_token == -1) {
            parse_error("Unexpected end of file in else-clause");
            return ERROR;
        }
        
        if (parser.current_token == TBEGIN) {
            if (parse_block() == ERROR) {
                parse_error("Invalid block in else-clause");
                return ERROR;
            }
        } else {
            if (parse_statement() == ERROR) {
                parse_error("Invalid statement in else-clause");
                return ERROR;
            }
        }
    }

    debug_printf("Exiting parse_if_statement successfully\n");
    return NORMAL;
}

static int parse_while_statement(void) {
    debug_printf("\n=== Entering parse_while_statement ===\n");
    debug_printf("Token state: current=%d, prev=%d, prev_prev=%d\n",
                parser.current_token, parser.previous_token, 
                parser.previous_previous_token);
    
    in_while_loop++;
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
    
    in_while_loop--;
    debug_printf("=== Exiting parse_while_statement ===\n\n");
    return NORMAL;
}

static int parse_procedure_call_statement(void) {
    if (match(TCALL) == ERROR) return ERROR;
    
    // Save procedure name and line number before matching
    char* proc_name = strdup(string_attr);
    int line_num = get_linenum();
    
    if (match(TNAME) == ERROR) {
        free(proc_name);
        return ERROR;
    }
    
    // Add reference to called procedure
    add_reference(proc_name, line_num);
    free(proc_name);
    
    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        if (parse_list_of_expressions() == ERROR) return ERROR;
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
            if (strlen(string_attr) == 1) {
                // Single character strings should be treated as expressions
                if (parse_expression() == ERROR) return ERROR;
            } else {
                if (match(TSTRING) == ERROR) return ERROR;
            }
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
                if (strlen(string_attr) == 1) {
                    // Single character strings should be treated as expressions
                    if (parse_expression() == ERROR) return ERROR;
                } else {
                    if (match(TSTRING) == ERROR) return ERROR;
                }
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
    // Add reference for variable usage
    char* var_name = strdup(string_attr);
    int line_num = get_linenum();
    
    if (match(TNAME) == ERROR) return ERROR;
    
    // Optional array indexing
    if (parser.current_token == TLSQPAREN) {
        if (match(TLSQPAREN) == ERROR) return ERROR;
        if (parse_expression() == ERROR) return ERROR;
        if (match(TRSQPAREN) == ERROR) return ERROR;
    }
    
    add_reference(var_name, line_num);
    free(var_name);
    return NORMAL;
}

// Helper functions to check operator types
static int is_relational_operator(int token) {
    return token == TEQUAL || token == TNOTEQ || 
           token == TLE || token == TLEEQ || 
           token == TGR || token == TGREQ;
}

static int is_additive_operator(int token) {
    return token == TPLUS || token == TMINUS || token == TOR;
}

static int is_multiplicative_operator(int token) {
    return token == TSTAR || token == TDIV || token == TAND;
}

static int is_standard_type(int token) {
    return token == TINTEGER || token == TBOOLEAN || token == TCHAR;
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
            
            // Support for operations after parenthesis
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
    set_error_state();  // Notify cross-referencer of error
    
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

static int parse_procedure(void) {
    debug_printf("Entering parse_procedure\n");
    
    if (match(TPROCEDURE) == ERROR) {
        parse_error("Expected 'procedure' keyword");
        return ERROR;
    }

    // Save procedure name before matching
    char* proc_name = strdup(string_attr);
    int def_line = get_linenum();  // Get the definition line
    
    if (match(TNAME) == ERROR) {
        free(proc_name);
        return ERROR;
    }
    
    // Add procedure to symbol table before entering its scope
    add_symbol(proc_name, TPROCEDURE, def_line, 1);
    enter_procedure(proc_name);
    free(proc_name);

    // Parse parameters if present
    if (parser.current_token == TLPAREN) {
        if (parse_formal_parameter_section() == ERROR) return ERROR;
    }
    
    if (match(TSEMI) == ERROR) return ERROR;
    if (parse_block() == ERROR) return ERROR;
    if (match(TSEMI) == ERROR) return ERROR;
    
    exit_procedure();  // Exit procedure scope
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

// 1. Output Format Grammar Rule
static int parse_output_format(void) {
    debug_printf("Parsing output format, current token: %d\n", parser.current_token);
    
    if (parser.current_token == TSTRING) {
        // Get string length
        int str_len = strlen(string_attr);
        debug_printf("String length: %d\n", str_len);
        
        if (match(TSTRING) == ERROR) return ERROR;
        
        // Multi-character strings cannot have format specifiers according to grammar
        if (str_len > 1) {
            debug_printf("Multi-char string - format specifier not allowed\n");
            return NORMAL;  // Early return for multi-char strings
        }
        
        // Single-character strings can have format specifiers
        if (parser.current_token == TCOLON) {
            if (match(TCOLON) == ERROR) return ERROR;
            if (match(TNUMBER) == ERROR) return ERROR;
        }
        return NORMAL;
    }
    
    // Handle normal expressions
    if (parse_expression() == ERROR) return ERROR;
    if (parser.current_token == TCOLON) {
        if (match(TCOLON) == ERROR) return ERROR;
        if (match(TNUMBER) == ERROR) return ERROR;
    }
    return NORMAL;
}

// List of variable names implementation
static int parse_list_of_variable_names(void) {
    if (parser.current_token != TNAME) {
        parse_error("Expected variable name");
        return ERROR;
    }
    if (match(TNAME) == ERROR) return ERROR;

    while (parser.current_token == TCOMMA) {
        if (match(TCOMMA) == ERROR) return ERROR;
        if (parser.current_token != TNAME) {
            parse_error("Expected variable name after comma");
            return ERROR;
        }
        if (match(TNAME) == ERROR) return ERROR;
    }
    return NORMAL;
}

// Fix compound statement to properly handle empty blocks
static int parse_compound_statement(void) {
    if (match(TBEGIN) == ERROR) return ERROR;
    
    // Handle optional statement list
    if (parser.current_token != TEND) {
        if (parse_statement_list() == ERROR) return ERROR;
    }
    
    return match(TEND);
}

// Assignment statement implementation
static int parse_assignment_statement(void) {
    if (parse_left_hand_part() == ERROR) return ERROR;
    if (match(TASSIGN) == ERROR) return ERROR;
    return parse_expression();
}

// Left-hand part implementation
static int parse_left_hand_part(void) {
    return parse_variable();
}

// Conditional statement implementation
static int parse_conditional_statement(void) {
    if (match(TIF) == ERROR) return ERROR;
    if (parse_expression() == ERROR) return ERROR;
    if (match(TTHEN) == ERROR) return ERROR;
    
    // Parse the then-statement
    if (parse_statement() == ERROR) return ERROR;
    
    // Handle else part - associates with closest if
    if (parser.current_token == TELSE) {
        if (match(TELSE) == ERROR) return ERROR;
        return parse_statement();
    }
    return NORMAL;
}

// Iteration statement implementation
static int parse_iteration_statement(void) {
    if (match(TWHILE) == ERROR) return ERROR;
    if (parse_expression() == ERROR) return ERROR;
    if (match(TDO) == ERROR) return ERROR;
    in_while_loop++;
    int result = parse_statement();
    in_while_loop--;
    return result;
}

// Exit statement implementation
static int parse_exit_statement(void) {
    // Enhanced break statement validation
    if (!in_while_loop) {
        parse_error("Break statement must be directly inside a while loop");
        return ERROR;
    }
    
    return match(TBREAK);
}

// Return statement implementation
static int parse_return_statement(void) {
    return match(TRETURN);
}

// Empty statement implementation
static int parse_empty_statement(void) {
    return NORMAL;  // Empty statement is ε (epsilon)
}

// Input statement implementation
static int parse_input_statement(void) {
    // Handle both read and readln
    if (parser.current_token != TREAD && parser.current_token != TREADLN) {
        parse_error("Expected read or readln");
        return ERROR;
    }
    if (match(parser.current_token) == ERROR) return ERROR;
    
    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        if (parse_variable() == ERROR) return ERROR;
        
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) return ERROR;
            if (parse_variable() == ERROR) return ERROR;
        }
        
        return match(TRPAREN);
    }
    return NORMAL;
}

// Output statement implementation
static int parse_output_statement(void) {
    // Handle both write and writeln
    if (parser.current_token != TWRITE && parser.current_token != TWRITELN) {
        parse_error("Expected write or writeln");
        return ERROR;
    }
    if (match(parser.current_token) == ERROR) return ERROR;
    
    if (parser.current_token == TLPAREN) {
        if (match(TLPAREN) == ERROR) return ERROR;
        if (parse_output_format() == ERROR) return ERROR;
        
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) return ERROR;
            if (parse_output_format() == ERROR) return ERROR;
        }
        
        return match(TRPAREN);
    }
    return NORMAL;
}

// Subprogram declaration implementation
static int parse_subprogram_declaration(void) {
    if (match(TPROCEDURE) == ERROR) return ERROR;

    // Save procedure name and line number before matching
    char* proc_name = strdup(string_attr);
    int def_line = get_linenum();

    if (match(TNAME) == ERROR) {
        free(proc_name);
        return ERROR;
    }
    
    // Add procedure to symbol table and enter its scope
    add_symbol(proc_name, TPROCEDURE, def_line, 1);
    enter_procedure(proc_name);
    free(proc_name);
    
    // Handle optional formal parameter section
    if (parser.current_token == TLPAREN) {
        if (parse_formal_parameter_section() == ERROR) return ERROR;
    }
    
    if (match(TSEMI) == ERROR) return ERROR;
    if (parse_block() == ERROR) return ERROR;
    
    exit_procedure();  // Exit procedure scope
    return match(TSEMI);
}

// Formal parameter section implementation
static int parse_formal_parameter_section(void) {
    if (match(TLPAREN) == ERROR) return ERROR;
    
    do {
        int param_line = get_linenum();
        char* param_names[MAXSTRSIZE];
        int param_count = 0;
        
        // Store parameter names
        param_names[param_count] = strdup(string_attr);
        param_count++;
        if (match(TNAME) == ERROR) {
            for (int i = 0; i < param_count; i++) free(param_names[i]);
            return ERROR;
        }
        
        while (parser.current_token == TCOMMA) {
            if (match(TCOMMA) == ERROR) {
                for (int i = 0; i < param_count; i++) free(param_names[i]);
                return ERROR;
            }
            param_names[param_count] = strdup(string_attr);
            param_count++;
            if (match(TNAME) == ERROR) {
                for (int i = 0; i < param_count; i++) free(param_names[i]);
                return ERROR;
            }
        }
        
        if (match(TCOLON) == ERROR) {
            for (int i = 0; i < param_count; i++) free(param_names[i]);
            return ERROR;
        }
        
        // Get parameter type
        int param_type = parser.current_token;
        if (parse_type() == ERROR) {
            for (int i = 0; i < param_count; i++) free(param_names[i]);
            return ERROR;
        }
        
        // Add parameters to symbol table
        for (int i = 0; i < param_count; i++) {
            add_symbol(param_names[i], param_type, param_line, 1);
            add_procedure_parameter(param_type);
            free(param_names[i]);
        }
        
    } while (parser.current_token == TSEMI && match(TSEMI) == NORMAL);
    
    return match(TRPAREN);
}

static int parse_factor(void) {
    switch (parser.current_token) {
        case TNAME:
            return parse_variable();
            
        case TNUMBER:
        case TTRUE:
        case TFALSE:
        case TSTRING:
            return match(parser.current_token);
            
        case TLPAREN:
            if (match(TLPAREN) == ERROR) return ERROR;
            if (parse_expression() == ERROR) return ERROR;
            return match(TRPAREN);
            
        case TNOT:
            if (match(TNOT) == ERROR) return ERROR;
            return parse_factor();
            
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            // Standard type "(" Expression ")"
            if (match(parser.current_token) == ERROR) return ERROR;
            if (match(TLPAREN) == ERROR) return ERROR;
            if (parse_expression() == ERROR) return ERROR;
            return match(TRPAREN);
            
        default:
            parse_error("Invalid factor");
            return ERROR;
    }
}

// List of expressions implementation
static int parse_list_of_expressions(void) {
    if (parse_expression() == ERROR) return ERROR;
    
    while (parser.current_token == TCOMMA) {
        if (match(TCOMMA) == ERROR) return ERROR;
        if (parse_expression() == ERROR) return ERROR;
    }
    return NORMAL;
}

// Fix statement list to handle empty blocks correctly
static int parse_statement_list(void) {
    // Allow empty statement list (begin end)
    if (parser.current_token == TEND) {
        return NORMAL;
    }
    
    if (parse_statement() == ERROR) return ERROR;
    
    while (parser.current_token == TSEMI) {
        if (match(TSEMI) == ERROR) return ERROR;
        if (parser.current_token == TEND) break;  // Allow for empty statement after semicolon
        if (parse_statement() == ERROR) return ERROR;
    }
    
    return NORMAL;
}
