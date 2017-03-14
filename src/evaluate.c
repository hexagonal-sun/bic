#include <stdio.h>
#include <stdlib.h>

#include "evaluate.h"

static eval_ctx toplevel_ctx = {
    .parent = NULL,
    .name = "top-level",
    .id_map = {
        .mappings = LIST_INIT(toplevel_ctx.id_map.mappings)
    }
};

static tree __evaluate_1(tree t, int depth);
static tree __evaluate(tree t, int depth);

static eval_ctx *cur_ctx = &toplevel_ctx;

/* Context management functions. */
static void pop_ctx(void)
{
    if (!cur_ctx->parent) {
        /* We attempted to pop off the top-level context.  This is a
         * serious error. */
        fprintf(stderr, "Error: attempted to pop top-level context\n");
        exit(EXIT_FAILURE);
    }

    cur_ctx = cur_ctx->parent;
}

static void push_ctx(const char *name)
{
    eval_ctx *new_ctx = malloc(sizeof(*new_ctx));

    if (!new_ctx) {
        perror("Error creating new context");
        exit(EXIT_FAILURE);
    }

    new_ctx->parent = cur_ctx;
    new_ctx->name = name;

    INIT_LIST(&new_ctx->id_map.mappings);

    cur_ctx = new_ctx;
}

static void __ctx_backtrace(eval_ctx *ctx, int depth)
{
    if (ctx->parent)
        __ctx_backtrace(ctx->parent, depth + 1);

    fprintf(stderr, "%2d: %s\n", depth, ctx->name);
}

static void ctx_backtrace(void)
{
    fprintf(stderr, "Evaluator Backtrace\n===================\n");
    __ctx_backtrace(cur_ctx, 0);
}

static tree resolve_identifier(struct identifier *id,
                               eval_ctx *ctx)
{
    identifier_mapping *i;

    list_for_each(i, &ctx->id_map.mappings, mappings) {
        if (i->id == id)
            return i->t;
    }

    return NULL;
}

static void map_identifier(struct identifier *id, tree t)
{
    identifier_mapping *new_map;

    if (resolve_identifier(id, cur_ctx)) {
        fprintf(stderr, "Error: attempted to map already existing identifier %s. Stopping.\n",
                id->name);
        ctx_backtrace();
        exit(EXIT_FAILURE);
    }

    new_map = malloc(sizeof(*new_map));

    new_map->id = id;
    new_map->t = t;

    list_add(&new_map->mappings, &cur_ctx->id_map.mappings);
}

static tree eval_identifier(tree t, int depth)
{
    /* Search through the identifier mappings of the current context
     * stack to find the appropriate object. */
    eval_ctx *search_ctx = cur_ctx;
    struct identifier *id = t->data.id;

    while (search_ctx) {
        tree search_result = resolve_identifier(id, search_ctx);

        if (search_result)
            return search_result;

        search_ctx = search_ctx->parent;
    }

    fprintf(stderr, "Error: could not resolve identifier %s. Stopping.\n",
            id->name);

    ctx_backtrace();

    exit(EXIT_FAILURE);
}

static tree eval_fn_call(tree t, int depth)
{
    /* There are three possibilities here:
     *
     * 1: The function call is to a /defined/ function, i.e. one that
     *    has been parsed and has a corresponding T_FN_DEF tree.  If
     *    that is the case, evaluate all arguments and evaluate the
     *    function body.
     *
     * 2: The function call is an external symbol, i.e. the function
     *    should have been declared and should have a corresponding
     *    T_FN_DECL.  If that is the case, evaluate the arguments and
     *    use the symbolic linker to find the function.
     *
     * 3: The function couldn't be found.  In that case, error.
     */
    tree function = __evaluate_1(t->data.fncall.identifier, depth + 1);

    if (is_T_FN_DEF(function)) {
        push_ctx(function->data.function.id->name);

        /* TODO: argument evaluation. */
        __evaluate(function->data.function.stmts, depth + 1);
    }
}

static tree eval_fn_def(tree t, int depth)
{
    map_identifier(t->data.function.id, t);
}

static tree __evaluate_1(tree t, int depth)
{
    tree result = NULL;

    if (!t)
        return result;

    switch (t->type)
    {
    case T_IDENTIFIER: result = eval_identifier(t, depth + 1); break;
    case T_FN_CALL:    result = eval_fn_call(t, depth + 1);    break;
    case T_FN_DEF:     result = eval_fn_def(t, depth + 1);     break;
    default:           result = NULL;                          break;
    }

    return result;
}

static tree __evaluate(tree head, int depth)
{
    tree result, i;

    if (!head)
        return NULL;

    for_each_tree(i, head)
        result = __evaluate_1(i, depth);

    return result;
}

void evaluate(tree t)
{
    __evaluate(t, 0);
}
