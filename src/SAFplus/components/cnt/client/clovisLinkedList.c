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
 * ModuleName  : cnt
 * File        : clovisLinkedList.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains a simple Linked-List implementation for the Container  
 * library.                                                                 
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clCommon.h>
#include <clCntApi.h>
#include <clCntErrors.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clRuleApi.h>
#if HA_ENABLED
#include <clovisCps.h>
#include "cps_main.h"
#endif
#include "clovisContainer.h"
#include "clovisBaseLinkedList.h"
#include "clovisLinkedList.h"
#include <clDebugApi.h>

#ifdef ORDERED_LIST

#define CL_CNT_ORDERED_LIST_KEY 0
static ClRcT
cclOrderedLinkedListContainerNodeFind (
        ClCntHandleT        containerHandle,
        ClCntKeyHandleT     userKey,
        ClCntNodeHandleT    *pNodeHandle);

static ClRcT
cclOrderedLinkedListContainerKeySizeGet (
        ClCntHandleT containerHandle,
        ClCntKeyHandleT userKey,
        ClUint32T* pSize);

extern  ClRcT
cclOrderedLinkedListContainerNodeAdd (
        ClCntHandleT containerHandle, 
        ClCntKeyHandleT userKey,
        ClCntDataHandleT userData,
        ClRuleExprT* pExp);

#endif
/*#include <containerCompId.h>*/
/******************************************************/
static ClRcT
cclLinkedListContainerNodeAdd (ClCntHandleT containerHandle,
                               ClCntKeyHandleT userKey,
                               ClCntDataHandleT userData,
                               ClRuleExprT* pExp);
static ClRcT
cclLinkedListContainerKeyDelete (ClCntHandleT containerHandle,
                                 ClCntKeyHandleT userKey);
static ClRcT
cclLinkedListContainerNodeDelete (ClCntHandleT containerHandle,
                                  ClCntNodeHandleT nodeHandle);
static ClRcT
cclLinkedListContainerNodeFind (ClCntHandleT containerHandle,
                                ClCntKeyHandleT userKey,
                                ClCntDataHandleT* pNodeHandle);
static ClRcT
cclLinkedListContainerFirstNodeGet (ClCntHandleT containerHandle,
                                    ClCntNodeHandleT* pNodeHandle);
static ClRcT
cclLinkedListContainerLastNodeGet (ClCntHandleT containerHandle,
                                   ClCntNodeHandleT* pNodeHandle);
static ClRcT
cclLinkedListContainerNextNodeGet (ClCntHandleT containerHandle,
                                   ClCntNodeHandleT currentNodeHandle,
                                   ClCntNodeHandleT* pNextNodeHandle);
static ClRcT
cclLinkedListContainerPreviousNodeGet (ClCntHandleT containerHandle,
                                       ClCntNodeHandleT nodeHandle,
                                       ClCntNodeHandleT* pPreviousNodeHandle);
static ClRcT
cclLinkedListContainerSizeGet (ClCntHandleT containerHandle,
                               ClUint32T* pSize);
static ClRcT
cclLinkedListContainerKeySizeGet (ClCntHandleT containerHandle,
                                  ClCntKeyHandleT userKey,
                                  ClUint32T* pSize);
static ClRcT
cclLinkedListContainerUserKeyGet(ClCntHandleT containerHandle,
                                 ClCntNodeHandleT nodeHandle,
                                 ClCntKeyHandleT* pUserKey);
static ClRcT
cclLinkedListContainerUserDataGet(ClCntHandleT containerHandle,
                                  ClCntNodeHandleT nodeHandle,
                                  ClCntDataHandleT *pUserDataHandle);

static ClRcT
cclLinkedListContainerDestroy (ClCntHandleT containerHandle);

static LinkedListNode_t*
cclLinkedListContainerNodeLocate (LinkedListHead_t* pLlHead,
					  ClCntKeyHandleT userKey);

/*******************************************************/

/* HA-aware container Modifications start : Hari*/
#if HA_ENABLED
ClRcT
cclHALinkedListCreate (ClCntKeyCompareCallbackT fpKeyCompare,
                       ClCntDeleteCallbackT      fpUserDeleteCallback,
                       ClCntDeleteCallbackT      fpUserDestroyCallback,
                       haInfo_t                 *pHaInfo,
                       ClCntHandleT     *pContainerHdl)
{
  ClRcT            retCode = CL_OK;
  LinkedListHead_t* pLlHead = NULL;

  /*Create the container */
  if (CL_OK != (retCode = clCntLlistCreate (fpKeyCompare,
                                            fpUserDeleteCallback,
                                            fpUserDestroyCallback,
                                            CL_CNT_NON_UNIQUE_KEY,
                                            pContainerHdl)))
  {
     /* Some thing went wrong!!*/
     return retCode;
  }
  pLlHead = (LinkedListHead_t*)(*pContainerHdl);
  pLlHead->container.haEnabled = TRUE;
  pLlHead->container.cpsHdl = pHaInfo->cpsHdl;
  pLlHead->container.containerId = pHaInfo->containerId;
  /* Let CPS know of this container */
  return clCPContainerReg(pHaInfo->cpsHdl,
                          (ClCntHandleT)pContainerHdl,
                          pHaInfo->domainId,
                          pHaInfo->containerId,
                          pHaInfo->frequency,
                          pHaInfo->order,
                          pHaInfo->autoCPOff,
                          pHaInfo->serialiser,
                          pHaInfo->deSerialiser);
}
#endif
/* HA-aware container Modifications End: Hari*/

