#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cross_referencer.h"
#include "debug.h"
#include "scan.h"

// Global state management for symbol processing
static ID *symbol_table = NULL;
static char *current_procedure = NULL;
static ID *current_procedure_id = NULL;
static Type* current_symbol_type = NULL;
static int error_state = 0;

// Parameter type tracking for procedures
struct ParamType {
    int type;
    struct ParamType *next;
};

// Array type construction state
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
        case TPROCEDURE: {
            if (current_symbol_type && current_symbol_type->paratp) {
                // Start with "procedure"
                strcpy(array_type, "procedure(");
                
                // Add parameter types
                struct ParamType *param = current_symbol_type->paratp;
                while (param) {
                    strcat(array_type, type_to_string(param->type));
                    if (param->next) {
                        strcat(array_type, ",");
                    }
                    param = param->next;
                }
                strcat(array_type, ")");
                return array_type;
            }
            return "procedure";
        }
        case TARRAY: {
            // Get array info from symbol being processed
            Type* array_type_info = current_symbol_type;
            if (array_type_info && array_type_info->etp) {
                snprintf(array_type, sizeof(array_type), "array[%d]of%s",
                        array_type_info->arraysize,
                        type_to_string(array_type_info->etp->ttype));
                return array_type;
            }
            // Fallback for incomplete array info
            return "array[0]ofunknown";
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

// Helper function to add parameter type to procedure
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
            new_line->nextlinep = NULL;
            
            // Insert in sorted order
            if (!id->irefp || id->irefp->reflinenum > linenum) {
                // Insert at start
                new_line->nextlinep = id->irefp;
                id->irefp = new_line;
            } else {
                // Find insertion point
                Line *current = id->irefp;
                while (current->nextlinep && current->nextlinep->reflinenum < linenum) {
                    current = current->nextlinep;
                }
                new_line->nextlinep = current->nextlinep;
                current->nextlinep = new_line;
            }
            
            found = 1;
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
                new_line->nextlinep = NULL;
                
                // Insert in sorted order
                if (!id->irefp || id->irefp->reflinenum > linenum) {
                    // Insert at start
                    new_line->nextlinep = id->irefp;
                    id->irefp = new_line;
                } else {
                    // Find insertion point
                    Line *current = id->irefp;
                    while (current->nextlinep && current->nextlinep->reflinenum < linenum) {
                        current = current->nextlinep;
                    }
                    new_line->nextlinep = current->nextlinep;
                    current->nextlinep = new_line;
                }
                
                found = 1;
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

// Helper function to manage procedure scope
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

// Helper function to access current_procedure
const char* get_current_procedure(void) {
    return current_procedure;
}

// Forward declarations of helper functions
static char* get_base_name(const char* full_name);
static char* normalize_name(const char* name);
static int compare_ids(const void *a, const void *b);
static void sort_references(Line** head);

// Fix compare_ids function to properly sort symbols
static int compare_ids(const void *a, const void *b) {
    ID *id1 = *(ID **)a;
    ID *id2 = *(ID **)b;

    // Procedures must come before variables
    if (id1->itp->ttype == TPROCEDURE && id2->itp->ttype != TPROCEDURE) {
        return -1;
    }
    if (id1->itp->ttype != TPROCEDURE && id2->itp->ttype == TPROCEDURE) {
        return 1;
    }

    // If both are procedures, sort by name
    if (id1->itp->ttype == TPROCEDURE && id2->itp->ttype == TPROCEDURE) {
        return strcmp(id1->name, id2->name);
    }

    // If both are variables, sort by definition line number
    return id1->deflinenum - id2->deflinenum;
}

// Fixed sort_references function to maintain ascending order
static void sort_references(Line** head) {
    if (!*head || !(*head)->nextlinep) return;  // Nothing to sort
    
    // Count references
    int count = 0;
    Line* current = *head;
    while (current) {
        count++;
        current = current->nextlinep;
    }
    
    // Create array of line numbers
    int* numbers = malloc(count * sizeof(int));
    current = *head;
    for (int i = 0; i < count; i++) {
        numbers[i] = current->reflinenum;
        current = current->nextlinep;
    }
    
    // Sort array in ascending order
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
    
    // Rebuild list in correct order (forward, not reverse)
    Line* sorted = NULL;
    Line* tail = NULL;
    for (int i = 0; i < count; i++) {  // Changed to forward iteration
        Line* new_line = malloc(sizeof(Line));
        new_line->reflinenum = numbers[i];
        new_line->nextlinep = NULL;
        
        if (!sorted) {
            sorted = new_line;
            tail = new_line;
        } else {
            tail->nextlinep = new_line;
            tail = new_line;
        }
    }
    
    // Clean up original list
    while (*head) {
        current = *head;
        *head = (*head)->nextlinep;
        free(current);
    }
    
    free(numbers);
    *head = sorted;
}

// Helper function to remove procedure name from scoped variables
static char* get_base_name(const char* full_name) {
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

void set_error_state(void) {
    error_state = 1;
}

int is_error_state(void) {
    return error_state;
}

// Helper function to get display name for symbol
static char* get_display_name(const ID* id) {
    if (!id->procname || id->itp->ttype == TPROCEDURE) {
        return get_base_name(id->name);
    }
    
    // For variables in procedures, show as "name:procedure"
    char* base = get_base_name(id->name);
    char* result = malloc(strlen(base) + strlen(id->procname) + 2);
    sprintf(result, "%s:%s", base, id->procname);
    free(base);
    return result;
}

// Modified print_cross_reference_table to handle procedure parameters
void print_cross_reference_table(void) {
    if (scanner.has_error || error_state) {
        return;
    }

    // Count symbols and create array
    int count = 0;
    for (ID *id = symbol_table; id != NULL; id = id->nextp) {
        count++;
    }
    
    ID **id_array = (ID **)malloc(count * sizeof(ID *));
    if (!id_array) return;
    
    // Fill array
    ID *id = symbol_table;
    for (int i = 0; i < count; i++) {
        id_array[i] = id;
        id = id->nextp;
    }

    // Sort array
    qsort(id_array, count, sizeof(ID *), compare_ids);

    // Print header line
    printf("----------------------------------\n");

    // Print sorted symbols
    for (int i = 0; i < count; i++) {
        id = id_array[i];
        char* display_name = get_display_name(id);
        
        current_symbol_type = id->itp;
        
        // Print symbol entry
        printf("%s|%s|%d|", display_name, type_to_string(id->itp->ttype), 
               id->deflinenum);
        
        // Print references
        Line *line = id->irefp;
        if (line) {
            printf("%d", line->reflinenum);
            line = line->nextlinep;
            while (line) {
                printf(",%d", line->reflinenum);
                line = line->nextlinep;
            }
        }
        printf("\n");
        
        free(display_name);
    }

    free(id_array);
}