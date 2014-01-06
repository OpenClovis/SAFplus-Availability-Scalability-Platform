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
 * File        : rule.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements Rule Base Engine (RBE)             
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clBufferApi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clOsalApi.h>
#include <clXdrApi.h>

#include <clRuleApi.h>
#include <clRuleErrors.h>
#include "rule.h"

#define CL_RULE_MSG_BUF_LEN 256 

#define UTIL_LOG_AREA		"UTL"
#define UTIL_LOG_CTX_RULE	"RULE"

/** @pkg  */

/**@#-*/

/* MACROS */

/* FORWARD DECLARATION */

/* GLOBALS */

/* LOCALS */

/**************************************************************************/
/*                                                                        */
/*  NAME: rbeGetEndian                                                    */
/*                                                                        */
/*  DESCRIPTION: Return the local endian type                             */
/*                                                                        */
/*  ARGUMENTS: N/A                                                        */
/*                                                                        */
/*  RETURNS: CL_RULE_LITTLE_END or CL_RULE_BIG_END depending upon the local       */
/* CPU architecture.                                                      */
/*                                                                        */
/*                                                                        */
/**************************************************************************/
static ClRuleExprFlagsT rbeGetEndian()
{
    int i = 1;
    return (*((char *)&i) ? CL_RULE_LITTLE_END : CL_RULE_BIG_END);
}

/**@#+*/

/**
 *                                                                       
 *  Allocate RBE expression. 
 *
 *  This function allocates and initializes a RBE expression.
 *                                                                     
 *  @param len: Length of the mask/value in multiple of 4 bytes. 
 *         pExpr : [OUT] allocated expression is returned here.
 *                                                                     
 *  @returns 
 *           CL_OK if expression is allocated successfully.
 *           CL_RULE_RC(CL_ERR_NULL_POINTER) or CL_RULE_RC(CL_ERR_NO_MEMORY) on error.  
 *
 *  @author clovis
 *  @version 1.0
 *
 *
 */                                                                     
ClRcT clRuleExprAllocate (ClUint8T len, ClRuleExprT* *pExpr)
{
    ClRuleExprT* expr;

    CL_FUNC_ENTER();

    if (pExpr == NULL)
    {
        /* Memory allocation failled!! Add more error handling */
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"NULL Parameter!!\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    expr = (ClRuleExprT*) clHeapCalloc(1,sizeof (ClRuleExprT) + len*8);

    if (expr == NULL)
    {
        /* Memory allocation failled!! Add more error handling */
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"Memory allocation failed!!\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NO_MEMORY));
    }
       
    expr->len = len;
    expr->flags = (rbeGetEndian() | CL_RULE_EXPR_FLAG_DEFAULT);

    *pExpr = expr;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Free a RBE Expression. 
 *
 *  This function Frees a RBE Expression (mem/structures used by RBE exp). 
 *                                                                     
 *  @param Expr RBE Expression to be freed.
 *                                                                     
 *  @returns 
 *          CL_OK on success, CL_RC_ERROR,CL_RULE_RC(CL_ERR_NULL_POINTER) if failed. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT clRuleExprDeallocate (ClRuleExprT* expr)
{
    ClRuleExprT *exprCurr, *exprNext;

    CL_FUNC_ENTER();

    /* should we add signature to the expression so we can do more checks? */
    if (expr == NULL)
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    exprCurr = expr;

    while (exprCurr != NULL)
    {
        exprNext = exprCurr->next;
        clHeapFree (exprCurr);
        exprCurr = exprNext;
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Append a RBE expression. 
 *
 *  This function appends a expression to another. It is used
 * to create more complex expression.
 *                                                                     
 *  @returns 
 *           CL_OK on success, 
 *           CL_RULE_RC(CL_ERR_NULL_POINTER) if any one of the two input
 *  expressions is null.
 *
 *  @author clovis
 *  @version 1.0
 *
 *
 */
ClRcT clRuleExprAppend (ClRuleExprT* first, ClRuleExprT* next)
{
    CL_FUNC_ENTER();

    if ((first == NULL) || (next == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    /* go to the end of the first expression */
    while (first->next != NULL)
        first = first->next;

    first->next = next;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Evaluate a RBE Expression given data. 
 *
 *  This function evaluates a RBE Expression on given data. 
 *  Only the top level expression is evaluated.
 *                                                                     
 *  @param Expr RBE Expression to be evaluated.
 *  @param data Data pointer against which the RBE need to executed. 
 *  @Length length of the data in multiples of 4 bytes.  
 * 
 *  @returns 
 *          CL_RULE_TRUE on true  and CL_RULE_FALSE on false. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRuleResultT rbeExprEvaluateOne (ClRuleExprT *expr, ClUint32T *data, int dataLen)
{
    int i;
    ClUint16T offset;
    int swap = 0;
    ClUint32T *val;

    /* @TODO optimize it for same endianess eval */
    CL_FUNC_ENTER();
    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"rbeExprEvaluateOne called expr: %p \
	                       data: %p len: %d\n", (void *)expr, (void*)data, dataLen);


    if (expr == NULL)
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"rbeExprEvaluateOne called NULL expr, returning TRUE\n");
        CL_FUNC_EXIT();
        return (CL_RULE_TRUE);
    }

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        swap = 1;

    if (swap)
        offset = CL_RULE_SWAP16 (expr->offset);
    else
        offset = expr->offset;

    if ((data == NULL) || (dataLen < (int)(offset + expr->len))) 
    {
        CL_FUNC_EXIT();
        return (CL_RULE_FALSE);
    }

    val = (ClUint32T *)expr->maskInt + expr->len;

    if (expr->flags &  CL_RULE_NON_ZERO_MATCH)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"calling for NON_ZERO_MATCH\n\r");
        if (swap)
        {
            for (i = 0; i < expr->len; i++)
            {
                if (expr->maskInt[i] & CL_RULE_SWAP32(data[offset + i]))
                {
                    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"MATCH\n\r");
                    CL_FUNC_EXIT();
                    return (CL_RULE_TRUE);
                }
            }
        } 
        else 
        {
            for (i = 0; i < expr->len; i++)
            {
                if (expr->maskInt[i] & data[offset + i])
                {
                    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"===MATCH ===\n\r");
                    CL_FUNC_EXIT();
                    return (CL_RULE_TRUE);
                }
            }
        }
    }
    else
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"calling for EXACT_MATCH\n\r");
        if (swap)
        {
            for (i = 0; i < expr->len; i++)
            {
                if ((expr->maskInt[i] & CL_RULE_SWAP32(data[offset + i]))
                    != val[i])
                {
                    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"MATCH FAILED\n\r");
                    CL_FUNC_EXIT();
                    return (CL_RULE_FALSE);
                }
            }
        } else 
        {
            for (i = 0; i < expr->len; i++)
            {
                if ((expr->maskInt[i] & data[offset + i]) != val[i])
                {
                    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"MATCH FAILED\n\r");
                    CL_FUNC_EXIT();
                    return (CL_RULE_FALSE);
                }
            }
        }
    }

    if (expr->flags & CL_RULE_NON_ZERO_MATCH)
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"MATCH FAILED\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_FALSE);
    }
    else
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"MATCH\n\r");
        CL_FUNC_EXIT();
        return (CL_RULE_TRUE);
    }
}


