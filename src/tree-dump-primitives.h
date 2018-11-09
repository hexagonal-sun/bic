#ifndef __TREE_DUMP_PRIMITIVES__
#define __TREE_DUMP_PRIMITIVES__

static inline void tree_dump_ptr(tree t, void *ptr)
{
    fprintf(stderr, "%p", ptr);
}

#define tree_dump_eval_alloc_ptr tree_dump_ptr
#define tree_dump_live_comp_base tree_dump_ptr

static inline void tree_dump_int(tree t, int s)
{
    fprintf(stderr, "%d", s);
}

#define tree_dump_comp_decl_expanded tree_dump_int
#define tree_dump_live_var_is_array tree_dump_int
#define tree_dump_eval_ctx_is_static tree_dump_int

static inline void tree_dump_string(tree t, const char *s)
{
    fprintf(stderr, "\"%s\"", s);
}

#define tree_dump_name tree_dump_string
#define tree_dump_eval_ctx_name tree_dump_string

static inline void tree_dump_szt(tree t, size_t s)
{
    fprintf(stderr, "%zu", s);
}

#define tree_dump_comp_decl_sz tree_dump_szt
#define tree_dump_decl_offset tree_dump_szt
#define tree_dump_live_var_array_length tree_dump_szt

static inline void tree_dump_live_var_val(tree t, union value *v)
{
    switch (TYPE(t)) {
#define DEFCTYPE(TNAME, DESC, CTYPE, FMT)                \
        case TNAME:                                      \
            fprintf(stderr, "%" #FMT, v->TNAME);         \
            break;
#include "ctypes.def"
#undef DEFCTYPE
    default:
        fprintf(stderr, "<Unprintable>");
        break;
    }
}

static inline void tree_dump_ffloat(tree t, mpf_t val)
{
    gmp_fprintf(stderr, "%Ff", val);
}

static inline void tree_dump_integer(tree t, mpz_t val)
{
    gmp_fprintf(stderr, "%Zd", val);
}

static inline void tree_dump_comp_decl_type(tree t, compound_type v)
{
    switch(v) {
    case sstruct:
        fputs("struct", stderr);
        break;
    case uunion:
        fputs("union", stderr);
        break;
    }
}

#endif
