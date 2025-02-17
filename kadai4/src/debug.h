#ifndef DEBUG_H
#define DEBUG_H

// Debug flags for different modules
extern int debug_scanner;
extern int debug_parser;
extern int debug_cross_referencer;
extern int debug_compiler;
extern int debug_pretty;

// Debug modes can be enabled via command line arguments:
// --debug-scan
// --debug-parse
// --debug-xref
// --debug-compile
// --debug-pretty
// --debug-all

#endif
