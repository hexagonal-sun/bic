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

static const char *current_filename;

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
    new_ctx->data.ectx.id_map = tree_make(CHAIN_HEAD);
    new_ctx->data.ectx.alloc_chain = tree_make(CHAIN_HEAD);

    cur_ctx = new_ctx;
}

static void track_alloc(void *ptr)
{
    tree alloc = tree_make(E_ALLOC);

    alloc->data.ptr = ptr;

    tree_chain(alloc, cur_ctx->data.ectx.alloc_chain);
}

static void ctx_backtrace(void)
{
    fprintf(stderr, "Evaluator Backtrace\n===================\n");
    tree_dump(cur_ctx);
}

static void __attribute__((noreturn)) eval_die(tree t, const char *format, ...)
{
    fprintf(stderr, "%s:%d:%d: error: ", current_filename, t->locus.line_no,
            t->locus.column_no);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    ctx_backtrace();

    exit(EXIT_FAILURE);
}

static tree resolve_id(tree id, tree idmap)
{
    tree i;

    for_each_tree(i, idmap) {
        if (strcmp(i->data.bin.left->data.id.name, id->data.id.name) == 0)
            return i->data.bin.right;
    }

    return NULL;
}

static tree resolve_identifier(tree id, tree ctx)
{
    return resolve_id(id, ctx->data.ectx.id_map);
}

static void __map_identifer(tree id, tree t, tree idmap)
{
    tree new_map = tree_make(E_MAP);

    if (!is_T_IDENTIFIER(id))
        eval_die(id, "Attempted to map non-identifier\n");

    new_map->data.bin.left = id;
    new_map->data.bin.right = t;

    tree_chain(new_map, idmap);
}

static size_t get_size_of_type(tree type, int depth)
{
    if (is_T_DECL_COMPOUND(type) && type->data.comp_decl.length != 0)
        /* Since the evaluation of a T_DECL_COMPOUND has possible side
         * affects (mapping itself into the current context), check to
         * see if it already knows it's size before evaluating a
         * sizeof() expression. */
        return type->data.comp_decl.length;
    else {
        tree sz, szof = tree_make(T_SIZEOF);
        szof->data.exp = type;
        sz = __evaluate_1(szof, depth);
        return mpz_get_ui(sz->data.integer);
    }
}

static void map_identifier(tree id, tree t)
{
    if (resolve_identifier(id, cur_ctx))
        eval_die(id, "Attempted to map already existing identifier %s.\n",
                id->data.id.name);

    __map_identifer(id, t, cur_ctx->data.ectx.id_map);
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

    eval_die(t, "Could not resolve identifier %s.\n",
            t->data.id.name);
}

static tree make_live_var(tree type)
{
    tree live_var = tree_make(T_LIVE_VAR);

    live_var->data.var.type = type;
    live_var->data.var.val = malloc(sizeof(*live_var->data.var.val));
    track_alloc(live_var->data.var.val);

    return live_var;
}

static tree make_int_from_live_var(tree var)
{
    tree ret = tree_make(T_INTEGER);
    tree type = var->data.var.type;

    switch (type->type) {
#define SETINT(type)                                                    \
        case type:                                                      \
            mpz_init_set_si(ret->data.integer, var->data.var.val->type); \
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
        eval_die(var, "Could not create integer type from live var.");
    }

    return ret;
}

static tree make_fncall_result(tree type, ptrdiff_t result)
{
    tree ret;

    if (!type)
        return NULL;

    ret = make_live_var(type);

    switch (type->type)
    {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)             \
        case TNAME:                                   \
            ret->data.var.val->TNAME = (CTYPE)result;  \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die(type, "Could not create function return value\n");
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
            arg_decl, arg_val;

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
                eval_die(t, "Invalid number of parameters to function");

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
            eval_die(t, "Could not resolve external symbol: %s\n", function_name);

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
    assert(id->type == T_IDENTIFIER);

    if (is_E_INCOMP_TYPE(type))
        eval_die(id, "Can not create incomplete type %s\n",
                 id->data.id.name);

    map_identifier(id, make_live_var(type));
}

