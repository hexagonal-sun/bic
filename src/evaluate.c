#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>

#include "evaluate.h"
#include "function_call.h"
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

static void ctx_backtrace(void)
{
    fprintf(stderr, "Evaluator Backtrace\n===================\n");
    tree_dump(cur_ctx);
}

static void __attribute__((noreturn)) eval_die(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    ctx_backtrace();

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
     *    T_DECL_FN.  If that is the case, evaluate the arguments and
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

    if (is_T_DECL_FN(function)) {
        tree arg, fn_arg_chain = NULL, args = t->data.fncall.arguments;
        char *function_name = function->data.function.id->data.id.name;
        push_ctx(function_name);

        void *function_address = dlsym(RTLD_DEFAULT, function_name);

        if (function_address == NULL)
            eval_die("Error: could not resolve external symbol: %s\n", function_name);

        /* Evaluate all arguments before passing into the marshalling
         * function. */
        fn_arg_chain = tree_make(CHAIN_HEAD);
        for_each_tree(arg, args) {
            tree fn_arg = tree_make(T_FN_ARG);
            fn_arg->data.exp = __evaluate_1(arg, depth + 1);
            tree_chain(fn_arg, fn_arg_chain);
        }

        do_call(function_address, fn_arg_chain);
    }

    return NULL;
}

static tree eval_fn_def(tree t, int depth)
{
    map_identifier(t->data.function.id, t);

    return NULL;
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
        switch (i->type) {
        case T_POINTER:
        {
            tree ptr_type = tree_make(D_T_PTR);
            ptr_type->data.ptr_type.type = type;
            make_and_map_live_var(i->data.ptr.id, ptr_type);
            break;
        }
        case T_IDENTIFIER:
            make_and_map_live_var(i, type);
            break;
        case T_DECL_FN:
            i->data.function.return_type = type;
            map_identifier(i->data.function.id, i);
            break;
        default:
            eval_die("Error: unknown rvalue in declaration.\n");
        }

    return NULL;
}

static void assign_integer(tree var, tree right)
{
    signed long int val;

    /* Obtain the value with which to assign. */
    switch (right->type)
    {
    case T_INTEGER:
         val = mpz_get_si(right->data.integer);
         break;
    default:
        eval_die("Error: unknown rvalue assignment to integer.\n");
    }

    switch (var->data.var.type->type) {
        #define DEFCTYPE(TNAME, DESC, CTYPE, FMT)        \
            case TNAME:                                  \
                var->data.var.val.TNAME = (CTYPE)val;    \
                break;
        #include "ctypes.def"
        #undef DEFCTYPE
    default:
        eval_die("Error: could not assign to non-integer type");
    }
}

static void assign_ptr(tree var, tree right)
{
    void *ptr;

    switch (right->type)
    {
    case T_STRING:
        ptr = right->data.string;
        break;
    default:
        eval_die("Error: could not assign to non-pointer type");
    }

    var->data.var.val.D_T_PTR = ptr;
}

static tree eval_assign(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);

    /* Ensure we have a valid lvalue. */
    if (!is_T_LIVE_VAR(left))
        eval_die("Error: not a valid lvalue.\n");

    switch (left->data.var.type->type) {
    case D_T_CHAR ... D_T_ULONGLONG:
        assign_integer(left, right);
        break;
    case D_T_PTR:
        assign_ptr(left, right);
        break;
    default:
        eval_die("Error: unknown assignment rvalue type.");
    }

    return NULL;
}

static tree make_int_from_live_var(tree var)
{
    tree ret = tree_make(T_INTEGER);
    tree type = var->data.var.type;

    switch (type->type) {
    case D_T_CHAR ... D_T_LONGLONG:
        /* Signed Types */
        mpz_init_set_si(ret->data.integer, var->data.var.val.D_T_LONGLONG);
        break;
    case D_T_UCHAR ... D_T_ULONGLONG:
        mpz_init_set_ui(ret->data.integer, var->data.var.val.D_T_ULONGLONG);
        break;
    default:
        eval_die("error: could not create integer type from live var.");
    }

    return ret;
}