/*******************************************************/
ClRcT
clCntThreadSafeLlistCreate(ClCntKeyCompareCallbackT  fpKeyCompare,
                           ClCntDeleteCallbackT      fpUserDeleteCallback,
                           ClCntDeleteCallbackT      fpUserDestroyCallback,
				           ClCntKeyTypeT             containerKeyType,
                           ClCntHandleT              *pContainerHandle)
{
    ClRcT            rc       = CL_OK;
    LinkedListHead_t *pLlHead = NULL;

    rc = clCntLlistCreate(fpKeyCompare, fpUserDeleteCallback,
                          fpUserDestroyCallback, containerKeyType, 
                          pContainerHandle);
    if( CL_OK != rc )
    {
        return rc;
    }
    pLlHead = (LinkedListHead_t *) *pContainerHandle;
    rc = clOsalMutexCreate(&pLlHead->listMutex);
    if( CL_OK != rc )
    {
        pLlHead->listMutex = CL_HANDLE_INVALID_VALUE;
        clCntDelete(*pContainerHandle);
        returnCntError(rc, "Mutex creation failed");
    }
    return rc;
}

ClRcT
clCntLlistCreate(ClCntKeyCompareCallbackT  fpKeyCompare,
                 ClCntDeleteCallbackT      fpUserDeleteCallback,
                 ClCntDeleteCallbackT      fpUserDestroyCallback,
				 ClCntKeyTypeT             containerKeyType,
                 ClCntHandleT              *pContainerHandle)
{
  LinkedListHead_t *pLlHead = NULL;
	
  nullChkRet(pContainerHandle);
  nullChkRet(fpKeyCompare);

  if(containerKeyType >= CL_CNT_INVALID_KEY_TYPE)
  {
      returnCntError(CL_ERR_INVALID_PARAMETER, 
                    "Passed containerType is not valid");
  }

  pLlHead = (LinkedListHead_t*) clHeapCalloc (1, (ClUint32T)sizeof (LinkedListHead_t));
  if (pLlHead == NULL) {
    /* clHeapAllocate failures are not a very nice thing to happen! DEBUG MESSAGE */
    *pContainerHandle = (ClCntHandleT)NULL;
    returnCntError(CL_ERR_NO_MEMORY, "memory allocation failed");
  }

  /* this field has been added to identify the type of the handle. */
  pLlHead->pFirstNode = NULL;
  pLlHead->pLastNode  = NULL;
  pLlHead->listMutex  = CL_HANDLE_INVALID_VALUE;

  //pLlHead->container.type        = (ClTypeT *) CL_CNT_LINK_LIST_HEAD;
  pLlHead->container.fpFunctionContainerNodeAdd = 
    cclLinkedListContainerNodeAdd;
  
  pLlHead->container.fpFunctionContainerKeyDelete =
    cclLinkedListContainerKeyDelete;
  
  pLlHead->container.fpFunctionContainerNodeDelete =
    cclLinkedListContainerNodeDelete;
  
  pLlHead->container.fpFunctionContainerNodeFind = 
    cclLinkedListContainerNodeFind;
  
  pLlHead->container.fpFunctionContainerFirstNodeGet = 
    cclLinkedListContainerFirstNodeGet;
  
  pLlHead->container.fpFunctionContainerLastNodeGet = 
    cclLinkedListContainerLastNodeGet;
  
  pLlHead->container.fpFunctionContainerNextNodeGet = 
    cclLinkedListContainerNextNodeGet;

  pLlHead->container.fpFunctionContainerPreviousNodeGet = 
    cclLinkedListContainerPreviousNodeGet;

  pLlHead->container.fpFunctionContainerSizeGet = 
    cclLinkedListContainerSizeGet;

  pLlHead->container.fpFunctionContainerKeySizeGet = 
    cclLinkedListContainerKeySizeGet;
	
  pLlHead->container.fpFunctionContainerUserDataGet =
    cclLinkedListContainerUserDataGet;

  pLlHead->container.fpFunctionContainerUserKeyGet =
    cclLinkedListContainerUserKeyGet;
    
  pLlHead->container.fpFunctionContainerKeyCompare =
    fpKeyCompare;
	
  pLlHead->container.fpFunctionContainerDestroy =
    cclLinkedListContainerDestroy;
    
  pLlHead->container.fpFunctionContainerDeleteCallBack =
    fpUserDeleteCallback;
  
  pLlHead->container.fpFunctionContainerDestroyCallBack =
    fpUserDestroyCallback;
    
  pLlHead->container.validContainer = CONTAINER_ID;

  if( CL_CNT_UNIQUE_KEY == containerKeyType )
  {
    pLlHead->container.isUniqueFlag = 1;
  }
  else
  {
    pLlHead->container.isUniqueFlag = 0;
  }
  *pContainerHandle = (ClCntHandleT) pLlHead;
  
  return CL_OK;  
}

static ClRcT
cclBasedLinkedListCreateNAdd(ClCntKeyHandleT   userKey, 
                             ClCntDataHandleT  userData, 
                             LinkedListNode_t  *pLlNewNode, 
                             ClRuleExprT       *pExp)
{
    ClCntHandleT  hTempCnt  = CL_HANDLE_INVALID_VALUE;
    ClRcT         errorCode = CL_OK;

    errorCode = cclBaseLinkedListCreate(&hTempCnt);
    if(errorCode != CL_OK)
    {
      returnCntError(errorCode, "Basic Linked list create failed"); 
    }
    pLlNewNode->containerHandle = (ClCntDataHandleT) hTempCnt;

    return cclBaseLinkedListContainerNodeAdd(hTempCnt, userKey, userData, 
                                             (ClCntNodeHandleT) pLlNewNode, pExp);    
}

