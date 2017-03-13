#pragma once

#include "identifier.h"
#include "tree.h"
#include "list.h"

typedef struct {
    struct identifier *id;
    tree t;
    list mappings;
} identifier_mapping;

typedef struct eval_ctx {
    identifier_mapping id_map;
    struct eval_ctx *parent;
    const char *name;
} eval_ctx;

void evaluate(tree t);
