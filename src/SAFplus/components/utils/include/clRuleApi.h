/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : utils
 * File        : clRuleApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module contains common RBE definitions.
 *
 *
 *****************************************************************************/


/********************************************************************************/
/************************************ Rule APIs *********************************/
/********************************************************************************/
/*                                                                              */
/*                                                                              */
/*  clRuleExprAllocate                                                          */
/*  clRuleExprDeallocate                                                        */
/*  clRuleExprAppend                                                            */
/*  clRuleExprDuplicate                                                         */
/*  clRuleExprEvaluate                                                          */
/*  clRuleDoubleExprEvaluate                                                    */
/*  clRuleExprLocalConvert                                                      */
/*  clRuleExprConvert                                                           */
/*  clRuleExprFlagsSet                                                          */
/*  clRuleExprOffsetSet                                                         */
/*  clRuleExprMaskSet                                                           */
/*  clRuleExprValueSet                                                          */
/*  clRuleExprFlagsGet                                                          */
/*  clRuleExprOffsetGet                                                         */
/*  clRuleExprMaskGet                                                           */
/*  clRuleExprValueGet                                                          */
/*  clRuleExprMemLenGet                                                         */
/*  clRuleExprPack                                                              */
/*  clRuleExprUnpack                                                            */
/*  clRuleExprPrint                                                             */
/*                                                                              */
/********************************************************************************/

/**
 * \file 
 * \brief Header file of RBE related APIs
 * \ingroup rule_apis
 */

/**
 * \addtogroup rule_apis
 * \{
 */

#ifndef _CL_RULE_API_H_
#define _CL_RULE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 *  Macros to convert big-to-little endian and vice versa.
 */
#define CL_RULE_SWAP32(x)  ( (((x)>>24)&0xFF) | \
                         (((x)>>8)&0xFF00) | \
                         (((x)&0xFF00)<<8) | \
                         (((x)&0xFF)<<24) )

#define CL_RULE_SWAP16(x)  ( (((x)>>8)&0xFF) | \
                         (((x)<<8)&0xFF00) )


/** @pkg cl.rbe */

/*******************************************************************************
 *
 * Rule Based Engine Library.
 *
 * <h1>Overview</h1>
 *
 * The Rule Based Engine (RBE) provides mechanism to create filters based on
 * simple expression. An expression is made of a mask and a value. The
 * expression is on user data. Expression evaluation consists of
 * applying the mask on a given offset in the user data and comparing the
 * result with the value. The expression evaluates to TRUE if the comparision
 * is successful.
 *
 * <h1>Interaction with other components</h1>
 *
 * The RBE engine forms a general purpose library that could be used any
 * any other software component. The RBE library doesn't depend on any
 * components.
 *
 * <h1>Configuration</h1>
 *
 * No explicit configuration is needed for RBE.
 *
 * <h1>Usage Scenario(s)</h1>
 *
 * The RBE is used to filter or select a subset of objects from a group.
 * For example, group object implements "filtered walk" using RBE which
 * is used by EM to select qualified subscribers to an event based on
 * the event data. Similarly IOC provides received message filtering
 * using RBE.
 *
 * Following are some examples on how to use the RBE functions.
 * <blockquote>
 * <pre>
 *    **
 *    * Let's say that we have a message header - an array of bytes and
 *    * the RBE is used to look for a perticulr pattern - viz
 *    * message from a perticular destination address and a
 *    * perticular tag.
 *    *
 *    * {
 *    * ClRuleExprT* expr = clRuleExprAllocate(3);
 *    *
 *    * clRuleExprOffsetSet(expr,0);
 *    * clRuleExprMaskSet(expr,0,0xFFFFFFFF);
 *    * clRuleExprMaskSet(expr,2,0xFFFF0000);
 *    *
 *    * clRuleExprValueSet(expr,0,0xABCDABCD);
 *    * clRuleExprValueSet(expr,2,0xDEAD0000);
 *    *
 *    *  Rbe expression, user data, data length = 2 32 bit units
 *    * if( clRuleExprEvaluate(expr, &msgHdr, 2) )
 *    *   {
 *    *     We have received a msg with ABCDABCD as the fist 4 bytes and
 *    *       DEAD as the next two bytes.
 *    *   }
 *    * }
 *
 * <pre/>
 * <blockquote/>
 *
 * @pkgdoc  cl.rbe
 * @pkgdoctid Rule Based Engine Library.
 *****************************************************************************/

