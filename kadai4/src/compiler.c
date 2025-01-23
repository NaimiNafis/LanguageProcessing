#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "token.h"
#include "parser.h"
#include "debug.h"
#include "scan.h"

// Error handling
int error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
    return -1;
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

// File pointer for CASL output and globals
FILE* caslfp = NULL;
static int temp_var_count = 0;  // Move this to top with other globals

// Static forward declarations
static int p_exp(void);
static int p_st(void);
static int p_factor(void);

// Label counter management
int get_label_num(void) {
    static int labelcounter = 1;
    return labelcounter++;
}

void gen_label(int labelnum) {
    fprintf(caslfp, "L%04d\n", labelnum);
}

void gen_code(char *opc, const char *opr) {
    fprintf(caslfp, "\t%s\t%s\n", opc, opr);
}

void gen_code_label(char *opc, char *opr, int label) {
    fprintf(caslfp, "\t%s\t%sL%04d\n", opc, opr, label);
}

// Add variable allocation
void gen_variable_allocation(const char* name, int size) {
    fprintf(caslfp, "; Variable allocation %s, size=%d\n", name, size);
    fprintf(caslfp, "$%s\tDS\t%d\n", name, size);
}

// Add array allocation
void gen_array_allocation(const char* name, int size) {
    fprintf(caslfp, "; Array allocation %s, size=%d\n", name, size);
    fprintf(caslfp, "$%s\tDS\t%d\n", name, size);
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
    fprintf(caslfp, "; Data section start\n");
    fprintf(caslfp, "\tSTART\n");
    fprintf(caslfp, "LIBBUF\tDS\t256\n");
}

void gen_data_section_end(void) {
    fprintf(caslfp, "; Data section end\n");
    fprintf(caslfp, "\tEND\n");
}

void gen_program_start(const char* name) {
    fprintf(caslfp, "; Program start: %s\n", name);
    fprintf(caslfp, "%s\tSTART\n", name);
    gen_save_registers();
}

void gen_program_end(void) {
    fprintf(caslfp, "; Program end\n");
    gen_restore_registers();
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");
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
    fprintf(caslfp, "$%s\tPROC\n", name);
    fprintf(caslfp, "\tPOP\tGR2\n");      // Get return address
    fprintf(caslfp, "\tPOP\tGR1\n");      // Get parameter address
    fprintf(caslfp, "\tST\tGR1,$$param%%%s\n", name);  // Store parameter address
    fprintf(caslfp, "\tPUSH\t0,GR2\n");   // Restore return address
    gen_save_registers();
}

void gen_procedure_exit(void) {
    gen_restore_registers();
    fprintf(caslfp, "\tRET\n");
    fprintf(caslfp, "\tEND\n");
}

void gen_procedure_call(const char* name) {
    gen_save_registers();           // Save current state
    fprintf(caslfp, "\tCALL\t$%s\n", name);
    gen_restore_registers();        // Restore state after call
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
    fprintf(caslfp, "\tIN\t#0001\n");
    fprintf(caslfp, "\tST\tGR1,$%s\n", var);
}

void gen_write(const char* var, int width) {
    fprintf(caslfp, "\tLD\tGR1,$%s\n", var);
    fprintf(caslfp, "\tOUT\t#0002,%d\n", width);
}

void gen_writeln(void) {
    fprintf(caslfp, "\tOUT\t#0003\n");
}

/* Syntax: if <expression> then <statement> [else <statement>] */
int p_ifst(void) {
    int label1, label2;
    extern int token;  // Assuming token is defined elsewhere

    if (token != TIF) return(error("Keyword 'if' is not found"));
    token = scan();

    if (p_exp() == ERROR) return ERROR;

    label1 = get_label_num();
    gen_code("CPA", "GR1,GR0");
    gen_code_label("JZE", "", label1);

    if (token != TTHEN) return(error("Keyword 'then' is not found"));
    token = scan();

    if (p_st() == ERROR) return ERROR;

    if (token == TELSE) {
        label2 = get_label_num();
        gen_code_label("JUMP", "", label2);
        gen_label(label1);

        token = scan();
        if (p_st() == ERROR) return ERROR;

        gen_label(label2);
    } else {
        gen_label(label1);
    }

    return NORMAL;
}

/* Syntax: <term> ::= <factor> { "*" | "div" | "and" <factor> } */
int p_term(void) {
    int opr;
    extern int token;  // Assuming token is defined elsewhere

    if (p_factor() == ERROR) return ERROR;
    gen_code("PUSH", "0,GR1");

    while (token == TSTAR || token == TDIV || token == TAND) {
        opr = token;
        token = scan();

        if (p_factor() == ERROR) return ERROR;
        gen_code("POP", "GR2");

        if (opr == TSTAR) {
            gen_code("MULA", "GR1,GR2");
        } else if (opr == TDIV) {
            gen_code("DIVA", "GR2,GR1");
            gen_code("LD", "GR1,GR2");
        } else if (opr == TAND) {
            gen_code("AND", "GR1,GR2");
        }
    }

    return NORMAL;
}

// Placeholder functions that need to be implemented
static int p_exp(void) {
    // Implement expression parsing
    return NORMAL;
}

static int p_st(void) {
    // Implement statement parsing
    return NORMAL;
}

static int p_factor(void) {
    // Implement factor parsing
    return NORMAL;
}

// Runtime error handlers
void gen_runtime_error_handlers(void) {
    // Stub for runtime error handlers
    // Division by zero handler
    fprintf(caslfp, "EDIVZ\tDC\t'Division by zero error'\n");
    fprintf(caslfp, "\tCALL\tERRPRT\n");
    fprintf(caslfp, "\tSVC\t1\n");  // Program termination
    
    // Array bounds handler
    fprintf(caslfp, "EBOUND\tDC\t'Array index out of bounds error'\n");
    fprintf(caslfp, "\tCALL\tERRPRT\n");
    fprintf(caslfp, "\tSVC\t1\n");
    
    // Overflow handler
    fprintf(caslfp, "EOVF\tDC\t'Arithmetic overflow error'\n");
    fprintf(caslfp, "\tCALL\tERRPRT\n");
    fprintf(caslfp, "\tSVC\t1\n");
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