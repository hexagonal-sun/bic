divert(-1)
include(parser-lex-funcs.m4)
divert(0)dnl
%{
`#'include <stdio.h>
`#'include <string.h>
`#'include <stdlib.h>
`#'include <stdbool.h>
`#'include "tree.h"
`#'include "typename.h"
`#'include "util.h"
`#'include "TARGET()parser.h"

int TARGET()parse(void);
int TARGET()lex(void);
void TARGET()error(const char *str);

extern tree TARGET()_parse_head;
CFILE_ONLY
    const char *parser_current_file;
REPL_ONLY
    const char *parser_current_file = "<REPL>";
ALL_TARGETS

static void set_locus(tree t, YYLTYPE locus)
{
    tLOCUS(t).line_no = locus.first_line;
    tLOCUS(t).column_no = locus.first_column;

    if(parser_current_file)
        tLOCUS(t).file = get_identifier(parser_current_file);
}

CFILE_ONLY
    void cfile_parser_set_file(const char *fname)
    {
        parser_current_file = fname;
    }
ALL_TARGETS

static tree build_func_ptr(tree ret_type, tree ret_type_ptr,
                           tree ptr, tree id, tree args)
{
    tree function, decl;
    bool is_decl = false;

    if (is_T_TYPEDEF(ret_type)) {
        add_typename(id);
        ret_type = tTYPEDEF_EXP(ret_type);
        is_decl = true;
   }

    ret_type = make_pointer_type(ret_type_ptr, ret_type);

    function = tree_make(T_DECL_FN);
    tFNDECL_NAME(function) = NULL;
    tFNDECL_RET_TYPE(function) = ret_type;
    tFNDECL_ARGS(function) = args;
    tFNDECL_STMTS(function) = NULL;

    if (is_decl)
    {
        tree td = tree_make(T_TYPEDEF);
        tTYPEDEF_EXP(td) = function;
        function = td;
    }

    decl = tree_make(T_DECL);

    if (id) {
        tDECL_TYPE(decl) = function;
        tDECL_DECLS(decl) = make_pointer_type(ptr, id);
    } else
        tDECL_TYPE(decl) = function;

    return decl;
}

static tree handle_declaration(tree type, tree declarator_list)
{
    tree decl = tree_make(T_DECL);
    tDECL_TYPE(decl) = type;
    tDECL_DECLS(decl) = declarator_list;

    /* Check to see if `type' is a typedef. If so, add all identifiers
     * in `declarator_list' to the type_names list.  This will make
     * the lexer tokenise all subsequent instances of the identifier
     * string as a TYPE_NAME token. */
    if (is_T_TYPEDEF(type)) {
        tree i;
        for_each_tree(i, declarator_list) {
            tree oldid = i;

            while (is_T_POINTER(oldid))
                oldid = tPTR_EXP(oldid);

            if (is_T_DECL_FN(oldid))
                oldid = tFNDECL_NAME(oldid);

            if (is_T_ARRAY(oldid))
               oldid = tARRAY_ID(oldid);

            if (!is_T_IDENTIFIER(oldid)) {
                yyerror("Expected identifier when processing typedef");
                return NULL;
            }

            add_typename(oldid);
        }
    }

    return decl;
}


%}

%union
{
    mpz_t integer;
    mpf_t ffloat;
    char *string;
    tree tree;
}

%define parse.error verbose
%locations

%define api.prefix {TARGET}

%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO
%token DOUBLE ENUM EXTERN FLOAT FOR GOTO IF INT LONG
%token REGISTER RETURN SHORT SIGNED SIZEOF STATIC STRUCT
%token SWITCH TYPEDEF UNION UNSIGNED VOID WHILE
%token EQUATE NOT_EQUATE LESS_OR_EQUAL GREATER_OR_EQUAL
%token SHIFT_LEFT SHIFT_RIGHT BOOL_OP_AND BOOL_OP_OR INC
%token DEC ELLIPSIS PTR_ACCESS BOOL REPL ADD_ASSIGN SUB_ASSIGN
%token DIV_ASSIGN LSHIFT_ASSIGN RSHIFT_ASSIGN XOR_ASSIGN

%right ')' ELSE

