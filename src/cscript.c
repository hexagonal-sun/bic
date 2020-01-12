#define _GNU_SOURCE
#include <string.h>
#include "evaluate.h"
#include "cscriptparser.h"
#include "gc.h"

extern FILE *cscriptin;
static const char* cscript_file;

tree cscript_parse_head;
GC_STATIC_TREE(cscript_parse_head);

/*
 * Parser's error callback.
 */
void cscripterror(const char *str)
{
    fprintf(stderr, "Parser Error: %s:%d %s.\n", cscript_file,
            cscriptlloc.first_line, str);

    exit(1);
}

static int parse_cscript(const char *fname)
{
    int parse_result;
    FILE *f;
    char *command;

    asprintf(&command, "cpp -E \"%s\"", fname);

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

int evaluate_cscript(const char *script_name,
                     int argc,
                     const char *argv[],
                     int idx)
{
  tree return_val;

  if (parse_cscript(script_name))
      return 254;

  return_val = evaluate(cscript_parse_head, script_name);

  if (!return_val)
    return 0;

  return get_c_main_return_value(return_val);
}
