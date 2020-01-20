#ifndef __CSCRIPT_H_
#define __CSCRIPT_H_
#include "tree.h"

void cscript_exit(tree result);

int evaluate_cscript(const char *file,
                     int argc,
                     char *argv[]);
#endif
