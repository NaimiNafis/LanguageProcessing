#include "scan.h"

FILE *input_file;
int line_number = 0;

int num_attr;
char string_attr[MAXSTRSIZE];

int init_scan(char *filename) {
    input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return -1;
    }
    line_number = 1;
    return 0;
}

#include <ctype.h>
#include <string.h>
#include "scan.h"

extern struct KEY key[KEYWORDSIZE];  // Keyword table

int scan() {
    int ch;
    
    // Skip whitespace and newlines
    do {
        ch = fgetc(input_file);
        if (ch == '\n') line_number++;
    } while (isspace(ch));
    
    if (ch == EOF) {
        return -1;  // End of file
    }
    
    // Handle identifiers and keywords
    if (isalpha(ch)) {
        int i = 0;
        // Collect the identifier or keyword
        do {
            string_attr[i++] = ch;
            ch = fgetc(input_file);
        } while (isalnum(ch) && i < MAXSTRSIZE - 1);
        
        string_attr[i] = '\0';  // Null-terminate the string
        ungetc(ch, input_file);  // Put back the last character

        // Check if the string is a keyword
        for (i = 0; i < KEYWORDSIZE; i++) {
            if (strcmp(string_attr, key[i].keyword) == 0) {
                return key[i].keytoken;  // Return keyword token
            }
        }

        return TNAME;  // Otherwise, return as an identifier (NAME token)
    }

    // Handle numbers
    if (isdigit(ch)) {
        ungetc(ch, input_file);
        fscanf(input_file, "%d", &num_attr);
        if (num_attr > 32767) {
            fprintf(stderr, "Error: Number too large\n");
            return -1;
        }
        return TNUMBER;  // Assuming TNUMBER is defined in token-list.h
    }

    // Handle operators and delimiters (example: `:=`)
    if (ch == ':') {
        int next = fgetc(input_file);
        if (next == '=') {
            return TASSIGN;  // Assuming TASSIGN is the token for `:=`
        }
        ungetc(next, input_file);
        return TCOLON;  // Assuming TCOLON is the token for `:`
    }

    // Add cases for other operators and delimiters (e.g., `;`, `(`, `)`, `+`, `-`)
    if (ch == ';') return TSEMI;
    if (ch == '(') return TLPAREN;
    if (ch == ')') return TRPAREN;
    if (ch == '+') return TPLUS;
    if (ch == '-') return TMINUS;

    return ch;  // Return the character itself if it's not a recognized token
}


int get_linenum() {
    return line_number;
}

void end_scan() {
    if (input_file != NULL) {
        fclose(input_file);
    }
}

int error(char *mes) {
    fprintf(stderr, "Error: %s\n", mes);
    exit(EXIT_FAILURE);
}
