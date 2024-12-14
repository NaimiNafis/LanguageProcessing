#ifndef CROSS_REFERENCER_H
#define CROSS_REFERENCER_H

#include "token.h"
#include "scan.h"

// Forward declaration for type system
struct ParamType;

// Type system structure for handling variable and procedure types
typedef struct TYPE {
    int ttype;      
    int arraysize;   
    struct TYPE *etp;    
    struct ParamType *paratp;  
} Type;

// Reference line tracking
typedef struct LINE {
    int reflinenum;
    struct LINE *nextlinep;
} Line;

// Symbol table entry structure
typedef struct ID {
    char *name;        
    char *procname;    
    Type *itp;        
    int ispara;       
    int deflinenum;   
    Line *irefp;      
    struct ID *nextp;
} ID;

// Core functionality
void init_cross_referencer(void);
void add_symbol(char *name, int type, int linenum, int is_definition);
void add_reference(char *name, int linenum);
void print_cross_reference_table(void);

// Procedure handling
const char* get_current_procedure(void);
void enter_procedure(const char *name);
void exit_procedure(void);
void add_procedure_parameter(int type);

// Type system utilities
const char* type_to_string(int type);
void set_array_info(int size, int base_type);
Type* create_array_type(int size, int base_type);

// Error management
void set_error_state(void);
int is_error_state(void);

#endif