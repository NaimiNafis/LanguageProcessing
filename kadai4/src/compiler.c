#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "compiler.h"
#include "token.h"
#include "parser.h"
#include "debug.h"
#include "scan.h"
#include "codegenerator.h"
#include "error.h"

// Add debug print function at the top
static void debug_compiler_printf(const char *format, ...) {
    if (debug_compiler) {
        va_list args;
        va_start(args, format);
        printf("[COMPILER] ");
        vprintf(format, args);
        va_end(args);
    }
}

// Error handling
void error(const char* message) {
    debug_compiler_printf("Error encountered: %s at line %d\n", message, get_linenum());
    fprintf(stderr, "Error: %s at line %d\n", message, get_linenum());
    exit(1);
}

void check_type_compatibility(int left_type, int right_type) {
    debug_compiler_printf("Checking type compatibility: left=%d, right=%d\n", left_type, right_type);
    // Allow readln/writeln to work with any type by skipping type check
    if (parser.previous_token == TREADLN || parser.previous_token == TWRITELN) {
        return;
    }
    
    if (left_type != right_type) {
        // Allow certain implicit conversions based on semantics
        if ((left_type == TINTEGER && right_type == TCHAR) ||
            (left_type == TBOOLEAN && right_type == TINTEGER) ||
            (left_type == TINTEGER && right_type == TBOOLEAN)) {
            return;
        }
        error("Type mismatch");
    }
}

void check_array_bounds(int index, int size) {
    debug_compiler_printf("Checking array bounds: index=%d, size=%d\n", index, size);
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

// Add code to handle arithmetic expressions
void gen_arithmetic(int op, const char* left, const char* right) {
    fprintf(caslfp, "    LD GR1, %s\n", left);
    fprintf(caslfp, "    PUSH 0, GR1\n");
    fprintf(caslfp, "    LD GR1, %s\n", right);
    
    switch(op) {
        case TPLUS:
            fprintf(caslfp, "    POP GR2\n");
            fprintf(caslfp, "    ADDA GR1, GR2\n");
            fprintf(caslfp, "    JOV EOVF\n");
            break;
        case TMINUS:
            // Similar for other operators
            break;
    }
}