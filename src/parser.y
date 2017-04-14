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
    mpf_t ffloat;
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
%token DEC ELLIPSIS

%token <string> IDENTIFIER
%token <string> CONST_BITS
%token <string> CONST_HEX
%token <string> CONST_STRING
%token <integer> INTEGER;
%token <ffloat> FLOAT_CST;

%type <tree> translation_unit
%type <tree> toplevel_declarations
%type <tree> compound_statement
%type <tree> function_definition
%type <tree> argument_specifier
%type <tree> direct_argument_list
%type <tree> argument_list
%type <tree> argument_decl
%type <tree> statement
%type <tree> statement_list
%type <tree> expression_statement
%type <tree> iteration_statement
%type <tree> primary_expression
%type <tree> postfix_expression
%type <tree> argument_expression_list
%type <tree> unary_expression
%type <tree> multiplicative_expression
%type <tree> additive_expression
%type <tree> relational_expression
%type <tree> assignment_expression
%type <tree> decl
%type <tree> decl_possible_pointer
%type <tree> declarator
%type <tree> initialiser
%type <tree> declarator_list
%type <tree> direct_declarator_list
%type <tree> struct_specifier
%type <tree> struct_decl_list
%type <tree> struct_decl
%type <tree> storage_class_specifier
%type <tree> direct_type_specifier
%type <tree> type_specifier
%type <tree> declaration
%type <tree> declaration_list

%%

translation_unit
: toplevel_declarations
{
    parse_head = tree_chain_head($1);
    $$ = parse_head;
}
| translation_unit toplevel_declarations
{
    tree_chain($2, $1);
}
;

toplevel_declarations
: declaration ';'
| function_definition
;

function_definition
: type_specifier IDENTIFIER argument_specifier compound_statement
{
    tree function_def = tree_make(T_FN_DEF);
    function_def->data.function.id = get_identifier($2);
    function_def->data.function.return_type = $1;
    function_def->data.function.arguments = $3;
    function_def->data.function.stmts = $4;
    $$ = function_def;
}
;

argument_specifier
: '(' ')'
{
    $$ = NULL;
}
| '(' VOID ')'
{
    $$ = NULL;
}
| '(' argument_list ')'
{
    $$ = $2;
}
;

direct_argument_list
: argument_decl
{
    $$ = tree_chain_head($1);
}
| direct_argument_list ',' argument_decl
{
    tree_chain($3, $1);
};

argument_list
: direct_argument_list
| direct_argument_list ',' ELLIPSIS
{
    tree variadic = tree_make(T_VARIADIC);
    tree_chain(variadic, $1);
}

argument_decl
: type_specifier decl_possible_pointer
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
    $$ = decl;
}
;

statement
: expression_statement
| compound_statement
| iteration_statement
;

statement_list: statement
{
    $$ = tree_chain_head($1);
}
| statement_list statement
{
    tree_chain($2, $1);
}
;

compound_statement
: '{' statement_list '}'
{
    $$ = $2;
}
| '{' declaration_list statement_list '}'
{
    tree_splice_chains($2, $3);
    $$ = $2;
}
;

expression_statement
: assignment_expression ';'
;

iteration_statement
: FOR '(' expression_statement expression_statement assignment_expression ')' statement
{
    tree for_loop = tree_make(T_LOOP_FOR);
    for_loop->data.floop.initialization = $3;
    for_loop->data.floop.condition = $4;
    for_loop->data.floop.afterthrought = $5;
    for_loop->data.floop.stmts = $7;
    $$ = for_loop;
}
;

primary_expression
: INTEGER
{
    tree number = tree_make(T_INTEGER);
    mpz_init_set(number->data.integer, $1);
    mpz_clear($1);
    $$ = number;
}
| FLOAT_CST
{
    tree ffloat = tree_make(T_FLOAT);
    mpf_init_set(ffloat->data.ffloat, $1);
    mpf_clear($1);
    $$ = ffloat;
}
| IDENTIFIER
{
    $$ = get_identifier($1);
}
| CONST_STRING
{
    tree str = tree_make(T_STRING);
    str->data.string = $1;
    $$ = str;
}
;

