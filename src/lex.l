%{
#include <stdio.h>
#include <string.h>
#include "tree.h"
#include "parser.h"
#define YY_NO_INPUT
#define YY_USER_ACTION {yylloc.first_line = yylloc.last_line = yylineno;}
%}

L                               [a-zA-Z_]
D                               [0-9]
W                               [\ \t]
X                               [0-9A-F]

%option noyywrap
%option yylineno

%%
"auto"				return AUTO;
"break"				return BREAK;
"case"				return CASE;
"char"				return CHAR;
"const"				return CONST;
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

{L}({L}|{D})*                   yylval.string = strdup(yytext); return IDENTIFIER;
[0-9]+                          yylval.number = atoi(yytext); return INTEGER;
0x{X}+                          yylval.string = strdup(yytext); return CONST_HEX;

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
"{"                             return '{';
"}"                             return '}';
"<<"                            return SHIFT_LEFT;
">>"                            return SHIFT_RIGHT;

"&&"                            return BOOL_OP_AND;
"||"                            return BOOL_OP_OR;
%%
