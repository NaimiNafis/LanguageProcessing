#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parser.h"
#include "scan.h"
#include "token.h"
#include "debug.h"

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
static int match(int expected_token);
static int parse_block(void);
void parse_error(const char* message);

// Parser state
static Parser parser;

void init_parser(void) {
    parser.current_token = scan();
    parser.line_number = get_linenum();
    parser.first_error_line = 0;
    parser.previous_token = 0;
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
    debug_printf("Entering parse_block\n");

    // Handle VAR and PROCEDURE declarations
    while (parser.current_token == TVAR || parser.current_token == TPROCEDURE) {
        if (parser.current_token == TVAR) {
            // Match VAR keyword once
            if (match(TVAR) == ERROR) {
                parse_error("Error parsing var keyword");
                return ERROR;
            }

            // Parse variable declarations until we hit BEGIN or PROCEDURE
            while (parser.current_token == TNAME) {
                debug_printf("Parsing variable declaration starting with: %d\n", parser.current_token);
                
                // Parse first variable name
                match(TNAME);

                // Parse additional variable names
                while (parser.current_token == TCOMMA) {
                    if (match(TCOMMA) == ERROR) {
                        parse_error("Error parsing comma in variable list");
                        return ERROR;
                    }
                    
                    if (parser.current_token != TNAME) {
                        parse_error("Variable name expected after comma");
                        return ERROR;
                    }
                    match(TNAME);
                }

                // Parse type declaration
                if (match(TCOLON) == ERROR) {
                    parse_error("Expected ':' after variable names");
                    return ERROR;
                }

                if (parse_type() == ERROR) {
                    parse_error("Invalid variable type");
                    return ERROR;  
                }

                if (match(TSEMI) == ERROR) {
                    parse_error("Expected semicolon after variable declaration");
                    return ERROR;
                }
            }
        }
        
        // Handle procedure declarations
        if (parser.current_token == TPROCEDURE) {
            if (parse_procedure() == ERROR) {
                parse_error("Failed to parse procedure declaration");
                return ERROR;
            }
        }
    }

    // Parse begin..end block
    if (match(TBEGIN) == ERROR) {
        parse_error("Expected 'begin'");
        return ERROR;
    }

    if (parse_statement_list() == ERROR) {
        parse_error("Failed to parse statement list");
        return ERROR;
    }

    if (match(TEND) == ERROR) {
        parse_error("Expected 'end'");
        return ERROR;
    }

    debug_printf("Exiting parse_block with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
    return NORMAL;
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
        }

        // Type declaration
        if (parser.current_token != TCOLON) {
            parse_error("Expected ':' after variable names");
            return ERROR;
        }
        match(TCOLON);

        // Parse the type
        if (parse_type() == ERROR) {
            parse_error("Invalid type declaration");
            return ERROR;
        }

        // Require semicolon
        if (parser.current_token != TSEMI) {
            parse_error("Expected semicolon after type declaration");
            return ERROR;
        }
        match(TSEMI);

        // Continue if there are more variable declarations
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

static int parse_type(void) {
    if (parser.current_token == TINTEGER || parser.current_token == TBOOLEAN || 
        parser.current_token == TCHAR) {
        match(parser.current_token);
        return NORMAL;
    }
    else if (parser.current_token == TARRAY) {
        match(TARRAY);
        
        if (match(TLSQPAREN) == ERROR) {
            parse_error("Expected '[' after ARRAY");
            return ERROR;
        }

        // Handle array size (number)
        if (parser.current_token != TNUMBER) {
            parse_error("Expected number for array size");
            return ERROR;
        }
        match(TNUMBER);

        if (match(TRSQPAREN) == ERROR) {
            parse_error("Expected ']' after array size");
            return ERROR;
        }

        if (match(TOF) == ERROR) {
            parse_error("Expected 'OF' after array declaration");
            return ERROR;
        }

        // Parse base type
        if (parser.current_token != TINTEGER && 
            parser.current_token != TBOOLEAN && 
            parser.current_token != TCHAR) {
            parse_error("Expected valid base type after OF");
            return ERROR;
        }
        match(parser.current_token);
        
        return NORMAL;
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

static int parse_statement_list(void) {
    debug_printf("Entering parse_statement_list with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);

    // Parse first statement
    if (parse_statement() == ERROR) return ERROR;

    while (parser.current_token != TEND && parser.current_token != -1) {
        debug_printf("Processing statement with token: %d\n", parser.current_token);

        if (parser.current_token == TSEMI) {
            debug_printf("Found semicolon, matching TSEMI\n");
            if (match(TSEMI) == ERROR) return ERROR;
            
            // Check for more statements after semicolon
            if (parser.current_token != TEND) {
                if (parse_statement() == ERROR) return ERROR;
            }
        } else if (parser.current_token == TELSE || 
                  parser.current_token == TBEGIN || 
                  parser.current_token == TEND ||
                  parser.current_token == TIF) {
            // Keep your special case handling for control structures
            debug_printf("Found control structure token: %d\n", parser.current_token);
            break;
        } else if (parser.current_token == TNAME || 
                  parser.current_token == TREAD || 
                  parser.current_token == TREADLN) {
            // Keep your special case handling for statements
            if (parser.current_token == parser.previous_token) {
                parse_error("Missing semicolon");
                return ERROR;
            }
            if (parse_statement() == ERROR) return ERROR;
        } else {
            debug_printf("Unexpected token after statement: %d\n", parser.current_token);
            parse_error("Expected semicolon or end of statement list");
            return ERROR;
        }
    }

    debug_printf("Exiting parse_statement_list with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
    return NORMAL;
}

static int parse_statement(void) {
    debug_printf("Entering parse_statement with token: %d at line: %d\n", 
                parser.current_token, parser.line_number);
                
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file in statement");
        return ERROR;
    }

    switch (parser.current_token) {
        case TNAME:
            if (parse_variable() == ERROR) return ERROR;
            // Handle assignment or procedure call
            if (parser.current_token == TASSIGN) {
                if (match(TASSIGN) == ERROR) return ERROR;
                if (parse_expression() == ERROR) return ERROR;
            } else if (parser.current_token == TLPAREN) {
                if (match(TLPAREN) == ERROR) return ERROR;
                if (parser.current_token != TRPAREN) {
                    if (parse_expression() == ERROR) return ERROR;
                    while (parser.current_token == TCOMMA) {
                        if (match(TCOMMA) == ERROR) return ERROR;
                        if (parse_expression() == ERROR) return ERROR;
                    }
                }
                if (match(TRPAREN) == ERROR) return ERROR;
            }
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
    debug_printf("Entering parse_while_statement\n");
    
    if (match(TWHILE) == ERROR) return ERROR;
    if (parse_condition() == ERROR) return ERROR;
    if (match(TDO) == ERROR) return ERROR;
    
    if (parser.current_token == TBEGIN) {
        if (match(TBEGIN) == ERROR) return ERROR;
        if (parse_statement_list() == ERROR) return ERROR;
        if (match(TEND) == ERROR) return ERROR;
    } else {
        if (parse_statement() == ERROR) return ERROR;
    }
    
    debug_printf("Exiting parse_while_statement\n");
    return NORMAL;
}

static int parse_procedure_call(void) {
    match(TCALL);
    match(TNAME);
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_expression();
        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            parse_expression();
        }
        match(TRPAREN);
    }
}

static int parse_read_statement(void) {
    // Handle both read and readln
    int is_readln = (parser.current_token == TREADLN);
    match(parser.current_token);  // match either TREAD or TREADLN

    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        
        // Must have at least one variable
        parse_variable();

        // Handle multiple variables
        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            parse_variable();
        }

        match(TRPAREN);
    }
}

static int parse_write_statement(void) {
    // Handle both write and writeln
    int is_writeln = (parser.current_token == TWRITELN);
    match(parser.current_token);  // match either TWRITE or TWRITELN

    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        
        // Parse first output specification
        if (parser.current_token == TSTRING) {
            match(TSTRING);
        } else {
            parse_expression();
            // Check for width specification
            if (parser.current_token == TCOLON) {
                match(TCOLON);
                if (parser.current_token != TNUMBER) {
                    parse_error("Number expected after colon");
                }
                match(TNUMBER);
            }
        }

        // Handle multiple output specifications
        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            if (parser.current_token == TSTRING) {
                match(TSTRING);
            } else {
                parse_expression();
                if (parser.current_token == TCOLON) {
                    match(TCOLON);
                    if (parser.current_token != TNUMBER) {
                        parse_error("Number expected after colon");
                    }
                    match(TNUMBER);
                }
            }
        }

        match(TRPAREN);
    }
}

