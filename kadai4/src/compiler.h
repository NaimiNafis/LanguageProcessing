#ifndef COMPILER_H
#define COMPILER_H

#include "parser.h"

// Error handling
void error(const char* message);

// Type checking and semantic validation
void check_type_compatibility(int left_type, int right_type);
void check_array_bounds(int index, int size);
void check_division_by_zero(int value);
void check_arithmetic_overflow(int op, int val1, int val2);
void check_parameter_count(const char* proc_name, int expected, int actual);
void check_parameter_types(const char* proc_name, int* expected_types, int* actual_types, int count);

// Symbol table functions
SymbolEntry* lookup_symbol(const char* name);
int is_current_procedure(const char* name);
int get_procedure_param_count(const char* name);
int* get_procedure_param_types(const char* name);

// Type conversion
int convert_type(int value, int from_type, int to_type);

// test
#endif