static size_t get_array_size(tree array_decl, tree base_type, int depth)
{
    size_t sz_of_each_element = get_size_of_type(base_type, depth), no_elms;
    tree no_elements = __evaluate_1(array_decl->data.bin.right, depth + 1);

    if (is_T_LIVE_VAR(no_elements))
        no_elements = make_int_from_live_var(no_elements);

    if (!is_T_INTEGER(no_elements))
        eval_die(array_decl, "Attempted to create array with non-constant size\n");

    no_elms = mpz_get_ui(no_elements->data.integer);

    return sz_of_each_element * no_elms;
}

static tree instantiate_array(tree array_decl, tree base_type, void *base,
                              size_t length)
{
    tree live_var, ptr = tree_make(D_T_PTR),
        id = array_decl->data.bin.left;

    if (!is_T_IDENTIFIER(id))
        eval_die(array_decl, "Unknown array name type\n");

    /* An array is basically a pointer; it has no bounds checking.
     * Therefore all we need to do is create a live var with a pointer
     * type. */
    ptr->data.exp = base_type;
    live_var = make_live_var(ptr);
    live_var->data.var.val->D_T_PTR = base;

    /* These are used by the sizeof() expression to return the correct
     * size. */
    live_var->data.var.is_array = 1;
    live_var->data.var.array_length = length;

    return live_var;
}

static void resolve_ptr_type(tree *ptr, tree *base_type)
{
    while (is_T_POINTER(*ptr)) {
        tree ptr_type = tree_make(D_T_PTR);
        ptr_type->data.exp = *base_type;
        *base_type = ptr_type;
        *ptr = (*ptr)->data.exp;
    }
}

static tree instantiate_struct(tree struct_decl, int depth, void *base);

static void handle_struct_decl(tree decl, tree live_struct, int depth)
{
    tree live_element,
        decl_type = __evaluate_1(decl->data.decl.type, depth + 1),
        decl_element = decl->data.decl.decls;

    void *base = live_struct->data.comp.base;

    resolve_ptr_type(&decl_element, &decl_type);

    if (is_T_ARRAY(decl_element)) {
        size_t array_sz = get_array_size(decl_element, decl_type, depth);
        live_element = instantiate_array(decl_element, decl_type,
                                         base + decl->data.decl.offset,
                                         array_sz);

        __map_identifer(decl_element->data.bin.left, live_element,
                        live_struct->data.comp.members);
        return;
    }

    if (is_CTYPE(decl_type)) {
        live_element = tree_make(T_LIVE_VAR);
        live_element->data.var.type = decl_type;
        live_element->data.var.val = base + decl->data.decl.offset;
        __map_identifer(decl_element, live_element,
                        live_struct->data.comp.members);
        return;
    }

    if (is_T_DECL_COMPOUND(decl_type)) {
        live_element = instantiate_struct(decl_type, depth,
                                          base + decl->data.decl.offset);

        __map_identifer(decl_element, live_element,
                        live_struct->data.comp.members);

        return;
    }

    eval_die(decl, "Unknown structure member decl\n");
}

static tree instantiate_struct(tree struct_decl, int depth, void *base)
{
    tree i, live_struct = tree_make(T_LIVE_COMPOUND);

    live_struct->data.comp.decl = struct_decl;
    live_struct->data.comp.base = base;
    live_struct->data.comp.members = tree_make(CHAIN_HEAD);

    for_each_tree(i, struct_decl->data.comp_decl.decls)
        handle_struct_decl(i, live_struct, depth);

    return live_struct;
}

static tree alloc_struct(tree struct_decl, int depth)
{
    void *base = malloc(struct_decl->data.comp_decl.length);
    track_alloc(base);

    return instantiate_struct(struct_decl, depth, base);
}

static tree alloc_array(tree array_decl, tree base_type, int depth)
{
    size_t array_sz = get_array_size(array_decl, base_type, depth);
    void *array_mem = malloc(array_sz);
    tree live_var = instantiate_array(array_decl, base_type, array_mem,
                                      array_sz);

    track_alloc(array_mem);

    return live_var;
}

