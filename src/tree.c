#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "gc.h"

#define eprintf(args...) fprintf (stderr, args)

tree tree_make(enum tree_type type)
{
    tree ret = alloc_tree();
    ret->type = type;
    INIT_LIST(&ret->chain);
    return ret;
}

tree get_identifier(char *name)
{
    tree ret = tree_make(T_IDENTIFIER);
    ret->data.id.name = name;
    return ret;
}

tree tree_build_bin(enum tree_type type, tree left, tree right)
{
    tree ret = tree_make(type);
    ret->data.bin.left = left;
    ret->data.bin.right = right;
    return ret;
}

static const char *tree_desc_string[] = {
    #define DEFTYPE(ETYPE, DESC) [ETYPE] = DESC ,
    #include "tree.def"
    #undef DEFTYPE
    #define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT) [ETYPE] = DESC ,
    #include "ctypes.def"
    #undef DEFCTYPE
};

static const char *tree_type_string(enum tree_type t)
 {
    switch (t) {
        #define DEFTYPE(ETYPE, DESC)    \
            case ETYPE:                 \
                return #ETYPE;
        #include "tree.def"
        #undef DEFTYPE
        #define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT)    \
            case ETYPE:                                 \
                return #ETYPE;
        #include "ctypes.def"
        #undef DEFCTYPE
        default:
            return "unknown";
    }
}

static void __tree_dump(tree head, int depth);

static void tree_print_indent(int depth)
{
    int i;
    for (i = 0; i < depth *2; i++)
        eprintf(" ");
}