/**
 *                                                                       
 *  Evaluate a complex RBE Expression. 
 *
 *  This function evaluates a RBE Expression. RBE expression
 * could be a complex expression, i.e., multiple expressions
 * can be chained together.
 *                                                                     
 *  @param Expr RBE Expression to be evaluated.
 *  @param data Data pointer against which the RBE need to executed. 
 *  @Length length of the data in multiples of 4 bytes.  
 * 
 *  @returns 
 *          CL_RULE_TRUE on true  and CL_RULE_FALSE on false. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRuleResultT clRuleExprEvaluate (ClRuleExprT* expr, ClUint32T *data, int dataLen)
{
    ClRuleExprT* exprTmp;

    CL_FUNC_ENTER();
    clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"%s called data %p len %d\n", __FUNCTION__, (void*)data, dataLen);

    if (expr == NULL)
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE, 
                   "%s: called with NULL expr, returning TRUE\n", __FUNCTION__);
        CL_FUNC_EXIT();
        return (CL_RULE_TRUE);
    }

    exprTmp = expr;
    /* optimize based on whether it is CL_RULE_EXPR_CHAIN_AND or 
     * CL_RULE_EXPR_CHAIN_OR or CL_RULE_EXPR_CHAIN_GROUP_OR 
     */
    if ( exprTmp->flags &  CL_RULE_EXPR_CHAIN_AND )
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"CHAINED WITH AND\n\r");
    expr_and:
        while (exprTmp != NULL)
        {
            /*
             * Check for OR and GROUP ORs within an AND group. 
             */
            if (exprTmp->flags & CL_RULE_EXPR_CHAIN_GROUP_OR)
                goto expr_gor;
            else if( !(exprTmp->flags & CL_RULE_EXPR_CHAIN_AND) )
                goto expr_or;

            if (rbeExprEvaluateOne (exprTmp, data, dataLen) == CL_RULE_FALSE)
            {
                CL_FUNC_EXIT();
                clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"AND MATCH FAILED\n");
                return (CL_RULE_FALSE);
            }
            exprTmp = exprTmp->next;
        }
        CL_FUNC_EXIT();
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"AND MATCH PASSED\n\r");
        return (CL_RULE_TRUE);
    } 
    else if ( exprTmp->flags & CL_RULE_EXPR_CHAIN_GROUP_OR) /* GROUP OR */
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"CHAINED WITH GROUP OR");
    expr_gor:

        while (exprTmp != NULL)
        {
            if (!(exprTmp->flags & CL_RULE_EXPR_CHAIN_GROUP_OR))
            {
                /* No expression matched in the group-or chain */
                clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"GROUP OR MATCH FAILED");
                CL_FUNC_EXIT();
                return CL_RULE_FALSE;
            }

            if (rbeExprEvaluateOne (exprTmp, data, dataLen) == CL_RULE_TRUE)
            {
                clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"GROUP OR MATCH PASSED");
                /*
                 * Skip all group ors.
                 */
                while( (exprTmp = exprTmp->next) &&
                       (exprTmp->flags & CL_RULE_EXPR_CHAIN_GROUP_OR));

                if(exprTmp)
                {
                    if(exprTmp->flags & CL_RULE_EXPR_CHAIN_AND)
                        goto expr_and;
                    else
                        goto expr_or;
                }

                CL_FUNC_EXIT();
                return CL_RULE_TRUE;
            }

            exprTmp = exprTmp->next;
        }

        CL_FUNC_EXIT();
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"GROUP OR MATCH FAILED");
        return (CL_RULE_FALSE);
    }
    else  /* CL_RULE_EXPR_CHAIN_OR */
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"CHAINED WITH OR\n\r");

    expr_or:
        while (exprTmp != NULL)
        {
            if (exprTmp->flags & CL_RULE_EXPR_CHAIN_GROUP_OR)
                goto expr_gor;
            else if(exprTmp->flags & CL_RULE_EXPR_CHAIN_AND)
                goto expr_and;
            
            if (rbeExprEvaluateOne (exprTmp, data, dataLen) == CL_RULE_TRUE)
            {
                CL_FUNC_EXIT();
                clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"OR MATCH PASSED\n\r");
                return (CL_RULE_TRUE);
            }
            exprTmp = exprTmp->next;
        }

        CL_FUNC_EXIT();
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"OR MATCH FAILED\n\r");
        return (CL_RULE_FALSE);
    } 
    
    /* shd never reach */
    CL_FUNC_EXIT();
    return (CL_RULE_FALSE);
}

