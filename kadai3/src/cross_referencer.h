#ifndef CROSS_REFERENCER_H
#define CROSS_REFERENCER_H

#include "token.h"
#include "scan.h"  // Add this include

// Forward declaration of ParamType
struct ParamType;

typedef struct TYPE {
    int ttype;  // TPINT, TPCHAR, TPBOOL, etc.
    int arraysize;  // Size of array if applicable
    struct TYPE *etp;  // Element type for array types
    struct ParamType *paratp;  // Parameter types for procedure types
} Type;

typedef struct LINE {
    int reflinenum;
    struct LINE *nextlinep;
} Line;

typedef struct ID {
    char *name;
    char *procname;  // Procedure name if defined within a procedure
    Type *itp;  // Type information
    int ispara;  // 1 if it's a parameter, 0 if it's a variable
    int deflinenum;  // Declaration line number
    Line *irefp;  // List of reference line numbers
    struct ID *nextp;
} ID;

void init_cross_referencer(void);
void add_symbol(char *name, int type, int linenum, int is_definition);
void add_reference(char *name, int linenum);
extern void print_cross_reference_table(void);

// Add new function declarations
void enter_procedure(const char *name);
void exit_procedure(void);
void add_procedure_parameter(int type);

// Add new function declaration
const char* type_to_string(int type);

// Add new function declarations for array handling
void set_array_info(int size, int base_type);
Type* create_array_type(int size, int base_type);

#endif // CROSS_REFERENCER_H