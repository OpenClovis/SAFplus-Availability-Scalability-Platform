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
 * File        : clovisRedBlackTree.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains a Red-Black Tree (balanced binary tree) implementation 
 * for the container library.                                                
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
#include "clovisContainer.h"
#include "clovisRedBlackTree.h"
#include "clovisBaseLinkedList.h"
#if HA_ENABLED
#include <clovisCps.h>
#include "cps_main.h"
#endif 
#include <clDebugApi.h>
/*#include <containerCompId.h>*/

/******************************************************************************/
typedef enum {CCL_RBTREE_BLACK,CCL_RBTREE_RED} CclRBTreeColor_e;
typedef enum {CCL_RBTree_FALSE, CCL_RBTree_TRUE} CclRBTreeBool_e;
/******************************************************************************/
typedef struct node {
  ClCntKeyHandleT key;
  ClCntDataHandleT data;
} CclNode_t;
/******************************************************************************/
typedef struct rbt_node {
  ClCntKeyHandleT userKey;
  ClCntHandleT containerHandle;
  struct rbt_node* pLeft;
  struct rbt_node* pRight;
  struct rbt_node* pParent;
  CclRBTreeColor_e  color;
} ElementHandle_t;
/******************************************************************************/
typedef struct rbt_head{
  CclContainer_t  container;
  ElementHandle_t* pFirstNode;
  ElementHandle_t* pSentinel;
  int  size;
  ClOsalMutexIdT  treeMutex;
} CclRBTreeHead_t;
/******************************************************************************/
static ClRcT
cclRBTreeContainerNodeAdd (ClCntHandleT rbtHandle,
                           ClCntKeyHandleT userKey, 
                           ClCntDataHandleT userData,
                           ClRuleExprT* pExp);
/******************************************************************************/
static ClRcT
cclRBTreeContainerKeyDelete (ClCntHandleT  rbtHandle, 
	                     ClCntKeyHandleT  userKey);
/******************************************************************************/
static ClRcT
cclRBTreeContainerNodeDelete (ClCntHandleT  rbtHandle, 
	                      ClCntNodeHandleT nodeHandle);
/******************************************************************************/
static ClRcT
cclRBTreeContainerDestroy (ClCntHandleT  rbtHandle);
/******************************************************************************/
static ClRcT
cclRBTreeContainerSizeGet (ClCntHandleT  rbtHandle,
                           ClUint32T* pSize);
/******************************************************************************/
static ClRcT
cclRBTreeContainerKeySizeGet (ClCntHandleT  rbtHandle,
                              ClCntKeyHandleT userKey,
                              ClUint32T* pSize);
/******************************************************************************/
static ClRcT
cclRBTreeContainerNodeFind (ClCntHandleT  rbtHandle, 
                            ClCntKeyHandleT  userKey,
                            ClCntNodeHandleT* pNodeHandle);
/******************************************************************************/
static ClRcT
cclRBTreeContainerFirstNodeGet (ClCntHandleT  rbtHandle,
                                ClCntNodeHandleT* pNodeHandle);
/******************************************************************************/
static ClRcT
cclRBTreeContainerLastNodeGet (ClCntHandleT  rbtHandle,
                               ClCntNodeHandleT* pNodeHandle);
/******************************************************************************/
static ClRcT
cclRBTreeContainerNextNodeGet (ClCntHandleT  rbtHandle, 
                               ClCntNodeHandleT  currentRBTElement,
                               ClCntNodeHandleT* pNextRBTElement);
/******************************************************************************/
static ClRcT
cclRBTreeContainerPreviousNodeGet (ClCntHandleT  rbtHandle, 
                                   ClCntNodeHandleT  currentRBTElement,
                                   ClCntNodeHandleT* pPreviousRBTElement);
/******************************************************************************/
static ClRcT
cclRBTreeContainerUserKeyGet (ClCntHandleT  rbtHandle,
                              ClCntNodeHandleT nodeHandle,
                              ClCntKeyHandleT *pUserKey);
/******************************************************************************/
static ClRcT
cclRBTreeContainerUserDataGet (ClCntHandleT  rbtHandle,
                               ClCntNodeHandleT nodeHandle,
                               ClCntDataHandleT *pUserData);
/******************************************************************************/
static void
cclRBTreeRightNodeDelete (CclRBTreeHead_t* pRBTreeHead, 
                          ElementHandle_t* pCurrent);
/******************************************************************************/
static void
cclRBTreeLeftNodeDelete (CclRBTreeHead_t* pRBTreeHead, 
                         ElementHandle_t* pCurrent);
/******************************************************************************/
static ElementHandle_t*
cclRBTreeLastRightNodeGet (CclRBTreeHead_t* pRBTreeHead,
                           ElementHandle_t* pCurrent);
/******************************************************************************/
static ElementHandle_t*
cclRBTreeLastLeftNodeGet(CclRBTreeHead_t* pRBTreeHead,
                         ElementHandle_t* pCurrent);
/******************************************************************************/
static void cclRBTreeLeftRotate (ClCntHandleT*  pRBTreeHandle,
                                 ElementHandle_t*  pElement);
/******************************************************************************/
static void cclRBTreeRightRotate (ClCntHandleT*  pRBTreeHandle,
                                  ElementHandle_t*  pElement);
/******************************************************************************/
static void cclRBTreeInsertBalance (ClCntHandleT  root,
                                    ElementHandle_t*  pNewNode);
/******************************************************************************/
static void cclRBTreeDeleteBalance (ClCntHandleT  root,
                                    ElementHandle_t*  pNewNode,
                                    ElementHandle_t*  pParent);
/******************************************************************************/
static ClRcT
cclRBTreeBaseCntDestroy(CclRBTreeHead_t  *pRBTreeHead, 
                        ClCntHandleT     containerHandle);
