#pragma once

#include "../tree.h"

struct argregs
{
    ptrdiff_t iarg[6];
};

ptrdiff_t handle_ptr_call(struct argregs *regs, void *pc);
void *get_entry_point_for_fn(tree fndef);
