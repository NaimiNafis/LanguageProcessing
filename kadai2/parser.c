#include "scan.h"
#include "parser.h"
#include <stdbool.h>

/* --- Global Variables --- */
static int current_indent = 0;  // Tracks current indentation level for pretty-printing

/* --- Helper Functions --- */
// Print with current indentation level
void print_with_indent(const char *str) {
    for (int i = 0; i < current_indent; i++) {
        printf("    ");
    }
    printf("%s", str);
}

// Utility function to check if the token is a literal or identifier
bool is_literal_or_identifier(void) {
    return token == TNUMBER || token == TSTRING || token == TNAME || token == TTRUE || token == TFALSE;
}

// Utility function to print literals or identifiers
void print_literal_or_identifier(void) {
    if (token == TNUMBER) {
        printf("%d", num_attr);
    } else if (token == TSTRING) {
        printf("'%s'", string_attr);
    } else {
        printf("%s", string_attr);
    }
}

// Utility function to check if the token is a binary operator
bool is_binary_operator(int tok) {
    return tok == TEQUAL || tok == TNOTEQ || tok == TLE || tok == TLEEQ || tok == TGR || tok == TGREQ ||
           tok == TPLUS || tok == TMINUS || tok == TSTAR || tok == TDIV || tok == TOR || tok == TAND;
}

/* --- Core Parsing Functions --- */
// Parse the main program structure
int parse_program(void) {
    if (token != TPROGRAM) return error("Keyword 'program' is missing");
    print_with_indent("program ");
    token = scan();

    if (token != TNAME) return error("Program name is missing");
    printf("%s", string_attr);
    token = scan();

    if (token != TSEMI) return error("Semicolon is missing after program name");
    printf(";\n");
    token = scan();

    if (parse_block() == ERROR) return ERROR;

    if (token != TDOT) return error("Period is missing at the end of the program");
    print_with_indent(".\n");
    token = scan();

    return NORMAL;
}

int parse_block(void) {
    // Handle variable declarations
    while (token == TVAR) {
        token = scan();  // Consume 'var'
        while (token == TNAME) {
            if (parse_variable_declaration() == ERROR) return ERROR;
        }
    }

    // Ensure 'begin' keyword starts the block
    if (token != TBEGIN) {
        return error("Keyword 'begin' is missing for block");
    }

    print_with_indent("begin\n");
    current_indent++;
    token = scan();  // Consume 'begin'

    // Parse statements until 'end'
    while (token != TEND) {
        if (parse_statement() == ERROR) return ERROR;
    }

    current_indent--;
    print_with_indent("end\n");
    token = scan();  // Consume 'end'

    // Check for semicolon after 'end'
    if (token == TSEMI) {
        token = scan();  // Consume the semicolon
    }

    return NORMAL;
}

// Parse a single variable declaration
int parse_variable_declaration(void) {
    print_with_indent("var ");
    while (1) {
        if (token != TNAME) return error("Variable name is missing");
        printf("%s", string_attr);
        token = scan();

        if (token == TCOLON) {
            printf(" : ");
            token = scan();
            break;
        } else if (token == TCOMMA) {
            printf(", ");
            token = scan();
        } else {
            return error("Colon or comma is missing in variable declaration");
        }
    }

    if (token == TINTEGER || token == TBOOLEAN || token == TCHAR) {
        printf("%s", tokenstr[token]);
        token = scan();
    } else {
        return error("Variable type is missing or invalid");
    }

    if (token != TSEMI) return error("Semicolon is missing after variable declaration");
    printf(";\n");
    token = scan();

    return NORMAL;
}

// Parse a single statement
int parse_statement(void) {
    switch (token) {
        case TNAME:
            return parse_assignment_statement();
        case TIF:
            return parse_if_statement();
        case TWHILE:
            return parse_while_statement();
        case TBEGIN:
            return parse_block();
        case TWRITELN:
        case TWRITE:
            return parse_write_statement();
        case TREADLN:
        case TREAD:
            return parse_read_statement();
        case TBREAK:
            print_with_indent("break;\n");
            token = scan();
            return NORMAL;
        default:
            return error("Unrecognized or missing statement");
    }
}

// Parse an assignment statement
int parse_assignment_statement(void) {
    printf("%s := ", string_attr);  // Print the identifier (e.g., x)
    token = scan();  // Consume the identifier

    if (token != TASSIGN) {
        return error("Missing ':=' in assignment statement");
    }
    token = scan();  // Consume ':='

    // Parse the expression
    if (parse_expression() == ERROR) {
        return error("Invalid expression in assignment statement");
    }

    // Handle semicolon or `end` after the assignment
    if (token == TSEMI) {
        printf(";\n");  // Print the semicolon
        token = scan();  // Consume the semicolon
    } else if (token == TEND) {
        printf("\n");  // No semicolon, just a newline
    } else {
        return error("Missing ';' after assignment statement");
    }

    return NORMAL;
}

