#ifndef __TREE_ACCESS_H_
#define __TREE_ACCESS_H_

#include "config.h"

#ifdef ENABLE_TREE_CHECKS
#define TREE_CHECK(obj, type)                                           \
    (tree_check((obj), (type), __FILE__, __LINE__, __func__))
#else
#define TREE_CHECK(obj, type) (obj)
#endif

/* Access macros for T_INTEGER objects */
#define tINT_VAL(obj) (_DATA( TREE_CHECK((obj), T_INTEGER)).integer)
#define tSTRING_VAL(obj) (_DATA( TREE_CHECK((obj), T_STRING)).string)

/* Access floating point object contents. */
#define tFLOAT(obj) (_DATA( TREE_CHECK((obj), T_FLOAT) ).ffloat)

/* Access string object contents. */

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
#define tEMAP_LEFT(obj) (_DATA( TREE_CHECK((obj), E_MAP)).binary_exp.left)
#define tEMAP_RIGHT(obj) (_DATA( TREE_CHECK((obj), E_MAP)).binary_exp.right)

#define tARRAY_ID(obj) (_DATA( TREE_CHECK((obj), T_ARRAY)).binary_exp.left)
#define tARRAY_SZ(obj) (_DATA( TREE_CHECK((obj), T_ARRAY)).binary_exp.right)

#define tARRAY_TYPE_BASE_TYPE(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_TYPE)).binary_exp.left)
#define tARRAY_TYPE_SZ(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_TYPE)).binary_exp.right)

#define tASSIGN_LHS(obj) (_DATA( TREE_CHECK((obj), T_ASSIGN)).binary_exp.left)
#define tASSIGN_RHS(obj) (_DATA( TREE_CHECK((obj), T_ASSIGN)).binary_exp.right)

#define tADD_LHS(obj) (_DATA( TREE_CHECK((obj), T_ADD)).binary_exp.left)
#define tADD_RHS(obj) (_DATA( TREE_CHECK((obj), T_ADD)).binary_exp.right)

#define tSUB_LHS(obj) (_DATA( TREE_CHECK((obj), T_SUB)).binary_exp.left)
#define tSUB_RHS(obj) (_DATA( TREE_CHECK((obj), T_SUB)).binary_exp.right)

#define tLSHIFT_LHS(obj) (_DATA( TREE_CHECK((obj), T_LSHIFT)).binary_exp.left)
#define tLSHIFT_RHS(obj) (_DATA( TREE_CHECK((obj), T_LSHIFT)).binary_exp.right)

#define tRSHIFT_LHS(obj) (_DATA( TREE_CHECK((obj), T_RSHIFT)).binary_exp.left)
#define tRSHIFT_RHS(obj) (_DATA( TREE_CHECK((obj), T_RSHIFT)).binary_exp.right)

#define tMUL_LHS(obj) (_DATA( TREE_CHECK((obj), T_MUL)).binary_exp.left)
#define tMUL_RHS(obj) (_DATA( TREE_CHECK((obj), T_MUL)).binary_exp.right)

#define tDIV_LHS(obj) (_DATA( TREE_CHECK((obj), T_DIV)).binary_exp.left)
#define tDIV_RHS(obj) (_DATA( TREE_CHECK((obj), T_DIV)).binary_exp.right)

#define tLT_LHS(obj) (_DATA( TREE_CHECK((obj), T_LT)).binary_exp.left)
#define tLT_RHS(obj) (_DATA( TREE_CHECK((obj), T_LT)).binary_exp.right)

#define tGT_LHS(obj) (_DATA( TREE_CHECK((obj), T_GT)).binary_exp.left)
#define tGT_RHS(obj) (_DATA( TREE_CHECK((obj), T_GT)).binary_exp.right)

#define tLTEQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_LTEQ)).binary_exp.left)
#define tLTEQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_LTEQ)).binary_exp.right)

#define tGTEQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_GTEQ)).binary_exp.left)
#define tGTEQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_GTEQ)).binary_exp.right)

