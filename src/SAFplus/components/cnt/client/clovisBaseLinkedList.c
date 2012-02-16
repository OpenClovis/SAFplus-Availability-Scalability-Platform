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
 * File        : clovisBaseLinkedList.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains a simple Linked-List implementation nternally used by  
 * the Container library.                                                    
 *****************************************************************************/
#include <string.h>

#include "clovisBaseLinkedList.h"
#include <clOsalApi.h>
#include <clCntErrors.h>

#include <clDebugApi.h>
/*#include <containerCompId.h>*/

/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerNodeDelete (ClCntHandleT containerHandle,
                                      ClCntNodeHandleT nodeHandle);
static ClRcT
cclBaseLinkedListContainerFirstNodeGet (ClCntHandleT containerHandle,
                                        ClCntNodeHandleT* pNodeHandle);
static ClRcT
cclBaseLinkedListContainerLastNodeGet (ClCntHandleT containerHandle,
                                       ClCntNodeHandleT* pNodeHandle);
static ClRcT
cclBaseLinkedListContainerNextNodeGet (ClCntHandleT containerHandle,
                                       ClCntNodeHandleT currentNodeHandle,
                                       ClCntNodeHandleT* pNextNodeHandle);
static ClRcT
cclBaseLinkedListContainerPreviousNodeGet (ClCntHandleT containerHandle,
                                           ClCntNodeHandleT nodeHandle,
                                           ClCntNodeHandleT* pPreviousNodeHandle);
static ClRcT
cclBaseLinkedListContainerSizeGet (ClCntHandleT containerHandle,
                                   ClUint32T* pSize);
static ClRcT
cclBaseLinkedListContainerDestroy (ClCntHandleT containerHandle);
/*****************************************************************************/
ClRcT
cclBaseLinkedListCreate (ClCntHandleT* pContainerHandle)
{
  BaseLinkedListHead_t *pLlHead  = NULL;
 
  CL_FUNC_ENTER();
  pLlHead = (BaseLinkedListHead_t*) clHeapCalloc (1, (ClUint32T)sizeof (BaseLinkedListHead_t));
  if (pLlHead == NULL) {
    /* clHeapAllocate failures are not a very nice thing to happen! DEBUG MESSAGE */
      returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }
  pLlHead->pFirstNode = NULL;
  pLlHead->pLastNode  = NULL;

  pLlHead->container.fpFunctionContainerNodeAdd = NULL;

  pLlHead->container.fpFunctionContainerNodeDelete =
    cclBaseLinkedListContainerNodeDelete;

  pLlHead->container.fpFunctionContainerNodeFind = NULL;

  pLlHead->container.fpFunctionContainerFirstNodeGet = 
    cclBaseLinkedListContainerFirstNodeGet;

  pLlHead->container.fpFunctionContainerLastNodeGet = 
    cclBaseLinkedListContainerLastNodeGet;

  pLlHead->container.fpFunctionContainerNextNodeGet = 
    cclBaseLinkedListContainerNextNodeGet;

  pLlHead->container.fpFunctionContainerPreviousNodeGet = 
    cclBaseLinkedListContainerPreviousNodeGet;

  pLlHead->container.fpFunctionContainerSizeGet = 
    cclBaseLinkedListContainerSizeGet;

  pLlHead->container.fpFunctionContainerDestroy =
    cclBaseLinkedListContainerDestroy;

  pLlHead->container.validContainer = CONTAINER_ID;

  *pContainerHandle = (ClCntHandleT) pLlHead;
  
  CL_FUNC_EXIT();
  return (CL_OK);  
}

