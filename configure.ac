#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bic], [v0.9], [dev@mattleach.net])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

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

# Checks for libraries.
AX_LIB_READLINE
AC_CHECK_LIB(gmp, __gmpz_init, ,
       [AC_MSG_ERROR([GNU MP not found, see https://gmplib.org/])])


# Checks for header files.
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h \
                  stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.o
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Configuration options for the program
BIC_PROMPT="BIC> "
AC_DEFINE_UNQUOTED([BIC_PROMPT], ["$BIC_PROMPT"],
                        [Interactive prompt for BIC])
AC_SUBST([BIC_PROMPT])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([memset strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 testsuite/Makefile
                 testsuite/prompt.exp])

AC_OUTPUT
