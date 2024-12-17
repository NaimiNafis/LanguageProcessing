#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <stdio.h>

// File pointer for CASL output
extern FILE* caslfp;

// Add before other declarations
void gen_save_registers(void);
void gen_restore_registers(void);

// Code generation basics
void gen_code(char *opc, const char *opr);
void gen_code_label(char *opc, char *opr, int label);
void gen_label(int labelnum);
int get_label_num(void);

// Memory and variable handling
void gen_variable_allocation(const char* name, int size);
void gen_array_allocation(const char* name, int size);
void gen_temp_var_section(void);
void gen_temp_var(const char* prefix);

// Stack operations
void gen_push(void);
void gen_pop(const char* reg);
void gen_push_address(const char* var);
void gen_push_expression_address(void);

// Arithmetic and logical operations
void gen_add(void);
void gen_subtract(void);
void gen_multiply(void);
void gen_divide(void);
void gen_and(void);
void gen_or(void);
void gen_not(void);

// Memory access
void gen_load(const char* var);
void gen_store(const char* var);
void gen_array_access(const char* array_name, const char* index_reg);

// Program structure
void gen_data_section_start(void);
void gen_data_section_end(void);
void gen_program_start(const char* name);
void gen_program_end(void);

// Procedure handling
void gen_procedure_entry(const char* name);
void gen_procedure_exit(void);
void gen_procedure_call(const char* name);
void gen_formal_parameter(const char* param_name, const char* proc_name);
void gen_local_variable(const char* var_name, const char* proc_name);

// Runtime checks
void gen_runtime_error_handlers(void);
void gen_bounds_check(void);
void gen_div_check(void);
void gen_overflow_check(void);

// Type conversion
void gen_type_conversion(int from_type, int to_type);

#endif
