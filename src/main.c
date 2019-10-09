#define _GNU_SOURCE
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
#include "preprocess.h"

#if defined(BUILD_LINUX)
 #include <gnu/lib-names.h>
#endif

extern FILE* cfilein;
extern int cfileparse();
extern void cfile_parser_set_file(const char *fname);
extern const char *parser_current_file;

tree cfile_parse_head;
GC_STATIC_TREE(cfile_parse_head);

/*
 * Parser's error callback.
 */
void cfileerror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", parser_current_file,
            cfilelloc.first_line, str);

    exit(1);
}

static int parse_file(const char *fname)
{
    int parse_result;
    FILE *f;
    char *command;

    asprintf(&command, "gcc -E \"%s\"", fname);

    if (!command) {
        fprintf(stderr, "Error: could not allocate preprocessor command.\n");
        return 1;
    }

    f = popen(command, "r");
    free(command);

    if (!f) {
        perror("Error: Could not open file");
        return 1;
    }

    cfilein = f;

    inhibit_gc();
    cfile_parser_set_file(fname);
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
    fprintf(stderr,
            "Usage: %s [-v] [-I INCLUDE_DIR]... [-l library]... [--] [CFILE]\n"
            "\n"
            "Arguments:\n"
            "  -l: Open the specified library for external symbol\n"
            "      resolution in the evaluator.\n"
            "\n"
            "  -I: add INCLUDE_DIR to the header search path.  This allows the user to use\n"
            "      #include directives to include a file that isn't in the default header\n"
            "      search path.\n"
            "\n"
            "  -v: Print version information and exit.\n"
            "\n"
            "  CFILE: Parse, evaluate and call a function called\n"
            "         `main' within the file."
            "\n"
            "Note: if no CFILE is passed on the command line the\n"
            "REPL (Read, Eval, Print Loop) is started\n", progname);
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

    while ((opt = getopt(argc, argv, "vl:I:")) != -1) {
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
