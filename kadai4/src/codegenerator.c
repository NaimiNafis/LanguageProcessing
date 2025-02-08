#include <stdio.h>
#include <string.h>
#include "codegenerator.h"
#include "compiler.h"
#include "token.h"

// Define caslfp here (not just declare)
FILE* caslfp = NULL;
static int temp_var_count = 0;
static int label_counter = 1;
static char current_proc[256] = "";

int get_label_num(void) {
    return label_counter++;
}

void gen_label(const char* label) {
    fprintf(caslfp, "L%04d\n", label_counter++);
}

void gen_code(const char* opc, const char* opr) {
    fprintf(caslfp, "\t%s\t%s\n", opc, opr);
}

void gen_code_label(char *opc, char *opr, int label) {
    fprintf(caslfp, "\t%s\t%sL%04d\n", opc, opr, label);
}

// Add variable allocation
void gen_variable_allocation(const char* name, int size) {
    if (current_proc[0] == '\0') {
        // Global variable
        fprintf(caslfp, "$%s  DC 0\n", name);
        fprintf(caslfp, "; %s : integer;\n\n", name);
    } else {
        // Local variable
        fprintf(caslfp, "$%s%%%s  DC 0\n", name, current_proc);
        fprintf(caslfp, "; %s : integer;\n\n", name);
    }
}

// Add array allocation
void gen_array_allocation(const char* name, int size) {
    if (current_proc[0] == '\0') {
        fprintf(caslfp, "$%s  DS %d\n", name, size);
    } else {
        fprintf(caslfp, "$%s%%%s  DS %d\n", name, current_proc, size);
    }
    fprintf(caslfp, "; %s : array[%d] of integer;\n\n", name, size);
}

// Add stack operations
void gen_push(void) {
    gen_code("PUSH", "0,GR1");
}

void gen_pop(const char* reg) {
    gen_code("POP", reg);
}

// Add arithmetic operations
void gen_add(void) {
    gen_code("POP", "GR2");
    gen_code("ADDA", "GR1,GR2");
}

void gen_multiply(void) {
    gen_code("POP", "GR2");
    gen_code("MULA", "GR1,GR2");
}

// Add array access
void gen_array_access(const char* array_name, const char* index_reg) {
    fprintf(caslfp, "\tLD\tGR1,$%s,%s\n", array_name, index_reg);
}

// Program structure
void gen_data_section(void) {
    fprintf(caslfp, "\tSTART\n");
    fprintf(caslfp, "LIBBUF\tDS\t256\n");  // Buffer for library routines
}

// Implement proper memory sections
void gen_data_section_start(void) {
    // Empty implementation - don't generate START and LIBBUF
}

void gen_data_section_end(void) {
    // Empty implementation - don't generate END
}

void gen_program_start(const char* name) {
    // Clear any existing content in the file by seeking to start
    fseek(caslfp, 0, SEEK_SET);
    fprintf(caslfp, "%%%s START L0001\n\n", name);
    fprintf(caslfp, "; program %s;\n\n", name);

    // Move runtime error handlers to the very end
    // Remove any call to gen_runtime_error_handlers here
}

void gen_program_end(void) {
    fprintf(caslfp, "    CALL FLUSH\n");
    fprintf(caslfp, "    RET\n");
    fprintf(caslfp, "    ; end.\n\n");
    // Add runtime error handlers at the very end
    gen_runtime_error_handlers();
}

// Register management
void gen_save_registers(void) {
    fprintf(caslfp, "\tPUSH\t0,GR1\n");
    fprintf(caslfp, "\tPUSH\t0,GR2\n");
    fprintf(caslfp, "\tPUSH\t0,GR3\n");
}

void gen_restore_registers(void) {
    fprintf(caslfp, "\tPOP\tGR3\n");
    fprintf(caslfp, "\tPOP\tGR2\n");
    fprintf(caslfp, "\tPOP\tGR1\n");
}

// Arithmetic operations
void gen_load(const char* var) {
    fprintf(caslfp, "\tLD\tGR1,$%s\n", var);
}

void gen_store(const char* var) {
    fprintf(caslfp, "\tST\tGR1,$%s\n", var);
}

void gen_subtract(void) {
    fprintf(caslfp, "\tPOP\tGR2\n");
    fprintf(caslfp, "\tSUBA\tGR1,GR2\n");
}

void gen_divide(void) {
    fprintf(caslfp, "\tPOP\tGR2\n");
    fprintf(caslfp, "\tDIVA\tGR2,GR1\n");
    fprintf(caslfp, "\tLD\tGR1,GR2\n");
}

// Boolean operations
void gen_and(void) {
    fprintf(caslfp, "\tPOP\tGR2\n");
    fprintf(caslfp, "\tAND\tGR1,GR2\n");
}

void gen_or(void) {
    fprintf(caslfp, "\tPOP\tGR2\n");
    fprintf(caslfp, "\tOR\tGR1,GR2\n");
}