static ClRcT
cclLinkedListContainerNodeAdd (ClCntHandleT     containerHandle,
                               ClCntKeyHandleT  userKey,
                               ClCntDataHandleT userData,
                               ClRuleExprT      *pExp)
{
  LinkedListHead_t  *pLlHead    = NULL;
  LinkedListNode_t  *pLlNewNode = NULL;
  LinkedListNode_t  *pLlNode    = NULL;
  ClRcT             errorCode   = CL_OK;
  
  pLlHead = (LinkedListHead_t*) containerHandle;

  CL_CNT_LOCK(pLlHead->listMutex);

  if( ((pLlHead->pFirstNode == NULL) && (pLlHead->pLastNode  != NULL)) || 
      ((pLlHead->pLastNode == NULL) && (pLlHead->pFirstNode != NULL)) )
  {
      /* sanity check */
      CL_CNT_UNLOCK(pLlHead->listMutex);
      returnCntError(CL_ERR_INVALID_STATE, 
              ("Container Internals got corrupted"));
  }
  /* make sure we don't have any node with a similar key already */
  pLlNode = cclLinkedListContainerNodeLocate(pLlHead, userKey);
  if (pLlNode != NULL) 
  {
    /* there's already a node with this key in the linked-list */
    /* if the isUniqueFlag is true */
    if((pLlHead->container.isUniqueFlag) != 0) 
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_DUPLICATE, "Node is already present");
    }
    errorCode =  cclBaseLinkedListContainerNodeAdd((ClCntHandleT) pLlNode->containerHandle,
                                                    userKey, userData, 
                                                    (ClCntNodeHandleT) pLlNode, pExp);
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return errorCode;
  }  
  
  pLlNewNode = (LinkedListNode_t*) clHeapCalloc (1, (ClUint32T)sizeof (LinkedListNode_t));
  if (pLlNewNode == NULL)
  {
    /* clHeapAllocate failures are not a very nice thing to happen! DEBUG MESSAGE */
     CL_CNT_UNLOCK(pLlHead->listMutex);
     returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }
  //pLlNewNode->type          = (ClTypeT *) CL_CNT_LINK_LIST_NODE;
  pLlNewNode->pNextNode     = NULL;
  pLlNewNode->pPreviousNode = NULL;
  pLlNewNode->userKey       = userKey;
  pLlNewNode->hListHead     = containerHandle;
  
  /* find an appropriate location */
  if( ( NULL == pLlHead->pFirstNode) && (pLlHead->pLastNode == NULL) )
  {
    /* the new node we are adding is the only node in the list */
    pLlHead->pFirstNode = pLlHead->pLastNode = pLlNewNode;
  }
  /* if the new-node-key is lesser than the first-node's key ... */
  else if (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, 
                                                            pLlHead->pFirstNode->userKey) < 0)
  {
    /* new-node must become the first node */
    pLlNewNode->pNextNode              = pLlHead->pFirstNode;
    pLlHead->pFirstNode->pPreviousNode = pLlNewNode;
    pLlHead->pFirstNode                = pLlNewNode;
  } 
  /* if the new-node-key is greater than the last-node's key ... */
  else if(pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey,
                                                           pLlHead->pLastNode->userKey) > 0) 
  {
    /* new-node must become the last node */
    pLlNewNode->pPreviousNode     = pLlHead->pLastNode;
    pLlHead->pLastNode->pNextNode = pLlNewNode;
    pLlHead->pLastNode            = pLlNewNode;
  }
  /* ok! now we got to figure out a good location within the linked list for this new-node */
  else
  {
    pLlNode = pLlHead->pFirstNode;
    while ((pLlNode) && (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, 
                                                                          pLlNode->userKey) > 0)) 
    {
        pLlNode = pLlNode->pNextNode;
    /* need to check if pLlNode is null or reached the last node */
    }
    if ((pLlNode) && (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, pLlNode->userKey) < 0)) 
    {
        /* good! */
        pLlNewNode->pNextNode     = pLlNode;
        pLlNewNode->pPreviousNode = pLlNode->pPreviousNode;
        if( pLlNode->pPreviousNode )
        {
            pLlNode->pPreviousNode->pNextNode = pLlNewNode;
        }
        if( pLlNode->pPreviousNode )
        {
            pLlNode->pPreviousNode   = pLlNewNode;
        }
    }
    else
    {
        clHeapFree(pLlNewNode);
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_INVALID_STATE, ("Couldn't find proper place for this Node"));
    }
  }
  errorCode = cclBasedLinkedListCreateNAdd(userKey, userData, pLlNewNode, pExp);
  if( CL_OK != errorCode )
  {
      clHeapFree(pLlNewNode);
      CL_CNT_UNLOCK(pLlHead->listMutex);
      returnCntError(errorCode, ("cclBasedLinkedListCreateNAdd failed"));
  }
 
  CL_CNT_UNLOCK(pLlHead->listMutex);
  return CL_OK;
}
/*******************************************************/
static ClRcT
cclLinkedListContainerKeyDelete(ClCntHandleT    containerHandle,
                                ClCntKeyHandleT userKey)
{
    LinkedListHead_t      *pLlHead    = NULL;
    LinkedListNode_t      *pLlNode    = NULL;
    ClCntNodeHandleT      currentNode = CL_HANDLE_INVALID_VALUE;
    BaseLinkedListNode_t  *pBaseNode  = NULL;
    ClRcT                 errorCode   = CL_OK;

    /* Head handle validations */
    pLlHead = (LinkedListHead_t*)containerHandle;

    CL_CNT_LOCK(pLlHead->listMutex);
    pLlNode = cclLinkedListContainerNodeLocate (pLlHead, userKey);
    if(pLlNode == NULL)
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "The Node doesn't exist");
    }

    /* check this Node's list, some Nodes are there */
    errorCode = clCntFirstNodeGet((ClCntHandleT) pLlNode->containerHandle, 
                                   &currentNode);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(errorCode, "Node doesn't exist");
    }

    /* check its the first node in the list */
    if (pLlHead->pFirstNode == pLlNode) 
    {
        pLlHead->pFirstNode = pLlNode->pNextNode;
    }
    /* check its the last node in the list */
    if( pLlHead->pLastNode == pLlNode )
    {
        pLlHead->pLastNode = pLlNode->pPreviousNode;
    }
    /* at this point in time, the previous and next pointers have to be valid */
    if( NULL != pLlNode->pPreviousNode )
    {
        pLlNode->pPreviousNode->pNextNode = pLlNode->pNextNode;
    }
    if( NULL != pLlNode->pNextNode )
    {
        pLlNode->pNextNode->pPreviousNode = pLlNode->pPreviousNode;
    }

    if(NULL != pLlHead->container.fpFunctionContainerDeleteCallBack) 
    {
        do
        {
            pBaseNode = (BaseLinkedListNode_t *) currentNode;

            pLlHead->container.fpFunctionContainerDeleteCallBack(pBaseNode->userKey, pBaseNode->userData);
            pBaseNode->userKey = 0; pBaseNode->userData = 0;
            
            errorCode = clCntNextNodeGet((ClCntHandleT) pLlNode->containerHandle, 
                    currentNode, &currentNode);
            if(errorCode != CL_OK) 
            {
                break;
            }
        }while(1);
    }
    errorCode = clCntDelete((ClCntHandleT) pLlNode->containerHandle);
    if(errorCode == CL_OK)
    {
        clHeapFree (pLlNode);
        CL_CNT_UNLOCK(pLlHead->listMutex);
        return CL_OK;
    }
    /* what do we do here, do we need to revert back which is not good. */

    CL_CNT_UNLOCK(pLlHead->listMutex);
    return errorCode;
}
/*******************************************************/
static ClRcT
cclLinkedListContainerNodeDelete (ClCntHandleT     containerHandle,
                                  ClCntNodeHandleT nodeHandle)
{
  BaseLinkedListNode_t  *pLlNode            = NULL;
  LinkedListNode_t      *tempLlNode         = NULL;
  LinkedListHead_t      *pLlHead            = NULL;
  ClCntNodeHandleT      tempNodeHandle      = 0;
  ClCntHandleT          tempContainerHandle = 0;
  ClCntKeyHandleT       userKey             = 0;
  ClCntDataHandleT      userData            = 0;
  ClRcT                 errorCode           = CL_OK;
	
  /* Container handle validations */
  pLlHead = (LinkedListHead_t*) containerHandle;

  /* Node handle validations */
  pLlNode = (BaseLinkedListNode_t *) nodeHandle;
  nullChkRet(pLlNode);
  if(pLlNode->validNode != NODE_ID) 
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "The Node handle is not valid");
  }
   
  CL_CNT_LOCK(pLlHead->listMutex);
  /* Validating the node handle belongs to this container */
  tempLlNode = (LinkedListNode_t*) pLlNode->parentNodeHandle;
  if( tempLlNode->hListHead != containerHandle )
  {
      CL_CNT_UNLOCK(pLlHead->listMutex); 
      returnCntError(CL_ERR_INVALID_HANDLE,
                     "Node handle doesn't belong to this ContainerHandle");
  }
  userKey    = tempLlNode->userKey;  
  userData   = pLlNode->userData;

  /*Found the proper node */
  tempContainerHandle = (ClCntHandleT)tempLlNode->containerHandle;
  errorCode = clCntNodeDelete(tempContainerHandle, nodeHandle);
  if(errorCode == CL_OK)
  {
      /*check if the base linked list container is empty*/
      if(clCntFirstNodeGet(tempContainerHandle, &tempNodeHandle) != CL_OK) 
      {
          errorCode = clCntDelete(tempContainerHandle);
          /* clHeapFree the linked list node*/

          if (pLlHead->pFirstNode == tempLlNode)
          {
              pLlHead->pFirstNode = tempLlNode->pNextNode;
          }

          if( pLlHead->pLastNode == tempLlNode)
          {
              pLlHead->pLastNode = tempLlNode->pPreviousNode;
          }
          /* at this point in time, the previous and next pointers have to be valid */
          if (tempLlNode->pPreviousNode != NULL) 
          {
              tempLlNode->pPreviousNode->pNextNode = tempLlNode->pNextNode;
          }

          if (tempLlNode->pNextNode != NULL) 
          {
              tempLlNode->pNextNode->pPreviousNode = tempLlNode->pPreviousNode;
          }
          clHeapFree (tempLlNode);
      }
      else
      {
          /*update the key*/
          tempLlNode->userKey = ((BaseLinkedListNode_t*)tempNodeHandle)->userKey;
      }
      if(NULL != pLlHead->container.fpFunctionContainerDeleteCallBack)
      {
          pLlHead->container.fpFunctionContainerDeleteCallBack(userKey, userData);
      }
      CL_CNT_UNLOCK(pLlHead->listMutex);
      return (CL_OK);
  }
  else
  {
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return errorCode;
  }

}
/*******************************************************/
static ClRcT
cclLinkedListContainerNodeFind (ClCntHandleT     containerHandle,
                                ClCntKeyHandleT  userKey,
                                ClCntNodeHandleT *pNodeHandle)
{
  LinkedListHead_t  *pLlHead       = NULL;
  LinkedListNode_t  *pLlNode       = NULL;
  ClCntNodeHandleT  tempNodeHandle = 0;
  ClRcT             errorCode      = CL_OK;
  
  pLlHead = (LinkedListHead_t*) containerHandle;
  
  CL_CNT_LOCK(pLlHead->listMutex);
  pLlNode = cclLinkedListContainerNodeLocate (pLlHead, userKey);
  if(pLlNode == NULL)
  {
      CL_CNT_UNLOCK(pLlHead->listMutex);
      returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
  }

  errorCode = clCntFirstNodeGet((ClCntHandleT) pLlNode->containerHandle, 
                                &tempNodeHandle);
  if( CL_OK != errorCode )
  {
      CL_CNT_UNLOCK(pLlHead->listMutex);
      returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
  }

  *pNodeHandle = tempNodeHandle;
  CL_CNT_UNLOCK(pLlHead->listMutex);
  return CL_OK;
}