static tree handle_decl(tree decl, tree base_type, int depth)
{
    tree decl_type = base_type;

    /* Strip off any pointer objects and add them to the base type. */
    resolve_ptr_type(&decl, &decl_type);

    if (is_T_DECL_COMPOUND(decl_type) && is_T_IDENTIFIER(decl)) {
        map_identifier(decl, alloc_struct(decl_type, depth));
        return decl;
    }

    if (is_T_ARRAY(decl)) {
        tree live_var = alloc_array(decl, decl_type, depth);
        tree id = decl->data.bin.left;

        /* We know that since the array was successfully instantiated,
         * the left element of the decl *must* be an identifier. */
        map_identifier(id, live_var);
        return id;
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
        eval_die(decl, "Unknown rvalue in declaration.\n");
    }
}

static tree map_typedef(tree id, tree type)
{
    resolve_ptr_type(&id, &type);

    if (!is_T_IDENTIFIER(id))
        eval_die(id, "Attempted to map type to non-identifier\n");

    map_identifier(id, type);

    return id;
}

static tree handle_extern_decl(tree extern_type, tree decl)
{
    tree live_var, id;
    void *sym_addr;

    resolve_ptr_type(&decl, &extern_type);

    id = decl;

    if (!is_T_IDENTIFIER(id))
        eval_die(decl, "attempted to extern something that isn't an "
                 "identifier\n");

    sym_addr = dlsym(RTLD_DEFAULT, id->data.id.name);

    if (!sym_addr)
        eval_die(decl, "Could not resolve extern symbol %s\n",
                 id->data.id.name);

    live_var = tree_make(T_LIVE_VAR);
    live_var->data.var.type = extern_type;
    live_var->data.var.val = sym_addr;

    map_identifier(id, live_var);

    return id;
}

static tree handle_extern(tree extern_type, tree decls, int depth)
{
    tree ret;
    tree type = __evaluate_1(extern_type->data.exp, depth +1), i;

    if (is_CHAIN_HEAD(decls))
        for_each_tree(i, decls)
            ret = handle_extern_decl(type, i);
    else
        ret = handle_extern_decl(type, decls);

    return ret;
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

static tree handle_forward_decl(tree type)
{
    tree id = type->data.exp;

    tree forward_decl = tree_make(E_INCOMP_TYPE);

    map_identifier(id, forward_decl);

    return id;
}

static tree eval_decl(tree t, int depth)
{
    tree base_type = t->data.decl.type,
        decls = t->data.decl.decls,
        i, ret;

    if (!decls && (is_T_STRUCT(base_type) || is_T_UNION(base_type)))
        return handle_forward_decl(base_type);

    base_type = __evaluate_1(t->data.decl.type, depth + 1);

    if (is_T_TYPEDEF(base_type))
        return handle_typedef(base_type, decls, depth + 1);

    if (is_T_EXTERN(base_type))
        return handle_extern(base_type, decls, depth + 1);

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
        val = right->data.var.val->D_T_LONG;
        break;
    default:
        eval_die(right, "unknown rvalue assignment to integer.\n");
    }

    switch (var->data.var.type->type) {
        #define DEFCTYPE(TNAME, DESC, CTYPE, FMT)        \
            case TNAME:                                  \
                var->data.var.val->TNAME = (CTYPE)val;    \
                break;
        #include "ctypes.def"
        #undef DEFCTYPE
    default:
        eval_die(var, "could not assign to non-integer type");
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
        val = right->data.var.val->D_T_DOUBLE;
        break;
    default:
        eval_die(right, "unknown rvalue assignment to float.\n");
    }

    switch (var->data.var.type->type) {
    case D_T_FLOAT:
        var->data.var.val->D_T_FLOAT = (float)val;
        break;
    case D_T_DOUBLE:
        var->data.var.val->D_T_DOUBLE = val;
        break;
    default:
        eval_die(var, "could not assign to non-float type");
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
        ptr = (void *)right->data.var.val->D_T_PTR;
        break;
    case T_INTEGER:
        ptr = (void *)mpz_get_ui(right->data.integer);
        break;
    default:
        eval_die(right, "Could not assign to non-pointer type");
    }

    var->data.var.val->D_T_PTR = ptr;
}