/*****************************************************************************/
ClRcT
cclBaseLinkedListContainerNodeAdd (ClCntHandleT      containerHandle,
                                   ClCntKeyHandleT   userKey,
                                   ClCntDataHandleT  userData,
                                   ClCntNodeHandleT  nodeHandle,
                                   ClRuleExprT       *pExp)
{
  BaseLinkedListHead_t *pLlHead    = NULL;
  BaseLinkedListNode_t *pLlNewNode = NULL;
  
  CL_FUNC_ENTER();

  pLlHead = (BaseLinkedListHead_t*) containerHandle;
  if( ((pLlHead->pFirstNode == NULL) && (pLlHead->pLastNode != NULL)) ||
      ((pLlHead->pLastNode  == NULL) && (pLlHead->pFirstNode != NULL)) )
  {
      returnCntError(CL_ERR_INVALID_STATE, 
              "Container head & tail pointers are inconsistent");
  }

  pLlNewNode = (BaseLinkedListNode_t*) clHeapCalloc (1, (ClUint32T)sizeof (BaseLinkedListNode_t));
  if (pLlNewNode == NULL)
  {
    /* clHeapAllocate failures are not a very nice thing to happen! DEBUG MESSAGE */
     returnCntError(CL_ERR_NO_MEMORY, "Memory allocation failed");
  }

  pLlNewNode->pNextNode        = NULL;
  pLlNewNode->pPreviousNode    = NULL;
  pLlNewNode->userKey          =  userKey;
  pLlNewNode->userData         = userData;
  pLlNewNode->validNode        = NODE_ID;
  pLlNewNode->pRbeExpression   = pExp;
  pLlNewNode->parentNodeHandle = nodeHandle;  

  if( pLlHead->pFirstNode == NULL )
  {
    /* the new node we are adding is the only node in the list */
    pLlHead->pFirstNode = pLlHead->pLastNode = pLlNewNode;
  }
  else
  {
   /* new-node must become the last node */
    pLlNewNode->pPreviousNode     = pLlHead->pLastNode;
    pLlHead->pLastNode->pNextNode = pLlNewNode;
    pLlHead->pLastNode            = pLlNewNode;
  }

  CL_FUNC_EXIT();
  return (CL_OK);
}

