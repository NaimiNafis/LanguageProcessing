#include <ctype.h>
#include "scan.h"

#define ERROR -1
#define NORMAL 0

FILE *fp;
char string_attr[MAXSTRSIZE];
int num_attr;
char cbuf = '\0';
int linenum = 1;
extern char *tokenstr[];

int match_keyword(const char *token_str);
int process_identifier(const char *token_str);
int process_symbol(char *token_str);
int process_number(const char *token_str);
int process_string_literal(void);
int skip_whitespace_and_comments(void);
int check_token_size(int length);
int parse_program(void);
int parse_block(void);
int parse_variable_declaration(void);
int parse_subprogram_declaration(void);
int parse_statement(void);
int parse_expression(void);
int parse_simple_expression(void);
int parse_term(void);
int parse_factor(void);
int parse_type(void);
int parse_array_type(void);
int parse_assignment_statement(void);
int parse_condition_statement(void);
int parse_iteration_statement(void);
int parse_exit_statement(void);
int parse_compound_statement(void);
int parse_variable_names(void);

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

void pretty_print(void) {
    int token;
    int indentation_level = 0;

    rewind(fp);

    while ((token = scan()) != -1) {
        switch (token) {
            case TPROGRAM:
            case TVAR:
            case TBEGIN:
                printf("%*s%s\n", indentation_level * 4, "", tokenstr[token]);
                indentation_level++;
                break;
            case TEND:
                indentation_level--;
                printf("%*s%s\n", indentation_level * 4, "", tokenstr[token]);
                break;
            case TSEMI:
                printf(";\n");
                break;
            case TDOT:
                printf(".\n");
                break;
            default:
                printf("%*s%s ", indentation_level * 4, "", tokenstr[token]);
                break;
        }
    }
}

int init_scan(char *filename) {
    fp = fopen(filename, "r");
    if (fp == NULL) {
        error("Unable to open file.");
        return -1;
    }
    linenum = 1;
    cbuf = (char) fgetc(fp);
    return 0;
}

int scan(void) {
    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);

    while (skip_whitespace_and_comments()) {
        if (cbuf == EOF) {
            printf("End of file reached at line %d\n", linenum);
            return -1;
        }
    }

    switch (cbuf) {
        case '+': case '-': case '*': case '=': case '<': case '>': case '(': case ')':
        case '[': case ']': case ':': case '.': case ',': case ';': case '!':
            buffer[0] = cbuf;
            cbuf = (char) fgetc(fp);
            return process_symbol(buffer);

        case '\'':
            return process_string_literal();

        default:
            if (isalpha(cbuf)) {
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isalpha(cbuf) || isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                int temp = match_keyword(buffer);
                return (temp != -1) ? temp : process_identifier(buffer);
            }

            if (isdigit(cbuf)) {
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                return process_number(buffer);
            }

            printf("Unexpected token: %c at line %d\n", cbuf, linenum);
            error("Unexpected token encountered");
            return -1;
    }
}

int skip_whitespace_and_comments(void) {
    while (1) {
        while (isspace(cbuf)) {
            if (cbuf == '\n') linenum++;
            cbuf = (char) fgetc(fp);
        }
        if (cbuf == '{') {
            while (cbuf != '}' && cbuf != EOF) {
                cbuf = (char) fgetc(fp);
                if (cbuf == '\n') linenum++;
            }
            if (cbuf == '}') cbuf = (char) fgetc(fp);
            continue;
        }
        if (cbuf == '/') {
            cbuf = (char) fgetc(fp);
            if (cbuf == '/') {
                while (cbuf != '\n' && cbuf != EOF) cbuf = (char) fgetc(fp);
                linenum++;
                cbuf = (char) fgetc(fp);
                continue;
            }
            if (cbuf == '*') {
                while (1) {
                    cbuf = (char) fgetc(fp);
                    if (cbuf == '*' && (cbuf = (char) fgetc(fp)) == '/') {
                        cbuf = (char) fgetc(fp);
                        break;
                    }
                    if (cbuf == '\n') linenum++;
                    if (cbuf == EOF) {
                        printf("Warning: Unterminated multi-line comment at line %d, skipping...\n", linenum);
                        return 1;
                    }
                }
                continue;
            } else {
                break;
            }
        }
        break;
    }
    return (cbuf == EOF) ? 1 : 0;
}

int match_keyword(const char *token_str) {
    for (int i = 0; i < KEYWORDSIZE; i++) {
        if (strcmp(token_str, key[i].keyword) == 0) {
            return key[i].keytoken;
        }
    }
    return -1;
}

int process_identifier(const char *token_str) {
    strncpy(string_attr, token_str, MAXSTRSIZE - 1);
    string_attr[MAXSTRSIZE - 1] = '\0';
    return TNAME;
}

int process_number(const char *token_str) {
    long value = strtol(token_str, NULL, 10);
    if (value <= 32767) {
        num_attr = (int) value;
    } else {
        error("Number exceeds maximum allowable value.");
        return -1;
    }
    return TNUMBER;
}

int process_string_literal(void) {
    int i = 0;
    char tempbuf[MAXSTRSIZE];

    while ((cbuf = fgetc(fp)) != EOF) {
        if (check_token_size(i + 1) == -1) return -1;
        if (cbuf == '\'') {
            cbuf = fgetc(fp);
            if (cbuf != '\'') {
                tempbuf[i] = '\0';
                strncpy(string_attr, tempbuf, MAXSTRSIZE);
                return TSTRING;
            }
            tempbuf[i++] = '\'';
        } else {
            tempbuf[i++] = cbuf;
        }
    }
    error("Unterminated string literal.");
    return -1;
}

int process_symbol(char *token_str) {
    switch (token_str[0]) {
        case '(': return TLPAREN;
        case ')': return TRPAREN;
        case '[': return TLSQPAREN;
        case ']': return TRSQPAREN;
        case '+': return TPLUS;
        case '-': return TMINUS;
        case '*': return TSTAR;
        case '=': return TEQUAL;
        case '<':
            if (cbuf == '>') { cbuf = fgetc(fp); return TNOTEQ; }
            if (cbuf == '=') { cbuf = fgetc(fp); return TLEEQ; }
            return TLE;
        case '>':
            if (cbuf == '=') { cbuf = fgetc(fp); return TGREQ; }
            return TGR;
        case ':':
            if (cbuf == '=') { cbuf = fgetc(fp); return TASSIGN; }
            return TCOLON;
        case '.': return TDOT;
        case ',': return TCOMMA;
        case ';': return TSEMI;
        default:
            error("Unrecognized symbol.");
            return -1;
    }
}

int check_token_size(int length) {
    if (length >= MAXSTRSIZE) {
        error("Token exceeds maximum size.");
        return -1;
    }
    return 1;
}

int error(char *mes) {
    fprintf(stderr, "Error: %s at line %d\n", mes, linenum);
    exit(EXIT_FAILURE);
    return -1;
}

int get_linenum(void) {
    return linenum;
}

void end_scan(void) {
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
}

int token = 0;
int indentation_level = 0;

void print_token(void) {
    printf("%*s%s ", indentation_level * 4, "", tokenstr[token]);
}

void expect(int expected, char *error_message) {
    if (token != expected) {
        error(error_message);
    }
    print_token();
    token = scan();
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
