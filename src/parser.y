%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int yyparse(void);
int yylex(void);

%}

%union
{
    int number;
    char *string;
}

%error-verbose
%locations

%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO
%token DOUBLE ELSE ENUM EXTERN FLOAT FOR GOTO IF INT LONG
%token REGISTER RETURN SHORT SIGNED SIZEOF STATIC STRUCT
%token SWITCH TYPEDEF UNION UNSIGNED VOID VOLATILE WHILE
%token EQUATE NOT_EQUATE LESS_OR_EQUAL GREATER_OR_EQUAL
%token SHIFT_LEFT SHIFT_RIGHT BOOL_OP_AND BOOL_OP_OR

%token <string> IDENTIFIER
%token <string> CONST_BITS
%token <string> CONST_HEX
%token <number> CONST_INT;

%%

statements: IDENTIFIER
| statements IDENTIFIER
;