%token <tree> IDENTIFIER
REPL_ONLY
%token <string> C_PRE_INC
ALL_TARGETS
%token <tree> TYPE_NAME
%token <string> CONST_BITS
%token <string> CONST_STRING
%token <integer> INTEGER;
%token <ffloat> FLOAT_CST;

CFILE_ONLY
%type <tree> translation_unit
%type <tree> toplevel_declarations
%type <tree> compound_statement
%type <tree> function_definition
%type <tree> jump_statement
%type <tree> repl_statement
REPL_ONLY
%type <tree> inspection_statement
ALL_TARGETS
%type <tree> declaration_statement
%type <tree> func_ptr_decl
%type <tree> argument_specifier
%type <tree> direct_argument_list
%type <tree> argument_list
%type <tree> argument_decl
%type <tree> statement
%type <tree> statement_list
%type <tree> expression_statement
%type <tree> iteration_statement
%type <tree> selection_statement
%type <tree> primary_expression
%type <tree> postfix_expression
%type <tree> argument_expression_list
%type <tree> unary_expression
%type <tree> cast_expression
%type <tree> multiplicative_expression
%type <tree> additive_expression
%type <tree> shift_expression
%type <tree> and_expression
%type <tree> exclusive_or_expression
%type <tree> inclusive_or_expression
%type <tree> relational_expression
%type <tree> equality_expression
%type <tree> logical_or_expression
%type <tree> logical_and_expression
%type <tree> assignment_expression
%type <tree> expression
%type <tree> conditional_expression
%type <tree> constant_expression
%type <tree> direct_declarator
%type <tree> decl_possible_pointer
%type <tree> pointer
%type <tree> declarator
%type <tree> initialiser
%type <tree> declarator_list
%type <tree> struct_specifier
%type <tree> union_specifier
%type <tree> enum_specifier
%type <tree> enumerator_list
%type <tree> enumerator
%type <tree> storage_class_specifier
%type <tree> direct_type_specifier
%type <tree> sizeof_specifier
%type <tree> type_specifier
%type <tree> struct_declaration_list
%type <tree> struct_declaration
%type <tree> struct_declarator_list
%type <tree> struct_declarator
%type <tree> declaration
%type <tree> declaration_list

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
        tFNDEF_NAME(function_def) = $2;
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
| func_ptr_decl
;

REPL_ONLY
    inspection_statement
    : '?' IDENTIFIER
    {
        tree inspect = tree_make(T_INSPECT);
        tINSPECT_EXP(inspect) = $2;
        $$ = inspect;
    }
ALL_TARGETS

statement
: expression_statement
| iteration_statement
| selection_statement
CFILE_ONLY
    | compound_statement
    | jump_statement
    | repl_statement
REPL_ONLY
    | declaration_statement
    | inspection_statement
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
ALL_TARGETS

declaration_statement
: declaration ';'
;

expression
: assignment_expression
| expression ',' assignment_expression
{
    tree comma = tree_make(T_COMMA);
    tCOMMA_LHS(comma) = $1;
    tCOMMA_RHS(comma) = $3;
    set_locus(comma, @2);
    $$ = comma;
}
;

expression_statement
: expression ';'
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
| FOR '(' declaration_statement expression_statement assignment_expression ')' statement
{
tree for_loop = tree_make(T_LOOP_FOR);
tFLOOP_INIT(for_loop) = $3;
tFLOOP_COND(for_loop) = $4;
tFLOOP_AFTER(for_loop) = $5;
tFLOOP_STMTS(for_loop) = $7;
set_locus(for_loop, @1);
$$ = for_loop;
}
| WHILE '(' assignment_expression ')' statement
{
    tree while_loop = tree_make(T_LOOP_WHILE);
    tWLOOP_COND(while_loop) = $3;
    tWLOOP_STMTS(while_loop) = $5;
    set_locus(while_loop, @1);
    $$ = while_loop;
}
;

