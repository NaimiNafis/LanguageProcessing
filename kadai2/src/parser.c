#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parser.h"
#include "scan.h"
#include "token.h"
#include "debug.h"

// Define a constant for EOF
#define EOF_TOKEN -1
#define MAX_ERRORS 5
#define SYNC_TOKENS_COUNT 6

// Forward declarations for parsing functions
static void parse_block(void);
static void parse_variable_declaration(void);
static void parse_name_list(void);
static void parse_type(void);
static void parse_array_type(void);
static void parse_procedure(void);
static void parse_parameter_list(void);
static void parse_statement_list(void);
static void parse_statement(void);
static void parse_assignment(void);
static void parse_if_statement(void);
static void parse_while_statement(void);
static void parse_procedure_call(void);
static void parse_read_statement(void);
static void parse_write_statement(void);
static void parse_variable(void);
static void parse_expression(void);
static void parse_term(void);
static void parse_factor(void);
static void parse_comparison(void);
static void parse_condition(void);
static void parse_subprogram_declaration(void);
static void parse_formal_parameters(void);
static void parse_variable_names(void);
static void parse_compound_statement(void);
static void parse_input_statement(void);
static void parse_output_statement(void);
static void parse_output_specification(void);
static void match(int expected_token);
void parse_error(const char* message);

// Parser state
static Parser parser;

static const int sync_tokens[] = {
    TEND, TELSE, TSEMI, TBEGIN, TVAR, TPROCEDURE
};

void init_parser(void) {
    parser.current_token = scan();
    parser.line_number = get_linenum();
    parser.error_count = 0;
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

static void parse_block(void) {
    debug_printf("Entering parse_block with token: %d at line: %d\n", parser.current_token, parser.line_number);
    if (parser.current_token == TVAR) {
        parse_variable_declaration();
    }

    while (parser.current_token == TPROCEDURE || parser.current_token == TVAR) {
        if (parser.current_token == TPROCEDURE) {
            parse_procedure();
        } else {
            parse_variable_declaration();
        }
    }

    match(TBEGIN);
    parse_statement_list();
    match(TEND);
    debug_printf("Exiting parse_block with token: %d at line: %d\n", parser.current_token, parser.line_number);
}

static void parse_variable_declaration(void) {
    while (parser.current_token == TVAR) {
        match(TVAR);
        
        do {
            // Parse variable names
            if (parser.current_token != TNAME) {
                parse_error("Variable name expected");
            }
            match(TNAME);
            
            // Handle multiple variables (comma-separated)
            while (parser.current_token == TCOMMA) {
                match(TCOMMA);
                if (parser.current_token != TNAME) {
                    parse_error("Variable name expected after comma");
                }
                match(TNAME);
            }
            
            // Check and parse type declaration
            if (parser.current_token != TCOLON) {
                parse_error("Colon expected");
            }
            match(TCOLON);
            
            parse_type();
            
            if (parser.current_token != TSEMI) {
                parse_error("Semicolon expected");
            }
            match(TSEMI);
            
        } while (parser.current_token == TNAME); // Continue if more variables are declared
    }
}

static void parse_name_list(void) {
    match(TNAME);
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        match(TNAME);
    }
}

static void parse_type(void) {
    switch (parser.current_token) {
        case TARRAY:
            parse_array_type();
            break;
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            match(parser.current_token);
            break;
        default:
            parse_error("Type declaration expected");
    }
}

static void parse_array_type(void) {
    match(TARRAY);
    
    // Check for opening bracket
    if (parser.current_token != TLSQPAREN) {
        parse_error("[ expected in array declaration");
    }
    match(TLSQPAREN);
    
    // Check for array size (unsigned integer)
    if (parser.current_token != TNUMBER) {
        parse_error("Array size expected");
    }
    match(TNUMBER);
    
    // Check for closing bracket
    if (parser.current_token != TRSQPAREN) {
        parse_error("] expected in array declaration");
    }
    match(TRSQPAREN);
    
    // Check for 'of' keyword
    if (parser.current_token != TOF) {
        parse_error("'of' expected in array declaration");
    }
    match(TOF);
    
    // Parse standard type
    switch (parser.current_token) {
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            match(parser.current_token);
            break;
        default:
            parse_error("Standard type expected");
    }
}

