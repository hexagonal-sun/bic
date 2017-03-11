#include "config.h"
#include "tree.h"
#include "parser.h"
#include "lex.h"
#include "identifier.h"
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

static void bic_repl(void)
{
    char *line;

    line = readline(BIC_PROMPT);
    while (line) {
        int parse_result;

        YY_BUFFER_STATE buffer = yy_scan_string(line);
        parse_result = yyparse();
        yy_delete_buffer(buffer);

        if (!parse_result)
            tree_dump(parse_head, 0);

        line = readline(BIC_PROMPT);
    }
}

static int parse_file(char *fname)
{
    int parse_result;
    FILE *f = fopen(fname, "r");

    if (!f) {
        perror("Error: Could not open file");
        return 1;
    }

    yyin = f;

    parse_result = yyparse();

    fclose(f);

    return parse_result;
}


int main(int argc, char *argv[])
{
    int i;

    identifier_init();

    if (argc == 1)
        bic_repl();
    else
        for (i = 1; i < argc; i++)
            if (!parse_file(argv[i]))
                tree_dump(parse_head, 0);
}
