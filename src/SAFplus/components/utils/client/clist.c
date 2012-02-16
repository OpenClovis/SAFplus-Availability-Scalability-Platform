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
 * File        : clist.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the Circular Linked List implementation.               
 *****************************************************************************/
#include <stdlib.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clClistApi.h>
#include <clClistErrors.h>

#include <clOsalApi.h>
#include <clOsalErrors.h>

#include <clDebugApi.h>
/*#include <containerCompId.h>*/
/*****************************************************************************/
#define CLIST_ID 0xABCD
#define CNODE_ID 0xCDEF

/*****************************************************************************/
/* This MACRO checks if a pointer is NULL */
#define NULL_POINTER_CHECK(X)  do { if(NULL == (X)) return CL_CLIST_RC(CL_ERR_NULL_POINTER); } while(0)

/*****************************************************************************/
/* This MACRO checks if a list is valid */
#define CLIST_VALIDITY_CHECK(X) do { if(NULL == (X)) {                   \
                                 return CL_CLIST_RC(CL_ERR_INVALID_HANDLE); \
                                } \
                                if(CLIST_ID != (X)->validList) { \
                                  return CL_CLIST_RC(CL_ERR_INVALID_HANDLE); \
                                }} while(0)

/*****************************************************************************/
/* This macro checks if the node is a clist node and if the node belongs to 
 * the specified list */
                            
#define CLNODE_VALIDITY_CHECK(X)  do{if(NULL == (X)) {                  \
                                    return CL_CLIST_RC(CL_ERR_INVALID_HANDLE); \
                                  } \
                                  if(CNODE_ID != (X)->validNode) { \
                                    return CL_CLIST_RC(CL_ERR_INVALID_HANDLE); \
                                  } \
                                  if(listHead != (X)->parentHandle) { \
                                    return CL_CLIST_RC(CL_ERR_INVALID_HANDLE); \
                                  }} while(0)