#define CL_RULE_EXPR_FLAG_BITS    (2)
#define CL_RULE_ARCH_FLAG_MASK     (0x3)
#define CL_RULE_EXPR_FLAG_MASK     (~(CL_RULE_ARCH_FLAG_MASK))
#define CL_RULE_EXPR_FLAG_DEFAULT  (CL_RULE_NON_ZERO_MATCH | CL_RULE_EXPR_CHAIN_AND)

/******************************************************************************
 *  Data Types
 *****************************************************************************/

/**
 *  Its filters acc,  filter the data based on the rule.Expression qualification.
 */
typedef enum
{

/**
 *  Little endian.
 */
    CL_RULE_LITTLE_END = 0x1,

/**
 *  Big endian.
 */
    CL_RULE_BIG_END = 0x2,

/**
 *  Non-zero match.
 */
    CL_RULE_NON_ZERO_MATCH = 0x4,

/**
 *  Exact match.
 */
    CL_RULE_MATCH_EXACT    = 0x8,

/**
 *  Appending two RBE expression with AND relation.
 */
    CL_RULE_EXPR_CHAIN_AND = 0x10,

/**
 *  Appending a grouping expression with a AND/OR
 */
    CL_RULE_EXPR_CHAIN_GROUP_OR  = 0x40,
/**
 *  Appending two RBE expression with OR relation.
 */
} ClRuleExprFlagsT;

/**
 *  RBE result enum.
 */
typedef enum
{

/**
 *  Expression evaluated failed.
 */
    CL_RULE_FALSE = 0,

/**
 *  Expression evaluation passed.
 */
    CL_RULE_TRUE,

/**
 *  Unknown result (not used).
 */
    CL_RULE_UNKNOWN
} ClRuleResultT;


/**
 *  Rule to filter data.Expression definition.
 */
typedef struct ClRuleExpr
{

/**
 *  Architecture and other flags.
 */
    ClUint8T flags;

/**
 *  Expression length in multiples of four bytes.
 */
    ClUint8T len;

/**
 *  Offset with the data where expression is applied.
 */
    ClUint16T offset;

/**
 *  Multiple RBEs can be chained to build complex expression.
 */
 struct ClRuleExpr *next;
#define maskByte BI_u.Byte
#define maskInt BI_u.Int
    union {
/**
 *  Expression mask and value array.
 */
        ClUint8T Byte[1];
        ClUint32T Int[1];
    } BI_u;
} ClRuleExprT;


/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \brief Allocates RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *  libClUtils
 *
 *  \param len Length of the mask or value in multiple of four bytes.
 *  \param ppExpr (out) Allocated expression is returned here.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER Error
 *  \retval CL_ERR_NO_MEMORY Error
 *
 *  \par Description:
 *  This function allocates a RBE expression and initializes it
 *  appropriately.
 *
 *  \sa clRuleExprDeallocate()
 *
 */
ClRcT clRuleExprAllocate (CL_IN ClUint8T len,
                              CL_OUT ClRuleExprT** ppExpr);

/**
 ************************************
 *  \brief Frees an RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param Expr RBE Expression to be freed.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER when the pExpr is NULL.
 *
 *  \par Description:
 *  This function Frees an RBE Expression (memory or structures used by RBE expression).
 *
 *  \sa clRuleExprAllocate()
 *
 */
ClRcT clRuleExprDeallocate (CL_IN ClRuleExprT* pExpr);

/**
 ************************************
 *  \brief Appends a RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pFirstExpr RBE Expression on which pNextExpr needs to be appended.
 *  \param pNextExpr RBE Expression which will be appended to pFirstExpr.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If any one of the two input expressions is null.
 *
 *  \par Description:
 *  This function appends a expression to another. It is used
 *  to create more complex expression by combining existing expressions.
 *
 *  \sa clRuleExprAllocate()
 *
 */
ClRcT clRuleExprAppend (CL_IN ClRuleExprT* pFirstExpr, CL_IN ClRuleExprT* pNextExpr);

/*
 ************************************
 *  \brief Duplicates a RBE Expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pSrcExpr Source RBE Expression to Copy.
 *  \param ppDstExpr Pointer to a new copy of the expression.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If any one of the two input expressions is null.
 *
 *  \par Description:
 *  This function makes a copy of the given RBE Expression.
 *  The API allocates the needed memory that must be freed
 *  by the caller using clRuleExprDeallocate().
 *
 *  \sa clRuleExprAllocate(), clRuleExprDeallocate()
 *
 */
ClRcT clRuleExprDuplicate (CL_IN ClRuleExprT* pSrcExpr, CL_OUT ClRuleExprT** ppDstExpr);


