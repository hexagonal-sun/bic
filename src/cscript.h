#ifndef __CSCRIPT_H_
#define __CSCRIPT_H_
#include "tree.h"
#include "tree-dump-primitives.h"

void cscript_exit(tree result);

int evaluate_cscript(const char *file,
                     bool dump_ast,
                     enum DUMP_TYPE dump_type,
                     int argc,
                     char *argv[]);
#endif