/*****************************************************************************/
typedef struct CircularLinkedListNode_t {
  ClUint32T                         validNode;
  ClClistT                     parentHandle;
  ClClistDataT                         userData;
  struct CircularLinkedListNode_t*   pPreviousNode;
  struct CircularLinkedListNode_t*   pNextNode;
} CircularLinkedListNode_t;
/*****************************************************************************/
typedef struct CircularLinkedListHead_t {
  ClUint32T                         validList;
  ClUint32T                         maxSize;
  ClUint32T                         dropPolicy;
  ClUint32T                         currentSize;
  ClClistDeleteCallbackT                      fpUserDeleteCallBack;
  ClClistDeleteCallbackT                      fpUserDestroyCallBack;
  struct CircularLinkedListNode_t*   pFirstNode;
  struct CircularLinkedListNode_t*   pLastNode;
} CircularLinkedListHead_t;
/*****************************************************************************/
ClRcT
clClistCreate(ClUint32T       maxSize,
                     ClClistDropPolicyT   dropPolicy,
                     ClClistDeleteCallbackT    fpUserDeleteCallBack,
                     ClClistDeleteCallbackT    fpUserDestroyCallBack,
                     ClClistT*  pListHead)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t *pCListHead = NULL;

  /* check if the pointer to list head is NULL */
  NULL_POINTER_CHECK(pListHead);

  /* check if the drop policy is valid */
  if(CL_DROP_MAX_TYPE <= dropPolicy) {
      errorCode = CL_CLIST_RC(CL_ERR_INVALID_PARAMETER);
      CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nDrop policy not valid"));
      return(errorCode);
  }
  
  /* check if the delete function pointer is NULL */
  NULL_POINTER_CHECK(fpUserDeleteCallBack);

  /* check if the destroy function pointer is NULL */
  NULL_POINTER_CHECK(fpUserDestroyCallBack);
  
  pCListHead = (CircularLinkedListHead_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListHead_t));

  /* check if clHeapAllocate has failed */
  NULL_POINTER_CHECK(pCListHead);

  /* check if the maximum size of the circular linked list is zero */
  if(0 == maxSize) {
    /* set the maximum size to the maximum value of ClUint32T */
    maxSize = ~0;
  }


  /* Initialize the first and last pointers to NULL and set the validity field.
   * Initialize maxSize to the value passed by user */
  pCListHead->pFirstNode = NULL;
  pCListHead->pLastNode = NULL;
  pCListHead->validList = CLIST_ID;
  pCListHead->maxSize = maxSize;
  pCListHead->dropPolicy = dropPolicy;
  pCListHead->currentSize = 0;
  pCListHead->fpUserDeleteCallBack = fpUserDeleteCallBack;
  pCListHead->fpUserDestroyCallBack = fpUserDestroyCallBack;

  /* Return the list handle in pListHead */
  *pListHead =pCListHead;
  return(CL_OK);

}
/*****************************************************************************/
ClRcT
clClistFirstNodeAdd(ClClistT  listHead,
                    ClClistDataT      userData)
{
    ClRcT errorCode = CL_ERR_NULL_POINTER;
    CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
    CircularLinkedListNode_t* pCListNode = NULL;

    /* Check if the Circular List Handle is valid */
    CLIST_VALIDITY_CHECK(pCListHead);
  
    /* Check if the number of nodes has reached a maximum value */
    if(pCListHead->currentSize >= pCListHead->maxSize) {
  
        /* Check if the dropPolicy is CL_NO_DROP */
        if(CL_NO_DROP == pCListHead->dropPolicy) {    
            errorCode = CL_CLIST_RC(CL_CLIST_ERR_MAXSIZE_REACHED);
            CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nDrop policy - Max size reached"));
            return (errorCode);
        }
        else {
            /* delete the first node in the list */
            errorCode = clClistNodeDelete(listHead, pCListHead->pFirstNode);      
        }
    }

    pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

    /* check if clHeapAllocate failed */
    NULL_POINTER_CHECK(pCListNode);
  

    /* fill up the contents of the node */
    pCListNode->validNode = CNODE_ID;
    pCListNode->userData = userData;
    pCListNode->parentHandle = listHead;


    /* if this is the first node being added into the list */
    if((NULL == pCListHead->pFirstNode) 
       || 
       (NULL == pCListHead->pLastNode)) 
    {
        CL_ASSERT(pCListHead->pFirstNode == NULL 
                  &&
                  pCListHead->pLastNode == NULL);
        pCListNode->pNextNode = pCListNode;
        pCListNode->pPreviousNode = pCListNode;
        pCListHead->pFirstNode = pCListNode;
        pCListHead->pLastNode = pCListNode;
        (pCListHead->currentSize)++;
        return (CL_OK);
       
    }

    pCListNode->pNextNode = pCListHead->pFirstNode;
    pCListNode->pPreviousNode = pCListHead->pLastNode;
    pCListHead->pFirstNode->pPreviousNode = pCListNode;
    pCListHead->pLastNode->pNextNode = pCListNode;

    pCListHead->pFirstNode = pCListNode;
    (pCListHead->currentSize)++;
    return (CL_OK);
}
/*****************************************************************************/
ClRcT
clClistLastNodeAdd(ClClistT  listHead,
                   ClClistDataT      userData)
{
    ClRcT errorCode = CL_ERR_NULL_POINTER;
    CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
    CircularLinkedListNode_t* pCListNode = NULL;

    /* Check if the Circular List Handle is valid */
    CLIST_VALIDITY_CHECK(pCListHead);

    /* Check if the number of nodes has reached a maximum value */
    if(pCListHead->currentSize >= pCListHead->maxSize) {
        /* Check if the dropPolicy is CL_NO_DROP */
        if(CL_NO_DROP == pCListHead->dropPolicy) {    
            errorCode = CL_CLIST_RC(CL_CLIST_ERR_MAXSIZE_REACHED);
            CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nDrop policy - Max size reached"));
            return (errorCode);
        }
        else {
            /* delete the first node in the list */
            errorCode = clClistNodeDelete(listHead, pCListHead->pFirstNode);      
        }    
    }

    pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

    /* check if clHeapAllocate failed */
    NULL_POINTER_CHECK(pCListNode);
  
    /* fill up the contents of the node */
    pCListNode->validNode = CNODE_ID;
    pCListNode->userData = userData;
    pCListNode->parentHandle = listHead;


    /* if this is the first node being added into the list */
    if((NULL == pCListHead->pFirstNode) 
       ||
       (NULL == pCListHead->pLastNode)) {

        CL_ASSERT(pCListHead->pFirstNode == NULL
                  &&
                  pCListHead->pLastNode == NULL);
        pCListNode->pNextNode = pCListNode;
        pCListNode->pPreviousNode = pCListNode;
        pCListHead->pFirstNode = pCListNode;
        pCListHead->pLastNode = pCListNode;
        (pCListHead->currentSize)++;
        return (CL_OK);
       
    }

    pCListNode->pNextNode = pCListHead->pFirstNode;
    pCListNode->pPreviousNode = pCListHead->pLastNode;
    pCListHead->pFirstNode->pPreviousNode = pCListNode;
    pCListHead->pLastNode->pNextNode = pCListNode;

    pCListHead->pLastNode = pCListNode;
    (pCListHead->currentSize)++;
    return (CL_OK);
}
/*****************************************************************************/
ClRcT
clClistAfterNodeAdd(ClClistT      listHead,
                           ClClistNodeT  currentNode,
                           ClClistDataT          userData)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t *pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t *pCListCurrentNode = (CircularLinkedListNode_t *)currentNode;
  CircularLinkedListNode_t *pCListNode = NULL;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListCurrentNode);

  /* Check if the number of nodes has reached a maximum value */
  if(pCListHead->currentSize >= pCListHead->maxSize) {
    /* Check if the dropPolicy is CL_NO_DROP */
    if(CL_NO_DROP == pCListHead->dropPolicy) {    
        errorCode = CL_CLIST_RC(CL_CLIST_ERR_MAXSIZE_REACHED);
        CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nDrop policy - Max size reached"));
        return (errorCode);
    }
    else {
      /* check if the current node after which the new node is to be added is the first node */
      if(pCListCurrentNode == pCListHead->pFirstNode) {
      /** if so, then delete the first node(node after which new node is being added) and
       *  make the node being added as the first node */
      
        /* delete the first node in the list */      
        errorCode =clClistNodeDelete(listHead, pCListHead->pFirstNode);
      
        pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

        /*check if clHeapAllocate failed */
        NULL_POINTER_CHECK(pCListNode);
  
        /* fill up the contents of the node */
        pCListNode->validNode = CNODE_ID;
        pCListNode->userData = userData;
        pCListNode->parentHandle = listHead;
      
        /* if this is the first node being added into the list */
        if((NULL == pCListHead->pFirstNode) && 
           (NULL == pCListHead->pLastNode)) {
           
           pCListNode->pNextNode = pCListNode;
           pCListNode->pPreviousNode = pCListNode;
           pCListHead->pFirstNode = pCListNode;
           pCListHead->pLastNode = pCListNode;
           (pCListHead->currentSize)++;
           return (CL_OK);
        }
    
        pCListNode->pNextNode = pCListHead->pFirstNode;
        pCListNode->pPreviousNode = pCListHead->pLastNode;
        pCListHead->pFirstNode->pPreviousNode = pCListNode;
        pCListHead->pLastNode->pNextNode = pCListNode;

        pCListHead->pFirstNode = pCListNode;
        (pCListHead->currentSize)++;
        return (CL_OK);
      
      }
      else { /* the node after which the new node is to be added is not the first node */
        /* delete the first node in the list */      
        errorCode = clClistNodeDelete(listHead, pCListHead->pFirstNode);  
      }
            
    }
  }  

  pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

  /*check if clHeapAllocate failed */
  NULL_POINTER_CHECK(pCListNode);
  
  /* fill up the contents of the node */
  pCListNode->validNode = CNODE_ID;
  pCListNode->userData = userData;
  pCListNode->parentHandle = listHead;


  /* Now, add the node after the specified node */
  pCListNode->pNextNode = pCListCurrentNode->pNextNode;
  pCListNode->pPreviousNode = pCListCurrentNode;
  pCListCurrentNode->pNextNode = pCListNode;

  if(NULL != pCListNode->pNextNode) {
    pCListNode->pNextNode->pPreviousNode = pCListNode;
  }

  /* if the currentNode is the last node, make the node to be added as the last node */
  if(pCListCurrentNode == pCListHead->pLastNode) {
    pCListHead->pLastNode = pCListNode;
  }

  (pCListHead->currentSize)++;
  return(CL_OK);

}
/*****************************************************************************/
ClRcT
clClistBeforeNodeAdd(ClClistT      listHead,
                            ClClistNodeT  currentNode,
                            ClClistDataT          userData)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t *pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t *pCListCurrentNode = (CircularLinkedListNode_t *)currentNode;
  CircularLinkedListNode_t *pCListNode = NULL;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListCurrentNode);

  /* Check if the number of nodes has reached a maximum value */
  if(pCListHead->currentSize >= pCListHead->maxSize) {
    /* Check if the dropPolicy is CL_NO_DROP */
    if(CL_NO_DROP == pCListHead->dropPolicy) {    
        errorCode = CL_CLIST_RC(CL_CLIST_ERR_MAXSIZE_REACHED);
        CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nDrop policy - Max size reached"));
        return (errorCode);
    }
    else {
      /* check if the node before which the new node is being added is the first node */
      if(pCListHead->pFirstNode == pCListCurrentNode) {
      
        /* delete the first node in the list */
        errorCode = clClistNodeDelete(listHead, pCListHead->pFirstNode);
      
        /* if this is the first node being added into the list */
        if((NULL == pCListHead->pFirstNode) && 
           (NULL == pCListHead->pLastNode)) {
           
           pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

           /* check if clHeapAllocate failed */
           NULL_POINTER_CHECK(pCListNode);
  
           /* fill up the contents of the node */
           pCListNode->validNode = CNODE_ID;
           pCListNode->userData = userData;
           pCListNode->parentHandle = listHead;
           
           pCListNode->pNextNode = pCListNode;
           pCListNode->pPreviousNode = pCListNode;
           pCListHead->pFirstNode = pCListNode;
           pCListHead->pLastNode = pCListNode;
           (pCListHead->currentSize)++;
           return (CL_OK);
        }
        pCListCurrentNode = pCListHead->pFirstNode;
      
      }
      else { /* if the node before which the new node is being added is not the first node */
        /* delete the first node in the list */
        errorCode = clClistNodeDelete(listHead, pCListHead->pFirstNode);
      }
            
    }
  }

  pCListNode = (CircularLinkedListNode_t *)clHeapAllocate((ClUint32T)sizeof(CircularLinkedListNode_t));

  /* check if clHeapAllocate failed */
  NULL_POINTER_CHECK(pCListNode);
  
  /* fill up the contents of the node */
  pCListNode->validNode = CNODE_ID;
  pCListNode->userData = userData;
  pCListNode->parentHandle = listHead;


  /* Now, add the node before the specified node */
  pCListNode->pNextNode = pCListCurrentNode;
  pCListNode->pPreviousNode = pCListCurrentNode->pPreviousNode;
  pCListCurrentNode->pPreviousNode = pCListNode;

  if(NULL != pCListNode->pPreviousNode) {
    pCListNode->pPreviousNode->pNextNode = pCListNode;
  }

  /* if the currentNode is the first node, make the node being added the first node */
  if(pCListCurrentNode == pCListHead->pFirstNode) {
    pCListHead->pFirstNode = pCListNode;
  }

  (pCListHead->currentSize)++;
  return(CL_OK);

}
/*****************************************************************************/
ClRcT
clClistNodeDelete(ClClistT      listHead,
                         ClClistNodeT  node)
{
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t* pCListNode = (CircularLinkedListNode_t *)node;
  ClClistDataT userData;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);
  
  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListNode);

  /* if everything is ok, delete the specified node from the list */
  
  /* if the node being deleted is the only node in the list */
  if( (pCListHead->pFirstNode == pCListNode) &&
      (pCListHead->pLastNode == pCListNode) ) {
      
    /*Make the first and last pointers NULL */
    pCListHead->pFirstNode = NULL;
    pCListHead->pLastNode = NULL;
      
    /* Decrement the currentSize of the list by 1 */
    (pCListHead->currentSize)--;
      
    /* clHeapFree the node and return CL_OK */
    userData = pCListNode->userData;
    pCListNode->validNode = 0;
    clHeapFree(pCListNode);
    pCListHead->fpUserDeleteCallBack(userData);
    return(CL_OK);
    
  }

  pCListNode->pPreviousNode->pNextNode = pCListNode->pNextNode;
  pCListNode->pNextNode->pPreviousNode = pCListNode->pPreviousNode;
  
  /* if the node being deleted is the first node */
  if(pCListNode == pCListHead->pFirstNode) {
    pCListHead->pFirstNode = pCListNode->pNextNode;
  }

  /* if the node being deleted is the last node */
  if(pCListNode == pCListHead->pLastNode) {
    pCListHead->pLastNode = pCListNode->pPreviousNode;
  }

  /* clHeapFree the node and return CL_OK */
  (pCListHead->currentSize)--;
  userData = pCListNode->userData;
  pCListNode->validNode = 0;
  clHeapFree(pCListNode);
  pCListHead->fpUserDeleteCallBack(userData);
  return(CL_OK);
  
}
/*****************************************************************************/
ClRcT
clClistFirstNodeGet(ClClistT       listHead,
                           ClClistNodeT*  pFirstNode)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  
  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* Check if pFirstNode pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pFirstNode);
  
  /* Check if the list is empty. If it is empty, return error */
  if( (NULL == pCListHead->pFirstNode) && 
      (NULL == pCListHead->pLastNode) ) {
      
    errorCode = CL_CLIST_RC(CL_ERR_NOT_EXIST);
    /* This expected error is how to check for an empty list, so even trace is too much logging */
    /* CL_DEBUG_PRINT (CL_DEBUG_TRACE,("Node does not exist")); */
    return(errorCode);
      
  }

  /* if everything is fine, return CL_OK */
  *pFirstNode = pCListHead->pFirstNode;
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clClistLastNodeGet(ClClistT       listHead,
                          ClClistNodeT*  pLastNode)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* Check if pLastNode pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pLastNode);
  
  /* Check if the list is empty. If it is empty, return error */
  if( (NULL == pCListHead->pFirstNode) && 
      (NULL == pCListHead->pLastNode) ) {
      
    errorCode = CL_CLIST_RC(CL_ERR_NOT_EXIST);
    CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nNode does not exist"));
    return(errorCode);
      
  }

  /* if everything is fine, return CL_OK */
  *pLastNode = pCListHead->pLastNode;
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clClistNextNodeGet(ClClistT       listHead,
                          ClClistNodeT   currentNode,
                          ClClistNodeT*  pNextNode)
{
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t* pCListCurrentNode = (CircularLinkedListNode_t *)currentNode;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListCurrentNode);

  /* Check if pNextNode pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pNextNode);
  
  /* if everything is fine, return CL_OK */
  *pNextNode = pCListCurrentNode->pNextNode;
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clClistPreviousNodeGet(ClClistT       listHead,
                              ClClistNodeT   currentNode,
                              ClClistNodeT*  pPreviousNode)
{
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t* pCListCurrentNode = (CircularLinkedListNode_t *)currentNode;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListCurrentNode);

  /* Check if pPreviousNode pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pPreviousNode);
  
  /* if everything is fine, return CL_OK */
  *pPreviousNode = pCListCurrentNode->pPreviousNode;
  return(CL_OK);
}
/*****************************************************************************/
static ClRcT
clCircularListLimitedWalk(ClClistT      listHead,
                          ClClistNodeT  startListNode,
                          ClClistNodeT  endListNode,
                          ClClistWalkCallbackT         fpUserWalkCallBack,
                          void*               userArg)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t*)listHead;
  CircularLinkedListNode_t* pCListStartNode = (CircularLinkedListNode_t*)startListNode;
  CircularLinkedListNode_t* pCListEndNode = (CircularLinkedListNode_t*)endListNode;
  ClClistNodeT cListTraverseNode = 0;
  ClClistDataT userData = 0;
  
  /* check if the list handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListStartNode);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListEndNode);

  /* check if the user walk function pointer is NULL */
  NULL_POINTER_CHECK(fpUserWalkCallBack);
  
  /* if everything is ok, traverse the list and for each data element, call the user walk function */
  cListTraverseNode = startListNode;

  while(cListTraverseNode != endListNode) /* repeat until you get to the end node */
  {
    /* get the data in the current node */
    errorCode = clClistDataGet(listHead, cListTraverseNode, &userData);
    if(CL_OK != errorCode) {
       return(errorCode);
    }
 
    /* Call the user function */
    fpUserWalkCallBack(userData, userArg);
    /* get the next node */
    errorCode = clClistNextNodeGet(listHead, cListTraverseNode, &cListTraverseNode);
    if(CL_OK != errorCode) {
       return(errorCode);
    }

 }

 /* get the data in the end node */
 errorCode = clClistDataGet(listHead, cListTraverseNode, &userData);
 if(CL_OK != errorCode) {
    return(errorCode);
 }
 
 /* Call the user function */
 fpUserWalkCallBack(userData, userArg);

 return(CL_OK);

}
/*****************************************************************************/
ClRcT
clClistWalk(ClClistT  listHead,
                   ClClistWalkCallbackT     fpUserWalkCallBack,
                   void*           userArg)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t*)listHead;
  ClClistNodeT cListTraverseNode = 0;
  ClClistNodeT cListLastNode = 0;
  
  /* check if the list handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if the user walk function pointer is NULL */
  NULL_POINTER_CHECK(fpUserWalkCallBack);
  
  /* if everything is ok, call the limited walk function with start node
   * as the first and end node as the last node */
  
  errorCode = clClistFirstNodeGet(listHead, &cListTraverseNode);
  if(CL_OK != errorCode) {
    if((CL_GET_ERROR_CODE(errorCode)) == CL_ERR_NOT_EXIST) {
      /* If the list is empty it means we dont have to walk anymore.
       * Hence return OK.
       */
      return (CL_OK);
    }
    return(errorCode);
  }

  errorCode = clClistLastNodeGet(listHead, &cListLastNode);
  if(CL_OK != errorCode) {
    if((CL_GET_ERROR_CODE(errorCode)) == CL_ERR_NOT_EXIST) {
      /* If the list is empty it means we dont have to walk anymore.
       * Hence return OK.
       */
      return (CL_OK);
    }
    return(errorCode);
  }

  errorCode = clCircularListLimitedWalk(listHead, cListTraverseNode, cListLastNode, fpUserWalkCallBack, userArg);
  return (errorCode);

}
/*****************************************************************************/
ClRcT
clClistDataGet(ClClistT      listHead,
                      ClClistNodeT  node,
                      ClClistDataT*         pUserData)
{
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  CircularLinkedListNode_t* pCListNode = (CircularLinkedListNode_t *)node;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* check if it is a valid node and if it belongs to the specified list */
  CLNODE_VALIDITY_CHECK(pCListNode);

  /* Check if pUserData pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pUserData);
  
  /* if everything is ok, return CL_OK */
  *pUserData = pCListNode->userData;
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clClistSizeGet(ClClistT  listHead,
                      ClUint32T*     pSize)
{
  CircularLinkedListHead_t* pCListHead = (CircularLinkedListHead_t *)listHead;
  
  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* Check if pSize pointer passed by the user is NULL */
  NULL_POINTER_CHECK(pSize);
  
  /* if everything is ok, return CL_OK */
  *pSize = pCListHead->currentSize;
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clClistDelete(ClClistT* pListHead)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  CircularLinkedListHead_t* pCListHead = NULL;
  CircularLinkedListNode_t* pCListNode = NULL;
  CircularLinkedListNode_t* pCListFreeNode = NULL;
  ClClistDataT userData = 0;
  ClUint32T size = 0;
  ClUint32T i = 0;
  
  /* Check if the pointer to Circular List Handle is NULL */
  NULL_POINTER_CHECK(pListHead);

  pCListHead = (CircularLinkedListHead_t *)*pListHead;

  /* Check if the Circular List Handle is valid */
  CLIST_VALIDITY_CHECK(pCListHead);

  /* if everything is ok, then delete all the nodes in the list */

  /* get the size of the list */
  errorCode = clClistSizeGet(*pListHead, &size);
  if(CL_OK != errorCode) {
    return(errorCode);
  }

  pCListNode = pCListHead->pFirstNode;

  for(i = 0;i < size; i++) {
    pCListFreeNode = pCListNode;
    /* copy the user data of the node being deleted */
    userData = pCListNode->userData;
    /* move the current node to next node */
    pCListNode = pCListNode->pNextNode;
    /* make the node invalid */
    pCListFreeNode->validNode = 0;
    /* clHeapFree the node */
    clHeapFree(pCListFreeNode);
    /* call the destroy callback function */
    pCListHead->fpUserDestroyCallBack(userData);
  }

  /* make the list handle invalid */
  pCListHead->validList = 0;

  /* clHeapFree the list handle */
  clHeapFree(pCListHead);

  /* Set the list handle to NULL and return CL_OK */
  *pListHead = NULL;

  /* return CL_OK */
  return (CL_OK);
  
}
/*****************************************************************************/
