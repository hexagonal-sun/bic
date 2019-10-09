divert(-1)
include(parser-lex-funcs.m4)
define(LEXLOC, TARGET`lloc')
define(LEXTEXT, TARGET`text')
define(LEXLVAL, TARGET`lval')
divert(0)dnl
%{
`#'include <stdio.h>
`#'include <string.h>
`#'include <stdarg.h>
`#'include "tree.h"
`#'include "util.h"
`#'include "TARGET()parser.h"
`#'include "typename.h"
`#'define YY_NO_INPUT

static int colnum = 1;
static int linenum = 1;

`#'define YY_USER_ACTION {LEXLOC.first_line = linenum;  \
        LEXLOC.first_column = colnum;                 \
        colnum=colnum+TARGET()leng;                    \
        LEXLOC.last_column=colnum - 1;                \
        LEXLOC.last_line = linenum;}


static char *sl_buf = NULL;
extern void cfile_parser_set_file(const char *fname);

static void handle_line_marker(char *s)
{
    /* Lex the '#' */
    s = strtok(s, " ");

    if (!s)
        return;

    /* Now the line number. */
    s = strtok(NULL, " ");

    if (!s)
        return;

    linenum = atoi(s) - 1;

    /* Now the file name.
     *
     * Note that the filename is sourrounded by quotes. Remove that here.*/
    s = strtok(NULL, " ");

    if (!s)
        return;

    s += 1;
    s[strlen(s) - 1] = 0;

    cfile_parser_set_file(s);
}

static void sl_begin(void)
{
    if (!sl_buf)
        sl_buf = malloc(1);

    sl_buf[0] = '\0';
}

static void sl_append_str(const char *s)
{
    char *old_sl_buf = sl_buf;
    sl_buf = concat_strings(sl_buf, s);
    free(old_sl_buf);
}

static void sl_append_char(char c)
{
    char *s = malloc(2);
    s[0] = c;
    s[1] = '\0';
    sl_append_str(s);
    free(s);
}

static void  __attribute__((noreturn))lex_err(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static char lex_char(char *str)
{
    size_t char_str_len = strlen(str);

    if (!(char_str_len == 3 || char_str_len == 4))
       lex_err("character string %s does not contain three or four chars.\n",
               str);

    if (str[0] != '\'')
        lex_err("first char in char string %s is not a `'`.\n", str);

    if (str[1] != ''\\')
        return str[1];

    switch (str[2]) {
    case 'n':
        return '\n';
    case '\\':
        return '\\';
    case 'r':
        return '\r';
    default:
        lex_err("Unknown char escapse sequence `%c'\n", str[2]);
    }
}

int TARGET()wrap(void) {
    return 1;
}
%}

%option prefix="TARGET" outfile="TARGET()lex.c"

L                               [a-zA-Z_]
D                               [0-9]
W                               [\ \t]
X                               [0-9A-Fa-f]

%x str_lit

%%
"__attribute__"[ \t]*"((".*"))" /* Ignore attributes */
"_Nullable"                     /* Ignore builtin */
("__")?"restrict"               /* Ignore builtin */
"__asm"("__")?[ \t]*"(".*")"    /* Ignore asm stmts. */
"__extension__"                 /* Ignore gcc warning suppression
                                 * extension. */
"volatile"                      /* Ignore volatile */
("__")?"inline"("__")?          /* Ignore inlining */
"auto"				return AUTO;
"break"				return BREAK;
"case"				return CASE;
"char"				return CHAR;
"<REPL>"      return REPL;
"const"
"continue"			return CONTINUE;
"default"			return DEFAULT;
"do"				return DO;
"double"			return DOUBLE;
"else"				return ELSE;
"enum"				return ENUM;
"extern"			return EXTERN;
"float"				return FLOAT;
"for"				return FOR;
"goto"				return GOTO;
"if"				return IF;
"int"				return INT;
"_Bool"			return BOOL;
"long"				return LONG;
"register"			return REGISTER;
"return"			return RETURN;
"short"				return SHORT;
"signed"			return SIGNED;
"sizeof"			return SIZEOF;
"static"			return STATIC;
"struct"			return STRUCT;
"switch"			return SWITCH;
"typedef"			return TYPEDEF;
"union"				return UNION;
"unsigned"			return UNSIGNED;
"void"				return VOID;
"while"				return WHILE;

REPL_ONLY
"`#'include"[ \t]*"<"({L}|{D}|\.|\/)+">" {LEXLVAL.string = strdup(LEXTEXT);
                                     return C_PRE_INC; }
CFILE_ONLY
"`#'"[ \t]*[0-9]+[ \t]*\"({L}|{D}|\-|\.|\/|<|>|\ )+\"[ 1-4]*    {handle_line_marker(LEXTEXT);}
ALL_TARGETS
{L}({L}|{D})*                   {LEXLVAL.tree = get_identifier(LEXTEXT);
                                 if (is_typename(LEXLVAL.tree))
                                     return TYPE_NAME;
                                 else
                                     return IDENTIFIER; }

-?[0-9]+                          mpz_init_set_str(LEXLVAL.integer, LEXTEXT, 10); return INTEGER;
-?[0-9]+\.[0-9]+                mpf_init_set_str(LEXLVAL.ffloat, LEXTEXT, 10); return FLOAT_CST;
0x{X}+                          mpz_init_set_str(LEXLVAL.integer, LEXTEXT, 0); return INTEGER;
L?'(\\.|[^\\'\n])+'	        mpz_init_set_si(LEXLVAL.integer, lex_char(LEXTEXT)); return INTEGER;
\"                              { BEGIN str_lit; sl_begin(); }
<str_lit>[^\\"\n]*              { sl_append_str(strdup(LEXTEXT)); }
<str_lit>\\n                    { sl_append_char('\n'); }
<str_lit>\\t                    { sl_append_char('\t'); }
<str_lit>\\[0-7]*               { sl_append_char(strtol(LEXTEXT+1, 0, 8)); }
<str_lit>\\[\\"]                { sl_append_char(LEXTEXT[1]); }
<str_lit>\"                     { LEXLVAL.string = strdup(sl_buf); BEGIN 0; return CONST_STRING; }
<str_lit>\\.                    { lex_err("bogus escape '%s' in string\n", LEXTEXT); }
<str_lit>\n                     { lex_err("newline in string\n"); }

[ \t\r]                         /* skip whitespace */
"\n"                            {linenum++; colnum = 1;}

"..."                           return ELLIPSIS;
"=="                            return EQUATE;
"!="                            return NOT_EQUATE;
"="                             return '=';
"("                             return '(';
")"                             return ')';
";"                             return ';';
","                             return ',';
"'"                             return '\'';
":"                             return ':';
"["                             return '[';
"]"                             return ']';
"."                             return '.';
"<="                            return LESS_OR_EQUAL;
">="                            return GREATER_OR_EQUAL;
"+="                            return ADD_ASSIGN;
"-="                            return SUB_ASSIGN;
"/="                            return DIV_ASSIGN;
"<<="                           return LSHIFT_ASSIGN;
">>="                           return RSHIFT_ASSIGN;
"^="                            return XOR_ASSIGN;
"<"                             return '<';
">"                             return '>';
"!"                             return '!';
"+"                             return '+';
"-"                             return '-';
"*"                             return '*';
"/"                             return '/';
"%"                             return '%';
"{"                             return '{';
"}"                             return '}';
"&"                             return '&';
"|"                             return '|';
"\?"                             return '?';
"^"                             return '^';
"<<"                            return SHIFT_LEFT;
">>"                            return SHIFT_RIGHT;
"++"                            return INC;
"--"                            return DEC;
"->"                            return PTR_ACCESS;

"&&"                            return BOOL_OP_AND;
"||"                            return BOOL_OP_OR;
%%
