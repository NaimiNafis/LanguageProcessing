#ifndef SCAN_H
#define SCAN_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "token.h"

#define MAXSTRSIZE 1024
#define S_ERROR -1

extern int num_attr;
extern char string_attr[MAXSTRSIZE];

// Function declarations
void error(const char *msg);
int init_scan(char *filename);
int scan(void);
int get_linenum(void);
void end_scan(void);

#endif