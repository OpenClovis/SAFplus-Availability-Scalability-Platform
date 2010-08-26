/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorMoIdUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Utility module that handles the MOID object
 *****************************************************************************/

/* FILES INCLUDED */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clCorApi.h>
#include <clCpmApi.h>

/* Internal Headers */
#include "clCorObj.h"
#include "clCorDmDefs.h"
#include "clCorPvt.h"
#include "clCorDmProtoType.h"
#include "clCorDeltaSave.h"


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* FORWARD DECLATIONS */
ClRcT clCorMoIdConcatenate(ClCorMOIdPtrT part1, ClCorMOIdPtrT part2, int copyWhere);
char * clCorMoClassQualifierStringGet(ClCorMoPathQualifierT qualifier);
extern ClRcT  corEOUpdatePwd (ClUint32T eoArg,  ClCorMOIdT* pwd, ClUint32T inLen,
                       char* outBuff, ClUint32T *outLen);




#if 0
/** 
 * Set the current moId path.
 * 
 * Set the current moId path for the caller EO. 
 *
 *  @param moIdH: path to set. This could be
 *   relative or absolute. If it is relative, it is
 *   appended to the current path. If this is absolute
 *   current path is replace by it.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoIdCd(ClCorMOIdPtrT moIdH)
{
    ClRcT rc = CL_OK;
    ClCorMOIdPtrT pwd = 0;
    clEoExecutionObjT* eoObj = NULL;

    CL_FUNC_ENTER();
    

    if (moIdH == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clEoMyEoObjectGet (&eoObj);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clEoMyEoObjectGet failed, rc = %x \n", rc));
        return rc;
    }

    if ((rc = clEoPrivateDataGet(eoObj,
                            CL_EO_COR_SERVER_COOKIE_ID,
			    (void **)&pwd)) != CL_OK) 
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    if (moIdH->qualifier == CL_COR_MO_PATH_RELATIVE)
    {
     /* if the new path is relative, concatenate with pwd */
         rc = clCorMoIdConcatenate(pwd, moIdH, 0); 
    } 
    else 
    {
        *pwd = *moIdH;
    }

    CL_FUNC_EXIT();
    return (rc);
}
/**
 * Get the current moId path.
 * 
 * Get the current moId path from the caller's EO context. 
 *
 *  @param moIdH: [OUT] current path is returned here.
 *                Caller allocate the memory for the moId.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoIdPwd(ClCorMOIdPtrT moIdH)
{
    ClRcT rc = CL_OK;
    ClCorMOIdPtrT pwd;
    clEoExecutionObjT* eoObj = NULL;


    CL_FUNC_ENTER();
    

    if (moIdH == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    rc = clEoMyEoObjectGet (&eoObj);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clEoMyEoObjectGet failed, rc = %x \n", rc));
        return rc;
    }

    if ((rc = clEoPrivateDataGet(eoObj,
                            CL_EO_COR_SERVER_COOKIE_ID,
			    (void **)&pwd)) != CL_OK) 
    {
        CL_FUNC_EXIT();
        return (rc);
    }

  /* 
   * pwd is either CL_COR_MO_PATH_ABSOLUTE or 
   * CL_COR_MO_PATH_RELATIVE_TO_BASE. It can never be
   * CL_COR_MO_PATH_RELATIVE!!!
   */
    CL_ASSERT (pwd->qualifier != CL_COR_MO_PATH_RELATIVE) 

    *moIdH = *pwd;

    CL_FUNC_EXIT();
    return (rc);
}
#endif 


/**
 * Get the first child.
 *
 * NOTE: The last node class tag has to be completed and instanceId is
 * to be left at zero, and that will be updated by this function.  Get
 * the First child Id updated.
 *
 *  @param this: [IN/OUT] updated ClCorMOId is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */
