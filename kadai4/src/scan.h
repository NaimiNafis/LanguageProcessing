#ifndef SCAN_H
#define SCAN_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "token.h"

#define MAXSTRSIZE 1024
#define S_ERROR -1

typedef struct {
    FILE *fp;
    int line_number;
    int has_error;
} Scanner;

extern Scanner scanner;
extern int num_attr;
extern char string_attr[MAXSTRSIZE];

// Function declarations
void error(const char *msg);
int init_scan(const char *filename);
int scan(void);
int get_linenum(void);
void end_scan(void);
extern const char* get_current_file(void);

#endif