/**
 ************************************
 *  \brief Evaluates a complex RBE Expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression to be evaluated.
 *  \param pData Data pointer against which the RBE needs to be compared.
 *  \param dataLen Length of the data in multiples of 4 bytes.
 *
 *  \retval CL_RULE_TRUE On true.
 *  \retval CL_RULE_FALSE On false.
 *
 *  \par Description:
 *  This function evaluates a RBE Expression. RBE expression
 *  could be a complex expression, i.e., multiple expressions
 *  can be chained together against a flat buffer pData of
 *  dataLen length.
 *
 *  \sa clRuleDoubleExprEvaluate()
 *
 */
ClRuleResultT  clRuleExprEvaluate(CL_IN ClRuleExprT* pExpr, CL_IN ClUint32T *pData, CL_IN int dataLen);


/**
 ************************************
 *  \brief Evaluates Double RBE Expressions.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr1 First RBE Expression to be evaluated.
 *  \param pExpr2 Second RBE Expression to be evaluated.
 *  \param dataLen Length of the data in multiples of 4 bytes.
 *
 *  \retval CL_RULE_TRUE On true.
 *  \retval CL_RULE_FALSE On false.
 *
 *  \par Description:
 *  This function evaluates a RBE Expression against another RBE expr.
 *  Both expressions are assumed to be of simple type.
 *
 *  \sa clRuleExprEvaluate()
 *
 */
ClRuleResultT  clRuleDoubleExprEvaluate (CL_IN ClRuleExprT* pExpr1, CL_IN ClRuleExprT* pExpr2);


/*
 ************************************
 *  \brief Converts a RBE expression to local endianess.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression to be converted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_RC_ERROR On error.
 *
 *  \par Description:
 *  This function converts a RBE expression to match local endianess.
 *
 *  \sa clRuleExprConvert()
 *
 */
ClRcT clRuleExprLocalConvert (CL_IN ClRuleExprT* pExpr);


/*
 ************************************
 *  \brief Endian converts a complex RBE Expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression to be converted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_RC_ERROR On error.
 *
 *  \par Description:
 *  This function converts endianness of a complex
 *  RBE expression.
 *
 *  \sa clRuleExprLocalConvert()
 *
 */
ClRcT clRuleExprConvert (CL_IN ClRuleExprT* pExpr);


/**
 ************************************
 *  \brief Sets Flags of an RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param flags Flags to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If pExpr is null.
 *
 *  \par Description:
 *  This function sets the RBE expression flags as specified by
 *  the parameter flags.
 *
 *  \sa clRuleExprFlagsGet()
 *
 */
ClRcT clRuleExprFlagsSet (CL_IN ClRuleExprT* pExpr, CL_IN ClRuleExprFlagsT flags);


/**
 ************************************
 *  \brief Sets offset of a RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param offset Offset value (multiples of 4 bytes).
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If pExpr is null.
 *
 *  \par Description:
 *  This function sets the offset field of a RBE expression
 *  as specified by the offset parameter.
 *
 *  \sa clRuleExprOffsetGet()
 *
 */
ClRcT clRuleExprOffsetSet (CL_IN ClRuleExprT* pExpr, CL_IN ClUint16T offset);


/**
 ************************************
 *  \brief Set Mask of an RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param offset Offset at which the mask to be set.
 *  \param mask Mask value to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If pExpr is null.
 *
 *  \par Description:
 *  This function sets the Mask field of an RBE expression
 *  as specified by the parameter mask.
 *
 *  \note
 *  Offset is in multiple of 4 bytes. Offset here is overall offset,
 *  i.e., mask set is expr->mask[offset - expr->offset].
 *
 *  \sa clRuleExprMaskGet()
 *
 */
ClRcT clRuleExprMaskSet (CL_IN ClRuleExprT* pExpr, CL_IN ClUint16T offset, CL_IN ClUint32T mask);


/**
 ************************************
 *  \brief Set Value of a RBE expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param offset Offset at which the value is to be set.
 *  \param mask Mask value to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If pExpr is null.
 *
 *  \par Description:
 *  This function sets the value field of a RBE expression
 *  to the value specified by the parameter value.
 *
 *  \note
 *  Offset is in multiple of 4 bytes. Offset here is overall offset,
 *  i.e., mask set is expr->mask[offset - expr->offset].
 *
 *  \sa clRuleExprValueGet()
 *
 */
ClRcT clRuleExprValueSet (CL_IN ClRuleExprT* pExpr, CL_IN ClUint16T offset, CL_IN ClUint32T value);


/**
 ************************************
 *  \brief Gets RBE expression flags.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param pFlags Flags to be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If pExpr is null.
 *
 *  \par Description:
 *  This function gets the flags of a RBE expression.
 *
 *  \sa clRuleExprFlagsSet()
 *
 */
