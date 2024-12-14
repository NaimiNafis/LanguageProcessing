#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cross_referencer.h"

static ID *symbol_table = NULL;

void init_cross_referencer(void) {
    symbol_table = NULL;
}

void add_symbol(char *name, int type, int linenum, int is_definition) {
    ID *new_id = (ID *)malloc(sizeof(ID));
    new_id->name = strdup(name);
    new_id->procname = NULL;
    new_id->itp = (Type *)malloc(sizeof(Type));
    new_id->itp->ttype = type;
    new_id->ispara = 0;
    new_id->deflinenum = linenum;
    new_id->irefp = NULL;
    new_id->nextp = symbol_table;
    symbol_table = new_id;

    if (!is_definition) {
        add_reference(name, linenum);
    }
}

void add_reference(char *name, int linenum) {
    ID *id = symbol_table;
    while (id != NULL) {
        if (strcmp(id->name, name) == 0) {
            Line *new_line = (Line *)malloc(sizeof(Line));
            new_line->reflinenum = linenum;
            new_line->nextlinep = id->irefp;
            id->irefp = new_line;
            return;
        }
        id = id->nextp;
    }
}

// Helper function to compare IDs
int compare_ids(const void *a, const void *b) {
    ID *id1 = *(ID **)a;
    ID *id2 = *(ID **)b;
    return strcmp(id1->name, id2->name);
}

// Ensure this function is defined only once
void print_cross_reference_table(void) {
    // Count symbols
    int count = 0;
    ID *id = symbol_table;
    while (id != NULL) {
        count++;
        id = id->nextp;
    }

    // Create array of pointers to IDs
    ID **id_array = (ID **)malloc(count * sizeof(ID *));
    id = symbol_table;
    for (int i = 0; i < count; i++) {
        id_array[i] = id;
        id = id->nextp;
    }

    // Sort array
    qsort(id_array, count, sizeof(ID *), compare_ids);

    // Print header line that will be discarded
    printf("----------------------------------\n");

    // Print sorted table - one entry per line, no spaces
    for (int i = 0; i < count; i++) {
        id = id_array[i];
        // Print without spaces: name|type|definitionline|references
        printf("%s|%d|%d|", id->name, id->itp->ttype, id->deflinenum);
        
        // Print references with commas, no spaces
        Line *line = id->irefp;
        if (line != NULL) {
            printf("%d", line->reflinenum);
            line = line->nextlinep;
            while (line != NULL) {
                printf(",%d", line->reflinenum);
                line = line->nextlinep;
            }
        }
        printf("\n");
    }

    free(id_array);
}