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
 * ModuleName  : cnt
 * File        : clovisContainer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file containers the Clovis Container library implementation.         
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <clCommon.h>
#include <clCntApi.h>
#include <clCntErrors.h>
#include <clCommonErrors.h>
#include <clRuleApi.h>
#include "clovisContainer.h"
#include "clovisBaseLinkedList.h"
#if HA_ENABLED
#include "cps_main.h"
#endif 
#include <clDebugApi.h>
/*#include <containerCompId.h>*/

/*******************************************************/
ClRcT clCntNodeAddAndNodeGet(ClCntHandleT containerHandle,
           ClCntKeyHandleT userKey,
           ClCntDataHandleT userData,
           ClRuleExprT* pExp, ClCntNodeHandleT* pNodeHandle)
{
    ClRcT rc = CL_OK;

    nullChkRet(pNodeHandle);

    rc = clCntNodeAdd(containerHandle, userKey, userData, pExp);
    if(CL_OK != rc)
    {
        return rc;
    }
    rc = clCntNodeFind (containerHandle, userKey, pNodeHandle);
    CL_ASSERT(rc == CL_OK); /* I just added it, so I should be able to find it! */
    
    if(CL_OK != rc)
    {
        return rc;
    }
        
    return CL_OK;
}

/*******************************************************/
ClRcT
clCntNodeAdd(ClCntHandleT     containerHandle,
             ClCntKeyHandleT  userKey,
             ClCntDataHandleT userData,
             ClRuleExprT      *pExp)
{
  CclContainer_t  *pContainer = NULL;
  ClRcT           errorCode   = CL_OK;
  
  pContainer = (CclContainer_t *) containerHandle;

  nullChkRet(pContainer);
  if( CONTAINER_ID != pContainer->validContainer)
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "Passed container handle is invalid");
  }

  if( NULL == pContainer->fpFunctionContainerNodeAdd )
  {
      returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }
 
  errorCode = pContainer->fpFunctionContainerNodeAdd (containerHandle,
                                                  userKey,
                                                  userData,
                                                  pExp);
  /* HA-aware Container Modifications Start: Hari*/

#if HA_ENABLED
  /* Addition is succesfull and the container is ha enabled*/
  if((pContainer->haEnabled)&& (errorCode == CL_OK))
  {
    /* Update the standby component */
    errorCode = clCPContainerElemSynch(pContainer->cpsHdl,
                           pContainer->domainId,
                           pContainer->containerId,
                           userKey,
                           0xFFFF);  /* ELEMENTADD =  0xFFFFF */
  }
#endif /*HA_ENABLED*/
  /* HA-aware Container Modifications End: Hari*/
  return (errorCode);

}
/*******************************************************/
ClRcT
clCntAllNodesDelete(ClCntHandleT containerHandle)
{
  CclContainer_t   *pContainer       = NULL;
  ClCntNodeHandleT containerNode     = CL_HANDLE_INVALID_VALUE;
  ClCntNodeHandleT nextContainerNode = CL_HANDLE_INVALID_VALUE;
  ClRcT            errorCode         = CL_OK;
  
  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  if(pContainer->validContainer != CONTAINER_ID)
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
              "Passed container handle is invalid");
  }

  errorCode = clCntFirstNodeGet(containerHandle, &containerNode);
  if(CL_OK != errorCode) 
  {
      /*
       * Check if container is empty.
       */
      if(CL_GET_ERROR_CODE(errorCode) == CL_ERR_NOT_EXIST)
          return CL_OK;
      return errorCode;
  }

  while(containerNode)
  {
      if(clCntNextNodeGet(containerHandle, containerNode, &nextContainerNode) != CL_OK)
      {
          nextContainerNode = 0;
      }
  
      errorCode = clCntNodeDelete(containerHandle, containerNode);
	  if(errorCode != CL_OK)
	  {
		  return(errorCode);
	  }
      containerNode = nextContainerNode;
  }

  return (CL_OK);
}
/*******************************************************/
ClRcT
clCntAllNodesForKeyDelete(ClCntHandleT    containerHandle,
                          ClCntKeyHandleT userKey)
{
  CclContainer_t  *pContainer = NULL;
  
  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  
  if(pContainer->validContainer != CONTAINER_ID)
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
              "Passed container handle is invalid");
  }

  if (pContainer->fpFunctionContainerKeyDelete == NULL) 
  {
      returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

#if HA_ENABLED
  /* HA-aware container Modifications Start: Hari*/
  /* This element is going to be deleted. Let the redundancy peer know it*/
  if(pContainer->haEnabled)
  {
      /* Update the standby component */
      errorCode = clCPContainerElemSynch(pContainer->cpsHdl,
                             pContainer->domainId,
                             pContainer->containerId,
                             userKey,
                             0x0000);  /* ELEMENTDEL =  0x00000*/
  }
  /* HA-aware container Modifications End: Hari*/
#endif /*HA_ENABLED*/

  return (pContainer->fpFunctionContainerKeyDelete (containerHandle,
					       userKey));
}
/*******************************************************/

