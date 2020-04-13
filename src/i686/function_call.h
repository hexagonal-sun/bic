#pragma once
#include <stddef.h>
#include "../tree.h"

struct arg {
    struct arg *next;
    union {
        ptrdiff_t i;
        float f;
    }val;
};

union function_return
{
    ptrdiff_t ival;
    double dval;
};

union function_return do_call(void *function_address, tree args, tree ret_type,
                              bool is_variadic);

extern ptrdiff_t __do_call(void *fn, struct arg *args);