static int parse_variable(void) {
    debug_printf("Entering parse_variable with token: %d\n", parser.current_token);
    
    if (match(TNAME) == ERROR) {
        return ERROR;
    }
    
    // Handle array indexing
    if (parser.current_token == TLSQPAREN) {
        if (match(TLSQPAREN) == ERROR) {
            return ERROR;
        }
        if (parse_expression() == ERROR) {  // Parse array index
            return ERROR;
        }
        if (match(TRSQPAREN) == ERROR) {
            return ERROR;
        }
    }
    
    debug_printf("Exiting parse_variable\n");
    return NORMAL;
}

static int parse_expression(void) {
    debug_printf("Entering parse_expression with token: %d\n", parser.current_token);
    
    if (parse_term() == ERROR) return ERROR;
    
    while (parser.current_token == TPLUS || parser.current_token == TMINUS) {
        if (match(parser.current_token) == ERROR) return ERROR;
        if (parse_term() == ERROR) return ERROR;
    }
    
    debug_printf("Exiting parse_expression\n");
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
    
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_expression();
        
        // Handle comparison operators
        if (parser.current_token == TEQUAL || 
            parser.current_token == TNOTEQ ||
            parser.current_token == TGR ||
            parser.current_token == TGREQ ||
            parser.current_token == TLE ||
            parser.current_token == TLEEQ) {
            match(parser.current_token);
            // Handle character literals after operator
            if (parser.current_token == TSTRING || parser.current_token == TCHAR) {
                match(parser.current_token);
            } else {
                parse_expression();
            }
        }
        match(TRPAREN);
    } else {
        parse_expression();
        // Handle same operators outside parentheses
        if (parser.current_token == TEQUAL || 
            parser.current_token == TNOTEQ ||
            parser.current_token == TGR ||
            parser.current_token == TGREQ ||
            parser.current_token == TLE ||
            parser.current_token == TLEEQ) {
            match(parser.current_token);
            if (parser.current_token == TSTRING || parser.current_token == TCHAR) {
                match(parser.current_token);
            } else {
                parse_expression();
            }
        }
    }
    
    debug_printf("Exiting parse_comparison with token: %d\n", parser.current_token);
}

