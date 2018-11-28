#include <dlfcn.h>
#include "config.h"
#include "tree.h"
#include "cfileparser.h"
#include "cfilelex.h"
#include "typename.h"
#include "evaluate.h"
#include "repl.h"
#include <stdio.h>
#include "util.h"
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

static int parse_file(const char *fname)
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

static void usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-l library]... [--] [CFILE]\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "  -l: Open the specified library for external symbol\n");
    fprintf(stderr, "      resolution in the evaluator.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  CFILE: Parse, evaluate and call a function called\n");
    fprintf(stderr, "         `main' within the file.");
    fprintf(stderr, "\n");
    fprintf(stderr, "Note: if no CFILE is passed on the command line the\n");
    fprintf(stderr, "REPL (Read, Eval, Print Loop) is started\n");
}

static int open_library(char *libname)
{
    char *tmp, *full_library_name;
    int ret;

    /* First, attempt to open just the library, if it has been fully
     * specified. */
    if (dlopen(libname, RTLD_NOW | RTLD_GLOBAL))
        return 1;

    /* Append ".so" and prepend "lib" onto `libname' so that we accept
     * library names as GCC does. */
    tmp = concat_strings("lib", libname);
    full_library_name = concat_strings(tmp, ".so");

    if(dlopen(full_library_name, RTLD_NOW | RTLD_GLOBAL))
        ret = 1;
    else
        ret = 0;

    free(tmp);
    free(full_library_name);

    return ret;
}

static int parse_args(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "l:I:")) != -1) {
        switch (opt) {
        case 'l':
            if (!open_library(optarg)) {
                fprintf(stderr, "Error: could not open library %s: %s\n",
                        optarg, dlerror());

                exit(EXIT_FAILURE);
            }
            break;

        case 'I':
            preprocessor_add_include_dir(optarg);
            break;
        default: /* '?' */
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return optind;
}

static int bic_eval_file(const char *file)
{
    tree return_val;

    if (parse_file(file))
        return 1;

    add_call_to_main(cfile_parse_head);
    return_val = evaluate(cfile_parse_head, file);

    if (!return_val)
        return 0;

    return get_c_main_return_value(return_val);
}

extern gc_obj *top_of_stack;

int main(int argc, char *argv[])
{
    gc_obj top;
    top_of_stack = &top;

    typename_init();
    eval_init();

    int idx = parse_args(argc, argv);

    if (idx == argc)
        bic_repl();
    else
        return bic_eval_file(argv[idx]);
}
