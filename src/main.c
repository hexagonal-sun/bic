#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include "config.h"
#include "tree.h"
#include "typename.h"
#include "evaluate.h"
#include "repl.h"
#include <stdio.h>
#include "util.h"
#include "gc.h"
#include "preprocess.h"
#include "tree-dump-dot.h"
#include "tree-dump.h"
#include "tree-dump-primitives.h"
#include "cscript.h"

#if defined(BUILD_LINUX)
 #include <gnu/lib-names.h>
#endif

enum DUMP_TYPE dump_type = TEXTUAL;

static int flag_print_ast = 0;
static const char *cscript_file = NULL;

static void add_call_to_main(tree head)
{
    tree main_fncall = tree_make(T_FN_CALL);

    tFNCALL_ID(main_fncall) = get_identifier(strdup("main"));
    tFNCALL_ARGS(main_fncall) = NULL;

    tree_chain(main_fncall, head);
}

static void usage(char *progname)
{
    fprintf(stderr,
            "Usage: %s [-v] [-T] [-D] [-I INCLUDE_DIR]... [-l library]... [-s CFILE/SCRPIT] [ARGS]\n"
            "\n"
            "Arguments:\n"
            "  -l: Open the specified library for external symbol\n"
            "      resolution in the evaluator.\n"
            "\n"
            "  -I: add INCLUDE_DIR to the header search path.  This allows the user to use\n"
            "      #include directives to include a file that isn't in the default header\n"
            "      search path.\n"
            "\n"
            "  -s: execute SCRIPT/CFILE. All parameters after this are passed as\n"
            "      arguments to the top-level code or main() via the normal argv argc\n"
            "      mechanism.  Arguments that are also valid bic are passed also, so ensure\n"
            "      any arguments to bic are passed *before* this one."
            "\n"
            "  -v: Print version information and exit.\n"
            "\n"
            "  -T: after parsing CFILE/SCRIPT, dump the AST and exit.\n"
            "\n"
            "  -D: output dumps in the `dot` format for use with graphviz.\n"
            "\n"
            "Note: if no -s is passed on the command line theREPL (Read, Eval, Print Loop)\n"
            "      is started\n", progname);
}

static void print_version(char *progname)
{
    fprintf(stderr,
            "This is " PACKAGE_STRING ".\n"
            "\n"
            "Enabled Features:\n"
            "\n"
            "   tree-check: "
#ifdef ENABLE_TREE_CHECK
            "yes\n"
#else
            "no\n"
#endif
        );
}

static int open_library(char *libname)
{
    char *tmp, *full_library_name;
    int ret;

#if defined(BUILD_LINUX)
    if (!strcmp(libname, "m"))
        libname = LIBM_SO;
#endif

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

    while ((opt = getopt(argc, argv, "vTDl:s:I:")) != -1) {
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
        case 'v':
            print_version(argv[0]);
            exit(EXIT_SUCCESS);
        case 'T':
            flag_print_ast = 1;
            break;
        case 's':
            cscript_file = strdup(optarg);
            /* Once we have seen the script parameter, don't process any more
             * options. This allows the cscript to be given it's own options. */
            return optind;
        case 'D':
            dump_type = DOT;
            break;
        default: /* '?' */
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return optind;
}

extern gc_obj *top_of_stack;

int main(int argc, char *argv[])
{
    gc_obj top;
    top_of_stack = &top;

    typename_init();
    eval_init();

    int opt_idx = parse_args(argc, argv);

    if (cscript_file) {
        /* Decrement the option index to include the script name as a
         * parameter. */
        opt_idx--;
        return evaluate_cscript(cscript_file, argc - opt_idx, &argv[opt_idx]);
    }

    bic_repl();
}
