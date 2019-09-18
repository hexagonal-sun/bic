#include "../tree.h"

enum argument_dest {
    CORE_REG = 0,
    VECTOR_REG = 1,
    STACKED = 2
};

union function_return {
    ptrdiff_t ival;
    double dval;
};

/* Ensure no structure packing occurs, since we'll be accessing the
 * structure from assembly. */
struct arg {
    struct arg *next;
    union function_return val;
    enum argument_dest dest;
} __attribute__((packed)) ;


union function_return do_call(void *function_address, tree args, tree ret_type,
                              bool is_variadic);
