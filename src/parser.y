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
%type <tree> decl
%type <tree> declarator
%type <tree> initialiser
%type <tree> declarator_list
%type <tree> direct_declarator_list
%type <tree> struct_specifier
%type <tree> struct_decl_list
%type <tree> struct_decl
%type <tree> type_specifier
%type <tree> declaration

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

statement
: assignment_expression
| declaration
;

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

decl
: IDENTIFIER
{
    tree identifier = tree_make(T_IDENTIFIER);
    identifier->data.id = get_identifier($1);
    $$ = identifier;
}
;

initialiser
: additive_expression
;

declarator
: decl
| decl '=' initialiser
{
    $$ = tree_build_bin(T_ASSIGN, $1, $3);
}
;

declarator_list
: declarator
| declarator ',' declarator_list
{
    $1->next = $3;
}
;

direct_declarator_list
: decl
| decl ',' direct_declarator_list
{
    $1 ->next = $3;
}
;

type_specifier
: CHAR
{
    $$ = tree_make(D_T_CHAR);
}
| SIGNED CHAR
{
    $$ = tree_make(D_T_CHAR);
}
| UNSIGNED CHAR
{
    $$ = tree_make(D_T_UCHAR);
}
| SHORT
{
    $$ = tree_make(D_T_SHORT);
}
| SHORT INT
{
    $$ = tree_make(D_T_SHORT);
}
| SIGNED SHORT
{
    $$ = tree_make(D_T_SHORT);
}
| SIGNED SHORT INT
{
    $$ = tree_make(D_T_SHORT);
}
| UNSIGNED SHORT
{
    $$ = tree_make(D_T_USHORT);
}
| UNSIGNED SHORT INT
{
    $$ = tree_make(D_T_USHORT);
}
| INT
{
    $$ = tree_make(D_T_INT);
}
| SIGNED INT
{
    $$ = tree_make(D_T_INT);
}
| UNSIGNED INT
{
    $$ = tree_make(D_T_UINT);
}
| LONG
{
    $$ = tree_make(D_T_LONG);
}
| LONG INT
{
    $$ = tree_make(D_T_LONG);
}
| SIGNED LONG
{
    $$ = tree_make(D_T_LONG);
}
| SIGNED LONG INT
{
    $$ = tree_make(D_T_LONG);
}
| UNSIGNED LONG
{
    $$ = tree_make(D_T_ULONG);
}
| UNSIGNED LONG INT
{
    $$ = tree_make(D_T_ULONG);
}
| LONG LONG
{
    $$ = tree_make(D_T_LONGLONG);
}
| LONG LONG INT
{
    $$ = tree_make(D_T_LONGLONG);
}
| SIGNED LONG LONG
{
    $$ = tree_make(D_T_LONGLONG);
}
| SIGNED LONG LONG INT
{
    $$ = tree_make(D_T_LONGLONG);
}
| UNSIGNED LONG LONG
{
    $$ = tree_make(D_T_ULONGLONG);
}
| UNSIGNED LONG LONG INT
{
    $$ = tree_make(D_T_ULONGLONG);
}
;

struct_specifier
: STRUCT IDENTIFIER '{' struct_decl_list '}'
{
    tree decl = tree_make(T_DECL_STRUCT);
    decl->data.structure.id = get_identifier($2);
    decl->data.structure.decls = $4;
    $$ = decl;
}
;

struct_decl_list
: struct_decl
| struct_decl_list struct_decl
{
    $1->next = $2;
}
;

struct_decl
: type_specifier direct_declarator_list ';'
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
    $$ = decl;
}

declaration
: type_specifier declarator_list
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
    $$ = decl;
}
| struct_specifier
;