/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerNodeDelete (ClCntHandleT     containerHandle,
                                      ClCntNodeHandleT nodeHandle)
{
  BaseLinkedListHead_t  *pLlHead  = NULL;
  BaseLinkedListNode_t  *pLlNode  = NULL;
	
  CL_FUNC_ENTER();

  pLlHead = (BaseLinkedListHead_t*) containerHandle;
  pLlNode = (BaseLinkedListNode_t *) nodeHandle;

  nullChkRet(pLlHead);
  nullChkRet(pLlNode);

  if(pLlHead->pFirstNode == pLlNode)
  {
    pLlHead->pFirstNode = pLlNode->pNextNode;
  }

  if (pLlHead->pLastNode == pLlNode) 
  {
    pLlHead->pLastNode = pLlNode->pPreviousNode;
  }

  /* at this point in time, the previous and next pointers have to be valid */
  if (pLlNode->pPreviousNode != NULL)
  {
    pLlNode->pPreviousNode->pNextNode = pLlNode->pNextNode;
  }
  
  if (pLlNode->pNextNode != NULL) 
  {
    pLlNode->pNextNode->pPreviousNode = pLlNode->pPreviousNode;
  }

  clHeapFree (pLlNode);

  CL_FUNC_EXIT();
  return CL_OK;
}
/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerFirstNodeGet(ClCntHandleT     containerHandle,
                                       ClCntNodeHandleT *pNodeHandle)
{
  BaseLinkedListHead_t *pLlHead  = NULL;
  
  CL_FUNC_ENTER();
  pLlHead = (BaseLinkedListHead_t*) containerHandle;

  nullChkRet(pLlHead);

  if( NULL == pLlHead->pFirstNode ) 
  {
    returnCntError(CL_ERR_NOT_EXIST, "First node doesn't exist");
  }
  *pNodeHandle = (ClCntNodeHandleT) (pLlHead->pFirstNode);  

  CL_FUNC_EXIT();
  return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerLastNodeGet (ClCntHandleT      containerHandle,
                                       ClCntNodeHandleT  *pNodeHandle)
{
  BaseLinkedListHead_t  *pLlHead  = NULL;
  
  CL_FUNC_ENTER();
  pLlHead = (BaseLinkedListHead_t*) containerHandle;

  nullChkRet(pLlHead);

  if( NULL == pLlHead->pLastNode )
  {
    returnCntError(CL_ERR_NOT_EXIST, "Last Node doesn't exist");
  }

  *pNodeHandle = (ClCntNodeHandleT) (pLlHead->pLastNode);  

  CL_FUNC_EXIT();
  return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerNextNodeGet(ClCntHandleT      containerHandle,
                                      ClCntNodeHandleT  currentNodeHandle,
                                      ClCntNodeHandleT  *pNextNodeHandle)
{
  BaseLinkedListNode_t *pLlNode  = NULL;
  BaseLinkedListHead_t *pLlHead  = NULL;
	
  CL_FUNC_ENTER();

  pLlHead = (BaseLinkedListHead_t *)containerHandle;
  pLlNode = (BaseLinkedListNode_t*)currentNodeHandle;

  nullChkRet(pLlHead);
  nullChkRet(pLlNode);

  if( NODE_ID != pLlNode->validNode )
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "The passed node handle is invalid");
  }
	
  if(pLlNode->pNextNode == NULL)
  {
      returnCntError(CL_ERR_NOT_EXIST, "The next node does not exist");
  }
  *pNextNodeHandle = (ClCntNodeHandleT) (pLlNode->pNextNode);

  CL_FUNC_EXIT();
  return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerPreviousNodeGet(ClCntHandleT     containerHandle,
                                          ClCntNodeHandleT currentNodeHandle,
                                          ClCntNodeHandleT *pPreviousNodeHandle)
{
  BaseLinkedListNode_t *pLlNode  = NULL;
  BaseLinkedListHead_t *pLlHead  = NULL;
	
  CL_FUNC_ENTER();

  pLlHead = (BaseLinkedListHead_t *) containerHandle;
  pLlNode = (BaseLinkedListNode_t*) currentNodeHandle;

  nullChkRet(pLlHead);
  nullChkRet(pLlNode);

  if(pLlNode->validNode != NODE_ID)
  {
      returnCntError(CL_ERR_INVALID_HANDLE, "The previous node doesn't exist");
  }

  if(pLlNode->pPreviousNode == NULL)
  {
      returnCntError(CL_ERR_NOT_EXIST, "The previous node doesn't exist");
  }
  
  *pPreviousNodeHandle = (ClCntNodeHandleT)(pLlNode->pPreviousNode);

  CL_FUNC_EXIT();
  return (CL_OK);
}

/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerSizeGet (ClCntHandleT containerHandle,
                                   ClUint32T    *pSize)
{
  BaseLinkedListHead_t *pLlHead = NULL;
  BaseLinkedListNode_t *pLlNode = NULL;
  ClUint32T            numNodes = 0;
    	
  CL_FUNC_ENTER();

  pLlHead = (BaseLinkedListHead_t*) containerHandle;
  nullChkRet(pLlHead);

  pLlNode = pLlHead->pFirstNode;
  while (pLlNode)
  {
    numNodes++;
    pLlNode = pLlNode->pNextNode;
  }

  *pSize = numNodes;
  CL_FUNC_EXIT();
  return (CL_OK);
}
/*****************************************************************************/
static ClRcT
cclBaseLinkedListContainerDestroy (ClCntHandleT containerHandle)
{
  BaseLinkedListHead_t  *pLlHead     = NULL;
  BaseLinkedListNode_t  *pLlNode     = NULL;
  BaseLinkedListNode_t  *pLlNextNode = NULL;
    	
  CL_FUNC_ENTER();
  pLlHead = (BaseLinkedListHead_t*) containerHandle;
  nullChkRet(pLlHead);

  for (pLlNode = pLlHead->pFirstNode; pLlNode != NULL; pLlNode = pLlNextNode)
  {
    pLlNextNode = pLlNode->pNextNode;
    clHeapFree (pLlNode);
  }
  clHeapFree (pLlHead);

  CL_FUNC_EXIT();
  return (CL_OK);
}
/*****************************************************************************/
#ifdef ORDERED_LIST
ClRcT
cclBaseOrderedLinkedListContainerNodeAdd (
        ClCntHandleT containerHandle,
        ClCntKeyHandleT userKey,
        ClCntDataHandleT userData,
        ClCntNodeHandleT nodeHandle,
        ClRuleExprT* pExp,
        ClCntKeyCompareCallbackT fpKeyCompareFunc )
{
  BaseLinkedListHead_t  *pLlHead    = NULL;
  BaseLinkedListNode_t  *pLlNewNode = NULL;
  BaseLinkedListNode_t  *tmpNode    = NULL;
  
  CL_FUNC_ENTER();
  pLlHead = (BaseLinkedListHead_t*) containerHandle;
  nullChkRet(pLlHead);

  pLlNewNode = (BaseLinkedListNode_t*) clHeapAllocate ((ClUint32T)sizeof (BaseLinkedListNode_t));
  if ( !pLlNewNode )
      return CL_CNT_RC (CL_ERR_NULL_POINTER);

  pLlNewNode->pNextNode         = NULL;
  pLlNewNode->pPreviousNode     = NULL;
  pLlNewNode->userData          = userData;
  pLlNewNode->userKey           = userKey;
  pLlNewNode->validNode         = NODE_ID;
  pLlNewNode->pRbeExpression    = pExp;
  pLlNewNode->parentNodeHandle  = nodeHandle;  
  //pLlNewNode->type              = (ClTypeT *) CL_CNT_BASE_LIST_NODE;
  
  if (pLlHead->pFirstNode == NULL) 
  {
      /*
       * This is the first node in the list
       */ 
      if (pLlHead->pLastNode != NULL) 
      {
          returnCntError(CL_ERR_INVALID_STATE, "The container got corrupted");
      } 
      pLlHead->pFirstNode = pLlHead->pLastNode = pLlNewNode;
      CL_FUNC_EXIT();
      return (CL_OK);
  } 
  /* 
   * Find the right position for this node based on the rank/order/key specified
   * lower values of userkey are assumned to be of higher ranks so userkey = 1 will
   * be inserted before userkey = 10
   */
  tmpNode = pLlHead->pFirstNode;
  while ( (tmpNode) && ((*fpKeyCompareFunc)(pLlNewNode->userKey,tmpNode->userKey)>=0) )
  {
      tmpNode = tmpNode->pNextNode;
  }

  if ( tmpNode == pLlHead->pFirstNode )
  {
      /*
       * This is the new first node in the list
       */
      pLlNewNode->pNextNode = tmpNode;
      tmpNode->pPreviousNode = pLlNewNode;
      pLlHead->pFirstNode = pLlNewNode;
  }
  else if ( tmpNode == NULL )
  {
      /*
       * This is the new last node in the list
       */
      pLlNewNode->pPreviousNode = pLlHead->pLastNode;
      pLlHead->pLastNode->pNextNode = pLlNewNode;
      pLlHead->pLastNode = pLlNewNode; 
  }
  else
  {
      /*
       * This node is in the middle of the list
       */
      pLlNewNode->pPreviousNode = tmpNode->pPreviousNode;
      pLlNewNode->pNextNode = tmpNode;
      pLlNewNode->pPreviousNode->pNextNode = pLlNewNode;
      pLlNewNode->pNextNode->pPreviousNode = pLlNewNode;
  }

  CL_FUNC_EXIT();
  return (CL_OK);
}
#endif /*ifdef ORDERED_LIST */
