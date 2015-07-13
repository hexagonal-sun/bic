#include "parser.h"
#include <stdio.h>

extern FILE* yyin;

/*
 * Parser's error callback.
 */
void yyerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", "<stdin>", yylloc.first_line, str);
}

int main()
{
    yyin = stdin;
    yyparse();
}
