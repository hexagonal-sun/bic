#include <stdio.h>
#include <stdlib.h>

#include "../evaluate.h"
#include "ptr_call.h"

struct ptr_map {
    void *entry_addr;
    tree fndef;
    struct ptr_map *next;
};

void *get_entry_point_for_fn(tree fndef)
{
    return NULL;
}

ptrdiff_t handle_ptr_call(struct argregs *regs)
{
    return 0;
}