ClRcT clRuleExprFlagsGet (CL_IN ClRuleExprT* pExpr, CL_OUT ClRuleExprFlagsT *pFlags);


/**
 ************************************
 *  \brief Get RBE expression Offset value.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr  RBE Expression for which flags to be set.
 *  \param pOffset Offset to be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If any one of the two input expressions is null.
 *
 *  \par Description:
 *  This function gets the offset value of a RBE expression
 *  set using clRuleExprOffsetSet().
 *
 *  \sa clRuleExprOffsetSet()
 *
 */
ClRcT clRuleExprOffsetGet (CL_IN ClRuleExprT* pExpr, CL_OUT ClUint16T *pOffset);


/**
 ************************************
 *  \brief Get RBE expression Mask value.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression for which flags to be set.
 *  \param offset Get Mask from this Offset.
 *  \param pMask Mask to be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If any one of the two input expressions is null.
 *
 *  \par Description:
 *  This function gets the Mask of an RBE expression
 *  set using clRuleExprMaskSet().
 *
 *  \note
 *  Offset is in multiple of 4 bytes. Offset here is overall offset,
 *  i.e., mask returned is expr->mask[offset - expr->offset].
 *
 *  \sa clRuleExprMaskSet()
 *
 */
ClRcT clRuleExprMaskGet (CL_IN ClRuleExprT* pExpr, CL_IN ClUint16T offset, CL_OUT ClUint32T *pMask);


/**
 ************************************
 *  \brief Get RBE expression value.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr  RBE Expression for which flags to be set.
 *  \param offset Get Mask from this Offset.
 *  \param pValue value to be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER If any one of the two input expressions is null.
 *
 *  \par Description:
 *  This function gets the value of a RBE expression
 *  set using clRuleExprValueSet().
 *
 *  \note
 *  Offset is in multiple of 4 bytes. Offset here is overall offset,
 *  i.e., mask returned is expr->mask[offset - expr->offset].
 *
 *  \sa clRuleExprValueSet()
 *
 */
ClRcT clRuleExprValueGet (CL_IN ClRuleExprT* pExpr, CL_IN ClUint16T offset, CL_OUT ClUint32T *pValue);


/**
 ************************************
 *  \brief Get the total memory used by the expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr  RBE Expression.
 *
 *  \par Return Value:
 *  Length in bytes needed to pack an expression.
 *
 *  \par Description:
 *  This function returns the memory needed to pack an
 *  expression into contiguous memory location.
 *
 *  \sa clRuleExprPack()
 *
 */
ClUint32T clRuleExprMemLenGet (CL_IN ClRuleExprT* pExpr);


/**
 ************************************
 *  \brief Pack an RBE Expression into the given memory area.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pSrcExpr Source RBE Expression to pack.
 *  \param pBuf Pointer to the memory location where to pack.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On failure.
 *
 *  \par Description:
 *  This function packs the given RBE Expression.
 *  The expression is packed at the given memory pointed by pBuf.
 *  It is assumed that the pBuf contains enough space to accomodate
 *  srcExpr. Caller can call clRuleExprMemLenGet() to get
 *  the memory needed to pack the srcExpr.
 *
 *  \sa clRuleExprMemLenGet(), clRuleExprUnpack()
 *
 */
ClRcT clRuleExprPack(CL_IN ClRuleExprT *pSrcExpr,
                     CL_OUT ClUint8T **ppBuf,
                     CL_OUT ClUint32T *pLen);


/**
 ************************************
 *  \brief UnPack a RBE Expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pSrcExpr Source RBE Expression to unpack.
 *  \param pDstExpr Pointer to a new copy of the expression.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On failure.
 *
 *  \par Description:
 *  This function makes unpack of the given RBE Expression.
 *
 *  \sa clRuleExprPack()
 *
 */
ClRcT clRuleExprUnpack(CL_IN ClUint8T *pBuf,
                       CL_IN ClUint32T len,
                       CL_OUT ClRuleExprT **ppDstExpr);


/**
 ************************************
 *  \brief Prints a  complex RBE Expression.
 *
 *  \par Header File:
 *  clRuleApi.h
 *
 *  \par Library Name:
 *
 *  \param pExpr RBE Expression to be printed.
 *
 *  \par Return Value:
 *  None.
 *
 *  \par Description:
 *  This function prints the given RBE Expression
 *  which could be a complex expression.
 *
 *  \par Related APIs:
 *  None.
 *
 */
ClRcT clRuleExprPrint (CL_IN ClRuleExprT* pExpr);

#ifdef __cplusplus
}
#endif

#endif /* _CL_RULE_API_H_ */

/** \} */
