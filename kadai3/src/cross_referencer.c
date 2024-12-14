#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cross_referencer.h"
#include "debug.h"
#include "scan.h"  // Add this include to get num_attr

static ID *symbol_table = NULL;
static char *current_procedure = NULL;  // Add this at the top with other globals
static ID *current_procedure_id = NULL; // Add this to track current procedure ID
static Type* current_symbol_type = NULL; // Add at the top with other static variables

// Add new struct to track procedure parameters
struct ParamType {
    int type;
    struct ParamType *next;
};

// Add static variables to store array info
static int current_array_size = 0;
static int current_base_type = 0;

void set_array_info(int size, int base_type) {
    debug_printf("Setting array info: size=%d, base_type=%d\n", size, base_type);
    current_array_size = size;
    current_base_type = base_type;
}

Type* create_array_type(int size, int base_type) {
    Type* array_type = (Type*)malloc(sizeof(Type));
    if (!array_type) {
        error("Memory allocation failed for array type");
        return NULL;
    }
    
    array_type->ttype = TARRAY;
    array_type->arraysize = size;  // Use the size directly
    array_type->etp = (Type*)malloc(sizeof(Type));
    if (!array_type->etp) {
        free(array_type);
        error("Memory allocation failed for array element type");
        return NULL;
    }
    
    array_type->etp->ttype = base_type;
    array_type->etp->arraysize = 0;
    array_type->etp->etp = NULL;
    array_type->paratp = NULL;
    
    debug_printf("Created array type: size=%d, base_type=%d\n", 
                size, base_type);
    
    return array_type;
}

void init_cross_referencer(void) {
    symbol_table = NULL;
}

// Modify type_to_string to use stored info directly
const char* type_to_string(int type) {
    static char array_type[256];  // Static buffer for array type string
    switch(type) {
        case TINTEGER: return "integer";
        case TBOOLEAN: return "boolean";
        case TCHAR: return "char";
        case TPROCEDURE: return "procedure";
        case TARRAY: {
            // Get array info from symbol being processed
            Type* array_type_info = current_symbol_type;  // You'll need to add this as a static variable
            if (array_type_info) {
                snprintf(array_type, sizeof(array_type), "array[%d]of%s",
                        array_type_info->arraysize,
                        type_to_string(array_type_info->etp->ttype));
            } else {
                snprintf(array_type, sizeof(array_type), "array[%d]of%s",
                        current_array_size,
                        type_to_string(current_base_type));
            }
            return array_type;
        }
        default: return "unknown";
    }
}

// Modify get_param_string function to handle multiple parameters correctly
char* get_param_string(struct ParamType *params) {
    if (!params) return strdup("");
    
    char *result = malloc(256);  // Reasonable buffer size
    result[0] = '(';
    result[1] = '\0';
    
    struct ParamType *current = params;
    while (current) {
        const char *type_str = type_to_string(current->type);
        strcat(result, type_str);
        current = current->next;
        if (current) {
            strcat(result, ",");
        }
    }
    strcat(result, ")");
    return result;
}

// Modify add_symbol to handle array types
void add_symbol(char *name, int type, int linenum, int is_definition) {
    // Don't add symbols if there was a parse error
    if (scanner.has_error) {
        return;
    }

    debug_printf("add_symbol: name=%s, type=%d, line=%d, is_def=%d, proc=%s\n", 
                name, type, linenum, is_definition, 
                current_procedure ? current_procedure : "global");

    ID *existing = NULL;
    char *scoped_name = NULL;
    char *lookup_name = NULL;

    // Use linenum as-is, don't modify it for variable definitions
    // This ensures we keep the line number from where the variable was declared

    // Create scoped name for variables in procedures
    if (current_procedure && type != TPROCEDURE) {
        size_t len = strlen(name) + strlen(current_procedure) + 2;
        scoped_name = malloc(len);
        snprintf(scoped_name, len, "%s:%s", name, current_procedure);
        lookup_name = scoped_name;
    } else {
        lookup_name = name;
    }

    // First look for existing symbol with exact name match
    for (ID *id = symbol_table; id != NULL; id = id->nextp) {
        if (strcmp(id->name, lookup_name) == 0) {
            existing = id;
            break;
        }
    }

    if (is_definition) {
        if (!existing) {
            ID *new_id = (ID *)malloc(sizeof(ID));
            new_id->name = scoped_name ? scoped_name : strdup(name);
            new_id->procname = current_procedure ? strdup(current_procedure) : NULL;

            // Handle array type
            if (type == TARRAY) {
                new_id->itp = create_array_type(current_array_size, current_base_type);
            } else {
                new_id->itp = (Type *)malloc(sizeof(Type));
                new_id->itp->ttype = type;
                new_id->itp->arraysize = 0;
                new_id->itp->etp = NULL;
                new_id->itp->paratp = NULL;
            }

            new_id->ispara = (current_procedure != NULL && type != TPROCEDURE);
            new_id->deflinenum = linenum;
            new_id->irefp = NULL;
            new_id->nextp = symbol_table;
            symbol_table = new_id;
            
            if (type == TPROCEDURE) {
                current_procedure_id = new_id;
            }
        } else if (scoped_name) {
            free(scoped_name);
        }
    } else {
        // For references, try scoped name first, then global
        if (!existing) {
            // Try global lookup if scoped lookup failed
            for (ID *id = symbol_table; id != NULL; id = id->nextp) {
                if (strcmp(id->name, name) == 0) {
                    existing = id;
                    break;
                }
            }
        }
        
        if (existing) {
            Line *new_line = (Line *)malloc(sizeof(Line));
            new_line->reflinenum = linenum;
            new_line->nextlinep = existing->irefp;
            existing->irefp = new_line;
        }
        
        if (scoped_name) free(scoped_name);
    }
}

