#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "token.h"
#include "parser.h"

void error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

void check_type_compatibility(int left_type, int right_type) {
    if (left_type != right_type) {
        // Allow certain implicit conversions based on semantics
        if ((left_type == TINTEGER && right_type == TCHAR) ||
            (left_type == TBOOLEAN && right_type == TINTEGER)) {
            return;
        }
        error("Type mismatch");
    }
}

void check_array_bounds(int index, int size) {
    if (index < 0 || index >= size) {
        error("Array index out of bounds");
    }
}

void check_parameter_count(const char* proc_name, int expected, int actual) {
    if (expected != actual) {
        fprintf(stderr, "Procedure %s expects %d parameters but got %d\n", 
                proc_name, expected, actual);
        error("Parameter count mismatch");
    }
}

void check_parameter_types(const char* proc_name, int* expected_types, 
                         int* actual_types, int count) {
    for (int i = 0; i < count; i++) {
        if (expected_types[i] != actual_types[i]) {
            fprintf(stderr, "Parameter %d type mismatch in procedure %s\n", 
                    i + 1, proc_name);
            error("Parameter type mismatch");
        }
    }
}

// Symbol table implementation (simplified for now)
static SymbolEntry symbol_table[1000];
static int symbol_count = 0;
static char current_procedure[256] = "";

SymbolEntry* lookup_symbol(const char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

int is_current_procedure(const char* name) {
    return strcmp(current_procedure, name) == 0;
}

int get_procedure_param_count(const char* name) {
    SymbolEntry* entry = lookup_symbol(name);
    return entry ? entry->param_count : 0;
}

int* get_procedure_param_types(const char* name) {
    SymbolEntry* entry = lookup_symbol(name);
    return entry ? entry->param_types : NULL;
}

void check_division_by_zero(int value) {
    if (value == 0) {
        error("Division by zero");
    }
}

void check_arithmetic_overflow(int op, int val1, int val2) {
    // Check for 16-bit integer overflow
    long result;
    switch(op) {
        case TPLUS: result = (long)val1 + val2; break;
        case TMINUS: result = (long)val1 - val2; break;
        case TSTAR: result = (long)val1 * val2; break;
        default: return;
    }
    if (result > 32767 || result < -32768) {
        error("Arithmetic overflow");
    }
}

int convert_type(int value, int from_type, int to_type) {
    // Implement type conversion rules based on semantics
    if (from_type == TCHAR && to_type == TINTEGER) {
        return (int)value;
    }
    if (from_type == TBOOLEAN && to_type == TINTEGER) {
        return value ? 1 : 0;
    }
    // Add other conversions as needed
    return value;
}