static tree eval_assign(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1);
    tree right = __evaluate_1(t->data.bin.right, depth + 1);

    /* Ensure we have a valid lvalue. */
    if (!is_T_LIVE_VAR(left))
        eval_die(t, "Not a valid lvalue.\n");

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
        eval_die(t, "Unknown assignment rvalue type.");
    }

    return NULL;
}

static void live_var_add(tree var, unsigned long int val)
{
    tree type = var->data.var.type;
    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)               \
        case TNAME:                                     \
            var->data.var.val->TNAME += val;             \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die(var, "Could not add to unknown live var type.");
    }
}

static void live_var_sub(tree var, unsigned long int val)
{
    tree type = var->data.var.type;
    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)               \
        case TNAME:                                     \
            var->data.var.val->TNAME -= val;             \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eval_die(var, "Could not subtract to unknown live var type.");
    }
}

static tree eval_post_inc(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die(t, "Not a valid lvalue.\n");

    ret = make_int_from_live_var(exp);

    live_var_add(exp, 1);

    return ret;
}

static tree eval_inc(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die(t, "Not a valid lvalue.\n");

    live_var_add(exp, 1);

    ret = make_int_from_live_var(exp);

    return ret;
}

static tree eval_post_dec(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die(t, "Not a valid lvalue.\n");

    ret = make_int_from_live_var(exp);
    live_var_sub(exp, 1);
    return ret;
}

static tree eval_dec(tree t, int depth)
{
    tree ret;
    tree exp = __evaluate_1(t->data.exp, depth + 1);
    if (!is_T_LIVE_VAR(exp))
        eval_die(t, "Not a valid lvalue.\n");

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
        eval_die(t, "Could not add to non integer type\n");

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
        eval_die(t, "Could not subtract to non integer type\n");

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
        eval_die(t, "Could not subtract to non integer type\n");

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
        eval_die(t, "Could not subtract to non integer type\n");

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
            eval_die(t, "Could not convert live var type to comparable type\n");
        }
        break;
    default:
        eval_die(t, "Could not convert type to comparable type\n");
    }

    return ret;
}

static tree eval_lt(tree t, int depth)
{
    tree left  = convert_to_comparable_type(t->data.bin.left, depth),
        right = convert_to_comparable_type(t->data.bin.right, depth),
        ret;

    if (left->type != right->type)
        eval_die(t, "Could not compare different types\n");

    switch (left->type) {
    case T_INTEGER:
    {
        int result = mpz_cmp(left->data.integer, right->data.integer);
        ret = tree_make(T_INTEGER);
        mpz_init_set_ui(ret->data.integer, result < 0 ? 1 : 0);
        break;
    }
    default:
        eval_die(t, "Unknown comparable types for lt\n");
    }

    return ret;
}

static tree eval_gt(tree t, int depth)
{
    tree left  = convert_to_comparable_type(t->data.bin.left, depth),
        right = convert_to_comparable_type(t->data.bin.right, depth),
        ret;

    if (left->type != right->type)
        eval_die(t, "Could not compare different types\n");

    switch (left->type) {
    case T_INTEGER:
    {
        int result = mpz_cmp(left->data.integer, right->data.integer);
        ret = tree_make(T_INTEGER);
        mpz_init_set_ui(ret->data.integer, result > 0 ? 1 : 0);
        break;
    }
    default:
        eval_die(t, "Unknown comparable types for gt\n");
    }

    return ret;
}