static void parse_parameter_list(void) {
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

static void parse_statement_list(void) {
    debug_printf("Entering parse_statement_list with token: %d at line: %d\n", parser.current_token, parser.line_number);
    while (parser.current_token != TEND && parser.current_token != -1) {
        debug_printf("Processing statement with token: %d\n", parser.current_token);
        parse_statement();
        
        if (parser.current_token == TSEMI) {
            debug_printf("Found semicolon, matching TSEMI\n");
            match(TSEMI);
        } else if (parser.current_token == TEND) {
            debug_printf("Found end of statement list\n");
            break;
        } else if (parser.current_token == TELSE || 
                  parser.current_token == TBEGIN || 
                  parser.current_token == TEND ||
                  parser.current_token == TIF) {  // Added TIF here
            // Allow these tokens without requiring semicolon
            continue;
        } else if (parser.current_token == TNAME || 
                  parser.current_token == TREAD || 
                  parser.current_token == TREADLN) {
            // Only check for missing semicolon if we're not starting a new valid statement
            if (parser.current_token == parser.previous_token) {
                parse_error("Missing semicolon");
            }
            continue;
        } else {
            debug_printf("Unexpected token after statement: %d\n", parser.current_token);
            parse_error("Expected semicolon or end of statement list");
        }
    }
    debug_printf("Exiting parse_statement_list with token: %d at line: %d\n", parser.current_token, parser.line_number);
}

static void parse_statement(void) {
    debug_printf("Entering parse_statement with token: %d at line: %d\n", parser.current_token, parser.line_number);
    switch (parser.current_token) {
        case TNAME:
            parse_assignment();
            break;
        case TIF:
            parse_if_statement();
            break;
        case TWHILE:
            parse_while_statement();
            break;
        case TBREAK:
            match(TBREAK);
            break;
        case TCALL:
            parse_procedure_call();
            break;
        case TRETURN:
            match(TRETURN);
            break;
        case TREAD:
        case TREADLN:
            debug_printf("Matched TREAD or TREADLN with token: %d at line: %d\n", parser.current_token, parser.line_number);
            parse_read_statement();
            break;
        case TWRITE:
        case TWRITELN:
            parse_write_statement();
            break;
        case TBEGIN:
            parse_block();
            break;
        case TEND:
            return;
        default:
            parse_error("Invalid statement");
    }
    debug_printf("Exiting parse_statement with token: %d at line: %d\n", parser.current_token, parser.line_number);
}

static void parse_assignment(void) {
    debug_printf("Entering parse_assignment with token: %d\n", parser.current_token);
    
    // Parse variable (including array access)
    parse_variable();
    
    match(TASSIGN);  // Match :=
    parse_expression();
    
    debug_printf("Exiting parse_assignment\n");
}

static void parse_if_statement(void) {
    debug_printf("Entering parse_if_statement with token: %d\n", parser.current_token);
    match(TIF);
    parse_condition();
    match(TTHEN);
    if (parser.current_token == TBEGIN) {
        parse_block();
    } else {
        parse_statement();
    }
    
    if (parser.current_token == TELSE) {
        debug_printf("Found ELSE token\n");
        match(TELSE);
        if (parser.current_token == TBEGIN) {
            parse_block();
        } else {
            parse_statement();
        }
    }
    debug_printf("Exiting parse_if_statement\n");
}
static void parse_while_statement(void) {
    match(TWHILE);
    parse_condition();
    match(TDO);
    if (parser.current_token == TBEGIN) {
        match(TBEGIN);
        parse_statement_list();
        match(TEND);
    } else {
        parse_statement();
    }
}

static void parse_procedure_call(void) {
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

static void parse_read_statement(void) {
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

static void parse_write_statement(void) {
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

static void parse_variable(void) {
    debug_printf("Entering parse_variable with token: %d\n", parser.current_token);
    match(TNAME);
    
    // Handle array indexing
    if (parser.current_token == TLSQPAREN) {
        match(TLSQPAREN);
        parse_expression();  // Parse array index
        match(TRSQPAREN);
    }
    debug_printf("Exiting parse_variable\n");
}

static void parse_expression(void) {
    parse_term();
    while (parser.current_token == TPLUS || parser.current_token == TMINUS) {
        match(parser.current_token);
        parse_term();
    }
}

static void parse_term(void) {
    debug_printf("Entering parse_term with token: %d\n", parser.current_token);
    parse_factor();
    while (parser.current_token == TSTAR || parser.current_token == TDIV) {
        if (parser.current_token == '/') {  // Detect invalid division operator
            parse_error("Invalid operator '/' - use 'div' instead");
        }
        match(parser.current_token);
        parse_factor();
    }
    debug_printf("Exiting parse_term\n");
}

static void parse_factor(void) {
    debug_printf("Entering parse_factor with token: %d\n", parser.current_token);
    int type;
    switch (parser.current_token) {
        case TLPAREN:
            match(TLPAREN);
            parse_expression();
            match(TRPAREN);
            break;
        case TNOT:
            match(TNOT);
            parse_factor();
            break;
        case TMINUS:
            match(TMINUS);
            parse_factor();
            break;
        case TINTEGER:
        case TCHAR:
        case TBOOLEAN:
            type = parser.current_token;
            match(parser.current_token);
            match(TLPAREN);
            parse_expression();
            match(TRPAREN);
            break;
        case TNAME:
        case TNUMBER:
        case TSTRING:
        case TTRUE:
        case TFALSE:
            match(parser.current_token);
            break;
        default:
            parse_error("Invalid expression");
    }
    debug_printf("Exiting parse_factor\n");
}

static void parse_comparison(void) {
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

static void parse_condition(void) {
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

static void match(int expected_token) {
    debug_printf("Matching token: %d, expected token: %d at line: %d\n", parser.current_token, expected_token, parser.line_number);
    if (parser.current_token == expected_token) {
        parser.previous_token = parser.current_token; 
        parser.current_token = scan();
        parser.line_number = get_linenum();
        debug_printf("Next token: %d at line: %d\n", parser.current_token, parser.line_number);
    } else {
        parse_error("Unexpected token");
    }
}


static void parse_procedure(void) {
    debug_printf("Entering parse_procedure with token: %d\n", parser.current_token);

    // Match procedure name
    if (parser.current_token != TNAME) {
        parse_error("Procedure name expected");
    }
    match(TNAME);

    // Handle optional formal parameters
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        
        // Parse first parameter group
        parse_variable_names();
        match(TCOLON);
        parse_type();

        // Parse additional parameter groups
        while (parser.current_token == TSEMI) {
            match(TSEMI);
            parse_variable_names();
            match(TCOLON);
            parse_type();
        }

        match(TRPAREN);
    }

    debug_printf("Exiting parse_procedure\n");
}

static void parse_subprogram_declaration(void) {
    debug_printf("Entering parse_subprogram_declaration\n");

    // Match procedure keyword
    if (parser.current_token != TPROCEDURE) {
        parse_error("'procedure' expected");
    }
    match(TPROCEDURE);

    // Match procedure name
    if (parser.current_token != TNAME) {
        parse_error("Procedure name expected");
    }
    match(TNAME);

    // Parse optional formal parameters
    if (parser.current_token == TLPAREN) {
        parse_formal_parameters();
    }

    // Match semicolon after header
    match(TSEMI);

    // Parse optional variable declarations
    if (parser.current_token == TVAR) {
        parse_variable_declaration();
    }

    // Parse compound statement
    parse_compound_statement();

    // Match final semicolon
    match(TSEMI);

    debug_printf("Exiting parse_subprogram_declaration\n");
}

static void parse_formal_parameters(void) {
    debug_printf("Entering parse_formal_parameters\n");
    
    match(TLPAREN);

    // Parse first parameter group
    parse_variable_names();
    match(TCOLON);
    parse_type();

    // Parse additional parameter groups
    while (parser.current_token == TSEMI) {
        match(TSEMI);
        parse_variable_names();
        match(TCOLON);
        parse_type();
    }

    match(TRPAREN);

    debug_printf("Exiting parse_formal_parameters\n");
}

static void parse_variable_names(void) {
    if (parser.current_token != TNAME) {
        parse_error("Variable name expected");
    }
    match(TNAME);

    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        if (parser.current_token != TNAME) {
            parse_error("Variable name expected after comma");
        }
        match(TNAME);
    }
}

static void parse_compound_statement(void) {
    // Match begin keyword
    if (parser.current_token != TBEGIN) {
        parse_error("'begin' expected");
    }
    match(TBEGIN);

    // Parse first statement
    parse_statement();

    // Parse remaining statements
    while (parser.current_token == TSEMI) {
        match(TSEMI);
        if (parser.current_token != TEND) {
            parse_statement();
        }
    }

    // Match end keyword
    if (parser.current_token != TEND) {
        parse_error("'end' expected");
    }
    match(TEND);
}

static void parse_input_statement(void) {
    debug_printf("Entering parse_input_statement\n");

    // Match read or readln
    if (parser.current_token != TREAD && parser.current_token != TREADLN) {
        parse_error("'read' or 'readln' expected");
    }
    match(parser.current_token);

    // Parse optional variable list
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_variable();

        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            parse_variable();
        }
        match(TRPAREN);
    }

    debug_printf("Exiting parse_input_statement\n");
}

static void parse_output_statement(void) {
    debug_printf("Entering parse_output_statement\n");

    // Match write or writeln
    if (parser.current_token != TWRITE && parser.current_token != TWRITELN) {
        parse_error("'write' or 'writeln' expected");
    }
    match(parser.current_token);

    // Parse optional output specifications
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_output_specification();

        while (parser.current_token == TCOMMA) {
            match(TCOMMA);
            parse_output_specification();
        }
        match(TRPAREN);
    }

    debug_printf("Exiting parse_output_statement\n");
}

static void parse_output_specification(void) {
    debug_printf("Entering parse_output_specification\n");

    if (parser.current_token == TSTRING) {
        match(TSTRING);
    } else {
        parse_expression();
        
        // Handle optional width specification
        if (parser.current_token == TCOLON) {
            match(TCOLON);
            if (parser.current_token != TNUMBER) {
                parse_error("Unsigned integer expected after colon");
            }
            match(TNUMBER);
        }
    }

    debug_printf("Exiting parse_output_specification\n");
}