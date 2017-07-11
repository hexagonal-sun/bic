#include "../tree.h"

enum argument_class {
    CLS_INTEGER = 0,
    CLS_SSE,
    CLS_SSEUP,
    CLS_X87,
    CLS_X87UP,
    CLS_COMPLEX_X87,
    CLS_NONE,
    CLS_MEMORY,
};

/* Ensure no structure packing occurs, since we'll be accessing the
 * structure from assembly. */
struct arg {
    struct arg *next;
    union {
        ptrdiff_t i;
        double d;
    } val;
    enum argument_class dest;
} __attribute__((packed)) ;

ptrdiff_t do_call(void *function_address, tree args);
