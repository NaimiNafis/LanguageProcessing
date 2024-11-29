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
static void parse_var_declarations(void);
static void parse_name_list(void);
static void parse_type(void);
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
static void match(int expected_token);
void parse_error(const char* message);

// Parser state
static Parser parser;

static const int sync_tokens[] = {
    TSEMI,   // Semicolon
    TEND,    // End
    TBEGIN,  // Begin 
    TDOT,    // Dot
    TELSE,   // Else
    -1       // EOF
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
    if (parser.current_token != TPROGRAM) {
        parse_error("program expected");
        return parser.error_count;
    }

    match(TPROGRAM);
    match(TNAME);
    match(TSEMI);
    parse_block();
    match(TDOT);

    if (parser.current_token != -1) {
        parse_error("End of file expected");
    }

    return parser.error_count;
}

static void parse_block(void) {
    debug_printf("Entering parse_block with token: %d at line: %d\n", parser.current_token, parser.line_number);
    if (parser.current_token == TVAR) {
        parse_var_declarations();
    }

    while (parser.current_token == TPROCEDURE || parser.current_token == TVAR) {
        if (parser.current_token == TPROCEDURE) {
            parse_procedure();
        } else {
            parse_var_declarations();
        }
    }

    match(TBEGIN);
    parse_statement_list();
    match(TEND);
    debug_printf("Exiting parse_block with token: %d at line: %d\n", parser.current_token, parser.line_number);
}

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
        } else if (parser.current_token == TNAME || 
                  parser.current_token == TREAD || 
                  parser.current_token == TREADLN) {
            // For backward compatibility - allow these to continue
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
    match(TNAME);
    match(TASSIGN);
    parse_expression();
}

static void parse_if_statement(void) {
    match(TIF);
    parse_condition();
    match(TTHEN);
    if (parser.current_token == TBEGIN) {
        match(TBEGIN);
        parse_statement_list();
        match(TEND);
    } else {
        parse_statement();
    }
    if (parser.current_token == TELSE) {
        match(TELSE);
        if (parser.current_token == TBEGIN) {
            match(TBEGIN);
            parse_statement_list();
            match(TEND);
        } else {
            parse_statement();
        }
    }
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
    debug_printf("Entering parse_read_statement with token: %d at line: %d\n", parser.current_token, parser.line_number);
    if (parser.current_token == TREAD) {
        match(TREAD);
    } else {
        debug_printf("About to match TREADLN, current token: %d at line: %d\n", parser.current_token, parser.line_number);
        match(TREADLN);
    }
    debug_printf("About to match TLPAREN, current token: %d at line: %d\n", parser.current_token, parser.line_number);
    match(TLPAREN);
    parse_variable();
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        parse_variable();
    }
    match(TRPAREN);
    if (parser.current_token == TSEMI) {
        match(TSEMI);
    }
    debug_printf("Exiting parse_read_statement with token: %d at line: %d\n", parser.current_token, parser.line_number);
}

static void parse_write_statement(void) {
    if (parser.current_token == TWRITE) {
        match(TWRITE);
    } else {
        match(TWRITELN);
    }
    match(TLPAREN);
    parse_expression();
    while (parser.current_token == TCOMMA) {
        match(TCOMMA);
        parse_expression();
    }
    match(TRPAREN);
    if (parser.current_token == TSEMI) {
        match(TSEMI);
    }
}

static void parse_variable(void) {
    match(TNAME);
    if (parser.current_token == TLSQPAREN) {
        match(TLSQPAREN);
        parse_expression();
        match(TRSQPAREN);
    }
}

static void parse_expression(void) {
    parse_term();
    while (parser.current_token == TPLUS || parser.current_token == TMINUS) {
        match(parser.current_token);
        parse_term();
    }
}

static void parse_term(void) {
    parse_factor();
    while (parser.current_token == TSTAR || parser.current_token == TDIV) {
        match(parser.current_token);
        parse_factor();
    }
}

static void parse_factor(void) {
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
}

static void parse_comparison(void) {
    parse_expression();
    if (parser.current_token == TEQUAL || 
        parser.current_token == TNOTEQ ||
        parser.current_token == TGR ||
        parser.current_token == TGREQ ||
        parser.current_token == TLE ||
        parser.current_token == TLEEQ) {
        match(parser.current_token);
        parse_expression();
    }
}

static void parse_condition(void) {
    parse_comparison();
    while (parser.current_token == TOR || parser.current_token == TAND) {
        match(parser.current_token);
        parse_comparison();
    }
}

void parse_error(const char* message) {
    if (parser.first_error_line == 0) {
        parser.first_error_line = get_linenum();
    }
    fprintf(stderr, "Syntax error at line %d: %s (token: %d)\n", 
            get_linenum(), message, parser.current_token);
    parser.error_count++;

    // Exit if too many errors
    if (parser.error_count >= MAX_ERRORS) {
        exit(1); 
    }

    // Skip tokens until a synchronization point
    while (parser.current_token != -1) { // -1 is EOF
        // Check if current token is a sync point
        for (int i = 0; i < SYNC_TOKENS_COUNT; i++) {
            if (parser.current_token == sync_tokens[i]) {
                return;
            }
        }
        parser.current_token = scan();
    }
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
    match(TPROCEDURE);
    match(TNAME);
    if (parser.current_token == TLPAREN) {
        match(TLPAREN);
        parse_parameter_list();
        match(TRPAREN);
    }
    match(TSEMI);
    parse_block();
    match(TSEMI);
}