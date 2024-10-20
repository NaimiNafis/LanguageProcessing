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

int scan() {
    int ch = fgetc(input_file);

    if (ch == EOF) {
        return -1; 
    }

    if (ch == '\n') {
        line_number++;
        return scan();
    }

    // Example: handle digits (integers)
    if (ch >= '0' && ch <= '9') {
        ungetc(ch, input_file);  // Put the character back to read the entire number
        fscanf(input_file, "%d", &num_attr);
        if (num_attr > 32767) {
            fprintf(stderr, "Error: Number too large\n");
            return -1;
        }
        return TNUMBER;
    }

    return ch;
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
