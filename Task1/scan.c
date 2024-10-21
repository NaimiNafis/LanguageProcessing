#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scan.h"

FILE *fp;  // File pointer to handle input
char string_attr[MAXSTRSIZE];  // Store string attributes
int num_attr;  // Store numerical attributes
char cbuf = '\0';  // Buffer for the current character being read
int linenum = 1;  // Line number tracker

// Helper function declarations
int match_keyword(const char *token_str);
int process_identifier(const char *token_str);
int process_symbol(char *token_str);
int process_number(const char *token_str);
int process_string_literal(void);
int skip_whitespace_and_comments(void);
int check_token_size(int length);

// Initialize file reading
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

// Main scan function: identifies and processes tokens
int scan(void) {
    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);

    // Skip whitespace and comments
    while (skip_whitespace_and_comments()) {
        if (cbuf == EOF) {
            printf("End of file reached at line %d\n", linenum);
            return -1;
        }
    }

    //printf("Current character: %c (Line: %d)\n", cbuf, get_linenum());

    switch (cbuf) {
        // Handle symbols directly
        case '+': case '-': case '*': case '=': case '<': case '>': case '(': case ')':
        case '[': case ']': case '{': case '}': case ':': case '.': case ',': case ';': case '!':
            buffer[0] = cbuf;
            cbuf = (char) fgetc(fp);
            return process_symbol(buffer);

        // Handle string literals
        case '\'':
            //printf("Processing string literal starting at line %d\n", get_linenum());
            return process_string_literal();

        // Handle keywords, identifiers, and numbers
        default:
            if (isalpha(cbuf)) {  // Keyword/Identifier
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isalpha(cbuf) || isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                //printf("Processing identifier/keyword: %s at line %d\n", buffer, get_linenum());
                int temp = match_keyword(buffer);
                return (temp != -1) ? temp : process_identifier(buffer);
            }

            if (isdigit(cbuf)) {  // Number
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isdigit(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                //printf("Processing number: %s at line %d\n", buffer, get_linenum());
                return process_number(buffer);
            }

            // Handle unexpected tokens
            printf("Unexpected token: %c at line %d\n", cbuf, linenum);
            error("Unexpected token encountered");
            return -1;
    }
}

// Skip over whitespace and comments
int skip_whitespace_and_comments(void) {
    while (1) {
        while (isspace(cbuf)) {
            if (cbuf == '\n') linenum++;  // Track line breaks
            cbuf = (char) fgetc(fp);
        }

        // Handle block comments
        if (cbuf == '{') {
            while (cbuf != '}' && cbuf != EOF) {
                cbuf = (char) fgetc(fp);
                if (cbuf == '\n') linenum++;
            }
            if (cbuf == '}') cbuf = (char) fgetc(fp);
            continue;
        }

        // Handle single-line comments
        if (cbuf == '/') {
            cbuf = (char) fgetc(fp);
            if (cbuf == '/') {
                while (cbuf != '\n' && cbuf != EOF) cbuf = (char) fgetc(fp);
                linenum++;
                cbuf = (char) fgetc(fp);
                continue;
            }

            // Handle multi-line comments
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
                break;  // Not a comment, return control to scan
            }
        }
        break;
    }
    return (cbuf == EOF) ? 1 : 0;
}

// Match keywords in the source
int match_keyword(const char *token_str) {
    for (int i = 0; i < KEYWORDSIZE; i++) {
        if (strcmp(token_str, key[i].keyword) == 0) {
            return key[i].keytoken;
        }
    }
    return -1;
}

// Process identifiers
int process_identifier(const char *token_str) {
    strncpy(string_attr, token_str, MAXSTRSIZE - 1);
    string_attr[MAXSTRSIZE - 1] = '\0';
    return TNAME;  // Identifier token
}

// Process numbers
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

// Handle string literals
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

// Handle symbol tokens
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

// Check if token size exceeds limits
int check_token_size(int length) {
    if (length >= MAXSTRSIZE) {
        error("Token exceeds maximum size.");
        return -1;
    }
    return 1;
}

// Error handling
int error(char *mes) {
    fprintf(stderr, "Error: %s at line %d\n", mes, linenum);
    exit(EXIT_FAILURE);
    return -1;
}

// Used for error-reporting function (parser or error handler)
int get_linenum(void) {
    return linenum;
}

// Clean up after scanning
void end_scan(void) {
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
}