static void live_var_add(tree var, unsigned long int val)
{
    tree type = var->data.var.type;
    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)               \
        case TNAME:                                     \
            var->data.var.val.TNAME += val;             \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die("Error: Could not add to unknown live var type.");
    }
}

static void live_var_sub(tree var, unsigned long int val)
{
    tree type = var->data.var.type;
    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)               \
        case TNAME:                                     \
            var->data.var.val.TNAME -= val;             \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die("Error: Could not subtract to unknown live var type.");
    }
}

static tree eval_post_inc(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die("Error: not a valid lvalue.\n");

    ret = make_int_from_live_var(exp);

    live_var_add(exp, 1);

    return ret;
}

static tree eval_post_dec(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die("Error: not a valid lvalue.\n");

    ret = make_int_from_live_var(exp);
    live_var_sub(exp, 1);
    return ret;
}

static tree eval_add(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);

    tree ret = tree_make(T_INTEGER);
    mpz_init(ret->data.integer);

    /* Resolve all identifiers. */
    if (is_T_LIVE_VAR(left))
        left = make_int_from_live_var(left);

    if (is_T_LIVE_VAR(right))
        right = make_int_from_live_var(right);

    if (!(is_T_INTEGER(left) && is_T_INTEGER(right)))
        eval_die("Error: could not add to non integer type\n");

    mpz_add(ret->data.integer, left->data.integer,
            right->data.integer);

    return ret;
}

static tree eval_sub(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);


    tree ret = tree_make(T_INTEGER);
    mpz_init(ret->data.integer);

    if (is_T_LIVE_VAR(left))
        left = make_int_from_live_var(left);

    if (is_T_LIVE_VAR(right))
        right = make_int_from_live_var(right);

    if (!(is_T_INTEGER(left) && is_T_INTEGER(right)))
        eval_die("Error: could not subtract to non integer type\n");

    mpz_sub(ret->data.integer, left->data.integer,
            right->data.integer);

    return ret;
}

static tree eval_mul(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);


    tree ret = tree_make(T_INTEGER);
    mpz_init(ret->data.integer);

    if (is_T_LIVE_VAR(left))
        left = make_int_from_live_var(left);

    if (is_T_LIVE_VAR(right))
        right = make_int_from_live_var(right);

    if (!(is_T_INTEGER(left) && is_T_INTEGER(right)))
        eval_die("Error: could not subtract to non integer type\n");

    mpz_mul(ret->data.integer, left->data.integer,
            right->data.integer);

    return ret;
}

static tree eval_div(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);


    tree ret = tree_make(T_INTEGER);
    mpz_init(ret->data.integer);

    if (is_T_LIVE_VAR(left))
        left = make_int_from_live_var(left);

    if (is_T_LIVE_VAR(right))
        right = make_int_from_live_var(right);

    if (!(is_T_INTEGER(left) && is_T_INTEGER(right)))
        eval_die("Error: could not subtract to non integer type\n");

    mpz_div(ret->data.integer, left->data.integer,
            right->data.integer);

    return ret;
}

/* All types evaluate to themselves. */
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)       \
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

static tree eval_string(tree t, int depth)
{
    /* A string evaluates to itself. */
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
    case T_STRING:     result = eval_string(t, depth + 1);     break;
    case T_P_INC:      result = eval_post_inc(t, depth + 1);   break;
    case T_P_DEC:      result = eval_post_dec(t, depth + 1);   break;
    case T_ADD:        result = eval_add(t, depth + 1);        break;
    case T_SUB:        result = eval_sub(t, depth + 1);        break;
    case T_MUL:        result = eval_mul(t, depth + 1);        break;
    case T_DIV:        result = eval_div(t, depth + 1);        break;
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)                               \
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
