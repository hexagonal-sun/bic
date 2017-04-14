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

static tree make_fncall_result(tree type, ptrdiff_t result)
{
    tree ret = tree_make(T_LIVE_VAR);

    ret->data.var.type = type;

    switch (type->type)
    {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)             \
        case TNAME:                                   \
            ret->data.var.val.TNAME = (CTYPE)result;  \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die("Error: could not create function return value\n");
    }

    return ret;
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
        tree arg_decls = function->data.function.arguments,
            arg_vals = t->data.fncall.arguments,
            arg_decl, arg_val, evaled_arg_vals;

        push_ctx(function->data.function.id->data.id.name);

        if (arg_decls) {
            size_t no_decls = 0, no_vals = 0;
            /* Ensure that the number of parameters passed to the
             * function matches the number in the function
             * declaration. */
            for_each_tree(arg_decl, arg_decls)
                no_decls++;

            for_each_tree(arg_val, arg_vals)
                no_vals++;

            if (no_vals != no_decls)
                eval_die("Error: Invalid number of parameters to function");

            /* Evaluate an assignment for each passed value. */
            arg_val = arg_vals;

            for_each_tree(arg_decl, arg_decls) {
                tree eval_arg_val,
                    decl_identifier;

                arg_val = list_entry(arg_val->chain.next,
                                     typeof(*arg_val), chain);

                /* We need to evaluate the argument value before
                 * adding the declaration on to the new context.  This
                 * prevents bugs with things such as:
                 *
                 * 0: int foo(int a)
                 * 1: {
                 * 2:      use(a);
                 * 3: }
                 * 4:
                 * 5: int main()
                 * 6: {
                 * 7:     int a = 10;
                 * 8:     foo(a);
                 * 9: }
                 *
                 * Notice that if we evaluated 'int a' on line 0
                 * first, then we evaluate the assignment 'a = a' the
                 * lhs would evaluate to the newly declared 'a' value.
                 * Therefore, if we evaluate 'a' on line 8 before the
                 * declaration on line 0, we obtain the correct lvalue
                 * for assignment. */
                eval_arg_val = __evaluate_1(arg_val, depth + 1);
                decl_identifier = __evaluate_1(arg_decl, depth + 1);

                __evaluate_1(tree_build_bin(T_ASSIGN, decl_identifier, eval_arg_val),
                             depth + 1);
            }
        }

        __evaluate(function->data.function.stmts, depth + 1);

        pop_ctx();
    }

    if (is_T_DECL_FN(function)) {
        tree arg, fn_arg_chain = NULL, args = t->data.fncall.arguments;
        char *function_name = function->data.function.id->data.id.name;
        ptrdiff_t res;
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

         res = do_call(function_address, fn_arg_chain);

         return make_fncall_result(function->data.function.return_type, res);
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

static tree handle_decl(tree decl, tree base_type, int depth)
{
    tree decl_type = base_type;

    /* Strip off any pointer objects and add them to the base type. */
    while (is_T_POINTER(decl)) {
        tree ptr_type = tree_make(D_T_PTR);
        ptr_type->data.exp = decl_type;
        decl_type = ptr_type;
        decl = decl->data.exp;
    }

    if (is_T_DECL_STRUCT(base_type) && is_T_IDENTIFIER(decl)) {
        tree i, ctx;

        push_ctx("Structure Declaration");

        for_each_tree(i, base_type->data.structure.decls)
            __evaluate_1(i, depth + 1);

        decl_type = cur_ctx;
        pop_ctx();

        decl_type->data.ectx.parent_ctx = NULL;
        decl_type->data.ectx.is_compound = 1;

        map_identifier(decl, decl_type);
        return decl;
    }

    switch (decl->type) {
    case T_IDENTIFIER:
        make_and_map_live_var(decl, decl_type);
        return decl;
    case T_DECL_FN:
        decl->data.function.return_type = decl_type;
        map_identifier(decl->data.function.id, decl);
        return decl;
    case T_ASSIGN:
    {
        tree ret = handle_decl(decl->data.bin.left, base_type, depth);
        __evaluate_1(decl, depth + 1);
        return ret;
    }
    default:
        eval_die("Error: unknown rvalue in declaration.\n");
    }
}

static tree map_typedef(tree id, tree type)
{
    if (!is_T_IDENTIFIER(id))
        eval_die("Attempted to map type to non-identifier\n");

    map_identifier(id, type);

    return id;
}

static tree handle_typedef(tree typedef_type, tree decls, int depth)
{
    tree ret;
    tree type = __evaluate_1(typedef_type->data.exp, depth + 1), i;

    if (is_CHAIN_HEAD(decls))
        for_each_tree(i, decls)
            ret = map_typedef(i, type);
    else
        ret = map_typedef(decls, type);

    return ret;
}

static tree eval_decl(tree t, int depth)
{
    tree base_type = __evaluate_1(t->data.decl.type, depth + 1),
        decls = t->data.decl.decls,
        i, ret;

    if (is_T_TYPEDEF(base_type))
        return handle_typedef(base_type, decls, depth + 1);

    if (!decls)
        return NULL;

    if (is_CHAIN_HEAD(decls))
        for_each_tree(i, decls)
            ret = handle_decl(i, base_type, depth);
    else
        ret = handle_decl(decls, base_type, depth);

    return ret;
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
    case T_LIVE_VAR:
        val = right->data.var.val.D_T_LONG;
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

static void assign_float(tree var, tree right)
{
    double val;

    /* Obtain the value with which to assign. */
    switch (right->type)
    {
    case T_INTEGER:
        val = (double)mpz_get_si(right->data.integer);
         break;
    case T_FLOAT:
        val = (double)mpf_get_d(right->data.ffloat);
        break;
    case T_LIVE_VAR:
        val = right->data.var.val.D_T_DOUBLE;
        break;
    default:
        eval_die("Error: unknown rvalue assignment to float.\n");
    }

    switch (var->data.var.type->type) {
    case D_T_FLOAT:
        var->data.var.val.D_T_FLOAT = (float)val;
        break;
    case D_T_DOUBLE:
        var->data.var.val.D_T_DOUBLE = val;
        break;
    default:
        eval_die("Error: could not assign to non-float type");
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
    case T_LIVE_VAR:
        ptr = (void *)right->data.var.val.D_T_PTR;
        break;
    case T_INTEGER:
        ptr = (void *)mpz_get_ui(right->data.integer);
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
    case D_T_FLOAT ... D_T_DOUBLE:
        assign_float(left, right);
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
#define SETINT(type)                                                    \
        case type:                                                      \
            mpz_init_set_si(ret->data.integer, var->data.var.val.type); \
            break;
        SETINT(D_T_CHAR);
        SETINT(D_T_SHORT);
        SETINT(D_T_INT);
        SETINT(D_T_LONG);
        SETINT(D_T_LONGLONG);
        SETINT(D_T_UCHAR);
        SETINT(D_T_USHORT);
        SETINT(D_T_UINT);
        SETINT(D_T_ULONG);
        SETINT(D_T_ULONGLONG);
#undef SETINT
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

static tree eval_inc(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die("Error: not a valid lvalue.\n");

    live_var_add(exp, 1);

    ret = make_int_from_live_var(exp);

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

static tree eval_dec(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die("Error: not a valid lvalue.\n");

    live_var_sub(exp, 1);
    ret = make_int_from_live_var(exp);
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

static tree convert_to_comparable_type(tree t, int depth)
{
    tree evaluated = __evaluate_1(t, depth + 1),
        ret;

    switch (evaluated->type) {
    case T_INTEGER:
        ret = evaluated;
        break;
    case T_LIVE_VAR:
        switch (evaluated->data.var.type->type) {
        case D_T_CHAR ... D_T_ULONGLONG:
            ret = make_int_from_live_var(evaluated);
            break;
        default:
            eval_die("Could not convert live var type to comparable type\n");
        }
        break;
    default:
        eval_die("Could not convert type to comparable type\n");
    }

    return ret;
}

static tree eval_lt(tree t, int depth)
{
    tree left  = convert_to_comparable_type(t->data.bin.left, depth),
        right = convert_to_comparable_type(t->data.bin.right, depth),
        ret;

    if (left->type != right->type)
        eval_die("Could not compare different types\n");

    switch (left->type) {
    case T_INTEGER:
    {
        int result = mpz_cmp(left->data.integer, right->data.integer);
        ret = tree_make(T_INTEGER);
        mpz_init_set_ui(ret->data.integer, result < 0 ? 1 : 0);
        break;
    }
    default:
        eval_die("Unknown comparable types for lt\n");
    }

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

static tree eval_float(tree t, int depth)
{
    /* A float evaluates to itself. */
    return t;
}

static tree eval_string(tree t, int depth)
{
    /* A string evaluates to itself. */
    return t;
}

static tree eval_live_var(tree t, int depth)
{
    /* A live var evaluates to itself. */
    return t;
}

static tree eval_typedef(tree t, int depth)
{
    /* A typedef will evaluate to itself. */
    return t;
}

static tree eval_decl_struct(tree t, int depth)
{
    if (t->data.structure.id)
        map_identifier(t->data.structure.id, t);

    return t;
}

static tree eval_access(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1),
           id = t->data.bin.right;

    if (!is_E_CTX(left) && !left->data.ectx.is_compound)
        eval_die("Unknown compound type in access\n");

    if (!is_T_IDENTIFIER(id))
        eval_die("Unknown accessor in access\n");

    return resolve_identifier(id, left);
}

static tree eval_addr(tree t, int depth)
{
    tree exp = __evaluate_1(t->data.exp, depth + 1),
        ret = tree_make(T_INTEGER);

    mpz_init_set_ui(ret->data.integer, (ptrdiff_t)exp);

    return ret;
}

static tree eval_deref(tree t, int depth)
{
    tree exp = __evaluate_1(t->data.exp, depth + 1);

    if (!is_T_LIVE_VAR(exp))
        eval_die("Derefencing something that isn't live\n");

    if (!is_D_T_PTR(exp->data.var.type))
        eval_die("Attempted to dereference a non-pointer\n");

    return (tree)exp->data.var.val.D_T_PTR;
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
    case T_FLOAT:      result = eval_float(t, depth + 1);      break;
    case T_INTEGER:    result = eval_integer(t, depth + 1);    break;
    case T_STRING:     result = eval_string(t, depth + 1);     break;
    case T_P_INC:      result = eval_post_inc(t, depth + 1);   break;
    case T_P_DEC:      result = eval_post_dec(t, depth + 1);   break;
    case T_INC:        result = eval_inc(t, depth + 1);        break;
    case T_DEC:        result = eval_dec(t, depth + 1);        break;
    case T_ADD:        result = eval_add(t, depth + 1);        break;
    case T_SUB:        result = eval_sub(t, depth + 1);        break;
    case T_MUL:        result = eval_mul(t, depth + 1);        break;
    case T_DIV:        result = eval_div(t, depth + 1);        break;
    case T_LT:         result = eval_lt(t, depth + 1);         break;
    case T_LIVE_VAR:   result = eval_live_var(t, depth + 1);   break;
    case T_TYPEDEF:    result = eval_typedef(t, depth + 1);    break;
    case T_DECL_STRUCT:result = eval_decl_struct(t, depth + 1);break;
    case T_ACCESS:     result = eval_access(t, depth + 1);     break;
    case T_ADDR:       result = eval_addr(t, depth + 1);       break;
    case T_DEREF:      result = eval_deref(t, depth + 1);      break;
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