postfix_expression
: primary_expression
| postfix_expression '(' ')'
{
    tree fncall = tree_make(T_FN_CALL);
    fncall->data.fncall.identifier = $1;
    fncall->data.fncall.arguments = NULL;
    $$ = fncall;
}
| postfix_expression '(' argument_expression_list ')'
{
    tree fncall = tree_make(T_FN_CALL);
    fncall->data.fncall.identifier = $1;
    fncall->data.fncall.arguments = $3;
    $$ = fncall;
}
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
| postfix_expression '.' IDENTIFIER
{
    tree access = tree_make(T_ACCESS);
    access->data.bin.left = $1;
    access->data.bin.right = get_identifier($3);
    $$ = access;
}
;

argument_expression_list
: assignment_expression
{
    $$ = tree_chain_head($1);
}
| argument_expression_list ',' assignment_expression
{
    tree_chain($3, $1);
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
| '&' unary_expression
{
    tree addr = tree_make(T_ADDR);
    addr->data.exp = $2;
    $$ = addr;
}
| '*' unary_expression
{
    tree deref = tree_make(T_DEREF);
    deref->data.exp = $2;
    $$ = deref;
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

relational_expression
: additive_expression
| relational_expression '<' additive_expression
{
    $$ = tree_build_bin(T_LT, $1, $3);
}
| relational_expression '>' additive_expression
{
    $$ = tree_build_bin(T_GT, $1, $3);
}
| relational_expression LESS_OR_EQUAL additive_expression
{
    $$ = tree_build_bin(T_LTEQ, $1, $3);
}
| relational_expression GREATER_OR_EQUAL additive_expression
{
    $$ = tree_build_bin(T_GTEQ, $1, $3);
}
;

assignment_expression
: relational_expression
| unary_expression '=' assignment_expression
{
    $$ = tree_build_bin(T_ASSIGN, $1, $3);
}
;

decl
: IDENTIFIER
{
    $$ = get_identifier($1);
}
| IDENTIFIER argument_specifier
{
    tree fn_decl = tree_make(T_DECL_FN);
    fn_decl->data.function.id = get_identifier($1);
    fn_decl->data.function.arguments = $2;
    $$ = fn_decl;
}
| decl '[' additive_expression ']'
{
    tree array = tree_make(T_ARRAY);
    array->data.array.decl = $1;
    array->data.array.exp = $3;
    $$ = array;
}
;

decl_possible_pointer
: decl
| '*' decl
{
    tree ptr = tree_make(T_POINTER);
    ptr->data.exp = $2;
    $$ = ptr;
}
;

initialiser
: additive_expression
;

declarator
: decl_possible_pointer
| decl_possible_pointer '=' initialiser
{
    $$ = tree_build_bin(T_ASSIGN, $1, $3);
}
;

declarator_list
: declarator
{
    $$ = tree_chain_head($1);
}
| declarator_list ',' declarator
{
    tree_chain($3, $1);
}
;

direct_declarator_list
: decl_possible_pointer
{
    $$ = tree_chain_head($1);
}
| decl_possible_pointer ',' direct_declarator_list
{
    tree_chain($3, $1);
}
;

storage_class_specifier
: TYPEDEF
{
    $$ = tree_make(T_TYPEDEF);
}

direct_type_specifier
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
| FLOAT
{
    $$ = tree_make(D_T_FLOAT);
}
| VOID
{
    $$ = tree_make(D_T_VOID);
}
| IDENTIFIER
{
    $$ = get_identifier($1);
}
| struct_specifier
;

type_specifier
: direct_type_specifier
| storage_class_specifier direct_type_specifier
{
    $1->data.exp = $2;
    $$ = $1;
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
| STRUCT '{' struct_decl_list '}'
{
    tree decl = tree_make(T_DECL_STRUCT);
    decl->data.structure.id = NULL;
    decl->data.structure.decls = $3;
    $$ = decl;
}
| STRUCT IDENTIFIER
{
    $$ = get_identifier($2);
}
;

struct_decl_list
: struct_decl
{
    $$ = tree_chain_head($1);
}
| struct_decl_list struct_decl
{
    tree_chain($2, $1);
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
| type_specifier
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = NULL;
    $$ = decl;
}
;

declaration_list
: declaration ';'
{
    $$ = tree_chain_head($1);
}
| declaration_list declaration ';'
{
    tree_chain($2, $1);
}
;
