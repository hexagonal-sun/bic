#include "config.h"
#include "tree.h"
#include "parser.h"
#include "lex.h"
#include <stdio.h>
#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
#error "No readline found"
#endif /* HAVE_LIBREADLINE */


extern FILE* yyin;

tree parse_head;

/*
 * Parser's error callback.
 */
void yyerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", "<stdin>", yylloc.first_line, str);
}

int main()
{
    char *line = readline(BIC_PROMPT);
    while (line) {
        YY_BUFFER_STATE buffer = yy_scan_string(line);
        yyparse();
        yy_delete_buffer(buffer);
        tree_dump(parse_head);
        line = readline(BIC_PROMPT);
    }
}
