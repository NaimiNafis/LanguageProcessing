// parser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parser.h"
#include "scan.h"
#include "token.h"

// Forward declarations
static void parse_block(void);
static void parse_var_declarations(void);
static void parse_name_list(void);
static void parse_type(void);
static void parse_procedure(void);
static void parse_parameter_list(void);
static void parse_statement_list(void);
static void parse_statement(void);
static void parse_assignment(void);
static void parse_condition(void);
static void parse_if_statement(void);
static void parse_while_statement(void);
static void parse_procedure_call(void);
static void parse_read_statement(void);
static void parse_write_statement(void);
static void parse_variable(void);
static void parse_expression(void);
static void parse_comparison(void);
static void report_syntax_error(const char* message);

// Parser state
static Parser parser;

// // Debug configuration
// #define DEBUG 1  // Set to 0 to disable debug output

// // Debug print function
// static void debug_print(const char* format, ...) {
//     if (DEBUG) {
//         va_list args;
//         va_start(args, format);
//         vfprintf(stderr, format, args);
//         va_end(args);
//     }
// }

// // Add debug prints in key parsing functions, e.g.:
// void init_parser(void) {
//     parser.current_token = scan();
//     parser.line_number = get_linenum();
//     parser.error_count = 0;
//     debug_print("Parser initialized with first token: %d\n", parser.current_token);  // Added newline character
//     printf("Initial line number: %d\n", parser.line_number);  // Print initial line number
// }

// // Update match function to use debug_print
// static void match(int expected) {
//     debug_print("Line %d: Expecting %s, got %s\n", 
//                 parser.line_number, 
//                 tokenstr[expected], 
//                 tokenstr[parser.current_token]);

//     if (parser.current_token == expected) {
//         parser.current_token = scan();
//         parser.line_number = get_linenum();
//         printf("Line number after matching token: %d\n", parser.line_number);  // Print line number
//     } else {
//         report_syntax_error("Unexpected token");
//     }
// }

void init_parser(void) {
    parser.current_token = scan();
    parser.line_number = get_linenum();
    parser.error_count = 0;
}

static void match(int expected) {
    if (parser.current_token == expected) {
        parser.current_token = scan();
        parser.line_number = get_linenum();
    } else {
        report_syntax_error("Unexpected token");
    }
}

// Error handling
void parse_error(const char* message) {
    fprintf(stderr, "Syntax error at line %d: %s\n", parser.line_number, message);
    parser.error_count++;
    exit(1);
}

// Main program parsing
int parse_program(void) {
    // Expect program declaration
    if (parser.current_token != TPROGRAM) {
        report_syntax_error("program expected");
        return parser.error_count;
    }

    match(TPROGRAM);
    match(TNAME);
    match(TSEMI);
    
    // Parse program block
    parse_block();
    
    // Expect program end
    match(TDOT);
    
    // Check for extra tokens
    if (parser.current_token != -1) {
        report_syntax_error("End of file expected");
    }
    
    return parser.error_count;
}

// Parse block structure - SINGLE IMPLEMENTATION
static void parse_block(void) {
    // Handle global var declarations first
    if (parser.current_token == TVAR) {
        parse_var_declarations();
    }
    
    // Handle procedures and their var declarations
    while (parser.current_token == TPROCEDURE || parser.current_token == TVAR) {
        if (parser.current_token == TPROCEDURE) {
            parse_procedure();
        } else {
            parse_var_declarations();
        }
    }

    // Parse main program body
    match(TBEGIN);
    parse_statement_list();
    match(TEND);
}

// Variable declaration parsing
static void parse_var_declarations(void) {
    while (parser.current_token == TVAR) {
        match(TVAR);
        do {
            parse_name_list();
            match(TCOLON);
            parse_type();
            match(TSEMI);
        } while (parser.current_token == TNAME);
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
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            match(parser.current_token);
            break;
        case TARRAY:
            match(TARRAY);
            match(TLSQPAREN);
            match(TNUMBER);
            match(TRSQPAREN);
            match(TOF);
            if (parser.current_token == TINTEGER || 
                parser.current_token == TBOOLEAN || 
                parser.current_token == TCHAR) {
                match(parser.current_token);
            } else {
                parse_error("Invalid array type");
            }
            break;
        default:
            parse_error("Invalid type declaration");
    }
}

static void parse_procedure(void) {
    match(TPROCEDURE);
    match(TNAME);
    match(TSEMI);
    
    // Handle procedure's local var declarations
    if (parser.current_token == TVAR) {
        parse_var_declarations();
    }
    
    match(TBEGIN);
    parse_statement_list();
    match(TEND);
    match(TSEMI);
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
    parse_statement();
    while (parser.current_token == TSEMI) {
        match(TSEMI);
        parse_statement();
    }
}

static void parse_statement(void) {
    printf("DEBUG: Entering parse_statement with token: %d\n", parser.current_token);

    switch (parser.current_token) {
        case TNAME:
            parse_assignment();
            break;
        case TIF:
            printf("DEBUG: Found IF token\n");
            parser.current_token = scan();
            printf("DEBUG: After IF scan, token is: %d\n", parser.current_token);
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
            parse_read_statement();
            break;
        case TWRITE:
        case TWRITELN:
            parse_write_statement();
            break;
        case TEND:  // Add END token handling
            printf("DEBUG: Found END token in statement\n");
            return;  // Let parent handle END token
        default:
            parse_error("Invalid statement");
            report_syntax_error("Unexpected token");
            break;
    }
}

