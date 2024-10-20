#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scan.h"

FILE *fp;  // Global file pointer to access across functions

char string_attr[MAXSTRSIZE];
int num_attr;
char cbuf = '\0';
int linenum = 1;

/* Helper function declarations */
int is_special_symbol(char c);
int match_keyword(const char *token_str);
int process_identifier(const char *token_str);
int process_symbol(char *token_str);
int process_number(const char *token_str);
int process_string_literal(void);
int skip_whitespace_and_comments(void);
int check_token_size(int length);

/* Initialize scanning by opening the file and preparing for scanning */
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

/* The main scan function that returns the token code */
int scan(void) {
    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);  // Initialize the buffer

    /* Skip any separators or whitespace */
    while (skip_whitespace_and_comments()) {
        if (cbuf == EOF) {
            printf("End of file reached at line %d\n", linenum);
            return -1;  // Return EOF if reached
        }
    }

    /* Debugging: Print the current character */
    printf("Current character: %c (Line: %d)\n", cbuf, linenum);

    /* Handle symbols */
    if (is_special_symbol(cbuf)) {
        buffer[0] = cbuf;
        cbuf = (char) fgetc(fp);  // Read the next character
        printf("Processing symbol: %c\n", buffer[0]);
        return process_symbol(buffer);
    }

    /* Handle numbers */
    if (isdigit(cbuf)) {
        buffer[0] = cbuf;
        for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
            if (check_token_size(i) == -1) {
                error("Token too long");
                return -1;
            }
            if (isdigit(cbuf)) {
                buffer[i] = cbuf;
            } else {
                break;
            }
        }
        printf("Processing number: %s\n", buffer);
        return process_number(buffer);
    }

    /* Handle strings */
    if (cbuf == '\'') {
        printf("Processing string literal starting at line %d\n", linenum);
        int token = process_string_literal();
        // After processing the string, immediately return the string token
        if (token == TSTRING) {
            return token;
        } else {
            return -1;
        }
    }

    /* Handle keywords and identifiers */
    if (isalpha(cbuf)) {
        buffer[0] = cbuf;
        for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
            if (check_token_size(i) == -1) {
                return -1;
            }
            if (isalpha(cbuf) || isdigit(cbuf)) {
                buffer[i] = cbuf;
            } else {
                break;
            }
        }
        printf("Processing identifier/keyword: %s\n", buffer);
        int temp = match_keyword(buffer);
        if (temp != -1) {
            return temp;  // It's a keyword
        } else {
            return process_identifier(buffer);  // It's an identifier (NAME)
        }
    }

    /* If no valid token is detected */
    printf("Unexpected token: %c at line %d\n", cbuf, linenum);
    error("Unexpected token encountered");
    return -1;
}

/* Skip separators like whitespace, comments, etc. */
int skip_whitespace_and_comments(void) {
    while (isspace(cbuf)) {  // Using isspace() from <ctype.h>
        if (cbuf == '\n') linenum++;
        cbuf = (char) fgetc(fp);
    }
    return (cbuf == EOF) ? 1 : 0;
}

/* Check if the token exceeds maximum size */
int check_token_size(int length) {
    if (length >= MAXSTRSIZE) {
        error("Token exceeds maximum size");
        return -1;
    }
    return 1;
}

/* Error function */
int error(char *mes) {
    fprintf(stderr, "Error: %s at line %d\n", mes, linenum);
    exit(EXIT_FAILURE);
    return -1;
}

/* Check if the character is a special symbol */
int is_special_symbol(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '=' || c == '>' || c == '<' ||
            c == '(' || c == ')' || c == '[' || c == ']' || c == ':' || c == '.' ||
            c == ',' || c == ';');
}

int get_linenum(void) {
    return linenum;  // Returns the current line number being processed
}

/* End scan: Close the file */
void end_scan(void) {
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
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
    string_attr[MAXSTRSIZE - 1] = '\0';  // Null-terminate
    return TNAME;  // Assuming TNAME is the token for identifiers
}

int process_number(const char *token_str) {
    long value = strtol(token_str, NULL, 10);
    if (value <= 32767) {
        num_attr = (int)value;
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
        if (check_token_size(i + 1) == -1) {
            return -1;
        }
        if (cbuf == '\'') {
            cbuf = fgetc(fp);
            if (cbuf != '\'') {
                // The string literal is now complete
                tempbuf[i] = '\0';  // Null-terminate the string
                strncpy(string_attr, tempbuf, MAXSTRSIZE);
                
                // If the next character is not a quote, return the string token
                return TSTRING;  // Assuming TSTRING is the token for strings
            }
            tempbuf[i++] = '\'';  // Handle escaped single quotes
        } else {
            tempbuf[i++] = cbuf;
        }
    }
    
    error("Unterminated string literal.");
    return -1;
}

int process_symbol(char *token_str) {
    switch (token_str[0]) {
        case '+': return TPLUS;
        case '-': return TMINUS;
        case '*': return TSTAR;
        case '=': return TEQUAL;
        case '>':
            if (cbuf == '=') {
                cbuf = fgetc(fp);  // Handle >=
                return TGREQ;
            }
            return TGR;  // Handle >
        case '<':
            if (cbuf == '=') {
                cbuf = fgetc(fp);  // Handle <=
                return TLEEQ;
            } else if (cbuf == '>') {
                cbuf = fgetc(fp);  // Handle <>
                return TNOTEQ;
            }
            return TLE;  // Handle <
        case '(': return TLPAREN;
        case ')': return TRPAREN;
        case '[': return TLSQPAREN;
        case ']': return TRSQPAREN;
        case ':':
            if (cbuf == '=') {
                cbuf = fgetc(fp);  // Handle :=
                return TASSIGN;
            }
            return TCOLON;  // Handle :
        case '.': return TDOT;
        case ',': return TCOMMA;
        case ';': return TSEMI;
        default:
            error("Unrecognized symbol.");
            return -1;
    }
}