static ClRcT
cclLinkedListContainerFirstNodeGet (ClCntHandleT      containerHandle,
                                    ClCntNodeHandleT  *pNodeHandle)
{
    LinkedListHead_t  *pLlHead            = NULL;
    ClCntNodeHandleT  tempNodeHandle      = 0;
    ClCntHandleT      tempContainerHandle = 0;
    ClRcT             errorCode           = CL_OK;

    pLlHead = (LinkedListHead_t*)containerHandle;

    CL_CNT_LOCK(pLlHead->listMutex);
    if(pLlHead->pFirstNode == NULL) 
    { 
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
    }
    tempContainerHandle = (ClCntHandleT)(pLlHead->pFirstNode->containerHandle);
    errorCode = clCntFirstNodeGet(tempContainerHandle,&tempNodeHandle);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
    }
    if( CL_HANDLE_INVALID_VALUE == tempNodeHandle )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
    }

    *pNodeHandle = tempNodeHandle;  
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return (CL_OK);
}
/*******************************************************/
static ClRcT
cclLinkedListContainerLastNodeGet (ClCntHandleT     containerHandle,
                                   ClCntNodeHandleT *pNodeHandle)
{
    LinkedListHead_t  *pLlHead       = NULL;
    ClCntNodeHandleT  tempNodeHandle = 0;
    ClRcT             errorCode      = CL_OK;

    pLlHead = (LinkedListHead_t*)containerHandle;
 
    CL_CNT_LOCK(pLlHead->listMutex);
    if(pLlHead->pLastNode == NULL) 
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Last node doesn't exist");
    }
    errorCode = clCntLastNodeGet(
                               (ClCntHandleT) (pLlHead->pLastNode->containerHandle),
                               &tempNodeHandle);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Last node doesn't exist");
    }

    *pNodeHandle = tempNodeHandle;  
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return CL_OK;
}
/*******************************************************/
static ClRcT
cclLinkedListContainerNextNodeGet (ClCntHandleT      containerHandle,
                                   ClCntNodeHandleT  currentNodeHandle,
                                   ClCntNodeHandleT  *pNextNodeHandle)
{
  BaseLinkedListNode_t  *pLlNode            = NULL;
  LinkedListHead_t      *pLlHead            = NULL;
  LinkedListNode_t      *tempLlNode         = NULL;
  ClCntNodeHandleT      tempNodeHandle      = 0;
  ClRcT                 errorCode           = CL_OK;
	
  /* Container handle validations */
  *pNextNodeHandle = (ClCntHandleT) NULL;
  pLlHead = (LinkedListHead_t *) containerHandle;	
  
  /* Node handle validations */
  pLlNode = (BaseLinkedListNode_t*) currentNodeHandle;
  nullChkRet(pLlNode);
  if(pLlNode->validNode != NODE_ID) 
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "The Node handle is not valid");
  }

  /* This node handle belongs to this container ? */
  CL_CNT_LOCK(pLlHead->listMutex);
  tempLlNode          = (LinkedListNode_t *) pLlNode->parentNodeHandle;
  if( tempLlNode->hListHead != containerHandle )
  {
      CL_CNT_UNLOCK(pLlHead->listMutex); 
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "This node handle doesn't belong to this container");
  }
  
  /*Found the proper node, Now find the next node */
  errorCode = clCntNextNodeGet((ClCntHandleT) tempLlNode->containerHandle, 
                                currentNodeHandle, &tempNodeHandle);
  if( CL_OK != errorCode )
  {
      /* may be the next node will be in the next keys list */
      tempLlNode = tempLlNode->pNextNode;
      /* chck NULL pointer and return */
      if( NULL == tempLlNode )
      {
          CL_CNT_UNLOCK(pLlHead->listMutex);
          returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
      }
      errorCode = clCntFirstNodeGet( 
              (ClCntHandleT) tempLlNode->containerHandle, &tempNodeHandle);
      if( CL_OK != errorCode )
      {
          CL_CNT_UNLOCK(pLlHead->listMutex);
          returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
      }
  }
  *pNextNodeHandle = tempNodeHandle;

  CL_CNT_UNLOCK(pLlHead->listMutex);
  return (CL_OK);
}
/*******************************************************/
static ClRcT
cclLinkedListContainerPreviousNodeGet (ClCntHandleT containerHandle,
                                       ClCntNodeHandleT currentNodeHandle,
                                       ClCntNodeHandleT* pPreviousNodeHandle)
{
    BaseLinkedListNode_t *pLlNode            = NULL;
    LinkedListHead_t     *pLlHead            = NULL;
    LinkedListNode_t     *tempLlNode         = NULL;
    ClCntHandleT         tempContainerHandle = 0;
    ClCntNodeHandleT     tempNodeHandle      = 0;
    ClRcT                errorCode           = CL_OK;

    /* containe handle validations */
    pLlHead = (LinkedListHead_t *) containerHandle;

    /* Node handle validations */
    pLlNode = (BaseLinkedListNode_t*) currentNodeHandle;
    nullChkRet(pLlNode);
    if(pLlNode->validNode != NODE_ID) 
    {
        returnCntError(CL_ERR_INVALID_HANDLE, "Passed handle is invalid");
    }


    CL_CNT_LOCK(pLlHead->listMutex);
    /* Get the previous node */
    tempLlNode          = (LinkedListNode_t *)pLlNode->parentNodeHandle;
    if( tempLlNode->hListHead != containerHandle )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex); 
        returnCntError(CL_ERR_INVALID_HANDLE, 
                "This node handle doesn't belong to this container");
    }

    tempContainerHandle = (ClCntHandleT)tempLlNode->containerHandle;  
    errorCode = clCntPreviousNodeGet(tempContainerHandle, currentNodeHandle, &tempNodeHandle);
    if( CL_OK != errorCode )
    {
        /* go back to the list */
        tempLlNode = tempLlNode->pPreviousNode;
        /* chk the null pointer */
        //nullChkRet(tempLlNode);
        if( NULL == tempLlNode )
        {
            CL_CNT_UNLOCK(pLlHead->listMutex);
            returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
        }
        /* get the previous key list first node */
        tempContainerHandle = (ClCntHandleT) tempLlNode->containerHandle;  
        errorCode = clCntLastNodeGet(tempContainerHandle,&tempNodeHandle);
        if( CL_OK != errorCode )
        {
            CL_CNT_UNLOCK(pLlHead->listMutex);
            returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
        }
    }

    *pPreviousNodeHandle = tempNodeHandle;
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return (CL_OK);
}
/*******************************************************/
static ClRcT
cclLinkedListContainerSizeGet(ClCntHandleT containerHandle,
                              ClUint32T    *pSize)
{
    LinkedListHead_t  *pLlHead  = NULL;
    LinkedListNode_t  *pLlNode  = NULL;
    ClUint32T         numNodes  = 0;
    ClUint32T         tempNum   = 0;
    ClRcT             errorCode = CL_OK;

    pLlHead = (LinkedListHead_t*) containerHandle;

    CL_CNT_LOCK(pLlHead->listMutex);
    *pSize  = 0;
    pLlNode = pLlHead->pFirstNode;
    while (pLlNode) 
    {
        tempNum = 0;
        errorCode = clCntSizeGet((ClCntHandleT) pLlNode->containerHandle,
                                 &tempNum);
        if( CL_OK != errorCode )
        {
            CL_CNT_UNLOCK(pLlHead->listMutex);
            returnCntError(errorCode, "Container size get failed");
        }
        pLlNode   = pLlNode->pNextNode;
        numNodes += tempNum;
    }

    *pSize = numNodes;
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return CL_OK;
}
/*******************************************************/
static ClRcT
cclLinkedListContainerKeySizeGet (ClCntHandleT    containerHandle,
                                  ClCntKeyHandleT userKey,
                                  ClUint32T       *pSize)
{
    LinkedListHead_t  *pLlHead  = NULL;
    LinkedListNode_t  *pLlNode  = NULL;
    ClUint32T         numNodes  = 0;
    ClRcT             errorCode = CL_OK;

    /* Validations */
    pLlHead = (LinkedListHead_t*) containerHandle;
    nullChkRet(pLlHead);

    /* Initialized to 0, safer side */
    *pSize = 0;
    CL_CNT_LOCK(pLlHead->listMutex);
    /* Locate the node */
    pLlNode = cclLinkedListContainerNodeLocate(pLlHead, userKey);
    if(pLlNode == NULL) 
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Key doesn't exist");
    }
    errorCode = clCntSizeGet((ClCntHandleT) pLlNode->containerHandle, 
                              &numNodes);
    if( CL_OK != errorCode )
    {
        CL_CNT_UNLOCK(pLlHead->listMutex);
        returnCntError(errorCode, "clCntSizeGet failed");
    }

    *pSize = numNodes;
    CL_CNT_UNLOCK(pLlHead->listMutex);
    return CL_OK;
}

