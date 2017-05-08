%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gc.h"
#include "tree.h"
#include "parser.h"

int yyparse(void);
int yylex(void);
void yyerror(const char *str);

extern tree parse_head;

static void set_locus(tree t, YYLTYPE locus)
{
    t->locus.line_no = locus.first_line;
    t->locus.column_no = locus.first_column;
}

static char * concat_string(const char *s1, const char *s2)
{
    char *ret = malloc(strlen(s1) + strlen(s2) + 1);
    ret[0] = '\0';
    strcat(ret, s1);
    strcat(ret, s2);
    return ret;
}

static tree type_names;
GC_TREE_DECL(type_names);

static void add_typename(char *s)
{
    tree_chain(get_identifier(s), type_names);
}

void parser_init(void)
{
    type_names = tree_make(CHAIN_HEAD);

    /* Add all builtin types here so the parser knows about them. */
    add_typename("__builtin_va_list");
}

int is_typename(char *identifier)
{
    tree i;

    for_each_tree(i, type_names) {
        if (strcmp(identifier, i->data.id.name) == 0)
            return 1;
    }

    return 0;
}
%}

%code provides {
void parser_init(void);
int is_typename(char *identifier);
}

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
%token DEC ELLIPSIS PTR_ACCESS

%token <string> IDENTIFIER
%token <string> TYPE_NAME
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
%type <tree> pointer
%type <tree> declarator
%type <tree> initialiser
%type <tree> declarator_list
%type <tree> direct_declarator_list
%type <tree> struct_specifier
%type <tree> union_specifier
%type <tree> enum_specifier
%type <tree> enumerator_list
%type <tree> enumerator
%type <tree> compound_decl_list
%type <tree> compound_decl
%type <tree> storage_class_specifier
%type <tree> direct_type_specifier
%type <tree> sizeof_specifier
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
    set_locus(function_def, @1);
    set_locus(function_def->data.function.id, @2);
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
    set_locus(variadic, @3);
    tree_chain(variadic, $1);
}

argument_decl
: type_specifier decl_possible_pointer
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
    set_locus(decl, @1);
    $$ = decl;
}
| type_specifier pointer
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = make_pointer_type($2, $1);
    set_locus(decl, @1);
    $$ = decl;
}
| type_specifier
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    set_locus(decl, @1);
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
    set_locus(for_loop, @1);
    $$ = for_loop;
}
;

primary_expression
: INTEGER
{
    tree number = tree_make(T_INTEGER);
    mpz_init_set(number->data.integer, $1);
    mpz_clear($1);
    set_locus(number, @1);
    $$ = number;
}
| FLOAT_CST
{
    tree ffloat = tree_make(T_FLOAT);
    mpf_init_set(ffloat->data.ffloat, $1);
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
    str->data.string = $1;
    set_locus(str, @1);
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
    set_locus(fncall, @1);
    $$ = fncall;
}
| postfix_expression '(' argument_expression_list ')'
{
    tree fncall = tree_make(T_FN_CALL);
    fncall->data.fncall.identifier = $1;
    fncall->data.fncall.arguments = $3;
    set_locus(fncall, @1);
    $$ = fncall;
}
| postfix_expression '[' assignment_expression ']'
{
    tree arr_access = tree_make(T_ARRAY_ACCESS);
    arr_access->data.bin.left = $1;
    arr_access->data.bin.right = $3;
    set_locus(arr_access, @1);
    $$ = arr_access;
}
| postfix_expression INC
{
    tree inc = tree_make(T_P_INC);
    inc->data.exp = $1;
    set_locus(inc, @2);
    $$ = inc;
}
| postfix_expression DEC
{
    tree dec = tree_make(T_P_DEC);
    dec->data.exp = $1;
    set_locus(dec, @2);
    $$ = dec;
}
| postfix_expression '.' IDENTIFIER
{
    tree access = tree_make(T_ACCESS);
    access->data.bin.left = $1;
    access->data.bin.right = get_identifier($3);
    set_locus(access, @2);
    set_locus(access->data.bin.right, @3);
    $$ = access;
}
| postfix_expression PTR_ACCESS IDENTIFIER
{
    tree deref = tree_make(T_DEREF);
    tree access = tree_make(T_ACCESS);

    deref->data.exp = $1;
    access->data.bin.left = deref;
    access->data.bin.right = get_identifier($3);

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
    inc->data.exp = $2;
    set_locus(inc, @1);
    $$ = inc;
}
| DEC unary_expression
{
    tree dec = tree_make(T_DEC);
    dec->data.exp = $2;
    set_locus(dec, @1);
    $$ = dec;
}
| '&' unary_expression
{
    tree addr = tree_make(T_ADDR);
    addr->data.exp = $2;
    set_locus(addr, @1);
    $$ = addr;
}
| '*' unary_expression
{
    tree deref = tree_make(T_DEREF);
    deref->data.exp = $2;
    set_locus(deref, @1);
    $$ = deref;
}
| SIZEOF '(' sizeof_specifier ')'
{
    tree szof = tree_make(T_SIZEOF);
    szof->data.exp = $3;
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

assignment_expression
: relational_expression
| unary_expression '=' assignment_expression
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
    fn_decl->data.function.id = get_identifier($1);
    fn_decl->data.function.arguments = $2;
    set_locus(fn_decl, @1);
    set_locus(fn_decl->data.function.id, @1);
    $$ = fn_decl;
}
| decl '[' additive_expression ']'
{
    tree array = tree_make(T_ARRAY);
    array->data.bin.left = $1;
    array->data.bin.right = $3;
    set_locus(array, @2);
    $$ = array;
}
| decl '[' ']'
{
    tree ptr = tree_make(T_POINTER);
    ptr->data.exp = $1;
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
    ptr->data.exp = $2;
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
    $1->data.exp = $2;
    $$ = $1;
}
;

