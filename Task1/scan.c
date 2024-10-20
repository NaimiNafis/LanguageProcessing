#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scan.h"

FILE *fp;  // Global file pointer to access across functions

char string_attr[MAXSTRSIZE];
int num_attr;
char current_char = '\0';
int line_number = 1;

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

    line_number = 1;
    current_char = (char) fgetc(fp);
    return 0;
}

/* The main scan function that returns the token code */
int scan(void) {
    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);  // Initialize the buffer

    /* Skip any separators or whitespace */
    while (skip_whitespace_and_comments()) {
        if (current_char == EOF) {
            printf("End of file reached at line %d\n", line_number);
            return -1;  // Return EOF if reached
        }
    }

    /* Debugging: Print the current character */
    printf("Current character: %c (Line: %d)\n", current_char, line_number);

    /* Handle symbols */
    if (is_special_symbol(current_char)) {
        buffer[0] = current_char;
        current_char = (char) fgetc(fp);  // Read the next character
        printf("Processing symbol: %c\n", buffer[0]);
        return process_symbol(buffer);
    }

    /* Handle numbers */
    if (isdigit(current_char)) {
        buffer[0] = current_char;
        for (i = 1; (current_char = (char) fgetc(fp)) != EOF; i++) {
            if (check_token_size(i) == -1) {
                error("Token too long");
                return -1;
            }
            if (isdigit(current_char)) {
                buffer[i] = current_char;
            } else {
                break;
            }
        }
        printf("Processing number: %s\n", buffer);
        return process_number(buffer);
    }

    /* Handle strings */
    if (current_char == '\'') {
        printf("Processing string literal starting at line %d\n", line_number);
        return process_string_literal();
    }

    /* Handle keywords and identifiers */
    if (isalpha(current_char)) {
        buffer[0] = current_char;
        for (i = 1; (current_char = (char) fgetc(fp)) != EOF; i++) {
            if (check_token_size(i) == -1) {
                return -1;
            }
            if (isalpha(current_char) || isdigit(current_char)) {
                buffer[i] = current_char;
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
    printf("Unexpected token: %c at line %d\n", current_char, line_number);
    error("Unexpected token encountered");
    return -1;
}


/* Skip separators like whitespace, comments, etc. */
int skip_whitespace_and_comments(void) {
    while (isspace(current_char)) {  // Using isspace() from <ctype.h>
        if (current_char == '\n') line_number++;
        current_char = (char) fgetc(fp);
    }
    return (current_char == EOF) ? 1 : 0;
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
    fprintf(stderr, "Error: %s at line %d\n", mes, line_number);
    exit(EXIT_FAILURE);
    return -1;
}

/* Check if the character is a special symbol */
int is_special_symbol(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')' || c == ':' || c == ';');
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
    
    while ((current_char = fgetc(fp)) != EOF) {
        if (i >= MAXSTRSIZE - 1) {
            error("String literal too long.");
            return -1;
        }
        if (current_char == '\'') {
            current_char = fgetc(fp);
            if (current_char != '\'') {
                tempbuf[i] = '\0';
                strncpy(string_attr, tempbuf, MAXSTRSIZE);
                return TSTRING;
            }
            tempbuf[i++] = '\'';  // Handle escaped quotes
        } else {
            tempbuf[i++] = current_char;
        }
    }
    error("Unterminated string literal.");
    return -1;
}

int process_symbol(char *token_str) {
    switch (token_str[0]) {
        case '(': return TLPAREN;
        case ')': return TRPAREN;
        case '+': return TPLUS;
        case '-': return TMINUS;
        case ':':
            if (current_char == '=') {
                current_char = fgetc(fp);
                return TASSIGN;
            }
            return TCOLON;
        case ';': return TSEMI;
        default:
            error("Unrecognized symbol.");
            return -1;
    }
}