ClRcT
clCntNodeDelete(ClCntHandleT      containerHandle,
                ClCntNodeHandleT  nodeHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  pContainer = (CclContainer_t *) containerHandle;

  nullChkRet(pContainer);
  
  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }

  if (pContainer->fpFunctionContainerNodeDelete == NULL) 
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

#if HA_ENABLED
  /* HA-aware container Modifications Start: Hari*/
  /* This element is going to be deleted. Let the redundancy peer know it*/
  if(pContainer->haEnabled)
  {
      ClCntKeyHandleT  userKey;

      if (CL_OK != clCntNodeUserKeyGet(containerHandle,
                                  nodeHandle,
                                  &userKey))
      {
         errorCode =CL_CNT_RC(CL_ERR_INVALID_STATE);
         CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nInvalid state"));
         return (errorCode);
      }

      /* Update the standby component */
      errorCode = clCPContainerElemSynch(pContainer->cpsHdl,
                             pContainer->domainId,
                             pContainer->containerId,
                             userKey,
                             0x0000);  /* ELEMENTDEL =  0x00000*/
  }
#endif 
  /* HA-aware container Modifications End: Hari*/

  return (pContainer->fpFunctionContainerNodeDelete (containerHandle,
						     nodeHandle));
}
/*******************************************************/
ClRcT
clCntNodeFind(ClCntHandleT      containerHandle,
              ClCntKeyHandleT   userKey,
              ClCntNodeHandleT  *pNodeHandle)
{
  CclContainer_t        *pContainer    = NULL;
  ClCntNodeHandleT      tempNodeHandle = CL_HANDLE_INVALID_VALUE;
  ClRcT                 errorCode      = CL_OK;
  BaseLinkedListNode_t  *pBLlNode      = NULL;

  nullChkRet(pNodeHandle);
  *pNodeHandle = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);

  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }

  if (pContainer->fpFunctionContainerNodeFind == NULL) 
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

  errorCode = (pContainer->fpFunctionContainerNodeFind(containerHandle,
						   userKey, &tempNodeHandle));
  
  if(errorCode != CL_OK )
  {
    return (errorCode);
  }
  else
  {
    pBLlNode     = (BaseLinkedListNode_t *) tempNodeHandle;
    *pNodeHandle = tempNodeHandle;
    return (CL_OK);
  }
  
}
/*******************************************************/
ClRcT
clCntNodeUserKeyGet(ClCntHandleT     containerHandle,
                    ClCntNodeHandleT nodeHandle, 
                    ClCntKeyHandleT* pUserKey)
{
    void            *pNode      = NULL;
    CclContainer_t  *pContainer = NULL;
    ClRcT           errorCode   = CL_OK;
    ClCntKeyHandleT tempKey     = CL_HANDLE_INVALID_VALUE;

    /* User key validations */
    nullChkRet(pUserKey);
    *pUserKey = CL_HANDLE_INVALID_VALUE;

    /*Container handle validations */
    pContainer = (CclContainer_t *) containerHandle;
    nullChkRet(pContainer);
    if(pContainer->validContainer != CONTAINER_ID)
    {
        returnCntError(CL_ERR_INVALID_HANDLE, "Passed handle is invalid");
    }
    /* Node Handle validations */
    pNode = (void*) nodeHandle;
    nullChkRet(pNode);

    errorCode = (pContainer->fpFunctionContainerUserKeyGet(containerHandle, 
                nodeHandle,
                &tempKey));
    if( CL_OK != errorCode )
    {
        return errorCode;
    }
    *pUserKey = tempKey;
    return CL_OK;
}
/*******************************************************/
ClRcT
clCntDataForKeyGet(ClCntHandleT      containerHandle,
                   ClCntKeyHandleT   userKey,
                   ClCntDataHandleT  *pUserData)
{
  ClCntNodeHandleT  containerNode = CL_HANDLE_INVALID_VALUE;
  CclContainer_t    *pContainer   = NULL;
  ClRcT             errorCode     = CL_OK;

  nullChkRet(pUserData);
  *pUserData = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }

  if(pContainer->isUniqueFlag == 0) 
  {
      returnCntError(CL_ERR_NOT_IMPLEMENTED,
              "This feature is not applicaple for non unique container");
  }

  errorCode = clCntNodeFind(containerHandle, userKey, &containerNode);
  if(errorCode != CL_OK)
  {
      returnCntError(CL_ERR_NOT_EXIST, "Key doesn't exist");
  }

  errorCode = clCntNodeUserDataGet(containerHandle, containerNode, pUserData);
  if(errorCode != CL_OK)
  {
    return(errorCode);
  }

  return(CL_OK);
}
/*******************************************************/
ClRcT
clCntNodeUserDataGet(ClCntHandleT      containerHandle,
                     ClCntNodeHandleT  nodeHandle, 
                     ClCntDataHandleT  *pUserDataHandle)
{
    void             *pNode         = NULL;
    CclContainer_t   *pContainer    = NULL;
    ClRcT            errorCode      = CL_OK;
    ClCntDataHandleT tempDataHandle = CL_HANDLE_INVALID_VALUE;

    nullChkRet(pUserDataHandle);
    *pUserDataHandle = CL_HANDLE_INVALID_VALUE;

    pContainer = (CclContainer_t *) containerHandle;
    nullChkRet(pContainer);

    if(pContainer->validContainer != CONTAINER_ID)
    {
        returnCntError(CL_ERR_INVALID_HANDLE, 
                "Passed container handle is invalid");
    }
    pNode = (void*) nodeHandle;
    nullChkRet(pNode);

    errorCode = (pContainer->fpFunctionContainerUserDataGet(containerHandle, nodeHandle,
                &tempDataHandle));
    if( CL_OK != errorCode )
    {
        return errorCode;
    }
    *pUserDataHandle = tempDataHandle;
    return (CL_OK);
}
/*******************************************************/
ClRcT
clCntFirstNodeGet(ClCntHandleT      containerHandle,
                  ClCntNodeHandleT  *pNodeHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  nullChkRet(pNodeHandle);
  *pNodeHandle = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  
  if(pContainer->validContainer != CONTAINER_ID) 
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }

  if (pContainer->fpFunctionContainerFirstNodeGet == NULL)
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

  return (pContainer->fpFunctionContainerFirstNodeGet (containerHandle,pNodeHandle));
}
/*******************************************************/
ClRcT
clCntLastNodeGet(ClCntHandleT     containerHandle,
                 ClCntNodeHandleT *pNodeHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  nullChkRet(pNodeHandle);
  *pNodeHandle = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);

  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }

  if (pContainer->fpFunctionContainerLastNodeGet == NULL)
  {
    /* can't be right! DEBUG MESSAGE */
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

  return (pContainer->fpFunctionContainerLastNodeGet (containerHandle, pNodeHandle));
}
/*******************************************************/
ClRcT
clCntNextNodeGet(ClCntHandleT     containerHandle,
                 ClCntNodeHandleT currentNodeHandle,
                 ClCntNodeHandleT *pNextNodeHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  nullChkRet(pNextNodeHandle);
  *pNextNodeHandle = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  
  if(pContainer->validContainer != CONTAINER_ID)
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "Passed handle is invalid");
  }

  if (pContainer->fpFunctionContainerNextNodeGet == NULL) 
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
    /* can't be right! DEBUG MESSAGE */
  }

  return (pContainer->fpFunctionContainerNextNodeGet (containerHandle,
  					              currentNodeHandle, pNextNodeHandle));
}
/*******************************************************/
ClRcT
clCntPreviousNodeGet(ClCntHandleT containerHandle,
                     ClCntNodeHandleT currentNodeHandle,
                     ClCntNodeHandleT* pPreviousNodeHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  nullChkRet(pPreviousNodeHandle);
  *pPreviousNodeHandle = CL_HANDLE_INVALID_VALUE;

  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }
  
  if (pContainer->fpFunctionContainerPreviousNodeGet == NULL) 
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

  return (pContainer->fpFunctionContainerPreviousNodeGet (containerHandle,
							  currentNodeHandle, pPreviousNodeHandle));
}
/*******************************************************/
ClRcT
clCntWalk(ClCntHandleT        containerHandle,
          ClCntWalkCallbackT  fpUserWalkCallback,
          ClCntArgHandleT     userDataArg,
          ClInt32T            dataLength)
{
    CclContainer_t       *pContainer   = NULL;
    ClCntNodeHandleT     containerNode = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT     nextContainerNode = CL_HANDLE_INVALID_VALUE;
    ClCntDataHandleT     userData      = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT      userKey       = CL_HANDLE_INVALID_VALUE;
    BaseLinkedListNode_t *pTemp        = NULL;
    ClRcT                errorCode     = CL_OK;
 
    nullChkRet(fpUserWalkCallback);

    pContainer = (CclContainer_t *) containerHandle;
    nullChkRet(pContainer);

    if(pContainer->validContainer != CONTAINER_ID)
    {
        returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
    }
  
    errorCode = clCntFirstNodeGet (containerHandle, &containerNode);
    if(errorCode != CL_OK)
    { 
        if((CL_GET_ERROR_CODE(errorCode)) == CL_ERR_NOT_EXIST) 
        {
            /* If the container is empty it means we dont have to walk anymore.
             * Hence return OK.
             */
            return (CL_OK);
        }
        return(errorCode);	  
    }

    while (containerNode)
    {
        pTemp = (BaseLinkedListNode_t*) containerNode;
        if(clCntNextNodeGet(containerHandle, containerNode, &nextContainerNode) != CL_OK)
        {
            nextContainerNode = 0;
        }
        if ((pTemp->pRbeExpression == NULL) || (clRuleExprEvaluate(pTemp->pRbeExpression, (ClUint32T*) userDataArg, dataLength)))
        {
            errorCode = clCntNodeUserKeyGet (containerHandle, containerNode, &userKey);
            errorCode = clCntNodeUserDataGet (containerHandle, containerNode, &userData);
  
            errorCode = fpUserWalkCallback(userKey, userData, userDataArg,dataLength);
            if(CL_OK != errorCode)
            {
                return (errorCode);
            }
        }
        containerNode = nextContainerNode;
    }

    return(CL_OK);
}
/*******************************************************/
ClRcT
clCntSizeGet(ClCntHandleT containerHandle,
             ClUint32T    *pSize)
{
    CclContainer_t *pContainer = NULL;

    nullChkRet(pSize);
    *pSize = 0;

    pContainer = (CclContainer_t *) containerHandle;
    nullChkRet(pContainer);

    if(pContainer->validContainer != CONTAINER_ID)
    {
        returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
    }
    if (pContainer->fpFunctionContainerSizeGet == NULL) 
    {
        /* can't be right! DEBUG MESSAGE */
        returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
    }

    return (pContainer->fpFunctionContainerSizeGet (containerHandle, pSize));
}
/*******************************************************/
ClRcT
clCntKeySizeGet(ClCntHandleT    containerHandle,
                ClCntKeyHandleT userKey,
                ClUint32T       *pSize)
{
  CclContainer_t  *pContainer = NULL;

  nullChkRet(pSize);
  *pSize = 0;
  
  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);

  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }
  if (pContainer->fpFunctionContainerKeySizeGet == NULL) 
  {
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

  return (pContainer->fpFunctionContainerKeySizeGet (containerHandle, userKey, pSize));
}
/*******************************************************/
ClRcT
clCntDelete(ClCntHandleT containerHandle)
{
  CclContainer_t  *pContainer = NULL;
  
  pContainer = (CclContainer_t *) containerHandle;
  nullChkRet(pContainer);
  
  if(pContainer->validContainer != CONTAINER_ID)
  {
    returnCntError(CL_ERR_INVALID_HANDLE, "Passed container handle is invalid");
  }
  if (pContainer->fpFunctionContainerDestroy == NULL) 
  {
    /* can't be right! DEBUG MESSAGE */
    returnCntError(CL_ERR_INVALID_STATE, "Container got corrupted");
  }

#if HA_ENABLED
  /* HA-aware container Modifications Start: Hari*/
  /* The container is getting destroyed. So de-register it with CPS*/
  if (pContainer->haEnabled)
  {
       errorCode = clCPContainerDeReg(pContainer->cpsHdl,
                          pContainer->domainId,
                          pContainer->containerId);
  }
#endif
  /* HA-aware container Modifications End: Hari*/
  return (pContainer->fpFunctionContainerDestroy (containerHandle));
}
/*******************************************************/

