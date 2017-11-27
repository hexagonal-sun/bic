#include "config.h"
#include "tree.h"
#include "cfileparser.h"
#include "cfilelex.h"
#include "typename.h"
#include "evaluate.h"
#include "repl.h"
#include <stdio.h>
#include "gc.h"

extern FILE* cfilein;
extern int cfileparse();

tree cfile_parse_head;
GC_STATIC_TREE(cfile_parse_head);

/*
 * Parser's error callback.
 */
void cfileerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", "<stdin>", cfilelloc.first_line, str);
    exit(1);
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

    inhibit_gc();
    parse_result = cfileparse();
    enable_gc();

    fclose(f);

    return parse_result;
}

static void add_call_to_main(tree head)
{
    tree main_fncall = tree_make(T_FN_CALL);

    tFNCALL_ID(main_fncall) = get_identifier(strdup("main"));
    tFNCALL_ARGS(main_fncall) = NULL;

    tree_chain(main_fncall, head);
}

extern gc_obj *top_of_stack;

int main(int argc, char *argv[])
{
    int i;
    gc_obj top;
    top_of_stack = &top;

    typename_init();
    eval_init();

    if (argc == 1)
        bic_repl();
    else
        for (i = 1; i < argc; i++)
            if (!parse_file(argv[i])) {
                add_call_to_main(cfile_parse_head);
                evaluate(cfile_parse_head, argv[i]);
            }
}