selection_statement
: IF '(' assignment_expression ')' statement
{
    tree ifstmt = tree_make(T_IF);
    tIF_COND(ifstmt) = $3;
    tIF_TRUE_STMTS(ifstmt) = $5;
    tIF_ELSE_STMTS(ifstmt) = NULL;
    set_locus(ifstmt, @1);
    $$ = ifstmt;
}
| IF '(' assignment_expression ')' statement ELSE statement
{
    tree ifstmt = tree_make(T_IF);
    tIF_COND(ifstmt) = $3;
    tIF_TRUE_STMTS(ifstmt) = $5;
    tIF_ELSE_STMTS(ifstmt) = $7;
    set_locus(ifstmt, @1);
    $$ = ifstmt;
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
    | BREAK ';'
    {
        $$ = tree_make(T_BREAK);
    }
    ;
ALL_TARGETS

CFILE_ONLY
    repl_statement
    : REPL ';'
    {
      $$ = tree_make(T_REPL);
    }
    ;
ALL_TARGETS

primary_expression
: INTEGER
{
    tree number = tree_make(T_INTEGER);
    mpz_init_set(tINT_VAL(number), $1);
    mpz_clear($1);
    set_locus(number, @1);
    $$ = number;
}
| FLOAT_CST
{
    tree ffloat = tree_make(T_FLOAT);
    mpf_init_set(tFLOAT_VAL(ffloat), $1);
    mpf_clear($1);
    set_locus(ffloat, @1);
    $$ = ffloat;
}
| IDENTIFIER
{
    tree identifier = $1;
    set_locus(identifier, @1);
    $$ = identifier;
}
| CONST_STRING
{
    tree str = tree_make(T_STRING);
    tSTRING_VAL(str) = $1;
    set_locus(str, @1);
    $$ = str;
}
| '(' expression ')'
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
| postfix_expression '[' expression ']'
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
    tCOMP_ACCESS_MEMBER(access) = $3;
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
    tCOMP_ACCESS_MEMBER(access) = $3;

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
| '!' unary_expression
{
    tree negate = tree_make(T_NEGATE);
    tNEGATE_EXP(negate) = $2;
    set_locus(negate, @1);
    $$ = negate;
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

cast_expression
: unary_expression
| '(' direct_type_specifier ')' cast_expression
{
    tree cast = tree_make(T_CAST);
    tCAST_NEWTYPE(cast) = $2;
    tCAST_EXP(cast) = $4;
    set_locus(cast, @1);
    $$ = cast;
}
;

multiplicative_expression
: cast_expression
| multiplicative_expression '*' cast_expression
{
    tree mul = tree_make(T_MUL);
    tMUL_LHS(mul) = $1;
    tMUL_RHS(mul) = $3;
    set_locus(mul, @2);
    $$ = mul;
}
| multiplicative_expression '/' cast_expression
{
    tree div = tree_make(T_DIV);
    tDIV_LHS(div) = $1;
    tDIV_RHS(div) = $3;
    set_locus(div, @2);
    $$ = div;
}
| multiplicative_expression '%' cast_expression
{
    tree mod = tree_make(T_MOD);
    tMOD_LHS(mod) = $1;
    tMOD_RHS(mod) = $3;
    set_locus(mod, @2);
    $$ = mod;
}
;

additive_expression
: multiplicative_expression
| additive_expression '+' multiplicative_expression
{
    tree add = tree_make(T_ADD);
    tADD_LHS(add) = $1;
    tADD_RHS(add) = $3;
    set_locus(add, @2);
    $$ = add;
}
| additive_expression '-'  multiplicative_expression
{
    tree sub = tree_make(T_SUB);
    tSUB_LHS(sub) = $1;
    tSUB_RHS(sub) = $3;
    set_locus(sub, @2);
    $$ = sub;
}
;

shift_expression
: additive_expression
| shift_expression SHIFT_LEFT additive_expression
{
    tree lshift = tree_make(T_LSHIFT);
    tLSHIFT_LHS(lshift) = $1;
    tLSHIFT_RHS(lshift) = $3;
    set_locus(lshift, @2);
    $$ = lshift;
}
| shift_expression SHIFT_RIGHT additive_expression
{
    tree rshift = tree_make(T_RSHIFT);
    tRSHIFT_LHS(rshift) = $1;
    tRSHIFT_RHS(rshift) = $3;
    set_locus(rshift, @2);
    $$ = rshift;
}
;

relational_expression
: shift_expression
| relational_expression '<' shift_expression
{
    tree lt = tree_make(T_LT);
    tLT_LHS(lt) = $1;
    tLT_RHS(lt) = $3;
    $$ = lt;
}
| relational_expression '>' shift_expression
{
    tree gt = tree_make(T_GT);
    tGT_LHS(gt) = $1;
    tGT_RHS(gt) = $3;
    set_locus(gt, @2);
    $$ = gt;
}
| relational_expression LESS_OR_EQUAL shift_expression
{
    tree ltoreq = tree_make(T_LTEQ);
    tLTEQ_LHS(ltoreq) = $1;
    tLTEQ_RHS(ltoreq) = $3;
    set_locus(ltoreq, @2);
    $$ = ltoreq;
}
| relational_expression GREATER_OR_EQUAL shift_expression
{
    tree gtoreq = tree_make(T_GTEQ);
    tGTEQ_LHS(gtoreq) = $1;
    tGTEQ_RHS(gtoreq) = $3;
    set_locus(gtoreq, @2);
    $$ = gtoreq;
}
;

equality_expression
: relational_expression
| equality_expression EQUATE relational_expression
{
    tree equal = tree_make(T_EQ);
    tEQ_LHS(equal) = $1;
    tEQ_RHS(equal) = $3;
    set_locus(equal, @2);
    $$ = equal;
}
| equality_expression NOT_EQUATE relational_expression
{
    tree not_equal = tree_make(T_N_EQ);
    tN_EQ_LHS(not_equal) = $1;
    tN_EQ_RHS(not_equal) = $3;
    set_locus(not_equal, @2);
    $$ = not_equal;
}
;

and_expression
: equality_expression
| and_expression '&' equality_expression
{
    tree inclusive_and = tree_make(T_I_AND);
    tI_AND_LHS(inclusive_and) = $1;
    tI_AND_RHS(inclusive_and) = $3;
    set_locus(inclusive_and, @2);
    $$ = inclusive_and;
}
;

exclusive_or_expression
: and_expression
| exclusive_or_expression '^' and_expression
{
    tree exclusive_or = tree_make(T_X_OR);
    tX_OR_LHS(exclusive_or) = $1;
    tX_OR_RHS(exclusive_or) = $3;
    set_locus(exclusive_or, @2);
    $$ = exclusive_or;
}
;

inclusive_or_expression
: exclusive_or_expression
| inclusive_or_expression '|' exclusive_or_expression
{
    tree inclusive_or = tree_make(T_I_OR);
    tI_OR_LHS(inclusive_or) = $1;
    tI_OR_RHS(inclusive_or) = $3;
    set_locus(inclusive_or, @2);
    $$ = inclusive_or;
}

logical_and_expression
: inclusive_or_expression
| logical_and_expression BOOL_OP_AND inclusive_or_expression
{
    tree logicand = tree_make(T_L_AND);
    tL_AND_LHS(logicand) = $1;
    tL_AND_RHS(logicand) = $3;
    set_locus(logicand, @2);
    $$ = logicand;
}

logical_or_expression
: logical_and_expression
| logical_or_expression BOOL_OP_OR logical_and_expression
{
    tree logicor = tree_make(T_L_OR);
    tL_OR_LHS(logicor) = $1;
    tL_OR_RHS(logicor) = $3;
    set_locus(logicor, @2);
    $$ = logicor;
}
;

conditional_expression
: logical_or_expression
| logical_or_expression '?' expression ':' conditional_expression
{
    tree infix = tree_make(T_INFIX);
    tINFIX_COND(infix) = $1;
    tINFIX_TRUE_STMT(infix) = $3;
    tINFIX_FALSE_STMT(infix) = $5;
    $$ = infix;
}

constant_expression
: conditional_expression
;

assignment_expression
: conditional_expression
| unary_expression '=' assignment_expression
{
    tree assign = tree_make(T_ASSIGN);
    tASSIGN_LHS(assign) = $1;
    tASSIGN_RHS(assign) = $3;
    set_locus(assign, @2);
    $$ = assign;
}
| unary_expression ADD_ASSIGN assignment_expression
{
    $$ = tree_make_binmod(T_ADD, tADD, $1, $3);
}
| unary_expression SUB_ASSIGN assignment_expression
{
    $$ = tree_make_binmod(T_SUB, tSUB, $1, $3);
}
| unary_expression  DIV_ASSIGN assignment_expression
{
    $$ = tree_make_binmod(T_DIV, tDIV, $1, $3);
}
| unary_expression LSHIFT_ASSIGN assignment_expression
{
    $$ = tree_make_binmod(T_LSHIFT, tLSHIFT, $1, $3);
}
| unary_expression RSHIFT_ASSIGN assignment_expression
{
    $$ = tree_make_binmod(T_RSHIFT, tRSHIFT, $1, $3);
}
| unary_expression XOR_ASSIGN assignment_expression
{
$$ = tree_make_binmod(T_X_OR, tX_OR, $1, $3);
}
;

direct_declarator
: IDENTIFIER
{
    tree id = $1;
    set_locus(id, @1);
    $$ = id;
}
| IDENTIFIER argument_specifier
{
    tree fn_decl = tree_make(T_DECL_FN);
    tFNDECL_NAME(fn_decl) = $1;
    tFNDECL_ARGS(fn_decl) = $2;
    set_locus(fn_decl, @1);
    set_locus(tFNDECL_NAME(fn_decl), @1);
    $$ = fn_decl;
}
| '(' IDENTIFIER ')' argument_specifier
{
    tree fn_decl = tree_make(T_DECL_FN);
    tFNDECL_NAME(fn_decl) = $2;
    tFNDECL_ARGS(fn_decl) = $4;
    set_locus(fn_decl, @2);
    set_locus(tFNDECL_NAME(fn_decl), @2);
    $$ = fn_decl;
}
| direct_declarator '[' additive_expression ']'
{
    tree array = tree_make(T_ARRAY);
    tARRAY_ID(array) = $1;
    tARRAY_SZ(array) = $3;
    set_locus(array, @2);
    $$ = array;
}
| direct_declarator '[' ']'
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
: direct_declarator
| pointer direct_declarator
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
    tree assign = tree_make(T_ASSIGN);
    tASSIGN_LHS(assign) = $1;
    tASSIGN_RHS(assign) = $3;
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
| SIGNED
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
| UNSIGNED
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
| BOOL
{
    tree type = tree_make(D_T_INT);
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
    tree id = $1;
    set_locus(id, @1);
    $$ = id;
}
| struct_specifier
| union_specifier
| enum_specifier
;

sizeof_specifier
: direct_type_specifier
| unary_expression
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
: STRUCT IDENTIFIER '{' struct_declaration_list '}'
{
    char *struct_name = concat_strings("struct ", tID_STR($2));
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = get_identifier(struct_name);
    tCOMP_DECL_DECLS(decl) = $4;
    tCOMP_DECL_TYPE(decl) = sstruct;
    set_locus(decl, @1);
    set_locus(tCOMP_DECL_ID(decl), @2);
    $$ = decl;
}
| STRUCT '{' struct_declaration_list '}'
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
    char *struct_name = concat_strings("struct ", tID_STR($2));
    tree ret = tree_make(T_STRUCT);
    tSTRUCT_EXP(ret) = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(tSTRUCT_EXP(ret), @2);
    $$ = ret;
}
| STRUCT TYPE_NAME
{
    char *struct_name = concat_strings("struct ", tID_STR($2));
    tree ret = tree_make(T_STRUCT);
    tSTRUCT_EXP(ret) = get_identifier(struct_name);
    set_locus(ret, @1);
    set_locus(tSTRUCT_EXP(ret), @2);
    $$ = ret;
}
;

union_specifier
: UNION IDENTIFIER '{' declaration_list '}'
{
    char *union_name = concat_strings("union ", tID_STR($2));
    tree decl = tree_make(T_DECL_COMPOUND);
    tCOMP_DECL_ID(decl) = get_identifier(union_name);
    tCOMP_DECL_DECLS(decl) = $4;
    tCOMP_DECL_TYPE(decl) = uunion;
    set_locus(decl, @1);
    set_locus(tCOMP_DECL_ID(decl), @2);
    $$ = decl;
}
| UNION '{' declaration_list '}'
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
    char *union_name = concat_strings("union ", tID_STR($2));
    tree ret = tree_make(T_UNION);
    tUNION_EXP(ret) = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(tUNION_EXP(ret), @2);
    $$ = ret;

}
| UNION TYPE_NAME
{
    char *union_name = concat_strings("union ", tID_STR($2));
    tree ret = tree_make(T_UNION);
    tUNION_EXP(ret) = get_identifier(union_name);
    set_locus(ret, @1);
    set_locus(tUNION_EXP(ret), @2);
    $$ = ret;

}
;

