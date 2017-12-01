#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "gc.h"

#define eprintf(args...) fprintf (stderr, args)

static const char *tree_desc_string[] = {
    #define DEFTYPE(ETYPE, DESC) [ETYPE] = DESC ,
    #include "tree.def"
    #undef DEFTYPE
    #define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT) [ETYPE] = DESC ,
    #include "ctypes.def"
    #undef DEFCTYPE
};

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

static void tree_dump_decl(tree t, int depth)
{
    eprintf(" offset :%zu\n", t->data.decl.offset);
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
    eprintf(" name: %s\n", t->data.function.id ?
            t->data.function.id->data.id.name : "<Unnamed>");
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

static void tree_dump_if(tree t, int depth)
{
    eprintf("\n");
    tree_print_indent(depth);

    eprintf(" condition:\n");
    __tree_dump(tIF_COND(t), depth + 1);

    tree_print_indent(depth);
    eprintf(" true statements:\n");
    __tree_dump(tIF_TRUE_STMTS(t), depth + 1);

    tree_print_indent(depth);
    eprintf(" else statements:\n");
    __tree_dump(tIF_ELSE_STMTS(t), depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_for_loop(tree t, int depth)
{
    eprintf("\n");
    tree_print_indent(depth);

    eprintf(" initialization:\n");
    __tree_dump(t->data.floop.initialization, depth + 1);

    tree_print_indent(depth);
    eprintf(" condition:\n");
    __tree_dump(t->data.floop.condition, depth + 1);

    tree_print_indent(depth);
    eprintf(" afterthrought:\n");
    __tree_dump(t->data.floop.afterthrought, depth + 1);

    tree_print_indent(depth);
    eprintf(" stmt(s):\n");
    __tree_dump(t->data.floop.stmts, depth + 1);
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

static void tree_dump_compound(tree t, int depth)
{
    eprintf(" size: %zu, name:\n", t->data.comp_decl.length);
    __tree_dump(t->data.comp_decl.id, depth + 1);
    tree_print_indent(depth);
    eprintf(" decl(s):\n");
    __tree_dump(t->data.comp_decl.decls, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_type(tree t, int depth)
{
    eprintf(" TYPE");
}

static void tree_dump_eval_ctx(tree t, int depth)
{
    /* Since the top-level context will contain all top-level
     * declarations, we supress output of that context. */
    if (!t->data.ectx.parent_ctx) {
        eprintf(" (Top level output suppressed)");
        return;
    }

    eprintf(" name %s\n", t->data.ectx.name);
    tree_print_indent(depth);

    eprintf(" mapping(s):\n");
    __tree_dump(t->data.ectx.id_map, depth + 1);
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
            eprintf("%" #FMT, t->data.var.val->TNAME);  \
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

static void tree_dump_live_compound(tree t, int depth)
{
    eprintf(" base: %p\n", t->data.comp.base);
    tree_print_indent(depth);

    eprintf(" decl:\n");
    __tree_dump(t->data.comp.decl, depth + 1);
    tree_print_indent(depth);

    eprintf(" members:\n");
    __tree_dump(t->data.comp.members, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_enumerator(tree t, int depth)
{
    eprintf(" id:\n");
    __tree_dump(t->data.enumerator.id, depth + 1);
    tree_print_indent(depth);

    eprintf(" enums:\n");
    __tree_dump(t->data.enumerator.enums, depth + 1);
    tree_print_indent(depth);
}

void __tree_dump_1(tree t, int depth)
{
    tree_print_indent(depth);
    eprintf("<tree at %p, type %s (%s),",
            t, tree_type_string(t->type), tree_desc_string[t->type]);
    switch (t->type) {
    case T_INTEGER:
        gmp_fprintf(stderr, " number %Zd", t->data.integer);
        break;
    case T_FLOAT:
        gmp_fprintf(stderr, " number %Ff", t->data.ffloat);
        break;
    case CPP_INCLUDE:
    case T_STRING:
        eprintf(" string: \"%s\"", t->data.string);
        break;
    case E_ALLOC:
        eprintf(" ptr: %p", t->data.ptr);
        break;
    case T_P_INC:
    case T_P_DEC:
    case T_INC:
    case T_DEC:
    case T_SIZEOF:
    case T_FN_ARG:
    case T_ADDR:
    case T_DEREF:
    case T_POINTER:
    case T_EXTERN:
    case T_STATIC:
    case T_STRUCT:
    case T_RETURN:
    case T_UNION:
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
    case T_LT:
    case T_GT:
    case T_LTEQ:
    case T_GTEQ:
    case T_EQ:
    case T_N_EQ:
    case T_L_OR:
    case T_L_AND:
    case T_CAST:
    case T_ARRAY:
    case T_ARRAY_TYPE:
    case T_ARRAY_ACCESS:
    case T_COMP_ACCESS:
    case T_ASSIGN:
    case E_MAP:
        tree_dump_binary(t,  depth);
        break;
    case T_DECL:
        tree_dump_decl(t,  depth);
        break;
    case T_IF:
        tree_dump_if(t, depth);
        break;
    case T_LOOP_FOR:
        tree_dump_for_loop(t, depth);
        break;
    case T_DECL_COMPOUND:
        tree_dump_compound(t,  depth);
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
    case T_LIVE_COMPOUND:
        tree_dump_live_compound(t, depth);
        break;
    case T_ENUMERATOR:
        tree_dump_enumerator(t, depth);
        break;
    case D_T_CHAR ... D_T_LONGDOUBLE:
    case D_T_VOID:
        tree_dump_type(t, depth);
        break;
    case D_T_PTR:
        tree_dump_single_exp(t, depth);
        break;
    case CHAIN_HEAD:
    case T_VARIADIC:
    case E_INCOMP_TYPE:
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
