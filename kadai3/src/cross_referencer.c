#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cross_referencer.h"
#include "debug.h"

static ID *symbol_table = NULL;

void init_cross_referencer(void) {
    symbol_table = NULL;
}

void add_symbol(char *name, int type, int linenum, int is_definition) {
    debug_printf("add_symbol: name=%s, type=%d, line=%d, is_def=%d\n", 
                name, type, linenum, is_definition);

    ID *existing = NULL;
    // First check if symbol already exists
    for (ID *id = symbol_table; id != NULL; id = id->nextp) {
        if (strcmp(id->name, name) == 0) {
            existing = id;
            break;
        }
    }

    if (is_definition) {
        if (existing == NULL) {
            // Create new symbol for definition
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
            debug_printf("Created new symbol definition: %s at line %d\n", name, linenum);
        }
    } else if (existing != NULL) {
        // Only add reference if symbol exists and it's not a definition
        add_reference(name, linenum);
    }
}

void add_reference(char *name, int linenum) {
    debug_printf("add_reference: name=%s, line=%d\n", name, linenum);
    
    ID *id = symbol_table;
    while (id != NULL) {
        if (strcmp(id->name, name) == 0) {
            Line *new_line = (Line *)malloc(sizeof(Line));
            new_line->reflinenum = linenum;
            new_line->nextlinep = id->irefp;
            id->irefp = new_line;
            debug_printf("Added reference for %s at line %d\n", name, linenum);
            return;
        }
        id = id->nextp;
    }
    debug_printf("Warning: No symbol found for reference: %s\n", name);
}

// Helper function to compare IDs
int compare_ids(const void *a, const void *b) {
    ID *id1 = *(ID **)a;
    ID *id2 = *(ID **)b;
    return strcmp(id1->name, id2->name);
}

// Helper function to convert type token to string
const char* type_to_string(int type) {
    switch(type) {
        case TINTEGER: return "integer";
        case TBOOLEAN: return "boolean";
        case TCHAR: return "char";
        default: return "unknown";
    }
}

// Add a function to sort references by line number
void sort_references(Line** head) {
    if (*head == NULL || (*head)->nextlinep == NULL) return;
    
    Line *sorted = NULL;
    Line *current = *head;
    
    while (current != NULL) {
        Line *next = current->nextlinep;
        Line *scan = sorted;
        Line *scan_prev = NULL;
        
        while (scan != NULL && scan->reflinenum < current->reflinenum) {
            scan_prev = scan;
            scan = scan->nextlinep;
        }
        
        if (scan_prev == NULL) {
            current->nextlinep = sorted;
            sorted = current;
        } else {
            current->nextlinep = scan_prev->nextlinep;
            scan_prev->nextlinep = current;
        }
        
        current = next;
    }
    
    *head = sorted;
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
        printf("%s|%s|%d|", id->name, type_to_string(id->itp->ttype), id->deflinenum);
        
        // Sort references before printing
        sort_references(&id->irefp);
        
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