static ClRcT
cclLinkedListContainerUserKeyGet(ClCntHandleT     containerHandle,
                                 ClCntNodeHandleT nodeHandle,
                                 ClCntKeyHandleT* pUserKey)
{
  BaseLinkedListNode_t *pTemp   = NULL;
  LinkedListNode_t     *pLlNode = NULL;

  /* Handle validations */
  pTemp = (BaseLinkedListNode_t*) nodeHandle;
  nullChkRet(pTemp);
  if( NODE_ID != pTemp->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "Passed handle is invalid");
  }
  
  pLlNode = (LinkedListNode_t *) pTemp->parentNodeHandle;  
  if( pLlNode->hListHead != containerHandle )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "This node handle doesn't belong to this container");
  }
  *pUserKey = pLlNode->userKey;

  return (CL_OK);
}
/*******************************************************/
static ClRcT
cclLinkedListContainerUserDataGet(ClCntHandleT      containerHandle,
                                  ClCntNodeHandleT  nodeHandle,
                                  ClCntDataHandleT  *pUserDataHandle)
{
  BaseLinkedListNode_t *pTemp   = NULL;
  LinkedListNode_t     *pLlNode = NULL;

  /* Handle Validations */
  pTemp = (BaseLinkedListNode_t*) nodeHandle;
  nullChkRet(pTemp);
  if( NODE_ID != pTemp->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "Passed node handle is invalid");
  }

  /* Chk this node belongs this container */
  pLlNode = (LinkedListNode_t *) pTemp->parentNodeHandle;  
  if( pLlNode->hListHead != containerHandle )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "This node handle doesn't belong to this container");
  }
  
  *pUserDataHandle = pTemp->userData;

  return (CL_OK);
}
/*******************************************************/
static LinkedListNode_t*
cclLinkedListContainerNodeLocate (LinkedListHead_t  *pLlHead,
                                  ClCntKeyHandleT   userKey)
{
  LinkedListNode_t* pLlNode = NULL;

  pLlNode = pLlHead->pFirstNode;

  while (pLlNode) {
    if (pLlHead->container.fpFunctionContainerKeyCompare(pLlNode->userKey, userKey) == 0) {
      break;
    }
    pLlNode = pLlNode->pNextNode;
  }
  return (pLlNode);

}

