#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scan.h"

FILE *fp;  // Global file pointer to access across functions

char string_attr[MAXSTRSIZE];
int num_attr;
char cbuf = '\0';  // Renamed from current_char to cbuf
int linenum = 1;

/* Helper function declarations */
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

    /* Main switch to handle different character types */
    switch (cbuf) {
        case ' ':
        case '\t':
        case '\n':
            // Whitespace is already handled by skip_whitespace_and_comments(), so this case is unnecessary.
            // This is a safeguard if whitespace appears unexpectedly.
            cbuf = (char) fgetc(fp);  // Read the next character
            return scan();  // Recur to process the next token

        case '+': case '-': case '*': case '=': case '<': case '>': case '(': case ')':
        case '[': case ']': case '{': case '}': case ':': case '.': case ',': case ';': case '!':
            // Handle symbols
            buffer[0] = cbuf;
            cbuf = (char) fgetc(fp);  // Move to next character
            return process_symbol(buffer);

        case '\'':
            // Handle strings
            printf("Processing string literal starting at line %d\n", linenum);
            return process_string_literal();

        default:
            if (isalpha(cbuf)) {
                // Handle keywords and identifiers
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) {
                        error("Token too long");
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

            if (isdigit(cbuf)) {
                // Handle numbers
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

            // If no valid token is detected, return an error
            printf("Unexpected token: %c at line %d\n", cbuf, linenum);
            error("Unexpected token encountered");
            return -1;
    }
}

/* Skip separators like whitespace, comments, etc. */
int skip_whitespace_and_comments(void) {
    while (1) {
        // Skip whitespace
        while (isspace(cbuf)) {
            if (cbuf == '\n') {
                linenum++;  // Increment line number for newlines
            }
            cbuf = (char) fgetc(fp);  // Read next character
        }

        // Handle block comments starting with '{' and ending with '}'
        if (cbuf == '{') {
            // Skip characters until closing '}'
            while (cbuf != '}' && cbuf != EOF) {
                cbuf = (char) fgetc(fp);
                if (cbuf == '\n') linenum++;  // Track new lines in comments
            }
            if (cbuf == '}') {
                cbuf = (char) fgetc(fp);  // Move past the closing '}'
            }
            return skip_whitespace_and_comments();  // Continue skipping
        }

        // Handle single-line comments (//)
        if (cbuf == '/') {
            cbuf = (char) fgetc(fp);  // Check the next character
            if (cbuf == '/') {
                // Skip the single-line comment till end of line
                while (cbuf != '\n' && cbuf != EOF) {
                    cbuf = (char) fgetc(fp);
                }
                linenum++;  // Increment line number at the end of the line
                cbuf = (char) fgetc(fp);  // Move to the next character
                continue;  // Continue skipping
            }
            // Handle multi-line comments (/* ... */)
            else if (cbuf == '*') {
                while (1) {
                    cbuf = (char) fgetc(fp);  // Get the next character
                    if (cbuf == '*' && (cbuf = (char) fgetc(fp)) == '/') {
                        // Found the closing */
                        cbuf = (char) fgetc(fp);  // Move past the end of comment
                        break;
                    }
                    if (cbuf == '\n') {
                        linenum++;  // Track new lines inside the comment
                    }
                    if (cbuf == EOF) {
                        error("Unterminated multi-line comment.");
                        return -1;
                    }
                }
                continue;  // Continue skipping after the comment
            } else {
                // It's not a comment, handle this character in scan()
                break;
            }
        } else {
            // If it's not whitespace or a comment, break the loop
            break;
        }
    }
    return (cbuf == EOF) ? 1 : 0;  // Return 1 if EOF, otherwise 0
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
        case '(': return TLPAREN;
        case ')': return TRPAREN;
        case '{': return TLBRACE;  // New: Handle '{'
        case '}': return TRBRACE;
        case '[': return TLSQPAREN;  // Handle '['
        case ']': return TRSQPAREN; 
        case '+': return TPLUS;
        case '-': return TMINUS;
        case '*': return TSTAR;
        case '=': return TEQUAL;
        case '<':
            if (cbuf == '>') {
                cbuf = fgetc(fp);
                return TNOTEQ;
            } else if (cbuf == '=') {
                cbuf = fgetc(fp);
                return TLEEQ;
            } else {
                return TLE;
            }
        case '>':
            if (cbuf == '=') {
                cbuf = fgetc(fp);
                return TGREQ;
            } else {
                return TGR;
            }
        case ':':
            if (cbuf == '=') {
                cbuf = fgetc(fp);
                return TASSIGN;
            }
            return TCOLON;
        case '.': return TDOT;
        case ',': return TCOMMA;
        case ';': return TSEMI;
        default:
            error("Unrecognized symbol.");
            return -1;
    }
}
