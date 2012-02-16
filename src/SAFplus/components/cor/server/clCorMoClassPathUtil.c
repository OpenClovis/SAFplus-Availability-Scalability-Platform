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
 * File        : clCorMoClassPathUtil.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains  MOClassPath Utilities.
 *****************************************************************************/

/* FILES INCLUDED */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>

/* Internal Headers */
#include "clCorPvt.h"
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorObj.h"


/**
 * Get Class Id.
 *
 * Getclass id given the moPath.
 *
 *  @param pPath    : [IN] ClCorMOClassPath from which to derive the class Id.
 *         svcId    : [IN] Service ID
 *         pClassId : [OUT] will contain the class id.
 *
 *  @returns CL_OK          on successs.
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)  NULL input parameter
 *
 */
ClRcT
_corMOPathToClassIdGet(ClCorMOClassPathPtrT pPath, ClCorServiceIdT svcId, ClCorClassTypeT* pClassId)
{
    CORMOClass_h    hCorMoClass  = NULL;
    CORMSOClass_h   hCorMsoClass = NULL;
    ClRcT          rc           = CL_OK;  
    CL_FUNC_ENTER();
    if ((pPath == NULL) || (pClassId == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n corMOPathToClassIdGet failed: NULL input parameter \n"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
                                                                                                                             
    rc = corMOClassHandleGet(pPath, &hCorMoClass);
    if((rc != CL_OK) || (hCorMoClass == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("corMOPathToClassIdGet: failed to get COR MO class handle, rc 0x%x"
                        "!!!\r\n", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    rc = corMSOClassHandleGet(hCorMoClass, svcId, &hCorMsoClass);
    if ((rc != CL_OK) || (hCorMsoClass == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("corMOPathToClassIdGet: failed to get COR MSO class handle, rc 0x%x"
                        "!!!\r\n", rc));
        CL_FUNC_EXIT();
        return rc;
    }
                                                                                                                             
    *pClassId = hCorMsoClass->classId;
    CL_FUNC_EXIT();
    return rc;
}


/**
 * Get the next sibling.
 *
 * Get the next sibling (to the right). updates corPath (this) on
 * CL_OK
 *
 *  @param this: [IN/OUT] updated corPath is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */
ClRcT
_clCorMoClassPathNextSiblingGet(ClCorMOClassPathPtrT this)
{
    MArrayNode_h node;
                                                                                                                             
    CL_FUNC_ENTER();
                                                                                                                             
                                                                                                                             
    if (this == NULL)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
                                                                                                                             
    /* 1. locate the parent
     * 2. Query the next child given a child
     * 3. update the index & return
     */
    node = corMOTreeParentGet(this);
    if(node && this->depth > 0)
      {
        ClCorMOClassPathT tmpCorPath;
        ClUint32T grpId = 1;
        int idx;
                                                                                                                             
        tmpCorPath = *this;
        /* get the index list from the moTree and check if the classes
         * are present.
         */
        if(mArrayId2Idx(moTree, (ClUint32T *)tmpCorPath.node, tmpCorPath.depth) != CL_OK)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
        idx = tmpCorPath.node[tmpCorPath.depth-1];  /* current index */
        node = mArrayNodeNextChildGet(node, &grpId, &idx);
        if(!node)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
        this->node[this->depth-1] = node->id;
                                                                                                                             
        CL_FUNC_EXIT();
        return(CL_OK);
      }
                                                                                                                             
    CL_FUNC_EXIT();
    return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
}



/**
 * Get the first child.
 *
 * Get the first child in the moTree. updates corPath (this) on
 * CL_OK
 *
 *  @param this: [IN/OUT] updated corPath is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */
ClRcT
_clCorMoClassPathFirstChildGet(ClCorMOClassPathPtrT this)
{
    MArrayNode_h node;
                                                                                                                             
    CL_FUNC_ENTER();
                                                                                                                             
    if (this == NULL)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
     
      if(!moTree)
         return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

    /* 1. locate the node
     * 2. get its first child
     * 3. update the index & return
     */
        if(this->depth)
        {
            node = corMOTreeFind(this);
        }
        else
        {
            node = moTree->root;
        }
    if(node)
      {
        ClUint32T grpId = 1;
        ClInt32T  idx = -1; /* get the first child */
                                                                                                                             
        node = mArrayNodeNextChildGet(node, &grpId, &idx);
        if(!node)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
        clCorMoClassPathAppend(this, node->id);
                                                                                                                             
        CL_FUNC_EXIT();
        return(CL_OK);
      }
    CL_FUNC_EXIT();
    return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
}
                                                                                                                             

