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
 * ModuleName  : cor
 * File        : clCorExpr.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains RBE Expr Util functions
 *****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <clBitApi.h>
#include <clDebugApi.h>
#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>

/* Internal Headers */

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* COR RBE Expr routines */

/**
 *  Set RBE Expr for the given MO Id.
 *
 *  API to set RBE Expression, given an MO Id.  The MO Id is converted
 *  to an RBE expression that matches it.
 *                                                                        
 *  @param moId      MO Id handle
 *  @param pRbeExpr  RBE Expression handle
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT 
corMOIdExprSet(ClCorMOIdPtrT moId,
               ClRuleExprT* pRbeExpr)
{
    int i=0;
    ClUint32T mask;
    ClUint32T value;

    CL_FUNC_ENTER();
    
    if ((pRbeExpr == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
    clRuleExprMaskSet(pRbeExpr, 0, 0x0);
    clRuleExprValueSet(pRbeExpr, 0, 0);
  
    /* set mask and value for valid portion of the ClCorMOId */
    for (i = 0; i < moId->depth; i++)
    {
        if (moId->node[i].type != CL_COR_CLASS_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = moId->node[i].type;
            clRuleExprMaskSet(pRbeExpr, 2*i, mask);
            clRuleExprValueSet(pRbeExpr, 2*i, value);
        }

        if (moId->node[i].instance != CL_COR_INSTANCE_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = moId->node[i].instance;
            clRuleExprMaskSet(pRbeExpr, 2*i + 1, mask);
            clRuleExprValueSet(pRbeExpr, 2*i + 1, value);
        }
    }

    /* 
     * set mask to 0xFFFFFFFF and value to 
     * CL_COR_INVALID_MO_INSTANCE/ID for the rest.
     */
    for (;i < CL_COR_HANDLE_MAX_DEPTH; i++)
    {
        clRuleExprMaskSet(pRbeExpr, 2*i, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i, CL_COR_INVALID_MO_ID);
        clRuleExprMaskSet(pRbeExpr, 2*i + 1, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_COR_INVALID_MO_INSTANCE);
    }

    /* set mask and value for service id */
    if (moId->svcId != CL_COR_SVC_WILD_CARD)
    {
        /*  Take care of Endianness here,
	 *  because svcId is 16 bit in moId. The svcId is followed
	 *  by depth (16 bit). The depth is being ignored.
	 *  pls see ClCorMOIdT
	 *  The service Id and depth together make a 32-bit int. and 
	 *  Rbe compares these 32-bits in data (moId) with Rbe Expression value.
	 */	    
	if(clByteEndian == CL_BIT_LITTLE_ENDIAN)
	{
		mask = 0xFFFF;
		value = moId->svcId & 0xFFFF;
	}else
	{
		mask = 0xFFFF0000;
		value = ((moId->svcId & 0xFFFF)<<16);
	}
        clRuleExprMaskSet(pRbeExpr, 2*CL_COR_HANDLE_MAX_DEPTH, mask);
        clRuleExprValueSet(pRbeExpr, 2*CL_COR_HANDLE_MAX_DEPTH, value);
    }
    /* set absolute/relative path */
    value = moId->qualifier;
    mask = 0xFFFFFFFF;
    clRuleExprMaskSet(pRbeExpr, (2*CL_COR_HANDLE_MAX_DEPTH)+1, mask);
    clRuleExprValueSet(pRbeExpr, (2*CL_COR_HANDLE_MAX_DEPTH)+1, value);
  
    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 *  Set RBE Expr for the given MO Id.
 *  The RBE is created in Network order.
 */

