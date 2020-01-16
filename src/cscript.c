#define _GNU_SOURCE
#include <string.h>
#include "evaluate.h"
#include "cscriptparser.h"
#include "gc.h"

extern FILE *cscriptin;
extern const char* cscript_current_file;

tree cscript_parse_head;
GC_STATIC_TREE(cscript_parse_head);

/*
 * Parser's error callback.
 */
void cscripterror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", cscript_current_file,
            cscriptlloc.first_line, str);

    exit(1);
}

static int parse_cscript(const char *fname)
{
    int parse_result;
    FILE *f;
    char *command;

    asprintf(&command, "gcc -x c -E \"%s\"", fname);

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

    cscriptin = f;

    inhibit_gc();
    parse_result = cscriptparse();
    enable_gc();

    fclose(f);

    return parse_result;
}

static void create_arguments(int argc, char *argv[])
{
    tree chain;
    /* int argc = n
     *
     * Where n is the number of arguments.
     */
    tree argc_decl = tree_make(T_DECL);
    tree argc_assign = tree_make(T_ASSIGN);

    tASSIGN_LHS(argc_assign) = get_identifier("argc");
    tASSIGN_RHS(argc_assign) = get_integer(argc);

    tDECL_SPECS(argc_decl) = tree_chain_head(tree_make(T_INT_TS));
    tDECL_DECLS(argc_decl) = argc_assign;

    chain = tree_chain_head(argc_decl);

    /* char *argv[n]; */
    tree argv_decl = tree_make(T_DECL);
    tree array = tree_make(T_ARRAY);
    tree ptr = tree_make(T_POINTER);

    tDECL_SPECS(argv_decl) = tree_chain_head(tree_make(T_CHAR_TS));

    tARRAY_SZ(array) = get_integer(argc);
    tARRAY_DECL(array) = get_identifier("argv");
    tPTR_EXP(ptr) = array;
    tDECL_DECLS(argv_decl) = ptr;

    tree_chain(argv_decl, chain);

    /* for (int i = 0; i < n; i++)
     *    argv[i] = <char * address>;
     */
    for (int i = 0; i < argc; i++)
    {
        tree assign = tree_make(T_ASSIGN);
        tree array_access = tree_make(T_ARRAY_ACCESS);
        tARR_ACCESS_IDX(array_access) = get_integer(i);
        tARR_ACCESS_OBJ(array_access) = get_identifier("argv");
        tASSIGN_LHS(assign) = array_access;
        tASSIGN_RHS(assign) = get_addr(argv[i]);
        tree_chain(assign, chain);
    }

    evaluate(chain, "<COMPOSE ARGS>");
}

int evaluate_cscript(const char *script_name,
                     int argc,
                     char *argv[])
{
  tree return_val;

  cscript_current_file = script_name;
  if (parse_cscript(script_name))
      return 254;

  create_arguments(argc, argv);

  return_val = evaluate(cscript_parse_head, script_name);

  if (!return_val)
    return 0;

  return get_c_main_return_value(return_val);
}
