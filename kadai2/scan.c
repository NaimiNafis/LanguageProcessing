#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scan.h"

/* --- Global Variables --- */
FILE *fp = NULL;                // File pointer to handle input
char string_attr[MAXSTRSIZE];   // Store string attributes
int num_attr = 0;               // Store numerical attributes
char cbuf = '\0';               // Current character buffer
int linenum = 1;                // Line number tracker
int token = -1;                 // Current token

// Helper function declarations
int match_keyword(const char *token_str);
int process_identifier(const char *token_str);
int process_symbol(char *token_str);
int process_number(const char *token_str);
int process_string_literal(void);
int skip_whitespace_and_comments(void);
int check_token_size(int length);

/* --- Token Metadata --- */
struct KEY key[KEYWORDSIZE] = {
    {"and", TAND}, {"array", TARRAY}, {"begin", TBEGIN}, {"boolean", TBOOLEAN},
    {"break", TBREAK}, {"call", TCALL}, {"char", TCHAR}, {"div", TDIV},
    {"do", TDO}, {"else", TELSE}, {"end", TEND}, {"false", TFALSE},
    {"if", TIF}, {"integer", TINTEGER}, {"not", TNOT}, {"of", TOF},
    {"or", TOR}, {"procedure", TPROCEDURE}, {"program", TPROGRAM}, {"read", TREAD},
    {"readln", TREADLN}, {"return", TRETURN}, {"then", TTHEN}, {"true", TTRUE},
    {"var", TVAR}, {"while", TWHILE}, {"write", TWRITE}, {"writeln", TWRITELN}
};

int numtoken[NUMOFTOKEN + 1] = {0};
char *tokenstr[NUMOFTOKEN + 1] = {
    "", "NAME", "program", "var", "array", "of", "begin", "end", "if", "then", "else", "procedure",
    "return", "call", "while", "do", "not", "or", "div", "and", "char", "integer", "boolean",
    "readln", "writeln", "true", "false", "NUMBER", "STRING", "+", "-", "*", "=", "<>", "<",
    "<=", ">", ">=", "(", ")", "[", "]", ":=", ".", ",", ":", ";", "read", "write", "break"
};

/* --- Scanning Functions --- */
int init_scan(char *filename) {
    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Unable to open file '%s'.\n", filename);
        return -1;
    }
    linenum = 1;
    cbuf = (char)fgetc(fp);
    return 0;
}

int scan(void) {
    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);

    // Initial EOF check
    if (cbuf == EOF) {
        printf("DEBUG: End of file reached at line %d\n", linenum);
        return -1;
    }

    printf("DEBUG: Skipping whitespace at line %d\n", linenum);

    // Skip whitespace and comments
    while (skip_whitespace_and_comments()) {
        if (cbuf == EOF) {
            printf("DEBUG: End of file reached at line %d\n", linenum);
            return -1;
        }
    }

    printf("DEBUG: Current character: '%c' at line %d\n", cbuf, linenum);
    printf("DEBUG: Current character after skipping: '%c' at line %d\n", cbuf, linenum);

    // Ensure no whitespace tokens are returned
    while (isspace(cbuf)) {
        cbuf = (char)fgetc(fp);
        if (cbuf == EOF) {
            printf("DEBUG: End of file reached at line %d\n", linenum);
            return -1;
        }
    }

    switch (cbuf) {
        // Handle symbols directly
        case '+': case '-': case '*': case '=': case '<': case '>': case '(': case ')':
        case '[': case ']': case ':': case '.': case ',': case ';': case '!':
            buffer[0] = cbuf;
            cbuf = (char)fgetc(fp);
            return process_symbol(buffer);

        // Handle string literals
        case '\'':
            printf("DEBUG: Processing string literal at line %d\n", linenum);
            return process_string_literal();

        // Handle keywords, identifiers, and numbers
        default:
            if (isalpha(cbuf)) {  // Keyword/Identifier
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char)fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isalpha(cbuf) || isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                printf("DEBUG: Processing identifier/keyword: '%s' at line %d\n", buffer, linenum);
                int temp = match_keyword(buffer);
                return (temp != -1) ? temp : process_identifier(buffer);
            }
            
            if (isdigit(cbuf)) {  // Number
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char)fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                printf("DEBUG: Processed number: '%s' at line %d\n", buffer, linenum);
                return process_number(buffer);
            }

            // Handle unexpected tokens
            printf("DEBUG: Unexpected token '%c' at line %d\n", cbuf, linenum);
            error("Unexpected token encountered");
            return -1;
    }
}

/* Helper Functions */
int match_keyword(const char *token_str) {
    for (int i = 0; i < KEYWORDSIZE; i++) {
        if (strcmp(token_str, key[i].keyword) == 0) {
            return key[i].keytoken;
        }
    }
    return -1;  // Not a keyword
}

int process_identifier(const char *token_str) {
    strncpy(string_attr, token_str, MAXSTRSIZE - 1);
    return TNAME;  // Identifier token
}

int process_number(const char *token_str) {
    long value = strtol(token_str, NULL, 10);
    if (value <= 32767) {
        num_attr = (int)value;
        return TNUMBER;
    } else {
        error("Number exceeds maximum allowable value.");
        return -1;
    }
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
            tempbuf[i++] = '\'';  // Handle escaped quotes
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

int skip_whitespace_and_comments(void) {
    while (1) {
        while (isspace(cbuf)) {  // Skip whitespace
            if (cbuf == '\n') linenum++;  // Track newlines
            cbuf = (char)fgetc(fp);
        }

        // Handle block comments
        if (cbuf == '{') {
            while (cbuf != '}' && cbuf != EOF) {
                cbuf = (char)fgetc(fp);
                if (cbuf == '\n') linenum++;
            }
            if (cbuf == '}') cbuf = (char)fgetc(fp);
            continue;
        }

        // Handle single-line comments
        if (cbuf == '/') {
            cbuf = (char)fgetc(fp);
            if (cbuf == '/') {
                while (cbuf != '\n' && cbuf != EOF) {
                    cbuf = (char)fgetc(fp);
                }
                if (cbuf == '\n') linenum++;
                cbuf = (char)fgetc(fp);
                continue;
            }

            // Handle multi-line comments
            if (cbuf == '*') {
                while (1) {
                    cbuf = (char)fgetc(fp);
                    if (cbuf == '*' && (cbuf = (char)fgetc(fp)) == '/') {
                        cbuf = (char)fgetc(fp);
                        break;
                    }
                    if (cbuf == '\n') linenum++;
                    if (cbuf == EOF) {
                        error("Unterminated multi-line comment");
                        return 1;
                    }
                }
                continue;
            }
        }

        break;
    }
    return (cbuf == EOF) ? 1 : 0;
}


int check_token_size(int length) {
    if (length >= MAXSTRSIZE) {
        error("Token exceeds maximum size.");
        return -1;
    }
    return 1;
}

int error(const char *mes) {
    fprintf(stderr, "Error: %s at line %d\n", mes, linenum);
    exit(EXIT_FAILURE);
}

int get_linenum(void) {
    return linenum;
}

void end_scan(void) {
    if (fp) fclose(fp);
    fp = NULL;
}