ClRuleResultT clRuleExprEvaluateOne (ClRuleExprT* expr1, ClRuleExprT* expr2)
{
    int i;

    CL_FUNC_ENTER();
    if ((expr1 == NULL) || (expr2 == NULL))
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,
                   "%s called with NULL expr, returning TRUE\n", __FUNCTION__);
        CL_FUNC_EXIT();
        return (CL_RULE_TRUE);
    }

    if (expr1->len != expr2->len)
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,
                   "%s: mismatch in expr length\n", __FUNCTION__);
        CL_FUNC_EXIT();
        return (CL_RULE_FALSE);
    }

    /* compare for exact match only */
    if ((expr1->flags & CL_RULE_ARCH_FLAG_MASK) !=
        (expr2->flags & CL_RULE_ARCH_FLAG_MASK))
    {
        for (i = 0; i < expr1->len; i++)
        {
            /*      mask1       &       value2                                != */
            if ((expr1->maskInt[i] & CL_RULE_SWAP32(expr2->maskInt[expr1->len + i])) != 
                /*      mask2       &       value1                                 */
                (CL_RULE_SWAP32(expr2->maskInt[i]) & expr1->maskInt[expr1->len + i]))
            {
                CL_FUNC_EXIT();
                return (CL_RULE_FALSE);
            }
        }
    } else {
        for (i = 0; i < expr1->len; i++)
        {
            /*      mask1       &       value2                    != */
            if ((expr1->maskInt[i] & expr2->maskInt[expr1->len + i]) != 
                /*      mask2       &       value1                                 */
                (expr2->maskInt[i] & expr1->maskInt[expr1->len + i]))
            {
                CL_FUNC_EXIT();
                return (CL_RULE_FALSE);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_RULE_TRUE);
}



/**
 *                                                                       
 *  Evaluate Double RBE Expressions. 
 *
 *  This function evaluates and RBE Expression against another RBE expr. 
 *  Both expressions are assumed to be of simple type.
 *                                                                     
 *  @param Expr1 First RBE Expression to be evaluated.
 *  @param Expr2 Second RBE Expression to be evaluated.
 * 
 *  @returns 
 *          CL_RULE_TRUE on true  and CL_RULE_FALSE on false. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */             
ClRuleResultT
clRuleDoubleExprEvaluate (ClRuleExprT* expr1, ClRuleExprT* expr2)
{
    ClRuleExprT* exprTemp1 = NULL;
    ClRuleExprT* exprTemp2 = NULL;
    ClUint32T noOfChain1 = 0;
    ClUint32T noOfChain2 = 0;
    ClRuleResultT result = CL_RULE_TRUE;

    /*
      1. First find out no of chains in both the RBEs.
      2. If the no of chains in both the RBEs are different then return FALSE.
      3. If no are same then go and proceed.
    */
    exprTemp1 = expr1;
    while (exprTemp1 != NULL)
    {
    	exprTemp1 = exprTemp1->next;
        noOfChain1++;
    }
    
    exprTemp2 = expr2;
    while (exprTemp2 != NULL)
    {
    	exprTemp2 = exprTemp2->next;
        noOfChain2++;
    }

    if(noOfChain1 != noOfChain2)
    {
        return (CL_RULE_FALSE);
    }


    exprTemp1 = expr1;
    exprTemp2 = expr2;
    
    while (exprTemp1 != NULL)
    {
        result = clRuleExprEvaluateOne (exprTemp1, exprTemp2);
        if(result == CL_RULE_FALSE)
        {
            return CL_RULE_FALSE;
        }
    	exprTemp1 = exprTemp1->next;
        exprTemp2 = exprTemp2->next;
    }
    
    return CL_RULE_TRUE;
}

                                                        
/**
 *                                                                       
 *  Endian convert a RBE Expression. 
 *
 *  This function converts the endianness of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression to be converted. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT rbeExprConvertOne (ClRuleExprT* expr)
{
    int i;

    CL_FUNC_ENTER();

    if (expr == NULL)
    {
        clLogTrace(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE, 
                   "rbeExprConvertOne: called with NULL expr\n");
        CL_FUNC_EXIT();
        return (CL_OK);  /* a null expression is a valid expresion */
    }


    expr->offset = CL_RULE_SWAP16 (expr->offset);

    for (i = 0; i < expr->len * 2; i++)
    {
        expr->maskInt[i] = CL_RULE_SWAP32 (expr->maskInt[i]);
    }
    expr->flags = (expr->flags & CL_RULE_EXPR_FLAG_MASK) |
    (((expr->flags & CL_RULE_ARCH_FLAG_MASK) == CL_RULE_LITTLE_END) ?
     CL_RULE_BIG_END : CL_RULE_LITTLE_END);

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *                                                                       
 *  Endian convert a complex RBE Expression. 
 *
 *  This function converts endianness of a complex
 * RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression to be converted. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT
