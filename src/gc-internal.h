/*
 * gc-internal.h - Declare the mark_object function.
 */

#ifndef _GC_INTERNAL_H_
#define _GC_INTERNAL_H_

#include "gc.h"

void mark_tree(tree t);
void mark_object(gc_obj obj);

#endif
