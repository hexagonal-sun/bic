#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "evaluate.h"
#include "gc.h"

static tree cur_ctx = NULL;
GC_TREE_DECL(cur_ctx);

static tree __evaluate_1(tree t, int depth);
static tree __evaluate(tree t, int depth);

/* Context management functions. */
static void pop_ctx(void)
{
    if (!cur_ctx->data.ectx.parent_ctx) {
        /* We attempted to pop off the top-level context.  This is a
         * serious error. */
        fprintf(stderr, "Error: attempted to pop top-level context\n");
        exit(EXIT_FAILURE);
    }

    cur_ctx = cur_ctx->data.ectx.parent_ctx;
}

static void push_ctx(const char *name)
{
    tree new_ctx = tree_make(E_CTX);

    if (!new_ctx) {
        perror("Error creating new context");
        exit(EXIT_FAILURE);
    }

    new_ctx->data.ectx.parent_ctx = cur_ctx;
    new_ctx->data.ectx.name = name;

    INIT_LIST(&new_ctx->data.ectx.id_map.mappings);

    cur_ctx = new_ctx;
}

static void __ctx_backtrace(tree ctx, int depth)
{
    if (ctx->data.ectx.parent_ctx)
        __ctx_backtrace(ctx->data.ectx.parent_ctx, depth + 1);

    fprintf(stderr, "%2d: %s\n", depth, ctx->data.ectx.name);
}

static void ctx_backtrace(void)
{
    fprintf(stderr, "Evaluator Backtrace\n===================\n");
    __ctx_backtrace(cur_ctx, 0);
}

static void eval_die(const char *format, ...)
{
    ctx_backtrace();

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static tree resolve_identifier(tree id, tree ctx)
{
    identifier_mapping *i;

    list_for_each(i, &ctx->data.ectx.id_map.mappings, mappings) {
        if (strcmp(i->id->data.id.name, id->data.id.name) == 0)
            return i->t;
    }

    return NULL;
}

static void map_identifier(tree id, tree t)
{
    identifier_mapping *new_map;

    if (resolve_identifier(id, cur_ctx))
        eval_die("Error: attempted to map already existing identifier %s. Stopping.\n",
                id->data.id.name);

    new_map = malloc(sizeof(*new_map));

    new_map->id = id;
    new_map->t = t;

    list_add(&new_map->mappings, &cur_ctx->data.ectx.id_map.mappings);
}

static tree eval_identifier(tree t, int depth)
{
    /* Search through the identifier mappings of the current context
     * stack to find the appropriate object. */
    tree search_ctx = cur_ctx;

    while (search_ctx) {
        tree search_result = resolve_identifier(t, search_ctx);

        if (search_result)
            return search_result;

        search_ctx = search_ctx->data.ectx.parent_ctx;
    }

    eval_die("Error: could not resolve identifier %s. Stopping.\n",
            t->data.id.name);
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
        push_ctx(function->data.function.id->data.id.name);

        /* TODO: argument evaluation. */
        __evaluate(function->data.function.stmts, depth + 1);
    }
}

static tree eval_fn_def(tree t, int depth)
{
    map_identifier(t->data.function.id, t);
}

static void make_and_map_live_var(tree id, tree type)
{
    tree live_var = tree_make(T_LIVE_VAR);

    assert(id->type == T_IDENTIFIER);

    live_var->data.var.type = type;

    map_identifier(id, live_var);
}

static tree eval_decl(tree t, int depth)
{
    tree type = __evaluate_1(t->data.decl.type, depth + 1),
        decls = t->data.decl.decls,
        i;

    for_each_tree(i, decls)
        make_and_map_live_var(i, type);

    return NULL;
}

static tree assign_integer(tree var, tree integer)
{
    signed long int val = mpz_get_si(integer->data.integer);
    switch (var->data.var.type->type) {
        #define DEFCTYPE(TNAME, DESC, CTYPE)             \
            case TNAME:                                  \
                var->data.var.val.TNAME = (CTYPE)val;    \
                break;
        #include "ctypes.def"
        #undef DEFCTYPE
    default:
        eval_die("Error: could not assign to non-integer type");
    }
}

static tree eval_assign(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);

    /* Ensure we have a valid lvalue. */
    if (!is_T_LIVE_VAR(left))
        eval_die("Error: not a valid lvalue.\n");

    switch (right->type) {
    case T_INTEGER:
        assign_integer(left, right);
        break;
    }
}

/* All types evaluate to themselves. */
#define DEFCTYPE(TNAME, DESC, CTYPE)            \
    static tree eval_##TNAME(tree t, int depth) \
    {                                           \
        return t;                               \
    }
#include "ctypes.def"
#undef DEFCTYPE

static tree eval_integer(tree t, int depth)
{
    /* An integer evaluates to itself. */
    return t;
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
    case T_DECL:       result = eval_decl(t, depth + 1);       break;
    case T_ASSIGN:     result = eval_assign(t, depth + 1);     break;
    case T_INTEGER:    result = eval_integer(t, depth + 1);    break;
#define DEFCTYPE(TNAME, DESC, CTYPE)                                    \
    case TNAME:        result = eval_##TNAME(t, depth + 1);    break;
#include "ctypes.def"
#undef DEFCTYPE
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

void eval_init(void)
{
    push_ctx("Toplevel");
}