clRuleExprConvert (ClRuleExprT* expr)
{
    ClRuleExprT* exprTmp = expr;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    while (exprTmp &&
           ((rc = rbeExprConvertOne (exprTmp)) != CL_OK))
    {
        exprTmp = exprTmp->next;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 *                                                                       
 *  Convert a RBE expression to local endianess. 
 *
 *  This function converts a RBE expression to match local endianess. 
 *                                                                     
 *  @param Expr  RBE Expression to be converted. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT
clRuleExprLocalConvert (ClRuleExprT* expr)
{
    ClRuleExprT* exprTmp = expr;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    while ((exprTmp) && (rc == CL_OK))
    {
        /* if expr is null or already in the local format, nothing to do */
        if ((exprTmp != NULL) && 
            ((exprTmp->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian()))
            rc = clRuleExprConvert (exprTmp); /* else convert */
        exprTmp = exprTmp->next;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 *                                                                       
 *  Set Falgs of a RBE expression. 
 *
 *  This function sets the RBE expression flags. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param flags flags to be set. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT
clRuleExprFlagsSet (ClRuleExprT* expr, ClRuleExprFlagsT flags)
{
    CL_FUNC_ENTER();

    if (expr == NULL)
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    expr->flags &= CL_RULE_ARCH_FLAG_MASK;
    expr->flags |= (CL_RULE_EXPR_FLAG_MASK & flags);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Set offset of a RBE expression. 
 *
 *  This function sets the offset field of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param offset offset value (multiples of 4 bytes). 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT
clRuleExprOffsetSet (ClRuleExprT* expr, ClUint16T offset)
{
    CL_FUNC_ENTER();

    if (expr == NULL)
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    /* can be assume that set is done on local endiantype expressions ?  */

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        expr->offset = CL_RULE_SWAP16 (offset);
    else
        expr->offset = offset;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Set Mask of a RBE expression. 
 *
 *  This function sets the Mask field of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param offset offset at which the mask to be set. 
 *  @param Mask  Mask value to be set. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 * NOTE: offset is in multiple of 4 bytes. Offset here is overall offset,
 * i.e., mask set is expr->mask[offset - expr->offset]            
 *
 */                                                                     
ClRcT
clRuleExprMaskSet (ClRuleExprT* expr, ClUint16T offset, ClUint32T mask)
{
    ClUint16T exprOffset;
    int swap = 0;

    CL_FUNC_ENTER();

    if (expr == NULL) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        swap = 1;

    if (swap)
        exprOffset = CL_RULE_SWAP16 (expr->offset);
    else
        exprOffset = expr->offset;

    if ((offset < exprOffset) || (offset >= (exprOffset + expr->len))) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"offset out of range!!\n");
        CL_FUNC_EXIT();
        return (RBE_ERR_OFFSET_OUT_OF_RANGE);
    }

    if (swap)
        expr->maskInt[offset - exprOffset] = CL_RULE_SWAP32 (mask);
    else
        expr->maskInt[offset - exprOffset] = mask;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Set Value of a RBE expression. 
 *
 *  This function sets the value field of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param offset offset at which the value to be set. 
 *  @param value  Mask value to be set. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *
 * NOTE: offset is in multiple of 4 bytes. Offset here is overall offset,
 * i.e., value set is expr->value[offset - expr->offset]            
 *                                                                     
 */
ClRcT
clRuleExprValueSet (ClRuleExprT* expr, ClUint16T offset, ClUint32T value)
{
    ClUint16T exprOffset;
    int swap = 0;

    CL_FUNC_ENTER();
    if (expr == NULL) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        swap = 1;

    if (swap)
        exprOffset = CL_RULE_SWAP16 (expr->offset);
    else
        exprOffset = expr->offset;

    if ((offset < exprOffset) || (offset >= (exprOffset + expr->len))) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"offset out of range!!\n");
        CL_FUNC_EXIT();
        return (RBE_ERR_OFFSET_OUT_OF_RANGE);
    }

    /* maskInt[expr->len] is where value[0] starts */

    if (swap)
        expr->maskInt[expr->len + offset - exprOffset] = CL_RULE_SWAP32 (value);
    else
        expr->maskInt[expr->len + offset - exprOffset] = value;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Get RBE expression flags.
 *
 *  This function gets the flags of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param &Flags Flags to be ruturned. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *                                                                     
 */
ClRcT
clRuleExprFlagsGet (ClRuleExprT* expr, ClRuleExprFlagsT *pFlags)
{
    CL_FUNC_ENTER();
    if ((expr == NULL) || (pFlags == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    *pFlags = (ClRuleExprFlagsT) expr->flags;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Get RBE expression Offset value.
 *
 *  This function gets the offset value of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param &offset Offset to be ruturned. 
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 *                                                                     
 */
ClRcT
clRuleExprOffsetGet (ClRuleExprT* expr, ClUint16T *pOffset)
{
    CL_FUNC_ENTER();
    if ((expr == NULL) || (pOffset == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    /* can be assume that set is done on local endiantype expressions ?  */

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        *pOffset = CL_RULE_SWAP16 (expr->offset);
    else
        *pOffset = expr->offset;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Get RBE expression Mask value.
 *
 *  This function gets the Mask of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param Offset Get Mask from this Offset.
 *  @param &Mask  Mask to be returned.
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 * NOTE: offset is in multiple of 4 bytes. Offset here is overall offset,
 * i.e., mask returned is expr->mask[offset - expr->offset]              
 *                                                                     
 */
ClRcT
clRuleExprMaskGet (ClRuleExprT* expr, ClUint16T offset, ClUint32T *pMask)
{
    ClUint16T exprOffset;
    int swap = 0;

    CL_FUNC_ENTER();
    if ((expr == NULL)  || (pMask == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        swap = 1;

    if (swap)
        exprOffset = CL_RULE_SWAP16 (expr->offset);
    else
        exprOffset = expr->offset;

    if ((offset < exprOffset) || (offset >= (exprOffset + expr->len))) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"offset out of range!!\n");
        CL_FUNC_EXIT();
        return (RBE_ERR_OFFSET_OUT_OF_RANGE);
    }

    if (swap)
        *pMask = CL_RULE_SWAP32 (expr->maskInt[offset - exprOffset]);
    else
        *pMask = expr->maskInt[offset - exprOffset];

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Get RBE expression value.
 *
 *  This function gets the value of a RBE expression. 
 *                                                                     
 *  @param Expr  RBE Expression for which flags to be set.
 *  @param Offset Get Mask from this Offset.
 *  @param &value value to be returned.
 * 
 *  @returns 
 *          CL_OK on success else CL_RC_ERROR.. 
 *
 *  @author clovis
 *  @version 1.0
 * NOTE: offset is in multiple of 4 bytes. Offset here is overall offset,
 * i.e., value returned is expr->value[offset - expr->offset]              
 *                                                                     
 */
ClRcT
clRuleExprValueGet (ClRuleExprT* expr, ClUint16T offset, ClUint32T *pValue)
{
    ClUint16T exprOffset;
    int swap = 0;

    CL_FUNC_ENTER();
    if ((expr == NULL)  || (pValue == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    if ((expr->flags & CL_RULE_ARCH_FLAG_MASK) != rbeGetEndian())
        swap = 1;

    if (swap)
        exprOffset = CL_RULE_SWAP16 (expr->offset);
    else
        exprOffset = expr->offset;

    if ((offset < exprOffset) || (offset >= (exprOffset + expr->len))) 
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"offset out of range!!\n");
        CL_FUNC_EXIT();
        return (RBE_ERR_OFFSET_OUT_OF_RANGE);
    }

    /* maskInt[expr->len] is where value[0] starts */

    if (swap)
        *pValue = CL_RULE_SWAP32 (expr->maskInt[expr->len + offset - exprOffset]);
    else
        *pValue = expr->maskInt[expr->len + offset - exprOffset];

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *                                                                       
 *  Get the total memory used by the expression.
 *
 *  This function returns the memory needed to pack an 
 * expression into contiguous memory location.
 *                                                                     
 *  @param Expr : expression
 *                                                                     
 *  @returns 
 *          Length in bytes needed to pack an expression.
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClUint32T clRuleExprMemLenGet (ClRuleExprT* expr)
{
    ClUint32T len = 0;

    CL_FUNC_ENTER();
    while (expr != NULL)
    {
        len += (sizeof (ClRuleExprT) + (expr->len * 8));
        expr = expr->next;
    }

    CL_FUNC_EXIT();
    return (len);
}


/**
 *                                                                       
 *  Duplicate a RBE Expression. 
 *
 *  This function makes a copy of the given RBE Expression. 
 *                                                                     
 *  @param srcExpr Source RBE Expression to Copy. 
 *  @param dstExpr Pointer to a new copy of the expression.
 *                                                                     
 *  @returns 
 *          CL_OK on success, CL_RC_ERROR if failed. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT clRuleExprDuplicate (ClRuleExprT* srcExpr, ClRuleExprT* *pDstExpr)
{
    ClRuleExprT* tmpS, *tmpD;
    ClRcT rc;

    CL_FUNC_ENTER();
    if ((srcExpr == NULL) || (pDstExpr == NULL))
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    /* 
     * note that srcExpr can be a complex expression, i.e., may 
     * contain a chain of expressions. Copy the first simple
     * expression from the srcExpr.
     */
    if ((rc = clRuleExprAllocate (srcExpr->len, pDstExpr)) != CL_OK)
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"expr allocation failed!!\n");
        CL_FUNC_EXIT();
        return (rc);
    }
    memcpy (*pDstExpr, srcExpr, sizeof (ClRuleExprT) + srcExpr->len*8);

    tmpS = srcExpr->next;
    tmpD = *pDstExpr;

    /* loop through to copy the expressions in the chain, if any */
    while (tmpS != NULL)
    {
        if ((rc = clRuleExprAllocate (tmpS->len, &tmpD->next) != CL_OK))
        {
            if(CL_OK != clRuleExprDeallocate (*pDstExpr))
            {
                clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"expr free failed!!\n");
            }
            clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"expr allocation failed!!\n");
    	    CL_FUNC_EXIT();
            return (rc);
        }
        memcpy (tmpD->next, tmpS, sizeof (ClRuleExprT) + tmpS->len*8);
        tmpS = tmpS->next; 
        tmpD = tmpD->next; 
    }
    tmpD->next = NULL;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *                                                                       
 *  Pack a RBE Expression into the given mem area.
 *
 *  This function packs the given RBE Expression.
 * The expression is packed at the given memory pointed by pBuf.
 * It is assumed that the pBuf contains enough space to accomodate
 * srcExpr. Caller can call clRuleExprMemLenGet() to get 
 * the memory needed to pack the srcExpr.
 * 
 *  @param srcExpr Source RBE Expression to pack. 
 *  @param pBuf Pointer to the memory location where to pack.
 *                                                                     
 *  @returns 
 *          CL_OK on success, CL_RULE_RC(CL_ERR_NULL_POINTER) if failed. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT clRuleExprPack(ClRuleExprT *srcExpr, ClUint8T **ppBuf, ClUint32T *pLen)
{
    ClRuleExprT *tmpS = NULL;
    ClUint32T nExpr = 0;
    ClBufferHandleT bufHdl = 0;
    ClRcT rc = CL_OK;

    if (!srcExpr || !ppBuf || !pLen)
    {
        return CL_RULE_RC(CL_ERR_NULL_POINTER);
    }

    tmpS = srcExpr;
    while (tmpS)
    {
        nExpr++;
        tmpS = tmpS->next;
    }
    
    rc = clBufferCreate(&bufHdl);
    if (rc != CL_OK) goto out;
    
    rc = clXdrMarshallClUint32T(&nExpr, bufHdl, 0);
    if (rc != CL_OK) goto out;

    tmpS = srcExpr;

    while (tmpS)
    {
        ClUint32T i = 0;
        
        rc = clXdrMarshallClUint8T(&tmpS->flags, bufHdl, 0);
        if (rc != CL_OK) goto out;

        rc = clXdrMarshallClUint8T(&tmpS->len, bufHdl, 0);
        if (rc != CL_OK) goto out;

        rc = clXdrMarshallClUint16T(&tmpS->offset, bufHdl, 0);
        if (rc != CL_OK) goto out;

        for (i = 0; i < tmpS->len; ++i)
        {
            rc = clXdrMarshallClUint32T(&tmpS->maskInt[i], bufHdl, 0);
            if (rc != CL_OK) goto out;
            
            rc = clXdrMarshallClUint32T(&tmpS->maskInt[tmpS->len+i],
                                        bufHdl,
                                        0);
            if (rc != CL_OK) goto out;
        }

        tmpS = tmpS->next;
    }

    rc = clBufferLengthGet(bufHdl, pLen);
    if (rc != CL_OK) goto out;

    rc = clBufferFlatten(bufHdl, ppBuf);
    if (rc != CL_OK) goto out;
    
    clBufferDelete(&bufHdl);
    
    return CL_OK;
out:
    return rc;
}

/**
 *                                                                       
 *  UnPack a RBE Expression. 
 *
 *  This function makes unpack of the given RBE Expression. 
 *                                                                     
 *  @param srcExpr Source RBE Expression to unpack. 
 *  @param dstExpr Pointer to a new copy of the expression.
 *                                                                     
 *  @returns 
 *          CL_OK on success, CL_RC_ERROR if failed. 
 *
 *  @author clovis
 *  @version 1.0
 *
 */                                                                     
ClRcT clRuleExprUnpack(ClUint8T *pBuf, ClUint32T bufLen, ClRuleExprT **pDstExpr)
{
    ClRuleExprT *tmpD = NULL;
    ClUint32T nExpr = 0;
    ClBufferHandleT bufHdl = 0;
    ClRcT rc = CL_OK;
    ClUint8T flags = 0;
    ClUint8T len = 0;
    ClUint16T offset = 0;
    ClUint32T i = 0;

    if (!pDstExpr)
    {
        return CL_RULE_RC(CL_ERR_NULL_POINTER);
    }

    if (!pBuf && !bufLen)
    {
        *pDstExpr = NULL;
        return CL_OK;
    }
    else if (!pBuf || !bufLen)
    {
        return CL_RULE_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clBufferCreate(&bufHdl);
    if (rc != CL_OK) goto out;

    rc = clBufferNBytesWrite(bufHdl, pBuf, bufLen);
    if (rc != CL_OK) goto out;
    
    rc = clXdrUnmarshallClUint32T(bufHdl, &nExpr);
    if (rc != CL_OK) goto out;

    tmpD = *pDstExpr;

    while (nExpr)
    {
        rc = clXdrUnmarshallClUint8T(bufHdl, &flags);
        if (rc != CL_OK) goto out;

        rc = clXdrUnmarshallClUint8T(bufHdl, &len);
        if (rc != CL_OK) goto out;

        rc = clXdrUnmarshallClUint16T(bufHdl, &offset);
        if (rc != CL_OK) goto out;

        if (!tmpD)
        {
            rc = clRuleExprAllocate(len, &tmpD);
            if (rc != CL_OK) goto out;
            *pDstExpr = tmpD;
            tmpD->next = NULL;
        }
        else
        {
            ClRuleExprT *t = NULL;
            rc = clRuleExprAllocate(len, &t);
            if (rc != CL_OK) goto out;
            t->next = NULL;
            tmpD->next = t;
            tmpD = tmpD->next;
        }

        tmpD->flags = flags;
        tmpD->len = len;
        tmpD->offset = offset;

        for (i = 0; i < len; ++i)
        {
            rc = clXdrUnmarshallClUint32T(bufHdl, &(tmpD->maskInt[i]));
            if (rc != CL_OK) goto out;

            rc = clXdrUnmarshallClUint32T(bufHdl, &(tmpD->maskInt[len+i]));
            if (rc != CL_OK) goto out;
        }

        --nExpr;
    }

    clBufferDelete(&bufHdl);
    return CL_OK;
out:
    return rc;
}





/**
 *                                                                       
 *  Print a RBE Expression. 
 *
 *  This function Prints the given RBE Expression. 
 *                                                                     
 *  @param Expr  RBE Expression to which to print.
 * 
 *  @returns None.
 *
 *  @author clovis
 *  @version 1.0
 *                                                                     
 */
ClRcT rbeExprPrintOne (ClRuleExprT* expr)
{
    int i;

    CL_FUNC_ENTER();

    if (expr == NULL)
    {
        clLogError(UTIL_LOG_AREA,UTIL_LOG_CTX_RULE,"NULL Parameter!!\n");
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    clOsalPrintf("\n\r  --\n\r");
    clOsalPrintf("  Expr:%p\n\r", expr);
    clOsalPrintf("  Endian:%s\n\r",
                 ((expr->flags & CL_RULE_ARCH_FLAG_MASK) == CL_RULE_LITTLE_END)
                 ? "LITTLE_END": "BIG_END");
    clOsalPrintf("  Flag:0x%x\n\r", expr->flags);
    clOsalPrintf("  Offset:0x%x\n\r", expr->offset);
    clOsalPrintf("  Len:0x%x\n\r", expr->len);
    clOsalPrintf("  Next:%p\n\r", expr->next);
    for (i = 0; i < expr->len; i++)
        clOsalPrintf("  Mask = 0x%8x   Value: 0x%8x\n\r",
                     expr->maskInt[i],
                     expr->maskInt[expr->len + i]);
    clOsalPrintf("--\n\r");
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *                                                                       
 *  Print a  complex RBE Expression. 
 *
 *  This function Prints the given RBE Expression
 *  which could be a complex expression.
 *                                                                     
 *  @param Expr  RBE Expression for which print.
 * 
 *  @returns None.
 *
 *  @author clovis
 *  @version 1.0
 *                                                                     
 */
ClRcT
clRuleExprPrint (ClRuleExprT* expr)
{
    ClRcT retCode = 0;
    CL_FUNC_ENTER();

    if(expr == NULL)
    {
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    while (expr != NULL)
    {
        retCode = rbeExprPrintOne (expr);
        expr = expr->next;
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *
 *  Print a complex RBE Expression into Message Buffer
 *
 *  This function Prints the given RBE Expression
 *  which could be a complex expression. Added specifically for Event
 *  to display the Filters on the CLI. Can be exposed in clRuleApi.h
 *  if other components need the same.
 *  
 *  @param dbhHdl the Handle that Debug has supplied.
 *
 *  @param Expr  RBE Expression for which print.
 * 
 *  @returns None.
 *
 *  @author clovis
 *  @version 1.0
 *                                                                     
 */
ClRcT clRuleExprPrintToMessageBuffer(ClDebugPrintHandleT msgHandle, ClRuleExprT* pExpr)
{
    ClRcT rc = CL_OK;

    ClUint32T i, bytesWritten = 0;
    char buf[CL_RULE_MSG_BUF_LEN] = {0}, *ptr = buf;

    CL_FUNC_ENTER();

    if(0 == msgHandle)
    {
        return (CL_RULE_RC(CL_ERR_INVALID_PARAMETER));
    }

    if(NULL == pExpr)
    {
        /* Let the caller handle this error (rather condition) */
        return (CL_RULE_RC(CL_ERR_NULL_POINTER));
    }

    while (pExpr != NULL)
    {
        bytesWritten = sprintf(buf, "_________________________________________________________\n\n");
        ptr += bytesWritten;
 
        bytesWritten = sprintf(ptr, "  Expr:%p\n\n", (void *)pExpr);
        ptr += bytesWritten;
 
        bytesWritten = sprintf(ptr, "  Endian:%s\n",
                               ((pExpr->flags & CL_RULE_ARCH_FLAG_MASK) == CL_RULE_LITTLE_END)
                               ? "LITTLE_END": "BIG_END");
        ptr += bytesWritten;
 
        bytesWritten = sprintf(ptr, "  Flag:0x%x\n", pExpr->flags);
        ptr += bytesWritten;
 
        bytesWritten = sprintf(ptr, "  Offset:0x%x\n", pExpr->offset);
        ptr += bytesWritten;

        bytesWritten = sprintf(ptr, "  Len:0x%x\n", pExpr->len);
        ptr += bytesWritten;

        bytesWritten = sprintf(ptr, "  Next:%p\n\n", (void *)(pExpr->next));
        ptr += bytesWritten;

        rc = clBufferNBytesWrite(msgHandle, (ClUint8T*)buf, ptr-buf);
        if (CL_OK != rc)
        {
            return rc;
        }

        for (i = 0; i < pExpr->len; i++)
        {
            bytesWritten = sprintf(buf, "  Mask = 0x%8x   Value: 0x%8x\n\r",
                                   pExpr->maskInt[i],
                                   pExpr->maskInt[pExpr->len + i]);
            rc = clBufferNBytesWrite(msgHandle, (ClUint8T*)buf, bytesWritten);
            if (CL_OK != rc)
            {
                return rc;
            }
        }

        bytesWritten = sprintf(buf, "_________________________________________________________\n");
        rc = clBufferNBytesWrite(msgHandle, (ClUint8T*)buf, bytesWritten);
        if (CL_OK != rc)
        {
            return rc;
        }

        pExpr = pExpr->next;
        ptr = buf; /* Reset the ptr */
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
