#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "identifier.h"

#define eprintf(args...) fprintf (stderr, args)

tree tree_make(enum tree_type type)
{
    tree ret = calloc(sizeof(*ret), 1);
    ret->type = type;
    return ret;
}

tree tree_build_bin(enum tree_type type, tree left, tree right)
{
    tree ret = tree_make(type);
    ret->data.bin.left = left;
    ret->data.bin.right = right;
    return ret;
}

static const char *tree_type_string(enum tree_type t)
 {
    switch (t) {
        #define DEFTYPE(ETYPE, DESC)    \
            case ETYPE:                 \
                return #ETYPE;
        #include "tree.def"
        #undef DEFTYPE
        default:
            return "unknown";
    }
}

static void tree_print_indent(int depth)
{
    int i;
    for (i = 0; i < depth *2; i++)
        eprintf(" ");
}

static void tree_dump_single_exp(tree t, int depth)
{
    eprintf(" exp:\n");
    tree_dump(t->data.exp, depth + 1);
}

static void tree_dump_identifier(identifier *id, int depth)
{
    tree_print_indent(depth);
    eprintf("<identifier at %p, name: %s>\n", id, id->name);
}

static void tree_dump_binary(tree t, int depth)
{
    eprintf(" left:\n");
    tree_dump(t->data.bin.left, depth + 1);
    tree_print_indent(depth);
    eprintf("right:\n");
    tree_dump(t->data.bin.right, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_decl(tree t, int depth)
{
    eprintf("\n");
    tree_print_indent(depth);
    eprintf(" type:\n");
    tree_dump(t->data.decl.type, depth + 1);
    tree_print_indent(depth);
    eprintf(" decl(s):\n");
    tree_dump(t->data.decl.decls, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_struct(tree t, int depth)
{
    eprintf(" name: %s\n", t->data.structure.id->name);
    tree_print_indent(depth);
    eprintf(" decl(s):\n");
    tree_dump(t->data.structure.decls, depth + 1);
    tree_print_indent(depth);
}

static void tree_dump_type(tree t, int depth)
{
    eprintf(" TYPE");
}

void tree_dump(tree tree, int depth)
{
    while (tree) {
        tree_print_indent(depth);
        eprintf("<tree at %p, next %p, type %s,",
                tree, tree->next, tree_type_string(tree->type));
        switch (tree->type) {
        case T_INTEGER:
            gmp_fprintf(stderr, " number %Zd", tree->data.integer);
            break;
        case T_P_INC:
            tree_dump_single_exp(tree, depth);
            break;
        case T_P_DEC:
            tree_dump_single_exp(tree, depth);
            break;
        case T_INC:
            tree_dump_single_exp(tree, depth);
            break;
        case T_DEC:
            tree_dump_single_exp(tree, depth);
            break;
        case T_IDENTIFIER:
            eprintf(" id:\n");
            tree_dump_identifier(tree->data.id, depth + 1);
            tree_print_indent(depth);
            break;
        case T_MUL:
        case T_DIV:
        case T_MOD:
        case T_ADD:
        case T_SUB:
        case T_ASSIGN:
            tree_dump_binary(tree, depth);
            break;
        case T_DECL:
            tree_dump_decl(tree, depth);
            break;
        case T_DECL_STRUCT:
            tree_dump_struct(tree, depth);
            break;
        case D_T_INT:
            tree_dump_type(tree, depth);
            break;
        }
        eprintf(">\n");
        tree = tree->next;
    }
}