static int parse_condition(void) {
    parse_comparison();
    while (parser.current_token == TOR || parser.current_token == TAND) {
        match(parser.current_token);
        parse_comparison();
    }
}

void parse_error(const char* message) {
    int current_line = get_linenum();
    
    if (parser.first_error_line == 0) {
        parser.first_error_line = current_line;
        scanner.has_error = 1;  // Signal scanner to stop
    }
    
    fprintf(stderr, "Syntax error at line %d: %s (token: %d)\n", 
            current_line, message, parser.current_token);

    longjmp(parser.error_jmp, parser.first_error_line);
}

static int match(int expected_token) {
    debug_printf("Matching token: %d, expected token: %d at line: %d\n", 
                 parser.current_token, expected_token, parser.line_number);
    
    if (parser.current_token == -1) {
        parse_error("Unexpected end of file");
        return ERROR;
    }

    if (parser.current_token == expected_token) {
        parser.previous_token = parser.current_token;
        int next_token = scan();
        if (next_token == -1 && expected_token != TDOT) { // Allow EOF after final dot
            parse_error("Unexpected end of file");
            return NORMAL;
        }
        parser.current_token = next_token;
        parser.line_number = get_linenum();
    } else {
        parse_error("Unexpected token");
        return ERROR;
    }
}

static int parse_procedure(void) {
    debug_printf("Entering parse_procedure\n");
    
    if (match(TPROCEDURE) == ERROR) {
        parse_error("Expected 'procedure' keyword");
        return ERROR;
    }

    // Parse procedure name
    if (parser.current_token != TNAME) {
        parse_error("Procedure name expected");
        return ERROR;
    }
    match(TNAME);

    // Handle parameter list
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);

        // Parse parameter groups
        do {
            // Parse parameter names
            if (parser.current_token != TNAME) {
                parse_error("Parameter name expected");
                return ERROR;
            }
            match(TNAME);

            while (parser.current_token == TCOMMA) {
                match(TCOMMA);
                if (parser.current_token != TNAME) {
                    parse_error("Parameter name expected after comma");
                    return ERROR;
                }
                match(TNAME);
            }

            // Parse parameter type
            if (match(TCOLON) == ERROR) {
                parse_error("Expected ':' after parameter names");
                return ERROR;
            }

            if (parse_type() == ERROR) {
                parse_error("Invalid parameter type");
                return ERROR;
            }

        } while (parser.current_token == TSEMI && match(TSEMI) == NORMAL); // Continue if more parameter groups

        if (match(TRPAREN) == ERROR) {
            parse_error("Expected ')' after parameters");
            return ERROR;
        }
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

    debug_printf("Exiting parse_procedure successfully\n");
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
