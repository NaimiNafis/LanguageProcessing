#ifndef TOKEN_H
#define TOKEN_H

/* Token definitions */
#define TNAME       1   /* Name: Alphabet { Alphabet | Digit } */
#define TPROGRAM    2   /* program : Keyword */
#define TVAR        3   /* var : Keyword */
#define TARRAY      4   /* array : Keyword */
#define TOF         5   /* of : Keyword */
#define TBEGIN      6   /* begin : Keyword */
#define TEND        7   /* end : Keyword */
#define TIF         8   /* if : Keyword */
#define TTHEN       9   /* then : Keyword */
#define TELSE       10  /* else : Keyword */
#define TPROCEDURE  11  /* procedure : Keyword */
#define TRETURN     12  /* return : Keyword */
#define TCALL       13  /* call : Keyword */
#define TWHILE      14  /* while : Keyword */
#define TDO         15  /* do : Keyword */
#define TNOT        16  /* not : Keyword */
#define TOR         17  /* or : Keyword */
#define TDIV        18  /* div : Keyword */
#define TAND        19  /* and : Keyword */
#define TCHAR       20  /* char : Keyword */
#define TINTEGER    21  /* integer : Keyword */
#define TBOOLEAN    22  /* boolean : Keyword */
#define TREADLN     23  /* readln : Keyword */
#define TWRITELN    24  /* writeln : Keyword */
#define TTRUE       25  /* true : Keyword */
#define TFALSE      26  /* false : Keyword */
#define TNUMBER     27  /* unsigned integer */
#define TSTRING     28  /* String */
#define TPLUS       29  /* + : symbol */
#define TMINUS      30  /* - : symbol */
#define TSTAR       31  /* * : symbol */
#define TEQUAL      32  /* = : symbol */
#define TNOTEQ      33  /* <> : symbol */
#define TLE         34  /* < : symbol */
#define TLEEQ       35  /* <= : symbol */
#define TGR         36  /* > : symbol */
#define TGREQ       37  /* >= : symbol */
#define TLPAREN     38  /* ( : symbol */
#define TRPAREN     39  /* ) : symbol */
#define TLSQPAREN   40  /* [ : symbol */
#define TRSQPAREN   41  /* ] : symbol */
#define TASSIGN     42  /* := : symbol */
#define TDOT        43  /* . : symbol */
#define TCOMMA      44  /* , : symbol */
#define TCOLON      45  /* : : symbol */
#define TSEMI       46  /* ; : symbol */
#define TREAD       47  /* read : Keyword */
#define TWRITE      48  /* write : Keyword */
#define TBREAK      49  /* break : Keyword */

#define NUMOFTOKEN  49
#define KEYWORDSIZE 28  /* Number of keywords */

// Define the keyword struct
typedef struct {
    char* keyword;
    int keytoken;
} keyword;

typedef struct {
    int kind;           // Uses existing token defines (TNAME, etc)
    union {
        char *str;      // For TNAME and string literals
        int num;        // For TNUMBER
    } value;
} Token;

// Declare the keyword array
extern keyword key[KEYWORDSIZE];
extern char* tokenstr[NUMOFTOKEN + 1];

// Declarations for parser
extern Token current_token;
extern int token;  // Holds current token kind

#endif