ClRcT clCntNonUniqueKeyDelete(ClCntHandleT container, ClCntKeyHandleT key, ClCntDataHandleT givenData, 
                              ClInt32T (*cmp)(ClCntDataHandleT data1, ClCntDataHandleT data2))
{
    ClUint32T sz = 0;
    ClRcT rc = CL_OK;
    ClCntNodeHandleT node = 0;
    ClCntNodeHandleT nextNode = 0;
    ClCntDataHandleT data = 0;

    if(!container || !key || !givenData || !cmp) 
        return CL_CNT_RC(CL_ERR_INVALID_PARAMETER);

    rc = clCntNodeFind(container, key, &node);
    if(rc != CL_OK)
        return rc;
    rc = clCntKeySizeGet(container, key, &sz);
    if(rc != CL_OK)
        return rc;
    while(sz-- > 0)
    {
        rc = clCntNodeUserDataGet(container, node, &data);
        if(rc != CL_OK)
            return rc;
        if(!cmp(givenData, data))
            return clCntNodeDelete(container, node);
        if(sz > 0) 
        {
            nextNode = 0;
            rc = clCntNextNodeGet(container, node, &nextNode);
            if(rc != CL_OK)
                break;
            node = nextNode;
        }
    }
    return CL_CNT_RC(CL_ERR_NOT_EXIST);
}