static tree eval_lteq(tree t, int depth)
{
    tree left  = convert_to_comparable_type(t->data.bin.left, depth),
        right = convert_to_comparable_type(t->data.bin.right, depth),
        ret;

    if (left->type != right->type)
        eval_die(t, "Could not compare different types\n");

    switch (left->type) {
    case T_INTEGER:
    {
        int result = mpz_cmp(left->data.integer, right->data.integer);
        ret = tree_make(T_INTEGER);
        mpz_init_set_ui(ret->data.integer, result <= 0 ? 1 : 0);
        break;
    }
    default:
        eval_die(t, "Unknown comparable types for lteq\n");
    }

    return ret;
}

static tree eval_gteq(tree t, int depth)
{
    tree left  = convert_to_comparable_type(t->data.bin.left, depth),
        right = convert_to_comparable_type(t->data.bin.right, depth),
        ret;

    if (left->type != right->type)
        eval_die(t, "Could not compare different types\n");

    switch (left->type) {
    case T_INTEGER:
    {
        int result = mpz_cmp(left->data.integer, right->data.integer);
        ret = tree_make(T_INTEGER);
        mpz_init_set_ui(ret->data.integer, result >= 0 ? 1 : 0);
        break;
    }
    default:
        eval_die(t, "Unknown comparable types for gteq\n");
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

static tree eval_live_compound(tree t, int depth)
{
    /* A live compound evaluates to itself. */
    return t;
}

static tree eval_extern(tree t, int depth)
{
    /* An extern will evaluate to itself. */
    return t;
}

static tree eval_typedef(tree t, int depth)
{
    /* A typedef will evaluate to itself. */
    return t;
}

static tree eval_loop_for(tree t, int depth)
{
    __evaluate_1(t->data.floop.initialization, depth + 1);

    do {

        tree cond_result = __evaluate_1(t->data.floop.condition,
                                        depth + 1);

        if (!is_T_INTEGER(cond_result))
            eval_die(t, "Unknown condition result");

        if (!mpz_get_si(cond_result->data.integer))
            break;

        push_ctx("For Loop");
        __evaluate(t->data.floop.stmts, depth + 1);
        pop_ctx();

        __evaluate_1(t->data.floop.afterthrought, depth + 1);

    } while (1);

    return NULL;
}


/* Build a simplified decl chain, ensuring all decls with multiple
 * identifiers are split.
 *
 * This is the equivalent of mutating
 *
 *     struct foobar {
 *         int a, b;
 *         float c, d;
 *         char *e, f;
 *     }
 *
 * to
 *
 *     struct foobar {
 *         int a;
 *         int b;
 *         float c;
 *         float d;
 *         char *e;
 *         char f;
 *     };
 */
static tree expand_decl_chain(tree decl_chain)
{
    tree decl, decl_element, new_chain = tree_make(CHAIN_HEAD);

    for_each_tree(decl, decl_chain) {
        for_each_tree(decl_element, decl->data.decl.decls) {
            tree new_decl = tree_make(T_DECL);

            new_decl->data.decl.type = decl->data.decl.type;
            new_decl->data.decl.decls = decl_element;

            tree_chain(new_decl, new_chain);
        }
    }

    return new_chain;
}

static tree eval_decl_compound(tree t, int depth)
{
    tree i, struct_id = t->data.comp_decl.id;
    int offset = 0;
    size_t max_member_sz = 0;

    /* We map to ourselves here so that any references to the same
     * compound in the decls will result in a reference to the
     * struct_decl.  Also check to see if there are any forward
     * declarations of the type.  If there are, copy 't' to the
     * unresolved decl and use that as our new 't' so all referencies
     * to it will be updated.*/
    if (struct_id) {
        tree forward_decl = resolve_identifier(struct_id, cur_ctx);

        if (forward_decl) {
            tree_copy(forward_decl, t);
            t = forward_decl;
        } else
            map_identifier(struct_id, t);
    }

    /* Don't attempt to expand an already expanded struct. */
    if (t->data.comp_decl.expanded)
        return t;

    /* Expand the compound decl chain so each decl can have it's own
     * offset.  */
    t->data.comp_decl.decls = expand_decl_chain(t->data.comp_decl.decls);

    /* Populate the decls with their offsets, as well as the total
     * size of the compound. */
    for_each_tree(i, t->data.comp_decl.decls) {
        tree id;
        size_t member_size;

        /* To calculate the size of each element of the compound, we
         * temporarily push the evaluation ctx, evaluate each decl
         * within the new evaluation ctx, resolve the identifier back
         * to it's live var and finally pass that through a sizeof()
         * evaluation.
         *
         * NOTE: we don't evaluate the types of the decls here as we
         * could create a loop back to ourselves (when declaring a
         * pointer to the same struct).  Instead, we evaluate all decl
         * types at struct installation time. */
        push_ctx("Compound Declaration");

        id = __evaluate_1(i, depth + 1);

        i->data.decl.offset = offset;

        member_size = get_size_of_type(resolve_identifier(id, cur_ctx), depth);

        if (t->data.comp_decl.type == sstruct)
            offset += member_size;
        else {
            offset = 0;
            if (member_size > max_member_sz)
                max_member_sz = member_size;
        }

        pop_ctx();
    }

    if (t->data.comp_decl.type == sstruct)
        t->data.comp_decl.length = offset;
    else
        t->data.comp_decl.length = max_member_sz;

    t->data.comp_decl.expanded = 1;

    return t;
}

static tree eval_enumerator(tree t, int depth)
{
    tree enum_id, enum_base_type = tree_make(ENUMTYPE);
    unsigned int enum_val = 0;

    /* Whenever we encounter this decl, we wish to treat it as just a
     * normal integer. */
    if (t->data.enumerator.id)
        map_identifier(t->data.enumerator.id, enum_base_type);

    /* Allocate a number for each member of the enumerator and map the
     * identifier to the value. */
    for_each_tree(enum_id, t->data.enumerator.enums) {
        tree integer = tree_make(T_INTEGER);
        mpz_init_set_ui(integer->data.integer, enum_val);

        map_identifier(enum_id, integer);

        enum_val++;
    }

    return enum_base_type;
}

static tree eval_access(tree t, int depth)
{
    tree left = __evaluate_1(t->data.bin.left, depth + 1),
           id = t->data.bin.right;

    if (!is_T_LIVE_COMPOUND(left))
        eval_die(t, "Unknown compound type in access\n");

    if (!is_T_IDENTIFIER(id))
        eval_die(t, "Unknown accessor in access\n");

    return resolve_id(id, left->data.comp.members);
}

static tree eval_struct(tree t, int depth)
{
    return __evaluate_1(t->data.exp, depth);
}

static tree eval_union(tree t, int depth)
{
    return __evaluate_1(t->data.exp, depth);
}

static tree eval_array_access(tree t, int depth)
{
    tree array = __evaluate_1(t->data.bin.left, depth + 1),
        index = __evaluate_1(t->data.bin.right, depth + 1),
        new_ptr, deref;
    size_t idx, base_type_length;
    void *base;

    if (!is_T_LIVE_VAR(array))
        eval_die(t, "Attempted to access non-live variable.\n");

    if (!is_D_T_PTR(array->data.var.type))
        eval_die(t, "Attempted deference on non pointer variable.\n");

    if (is_T_LIVE_VAR(index))
        index = make_int_from_live_var(index);

    if (!is_T_INTEGER(index))
        eval_die(index, "Unknown index type\n");

    idx = mpz_get_ui(index->data.integer);

    base = array->data.var.val->D_T_PTR;

    /* Find the size of the type that the pointer points to. */
    base_type_length = get_size_of_type(array->data.var.type->data.exp, depth);

    /* Calculate the offset into the array and make a pointer to point
     * to that offset. */
    new_ptr = make_live_var(array->data.var.type);
    new_ptr->data.var.val->D_T_PTR = base + (base_type_length * idx);

    /* Dereference the pointer and return the result. */
    deref = tree_make(T_DEREF);
    deref->data.exp = new_ptr;

    return __evaluate_1(deref, depth + 1);
}

static int get_ctype_size(tree t)
{
    switch (t->type)
    {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)       \
        case TNAME:                             \
            return sizeof(CTYPE);
#include "ctypes.def"
#undef DEFCTYPE
        default:
            eval_die(t, "Unknown ctype\n");
    }
}

static tree eval_sizeof(tree t, int depth)
{
    tree exp = __evaluate_1(t->data.exp, depth + 1),
        type = exp,
         ret = tree_make(T_INTEGER);

    if (is_T_LIVE_VAR(exp))
        type = exp->data.var.type;

    if (is_T_LIVE_COMPOUND(exp))
        type = exp->data.comp.decl;

    if (is_D_T_PTR(type) && exp->data.var.is_array) {
        mpz_init_set_ui(ret->data.integer, exp->data.var.array_length);
        return ret;
    }

    if (is_CTYPE(type)) {
        mpz_init_set_ui(ret->data.integer, get_ctype_size(type));
        return ret;
    }

    if (is_T_DECL_COMPOUND(type)) {
        mpz_init_set_ui(ret->data.integer, type->data.comp_decl.length);
        return ret;
    }

    eval_die(t, "Could not calculate size of expression");
}

static tree eval_addr(tree t, int depth)
{
    tree exp = __evaluate_1(t->data.exp, depth + 1),
        ptr_type, ret;

    if (!is_LIVE(exp))
        eval_die(t, "attempted to take address of non-live variable.\n");

    ptr_type = tree_make(D_T_PTR);
    ptr_type->data.exp = exp->data.var.type;

    ret = make_live_var(ptr_type);

    ret->data.var.val->D_T_PTR = exp->data.var.val;

    return ret;
}

static tree eval_deref(tree t, int depth)
{
    tree exp = __evaluate_1(t->data.exp, depth + 1),
        new_type, ret;

    if (!is_T_LIVE_VAR(exp))
        eval_die(t, "Derefencing something that isn't live\n");

    if (!is_D_T_PTR(exp->data.var.type))
        eval_die(t, "Attempted to dereference a non-pointer\n");

    new_type = exp->data.var.type->data.exp;

    if (is_T_DECL_COMPOUND(new_type))
        return instantiate_struct(new_type, depth,
                                  exp->data.var.val->D_T_PTR);

    /* All live vars that are created by a pointer dereference won't
     * have their values free'd by the GC. */
    ret = tree_make(T_LIVE_VAR);
    ret->data.var.type = new_type;
    ret->data.var.val = exp->data.var.val->D_T_PTR;

    return ret;
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
    case T_GT:         result = eval_gt(t, depth + 1);         break;
    case T_LTEQ:       result = eval_lteq(t, depth + 1);       break;
    case T_GTEQ:       result = eval_gteq(t, depth + 1);       break;
    case T_LIVE_VAR:   result = eval_live_var(t, depth + 1);   break;
    case T_LIVE_COMPOUND: result = eval_live_compound(t, depth + 1); break;
    case T_EXTERN:     result = eval_extern(t, depth + 1);     break;
    case T_TYPEDEF:    result = eval_typedef(t, depth + 1);    break;
    case T_LOOP_FOR:   result = eval_loop_for(t, depth + 1);   break;
    case T_DECL_COMPOUND:result = eval_decl_compound(t, depth + 1);break;
    case T_ENUMERATOR: result = eval_enumerator(t, depth + 1); break;
    case T_SIZEOF:     result = eval_sizeof(t, depth + 1);     break;
    case T_ACCESS:     result = eval_access(t, depth + 1);     break;
    case T_STRUCT:     result = eval_struct(t, depth + 1);     break;
    case T_UNION:      result = eval_union(t, depth + 1);      break;
    case T_ARRAY_ACCESS:result = eval_array_access(t, depth + 1); break;
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

    if (!is_CHAIN_HEAD(head))
        return __evaluate_1(head, depth);

    for_each_tree(i, head)
        result = __evaluate_1(i, depth);

    return result;
}

void evaluate(tree t, const char *fname)
{
    current_filename = fname;
    __evaluate(t, 0);
}

void eval_init(void)
{
    push_ctx("Toplevel");
}
