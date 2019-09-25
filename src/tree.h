#ifndef __TREE_H_
#define __TREE_H_

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>
#include "list.h"
#include "tree-access.h"

typedef struct tree *tree;

enum tree_type {
    #define DEFTYPE(TNAME, DESC) TNAME ,
    #include "tree.def"
    #undef DEFTYPE
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM) TNAME ,
    #include "ctypes.def"
    #undef DEFCTYPE
};

typedef enum {sstruct, uunion} compound_type;

#define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT, FFMEM)   \
    typedef STDINTSZ ETYPE##_t;
#include "ctypes.def"
#undef DEFCTYPE

static const inline char *tree_type_string(enum tree_type t)
 {
    switch (t) {
        #define DEFTYPE(ETYPE, DESC)    \
            case ETYPE:                 \
                return #ETYPE;
        #include "tree.def"
        #undef DEFTYPE
#define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT, FFMEM)     \
            case ETYPE:                                 \
                return #ETYPE;
        #include "ctypes.def"
        #undef DEFCTYPE
        default:
            return "unknown";
    }
}

union value {
#define DEFCTYPE(TNAME, DESC, STDINTSZ, FMT, FFMEM)   \
        STDINTSZ TNAME;
    #include "ctypes.def"
    #undef DEFCTYPE
};

/* The underlying type that is used to represent a live enumeration
 * value. */
#define ENUMTYPE D_T_UINT

#include "tree-base.h"

struct tree_locus {
    size_t line_no;
    size_t column_no;
    tree file;
};

#define tLOCUS(obj) ((obj)->locus)

struct tree {
    enum tree_type type;
    struct tree_data data;
    struct tree_locus locus;
    list chain;
};

/* These macros access the members of the `tree' struct. */
#define TYPE(obj) (obj)->type
#define _DATA(obj) (obj)->data

static const inline char *tree_description_string(tree t)
{
    switch (TYPE(t)) {
#define DEFTYPE(ETYPE, DESC)                    \
        case ETYPE:                             \
            return DESC;
#include "tree.def"
#undef DEFTYPE
#define DEFCTYPE(ETYPE, DESC, STDINTSZ, FMT, FFMEM) \
        case ETYPE:                                 \
            return DESC;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        return "unknown";
    }
}

static inline tree tree_check(tree obj, enum tree_type type,
                              const char *file,
                              int line,
                              const char *function)
{
    if (TYPE (obj) != type) {
        fprintf(stderr, "Fatal error: %s:%d %s: Tree type miss-match; have %s, should be %s\n",
                file, line, function, tree_type_string(TYPE(obj)),
                tree_type_string(type));
        exit(1);
    }

    return obj;
}

tree tree_make(enum tree_type);
tree get_identifier(const char *name);

static inline tree tree_chain_head(tree head)
{
    tree head_marker = tree_make(CHAIN_HEAD);
    list_add(&head->chain, &head_marker->chain);
    return head_marker;
}

static inline tree make_pointer_type(tree ptr, tree type)
{
    tree ret = ptr;

    /* If there is no pointer, the type is just type. */
    if (!ptr)
        return type;

    while (tPTR_EXP(ptr))
        ptr = tPTR_EXP(ptr);

    tPTR_EXP(ptr) = type;

    return ret;
}

static inline void tree_chain(tree new, tree chain)
{
    list_add_tail(&new->chain, &chain->chain);
}

#define for_each_tree(pos, head)                \
    list_for_each((pos), &(head)->chain, chain)

#define for_each_tree_safe(pos, tmp, head)      \
    list_for_each_safe((pos), (tmp), &(head)->chain, chain)

static inline void tree_splice_chains(tree chain_dest, tree chain_src)
{
    tree i, n;

    for_each_tree_safe(i, n, chain_src) {
        list_del(&i->chain);
        tree_chain(i, chain_dest);
    }
}

void tree_dump(tree tree);

#define DEFTYPE(ETYPE, DESC)                    \
    static inline int is_##ETYPE (tree t)       \
    {                                           \
        if (!t)                                 \
            return 0;                           \
                                                \
        if (t->type == ETYPE)                   \
            return 1;                           \
        else                                    \
            return 0;                           \
    }
#include "tree.def"
#undef DEFTYPE

#define DEFCTYPE(ETYPE, DESC, FMT, CTYPE, FFMEM)      \
    static inline int is_##ETYPE (tree t)       \
    {                                           \
        if (!t)                                 \
            return 0;                           \
                                                \
        if (t->type == ETYPE)                   \
            return 1;                           \
        else                                    \
            return 0;                           \
    }
#include "ctypes.def"
#undef DEFCTYPE

static inline int is_CTYPE(tree t)
{
    if (!t)
        return 0;

    switch (t->type)
    {
#define DEFCTYPE(ETYPE, DESC, FMT, CTYPE, FFMEM)  \
        case ETYPE:
#include "ctypes.def"
#undef DEFCTYPE
        return 1;
    default:
        return 0;
    }
}

#define tree_make_binmod(op, binopprefix, left, right)   \
    ({                                                   \
        tree mod = tree_make(op);                        \
        tree assign = tree_make(T_ASSIGN);               \
                                                         \
        binopprefix##_LHS(mod) = (left);                 \
        binopprefix##_RHS(mod) = (right);                \
                                                         \
        tASSIGN_LHS(assign) = (left);                    \
        tASSIGN_RHS(assign) = (mod);                     \
                                                         \
        assign;                                          \
    })

static inline int is_LIVE(tree exp)
{
    if (is_T_LIVE_VAR(exp) || is_T_LIVE_COMPOUND(exp))
        return 1;
    else
        return 0;
}

/* Copy the contents of one tree to another and overwrite.  NOTE this
 * does not do a deep copy! */
static inline void tree_copy(tree dest, tree src)
{
    memcpy(dest, src, sizeof(*dest));
}

#endif