/******************************************************************************/
#if HA_ENABLED
/* HA-aware container Modifications start : Hari*/
ClRcT
cclHARedBlackTreeCreate (ClCntKeyCompareCallbackT fpKeyCompare,
                         ClCntDeleteCallbackT      fpUserDeleteCallback,
                         ClCntDeleteCallbackT      fpUserDestroyCallback,
                         haInfo_t                 *pHaInfo,
                         ClCntHandleT     *pContainerHdl)
{
  ClRcT            retCode;
  CclRBTreeHead_t* pElement = NULL; 

  /*Create the container */
  if (CL_OK != (retCode = clCntRbtreeCreate(fpKeyCompare,
                                                   fpUserDeleteCallback,
                                                   fpUserDestroyCallback,
                                                   CL_CNT_NON_UNIQUE_KEY,
                                                   pContainerHdl)))
  {
     /* Some thing went wrong!!*/
     return retCode;
  }
  pElement = (CclRBTreeHead_t*)(*pContainerHdl);
  pElement->container.haEnabled = TRUE;
  pElement->container.cpsHdl = pHaInfo->cpsHdl;
  pElement->container.containerId = pHaInfo->containerId;
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
/******************************************************************************/
ClRcT
clCntThreadSafeRbtreeCreate(ClCntKeyCompareCallbackT fpKeyCompare,
                            ClCntDeleteCallbackT     fpUserDeleteCallback,
                            ClCntDeleteCallbackT     fpUserDestroyCallback,
				            ClCntKeyTypeT            containerKeyType,
                            ClCntHandleT             *pContainerHandle)
{
    ClRcT             errorCode = CL_OK;
    CclRBTreeHead_t  *pElement  = NULL; 

    errorCode = clCntRbtreeCreate(fpKeyCompare, fpUserDeleteCallback, 
                                  fpUserDestroyCallback, containerKeyType, 
                                  pContainerHandle);
    if( CL_OK != errorCode )
    {
        return errorCode;
    }
    pElement = (CclRBTreeHead_t *) *pContainerHandle;
    errorCode = clOsalMutexCreate(&pElement->treeMutex);
    if( CL_OK != errorCode )
    {
        clCntDelete(*pContainerHandle);
        returnCntError(errorCode, "Mutex Create failed");
    }

    return errorCode;
}

ClRcT
clCntRbtreeCreate(ClCntKeyCompareCallbackT fpKeyCompare,
                  ClCntDeleteCallbackT     fpUserDeleteCallback,
                  ClCntDeleteCallbackT     fpUserDestroyCallback,
				  ClCntKeyTypeT            containerKeyType,
                  ClCntHandleT             *pContainerHandle)
{
  CclRBTreeHead_t  *pElement = NULL; 
	
  /* Validate all the inputs against NULL pointer */
  nullChkRet(pContainerHandle);
  nullChkRet(fpKeyCompare);
  if(containerKeyType >= CL_CNT_INVALID_KEY_TYPE)
  {
      returnCntError(CL_ERR_INVALID_PARAMETER, 
                     "The container type is not proper");
  }

  /* Head of the red black tree to be created*/
  pElement = (CclRBTreeHead_t*) clHeapCalloc(1, (ClUint32T)sizeof(CclRBTreeHead_t));
  if( NULL == pElement )
  {
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }
  pElement->treeMutex = CL_HANDLE_INVALID_VALUE;
  pElement->pSentinel = (ElementHandle_t*) clHeapCalloc (1, (ClUint32T)sizeof(ElementHandle_t)); 
  if( NULL == pElement->pSentinel )
  {
      clHeapFree(pElement);
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }
  /* Populate the sentinal node */
  pElement->pSentinel->pRight  = pElement->pSentinel;
  pElement->pSentinel->pLeft   = pElement->pSentinel;
  pElement->pSentinel->pParent = NULL;
  pElement->pSentinel->color   = CCL_RBTREE_BLACK; 
  pElement->pSentinel->userKey = NULL;

  /* Populate Head of the Tree */
  //pElement->container.type       = (ClTypeT *) CL_CNT_RBTREE_HEAD;
  pElement->pFirstNode = pElement->pSentinel;
  /* Assign the corresponding functions to the container */
  pElement->container.fpFunctionContainerNodeAdd =
    cclRBTreeContainerNodeAdd;
  pElement->container.fpFunctionContainerKeyDelete =
    cclRBTreeContainerKeyDelete;
  pElement->container.fpFunctionContainerNodeDelete =
    cclRBTreeContainerNodeDelete;
  pElement->container.fpFunctionContainerDestroy =
    cclRBTreeContainerDestroy;
  pElement->container.fpFunctionContainerNodeFind =
    cclRBTreeContainerNodeFind;
  pElement->container.fpFunctionContainerFirstNodeGet =
    cclRBTreeContainerFirstNodeGet;
  pElement->container.fpFunctionContainerLastNodeGet =
    cclRBTreeContainerLastNodeGet;
  pElement->container.fpFunctionContainerNextNodeGet =
    cclRBTreeContainerNextNodeGet;
  pElement->container.fpFunctionContainerPreviousNodeGet =
    cclRBTreeContainerPreviousNodeGet;
  pElement->container.fpFunctionContainerSizeGet =
    cclRBTreeContainerSizeGet;
  pElement->container.fpFunctionContainerKeySizeGet =
    cclRBTreeContainerKeySizeGet;
  pElement->container.fpFunctionContainerUserKeyGet =
    cclRBTreeContainerUserKeyGet;
  pElement->container.fpFunctionContainerUserDataGet =
    cclRBTreeContainerUserDataGet;
  pElement->container.fpFunctionContainerKeyCompare =
    fpKeyCompare;
  pElement->container.fpFunctionContainerDeleteCallBack =
    fpUserDeleteCallback;
  pElement->container.fpFunctionContainerDestroyCallBack =
    fpUserDestroyCallback;

  if( CL_CNT_UNIQUE_KEY == containerKeyType )
  {
    pElement->container.isUniqueFlag = 1;
  }
  else
  {
    pElement->container.isUniqueFlag = 0;
  }
  pElement->container.validContainer = CONTAINER_ID;
  *pContainerHandle = (ClCntHandleT) pElement;

  return (CL_OK);
}

/*****************************************************************************/
static ClRcT
cclRBTreeContainerNodeAdd (ClCntHandleT      rbtHandle,
                           ClCntKeyHandleT   userKey,
                           ClCntDataHandleT  userData,
                           ClRuleExprT       *pExp)
{
  /* Pointer to the head of the RB tree */
  CclRBTreeHead_t  *pRBTreeHead = NULL;    
  /* This is for walking thru the tree */
  ElementHandle_t  *pTemp       = NULL;
  /* Pointer to fetch and create new node */
  ElementHandle_t  *pNewNode    = NULL;
  /* Pointer to parent of the new node */
  ElementHandle_t  *pParent     = NULL;
  ClCntHandleT     tempContainerHandle = CL_HANDLE_INVALID_VALUE;
  ClCntNodeHandleT tempNodeHandle  = CL_HANDLE_INVALID_VALUE;
  ClRcT            errorCode       = CL_OK;
  ClInt32T         keyCmpRet       = 0;

  /* Container handle validations */
  pRBTreeHead = (CclRBTreeHead_t*) rbtHandle;

  CL_CNT_LOCK(pRBTreeHead->treeMutex);
  pTemp   = (ElementHandle_t*) pRBTreeHead->pFirstNode;
  pParent = pTemp;
  /* Traverse through the tree until you find the correct node */
  while(pTemp != pRBTreeHead->pSentinel)
  {
      if( 0 == (keyCmpRet = pRBTreeHead->container.fpFunctionContainerKeyCompare(
                      pTemp->userKey, userKey)) )
      {
          /*duplicate userKey if the isUniqueFlag is true */
          if((pRBTreeHead->container.isUniqueFlag) != 0)
          {
              /* return error */
              CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
              returnCntError(CL_ERR_DUPLICATE, "Duplicate key found");
          }
          tempContainerHandle = pTemp->containerHandle;
          tempNodeHandle      = (ClCntNodeHandleT)pTemp;
          pRBTreeHead->size   = pRBTreeHead->size + 1;
          errorCode = cclBaseLinkedListContainerNodeAdd(tempContainerHandle, userKey,
                                                        userData, tempNodeHandle, pExp);
          CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
          return errorCode;
      }
      /* Store the parent node before traversing to the next node */
      pParent = pTemp;
      if( keyCmpRet < 0 )
      {
          /* go to right direction */
          pTemp = pTemp->pRight;
          continue;
      }
      /* go to left direction */
      pTemp = pTemp->pLeft;
  }
  /* Create new node */
  if( NULL == (pNewNode   = (ElementHandle_t*) clHeapCalloc(1, 
                                        (ClUint32T)sizeof(ElementHandle_t))) )
  {
      CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation is failed");
  }
  pNewNode->pLeft     = pRBTreeHead->pSentinel;
  pNewNode->pRight    = pRBTreeHead->pSentinel;
  pNewNode->pParent   = pParent;
  pNewNode->color     = CCL_RBTREE_RED;
  pNewNode->userKey   = userKey;
  //pNewNode->hListHead = rbtHandle;

  /* Create a list for this node */
  errorCode = cclBaseLinkedListCreate(&tempContainerHandle);
  pNewNode->containerHandle = tempContainerHandle;
  if(errorCode != CL_OK)
  {
    clHeapFree(pNewNode);
    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
    returnCntError(errorCode, "Base Link list create failed");
  }
  errorCode = cclBaseLinkedListContainerNodeAdd(tempContainerHandle, userKey,
                                                userData, (ClCntNodeHandleT) pNewNode,
                                                pExp);
  if( CL_OK != errorCode )
  {
      clHeapFree(pNewNode);
      CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
      returnCntError(errorCode, "Base list node add failed");
  }

  /*We got a parent node and new node, find a proper place here and add the node*/ 
  if(pParent == pRBTreeHead->pSentinel)
  {
    /* root node */
    pRBTreeHead->pFirstNode = pNewNode;
  }
  else 
  {
    if( keyCmpRet < 0 )
    {
      /* Parent's key is small, it will become as right NODE */
      pParent->pRight = pNewNode;
    }
    else 
    {
      /* Parent's key is large, it will become as lefr NODE */
      pParent->pLeft = pNewNode;
    }
  }
  /* Balance the tree */
  cclRBTreeInsertBalance(rbtHandle, pNewNode);
  pRBTreeHead->size = pRBTreeHead->size + 1;

  CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
  return (CL_OK);
}

static ClRcT
cclRBTreeBaseCntDelete(CclRBTreeHead_t  *pRBTreeHead, 
                       ClCntHandleT     containerHandle)
{
    BaseLinkedListNode_t *pBLlNode    = NULL;
    ClCntNodeHandleT     currentNode  = CL_HANDLE_INVALID_VALUE;
    ClUint32T            tempSize     = 0;
    ClRcT                errorCode    = CL_OK;
    ClUint32T            i            = 0;

	if(NULL != pRBTreeHead->container.fpFunctionContainerDeleteCallBack) 
    {
    	errorCode = clCntSizeGet(containerHandle,&tempSize);  
    	errorCode = clCntFirstNodeGet(containerHandle, &currentNode);
  
    	for(i=0;i<tempSize;i++)
        {
       		pBLlNode = (BaseLinkedListNode_t*)currentNode;
		    pRBTreeHead->container.fpFunctionContainerDeleteCallBack(pBLlNode->userKey, pBLlNode->userData);
    
       		errorCode = clCntNextNodeGet(containerHandle, currentNode, &currentNode);
       		if(errorCode != CL_OK)
         		break;
    	}
	}
    errorCode = clCntDelete(containerHandle);
    
    pRBTreeHead->size -= tempSize;

    return errorCode;
}

/*****************************************************************************/
static ClRcT
cclRBTreeContainerKeyDelete (ClCntHandleT     rbtHandle,
                             ClCntKeyHandleT  userKey)
{
    CclRBTreeHead_t  *pRBTreeHead = NULL; /* Pointer to the first node */
    ElementHandle_t  *pTemp       = NULL; /* Pointer to the node to be deleted */
    ElementHandle_t  *pLastNode   = NULL; /* Temporary pointer to the last node */
    ElementHandle_t  *pChild      = NULL; /* Temporary pointer to rt/left child */
    ElementHandle_t  *pParent     = NULL; /* pointer to the parent of the node being deleted*/
    ClInt32T         keyCmpRet    = 0;
    ClRcT            errorCode    = CL_OK;

    pRBTreeHead = (CclRBTreeHead_t*) rbtHandle;

    CL_CNT_LOCK(pRBTreeHead->treeMutex);
    for (pTemp = pRBTreeHead->pFirstNode; pTemp != pRBTreeHead->pSentinel; )
    {
        if( 0 == (keyCmpRet = pRBTreeHead->container.fpFunctionContainerKeyCompare(
                        pTemp->userKey, userKey)) ) 
        {
            /* Key found, return the node */
            break;
        }
        /* if userKey is greater than nodes userKey, then go to right child */
        /* else go to the left child  */
        if( keyCmpRet < 0 )
        {
            pTemp = pTemp->pRight;
            continue;
        }
        pTemp = pTemp->pLeft;
    }
    if(pTemp == pRBTreeHead->pSentinel)
    {
        /* Node not found. No such userKey exist */
        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        returnCntError(CL_ERR_NOT_EXIST, "No such user key exist");
    }

    /* The one and only node in the tree, so delete it */
    if((pTemp == pRBTreeHead->pFirstNode) && 
            (pTemp->pLeft == pRBTreeHead->pSentinel) &&
            (pTemp->pRight == pRBTreeHead->pSentinel))
    {
        pRBTreeHead->pFirstNode = pRBTreeHead->pSentinel;		

        cclRBTreeBaseCntDelete(pRBTreeHead, pTemp->containerHandle);
        clHeapFree(pTemp);
        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        return (CL_OK);
    }

    if (pTemp->pLeft == pRBTreeHead->pSentinel || pTemp->pRight == pRBTreeHead->pSentinel) 
    {
        /* pTemp has a pRBTreeHead->pSentinel node as a child */
        pChild = pTemp; 
    } 
    else 
    {
        /* find tree successor with a sentinel node as a child */
        pChild = pTemp->pRight;
        pChild = cclRBTreeLastLeftNodeGet (pRBTreeHead, pChild);
    }

    /* pLastNode is pChild's only child */
    if (pChild->pLeft != pRBTreeHead->pSentinel) 
    {
        pLastNode = pChild->pLeft;
    }
    else
    {
        pLastNode = pChild->pRight;
    }

    pParent = pChild->pParent;
    if(pLastNode != pRBTreeHead->pSentinel)
        pLastNode->pParent = pChild->pParent;

    if (pChild->pParent != pRBTreeHead->pSentinel) 
    {
        if (pChild == pChild->pParent->pLeft) {
            pChild->pParent->pLeft = pLastNode;
        }
        else {
            pChild->pParent->pRight = pLastNode;
        }
    }
    else /*make this as the parent node */
    {
        pRBTreeHead->pFirstNode = (ElementHandle_t*) pLastNode;
    }

    errorCode = cclRBTreeBaseCntDelete(pRBTreeHead, pTemp->containerHandle);
    if(errorCode != CL_OK)
    {
        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        return(errorCode);
    }

    if (pChild != pTemp) 
    {
        BaseLinkedListHead_t* pLlHead;
        BaseLinkedListNode_t* pLlNewNode;

        /* Copy the contents to the new parent */
        pTemp->userKey = pChild->userKey;
        pTemp->containerHandle = pChild->containerHandle;

        pLlHead = (BaseLinkedListHead_t*) pTemp->containerHandle;

        if (pLlHead == NULL)
        {
            CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
            returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
        }

        pLlNewNode = (BaseLinkedListNode_t*)pLlHead->pFirstNode;

        /* Bug#1799. When the node is deleted, the parent node of the base linked
         * list container should now point to the new parent. 
         */
        while(pLlNewNode != NULL)
        {
            pLlNewNode->parentNodeHandle = (ClCntNodeHandleT)pTemp;
            pLlNewNode = pLlNewNode->pNextNode;
        }
    }
    if(pChild->color == CCL_RBTREE_BLACK)	
    {
        cclRBTreeDeleteBalance(rbtHandle, pLastNode, pParent);
    }

    clHeapFree(pChild); 
    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
    return (CL_OK);
}

/******************************************************************************/
static ClRcT
cclRBTreeContainerDestroy (ClCntHandleT  rbtHandle)
{
    CclRBTreeHead_t       *pRBTreeHead = NULL; 
    ElementHandle_t       *pTemp       = NULL; 
    ElementHandle_t       *pRightChild = NULL; 
    ElementHandle_t       *pLeftChild  = NULL;

    pRBTreeHead = (CclRBTreeHead_t*) rbtHandle;

    CL_CNT_LOCK(pRBTreeHead->treeMutex);
    pTemp       = (ElementHandle_t*) pRBTreeHead->pFirstNode;
    pLeftChild  = (ElementHandle_t*) cclRBTreeLastLeftNodeGet (pRBTreeHead,pTemp);
    pRightChild = (ElementHandle_t*) cclRBTreeLastRightNodeGet (pRBTreeHead,pTemp);

    while(pRBTreeHead->pFirstNode != pRBTreeHead->pSentinel)
    {
        if((pLeftChild != pRBTreeHead->pSentinel) && 
           (pLeftChild != ((ElementHandle_t*) pRBTreeHead->pFirstNode))) 
        {
            cclRBTreeRightNodeDelete (pRBTreeHead, pLeftChild);
            pLeftChild = pLeftChild->pParent;

            cclRBTreeBaseCntDestroy(pRBTreeHead,
                    pLeftChild->pLeft->containerHandle);

            if(pLeftChild->pLeft != pRBTreeHead->pSentinel)
            {
                clHeapFree(pLeftChild->pLeft);
            }
        }
        else
        {
            if((pRightChild != pRBTreeHead->pSentinel) && 
               (pRightChild != ((ElementHandle_t*) pRBTreeHead->pFirstNode))) 
            {
                cclRBTreeLeftNodeDelete (pRBTreeHead, pRightChild);
                pRightChild = pRightChild->pParent;

                cclRBTreeBaseCntDestroy(pRBTreeHead,
                                        pRightChild->pRight->containerHandle);
                if(pRightChild->pRight != pRBTreeHead->pSentinel)
                {
                    clHeapFree(pRightChild->pRight);
                }
            }
            else
            {
                break;
            }
        }	
    }

    if( pRBTreeHead->pFirstNode->containerHandle != CL_HANDLE_INVALID_VALUE )
    {
        cclRBTreeBaseCntDestroy(pRBTreeHead, pRBTreeHead->pFirstNode->containerHandle);
    }

    pRBTreeHead->container.validContainer = 0;
    if(pRBTreeHead->pFirstNode != pRBTreeHead->pSentinel)
    {
        clHeapFree(pRBTreeHead->pFirstNode);
    }
    clHeapFree(pRBTreeHead->pSentinel);
    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
    CL_CNT_DESTROY(pRBTreeHead->treeMutex);
    clHeapFree(pRBTreeHead);
    return (CL_OK);
}

static ClRcT
cclRBTreeBaseCntDestroy(CclRBTreeHead_t  *pRBTreeHead,
                        ClCntHandleT     containerHandle)
{
    ClRcT                errorCode   = CL_OK;
    ClUint32T            i           = 0;
    ClUint32T            tempSize    = 0;
    ClCntNodeHandleT     currentNode = CL_HANDLE_INVALID_VALUE;
    BaseLinkedListNode_t *pBLlNode   = NULL;

    if(NULL != pRBTreeHead->container.fpFunctionContainerDestroyCallBack) 
    {
        errorCode = clCntSizeGet(containerHandle,&tempSize);    
        errorCode = clCntFirstNodeGet(containerHandle, &currentNode);    
        if(errorCode == CL_OK)
        {
            for(i=0; i<tempSize; i++)
            {
                pBLlNode = (BaseLinkedListNode_t*)currentNode;  
                pRBTreeHead->container.fpFunctionContainerDestroyCallBack(pBLlNode->userKey, pBLlNode->userData);    
                errorCode = clCntNextNodeGet(containerHandle, currentNode, &currentNode);
                if(errorCode != CL_OK) 
                {
                    if( i != (tempSize - 1) )
                    {
                       clDbgCodeError(CL_CNT_RC(CL_ERR_NOT_EXIST), 
                        ("Container size doesn't not match with number of container nodes"));
                       return CL_CNT_RC(CL_ERR_NOT_EXIST);
                    }
                    break;
                }
            }
        }
    }
    errorCode = clCntDelete(containerHandle);

    return errorCode;
}

/******************************************************************************/
static void
cclRBTreeRightNodeDelete(CclRBTreeHead_t* pRBTreeHead,
                         ElementHandle_t* pCurrent)
{

  ClCntHandleT         tempContainerHandle = CL_HANDLE_INVALID_VALUE;

  ElementHandle_t* pRightChild = (ElementHandle_t*)pCurrent;
  pRightChild = (ElementHandle_t*) cclRBTreeLastRightNodeGet (pRBTreeHead,pCurrent);
  	
  while((pRightChild != pRBTreeHead->pSentinel) && (pRightChild != pCurrent))
  {
      if(pRightChild->pRight != pRBTreeHead->pSentinel) 
      {
          cclRBTreeLeftNodeDelete (pRBTreeHead, pRightChild);
          pRightChild = pRightChild->pParent;

          cclRBTreeBaseCntDestroy(pRBTreeHead,
                  pRightChild->pRight->containerHandle);

          if(pRightChild->pRight != pRBTreeHead->pSentinel)
          {
              clHeapFree(pRightChild->pRight);
          }
      }
      else 
      {
          cclRBTreeLeftNodeDelete (pRBTreeHead, pRightChild);

          pRightChild = pRightChild->pParent;

          tempContainerHandle = pRightChild->pRight->containerHandle;
          cclRBTreeBaseCntDestroy(pRBTreeHead,
                  pRightChild->pRight->containerHandle);

          if(pRightChild->pRight != pRBTreeHead->pSentinel)
          {
              clHeapFree(pRightChild->pRight);
          }
      }
  }
  return ;
}

/******************************************************************************/
static void
cclRBTreeLeftNodeDelete(CclRBTreeHead_t* pRBTreeHead,
                        ElementHandle_t* pCurrent)
{
  ElementHandle_t* pLeftChild = (ElementHandle_t*)pCurrent;
  pLeftChild = (ElementHandle_t*) cclRBTreeLastLeftNodeGet (pRBTreeHead,pCurrent);  

  while((pLeftChild != pRBTreeHead->pSentinel) && (pLeftChild != pCurrent)) 
  {
    if(pLeftChild->pRight != pRBTreeHead->pSentinel)	
    {
      cclRBTreeRightNodeDelete (pRBTreeHead, pLeftChild);
      pLeftChild = pLeftChild->pParent;
    
      cclRBTreeBaseCntDestroy(pRBTreeHead, 
                              pLeftChild->pLeft->containerHandle);
      if(pLeftChild->pLeft != pRBTreeHead->pSentinel)
      {
        clHeapFree(pLeftChild->pLeft);
      }
    }
    else
    {
      cclRBTreeRightNodeDelete (pRBTreeHead, pLeftChild);
      pLeftChild = pLeftChild->pParent;
      
      cclRBTreeBaseCntDestroy(pRBTreeHead,
                              pLeftChild->pLeft->containerHandle);

      if(pLeftChild->pLeft != pRBTreeHead->pSentinel)
      {
        clHeapFree(pLeftChild->pLeft);
      }
    }
  }
  return;
}

/******************************************************************************/
static ElementHandle_t*
cclRBTreeLastLeftNodeGet(CclRBTreeHead_t* pRBTreeHead,
                         ElementHandle_t* pCurrent)
{
  ElementHandle_t* pLeftChild = (ElementHandle_t*) pCurrent;

  while(pLeftChild != pRBTreeHead->pSentinel)
  {
    if(pLeftChild->pLeft == pRBTreeHead->pSentinel)
    {
        break;
    }
    pLeftChild = pLeftChild->pLeft;
  }
  return pLeftChild;

}

/******************************************************************************/
static ElementHandle_t*
cclRBTreeLastRightNodeGet(CclRBTreeHead_t* pRBTreeHead,
                          ElementHandle_t* pCurrent)
{
  ElementHandle_t* pRightChild = (ElementHandle_t*) pCurrent;

  while(pRightChild != pRBTreeHead->pSentinel)
  {
    if(pRightChild->pRight == pRBTreeHead->pSentinel) 
    {
        break;
    }
    pRightChild = pRightChild->pRight;
  }
  return pRightChild;

}

/******************************************************************************/
static ClRcT
cclRBTreeContainerSizeGet (ClCntHandleT  rbtHandle,
                           ClUint32T* pSize)
{
  CclRBTreeHead_t* pRBTreeHead;
  
  pRBTreeHead = ((CclRBTreeHead_t*)rbtHandle);

  *pSize = (ClUint32T)(((CclRBTreeHead_t *)pRBTreeHead)->size);
  return (CL_OK);

}

/******************************************************************************/
static ClRcT
cclRBTreeContainerNodeFind (ClCntHandleT  rbtHandle,
                            ClCntKeyHandleT  userKey,
                            ClCntNodeHandleT* pNodeHandle)
{
  ElementHandle_t* pTemp = NULL; /* Temporary pointer to node being searched */
  CclRBTreeHead_t* pRBTreeHead = ((CclRBTreeHead_t*)rbtHandle);
  ClCntNodeHandleT tempNodeHandle;
  ClRcT errorCode;

  *pNodeHandle = CL_HANDLE_INVALID_VALUE;
  CL_CNT_LOCK( ((CclRBTreeHead_t *) rbtHandle)->treeMutex);
  if(((CclRBTreeHead_t*)rbtHandle)->pFirstNode == ((CclRBTreeHead_t*)rbtHandle)->pSentinel) 
  {
    /* Empty tree, add debug statements here */
    CL_CNT_UNLOCK( ((CclRBTreeHead_t *) rbtHandle)->treeMutex);
    returnCntError(CL_ERR_NOT_EXIST, "Node doesn't exist");
  }

  for(pTemp = pRBTreeHead->pFirstNode; pTemp != ((CclRBTreeHead_t*)rbtHandle)->pSentinel; )
  {
    if(pRBTreeHead->container.fpFunctionContainerKeyCompare(pTemp->userKey, userKey) == 0)	
    {
      /* Key found, return the node */
      errorCode = clCntFirstNodeGet(pTemp->containerHandle,&tempNodeHandle);
      if(errorCode == CL_OK)
      {
        *pNodeHandle = tempNodeHandle;
        CL_CNT_UNLOCK( ((CclRBTreeHead_t *) rbtHandle)->treeMutex);
        return (CL_OK);
      }
      else 
      {
        *pNodeHandle = (ClCntNodeHandleT)NULL;
        CL_CNT_UNLOCK( ((CclRBTreeHead_t *) rbtHandle)->treeMutex);
        return (errorCode);
      }
    }

    /* if userKey is greater than nodes userKey, then go to right child */
    /* else go to the left child                                */
    if(pRBTreeHead->container.fpFunctionContainerKeyCompare(pTemp->userKey, userKey) < 0) {
      pTemp = pTemp->pRight;
    }
    else {
      pTemp = pTemp->pLeft;
    }
  }

  CL_CNT_UNLOCK( ((CclRBTreeHead_t *) rbtHandle)->treeMutex);
  errorCode = CL_RC(CL_CID_CNT, CL_ERR_NOT_EXIST);
  return (errorCode);

}

/******************************************************************************/
static ClRcT
cclRBTreeContainerFirstNodeGet (ClCntHandleT  rbtHandle,
                                ClCntNodeHandleT* pNodeHandle)
{
  CclRBTreeHead_t  *pTemp = NULL;
  ElementHandle_t  *pNode = NULL;
  ClCntNodeHandleT tempNodeHandle;
  ClRcT            errorCode = CL_ERR_NOT_EXIST;

  pTemp = (CclRBTreeHead_t*)rbtHandle;

  CL_CNT_LOCK(pTemp->treeMutex);
  *pNodeHandle = CL_HANDLE_INVALID_VALUE;
  pNode = (ElementHandle_t*) cclRBTreeLastLeftNodeGet (pTemp,pTemp->pFirstNode);
  if(pNode != pTemp->pSentinel)
  {
    errorCode = clCntFirstNodeGet(pNode->containerHandle,&tempNodeHandle);
    if(errorCode == CL_OK) 
    {
      *pNodeHandle = tempNodeHandle;
      CL_CNT_UNLOCK(pTemp->treeMutex);
      return (CL_OK);
    }
  }
  CL_CNT_UNLOCK(pTemp->treeMutex);
  returnCntError(errorCode, "Node doesn't exist");
}

/******************************************************************************/
static ClRcT
cclRBTreeContainerLastNodeGet (ClCntHandleT  rbtHandle,
                               ClCntNodeHandleT* pNodeHandle)
{
  CclRBTreeHead_t  *pTemp         = NULL;
  ElementHandle_t  *pNode         = NULL;
  ClCntNodeHandleT tempNodeHandle = CL_HANDLE_INVALID_VALUE;
  ClRcT            errorCode      = CL_ERR_NOT_EXIST;

  pTemp = (CclRBTreeHead_t*)rbtHandle;

  CL_CNT_LOCK(pTemp->treeMutex);
  pNode = (ElementHandle_t*) cclRBTreeLastRightNodeGet (pTemp,pTemp->pFirstNode);
	
  if(pNode != pTemp->pSentinel)
  {
    errorCode = clCntLastNodeGet(pNode->containerHandle,&tempNodeHandle);
    if(errorCode == CL_OK)
    {
      *pNodeHandle = tempNodeHandle;
      CL_CNT_UNLOCK(pTemp->treeMutex);
      return (CL_OK);
    }
  }
  CL_CNT_UNLOCK(pTemp->treeMutex);
  returnCntError(errorCode, "Node does not exist");
}

/******************************************************************************/
static ClRcT
cclRBTreeContainerNextNodeGet (ClCntHandleT  rbtHandle,
                               ClCntNodeHandleT  rbtElement,
                               ClCntNodeHandleT* pNextRBTElement)
{
    ElementHandle_t* pParent = NULL;
    ElementHandle_t* pNext = NULL;
    CclRBTreeHead_t* pRBTreeHead;	
    ElementHandle_t* pTemp;
    BaseLinkedListNode_t *pBLlNode;
    ClCntNodeHandleT tempNodeHandle;
    ClRcT errorCode;

    pRBTreeHead      = (CclRBTreeHead_t*)rbtHandle;
    *pNextRBTElement = CL_HANDLE_INVALID_VALUE;

    pBLlNode = (BaseLinkedListNode_t *)rbtElement;
    nullChkRet(pBLlNode);
    if( NODE_ID != pBLlNode->validNode )
    {
        returnCntError(CL_ERR_INVALID_HANDLE,
                "Passed handle is invalid");
    }

    CL_CNT_LOCK(pRBTreeHead->treeMutex);
    pTemp = (ElementHandle_t *)pBLlNode->parentNodeHandle;
    errorCode = clCntNextNodeGet(pTemp->containerHandle,rbtElement,&tempNodeHandle);

    if(errorCode == CL_OK)
    {
        *pNextRBTElement = tempNodeHandle;
        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        return (CL_OK);
    }
    else
    {
        /* For root node , return the left child */ 
        if(pTemp == (ElementHandle_t*)pRBTreeHead->pFirstNode) 
        {
            if(pTemp->pRight == pRBTreeHead->pSentinel) 
            {
                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
            }
        }
        /* If a node has left child return it */
        if(pTemp->pRight != pRBTreeHead->pSentinel) 
        {
            /* SHould return left end node */
            if(pTemp->pRight->pLeft == pRBTreeHead->pSentinel)
            {
                errorCode = clCntFirstNodeGet(pTemp->pRight->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK)
                {
                    *pNextRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else 
                {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    returnCntError(errorCode, "Node does not exist");
                }
            }
            else
            {
                pNext = pTemp->pRight->pLeft;
                pNext = cclRBTreeLastLeftNodeGet (pRBTreeHead,pNext);
                errorCode = clCntFirstNodeGet(pNext->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK)
                {
                    *pNextRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else 
                {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    returnCntError(errorCode, "Node does not exist");
                }
            }
        }

        if(pTemp->pParent->pLeft != pRBTreeHead->pSentinel) 
        {
            /* If the node is the right child return parent */
            if(pTemp->pParent->pLeft == pTemp) {
                errorCode = clCntFirstNodeGet(pTemp->pParent->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK) {
                    *pNextRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else 
                {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    returnCntError(errorCode, "Node does not exist");
                }
            }
        }

        if(pTemp->pParent->pRight != pRBTreeHead->pSentinel) {
            if(pTemp->pParent->pRight == pTemp) {
                /* Right child */
                pParent = (ElementHandle_t*)pTemp->pParent;

                while(pParent != pRBTreeHead->pSentinel) {
                    if(pParent->pParent == pRBTreeHead->pSentinel)
                    {
                        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                        returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
                    }
                    if(pParent->pParent->pLeft != pRBTreeHead->pSentinel) {
                        if(pParent->pParent->pLeft == pParent) {
                            errorCode = clCntFirstNodeGet(pParent->pParent->containerHandle,&tempNodeHandle);
                            if(errorCode == CL_OK)
                            {
                                *pNextRBTElement = tempNodeHandle;
                                /* Right child hence return Parent*/
                                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                                return (CL_OK);
                            }
                            else
                            {
                                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                                returnCntError(errorCode, "Node does not exist");
                            }
                        }
                    }
                    if(pParent->pParent->pRight != pRBTreeHead->pSentinel)  {
                        if(pParent->pParent->pRight == pParent) {
                            /* Left child, hence move up */
                            pParent = pParent->pParent;
                        }
                    }
                }
            }
        }

        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
    }
}

/******************************************************************************/
static ClRcT
cclRBTreeContainerPreviousNodeGet (ClCntHandleT  rbtHandle,
                                   ClCntNodeHandleT  rbtElement,
                                   ClCntNodeHandleT* pPreviousRBTElement)
{
    ElementHandle_t* pParentNode = NULL;
    CclRBTreeHead_t* pRBTreeHead;	
    ElementHandle_t* pTemp;
    ElementHandle_t* pNext;
    BaseLinkedListNode_t *pBLlNode;
    ClCntNodeHandleT tempNodeHandle;
    ClRcT errorCode;

    pRBTreeHead = (CclRBTreeHead_t*)rbtHandle;

    pBLlNode = (BaseLinkedListNode_t *)rbtElement;
    nullChkRet(pBLlNode);
    if( NODE_ID != pBLlNode->validNode )
    {
        returnCntError(CL_ERR_INVALID_HANDLE, 
                "Passed handle is invalid");
    }

    CL_CNT_LOCK(pRBTreeHead->treeMutex);
    pTemp = (ElementHandle_t *)pBLlNode->parentNodeHandle; 
    errorCode = clCntPreviousNodeGet(pTemp->containerHandle,rbtElement,&tempNodeHandle);

    if(errorCode == CL_OK) 
    {
        *pPreviousRBTElement = tempNodeHandle;
        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        return (CL_OK);
    }
    else {
        /* For root node , return the left child */ 
        if(pTemp == (ElementHandle_t*)pRBTreeHead->pFirstNode) {
            if(pTemp->pLeft == pRBTreeHead->pSentinel)
            {
                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
            }
        }

        /* If a node has left child return it */
        if(pTemp->pLeft != pRBTreeHead->pSentinel) {
            /* Should return right most node */
            if(pTemp->pLeft->pRight == pRBTreeHead->pSentinel) {
                errorCode = clCntLastNodeGet(pTemp->pLeft->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK)
                {
                    *pPreviousRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else
                {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (errorCode);
                }
            }
            else {
                pNext = pTemp->pLeft->pRight;
                pNext = cclRBTreeLastRightNodeGet (pRBTreeHead,pNext);
                errorCode = clCntLastNodeGet(pNext->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK) 
                {
                    *pPreviousRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (errorCode);
                }
            }
        }

        if(pTemp->pParent->pRight != pRBTreeHead->pSentinel) {
            /* If the node is the right child return parent */
            if(pTemp->pParent->pRight == pTemp) {
                errorCode = clCntLastNodeGet(pTemp->pParent->containerHandle,&tempNodeHandle);
                if(errorCode == CL_OK) 
                {
                    *pPreviousRBTElement = tempNodeHandle;
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (CL_OK);
                }
                else
                {
                    CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                    return (errorCode);
                }
            }
        }

        if(pTemp->pParent->pLeft != pRBTreeHead->pSentinel) {
            if(pTemp->pParent->pLeft == pTemp) {
                pParentNode = (ElementHandle_t*)pTemp->pParent;
                while(pParentNode != pRBTreeHead->pSentinel)	{
                    /* First right node, prev will be parent 
                     * if no left child exist */
                    if(pParentNode->pParent == pRBTreeHead->pSentinel)	{
                        if(pParentNode->pParent->pRight == pRBTreeHead->pSentinel) 
                        {
                            CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                            returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
                        }
                        errorCode = clCntLastNodeGet(pParentNode->containerHandle,&tempNodeHandle);
                        if(errorCode == CL_OK) 
                        {
                            *pPreviousRBTElement = tempNodeHandle;
                            CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                            return (CL_OK);
                        }
                        else {
                            CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                            return (errorCode);
                        }
                    }

                    if(pParentNode->pParent->pRight != pRBTreeHead->pSentinel) {
                        if(pParentNode->pParent->pRight == pParentNode) {
                            errorCode = clCntLastNodeGet(pParentNode->pParent->containerHandle,&tempNodeHandle);
                            if(errorCode == CL_OK) {
                                *pPreviousRBTElement = tempNodeHandle;
                                /* Right child hence return Parent*/
                                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                                return (CL_OK);
                            }
                            else {
                                CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
                                return (errorCode);
                            }
                        } 
                    } 
                    if(pParentNode->pParent->pLeft != pRBTreeHead->pSentinel) {
                        if(pParentNode->pParent->pLeft == pParentNode) {
                            /* Left child, hence move up */
                            pParentNode = pParentNode->pParent;
                        }
                    } 
                }
            }
        }

        CL_CNT_UNLOCK(pRBTreeHead->treeMutex);
        returnCntError(CL_ERR_NOT_EXIST, "Node does not exist");
    }
}

/************************************************************************/
/*									*/
/*				Left Rotate				*/
/*             pTemp      <<<<<<-------------    pElement               */
/*              / \                                  /  \		*/
/*             /   \                                /    \		*/
/*            /     \                              /      \		*/
/*      pElement   C                             A     pTemp		*/
/*          / \                                          /  \		*/
/*         /   \                                        /    \		*/
/*        A     B                                      B      C		*/
/*									*/
/*									*/
/************************************************************************/

static void
cclRBTreeLeftRotate (ClCntHandleT    *pRBTreeHandle,
                     ElementHandle_t *pElement)
{
  /* This pointer will become parent after rotation */
  ElementHandle_t* pTemp = NULL; 
  ElementHandle_t* pRoot = (ElementHandle_t*)((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode;

  pTemp = pElement->pRight;

  /* B is now pElement's right child */

  pElement->pRight = pTemp->pLeft;

  /*If B is not null assign pElement as B's parent node */
  if(pTemp->pLeft != ((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel)	{
    pTemp->pLeft->pParent = pElement;
  }

  if(pTemp != ((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel) {
    pTemp->pParent = pElement->pParent;
    if(pElement == pRoot) {
      ((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode = (ElementHandle_t *)pTemp;
    }
  }

  /* Set the parent pointers */
  if(pElement->pParent == ((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel) {
    ((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode = (ElementHandle_t *)pTemp;
  }
  else {
    if(pElement == (pElement->pParent)->pLeft) {
      pElement->pParent->pLeft = pTemp;
    }
    else {
      pElement->pParent->pRight = pTemp;
    }
  }

  /* Last set of rotation,make pTemp the parent of pElement */
  pTemp->pLeft = pElement;
  pElement->pParent = pTemp;
  return;

}

/************************************************************************/
/*								        */
/*				Right Rotate				*/
/*           pElement      ------------->>>>>>>    pTemp                */
/*              / \                                  /  \		*/
/*             /   \                                /    \		*/
/*            /     \                              /      \		*/
/*        pTemp     C                             A    pElement	        */
/*          / \                                          /  \		*/
/*         /   \                                        /    \		*/
/*        A     B                                      B      C		*/
/*									*/
/*									*/
/************************************************************************/

static void 
cclRBTreeRightRotate (ClCntHandleT* pRBTreeHandle,
                      ElementHandle_t* pElement)
{
  /* Pointer to the root of the red black tree */
  ElementHandle_t* pRoot = (ElementHandle_t*)((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode;
  /* This pointer will become parent after rotation */
  ElementHandle_t* pTemp = NULL; 

  pTemp = pElement->pLeft;
  pElement->pLeft = pTemp->pRight;

  /*If B is not null assign pElement as B's parent node */
  if(pTemp->pRight !=((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel){
    pTemp->pRight->pParent = pElement;
  }

  if(pTemp != ((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel) {
    pTemp->pParent = pElement->pParent;
    if(pElement == pRoot) {
      ((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode = (ElementHandle_t*)pTemp;
    }
  }

  /* Set the parent pointers */
  if(pElement->pParent == ((CclRBTreeHead_t*)pRBTreeHandle)->pSentinel) {
    ((CclRBTreeHead_t*)pRBTreeHandle)->pFirstNode = (ElementHandle_t*)pTemp;
  }
  else {
    if(pElement == (pElement->pParent)->pLeft) {
      pElement->pParent->pLeft = pTemp;
    }
    else {
      pElement->pParent->pRight = pTemp;
    }
  }

  /* Last setp of rotation,make pTemp the parent of pElement */
  pTemp->pRight = pElement;
  pElement->pParent = pTemp;
  return;

}

/******************************************************************************/
static void 
cclRBTreeInsertBalance (ClCntHandleT    root,
                        ElementHandle_t *pNewNode) 
{
  ElementHandle_t  *pUncleNode = NULL; /* Pointer to uncle node */
  ElementHandle_t  *pFirstNode = NULL;
  CclRBTreeHead_t  *pRBroot    = NULL;

  pRBroot = (CclRBTreeHead_t *) root;
  
  if( pNewNode == pRBroot->pSentinel )
  {
      /* nothing to do with, silently leaving */
      return ;
  }
  
  pFirstNode = pRBroot->pFirstNode;
  /* Verify the properties of red black trees */
  while( (pNewNode != pFirstNode) && (pNewNode->pParent->color == CCL_RBTREE_RED) )
  {
     /* both parent and child are RED colors, Chk the parent of new node is left child */
    if (pNewNode->pParent == pNewNode->pParent->pParent->pLeft)
    {
      /* Get Uncle node*/
      pUncleNode = pNewNode->pParent->pParent->pRight;
      if (pUncleNode->color == CCL_RBTREE_RED)
      {
        /* Flip the color of uncle,parent and grant parent */
        pNewNode->pParent->color          = CCL_RBTREE_BLACK;
        pNewNode->pParent->pParent->color = CCL_RBTREE_RED;
        pUncleNode->color                 = CCL_RBTREE_BLACK;
        pNewNode                          = pNewNode->pParent->pParent;
      } 
      else  /* Uncle node is black */
      {
        /* If the node is the right child */
	    if (pNewNode == pNewNode->pParent->pRight)
        {
          pNewNode = pNewNode->pParent;
          /* Left rotate the node */
          cclRBTreeLeftRotate ((ClCntHandleT*)root, pNewNode);
        }
        /* Toggle colors for parents and rotate */
        pNewNode->pParent->color          = CCL_RBTREE_BLACK;
        pNewNode->pParent->pParent->color = CCL_RBTREE_RED;
        cclRBTreeRightRotate ((ClCntHandleT*)root, pNewNode->pParent->pParent);
      }
    } 
    else /* If the parent of the new node is right child */
    {
      pUncleNode = pNewNode->pParent->pParent->pLeft;
      if (pUncleNode->color == CCL_RBTREE_RED) 
      {
        /* Uncle node is red */
        pNewNode->pParent->color = CCL_RBTREE_BLACK;
        pUncleNode->color        = CCL_RBTREE_BLACK;
        pNewNode->pParent->pParent->color = CCL_RBTREE_RED;
        pNewNode = pNewNode->pParent->pParent;
      }
      else	/* Uncle node is black */ 
      {
        /* If the node is the left child */
        if (pNewNode == pNewNode->pParent->pLeft)
        {
          pNewNode = pNewNode->pParent;
          cclRBTreeRightRotate ((ClCntHandleT*)root, pNewNode);
        }
        /* Toggle colors for parents and left rotate */
        pNewNode->pParent->color = CCL_RBTREE_BLACK;
        pNewNode->pParent->pParent->color = CCL_RBTREE_RED;
        cclRBTreeLeftRotate ((ClCntHandleT*)root, (ElementHandle_t*)(pNewNode->pParent->pParent));
      }
    }
  }

  pRBroot->pFirstNode->color =  CCL_RBTREE_BLACK;
  return;

}

/******************************************************************************/
static void cclRBTreeDeleteBalance (ClCntHandleT  root,
                                    ElementHandle_t* pNewNode,
                                    ElementHandle_t *pParent) 
{
    ElementHandle_t* pUncleNode = NULL; /* Pointer to uncle node */
    ElementHandle_t* pFirstNode = NULL;
    ElementHandle_t *pSentinel = NULL;
  
    pSentinel = ((CclRBTreeHead_t*)root)->pSentinel;
    if(!pParent || pParent == pSentinel)
        goto out;

    pFirstNode = (ElementHandle_t*)(((CclRBTreeHead_t*)root)->pFirstNode);
  
    /* Verify the properties of red black trees */
    while (
           (pNewNode == pSentinel
            ||
            pNewNode->color == CCL_RBTREE_BLACK)
           &&
           (pNewNode != pFirstNode))
    {
        /* If the new node is left child */
        if (pNewNode == pParent->pLeft) {
            pUncleNode = (ElementHandle_t*)pParent->pRight;
            /* Uncle node is red */
            if (pUncleNode->color == CCL_RBTREE_RED) {
                pUncleNode->color = CCL_RBTREE_BLACK;
                pParent->color = CCL_RBTREE_RED;
                cclRBTreeLeftRotate ((ClCntHandleT*)root, pParent);
                pUncleNode = pParent->pRight;
            }

            /* check if the left and right child are black */
            if(pUncleNode->pLeft->color == CCL_RBTREE_BLACK &&
               pUncleNode->pRight->color == CCL_RBTREE_BLACK) {

                pUncleNode->color = CCL_RBTREE_RED;
                pNewNode = pParent;
                pParent = pNewNode->pParent;
            } 
            else {
                /* right node is black */
                if(pUncleNode->pRight->color == CCL_RBTREE_BLACK) {
                    pUncleNode->pLeft->color = CCL_RBTREE_BLACK;
                    pUncleNode->color = CCL_RBTREE_RED;
                    /* right rotate pUncleNode */
                    cclRBTreeRightRotate ((ClCntHandleT*)root, pUncleNode);
                    pUncleNode = pParent->pRight;
                }
	
                pUncleNode->color = pParent->color;
                pParent->color = CCL_RBTREE_BLACK;
                pUncleNode->pRight->color = CCL_RBTREE_BLACK;

                /*left rotate the parent node */
                cclRBTreeLeftRotate ((ClCntHandleT*)root, pParent);
                pNewNode = (ElementHandle_t*)pFirstNode;
            }
        }
        else {
            pUncleNode = (ElementHandle_t*)pParent->pLeft;
            if (pUncleNode->color == CCL_RBTREE_RED) {
                pUncleNode->color = CCL_RBTREE_BLACK;
                pParent->color = CCL_RBTREE_RED;
                cclRBTreeRightRotate ((ClCntHandleT*)root, pParent);
                pUncleNode = pParent->pLeft;
            }

            /* check if the left and right child are black */
            if(pUncleNode->pLeft->color == CCL_RBTREE_BLACK &&
               pUncleNode->pRight->color == CCL_RBTREE_BLACK)	{

                pUncleNode->color = CCL_RBTREE_RED;
                pNewNode = pParent;
                pParent = pNewNode->pParent;
            } 
            else {
                /* left node is black */
                if(pUncleNode->pLeft->color == CCL_RBTREE_BLACK)	{
                    pUncleNode->pRight->color = CCL_RBTREE_BLACK;
                    pUncleNode->color = CCL_RBTREE_RED;
                    /* left rotate the node */
                    cclRBTreeLeftRotate ((ClCntHandleT*)root, pUncleNode);
                    pUncleNode = pParent->pLeft;
                }
                pUncleNode->color = pParent->color;
                pParent->color = CCL_RBTREE_BLACK;
                pUncleNode->pLeft->color = CCL_RBTREE_BLACK;

                /* right rotate the parent */
                cclRBTreeRightRotate((ClCntHandleT*)root, pParent);
                pNewNode = (ElementHandle_t*)pFirstNode;
            }
        }
    }

    out:
    if(pNewNode != pSentinel)
        pNewNode->color = CCL_RBTREE_BLACK;

    return;

}

/*****************************************************************************/
static ClRcT
cclRBTreeContainerNodeDelete (ClCntHandleT  rbtHandle, 
                              ClCntNodeHandleT nodeHandle)
{
  BaseLinkedListNode_t* pBLlNode;
  ElementHandle_t *pRBTNode;
  CclRBTreeHead_t* pRBTHead;
  ClCntNodeHandleT tempNodeHandle;
  ClCntHandleT tempContainerHandle;
  ClCntKeyHandleT rbtKey = 0;
  ClCntDataHandleT rbtData = 0;
  ClRcT errorCode;

  pRBTHead = (CclRBTreeHead_t*)rbtHandle;
 
  pBLlNode = (BaseLinkedListNode_t *)nodeHandle;
  nullChkRet(pBLlNode);
  if( NODE_ID != pBLlNode->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "Passed node handle is invalid");
  }
  CL_CNT_LOCK(pRBTHead->treeMutex);
  pRBTNode = (ElementHandle_t*)pBLlNode->parentNodeHandle;
#if 0
  if( pRBTNode->hListHead != rbtHandle )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "This node handle doesn't belong to this container");
  }
#endif
  tempContainerHandle = (ClCntHandleT)pRBTNode->containerHandle;  
  
  rbtKey = pRBTNode->userKey;
  rbtData =pBLlNode->userData;

  errorCode = clCntNodeDelete(tempContainerHandle, nodeHandle);
  
  if(errorCode == CL_OK) {
    /*check if the base linked list container is empty*/
    if(clCntFirstNodeGet(tempContainerHandle, &tempNodeHandle) != CL_OK) 
    {
      /* delete the tree node also */
        errorCode = clCntAllNodesForKeyDelete(rbtHandle,rbtKey);
        if(errorCode != CL_OK)
        {
            CL_CNT_UNLOCK(pRBTHead->treeMutex);
            return (errorCode);
        }
    }
    else
    {
        /*update the key*/
        pRBTNode->userKey = ((BaseLinkedListNode_t*)tempNodeHandle)->userKey;
    }

    if(NULL != pRBTHead->container.fpFunctionContainerDeleteCallBack) 
    {
        pRBTHead->container.fpFunctionContainerDeleteCallBack(rbtKey,rbtData);
    }
    --pRBTHead->size;
  }
  CL_CNT_UNLOCK(pRBTHead->treeMutex);
  return errorCode;
}
/*****************************************************************************/
static ClRcT
cclRBTreeContainerKeySizeGet (ClCntHandleT  rbtHandle,
                              ClCntKeyHandleT userKey,
                              ClUint32T* pSize)
{
  CclRBTreeHead_t* pRBTHead;
  ElementHandle_t* pRBTNode;
  ClCntNodeHandleT tempNodeHandle;
  BaseLinkedListNode_t *pBLlNode;
  ClUint32T number_of_nodes = 0;
  ClRcT errorCode;

  pRBTHead = (CclRBTreeHead_t*)rbtHandle;
  *pSize   = 0;

  CL_CNT_LOCK(pRBTHead->treeMutex);
  errorCode = clCntNodeFind(rbtHandle,userKey,&tempNodeHandle);
  
  if(errorCode == CL_OK) 
  {
    pBLlNode = (BaseLinkedListNode_t *)tempNodeHandle;
    pRBTNode = (ElementHandle_t *)pBLlNode->parentNodeHandle;
    errorCode = clCntSizeGet(pRBTNode->containerHandle,&number_of_nodes);

    if(errorCode == CL_OK)
    {
      *pSize = number_of_nodes;
      CL_CNT_UNLOCK(pRBTHead->treeMutex);
      return (CL_OK);
    }
  }
  CL_CNT_UNLOCK(pRBTHead->treeMutex);
  return errorCode;
}
/******************************************************************************/
static ClRcT
cclRBTreeContainerUserKeyGet (ClCntHandleT     rbtHandle,
                              ClCntNodeHandleT nodeHandle,
                              ClCntKeyHandleT  *pUserKey)
{
  BaseLinkedListNode_t *pTemp    = NULL;
  ElementHandle_t      *pElement = NULL;

  /* Validate the node handle */
  pTemp = (BaseLinkedListNode_t*)nodeHandle;
  nullChkRet(pTemp);
  if( NODE_ID != pTemp->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                     "This node handle doesn't belong to this container");
  }
  /* Check this handle belongs to this RBtree */
  pElement = (ElementHandle_t *) pTemp->parentNodeHandle; 
#if 0
  if( pElement->hListHead != rbtHandle )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                    "This node handle does not belong to this Tree");
  }
#endif

  *pUserKey = pElement->userKey;
  return (CL_OK);

}
/******************************************************************************/
static ClRcT
cclRBTreeContainerUserDataGet (ClCntHandleT     rbtHandle,
                               ClCntNodeHandleT nodeHandle,
                               ClCntDataHandleT *pUserData)
{
  BaseLinkedListNode_t *pTemp    = NULL;
  ElementHandle_t      *pElement = NULL;

  /* Validate the node handle */
  pTemp = (BaseLinkedListNode_t*)nodeHandle;
  nullChkRet(pTemp);
  if( NODE_ID != pTemp->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                    "This node handle is invalid");
  }

  /* Check this handle belongs to this RBtree */
  pElement = (ElementHandle_t *) pTemp->parentNodeHandle; 
#if 0
  if( pElement->hListHead != rbtHandle )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, 
                    "This node handle does not belong to this Tree");
  }
#endif
  *pUserData = pTemp->userData;
  return (CL_OK);
}
/******************************************************************************/
