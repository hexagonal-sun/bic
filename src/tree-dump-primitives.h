#ifndef __TREE_DUMP_PRIMITIVES__
#define __TREE_DUMP_PRIMITIVES__

static inline void tree_dump_ptr(tree t, void *ptr)
{
    fprintf(stderr, "%p", ptr);
}

static inline void tree_dump_int(tree t, int s)
{
    fprintf(stderr, "%d", s);
}

static inline void tree_dump_flag(tree t, bool flag)
{
    if (flag)
        fprintf(stderr, "true");
    else
        fprintf(stderr, "false");
}

static inline void tree_dump_string(tree t, const char *s)
{
    fprintf(stderr, "\"%s\"", s);
}

static inline void tree_dump_jmp_buf(tree t, jmp_buf b)
{
    return;
}

static inline void tree_dump_sz(tree t, size_t s)
{
    fprintf(stderr, "%zu", s);
}

static inline void tree_dump_live_var_val(tree t, union value *v)
{
    tree lv_type = tLV_TYPE(t);

#define DEFCTYPE(TNAME, DESC, CTYPE, FMT, FFMEM)         \
    if (is_##TNAME(lv_type)) {                           \
        fprintf(stderr, "%" #FMT, v->TNAME);             \
        return;                                          \
    }
#include "ctypes.def"
#undef DEFCTYPE

    fprintf(stderr, "<Unprintable>");
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