struct_specifier
: STRUCT IDENTIFIER '{' compound_decl_list '}'
{
    char *struct_name = concat_string("struct ", $2);
    tree decl = tree_make(T_DECL_COMPOUND);
    decl->data.comp_decl.id = get_identifier(struct_name);
    decl->data.comp_decl.decls = $4;
    decl->data.comp_decl.type = sstruct;
    set_locus(decl, @1);
    set_locus(decl->data.comp_decl.id, @2);
    free($2);
    $$ = decl;
}
| STRUCT '{' compound_decl_list '}'
{
    tree decl = tree_make(T_DECL_COMPOUND);
    decl->data.comp_decl.id = NULL;
    decl->data.comp_decl.decls = $3;
    decl->data.comp_decl.type = sstruct;
    set_locus(decl, @1);
    $$ = decl;
}
| STRUCT IDENTIFIER
{
    char *struct_name = concat_string("struct ", $2);
    tree ret = tree_make(T_STRUCT);
    ret->data.exp = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(ret->data.exp, @2);
    free($2);
    $$ = ret;
}
| STRUCT TYPE_NAME
{
    char *struct_name = concat_string("struct ", $2);
    tree ret = tree_make(T_STRUCT);
    ret->data.exp = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(ret->data.exp, @2);
    free($2);
    $$ = ret;
}
;

union_specifier
: UNION IDENTIFIER '{' compound_decl_list '}'
{
    char *union_name = concat_string("union ", $2);
    tree decl = tree_make(T_DECL_COMPOUND);
    decl->data.comp_decl.id = get_identifier(union_name);
    decl->data.comp_decl.decls = $4;
    decl->data.comp_decl.type = uunion;
    set_locus(decl, @1);
    set_locus(decl->data.comp_decl.id, @2);
    free($2);
    $$ = decl;
}
| UNION '{' compound_decl_list '}'
{
    tree decl = tree_make(T_DECL_COMPOUND);
    decl->data.comp_decl.id = NULL;
    decl->data.comp_decl.decls = $3;
    decl->data.comp_decl.type = uunion;
    set_locus(decl, @1);
    $$ = decl;
}
| UNION IDENTIFIER
{
    char *union_name = concat_string("union ", $2);
    tree ret = tree_make(T_UNION);
    ret->data.exp = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(ret->data.exp, @2);
    free($2);
    $$ = ret;

}
| UNION TYPE_NAME
{
    char *union_name = concat_string("union ", $2);
    tree ret = tree_make(T_UNION);
    ret->data.exp = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(ret->data.exp, @2);
    free($2);
    $$ = ret;

}
;

enum_specifier
: ENUM '{' enumerator_list '}'
{
    tree enumerator = tree_make(T_ENUMERATOR);
    enumerator->data.enumerator.id = NULL;
    enumerator->data.enumerator.enums = $3;
    set_locus(enumerator, @1);
    $$ = enumerator;
}
| ENUM IDENTIFIER '{' enumerator_list '}'
{
    char *enum_name = concat_string("enum ", $2);
    tree enumerator = tree_make(T_ENUMERATOR);
    enumerator->data.enumerator.id = get_identifier(enum_name);
    enumerator->data.enumerator.enums = $4;
    set_locus(enumerator, @1);
    set_locus(enumerator->data.enumerator.id, @2);
    free($2);
    $$ = enumerator;
}
| ENUM IDENTIFIER
{
    char *enum_name = concat_string("enum ", $2);
    tree id = get_identifier(enum_name);
    set_locus(id, @2);
    free($2);
    $$ = id;
}
| ENUM TYPE_NAME
{
    char *enum_name = concat_string("enum ", $2);
    tree id = get_identifier(enum_name);
    set_locus(id, @2);
    free($2);
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

compound_decl
: type_specifier direct_declarator_list ';'
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
    set_locus(decl, @1);
    $$ = decl;
}

declaration
: type_specifier declarator_list
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = $2;
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
                oldid = oldid->data.exp;

            if (is_T_DECL_FN(oldid))
                oldid = oldid->data.function.id;

            if (!is_T_IDENTIFIER(oldid)) {
                YYERROR;
            }

            newid = tree_make(T_IDENTIFIER);
            newid->data.id.name = strdup(oldid->data.id.name);
            tree_chain(newid, type_names);
        }
    }
}
| type_specifier
{
    tree decl = tree_make(T_DECL);
    decl->data.decl.type = $1;
    decl->data.decl.decls = NULL;
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
;
