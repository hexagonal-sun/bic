#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bic], [v0.9.1], [dev@mattleach.net])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_BUILD

LT_INIT
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])


# Checks for programs.
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_MKDIR_P
AC_PROG_RANLIB
AM_PROG_AS

AC_CHECK_PROG([expect],[expect],[yes],[no])
AM_CONDITIONAL([FOUND_EXPECT], [test "x$expect" = xyes])
AM_COND_IF([FOUND_EXPECT],,[AC_MSG_WARN([Expect not found, disabling REPL test cases.])])

# Checks for libraries.
AX_LIB_READLINE
AC_CHECK_LIB(gmp, __gmpz_init, ,
       [AC_MSG_ERROR([GNU MP not found, see https://gmplib.org/])])

AC_CHECK_LIB(dl, dlsym, ,
       [AC_MSG_ERROR([Dynamic linker not found])])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h \
                  stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.o
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])
AX_CXX_COMPILE_STDCXX_11(, [mandatory])

# Configuration options for the program
BIC_PROMPT="BIC> "
AC_DEFINE_UNQUOTED([BIC_PROMPT], ["$BIC_PROMPT"],
                        [Interactive prompt for BIC])
AC_SUBST([BIC_PROMPT])


case "${build_os}" in
     linux*)
        AC_DEFINE([BUILD_LINUX], [1],
                  [bic is to be built for the Linux OS])
        ;;
     darwin*)
        AC_DEFINE([BUILD_DARWIN], [1],
                  [bic is to be built for the Darwin OS])
        ;;
     *)
        AC_MSG_ERROR(["OS ${build_os} not supported"])
        ;;
esac

BUILD_CPU=${build_cpu}
AC_SUBST([BUILD_CPU])

LT_SYS_SYMBOL_USCORE
if test "x${sys_symbol_underscore}" = xyes; then
AC_DEFINE([SYMBOL_USCORE], [1],
          [defined when compiled symbols are prefiex with and underscore])
fi

# Add debug option that will turn on tree checking.
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug],
        [Enable various debugging features (tree-check)]))

AS_IF([test "x$enable_debug" = "xyes"], [
    AC_DEFINE([ENABLE_TREE_CHECKS], [1],
              [Defined when all tree accesses should be checked])
])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([gethostname memset strdup strerror strstr])

AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 src/Makefile
                 src/x86_64/Makefile
                 src/aarch64/Makefile
                 src/armv7l/Makefile
                 testsuite/Makefile])

AC_OUTPUT