/*******************************************************/
static ClRcT
cclLinkedListContainerDestroy (ClCntHandleT containerHandle)
{
LinkedListHead_t       *pLlHead     = NULL;
  LinkedListNode_t     *pLlNode     = NULL;
  LinkedListNode_t     *pLlNextNode = NULL;
  ClCntNodeHandleT     currentNode  = 0;
  BaseLinkedListNode_t *pBaseNode   = NULL;
  ClRcT                errorCode    = CL_OK;
  	
  /* Handle validations */
  pLlHead = (LinkedListHead_t*) containerHandle;
  
  CL_CNT_LOCK(pLlHead->listMutex);
  /* walk thru all the nodes and destroy */
  for (pLlNode = pLlHead->pFirstNode; pLlNode != NULL; pLlNode = pLlNextNode)
  {
    pLlNextNode  = pLlNode->pNextNode;
    if(NULL != pLlHead->container.fpFunctionContainerDestroyCallBack)
    {	  
    	/* get the first node */
    	errorCode = clCntFirstNodeGet((ClCntHandleT) pLlNode->containerHandle,
                                       &currentNode);
    	if(errorCode != CL_OK) 
        {
            CL_CNT_UNLOCK(pLlHead->listMutex);
            returnCntError(errorCode, "First node get failed");
    	}
    	do
    	{
       		pBaseNode = (BaseLinkedListNode_t *)currentNode;
  
       		pLlHead->container.fpFunctionContainerDestroyCallBack(pBaseNode->userKey, pBaseNode->userData);
            pBaseNode->userKey=NULL;
            pBaseNode->userData=NULL;
     
       		errorCode = clCntNextNodeGet(
                        (ClCntHandleT) pLlNode->containerHandle, 
                        currentNode, &currentNode);
       		if(errorCode != CL_OK)
            {
         		break;
       		}
 
    	}while(1);
	}
    errorCode = clCntDelete((ClCntHandleT) pLlNode->containerHandle);
    if(errorCode != CL_OK)
    {
       CL_CNT_UNLOCK(pLlHead->listMutex);
       returnCntError(errorCode, "clCntDelete of BL failed");
    }
    clHeapFree (pLlNode);
  }

  CL_CNT_UNLOCK(pLlHead->listMutex);
  CL_CNT_DESTROY(pLlHead->listMutex);
  clHeapFree (pLlHead);
  return (CL_OK);
}
/*******************************************************/
#ifdef ORDERED_LIST
ClRcT
clCntOrderedLlistCreate (
        ClCntKeyCompareCallbackT    fpKeyCompare,
        ClCntDeleteCallbackT        fpUserDeleteCallback,
        ClCntDeleteCallbackT        fpUserDestroyCallback,
        ClCntKeyTypeT               containerKeyType,
        ClCntHandleT                *pContainerHandle)
{
  LinkedListHead_t  *pLlHead  = NULL;
	
  nullChkRet(fpKeyCompare);
  nullChkRet(pContainerHandle);
  if(containerKeyType >= CL_CNT_INVALID_KEY_TYPE)
  {
      returnCntError(CL_ERR_INVALID_PARAMETER, 
              "Container key type is not proper");
  }
  pLlHead = (LinkedListHead_t*) clHeapCalloc(1, (ClUint32T)sizeof (LinkedListHead_t));
  if (pLlHead == NULL) 
  {
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }
  pLlHead->pFirstNode = NULL;
  pLlHead->pLastNode  = NULL;
  pLlHead->listMutex  = CL_HANDLE_INVALID_VALUE;
  //pLlHead->container.type      = (ClTypeT *) CL_CNT_LINK_LIST_HEAD;

  pLlHead->container.fpFunctionContainerNodeAdd = 
      cclOrderedLinkedListContainerNodeAdd;
  /*
   * No container key delete function as all entries will map to same key
   */
  pLlHead->container.fpFunctionContainerKeyDelete = NULL;
  
  pLlHead->container.fpFunctionContainerNodeDelete = 
      cclLinkedListContainerNodeDelete;

  pLlHead->container.fpFunctionContainerNodeFind = 
      cclOrderedLinkedListContainerNodeFind;

  pLlHead->container.fpFunctionContainerFirstNodeGet = 
      cclLinkedListContainerFirstNodeGet;

  pLlHead->container.fpFunctionContainerLastNodeGet = 
    cclLinkedListContainerLastNodeGet;

  pLlHead->container.fpFunctionContainerNextNodeGet = 
    cclLinkedListContainerNextNodeGet;

  pLlHead->container.fpFunctionContainerPreviousNodeGet = 
    cclLinkedListContainerPreviousNodeGet;

  pLlHead->container.fpFunctionContainerSizeGet = 
    cclLinkedListContainerSizeGet;

  pLlHead->container.fpFunctionContainerKeySizeGet = 
    cclOrderedLinkedListContainerKeySizeGet;
	
  pLlHead->container.fpFunctionContainerUserDataGet =
    cclLinkedListContainerUserDataGet;

  pLlHead->container.fpFunctionContainerUserKeyGet =
    cclLinkedListContainerUserKeyGet;
    
  pLlHead->container.fpFunctionContainerKeyCompare =
    fpKeyCompare;
	
  pLlHead->container.fpFunctionContainerDestroy =
    cclLinkedListContainerDestroy;
    
  pLlHead->container.fpFunctionContainerDeleteCallBack =
    fpUserDeleteCallback;
  
  pLlHead->container.fpFunctionContainerDestroyCallBack =
    fpUserDestroyCallback;
    
  pLlHead->container.isUniqueFlag = 0;

  pLlHead->container.validContainer = CONTAINER_ID;

  *pContainerHandle = (ClCntHandleT)pLlHead;
  
  return (CL_OK);  
}
/*******************************************************/
ClRcT cclOrderedLinkedListContainerNodeAdd (
        ClCntHandleT        containerHandle, 
        ClCntKeyHandleT     userKey,
        ClCntDataHandleT    userData,
        ClRuleExprT         *pExp)
{
  LinkedListHead_t  *pLlHead    = NULL;
  LinkedListNode_t  *pLlNewNode = NULL;
  LinkedListNode_t  *pLlNode    = NULL;
  ClCntNodeHandleT  dummyHandle = 0;
  ClCntHandleT      tempContainerHandle = 0;
  ClRcT             errorCode = CL_OK;
  ClCntKeyCompareCallbackT  keyCompareFunc;
  
  /* Container handle validations */
  pLlHead = (LinkedListHead_t*) containerHandle;
  nullChkRet(pLlHead);

  /* perform the sanity check */
  if( ((pLlHead->pFirstNode == NULL) && (pLlHead->pLastNode != NULL)) ||
      ((pLlHead->pLastNode == NULL) && (pLlHead->pFirstNode != NULL)) )
  {
      returnCntError(CL_ERR_INVALID_STATE, "List got corrupted");
  }
  /* Get the key compare function */
  keyCompareFunc = pLlHead->container.fpFunctionContainerKeyCompare;

  /* make sure we don't have any node with a similar key already */
  pLlNode = cclLinkedListContainerNodeLocate(pLlHead, userKey);
  if (pLlNode != NULL) 
  {
    /* there's already a node with this key in the linked-list */
    tempContainerHandle = (ClCntHandleT)pLlNode->containerHandle;	
    dummyHandle = (ClCntNodeHandleT)pLlNode;
    return cclBaseOrderedLinkedListContainerNodeAdd(tempContainerHandle, userKey, userData, dummyHandle, pExp,
            keyCompareFunc);
  }  
  pLlNewNode = (LinkedListNode_t*) clHeapCalloc (1, (ClUint32T)sizeof (LinkedListNode_t));
  if (pLlNewNode == NULL)
  {
    /* clHeapAllocate failures are not a very nice thing to happen! DEBUG MESSAGE */
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }

  pLlNewNode->pNextNode     = NULL;
  pLlNewNode->pPreviousNode = NULL;
  pLlNewNode->userKey       = userKey;
  //pLlNewNode->type          = (ClTypeT *) CL_CNT_LINK_LIST_NODE;
  pLlNewNode->hListHead     = containerHandle;
  
  if (pLlHead->pFirstNode == NULL) 
  {
    /* the new node we are adding is the only node in the list */
    pLlHead->pFirstNode = pLlHead->pLastNode = pLlNewNode;
    errorCode = cclBaseLinkedListCreate(&tempContainerHandle);
    if(errorCode != CL_OK) 
    {
        clHeapFree(pLlNewNode);
        return errorCode;
    }
    dummyHandle = (ClCntNodeHandleT)pLlNewNode;	
    pLlNewNode->containerHandle = (ClCntDataHandleT)tempContainerHandle;
    return cclBaseOrderedLinkedListContainerNodeAdd(tempContainerHandle, userKey, userData, dummyHandle, pExp,
            keyCompareFunc);    
  }
  /* find an appropriate location */
  /* if the new-node-key is lesser than the first-node's key ... */
  if (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, pLlHead->pFirstNode->userKey) < 0) 
  {
    /* new-node must become the first node */
    pLlNewNode->pNextNode               = pLlHead->pFirstNode;
    pLlHead->pFirstNode->pPreviousNode  = pLlNewNode;
    pLlHead->pFirstNode                 = pLlNewNode;
	
    errorCode = cclBaseLinkedListCreate(&tempContainerHandle);
    if(errorCode != CL_OK) 
    { 
        clHeapFree(pLlNewNode);
        return errorCode;
    }
    dummyHandle = (ClCntNodeHandleT)pLlNewNode;	
    pLlNewNode->containerHandle = (ClCntDataHandleT)tempContainerHandle;
    return cclBaseOrderedLinkedListContainerNodeAdd(tempContainerHandle, userKey, userData, dummyHandle, pExp,
            keyCompareFunc);
  }

  /* if the new-node-key is greater than the last-node's key ... */
  if ( pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, pLlHead->pLastNode->userKey) > 0) 
  {
      /* new-node must become the last node */
      pLlNewNode->pPreviousNode = pLlHead->pLastNode;
      pLlHead->pLastNode->pNextNode = pLlNewNode;
      pLlHead->pLastNode = pLlNewNode;
      errorCode = cclBaseLinkedListCreate(&tempContainerHandle);
      if(errorCode != CL_OK) 
      {
          clHeapFree(pLlNewNode);
          return errorCode;
      }
      dummyHandle = (ClCntNodeHandleT)pLlNewNode;	
      pLlNewNode->containerHandle = (ClCntDataHandleT)tempContainerHandle;
      return cclBaseOrderedLinkedListContainerNodeAdd(tempContainerHandle,userKey, userData, dummyHandle, pExp,
              keyCompareFunc);
  } 
  
  /* ok! now we got to figure out a good location within the linked list for this new-node */
  pLlNode = pLlHead->pFirstNode;

  while ((pLlNode) && (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, pLlNode->userKey) > 0)) 
  {
      pLlNode = pLlNode->pNextNode;
      /* need to check if pLlNode is null or reached the last node */
  }
  
  if ((pLlNode) && (pLlHead->container.fpFunctionContainerKeyCompare(pLlNewNode->userKey, pLlNode->userKey) < 0)) 
  {
      /* good! */
      LinkedListNode_t* pPreviousNode;
      LinkedListNode_t* pNextNode; 
      pNextNode = pLlNode;
      pPreviousNode = pLlNode->pPreviousNode; 
      pLlNewNode->pPreviousNode = pPreviousNode;
      pLlNewNode->pNextNode = pNextNode; 
      if (pPreviousNode) 
      {
          pPreviousNode->pNextNode = pLlNewNode;
      }
      if (pNextNode) 
      {
          pNextNode->pPreviousNode = pLlNewNode;
      } 
      
      errorCode = cclBaseLinkedListCreate(&tempContainerHandle);
      if(errorCode != CL_OK) 
      { 
          clHeapFree(pLlNewNode);
          return errorCode;
      }	  
      dummyHandle = (ClCntNodeHandleT)pLlNewNode;
      pLlNewNode->containerHandle = (ClCntDataHandleT)tempContainerHandle;
      return cclBaseOrderedLinkedListContainerNodeAdd(tempContainerHandle, userKey, userData, dummyHandle, pExp,
              keyCompareFunc); 
  }
 
  errorCode = CL_CNT_RC(CL_ERR_INVALID_STATE);
  return (errorCode);
}
/*******************************************************/
static ClRcT
cclOrderedLinkedListContainerNodeFind (
        ClCntHandleT        containerHandle,
        ClCntKeyHandleT     userKey,
        ClCntNodeHandleT    *pNodeHandle)
{
    return cclLinkedListContainerNodeFind (containerHandle,userKey, pNodeHandle);
}
/*******************************************************/
static ClRcT
cclOrderedLinkedListContainerKeySizeGet (
        ClCntHandleT containerHandle,
        ClCntKeyHandleT userKey,
        ClUint32T* pSize)
{
    return cclLinkedListContainerKeySizeGet (containerHandle,userKey, pSize);
}
/*******************************************************/
#endif /* #ifdef ORDERED_LIST */ 