// Parse an if statement
int parse_if_statement(void) {
    print_with_indent("if ");
    token = scan();

    if (parse_expression() == ERROR) return ERROR;

    if (token != TTHEN) return error("Keyword 'then' is missing in if statement");
    printf(" then\n");
    token = scan();

    current_indent++;
    if (parse_statement() == ERROR) return ERROR;
    current_indent--;

    if (token == TELSE) {
        print_with_indent("else\n");
        token = scan();
        current_indent++;
        if (parse_statement() == ERROR) return ERROR;
        current_indent--;
    }

    return NORMAL;
}

// Parse a while statement
int parse_while_statement(void) {
    print_with_indent("while ");
    token = scan();

    if (parse_expression() == ERROR) return ERROR;

    if (token != TDO) return error("Keyword 'do' is missing in while statement");
    printf(" do\n");
    token = scan();

    current_indent++;
    if (parse_statement() == ERROR) return ERROR;
    current_indent--;

    return NORMAL;
}

// Parse an expression
int parse_expression(void) {
    // Handle literals, identifiers, and grouped expressions
    if (is_literal_or_identifier()) {
        print_literal_or_identifier();
        token = scan();
    } else if (token == TNOT) {  // Handle `not` operator
        printf("not ");
        token = scan();
        if (parse_expression() == ERROR) return error("Invalid operand for 'not'");
    } else if (token == TLPAREN) {  // Handle grouped expressions
        printf("(");
        token = scan();
        if (parse_expression() == ERROR) return error("Invalid grouped expression");
        if (token != TRPAREN) return error("Missing ')' in expression");
        printf(")");
        token = scan();
    } else {
        return error("Invalid or missing expression");
    }

    // Handle binary operators
    while (is_binary_operator(token)) {
        printf(" %s ", tokenstr[token]);  // Print the operator
        token = scan();  // Consume operator

        if (!is_literal_or_identifier() && token != TLPAREN) {
            return error("Expected a valid operand after operator");
        }

        if (parse_expression() == ERROR) {
            return error("Error parsing the right-hand side of the binary operator");
        }
    }

    return NORMAL;
}

// Parse a write or writeln statement
int parse_write_statement(void) {
    printf("%s(", tokenstr[token]);  // Print `writeln` or `write`
    token = scan();  // Move past `writeln` or `write`

    if (token != TLPAREN) {
        return error("Missing '(' after writeln or write");
    }

    token = scan();  // Move past '('

    // Handle arguments inside parentheses
    if (token != TRPAREN) {  // If not immediately closing parentheses
        if (parse_expression() == ERROR) return ERROR;

        while (token == TCOMMA) {  // Handle multiple arguments separated by commas
            printf(", ");
            token = scan();  // Consume the comma
            if (parse_expression() == ERROR) return ERROR;
        }
    }

    if (token != TRPAREN) {
        return error("Missing ')' in writeln or write statement");
    }

    printf(");\n");  // Print closing parenthesis and semicolon
    token = scan();  // Move past ')'

    // Check for semicolon or `end`
    if (token == TSEMI) {
        token = scan();  // Consume the semicolon
    } else if (token != TEND) {
        return error("Missing ';' after writeln or write statement");
    }

    return NORMAL;
}

// Parse a read or readln statement
int parse_read_statement(void) {
    printf("%s(", tokenstr[token]);  // Print `readln` or `read`
    token = scan();  // Move past `readln` or `read`

    if (token != TLPAREN) {
        return error("Missing '(' after readln or read");
    }

    printf("(");  // Print the opening parenthesis
    token = scan();  // Move past '('

    // Expect at least one variable name
    if (token != TNAME) {
        return error("Invalid or missing variable name in readln or read");
    }

    printf("%s", string_attr);  // Print the first variable name
    token = scan();  // Move past the variable

    while (token == TCOMMA) {  // Handle multiple variable names separated by commas
        printf(", ");
        token = scan();  // Move past the comma

        if (token != TNAME) {
            return error("Invalid or missing variable name after ',' in readln or read");
        }

        printf("%s", string_attr);  // Print the next variable name
        token = scan();  // Move past the variable
    }

    if (token != TRPAREN) {
        return error("Missing ')' in readln or read statement");
    }

    printf(")");  // Print the closing parenthesis
    token = scan();  // Move past ')'

    if (token != TSEMI) {
        return error("Missing ';' after readln or read statement");
    }

    printf(";\n");  // Print the semicolon
    token = scan();  // Move past ';'

    return NORMAL;
}