// Ensure the error reporting function uses the correct line number
void report_syntax_error(const char* message) {
    fprintf(stderr, "Syntax error at line %d: %s\n", get_linenum(), message);
    printf("Line number when reporting syntax error: %d\n", parser.line_number);  // Print line number
    parser.error_count++;
    exit(1);
}

static void parse_assignment(void) {
    parse_variable();
    match(TASSIGN);
    parse_expression();
}

static void parse_if_statement(void) {
    printf("DEBUG: Entering parse_if_statement\n");
    
    parse_condition();
    
    printf("DEBUG: Condition parsed, looking for THEN\n");
    
    if (parser.current_token == TTHEN) {
        match(TTHEN);
        
        if (parser.current_token == TBEGIN) {
            printf("DEBUG: Found BEGIN block\n");
            match(TBEGIN);
            parse_statement_list();
            printf("DEBUG: Looking for END token\n");
            match(TEND);
        } else {
            parse_statement();
        }
        
        // Keep ELSE clause for backward compatibility
        if (parser.current_token == TELSE) {
            printf("DEBUG: Found ELSE clause\n");
            match(TELSE);
            if (parser.current_token == TBEGIN) {
                match(TBEGIN);
                parse_statement_list();
                match(TEND);
            } else {
                parse_statement();
            }
        }
    } else {
        error("Expected 'then' after if condition");
    }
}

static void parse_while_statement(void) {
    match(TWHILE);
    parse_condition();
    match(TDO);
    
    // Handle begin/end block
    if (parser.current_token == TBEGIN) {
        match(TBEGIN);
        parse_statement_list();
        match(TEND);
    } else {
        parse_statement();  // Single statement case
    }
}

// In parser.c
static void parse_condition(void) {
    printf("DEBUG: Parsing condition, current token: %d\n", parser.current_token);
    parse_comparison();
    
    while (parser.current_token == TOR || parser.current_token == TAND) {
        printf("DEBUG: Found %s at line %d\n", 
               parser.current_token == TOR ? "OR" : "AND", 
               get_linenum());
        
        parser.current_token = scan();
        
        printf("DEBUG: Next token after OR/AND: %d at line %d\n", 
               parser.current_token, 
               get_linenum());
        
        if (parser.current_token == TLPAREN) {
            printf("DEBUG: Found opening parenthesis\n");
            parser.current_token = scan();
            parse_comparison();
            if (parser.current_token == TRPAREN) {
                printf("DEBUG: Found closing parenthesis\n");
                parser.current_token = scan();
            } else {
                error("Missing closing parenthesis");
            }
        } else {
            printf("DEBUG: No parenthesis found, continuing with comparison\n");
            parse_comparison();
        }
    }
}

static void parse_comparison(void) {
    parse_expression();
    
    if (parser.current_token == TEQUAL || 
        parser.current_token == TNOTEQ ||
        parser.current_token == TGR ||
        parser.current_token == TGREQ ||
        parser.current_token == TLE ||
        parser.current_token == TLEEQ) {
        parser.current_token = scan();
        parse_expression();
    }
}

static void parse_variable(void) {
    if (parser.current_token == TNAME) {
        parser.current_token = scan();
        if (parser.current_token == TLSQPAREN) {
            parser.current_token = scan();
            parse_expression();
            if (parser.current_token == TRSQPAREN) {
                parser.current_token = scan();
            } else {
                error("Missing closing bracket");
            }
        }
    } else {
        error("Expected variable name");
    }
}

// In parser.c
static void parse_expression(void) {

    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_expression();
        match(TRPAREN);
        
        if (parser.current_token == TOR || parser.current_token == TAND) {
            match(parser.current_token);
            parse_expression();
        }
        return;
    }

    // Handle NOT operator
    if (parser.current_token == TNOT) {
        match(TNOT);
        parse_expression();
        return;
    }

    // Parse terms
    switch (parser.current_token) {
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
    
    // Handle comparison operators
    if (parser.current_token == TEQUAL || 
        parser.current_token == TNOTEQ ||
        parser.current_token == TGR ||
        parser.current_token == TGREQ ||
        parser.current_token == TLE ||
        parser.current_token == TLEEQ) {
        match(parser.current_token);
        parse_expression();
    }

    // Handle arithmetic operators
    while (parser.current_token == TPLUS || 
           parser.current_token == TMINUS || 
           parser.current_token == TSTAR || 
           parser.current_token == TDIV) {
        match(parser.current_token);
        parse_expression();
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
    if (parser.current_token == TREAD)
        match(TREAD);
    else
        match(TREADLN);
    match(TLPAREN);
    parse_variable();
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        parse_variable();
    }
    match(TRPAREN);
}

static void parse_write_statement(void) {
    if (parser.current_token == TWRITE)
        match(TWRITE);
    else
        match(TWRITELN);
    match(TLPAREN);
    parse_expression();
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        parse_expression();
    }
    match(TRPAREN);
}