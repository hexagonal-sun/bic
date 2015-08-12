%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"

int yyparse(void);
int yylex(void);
void yyerror(const char *str);

extern tree parse_head;


%}

%union
{
    mpz_t integer;
    char *string;
    tree tree;
}

%error-verbose
%locations

%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO
%token DOUBLE ELSE ENUM EXTERN FLOAT FOR GOTO IF INT LONG
%token REGISTER RETURN SHORT SIGNED SIZEOF STATIC STRUCT
%token SWITCH TYPEDEF UNION UNSIGNED VOID VOLATILE WHILE
%token EQUATE NOT_EQUATE LESS_OR_EQUAL GREATER_OR_EQUAL
%token SHIFT_LEFT SHIFT_RIGHT BOOL_OP_AND BOOL_OP_OR INC
%token DEC

%token <string> IDENTIFIER
%token <string> CONST_BITS
%token <string> CONST_HEX
%token <integer> INTEGER;

%type <tree> statements
%type <tree> statement
%type <tree> primary_expression
%type <tree> postfix_expression
%type <tree> unary_expression
%type <tree> multiplicative_expression
%type <tree> additive_expression
%type <tree> assignment_expression

%%

statements: statement  ';'
{
    parse_head = $1;
}
| statements statement ';'
{
    $1->next = $2;
    $$ = $2;
}

statement: additive_expression

primary_expression
: INTEGER
{
    tree number = tree_make(T_INTEGER);
    mpz_set(number->data.integer, $1);
    $$ = number;
}
| IDENTIFIER
{
    tree identifier = tree_make(T_IDENTIFIER);
    identifier->data.id = get_identifier($1);
    $$ = identifier;
}

postfix_expression
: primary_expression
| postfix_expression INC
{
    tree inc = tree_make(T_P_INC);
    inc->data.exp = $1;
    $$ = inc;
}
| postfix_expression DEC
{
    tree dec = tree_make(T_P_DEC);
    dec->data.exp = $1;
    $$ = dec;
}
;

unary_expression
: postfix_expression
| INC unary_expression
{
    tree inc = tree_make(T_INC);
    inc->data.exp = $2;
    $$ = inc;
}
| DEC unary_expression
{
    tree dec = tree_make(T_DEC);
    dec->data.exp = $2;
    $$ = dec;
}
;

multiplicative_expression
: unary_expression
| multiplicative_expression '*' unary_expression
{
    $$ = tree_build_bin(T_MUL, $1, $3);
}
| multiplicative_expression '/' unary_expression
{
    $$ = tree_build_bin(T_DIV, $1, $3);
}
| multiplicative_expression '%' unary_expression
{
    $$ = tree_build_bin(T_MOD, $1, $3);
}
;

additive_expression
: multiplicative_expression
| additive_expression '+' multiplicative_expression
{
    $$ = tree_build_bin(T_ADD, $1, $3);
}
| additive_expression '-'  multiplicative_expression
{
    $$ = tree_build_bin(T_SUB, $1, $3);
}
;

assignment_expression
: additive_expression
| unary_expression '=' assignment_expression
{
    $$ = tree_build_bin(T_ASSIGN, $1, $3);
}
;