// Add function to add parameter type to procedure
void add_procedure_parameter(int type) {
    if (!current_procedure_id || current_procedure_id->itp->ttype != TPROCEDURE) return;
    
    struct ParamType *new_param = malloc(sizeof(struct ParamType));
    new_param->type = type;
    new_param->next = NULL;
    
    if (!current_procedure_id->itp->paratp) {
        current_procedure_id->itp->paratp = new_param;
    } else {
        struct ParamType *last = current_procedure_id->itp->paratp;
        while (last->next) last = last->next;
        last->next = new_param;
    }
}

void add_reference(char *name, int linenum) {
    debug_printf("add_reference: name=%s, line=%d, current_proc=%s\n", 
                name, linenum, current_procedure ? current_procedure : "global");
    
    ID *id = symbol_table;
    char *scoped_name = NULL;
    char *global_name = strdup(name);
    int found = 0;

    // First try with current procedure scope
    if (current_procedure) {
        size_t len = strlen(name) + strlen(current_procedure) + 2;
        scoped_name = malloc(len);
        snprintf(scoped_name, len, "%s:%s", name, current_procedure);
    }

    // First look for scoped version
    while (id != NULL) {
        if (scoped_name && strcmp(id->name, scoped_name) == 0) {
            Line *new_line = (Line *)malloc(sizeof(Line));
            new_line->reflinenum = linenum;
            new_line->nextlinep = id->irefp;
            id->irefp = new_line;
            found = 1;
            debug_printf("Added scoped reference for %s at line %d\n", scoped_name, linenum);
            break;
        }
        id = id->nextp;
    }

    // If not found in scope, look for global
    if (!found) {
        id = symbol_table;
        while (id != NULL) {
            if (strcmp(id->name, global_name) == 0) {
                Line *new_line = (Line *)malloc(sizeof(Line));
                new_line->reflinenum = linenum;
                new_line->nextlinep = id->irefp;
                id->irefp = new_line;
                found = 1;
                debug_printf("Added global reference for %s at line %d\n", global_name, linenum);
                break;
            }
            id = id->nextp;
        }
    }

    if (!found) {
        debug_printf("Warning: No symbol found for reference: %s (scoped: %s)\n", 
                    name, scoped_name ? scoped_name : "none");
    }

    if (scoped_name) free(scoped_name);
    free(global_name);
}

// Add these functions to manage procedure scope
void enter_procedure(const char *name) {
    if (current_procedure) free(current_procedure);
    current_procedure = strdup(name);
    debug_printf("Entering procedure scope: %s\n", name);
}

void exit_procedure(void) {
    if (current_procedure) {
        debug_printf("Exiting procedure scope: %s\n", current_procedure);
        free(current_procedure);
        current_procedure = NULL;
        current_procedure_id = NULL; // Reset current procedure ID
    }
}

// Add this function to access current_procedure
const char* get_current_procedure(void) {
    return current_procedure;
}

// Helper function to compare IDs
int compare_ids(const void *a, const void *b) {
    ID *id1 = *(ID **)a;
    ID *id2 = *(ID **)b;
    return strcmp(id1->name, id2->name);
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

// Helper function to remove procedure name from scoped variables
char* get_base_name(const char* full_name) {
    char *colon = strchr(full_name, ':');
    if (colon) {
        size_t base_len = colon - full_name;
        char *base = malloc(base_len + 1);
        strncpy(base, full_name, base_len);
        base[base_len] = '\0';
        return base;
    }
    return strdup(full_name);
}

// Helper function to normalize variable scoping
char* normalize_name(const char* name) {
    if (!name) return NULL;
    
    // For display purposes, handle scoped names
    char* colon = strchr(name, ':');
    if (colon) {
        // For procedure variables, format as "name:proc"
        char* proc_part = colon + 1;
        char* var_part = strdup(name);
        var_part[colon - name] = '\0';
        size_t len = strlen(var_part) + strlen(proc_part) + 2;
        char* result = malloc(len);
        snprintf(result, len, "%s:%s", var_part, proc_part);
        free(var_part);
        return result;
    }
    return strdup(name);
}

void print_cross_reference_table(void) {
    // Don't print table if there was a parse error
    if (scanner.has_error) {
        return;
    }

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

    // Print sorted table - modified for scoped names
    for (int i = 0; i < count; i++) {
        id = id_array[i];
        char* display_name = normalize_name(id->name);
        
        // Set the current symbol type before printing
        current_symbol_type = id->itp;
        
        if (id->itp->ttype == TPROCEDURE && id->itp->paratp) {
            // Print procedure with parameter types
            char* param_str = get_param_string((struct ParamType*)id->itp->paratp);
            printf("%s|%s%s|%d|", display_name, type_to_string(id->itp->ttype), 
                   param_str, id->deflinenum);
            free(param_str);
        } else {
            // Print normal symbol
            printf("%s|%s|%d|", display_name, type_to_string(id->itp->ttype), 
                   id->deflinenum);
        }
        free(display_name);
        
        // Reset the current symbol type
        current_symbol_type = NULL;
        
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