possible_comma:
| ','
;

enum_specifier
: ENUM '{' enumerator_list possible_comma '}'
{
    tree enumerator = tree_make(T_ENUMERATOR);
    tENUM_NAME(enumerator) = NULL;
    tENUM_ENUMS(enumerator) = $3;
    set_locus(enumerator, @1);
    $$ = enumerator;
}
| ENUM IDENTIFIER '{' enumerator_list possible_comma '}'
{
    char *enum_name = concat_strings("enum ", tID_STR($2));
    tree enumerator = tree_make(T_ENUMERATOR);
    tENUM_NAME(enumerator) = get_identifier(enum_name);
    tENUM_ENUMS(enumerator) = $4;
    set_locus(enumerator, @1);
    set_locus(tENUM_NAME(enumerator), @2);
    $$ = enumerator;
}
| ENUM IDENTIFIER
{
    char *enum_name = concat_strings("enum ", tID_STR($2));
    tree id = get_identifier(enum_name);
    set_locus(id, @2);
    $$ = id;
}
| ENUM TYPE_NAME
{
    char *enum_name = concat_strings("enum ", tID_STR($2));
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
    tree id = $1;
    set_locus(id, @1);
    $$ = id;
}
| IDENTIFIER '=' logical_or_expression
{
    tree id = $1;
    set_locus(id, @1);
    tree assign = tree_make(T_ASSIGN);
    tASSIGN_LHS(assign) = id;
    tASSIGN_RHS(assign) = $3;
    set_locus(assign, @2);
    $$ = assign;
}
;

