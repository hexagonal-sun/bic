#ifndef __TREE_ACCESS_H_
#define __TREE_ACCESS_H_

#include "config.h"

#ifdef ENABLE_TREE_CHECKS
#define TREE_CHECK(obj, type)                                           \
    (tree_check((obj), (type), __FILE__, __LINE__, __func__))
#else
#define TREE_CHECK(obj, type) (obj)
#endif

/* Access integer objects contents. */
#define tINT(obj) (_DATA( TREE_CHECK((obj), T_INTEGER) ).integer)

/* Access floating point object contents. */
#define tFLOAT(obj) (_DATA( TREE_CHECK((obj), T_FLOAT) ).ffloat)

/* Access string object contents. */
#define tSTRING(obj) (_DATA( TREE_CHECK((obj), T_STRING) ).string)

/* Below are all objects that have data stored in the 'exp' member of
 * the tree union. */
#define tSZOF_EXP(obj) (_DATA( TREE_CHECK((obj), T_SIZEOF) ).exp)
#define tFNARG_EXP(obj) (_DATA( TREE_CHECK((obj), T_FN_ARG) ).exp)
#define tDTPTR_EXP(obj) (_DATA( TREE_CHECK((obj), D_T_PTR) ).exp)
#define tEXTERN_EXP(obj) (_DATA( TREE_CHECK((obj), T_EXTERN) ).exp)
#define tTYPEDEF_EXP(obj) (_DATA( TREE_CHECK((obj), T_TYPEDEF) ).exp)
#define tPINC_EXP(obj) (_DATA( TREE_CHECK((obj), T_P_INC) ).exp)
#define tPDEC_EXP(obj) (_DATA( TREE_CHECK((obj), T_P_DEC) ).exp)
#define tDEC_EXP(obj) (_DATA( TREE_CHECK((obj), T_DEC) ).exp)
#define tINC_EXP(obj) (_DATA( TREE_CHECK((obj), T_INC) ).exp)
#define tRET_EXP(obj) (_DATA( TREE_CHECK((obj), T_RETURN) ).exp)
#define tSTRUCT_EXP(obj) (_DATA ( TREE_CHECK((obj), T_STRUCT)).exp)
#define tUNION_EXP(obj) (_DATA (TREE_CHECK((obj), T_UNION) ).exp)
#define tSTATIC_EXP(obj) (_DATA ( TREE_CHECK((obj), T_STATIC)).exp)
#define tDEREF_EXP(obj) (_DATA (TREE_CHECK((obj), T_DEREF)).exp)
#define tADDR_EXP(obj) (_DATA (TREE_CHECK((obj), T_ADDR)).exp)
#define tPTR_EXP(obj) (_DATA (TREE_CHECK((obj), T_POINTER)).exp)

#endif