ClRcT
_clCorMoIdFirstInstanceGet(ClCorMOIdPtrT this)
{
    ObjTreeNode_h node = NULL;
                                                                                                                             
    CL_FUNC_ENTER();
                                                                                                                             
                                                                                                                             
    if (this == NULL)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    if(this->svcId != CL_COR_INVALID_SVC_ID)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
      }
    /* 1. locate the node
     * 2. get its first child
     * 3. update the index
     */
    node = corMOObjParentGet(this);
    if(node)
      {
        ClCorMOClassPathT tmpCorPath;
        ClUint32T grpId;
	ClInt32T tmpGrpId;
        ClInt32T idx = -1; /* get the first child */
                                                                                                                             
        /* convert the ClCorMOId to corPath and
         * get the group id for the child
         */
        clCorMoIdToMoClassPathGet(this, &tmpCorPath);
        /* get the index list from the moTree and check if the classes
         * are present.
         */
        if(mArrayId2Idx(moTree, (ClUint32T *)tmpCorPath.node, tmpCorPath.depth) != CL_OK)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
        grpId = tmpCorPath.node[tmpCorPath.depth-1];
	tmpGrpId = grpId;
        /*node = NULL;*/
        node = mArrayNodeNextChildGet(node, &grpId, &idx);
        if(!node || (grpId != tmpGrpId))
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
          }
	/* update instance id in moId.*/
        this->node[this->depth-1].instance = idx;
        CL_FUNC_EXIT();
        return(CL_OK);
      }
                                                                                                                             
    CL_FUNC_EXIT();
    return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
}
                                                                                                                             
/**
 * Get the Next Sibling.
 *
 * Get the Next Sibling in the MO Id tree.
 *
 *  @param this: [IN/OUT] updated ClCorMOId is returned.
 *                Caller allocate the memory for the moId.
 *
 *  @returns CL_OK on successs.
 *
 */
ClRcT
_clCorMoIdNextSiblingGet(ClCorMOIdPtrT this)
{
    ObjTreeNode_h node;
                                                                                                                             
    CL_FUNC_ENTER();
                                                                                                                             
                                                                                                                             
    if (this == NULL)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
      }
    if(this->svcId != CL_COR_INVALID_SVC_ID)
      {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
      }
                                                                                                                             
    /* 1. locate the node
     * 2. get its next sibling
     * 3. update the index
     */
    node = corMOObjParentGet(this);
    if(node && this->depth > 0)
      {
        ClCorMOClassPathT tmpCorPath;
        ClUint32T grpId;
        ClInt32T idx = this->node[this->depth-1].instance; /* get the next child */
                                                                                                                             
        /* convert the ClCorMOId to corPath and
         * get the group id for the child
         */
        clCorMoIdToMoClassPathGet(this, &tmpCorPath);
        /* get the index list from the moTree and check if the classes
         * are present.
         */
        if(mArrayId2Idx(moTree, (ClUint32T *) tmpCorPath.node, tmpCorPath.depth) != CL_OK)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND);
          }
        grpId = tmpCorPath.node[tmpCorPath.depth-1];
        node = mArrayNodeNextChildGet(node, &grpId, &idx);
        if(!node)
          {
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_NOT_FOUND);
          }
	/* update class and instance id in moId.*/
        this->node[this->depth-1].type = node->id;
        this->node[this->depth-1].instance = idx;
                                                                                                                             
        CL_FUNC_EXIT();
        return(CL_OK);
      }
                                                                                                                             
    CL_FUNC_EXIT();
    return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
}

/* Wrapper IPI to print the MoId in string for debugging purposes */
ClCharT* _clCorMoIdStrGet(ClCorMOIdPtrT moIdh, ClCharT* tmpStr)
{
    ClNameT moIdName;
    ClRcT rc = CL_OK;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};

    if(NULL == tmpStr)
    {
        clLogError("DBG", "XMO", "NULL pointer passed for the MOId string.");
        return NULL;
    }

    rc = _clCorMoIdToMoIdNameGet(moIdh, &moIdName);
    if (rc != CL_OK)
    {
        sprintf(tmpStr, " ");        
        return ((ClCharT*) tmpStr);
    }

    /* Append the service id information */
    snprintf(moIdStr, CL_MAX_NAME_LENGTH - strlen(":[Svc: ] "), "%s ", moIdName.value);

    snprintf(tmpStr, CL_MAX_NAME_LENGTH, "%s:[Svc:%d]", moIdStr, moIdh->svcId);

    /* Return the pointer to the information */    
    return ((ClCharT*) tmpStr);
}

