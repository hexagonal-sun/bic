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

/* Below are all objects that have data stored in the 'left' and
 * 'right' members of the tree union. */
#define tEMAP_LEFT(obj) (_DATA( TREE_CHECK((obj), E_MAP)).bin.left)
#define tEMAP_RIGHT(obj) (_DATA( TREE_CHECK((obj), E_MAP)).bin.right)

#define tARRAY_ID(obj) (_DATA( TREE_CHECK((obj), T_ARRAY)).bin.left)
#define tARRAY_SZ(obj) (_DATA( TREE_CHECK((obj), T_ARRAY)).bin.right)

#define tASSIGN_LHS(obj) (_DATA( TREE_CHECK((obj), T_ASSIGN)).bin.left)
#define tASSIGN_RHS(obj) (_DATA( TREE_CHECK((obj), T_ASSIGN)).bin.right)

#define tADD_LHS(obj) (_DATA( TREE_CHECK((obj), T_ADD)).bin.left)
#define tADD_RHS(obj) (_DATA( TREE_CHECK((obj), T_ADD)).bin.right)

#define tSUB_LHS(obj) (_DATA( TREE_CHECK((obj), T_SUB)).bin.left)
#define tSUB_RHS(obj) (_DATA( TREE_CHECK((obj), T_SUB)).bin.right)

#define tMUL_LHS(obj) (_DATA( TREE_CHECK((obj), T_MUL)).bin.left)
#define tMUL_RHS(obj) (_DATA( TREE_CHECK((obj), T_MUL)).bin.right)

#define tDIV_LHS(obj) (_DATA( TREE_CHECK((obj), T_DIV)).bin.left)
#define tDIV_RHS(obj) (_DATA( TREE_CHECK((obj), T_DIV)).bin.right)

#define tLT_LHS(obj) (_DATA( TREE_CHECK((obj), T_LT)).bin.left)
#define tLT_RHS(obj) (_DATA( TREE_CHECK((obj), T_LT)).bin.right)

#define tGT_LHS(obj) (_DATA( TREE_CHECK((obj), T_GT)).bin.left)
#define tGT_RHS(obj) (_DATA( TREE_CHECK((obj), T_GT)).bin.right)

#define tLTEQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_LTEQ)).bin.left)
#define tLTEQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_LTEQ)).bin.right)

#define tGTEQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_GTEQ)).bin.left)
#define tGTEQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_GTEQ)).bin.right)

#define tCOMP_ACCESS_OBJ(obj) (_DATA( TREE_CHECK((obj), T_COMP_ACCESS)).bin.left)
#define tCOMP_ACCESS_MEMBER(obj) (_DATA( TREE_CHECK((obj), T_COMP_ACCESS)).bin.right)

#define tARR_ACCESS_OBJ(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_ACCESS)).bin.left)
#define tARR_ACCESS_IDX(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_ACCESS)).bin.right)

/* Evaluation ctx (E_CTX) accessor macros. */
#define tID_MAP(obj) (_DATA( TREE_CHECK((obj), E_CTX)).ectx.id_map)
#define tPARENT_CTX(obj) (_DATA( TREE_CHECK((obj), E_CTX)).ectx.parent_ctx)
#define tALLOC_CHAIN(obj) (_DATA( TREE_CHECK((obj), E_CTX)).ectx.alloc_chain)
#define tCTX_NAME(obj) (_DATA( TREE_CHECK((obj), E_CTX)).ectx.name)
#define tIS_STATIC(obj) (_DATA( TREE_CHECK((obj), E_CTX)).ectx.is_static)

/* E_ALLOC accessor macros. */
#define tALLOC_PTR(obj) (_DATA( TREE_CHECK((obj), E_ALLOC)).ptr)

/* T_IDENTIFIER accessor macros. */
#define tID_STR(obj) (_DATA( TREE_CHECK((obj), T_IDENTIFIER)).id.name)

/* T_COMP_DECL accessor macros. */
#define tCOMP_DECL_ID(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).comp_decl.id)
#define tCOMP_DECL_DECLS(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).comp_decl.decls)
#define tCOMP_DECL_SZ(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).comp_decl.length)
#define tCOMP_DECL_EXPANDED(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).comp_decl.expanded)
#define tCOMP_DECL_TYPE(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).comp_decl.type)

/* T_LIVE_VAR accessor macros. */
#define tLV_TYPE(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).var.type)
#define tLV_VAL(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).var.val)
#define tLV_IS_ARRAY(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).var.is_array)
#define tLV_ARRAY_SZ(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).var.array_length)

/* T_FN_CALL accessor macros. */
#define tFNCALL_ID(obj) (_DATA( TREE_CHECK((obj), T_FN_CALL)).fncall.identifier)
#define tFNCALL_ARGS(obj) (_DATA( TREE_CHECK((obj), T_FN_CALL)).fncall.arguments)

/* T_FN_DEF accessor macros. */
#define tFNDEF_NAME(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function.id)
#define tFNDEF_RET_TYPE(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function.return_type)
#define tFNDEF_ARGS(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function.arguments)
#define tFNDEF_STMTS(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function.stmts)

#endif
