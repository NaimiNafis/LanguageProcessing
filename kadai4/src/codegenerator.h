#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <stdio.h>

#include "parser.h"

// File pointer for CASL output
extern FILE* caslfp;

// Declare function prototypes moved from compiler.c
void gen_save_registers(void);
void gen_restore_registers(void);
void gen_code(char *opc, const char *opr);
void gen_code_label(char *opc, char *opr, int label);
void gen_label(int labelnum);
int  get_label_num(void);
void gen_variable_allocation(const char* name, int size);
void gen_array_allocation(const char* name, int size);
void gen_temp_var_section(void);
void gen_temp_var(const char* prefix);
void gen_push(void);
void gen_pop(const char* reg);
void gen_push_address(const char* var);
void gen_push_expression_address(void);
void gen_add(void);
void gen_subtract(void);
void gen_multiply(void);
void gen_divide(void);
void gen_and(void);
void gen_or(void);
void gen_not(void);
void gen_load(const char* var);
void gen_store(const char* var);
void gen_array_access(const char* array_name, const char* index_reg);
void gen_data_section_start(void);
void gen_data_section_end(void);
void gen_program_start(const char* name);
void gen_program_end(void);
void gen_procedure_entry(const char* name);
void gen_procedure_exit(void);
void gen_procedure_call(const char* name);
void gen_formal_parameter(const char* param_name, const char* proc_name);
void gen_local_variable(const char* var_name, const char* proc_name);
void gen_read(const char* var);
void gen_write(const char* var, int width);
void gen_write_string(const char* msg);
void gen_compare(int op);
void gen_runtime_error_handlers(void);
void gen_bounds_check(void);
void gen_div_check(void);
void gen_overflow_check(void);
void gen_type_conversion(int from_type, int to_type);
void gen_lib_subroutines(void);

// Register allocation optimization
void gen_optimize_registers(void);
void gen_free_register(int reg);
int  gen_alloc_register(void);

#endif