void gen_not(void) {
    fprintf(caslfp, "\tXOR\tGR1,=#FFFF\n");
}

void gen_compare(int op) {
    fprintf(caslfp, "\tCPA\tGR1,GR2\n");
    switch(op) {
        case TEQUAL:  fprintf(caslfp, "\tJZE\t"); break;
        case TNOTEQ:  fprintf(caslfp, "\tJNZ\t"); break;
        case TLE:     fprintf(caslfp, "\tJMI\t"); break;
        case TLEEQ:   fprintf(caslfp, "\tJLE\t"); break;
        case TGR:     fprintf(caslfp, "\tJPL\t"); break;
        case TGREQ:   fprintf(caslfp, "\tJGE\t"); break;
    }
}

// Procedure handling
void gen_procedure_entry(const char* name) {
    strncpy(current_proc, name, sizeof(current_proc) - 1);
    fprintf(caslfp, "\n$%s\n", name);
    // Add parameter handling
    fprintf(caslfp, "    POP GR2\n");
    fprintf(caslfp, "    POP GR1\n");
    // Store parameters as needed
    fprintf(caslfp, "    PUSH 0, GR2\n\n");
    fprintf(caslfp, "    ; begin\n");
}

void gen_procedure_exit(void) {
    fprintf(caslfp, "    RET\n");
    fprintf(caslfp, "    ; end;\n\n");
    current_proc[0] = '\0';
}

void gen_procedure_call(const char* name, int param_count) {
    fprintf(caslfp, "    LAD GR1, $%s\n", name);
    fprintf(caslfp, "    PUSH 0, GR1\n");
    fprintf(caslfp, "    CALL $%s\n", name);
    fprintf(caslfp, "    ; call %s", name);
    if (param_count > 0) {
        fprintf(caslfp, " (");
        // Add parameter handling
        fprintf(caslfp, ")");
    }
    fprintf(caslfp, ";\n\n");
}

void gen_procedure_param(const char* proc_name, const char* param_name) {
    fprintf(caslfp, "$$%s%%%s  DC 0\n", param_name, proc_name);
    fprintf(caslfp, "; procedure %s ( %s : integer );\n\n", proc_name, param_name);
}

// Call-by-reference parameter handling
void gen_push_address(const char* var) {
    fprintf(caslfp, "\tLAD\tGR1,$%s\n", var);
    fprintf(caslfp, "\tPUSH\t0,GR1\n");
}

void gen_push_expression_address(void) {
    // Stub for pushing an expression address
    char temp_name[32];
    snprintf(temp_name, sizeof(temp_name), "temp%d", temp_var_count++);
    
    // Allocate temporary storage
    fprintf(caslfp, "$%s\tDS\t1\n", temp_name);
    
    // Store expression result
    fprintf(caslfp, "\tST\tGR1,$%s\n", temp_name);
    
    // Push address
    fprintf(caslfp, "\tLAD\tGR1,$%s\n", temp_name);
    fprintf(caslfp, "\tPUSH\t0,GR1\n");
}

void gen_formal_parameter(const char* param_name, const char* proc_name) {
    fprintf(caslfp, "$$%s%%%s\tDC\t0\n", param_name, proc_name);
}

void gen_local_variable(const char* var_name, const char* proc_name) {
    fprintf(caslfp, "$%s%%%s\tDC\t0\n", var_name, proc_name);
}

// Input/Output
void gen_read(const char* var) {
    // Instead of "IN #0001" etc., call READINT and READLINE
    fprintf(caslfp, "\tLAD\tGR1,$%s\n", var);
    fprintf(caslfp, "\tCALL\tREADINT\n");
    fprintf(caslfp, "\tCALL\tREADLINE\n");
}

void gen_write(const char* var, int width) {
    // Load variable and call WRITEINT
    fprintf(caslfp, "\tLD\tGR1,$%s\n", var);
    fprintf(caslfp, "\tLAD\tGR2,=%d\n", width);
    fprintf(caslfp, "\tCALL\tWRITEINT\n");
    // Optionally write a line break
    fprintf(caslfp, "\tCALL\tWRITELINE\n");
}

// Example helper for writing a string literal
void gen_write_string(const char* msg) {
    static int str_counter = 0;
    fprintf(caslfp, "STR%d\tDC\t'%s'\n", str_counter, msg);
    fprintf(caslfp, "\tLAD\tGR1,STR%d\n", str_counter);
    fprintf(caslfp, "\tCALL\tWRITESTR\n");
    fprintf(caslfp, "\tCALL\tWRITELINE\n");
    str_counter++;
}