ClRcT _corMoIdToClassGet(ClCorMOIdPtrT pMoId, ClCorClassTypeT* pClassId)
{
    ClCorMOClassPathT moPath;
    ClCorClassTypeT classId = 0;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    
    if (pMoId->svcId == CL_COR_INVALID_SRVC_ID)
    {
        classId = pMoId->node[pMoId->depth-1].type;
    }
    else
    {
        rc = clCorMoIdToMoClassPathGet(pMoId, &moPath);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the MO Path from the MoId. rc [0x%x]\n", rc));
            CL_FUNC_EXIT();
            return rc;
        }
 
        /* Get the class id from the mo path */
        rc = _corMOPathToClassIdGet(&(moPath), pMoId->svcId, &classId);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the class id from mo path. rc [0x%x]\n", rc));
            CL_FUNC_EXIT();
            return rc;
        }
    }

    *pClassId = classId;

    CL_FUNC_EXIT();
    return CL_OK;
}

CORAttr_h dmClassAttrInfoGet(CORClass_h classH, ClCorAttrPathT* pAttrPath, ClCorAttrIdT attrId)
{
    CORAttr_h attrH = NULL;
    CORClass_h tmpClassH = NULL;
    ClUint32T idx = 0;
   
    CL_FUNC_ENTER();
    
    tmpClassH = classH;
    
    for (idx=0; idx < pAttrPath->depth; idx++)
    {
        attrH = dmClassAttrGet(tmpClassH, pAttrPath->node[idx].attrId);

        if (attrH != NULL)
        {
            tmpClassH = dmClassGet(attrH->attrType.u.remClassId);
        }
        else 
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Attr Info not found for the attribute [0x%x]", attrId));
            CL_FUNC_EXIT();
            return (NULL);
        }
    }

    /* Now tmpClassH will contain the last class handle */
    attrH = dmClassAttrGet(tmpClassH, attrId);

    if (attrH == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Attr Info not found for the attribute [0x%x]", attrId));
        CL_FUNC_EXIT();
        return (NULL);
    }

    CL_FUNC_EXIT();
    return (attrH); 
}

ClRcT _clCorUtilMoAndMSOCreate(ClCorMOIdPtrT pMoId)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId = {{{0}}};
    ClCorMOIdT tempMoId = {{{0}}};
    ClCorMOClassPathT moClassPath = {{0}};
    ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};
    CORMOClass_h moClassHandle;
    CORMSOClass_h msoClassHandle;
    ClCorServiceIdT svcId = -1;

    if (!pMoId)
    {
        clLogError("UTIL", "MOMSOCREATE", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    memcpy((void *) &moId, (void *) pMoId, sizeof(ClCorMOIdT));

    clCorMoIdServiceSet(&moId, CL_COR_SVC_ERR_INVALID_ID);

    _clCorMoIdStrGet(&moId, moIdStr);

    rc = clCorMoIdToMoClassPathGet(&moId, &moClassPath); 
    if (rc != CL_OK)
    {
        clLogError("UTIL", "MOMSOCREATE", "Failed to get mo class path from moId. rc [0x%x]", rc);
        return rc;
    }

    rc = corMOClassHandleGet(&moClassPath, &moClassHandle);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MOMSOCREATE", "Failed to get MO class handle. rc [0x%x]", rc);
        return rc;
    }

    rc = _corMOObjCreate(&moId);
    if (rc != CL_OK && 
            CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MO_ALREADY_PRESENT)
    {
        clLogError("UTL", "MOMSOCREATE", "Failed to create Node MO object [%s]. rc [0x%x]", 
                moIdStr, rc);
        return rc;
    }
    
    memcpy(&tempMoId, &moId, sizeof(ClCorMOIdT));

    /* Store the MO into delta db. */
    if (rc == CL_OK)
        clCorDeltaDbStore(tempMoId, NULL, CL_COR_DELTA_MO_CREATE);

    for (svcId=CL_COR_SVC_ID_DEFAULT+1; svcId<CL_COR_SVC_ID_MAX; svcId++)
    {
        rc = corMSOClassHandleGet(moClassHandle, svcId, &msoClassHandle);
        if (rc == CL_OK)
        {
            rc = _corMSOObjCreate(&moId, svcId);
            if (rc != CL_OK && 
                    CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MSO_ALREADY_PRESENT)
            {
                clLogError("UTL", "MOMSOCREATE", "Failed to create Node Mso object [%s] for svcId [%d]. rc [0x%x]", 
                        moIdStr, svcId, rc);
                return rc;
            }

            /* Store the MSO into delta db. */
            if (rc == CL_OK)
            { 
                clCorMoIdServiceSet(&tempMoId, svcId);
                clCorDeltaDbStore(tempMoId, NULL, CL_COR_DELTA_MSO_CREATE);
            }
        }
    }

    return CL_OK;
}
