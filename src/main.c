#include "config.h"
#include "tree.h"
#include "cfileparser.h"
#include "cfilelex.h"
#include "typename.h"
#include "evaluate.h"
#include "repl.h"
#include <gc.h>
#include <stdio.h>

extern FILE* cfilein;
extern int cfileparse();

tree parse_head;

static void * gmp_realloc_stub(void *ptr, size_t old, size_t new)
{
    return GC_realloc(ptr, new);
}

static void gmp_free_stub(void *ptr, size_t sz)
{
    GC_free(ptr);
}

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
    int i;

    GC_INIT();
    mp_set_memory_functions(&GC_malloc, &gmp_realloc_stub, &gmp_free_stub);

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