#define tEQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_EQ)).binary_exp.left)
#define tEQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_EQ)).binary_exp.right)

#define tN_EQ_LHS(obj) (_DATA( TREE_CHECK((obj), T_N_EQ)).binary_exp.left)
#define tN_EQ_RHS(obj) (_DATA( TREE_CHECK((obj), T_N_EQ)).binary_exp.right)

#define tL_OR_LHS(obj) (_DATA( TREE_CHECK((obj), T_L_OR)).binary_exp.left)
#define tL_OR_RHS(obj) (_DATA( TREE_CHECK((obj), T_L_OR)).binary_exp.right)

#define tL_AND_LHS(obj) (_DATA( TREE_CHECK((obj), T_L_AND)).binary_exp.left)
#define tL_AND_RHS(obj) (_DATA( TREE_CHECK((obj), T_L_AND)).binary_exp.right)

#define tCAST_NEWTYPE(obj) (_DATA( TREE_CHECK((obj), T_CAST)).binary_exp.left)
#define tCAST_EXP(obj) (_DATA( TREE_CHECK((obj), T_CAST)).binary_exp.right)

#define tBITFIELD_EXPR_SZ(obj) (_DATA( TREE_CHECK((obj), T_BITFIELD_EXPR)).binary_exp.left)
#define tBITFIELD_EXPR_DECL(obj) (_DATA( TREE_CHECK((obj), T_BITFIELD_EXPR)).binary_exp.right)

#define tCOMP_ACCESS_OBJ(obj) (_DATA( TREE_CHECK((obj), T_COMP_ACCESS)).binary_exp.left)
#define tCOMP_ACCESS_MEMBER(obj) (_DATA( TREE_CHECK((obj), T_COMP_ACCESS)).binary_exp.right)

#define tARR_ACCESS_OBJ(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_ACCESS)).binary_exp.left)
#define tARR_ACCESS_IDX(obj) (_DATA( TREE_CHECK((obj), T_ARRAY_ACCESS)).binary_exp.right)

#define tIF_COND(obj) (_DATA( TREE_CHECK((obj), T_IF)).if_stmt.condition)
#define tIF_TRUE_STMTS(obj) (_DATA( TREE_CHECK((obj), T_IF)).if_stmt.true_stmts)
#define tIF_ELSE_STMTS(obj) (_DATA( TREE_CHECK((obj), T_IF)).if_stmt.else_stmts)

/* Evaluation ctx (E_CTX) accessor macros. */
#define tID_MAP(obj) (_DATA( TREE_CHECK((obj), E_CTX)).eval_ctx.id_map)
#define tPARENT_CTX(obj) (_DATA( TREE_CHECK((obj), E_CTX)).eval_ctx.parent_ctx)
#define tCTX_NAME(obj) (_DATA( TREE_CHECK((obj), E_CTX)).eval_ctx.name)
#define tIS_STATIC(obj) (_DATA( TREE_CHECK((obj), E_CTX)).eval_ctx.is_static)
#define tALLOC_CHAIN(obj) (_DATA( TREE_CHECK((obj), E_CTX)).eval_ctx.alloc_chain)

/* E_ALLOC accessor macros. */
#define tALLOC_PTR(obj) (_DATA( TREE_CHECK((obj), E_ALLOC)).ptr)

/* T_IDENTIFIER accessor macros. */
#define tID_STR(obj) (_DATA( TREE_CHECK((obj), T_IDENTIFIER)).identifier.name)

/* T_COMP_DECL accessor macros. */
#define tCOMP_DECL_ID(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).compound_decl.id)
#define tCOMP_DECL_DECLS(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).compound_decl.decls)
#define tCOMP_DECL_SZ(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).compound_decl.length)
#define tCOMP_DECL_EXPANDED(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).compound_decl.expanded)
#define tCOMP_DECL_TYPE(obj) (_DATA( TREE_CHECK((obj), T_DECL_COMPOUND)).compound_decl.type)

