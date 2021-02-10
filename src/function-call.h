#pragma once
#include <stddef.h>
#include "tree.h"

union function_return
{
    ptrdiff_t ival;
    double dval;
};

void do_ext_call(void *function_address,
                 tree args,
                 tree ret_lv,
                 int *variadic_pos);

void *get_entry_point_for_fn(tree fndef);
