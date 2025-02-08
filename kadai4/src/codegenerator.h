#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <stdio.h>

#include "parser.h"

// File pointer for CASL output
extern FILE* caslfp;

// Program structure
void gen_program_start(const char* name);
void gen_program_end(void);
void gen_data_section_start(void);
void gen_data_section_end(void);

// Variable and procedure handling
void gen_variable_allocation(const char* name, int size);
void gen_array_allocation(const char* name, int size);
void gen_procedure_entry(const char* name);
void gen_procedure_exit(void);
void gen_procedure_param(const char* proc_name, const char* param_name);
void gen_procedure_local_var(const char* proc_name, const char* var_name);

// Code generation helpers
void gen_label(const char* label);
void gen_comment(const char* comment);
void gen_pascal_comment(const char* comment);

// Error handlers
void gen_runtime_error_handlers(void);

// Add these declarations
void gen_code(const char* opc, const char* opr);
void gen_push(void);
void gen_add(void);
void gen_subtract(void);
void gen_or(void);
void gen_div_check(void);
void gen_multiply(void);
void gen_overflow_check(void);
void gen_and(void);
void gen_push_address(const char* var_name);
void gen_push_expression_address(void);
void gen_procedure_call(const char* name, int param_count);  // Modified signature
void gen_array_access(const char* array_name, const char* index_reg);
void gen_bounds_check(void);
void gen_load(const char* var_name);
void gen_store(const char* var_name);

#endif