/* T_LIVE_VAR accessor macros. */
#define tLV_TYPE(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).live_var.type)
#define tLV_VAL(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).live_var.val)
#define tLV_IS_ARRAY(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).live_var.is_array)
#define tLV_ARRAY_SZ(obj) (_DATA( TREE_CHECK((obj), T_LIVE_VAR)).live_var.array_length)

/* T_FN_CALL accessor macros. */
#define tFNCALL_ID(obj) (_DATA( TREE_CHECK((obj), T_FN_CALL)).function_call.identifier)
#define tFNCALL_ARGS(obj) (_DATA( TREE_CHECK((obj), T_FN_CALL)).function_call.arguments)

/* T_FN_DEF accessor macros. */
#define tFNDEF_NAME(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function_data.id)
#define tFNDEF_RET_TYPE(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function_data.return_type)
#define tFNDEF_ARGS(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function_data.arguments)
#define tFNDEF_STMTS(obj) (_DATA( TREE_CHECK((obj), T_FN_DEF)).function_data.stmts)

/* T_FN_DECL accessor macros. */
#define tFNDECL_NAME(obj) (_DATA( TREE_CHECK((obj), T_DECL_FN)).function_data.id)
#define tFNDECL_RET_TYPE(obj) (_DATA( TREE_CHECK((obj), T_DECL_FN)).function_data.return_type)
#define tFNDECL_ARGS(obj) (_DATA( TREE_CHECK((obj), T_DECL_FN)).function_data.arguments)
#define tFNDECL_STMTS(obj) (_DATA( TREE_CHECK((obj), T_DECL_FN)).function_data.stmts)

/* T_DECL accessor macros.  */
#define tDECL_TYPE(obj) (_DATA( TREE_CHECK((obj), T_DECL)).declaration.type)
#define tDECL_DECLS(obj) (_DATA( TREE_CHECK((obj), T_DECL)).declaration.decls)
#define tDECL_OFFSET(obj) (_DATA( TREE_CHECK((obj), T_DECL)).declaration.offset)

/* T_LIVE_COMPOUND macros. */
#define tLV_COMP_DECL(obj) (_DATA( TREE_CHECK((obj), T_LIVE_COMPOUND)).live_compound.decl)
#define tLV_COMP_BASE(obj) (_DATA( TREE_CHECK((obj), T_LIVE_COMPOUND)).live_compound.base)
#define tLV_COMP_MEMBERS(obj) (_DATA( TREE_CHECK((obj), T_LIVE_COMPOUND)).live_compound.members)

/* T_LOOP_FOR macros. */
#define tFLOOP_INIT(obj) (_DATA( TREE_CHECK((obj), T_LOOP_FOR)).for_loop.initialization)
#define tFLOOP_COND(obj) (_DATA( TREE_CHECK((obj), T_LOOP_FOR)).for_loop.condition)
#define tFLOOP_AFTER(obj) (_DATA( TREE_CHECK((obj), T_LOOP_FOR)).for_loop.afterthrought)
#define tFLOOP_STMTS(obj) (_DATA( TREE_CHECK((obj), T_LOOP_FOR)).for_loop.stmts)

/* T_ENUMERATOR macros. */
#define tENUM_NAME(obj) (_DATA( TREE_CHECK((obj), T_ENUMERATOR)).enum_type.id)
#define tENUM_ENUMS(obj) (_DATA( TREE_CHECK((obj), T_ENUMERATOR)).enum_type.enums)

/* T_EXT_FUNC macros. */
#define tEXT_FUNC_NAME(obj) (_DATA( TREE_CHECK((obj), T_EXT_FUNC)).external_func.id)
#define tEXT_FUNC_FNDECL(obj) (_DATA( TREE_CHECK((obj), T_EXT_FUNC)).external_func.fndecl)

/* CPP_INCLUDE macros */
#define tCPP_INCLUDE_STR(obj) (_DATA( TREE_CHECK((obj), CPP_INCLUDE) ).string)

#endif
