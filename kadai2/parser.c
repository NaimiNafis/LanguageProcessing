#include "parser.h"
#include "scan.h"

int token = 0;                // Token variable
int indentation_level = 0;    // Indentation level for pretty printing

void expect(int expected, const char *error_message) {
    if (token != expected) {
        error(error_message);
    }
    print_token();
    token = scan();
}

void print_token(void) {
    printf("%*s%s ", indentation_level * 4, "", get_token_string(token));
}

void pretty_print(void) {
    rewind_scan_file();  // Use helper function to rewind the input file

    while ((token = scan()) != -1) {
        switch (token) {
            case TPROGRAM:
            case TVAR:
            case TBEGIN:
                printf("%*s%s\n", indentation_level * 4, "", get_token_string(token));
                indentation_level++;
                break;
            case TEND:
                indentation_level--;
                printf("%*s%s\n", indentation_level * 4, "", get_token_string(token));
                break;
            case TSEMI:
                printf(";\n");
                break;
            case TDOT:
                printf(".\n");
                break;
            default:
                printf("%*s%s ", indentation_level * 4, "", get_token_string(token));
                break;
        }
    }
}

int parse_program() {
    if (token != TPROGRAM) return error("Keyword 'program' is missing");
    token = scan();
    if (token != TNAME) return error("Program name is missing");
    token = scan();
    if (token != TSEMI) return error("Semicolon is missing");
    token = scan();
    if (parse_block() == ERROR) return ERROR;
    if (token != TDOT) return error("Period is missing at the end of the program");
    token = scan();
    return NORMAL;
}

int parse_block(void) {
    if (scan() == TVAR) {
        while (scan() == TNAME) {
            if (scan() != TCOLON) return error("Expected ':' in variable declaration");
            if (scan() != TINTEGER && scan() != TBOOLEAN && scan() != TCHAR)
                return error("Expected a valid type (integer, boolean, char)");
            if (scan() != TSEMI) return error("Expected ';' after variable declaration");
        }
    }
    if (scan() != TBEGIN) return error("Expected 'begin' to start the block");
    if (scan() != TEND) return error("Expected 'end' to close the block");
    return 0;
}

int parse_variable_declaration(void) {
    expect(TVAR, "Expected 'var'");
    do {
        parse_variable_names();
        expect(TCOLON, "Expected ':'");
        parse_type();
        expect(TSEMI, "Expected ';'");
    } while (token == TNAME);
    return NORMAL;
}

int parse_variable_names(void) {
    expect(TNAME, "Expected variable name");
    while (token == TCOMMA) {
        print_token();
        token = scan();
        expect(TNAME, "Expected variable name after ','");
    }
    return NORMAL;
}

int parse_type(void) {
    if (token == TINTEGER || token == TBOOLEAN || token == TCHAR) {
        print_token();
        token = scan();
    } else if (token == TARRAY) {
        parse_array_type();
    } else {
        return error("Expected a valid type");
    }
    return NORMAL;
}

int parse_array_type(void) {
    expect(TARRAY, "Expected 'array'");
    expect(TLSQPAREN, "Expected '['");
    expect(TNUMBER, "Expected array size");
    expect(TRSQPAREN, "Expected ']'");
    expect(TOF, "Expected 'of'");
    return parse_type();
}

int parse_compound_statement(void) {
    expect(TBEGIN, "Expected 'begin'");
    indentation_level++;
    while (token != TEND) {
        parse_statement();
        if (token == TSEMI) {
            print_token();
            token = scan();
        }
    }
    indentation_level--;
    expect(TEND, "Expected 'end'");
    return NORMAL;
}

int parse_statement(void) {
    switch (token) {
        case TNAME: return parse_assignment_statement();
        case TIF: return parse_condition_statement();
        case TWHILE: return parse_iteration_statement();
        case TBREAK: return parse_exit_statement();
        case TBEGIN: return parse_compound_statement();
        default: return NORMAL;
    }
}

int parse_assignment_statement(void) {
    expect(TNAME, "Expected variable name");
    expect(TASSIGN, "Expected ':='");
    return parse_expression();
}

int parse_condition_statement(void) {
    expect(TIF, "Expected 'if'");
    if (parse_expression() == ERROR) return ERROR;
    expect(TTHEN, "Expected 'then'");
    parse_statement();
    if (token == TELSE) {
        print_token();
        token = scan();
        parse_statement();
    }
    return NORMAL;
}

int parse_iteration_statement(void) {
    expect(TWHILE, "Expected 'while'");
    if (parse_expression() == ERROR) return ERROR;
    expect(TDO, "Expected 'do'");
    return parse_statement();
}

int parse_exit_statement(void) {
    expect(TBREAK, "Expected 'break'");
    return NORMAL;
}

int parse_expression(void) {
    if (parse_simple_expression() == ERROR) return ERROR;
    while (token == TEQUAL || token == TNOTEQ || token == TLE || token == TLEEQ || token == TGR || token == TGREQ) {
        print_token();
        token = scan();
        if (parse_simple_expression() == ERROR) return ERROR;
    }
    return NORMAL;
}

int parse_simple_expression(void) {
    if (token == TPLUS || token == TMINUS) {
        print_token();
        token = scan();
    }
    return parse_term();
}

int parse_term(void) {
    if (parse_factor() == ERROR) return ERROR;
    while (token == TSTAR || token == TDIV || token == TAND) {
        print_token();
        token = scan();
        if (parse_factor() == ERROR) return ERROR;
    }
    return NORMAL;
}

int parse_factor(void) {
    if (token == TNAME || token == TNUMBER || token == TSTRING || token == TTRUE || token == TFALSE) {
        print_token();
        token = scan();
    } else if (token == TLPAREN) {
        expect(TLPAREN, "Expected '('");
        if (parse_expression() == ERROR) return ERROR;
        expect(TRPAREN, "Expected ')'");
    } else {
        return error("Expected a valid factor");
    }
    return NORMAL;
}