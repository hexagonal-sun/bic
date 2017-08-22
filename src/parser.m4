divert(-1)
include(parser-lex-funcs.m4)
divert(0)dnl
%{
`#'include <gc.h>
`#'include <stdio.h>
`#'include <string.h>
`#'include <stdlib.h>
`#'include "tree.h"
`#'include "typename.h"
`#'include "TARGET()parser.h"

int TARGET()parse(void);
int TARGET()lex(void);
void TARGET()error(const char *str);

`#'define YYMALLOC GC_MALLOC
`#'define YYFREE GC_FREE

extern tree TARGET()_parse_head;

static void set_locus(tree t, YYLTYPE locus)
{
    t->locus.line_no = locus.first_line;
    t->locus.column_no = locus.first_column;
}

static char * concat_string(const char *s1, const char *s2)
{
    char *ret = GC_MALLOC(strlen(s1) + strlen(s2) + 1);
    ret[0] = '\0';
    strcat(ret, s1);
    strcat(ret, s2);
    return ret;
}
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

%define api.prefix {TARGET}

%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO
%token DOUBLE ELSE ENUM EXTERN FLOAT FOR GOTO IF INT LONG
%token REGISTER RETURN SHORT SIGNED SIZEOF STATIC STRUCT
%token SWITCH TYPEDEF UNION UNSIGNED VOID VOLATILE WHILE
%token EQUATE NOT_EQUATE LESS_OR_EQUAL GREATER_OR_EQUAL
%token SHIFT_LEFT SHIFT_RIGHT BOOL_OP_AND BOOL_OP_OR INC
%token DEC ELLIPSIS PTR_ACCESS

%token <string> IDENTIFIER
REPL_ONLY
%token <string> C_PRE_INC
ALL_TARGETS
%token <string> TYPE_NAME
%token <string> CONST_BITS
%token <string> CONST_HEX
%token <string> CONST_STRING
%token <integer> INTEGER;
%token <ffloat> FLOAT_CST;

CFILE_ONLY
%type <tree> translation_unit
%type <tree> toplevel_declarations
%type <tree> compound_statement
%type <tree> function_definition
%type <tree> jump_statement
%type <tree> declaration_list
REPL_ONLY
%type <tree> declaration_statement
ALL_TARGETS
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
%type <tree> logical_expression
%type <tree> assignment_expression
%type <tree> decl
%type <tree> decl_possible_pointer
%type <tree> pointer
%type <tree> declarator
%type <tree> initialiser
%type <tree> declarator_list
%type <tree> direct_declarator_list
%type <tree> struct_specifier
%type <tree> union_specifier
%type <tree> enum_specifier
%type <tree> enumerator_list
%type <tree> func_ptr_decl
%type <tree> enumerator
%type <tree> compound_decl_list
%type <tree> compound_decl
%type <tree> storage_class_specifier
%type <tree> direct_type_specifier
%type <tree> sizeof_specifier
%type <tree> type_specifier
%type <tree> declaration

%%

CFILE_ONLY
   translation_unit
   : toplevel_declarations
REPL_ONLY
   statement_list: statement
ALL_TARGETS
{
    TARGET()_parse_head = tree_chain_head($1);
    $$ = TARGET()_parse_head;
}
CFILE_ONLY
   | translation_unit toplevel_declarations
REPL_ONLY
   | statement_list statement
ALL_TARGETS
{
    tree_chain($2, $1);
}
;

CFILE_ONLY
    toplevel_declarations
    : declaration ';'
    | function_definition
    ;

    function_definition
    : type_specifier IDENTIFIER argument_specifier compound_statement
    {
        tree function_def = tree_make(T_FN_DEF);
        tFNDEF_NAME(function_def) = get_identifier($2);
        tFNDEF_RET_TYPE(function_def) = $1;
        tFNDEF_ARGS(function_def) = $3;
        tFNDEF_STMTS(function_def) = $4;
        set_locus(function_def, @1);
        set_locus(tFNDEF_NAME(function_def), @2);
        $$ = function_def;
    }
    ;
ALL_TARGETS

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
    set_locus(variadic, @3);
    tree_chain(variadic, $1);
}

argument_decl
: type_specifier decl_possible_pointer
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = $1;
    tDECL_DECLS(decl) = $2;
    set_locus(decl, @1);
    $$ = decl;
}
| type_specifier pointer
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = make_pointer_type($2, $1);
    set_locus(decl, @1);
    $$ = decl;
}
| type_specifier
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = $1;
    set_locus(decl, @1);
    $$ = decl;
}
;

statement
: expression_statement
| iteration_statement
CFILE_ONLY
    | compound_statement
    | jump_statement
REPL_ONLY
    | declaration_statement
    | C_PRE_INC
    {
        tree ret = tree_make(CPP_INCLUDE);
        tCPP_INCLUDE_STR(ret) = $1;
        $$ = ret;
    }
ALL_TARGETS
;

CFILE_ONLY
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
REPL_ONLY
    declaration_statement
    : declaration ';'
    ;
ALL_TARGETS


expression_statement
: assignment_expression ';'
;

iteration_statement
: FOR '(' expression_statement expression_statement assignment_expression ')' statement
{
    tree for_loop = tree_make(T_LOOP_FOR);
    tFLOOP_INIT(for_loop) = $3;
    tFLOOP_COND(for_loop) = $4;
    tFLOOP_AFTER(for_loop) = $5;
    tFLOOP_STMTS(for_loop) = $7;
    set_locus(for_loop, @1);
    $$ = for_loop;
}
;

CFILE_ONLY
    jump_statement
    : RETURN ';'
    {
        $$ = tree_make(T_RETURN);
    }
    | RETURN expression_statement
    {
        tree ret = tree_make(T_RETURN);
        tRET_EXP(ret) = $2;
        $$ = ret;
    }
    ;
ALL_TARGETS

primary_expression
: INTEGER
{
    tree number = tree_make(T_INTEGER);
    mpz_init_set(tINT(number), $1);
    mpz_clear($1);
    set_locus(number, @1);
    $$ = number;
}
| FLOAT_CST
{
    tree ffloat = tree_make(T_FLOAT);
    mpf_init_set(tFLOAT(ffloat), $1);
    mpf_clear($1);
    set_locus(ffloat, @1);
    $$ = ffloat;
}
| IDENTIFIER
{
    tree identifier = get_identifier($1);
    set_locus(identifier, @1);
    $$ = identifier;
}
| CONST_STRING
{
    tree str = tree_make(T_STRING);
    tSTRING(str) = $1;
    set_locus(str, @1);
    $$ = str;
}
| '(' assignment_expression ')'
{
    $$ = $2;
}
;

postfix_expression
: primary_expression
| postfix_expression '(' ')'
{
    tree fncall = tree_make(T_FN_CALL);
    tFNCALL_ID(fncall) = $1;
    tFNCALL_ARGS(fncall) = NULL;
    set_locus(fncall, @1);
    $$ = fncall;
}
| postfix_expression '(' argument_expression_list ')'
{
    tree fncall = tree_make(T_FN_CALL);
    tFNCALL_ID(fncall) = $1;
    tFNCALL_ARGS(fncall) = $3;
    set_locus(fncall, @1);
    $$ = fncall;
}
| postfix_expression '[' assignment_expression ']'
{
    tree arr_access = tree_make(T_ARRAY_ACCESS);
    tARR_ACCESS_OBJ(arr_access) = $1;
    tARR_ACCESS_IDX(arr_access) = $3;
    set_locus(arr_access, @1);
    $$ = arr_access;
}
| postfix_expression INC
{
    tree inc = tree_make(T_P_INC);
    tPINC_EXP(inc) = $1;
    set_locus(inc, @2);
    $$ = inc;
}
| postfix_expression DEC
{
    tree dec = tree_make(T_P_DEC);
    tPDEC_EXP(dec) = $1;
    set_locus(dec, @2);
    $$ = dec;
}
| postfix_expression '.' IDENTIFIER
{
    tree access = tree_make(T_COMP_ACCESS);
    tCOMP_ACCESS_OBJ(access) = $1;
    tCOMP_ACCESS_MEMBER(access) = get_identifier($3);
    set_locus(access, @2);
    set_locus(tCOMP_ACCESS_MEMBER(access), @3);
    $$ = access;
}
| postfix_expression PTR_ACCESS IDENTIFIER
{
    tree deref = tree_make(T_DEREF);
    tree access = tree_make(T_COMP_ACCESS);

    tDEREF_EXP(deref) = $1;
    tCOMP_ACCESS_OBJ(access) = deref;
    tCOMP_ACCESS_MEMBER(access) = get_identifier($3);

    set_locus(deref, @2);
    set_locus(access, @2);

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
    tINC_EXP(inc) = $2;
    set_locus(inc, @1);
    $$ = inc;
}
| DEC unary_expression
{
    tree dec = tree_make(T_DEC);
    tDEC_EXP(dec) = $2;
    set_locus(dec, @1);
    $$ = dec;
}
| '&' unary_expression
{
    tree addr = tree_make(T_ADDR);
    tADDR_EXP(addr) = $2;
    set_locus(addr, @1);
    $$ = addr;
}
| '*' unary_expression
{
    tree deref = tree_make(T_DEREF);
    tDEREF_EXP(deref) = $2;
    set_locus(deref, @1);
    $$ = deref;
}
| SIZEOF '(' sizeof_specifier ')'
{
    tree szof = tree_make(T_SIZEOF);
    tSZOF_EXP(szof) = $3;
    set_locus(szof, @1);
    $$ = szof;
}
;

multiplicative_expression
: unary_expression
| multiplicative_expression '*' unary_expression
{
    tree mul = tree_build_bin(T_MUL, $1, $3);
    set_locus(mul, @2);
    $$ = mul;
}
| multiplicative_expression '/' unary_expression
{
    tree div = tree_build_bin(T_DIV, $1, $3);
    set_locus(div, @2);
    $$ = div;
}
| multiplicative_expression '%' unary_expression
{
    tree mod = tree_build_bin(T_MOD, $1, $3);
    set_locus(mod, @2);
    $$ = mod;
}
;

additive_expression
: multiplicative_expression
| additive_expression '+' multiplicative_expression
{
    tree add = tree_build_bin(T_ADD, $1, $3);
    set_locus(add, @2);
    $$ = add;
}
| additive_expression '-'  multiplicative_expression
{
    tree sub = tree_build_bin(T_SUB, $1, $3);
    set_locus(sub, @2);
    $$ = sub;
}
;

relational_expression
: additive_expression
| relational_expression '<' additive_expression
{
    tree lt = tree_build_bin(T_LT, $1, $3);
    set_locus(lt, @2);
    $$ = lt;
}
| relational_expression '>' additive_expression
{
    tree gt = tree_build_bin(T_GT, $1, $3);
    set_locus(gt, @2);
    $$ = gt;
}
| relational_expression LESS_OR_EQUAL additive_expression
{
    tree ltoreq = tree_build_bin(T_LTEQ, $1, $3);
    set_locus(ltoreq, @2);
    $$ = ltoreq;
}
| relational_expression GREATER_OR_EQUAL additive_expression
{
    tree gtoreq = tree_build_bin(T_GTEQ, $1, $3);
    set_locus(gtoreq, @2);
    $$ = gtoreq;
}
;

logical_expression
: relational_expression
| logical_expression BOOL_OP_OR relational_expression
{
    tree logicor = tree_build_bin(T_L_OR, $1, $3);
    set_locus(logicor, @2);
    $$ = logicor;
}
| logical_expression BOOL_OP_AND relational_expression
{
    tree logicand = tree_build_bin(T_L_AND, $1, $3);
    set_locus(logicand, @2);
    $$ = logicand;
}
;

assignment_expression
: logical_expression
| unary_expression '=' logical_expression
{
    tree assign = tree_build_bin(T_ASSIGN, $1, $3);
    set_locus(assign, @2);
    $$ = assign;
}
;

decl
: IDENTIFIER
{
    tree id = get_identifier($1);
    set_locus(id, @1);
    $$ = id;
}
| IDENTIFIER argument_specifier
{
    tree fn_decl = tree_make(T_DECL_FN);
    tFNDECL_NAME(fn_decl) = get_identifier($1);
    tFNDECL_ARGS(fn_decl) = $2;
    set_locus(fn_decl, @1);
    set_locus(tFNDECL_NAME(fn_decl), @1);
    $$ = fn_decl;
}
| decl '[' additive_expression ']'
{
    tree array = tree_make(T_ARRAY);
    tARRAY_ID(array) = $1;
    tARRAY_SZ(array) = $3;
    set_locus(array, @2);
    $$ = array;
}
| decl '[' ']'
{
    tree ptr = tree_make(T_POINTER);
    tPTR_EXP(ptr) = $1;
    set_locus(ptr, @2);
    $$ = ptr;
}
;

pointer
: '*'
{
    tree ptr = tree_make(T_POINTER);
    set_locus(ptr, @1);
    $$ = ptr;
}
| '*' pointer
{
    tree ptr = tree_make(T_POINTER);
    tPTR_EXP(ptr) = $2;
    set_locus(ptr, @1);
    $$ = ptr;
}
;

decl_possible_pointer
: decl
| pointer decl
{
    $$ = make_pointer_type($1, $2);
}
;

initialiser
: additive_expression
;

declarator
: decl_possible_pointer
| decl_possible_pointer '=' initialiser
{
    tree assign = tree_build_bin(T_ASSIGN, $1, $3);
    set_locus(assign, @2);
    $$ = assign;
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
| direct_declarator_list ',' decl_possible_pointer
{
    tree_chain($3, $1);
}
;

storage_class_specifier
: TYPEDEF
{
    tree ttypedef = tree_make(T_TYPEDEF);
    set_locus(ttypedef, @1);
    $$ = ttypedef;
}
| EXTERN
{
    tree eextern = tree_make(T_EXTERN);
    set_locus(eextern, @1);
    $$ = eextern;
}
| STATIC
{
    tree sstatic = tree_make(T_STATIC);
    set_locus(sstatic, @1);
    $$ = sstatic;
}
;

direct_type_specifier
: CHAR
{
    tree type = tree_make(D_T_CHAR);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED CHAR
{
    tree type = tree_make(D_T_CHAR);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED CHAR
{
    tree type = tree_make(D_T_UCHAR);
    set_locus(type, @1);
    $$ = type;
}
| SHORT
{
    tree type = tree_make(D_T_SHORT);
    set_locus(type, @1);
    $$ = type;
}
| SHORT INT
{
    tree type = tree_make(D_T_SHORT);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED SHORT
{
    tree type = tree_make(D_T_SHORT);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED SHORT INT
{
    tree type = tree_make(D_T_SHORT);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED SHORT
{
    tree type = tree_make(D_T_USHORT);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED SHORT INT
{
    tree type = tree_make(D_T_USHORT);
    set_locus(type, @1);
    $$ = type;
}
| INT
{
    tree type = tree_make(D_T_INT);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED INT
{
    tree type = tree_make(D_T_INT);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED INT
{
    tree type = tree_make(D_T_UINT);
    set_locus(type, @1);
    $$ = type;
}
| LONG
{
    tree type = tree_make(D_T_LONG);
    set_locus(type, @1);
    $$ = type;
}
| LONG INT
{
    tree type = tree_make(D_T_LONG);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED LONG
{
    tree type = tree_make(D_T_LONG);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED LONG INT
{
    tree type = tree_make(D_T_LONG);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED LONG
{
    tree type = tree_make(D_T_ULONG);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED LONG INT
{
    tree type = tree_make(D_T_ULONG);
    set_locus(type, @1);
    $$ = type;
}
| LONG UNSIGNED INT
{
    tree type = tree_make(D_T_ULONG);
    set_locus(type, @1);
    $$ = type;
}
| LONG LONG
{
    tree type = tree_make(D_T_LONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| LONG LONG INT
{
    tree type = tree_make(D_T_LONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED LONG LONG
{
    tree type = tree_make(D_T_LONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| SIGNED LONG LONG INT
{
    tree type = tree_make(D_T_LONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED LONG LONG
{
    tree type = tree_make(D_T_ULONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| UNSIGNED LONG LONG INT
{
    tree type = tree_make(D_T_ULONGLONG);
    set_locus(type, @1);
    $$ = type;
}
| FLOAT
{
    tree type = tree_make(D_T_FLOAT);
    set_locus(type, @1);
    $$ = type;
}
| DOUBLE
{
    tree type = tree_make(D_T_DOUBLE);
    set_locus(type, @1);
    $$ = type;
}
| LONG DOUBLE
{
    tree type = tree_make(D_T_LONGDOUBLE);
    set_locus(type, @1);
    $$ = type;
}
| VOID
{
    tree type = tree_make(D_T_VOID);
    set_locus(type, @1);
    $$ = type;
}
| TYPE_NAME
{
    tree id = get_identifier($1);
    set_locus(id, @1);
    $$ = id;
}
| struct_specifier
| union_specifier
| enum_specifier
;

sizeof_specifier
: direct_type_specifier
| primary_expression
| direct_type_specifier pointer
{
    $$ = make_pointer_type($2, $1);
}
;

type_specifier
: direct_type_specifier
| storage_class_specifier direct_type_specifier
{
    switch ($1->type)
    {
    case T_TYPEDEF:
        tTYPEDEF_EXP($1) = $2;
        break;
    case T_EXTERN:
        tEXTERN_EXP($1) = $2;
        break;
    case T_STATIC:
        tSTATIC_EXP($1) = $2;
        break;
    default:
        yyerror("Unknown storage_class_specifier tree type");
        YYERROR;
    }

    $$ = $1;
}
;

struct_specifier
: STRUCT IDENTIFIER '{' compound_decl_list '}'
{
    char *struct_name = concat_string("struct ", $2);
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = get_identifier(struct_name);
    tCOMP_DECL_DECLS(decl) = $4;
    tCOMP_DECL_TYPE(decl) = sstruct;
    set_locus(decl, @1);
    set_locus(tCOMP_DECL_ID(decl), @2);
    $$ = decl;
}
| STRUCT '{' compound_decl_list '}'
{
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = NULL;
    tCOMP_DECL_DECLS(decl) = $3;
    tCOMP_DECL_TYPE(decl) = sstruct;
    set_locus(decl, @1);
    $$ = decl;
}
| STRUCT IDENTIFIER
{
    char *struct_name = concat_string("struct ", $2);
    tree ret = tree_make(T_STRUCT);
    tSTRUCT_EXP(ret) = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(tSTRUCT_EXP(ret), @2);
    $$ = ret;
}
| STRUCT TYPE_NAME
{
    char *struct_name = concat_string("struct ", $2);
    tree ret = tree_make(T_STRUCT);
    tSTRUCT_EXP(ret) = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(tSTRUCT_EXP(ret), @2);
    $$ = ret;
}
;

union_specifier
: UNION IDENTIFIER '{' compound_decl_list '}'
{
    char *union_name = concat_string("union ", $2);
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = get_identifier(union_name);
    tCOMP_DECL_DECLS(decl) = $4;
    tCOMP_DECL_TYPE(decl) = uunion;
    set_locus(decl, @1);
    set_locus(tCOMP_DECL_ID(decl), @2);
    $$ = decl;
}
| UNION '{' compound_decl_list '}'
{
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = NULL;
    tCOMP_DECL_DECLS(decl) = $3;
    tCOMP_DECL_TYPE(decl) = uunion;
    set_locus(decl, @1);
    $$ = decl;
}
| UNION IDENTIFIER
{
    char *union_name = concat_string("union ", $2);
    tree ret = tree_make(T_UNION);
    tUNION_EXP(ret) = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(tUNION_EXP(ret), @2);
    $$ = ret;

}
| UNION TYPE_NAME
{
    char *union_name = concat_string("union ", $2);
    tree ret = tree_make(T_UNION);
    tUNION_EXP(ret) = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(tUNION_EXP(ret), @2);
    $$ = ret;

}
;

enum_specifier
: ENUM '{' enumerator_list '}'
{
    tree enumerator = tree_make(T_ENUMERATOR);
    tENUM_NAME(enumerator) = NULL;
    tENUM_ENUMS(enumerator) = $3;
    set_locus(enumerator, @1);
    $$ = enumerator;
}
| ENUM IDENTIFIER '{' enumerator_list '}'
{
    char *enum_name = concat_string("enum ", $2);
    tree enumerator = tree_make(T_ENUMERATOR);
    tENUM_NAME(enumerator) = get_identifier(enum_name);
    tENUM_ENUMS(enumerator) = $4;
    set_locus(enumerator, @1);
    set_locus(tENUM_NAME(enumerator), @2);
    $$ = enumerator;
}
| ENUM IDENTIFIER
{
    char *enum_name = concat_string("enum ", $2);
    tree id = get_identifier(enum_name);
    set_locus(id, @2);
    $$ = id;
}
| ENUM TYPE_NAME
{
    char *enum_name = concat_string("enum ", $2);
    tree id = get_identifier(enum_name);
    set_locus(id, @2);
    $$ = id;
}
;

enumerator_list
: enumerator
{
    $$ = tree_chain_head($1);
}
| enumerator_list ',' enumerator
{
    tree_chain($3, $1);
}
;

enumerator
: IDENTIFIER
{
    tree id = get_identifier($1);
    set_locus(id, @1);
    $$ = id;
}

compound_decl_list
: compound_decl
{
    $$ = tree_chain_head($1);
}
| compound_decl_list compound_decl
{
    tree_chain($2, $1);
}
;

func_ptr_decl
: type_specifier '(' pointer IDENTIFIER ')' argument_specifier
{
    tree function, decl;
    function = tree_make(T_DECL_FN);
    tFNDECL_NAME(function) = NULL;
    tFNDECL_RET_TYPE(function) = $1;
    tFNDECL_ARGS(function) = $6;
    tFNDECL_STMTS(function) = NULL;

    decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = function;
    tDECL_DECLS(decl) = make_pointer_type($3, get_identifier($4));

    set_locus(function, @1);
    set_locus(decl, @4);

    $$ = decl;
}

compound_decl
: type_specifier direct_declarator_list ';'
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = $1;
    tDECL_DECLS(decl) = $2;
    set_locus(decl, @1);
    $$ = decl;
}
| func_ptr_decl ';'
;

declaration
: type_specifier declarator_list
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = $1;
    tDECL_DECLS(decl) = $2;
    set_locus(decl, @1);
    $$ = decl;

    /* Check to see if `type_specifier' is a typedef. If so, add all
     * identifiers in `declarator_list' to the type_names list.  This
     * will make the lexer tokenise all subsequent instances of the
     * identifier string as a TYPE_NAME token. */
    if (is_T_TYPEDEF($1)) {
        tree i;
        for_each_tree(i, $2) {
            tree newid, oldid = i;

            while (is_T_POINTER(oldid))
                oldid = tPTR_EXP(oldid);

            if (is_T_DECL_FN(oldid))
                oldid = tFNDECL_NAME(oldid);

            if (is_T_ARRAY(oldid))
               oldid = tARRAY_ID(oldid);

            if (!is_T_IDENTIFIER(oldid)) {
                yyerror("Expected identifier when processing typedef");
                YYERROR;
            }

            add_typename(GC_STRDUP(tID_STR(oldid)));
        }
    }
}
CFILE_ONLY
    | func_ptr_decl
    | type_specifier
    {
        tree decl = tree_make(T_DECL);
        tDECL_TYPE(decl) = $1;
        tDECL_DECLS(decl) = NULL;
        set_locus(decl, @1);
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
ALL_TARGETS
;
