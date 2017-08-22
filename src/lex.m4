divert(-1)
include(parser-lex-funcs.m4)
define(LEXLOC, TARGET`lloc')
define(LEXTEXT, TARGET`text')
define(LEXLVAL, TARGET`lval')
divert(0)dnl
%{
`#'include <gc.h>
`#'include <stdio.h>
`#'include <string.h>
`#'include <stdarg.h>
`#'include "tree.h"
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

static void sl_begin(void)
{
    if (!sl_buf)
        sl_buf = GC_MALLOC(1);

    sl_buf[0] = '\0';
}

static void sl_append_str(const char *s)
{
    char *buf, *oldbuf = sl_buf;
    buf = GC_MALLOC(strlen(sl_buf) + strlen(s) + 1);
    buf[0] = '\0';
    strcat(buf, sl_buf);
    strcat(buf, s);
    sl_buf = buf;
}

static void sl_append_char(char c)
{
    char *s = GC_MALLOC(2);
    s[0] = c;
    s[1] = '\0';
    sl_append_str(s);
}

static void lex_err(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

int TARGET()wrap(void) {
    return 1;
}
%}

%option prefix="TARGET" outfile="TARGET()lex.c"

L                               [a-zA-Z_]
D                               [0-9]
W                               [\ \t]
X                               [0-9A-F]

%x str_lit

%%
"__attribute__"[ \t]*"((".*"))" /* Ignore attributes */
"_Nullable"                     /* Ignore builtin */
("__")?"restrict"               /* Ignore builtin */
"__asm"("__")?[ \t]*"(".*")"    /* Ignore asm stmts. */
"__extension__"                 /* Ignore gcc warning suppression
                                 * extension. */
"inline"                        /* Ignore inlining */
"auto"				return AUTO;
"break"				return BREAK;
"case"				return CASE;
"char"				return CHAR;
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
"volatile"			return VOLATILE;
"while"				return WHILE;

REPL_ONLY
"`#'include"[ \t]*"<"({L}|{D}|\.)+">" {LEXLVAL.string = GC_STRDUP(LEXTEXT);
                                     return C_PRE_INC; }
ALL_TARGETS
{L}({L}|{D})*                   {LEXLVAL.string = GC_STRDUP(LEXTEXT);
                                 if (is_typename(LEXTEXT))
                                     return TYPE_NAME;
                                 else
                                     return IDENTIFIER; }

-?[0-9]+                          mpz_init_set_str(LEXLVAL.integer, LEXTEXT, 10); return INTEGER;
-?[0-9]+\.[0-9]+                mpf_init_set_str(LEXLVAL.ffloat, LEXTEXT, 10); return FLOAT_CST;
0x{X}+                          LEXLVAL.string = GC_STRDUP(LEXTEXT); return CONST_HEX;
\"                              { BEGIN str_lit; sl_begin(); }
<str_lit>[^\\"\n]*              { sl_append_str(LEXTEXT); }
<str_lit>\\n                    { sl_append_char('\n'); }
<str_lit>\\t                    { sl_append_char('\t'); }
<str_lit>\\[0-7]*               { sl_append_char(strtol(LEXTEXT+1, 0, 8)); }
<str_lit>\\[\\"]                { sl_append_char(LEXTEXT[1]); }
<str_lit>\"                     { LEXLVAL.string = GC_STRDUP(sl_buf); BEGIN 0; return CONST_STRING; }
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
"<<"                            return SHIFT_LEFT;
">>"                            return SHIFT_RIGHT;
"++"                            return INC;
"--"                            return DEC;
"->"                            return PTR_ACCESS;

"&&"                            return BOOL_OP_AND;
"||"                            return BOOL_OP_OR;
%%