ClRcT clCntNonUniqueKeyFind(ClCntHandleT container, ClCntKeyHandleT key, ClCntDataHandleT givenData,
                            ClInt32T (*cmp)(ClCntDataHandleT data1, ClCntDataHandleT data2), 
                            ClCntDataHandleT *pDataHandle)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT node = 0;
    ClCntNodeHandleT nextNode = 0;
    ClCntDataHandleT data = 0;
    ClUint32T sz = 0;

    if(!container || !key || !givenData || !cmp || !pDataHandle)
        return CL_CNT_RC(CL_ERR_INVALID_PARAMETER);

    *pDataHandle = 0;

    rc = clCntNodeFind(container, key, &node);
    if(rc != CL_OK) return rc;

    rc = clCntKeySizeGet(container, key, &sz);
    if(rc != CL_OK) return rc;
    
    while(sz-- > 0 && node)
    {
        rc = clCntNodeUserDataGet(container, node, &data);
        if(rc != CL_OK)
            return rc;
        nextNode = 0;
        if(sz > 0)
        {
            rc = clCntNextNodeGet(container, node, &nextNode);
            if(rc != CL_OK)
                nextNode = 0;
        }
        if(!cmp(givenData, data))
        {
            *pDataHandle = data;
            return CL_OK;
        }
        node = nextNode;
    }
    
    return CL_CNT_RC(CL_ERR_NOT_EXIST);
}