ClRcT 
corMOIdExprH2NSet(ClCorMOIdPtrT moId,
               ClRuleExprT* pRbeExpr)
{
    int i=0;
    ClUint32T mask;
    ClUint32T value;

    CL_FUNC_ENTER();
    
    if ((pRbeExpr == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
    clRuleExprMaskSet(pRbeExpr, 0, 0x0);
    clRuleExprValueSet(pRbeExpr, 0, 0);
  
    /* set mask and value for valid portion of the ClCorMOId */
    for (i = 0; i < moId->depth; i++)
    {
        if (moId->node[i].type != CL_COR_CLASS_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = moId->node[i].type;
            clRuleExprMaskSet(pRbeExpr, 2*i, mask);
            clRuleExprValueSet(pRbeExpr, 2*i, CL_BIT_H2N32(value));
        }

        if (moId->node[i].instance != CL_COR_INSTANCE_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = moId->node[i].instance;
            clRuleExprMaskSet(pRbeExpr, 2*i + 1, mask);
            clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_BIT_H2N32(value));
        }
    }

    /* 
     * set mask to 0xFFFFFFFF and value to 
     * CL_COR_INVALID_MO_INSTANCE/ID for the rest.
     */
    for (;i < CL_COR_HANDLE_MAX_DEPTH; i++)
    {
        clRuleExprMaskSet(pRbeExpr, 2*i, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i, CL_BIT_H2N32(CL_COR_INVALID_MO_ID));
        clRuleExprMaskSet(pRbeExpr, 2*i + 1, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_BIT_H2N32(CL_COR_INVALID_MO_INSTANCE));
    }

    /* set mask and value for service id */
    if (moId->svcId != CL_COR_SVC_WILD_CARD)
    {
        /*  Take care of Endianness here,
	 *  because svcId is 16 bit in moId. The svcId is followed
	 *  by depth (16 bit). The depth is being ignored.
	 *  pls see ClCorMOIdT
	 *  The service Id and depth together make a 32-bit int. and 
	 *  Rbe compares these 32-bits in data (moId) with Rbe Expression value.
	 */	    
        mask = 0xFFFF0000;
	if(clByteEndian == CL_BIT_LITTLE_ENDIAN)
	{   
        value = moId->svcId;
        value = CL_BIT_SWAP16(value);
	  	/*  value <<= 16; */
	}
       else
	{
		value = ((moId->svcId & 0xFFFF)<<16);
	}

        clRuleExprMaskSet(pRbeExpr, 2*CL_COR_HANDLE_MAX_DEPTH,  CL_BIT_H2N32(mask));
        clRuleExprValueSet(pRbeExpr, 2*CL_COR_HANDLE_MAX_DEPTH, value);
    }
    /* set absolute/relative path */
    value = moId->qualifier;
    mask = 0xFFFFFFFF;
    clRuleExprMaskSet(pRbeExpr, (2*CL_COR_HANDLE_MAX_DEPTH)+1, mask);
    clRuleExprValueSet(pRbeExpr, (2*CL_COR_HANDLE_MAX_DEPTH)+1, CL_BIT_H2N32(value));
  
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *  Set RBE Expr for the given Attr Path.
 *
 *  API to set RBE Expression, given an Attr Path.  The Attr Path is converted
 *  to an RBE expression that matches it.
 *                                                                        
 *  @param AttrPath      AttrPath handle
 *  @param pRbeExpr  RBE Expression handle
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT 
corAttrPathExprSet(ClCorAttrPathPtrT pAttrPath,
               ClRuleExprT* pRbeExpr)
{
    int i=0;
    ClUint32T mask;
    ClUint32T value;

    CL_FUNC_ENTER();
    
    if ((pRbeExpr == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
    clRuleExprMaskSet(pRbeExpr, 0, 0x0);
    clRuleExprValueSet(pRbeExpr, 0, 0);
  
    /* set mask and value for valid portion of the ClCorMOId */
    for (i = 0; i < pAttrPath->depth; i++)
    {
        if (pAttrPath->node[i].attrId != CL_COR_ATTR_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = pAttrPath->node[i].attrId;
            clRuleExprMaskSet(pRbeExpr, 2*i, mask);
            clRuleExprValueSet(pRbeExpr, 2*i, value);
        }

        if (pAttrPath->node[i].index != CL_COR_INDEX_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = pAttrPath->node[i].index;
            clRuleExprMaskSet(pRbeExpr, 2*i + 1, mask);
            clRuleExprValueSet(pRbeExpr, 2*i + 1, value);
        }
    }

    /* 
     * set mask to 0xFFFFFFFF and value to 
     * CL_COR_INVALID_MO_INSTANCE/ID for the rest.
     */
    for (;i < CL_COR_CONT_ATTR_MAX_DEPTH; i++)
    {
        clRuleExprMaskSet(pRbeExpr, 2*i, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i, CL_COR_INVALID_ATTR_ID);
        clRuleExprMaskSet(pRbeExpr, 2*i + 1, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_COR_INVALID_ATTR_IDX);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 *  Set RBE Expr for the given Attr Path.
 *  The RBE expr is created in network order.
 */

ClRcT 
corAttrPathExprH2NSet(ClCorAttrPathPtrT pAttrPath,
               ClRuleExprT* pRbeExpr)
{
    int i=0;
    ClUint32T mask;
    ClUint32T value;

    CL_FUNC_ENTER();
    
    if ((pRbeExpr == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
    clRuleExprMaskSet(pRbeExpr, 0, 0x0);
    clRuleExprValueSet(pRbeExpr, 0, 0);
  
    /* set mask and value for valid portion of the ClCorMOId */
    for (i = 0; i < pAttrPath->depth; i++)
    {
        if (pAttrPath->node[i].attrId != CL_COR_ATTR_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = pAttrPath->node[i].attrId;
            clRuleExprMaskSet(pRbeExpr, 2*i, mask);
            clRuleExprValueSet(pRbeExpr, 2*i, CL_BIT_H2N32(value));
        }

        if (pAttrPath->node[i].index != CL_COR_INDEX_WILD_CARD)
        {
            mask = 0xFFFFFFFF;
            value = pAttrPath->node[i].index;
            clRuleExprMaskSet(pRbeExpr, 2*i + 1, mask);
            clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_BIT_H2N32(value));
        }
    }

    /* 
     * set mask to 0xFFFFFFFF and value to 
     * CL_COR_INVALID_MO_INSTANCE/ID for the rest.
     */
    for (;i < CL_COR_CONT_ATTR_MAX_DEPTH; i++)
    {
        clRuleExprMaskSet(pRbeExpr, 2*i, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i, CL_BIT_H2N32(CL_COR_INVALID_ATTR_ID));
        clRuleExprMaskSet(pRbeExpr, 2*i + 1, 0xFFFFFFFF);
        clRuleExprValueSet(pRbeExpr, 2*i + 1, CL_BIT_H2N32(CL_COR_INVALID_ATTR_IDX));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *  Create RBE Expr for the given MO Id.
 *
 *  API to create an RBE Expression, given an MO Id.  The MO Id is
 *  converted to an RBE expression that matches it.
 *                                                                        
 *  @param moId    MO Id handle
 *  @param pRbeExpr  [out] handle to the newly created RBE expression on Success
 *
 *  @returns CL_OK  - Success<br>
 *           MO_NO_MEM - Failed to allocate memory<br>
 */
ClRcT 
corMOIdExprCreate(ClCorMOIdPtrT moId,
                  ClRuleExprT* *pRbeExpr)
{
    int expLen;
  
    CL_FUNC_ENTER();
    if ((pRbeExpr == NULL))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "NULL argument\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    expLen = sizeof (ClCorMOIdT)/4;

    if (clRuleExprAllocate (expLen, pRbeExpr) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clRuleExprAllocate failed\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    
    /* set the expression */
    corMOIdExprSet(moId, *pRbeExpr);
  
    /* set the initial offset */
    clRuleExprOffsetSet(*pRbeExpr, 0);
    clRuleExprFlagsSet(*pRbeExpr, CL_RULE_MATCH_EXACT);
  
#ifdef EXPR_DEBUG
    clRuleExprPrint (*pRbeExpr);
#endif

    CL_FUNC_EXIT();
    return (CL_OK);
}
