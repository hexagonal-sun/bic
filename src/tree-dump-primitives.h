#ifndef __TREE_DUMP_PRIMITIVES__
#define __TREE_DUMP_PRIMITIVES__

enum DUMP_TYPE
{
    TEXTUAL,
    DOT,
};

static inline void tree_dump_ptr(tree t, void *ptr, enum DUMP_TYPE dt)
{
    fprintf(stderr, "%p", ptr);
}

static inline void tree_dump_int(tree t, int s, enum DUMP_TYPE dt)
{
    fprintf(stderr, "%d", s);
}

static inline void tree_dump_flag(tree t, bool flag, enum DUMP_TYPE dt)
{
    if (flag)
        fprintf(stderr, "true");
    else
        fprintf(stderr, "false");
}

static inline void tree_dump_string(tree t, const char *s, enum DUMP_TYPE dt)
{
    if (dt == TEXTUAL)
        fprintf(stderr, "\"%s\"", s);
    else
        fprintf(stderr, "%s", s);
}

static inline void tree_dump_jmp_buf(tree t, jmp_buf b, enum DUMP_TYPE dt)
{
    return;
}

static inline void tree_dump_sz(tree t, size_t s, enum DUMP_TYPE dt)
{
    fprintf(stderr, "%zu", s);
}

static inline void tree_dump_live_var_val(tree t, union value *v, enum DUMP_TYPE dt)
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

static inline void tree_dump_ffloat(tree t, mpf_t val, enum DUMP_TYPE dt)
{
    gmp_fprintf(stderr, "%Ff", val);
}

static inline void tree_dump_integer(tree t, mpz_t val, enum DUMP_TYPE dt)
{
    gmp_fprintf(stderr, "%Zd", val);
}

static inline void tree_dump_ffi_closure(tree t, ffi_closure *closure, enum DUMP_TYPE dt)
{
    return;
}
 
static inline void tree_dump_ffi_types(tree t, ffi_type **types,
                                       enum DUMP_TYPE dt) {
    return;
}

static inline void tree_dump_ffi_cif(tree t, ffi_cif *closure,
                                         enum DUMP_TYPE dt)
{
    return;
}

static inline void tree_dump_comp_decl_type(tree t, compound_type v, enum DUMP_TYPE dt)
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