// Runtime error handlers
void gen_runtime_error_handlers(void) {
    fprintf(caslfp, "; ------------------------\n");
    fprintf(caslfp, "; Utility functions\n");
    fprintf(caslfp, "; ------------------------\n\n");

    // Overflow handler
    fprintf(caslfp, "; Overflow error handling\n");
    fprintf(caslfp, "EOVF\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    LAD GR1, EOVF1\n");
    fprintf(caslfp, "    LD GR2, GR0\n");
    fprintf(caslfp, "    CALL WRITESTR\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    SVC 1\n");
    fprintf(caslfp, "EOVF1  DC '***** Run-Time Error: Overflow *****'\n\n");

    // Zero-Divide handler
    fprintf(caslfp, "; Zero-Divide error handling\n");
    fprintf(caslfp, "E0DIV\n");
    fprintf(caslfp, "    JNZ EOVF\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    LAD GR1, E0DIV1\n");
    fprintf(caslfp, "    LD GR2, GR0\n");
    fprintf(caslfp, "    CALL WRITESTR\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    SVC 2\n");
    fprintf(caslfp, "E0DIV1  DC '***** Run-Time Error: Zero-Divide *****'\n\n");

    // Range-Over handler
    fprintf(caslfp, "; Range-Over error handling\n");
    fprintf(caslfp, "EROV\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    LAD GR1, EROV1\n");
    fprintf(caslfp, "    LD GR2, GR0\n");
    fprintf(caslfp, "    CALL WRITESTR\n");
    fprintf(caslfp, "    CALL WRITELINE\n");
    fprintf(caslfp, "    SVC 3\n");
    fprintf(caslfp, "EROV1  DC '***** Run-Time Error: Range-Over in Array Index *****'\n\n");
    
    fprintf(caslfp, "END\n");
}

void gen_bounds_check(void) {
    fprintf(caslfp, "\tCPA\tGR1,=0\n");   // Check lower bound
    fprintf(caslfp, "\tJMI\tEBOUND\n");    // Jump if negative
    fprintf(caslfp, "\tCPA\tGR1,GR2\n");   // Compare with array size
    fprintf(caslfp, "\tJGE\tEBOUND\n");    // Jump if >= size
}

void gen_div_check(void) {
    fprintf(caslfp, "\tCPA\tGR1,=0\n");
    fprintf(caslfp, "\tJZE\tEDIVZ\n");
}

void gen_overflow_check(void) {
    fprintf(caslfp, "\tJOV\tEOVF\n");  // Jump on overflow
}

// Type conversion implementations
void gen_type_conversion(int from_type, int to_type) {
    switch (from_type) {
        case TINTEGER:
            if (to_type == TBOOLEAN) {
                // Convert to boolean (0 = false, non-zero = true)
                fprintf(caslfp, "\tCPA\tGR1,=0\n");
                fprintf(caslfp, "\tLD\tGR1,=0\n");
                fprintf(caslfp, "\tJZE\t$+2\n");
                fprintf(caslfp, "\tLD\tGR1,=1\n");
            } else if (to_type == TCHAR) {
                // Keep only lowest 7 bits
                fprintf(caslfp, "\tAND\tGR1,=127\n");
            }
            break;
            
        case TBOOLEAN:
            if (to_type == TINTEGER) {
                // Already 0 or 1
                break;
            } else if (to_type == TCHAR) {
                // Convert to character code (0 or 1)
                fprintf(caslfp, "\tAND\tGR1,=1\n");
            }
            break;
            
        case TCHAR:
            if (to_type == TINTEGER) {
                // Character code is already an integer
                break;
            } else if (to_type == TBOOLEAN) {
                // Convert to boolean (0 = false, non-zero = true)
                fprintf(caslfp, "\tCPA\tGR1,=0\n");
                fprintf(caslfp, "\tLD\tGR1,=0\n");
                fprintf(caslfp, "\tJZE\t$+2\n");
                fprintf(caslfp, "\tLD\tGR1,=1\n");
            }
            break;
    }
}

// Temporary variable handling

void gen_temp_var_section(void) {
    fprintf(caslfp, "\t.DATA\n");  // Start data section
}

void gen_temp_var(const char* prefix) {
    fprintf(caslfp, "$%s%d\tDS\t1\n", prefix, temp_var_count++);
}

// Add definitions for READINT, READLINE, WRITESTR, WRITEINT, WRITELINE, ERRPRT, etc.
void gen_lib_subroutines(void) {
    fprintf(caslfp, "; Standard library routines\n");
    fprintf(caslfp, "WRITESTR\tSTART\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");

    fprintf(caslfp, "WRITEINT\tSTART\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");

    fprintf(caslfp, "WRITELINE\tSTART\n");
    fprintf(caslfp, "\tOUT\tDC,='\n'\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");

    fprintf(caslfp, "READINT\tSTART\n");
    fprintf(caslfp, "\tIN\t,GR1\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");

    fprintf(caslfp, "READLINE\tSTART\n");
    fprintf(caslfp, "\tIN\t,GR1\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");

    fprintf(caslfp, "ERRPRT\tSTART\n");
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");
}

void gen_comment(const char* comment) {
    fprintf(caslfp, "    ; %s\n", comment);
}

void gen_pascal_comment(const char* comment) {
    fprintf(caslfp, "; %s\n", comment);
}