ALL_TARGETS

func_ptr_decl
: type_specifier pointer '(' pointer IDENTIFIER ')' argument_specifier
{
    tree decl = build_func_ptr($1, $2, $4, $5, $7);

    set_locus(decl, @5);

    $$ = decl;
}
| type_specifier '(' pointer IDENTIFIER ')' argument_specifier
{
    tree decl = build_func_ptr($1, NULL, $3, $4, $6);

    set_locus(decl, @5);

    $$ = decl;
}
| type_specifier pointer '(' pointer ')' argument_specifier
{
    tree decl = build_func_ptr($1, $2, $4, NULL, $6);

    set_locus(decl, @4);

    $$ = decl;
}
| type_specifier '(' pointer ')' argument_specifier
{
    tree decl = build_func_ptr($1, NULL, $3, NULL, $5);

    set_locus(decl, @4);

    $$ = decl;
}
;

declaration
: type_specifier declarator_list
{
    tree decl = handle_declaration($1, $2);
    set_locus(decl, @1);
    $$ = decl;
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
ALL_TARGETS
;

struct_declaration_list
: struct_declaration
{
  $$ = tree_chain_head($1);
}
| struct_declaration_list struct_declaration
{
  tree_chain($2, $1);
}
;

struct_declaration
: type_specifier struct_declarator_list ';'
{
    tree decl = handle_declaration($1, $2);
    set_locus(decl, @1);
    $$ = decl;
}
| type_specifier ';'
{
    tree decl = handle_declaration($1, NULL);
    set_locus(decl, @1);
    $$ = decl;
}
| func_ptr_decl ';'
;

struct_declarator_list
: struct_declarator
{
    $$ = tree_chain_head($1);
}
| struct_declarator_list ',' struct_declarator
{
    tree_chain($3, $1);
}
;

struct_declarator
: declarator
| ':' constant_expression
{
    tree bitfield = tree_make(T_BITFIELD);
    tBITFIELD_SZ(bitfield) = $2;
    tBITFIELD_DECLARATOR(bitfield) = NULL;
    set_locus(bitfield, @1);
    $$ = bitfield;
}
| declarator ':' constant_expression
{
    tree bitfield = tree_make(T_BITFIELD);
    tBITFIELD_SZ(bitfield) = $3;
    tBITFIELD_DECLARATOR(bitfield) = $1;
    set_locus(bitfield, @2);
    $$ = bitfield;
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
