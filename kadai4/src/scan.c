#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "scan.h"
#include "token.h"
#include "error.h"
#include "debug.h"

// Forward declarations
static void debug_scan_printf(const char *format, ...);

// Global variables
static FILE *fp = NULL;
extern int debug_scanner;
char string_attr[MAXSTRSIZE];
int num_attr;
char cbuf = '\0';
int linenum = 1;
extern keyword key[KEYWORDSIZE];
static const char* current_filename = NULL;

static void debug_scan_printf(const char *format, ...) {
    if (debug_scanner) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// Helper function declarations
int match_keyword(const char *token_str);
int process_identifier(const char *token_str);
int process_symbol(char *token_str);
int process_number(const char *token_str);
int process_string_literal(void);
int skip_whitespace_and_comments(void);
int check_token_size(int length);
static int scan_number(void);

Scanner scanner = {0};  // Initialize all fields to 0

int init_scan(const char *filename) {
    scanner.has_error = 0;
    current_filename = filename;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        error("Unable to open file.");
        return -1;
    }
    linenum = 1;
    cbuf = (char) fgetc(fp);
    return 0;
}

const char* get_current_file(void) {
    return current_filename;
}

static void update_line_number(char c) {
    if (c == '\n') {
        linenum++;
    }
}

// Main scan function: identifies and processes tokens
int scan(void) {
    if (fp == NULL) {
        return -1;  // Return error token if file pointer is invalid
    }

    // Stop scanning if error occurred
    if (scanner.has_error) {
        return -1;
    }

    char buffer[MAXSTRSIZE];
    int i = 0;

    memset(buffer, '\0', MAXSTRSIZE);

    // Skip whitespace and comments
    while (skip_whitespace_and_comments()) {
        if (cbuf == EOF) {
            debug_scan_printf("End of file reached at line %d\n", linenum);
            return -1;
        }
    }

    debug_scan_printf("Current character: %c (Line: %d)\n", cbuf, get_linenum());

    switch (cbuf) {
        // Handle symbols directly
        case '+': case '-': case '*': case '=': case '<': case '>': case '(': case ')':
        case '[': case ']': case ':': case '.': case ',': case ';': case '!':
            buffer[0] = cbuf;
            cbuf = (char) fgetc(fp);
            return process_symbol(buffer);

        // Handle string literals
        case '\'':
            debug_scan_printf("Processing string literal starting at line %d\n", get_linenum());
            return process_string_literal();

        // Handle keywords, identifiers, and numbers
        default:
            if (isalpha(cbuf)) {  // Keyword/Identifier
                buffer[0] = cbuf;
                for (i = 1; (cbuf = (char) fgetc(fp)) != EOF; i++) {
                    if (check_token_size(i) == -1) return -1;
                    if (isalnum(cbuf)) {
                        buffer[i] = cbuf;
                    } else {
                        break;
                    }
                }
                buffer[i] = '\0';  // Null-terminate the buffer
                debug_scan_printf("Processing identifier/keyword: %s at line %d\n", buffer, get_linenum());
                int temp = match_keyword(buffer);
                if (temp != -1) {
                    // Keyword matched
                    debug_scan_printf("DEBUG: Keyword token: %d\n", temp);
                    return temp;
                } else {
                    // Not a keyword, process as identifier
                    return process_identifier(buffer);
                }
            }

            if (isdigit(cbuf)) {  // Number
                return scan_number();
            }

            // Handle unexpected tokens
            debug_scan_printf("Unexpected token: %c at line %d\n", cbuf, linenum);
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
                    if (cbuf == '\n') linenum++; debug_scan_printf("Line number incremented to: %d\n", linenum);
                    if (cbuf == EOF) {
                        debug_scan_printf("Warning: Unterminated multi-line comment at line %d, skipping...\n", linenum);
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
            debug_scan_printf("DEBUG: Matched keyword: %s with token: %d\n", token_str, key[i].keytoken);
            return key[i].keytoken;
        }
    }
    debug_scan_printf("DEBUG: No match for keyword: %s\n", token_str);
    return -1;  // Return -1 if not a keyword
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
            if (cbuf == '\'') {
                // Double single quote - store as single quote
                tempbuf[i++] = '\'';
                continue;
            }
            // Single quote - end of string
            tempbuf[i] = '\0';
            strncpy(string_attr, tempbuf, MAXSTRSIZE - 1);
            string_attr[MAXSTRSIZE - 1] = '\0';
            debug_scan_printf("Processed string literal: '%s' (length: %d)\n", string_attr, (int)strlen(string_attr));
            return TSTRING;
        }
        tempbuf[i++] = cbuf;
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
// void error(const char *msg) {
//     fprintf(stderr, "Error: %s at line %d\n", msg, linenum);
//     debug_printf("Error reported at line: %d\n", linenum);
//     exit(EXIT_FAILURE);
// }

// Used for error-reporting function (parser or error handler)
int get_linenum(void) {
    debug_scan_printf("get_linenum called, returning: %d\n", linenum);
    return linenum;
}

// Clean up after scanning
void end_scan(void) {
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
}

int scan_number() {
    char num_buffer[MAXSTRSIZE] = {0};  // Use MAXSTRSIZE instead of 256
    int num_len = 0;
    num_attr = 0;

    // Store the original format while parsing with bounds checking
    while (isdigit(cbuf) && num_len < MAXSTRSIZE - 1) {  // Leave room for null terminator
        num_buffer[num_len++] = cbuf;  // Store original character
        num_attr = num_attr * 10 + (cbuf - '0');
        cbuf = (char) fgetc(fp);
    }
    num_buffer[num_len] = '\0';
    
    // Check for buffer overflow
    if (isdigit(cbuf)) {
        error("Number too long");  // Use the existing error function
        return -1;
    }
    
    // Store original format in string_attr using strncpy for safety
    strncpy(string_attr, num_buffer, MAXSTRSIZE - 1);
    string_attr[MAXSTRSIZE - 1] = '\0';  // Ensure null termination
    
    return TNUMBER;
}