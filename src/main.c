#include "config.h"
#include "tree.h"
#include "cfileparser.h"
#include "cfilelex.h"
#include "typename.h"
#include "evaluate.h"
#include "gc.h"
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


extern FILE* cfilein;
extern int cfileparse();

tree parse_head;
GC_TREE_DECL(parse_head);

/*
 * Parser's error callback.
 */
void cfileerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", "<stdin>", cfilelloc.first_line, str);
    exit(1);
}

static void bic_repl(void)
{
    char *line;

    line = readline(BIC_PROMPT);
    while (line) {
        int parse_result;

        YY_BUFFER_STATE buffer = cfile_scan_string(line);
        parse_result = cfileparse();
        cfile_delete_buffer(buffer);

        if (!parse_result)
            tree_dump(parse_head);

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

    cfilein = f;

    parse_result = cfileparse();

    fclose(f);

    return parse_result;
}

static void add_call_to_main(tree head)
{
    tree main_fncall = tree_make(T_FN_CALL);

    tFNCALL_ID(main_fncall) = get_identifier("main");
    tFNCALL_ARGS(main_fncall) = NULL;

    tree_chain(main_fncall, head);
}

int main(int argc, char *argv[])
{
    tree top;
    int i;

    top_of_stack = &top;

    typename_init();
    eval_init();

    if (argc == 1)
        bic_repl();
    else
        for (i = 1; i < argc; i++)
            if (!parse_file(argv[i])) {
                add_call_to_main(parse_head);
                evaluate(parse_head, argv[i]);
            }
}