static void tree_dump_single_exp(tree t, int depth)
{
    eprintf(" exp:\n");
    __tree_dump(t->data.exp, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_binary(tree t, int depth)
{
    eprintf(" left:\n");
    __tree_dump(t->data.bin.left, depth + 1);
    tree_print_indent(depth);
    eprintf("right:\n");
    __tree_dump(t->data.bin.right, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_array(tree t, int depth)
{
    eprintf(" decl:\n");
    __tree_dump(t->data.array.decl, depth + 1);
    tree_print_indent(depth);
    eprintf(" exp:\n");
    __tree_dump(t->data.array.exp, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_decl(tree t, int depth)
{
    eprintf("\n");
    tree_print_indent(depth);
    eprintf(" type:\n");
    __tree_dump(t->data.decl.type, depth + 1);
    tree_print_indent(depth);
    eprintf(" decl(s):\n");
    __tree_dump(t->data.decl.decls, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_function(tree t, int depth)
{
    eprintf(" name: %s\n", t->data.function.id->data.id.name);
    tree_print_indent(depth);

    eprintf(" Return Type:\n");
    __tree_dump(t->data.function.return_type, depth + 1);
    tree_print_indent(depth);

    eprintf(" argument(s):\n");
    __tree_dump(t->data.function.arguments, depth + 1);
    tree_print_indent(depth);

    eprintf(" stmt(s):\n");
    __tree_dump(t->data.function.stmts, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_fncall(tree t, int depth)
{
    eprintf("\n");
    tree_print_indent(depth);
    eprintf(" identifier:\n");
    __tree_dump(t->data.fncall.identifier, depth + 1);

    tree_print_indent(depth);
    eprintf(" argument(s):\n");
    __tree_dump(t->data.fncall.arguments, depth + 1);

    tree_print_indent(depth);
}

static void tree_dump_struct(tree t, int depth)
{
    eprintf(" name:\n");
    __tree_dump(t->data.structure.id, depth + 1);
    tree_print_indent(depth);
    eprintf(" decl(s):\n");
    __tree_dump(t->data.structure.decls, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_type(tree t, int depth)
{
    eprintf(" TYPE");
}

static void tree_dump_eval_ctx(tree t, int depth)
{
    identifier_mapping *i;

    /* Since the top-level context will contain all top-level
     * declarations, we supress output of that context. */
    if (!t->data.ectx.parent_ctx && !t->data.ectx.is_compound) {
        eprintf(" (Top level output suppressed)");
        return;
    }

    eprintf(" name %s\n", t->data.ectx.name);
    tree_print_indent(depth);
    eprintf(" mapping(s):\n");
    list_for_each(i, &t->data.ectx.id_map.mappings, mappings) {
        tree_print_indent(depth + 1);
        eprintf("<mapping, identifier:\n");
        __tree_dump(i->id, depth + 2);
        tree_print_indent(depth + 1);
        eprintf(" var:\n");
        __tree_dump(i->t, depth + 2);
        tree_print_indent(depth + 1);
        eprintf(">\n");
    }
    tree_print_indent(depth);
    eprintf(" parent(s):\n");
    __tree_dump(t->data.ectx.parent_ctx, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_live_var(tree t, int depth)
{
    tree type = t->data.var.type;

    eprintf("\n");
    tree_print_indent(depth);
    eprintf(" type:\n");
    __tree_dump(t->data.var.type, depth + 1);
    tree_print_indent(depth);
    eprintf(" val: ");

    switch (type->type) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)               \
        case TNAME:                                     \
            eprintf("%" #FMT, t->data.var.val.TNAME);  \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        eprintf("<Unprintable>");
        break;
    }
    eprintf("\n");
    tree_print_indent(depth);
}

void __tree_dump_1(tree t, int depth)
{
    tree_print_indent(depth);
    eprintf("<tree at %p, next %p, reachable: %d, type %s (%s),",
            t,  t->chain.next, t->reachable,
            tree_type_string(t->type), tree_desc_string[t->type]);
    switch (t->type) {
    case T_INTEGER:
        gmp_fprintf(stderr, " number %Zd", t->data.integer);
        break;
    case T_STRING:
        eprintf(" string: \"%s\"", t->data.string);
        break;
    case T_P_INC:
        tree_dump_single_exp(t,  depth);
        break;
    case T_P_DEC:
        tree_dump_single_exp(t,  depth);
        break;
    case T_INC:
        tree_dump_single_exp(t,  depth);
        break;
    case T_DEC:
        tree_dump_single_exp(t,  depth);
        break;
    case T_FN_ARG:
        tree_dump_single_exp(t, depth);
        break;
    case T_ADDR:
        tree_dump_single_exp(t, depth);
        break;
    case T_POINTER:
        tree_dump_single_exp(t, depth);
        break;
    case T_TYPEDEF:
        tree_dump_single_exp(t, depth);
        break;
    case T_IDENTIFIER:
        eprintf(" name %s", t->data.id.name);
        break;
    case T_MUL:
    case T_DIV:
    case T_MOD:
    case T_ADD:
    case T_SUB:
    case T_ACCESS:
    case T_ASSIGN:
        tree_dump_binary(t,  depth);
        break;
    case T_ARRAY:
        tree_dump_array(t, depth);
    case T_DECL:
        tree_dump_decl(t,  depth);
        break;
    case T_DECL_STRUCT:
        tree_dump_struct(t,  depth);
        break;
    case T_FN_DEF:
    case T_DECL_FN:
        tree_dump_function(t,  depth);
        break;
    case T_FN_CALL:
        tree_dump_fncall(t,  depth);
        break;
    case E_CTX:
        tree_dump_eval_ctx(t, depth);
        break;
    case T_LIVE_VAR:
        tree_dump_live_var(t, depth);
        break;
    case D_T_CHAR ... D_T_ULONGLONG:
        tree_dump_type(t, depth);
        break;
    case D_T_PTR:
        tree_dump_single_exp(t, depth);
        break;
    case CHAIN_HEAD:
        break;
    }
    eprintf(">\n");
}

static void __tree_dump(tree head, int depth)
{
    tree i;

    if (!head) {
        tree_print_indent(depth);
        eprintf("<null>\n");
        return;
    }

    if (is_CHAIN_HEAD(head)) {
        tree_print_indent(depth);
        eprintf("<chain\n");

        for_each_tree(i, head)
            __tree_dump_1(i, depth+1);

        tree_print_indent(depth);
        eprintf(">\n");
    }
    else
        __tree_dump_1(head, depth);
}

void tree_dump(tree head)
{
    __tree_dump(head, 0);
}
