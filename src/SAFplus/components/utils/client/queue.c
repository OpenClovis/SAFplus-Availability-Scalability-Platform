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
 * File        : queue.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the Queue implementation.           
 *****************************************************************************/
#include <stdlib.h>

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clOsalErrors.h>

#include <clQueueApi.h>
#include <clQueueErrors.h>

#include <clClistApi.h>
#include <clClistErrors.h>
                                 
#include <clDebugApi.h>
/*
 * #include <dbg.h>
#include <clDebugApi.h>
 * #include <containerCompId.h>
 */

/*****************************************************************************/
#define VALID_Q      0x98ABCD

/*****************************************************************************/
/* This MACRO checks if a pointer is NULL */
#define NULL_POINTER_CHECK(X)  do { if(NULL == (X)) return CL_QUEUE_RC(CL_ERR_NULL_POINTER); } while(0)


/*****************************************************************************/
/* This MACRO checks if a queue handle is valid */
#define QUEUE_VALIDITY_CHECK(X)  do { if(NULL == (X)) { \
                                   return CL_QUEUE_RC(CL_ERR_INVALID_HANDLE); \
                                 } \
                                 if(VALID_Q != (X)->validQueue) { \
                                   return CL_QUEUE_RC(CL_ERR_INVALID_HANDLE); \
                                 }} while(0)

/*****************************************************************************/
typedef struct QueueHead_t {
  ClUint32T     validQueue;
  ClQueueDequeueCallbackT      fpUserDequeueCallBack;
  ClQueueDequeueCallbackT      fpUserDestroyCallBack;
  ClClistT cListHandle;
} QueueHead_t;
/*****************************************************************************/
ClRcT
clQueueCreate(ClUint32T  maxSize,
              ClQueueDequeueCallbackT   fpUserDequeueCallBack,
              ClQueueDequeueCallbackT   fpUserDestroyCallBack,
              ClQueueT*    pQueueHead)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = NULL;
  
  /* check if pQueueHead pointer is NULL */
  NULL_POINTER_CHECK(pQueueHead);
  
  /* check if the user call back function pointer is NULL */
  NULL_POINTER_CHECK(fpUserDequeueCallBack);

  /* check if the user call back function pointer is NULL */
  NULL_POINTER_CHECK(fpUserDestroyCallBack);
  
  pQueue = (QueueHead_t *)clHeapAllocate((ClUint32T)sizeof(QueueHead_t));
  
  /* check if clHeapAllocate failed */
  NULL_POINTER_CHECK(pQueue);
  
  errorCode = clClistCreate(maxSize, CL_NO_DROP, fpUserDequeueCallBack, fpUserDestroyCallBack, &(pQueue->cListHandle));

  if(CL_OK != errorCode) {
     /* failed to create a circular list. return error */
     clHeapFree(pQueue);
     CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nFailed to create a circular list"));
     return(errorCode);
  }

  pQueue->validQueue = VALID_Q;
  pQueue->fpUserDequeueCallBack = fpUserDequeueCallBack;
  pQueue->fpUserDestroyCallBack = fpUserDestroyCallBack;
  *pQueueHead = (ClQueueT)pQueue;
  return(CL_OK);  
}
/*****************************************************************************/
ClRcT
clQueueNodeInsert(ClQueueT      queueHead, 
          ClQueueDataT  userData)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = (QueueHead_t *)queueHead;
  
  /* check if the queue handle is valid */
  QUEUE_VALIDITY_CHECK(pQueue);
  
  /* if everything is ok, add the data at the end of the circular list */
  errorCode = clClistLastNodeAdd(pQueue->cListHandle,userData);
  if(CL_OK != errorCode) {
    return(errorCode);
  }
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clQueueNodeDelete(ClQueueT  queueHead, ClQueueDataT* userData)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = (QueueHead_t *)queueHead;
  ClClistNodeT cListNode = 0;
/*  ClQueueDataT userData = 0; */

  /* check if the queue handle is valid */
  QUEUE_VALIDITY_CHECK(pQueue);

  /* check if userData handle is NULL */
  NULL_POINTER_CHECK(userData); 

  /* if everything is ok, get the first node in the list and delete it */
  errorCode = clClistFirstNodeGet(pQueue->cListHandle, &cListNode);
  if(CL_OK != errorCode) {
    return(errorCode);
  }

  /* get the data from the node */
  errorCode = clClistDataGet(pQueue->cListHandle, cListNode, userData);
  if(CL_OK != errorCode) {
    return(errorCode);
  }

  /* delete the node */
  errorCode = clClistNodeDelete(pQueue->cListHandle, cListNode);
  if(CL_OK != errorCode) {
    return(errorCode);
  }

  /* return CL_OK */
  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clQueueWalk(ClQueueT         queueHead,
            ClQueueWalkCallbackT fpUserWalkFunction,
            void*           userArg)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = (QueueHead_t *)queueHead;
  ClClistNodeT cListNode = 0;
  
  /* check if the queue handle is valid */
  QUEUE_VALIDITY_CHECK(pQueue);

  /* check if the user walk function pointer is NULL */
  NULL_POINTER_CHECK(fpUserWalkFunction);

  /* if everything is ok, traverse the queue and for each data element, call the user walk function */

  /* Get the first node */
  errorCode = clClistFirstNodeGet(pQueue->cListHandle,&cListNode);
  if(CL_OK != errorCode) {
    if((CL_GET_ERROR_CODE(errorCode)) == CL_ERR_NOT_EXIST) {
      /* If the queue is empty it means we dont have to walk anymore.
       * Hence return OK.
       */
      return (CL_OK);
    }
    return(errorCode);
  }

  errorCode = clClistWalk(pQueue->cListHandle, fpUserWalkFunction, userArg);
  if(CL_OK != errorCode) {
    return (errorCode);
  }

  return(CL_OK);
}
/*****************************************************************************/
ClRcT
clQueueSizeGet(ClQueueT      queueHead,
               ClUint32T*  pSize)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = (QueueHead_t *)queueHead;

  /* check if the queue handle is valid */
  QUEUE_VALIDITY_CHECK(pQueue);
  
  /* check if the size pointer is NULL */
  NULL_POINTER_CHECK(pSize);

  /* if everything is OK, then return CL_OK */
  errorCode = clClistSizeGet(pQueue->cListHandle, pSize);
  if(CL_OK != errorCode) {
    return(errorCode);
  }
  
  return(CL_OK);

}
/*****************************************************************************/
ClRcT
clQueueDelete(ClQueueT* pQueueHead)
{
  ClRcT errorCode = CL_ERR_NULL_POINTER;
  QueueHead_t* pQueue = NULL;

  /* check if the pointer to the handle is NULL */
  NULL_POINTER_CHECK(pQueueHead);

  pQueue = (QueueHead_t *)*pQueueHead;

  /* check if the queue handle is valid */
  QUEUE_VALIDITY_CHECK(pQueue);

  /* if everything is OK, destroy the queue */

  errorCode = clClistDelete(&(pQueue->cListHandle));
  if(CL_OK != errorCode) {
    return (errorCode);
  }

  clHeapFree(pQueue);
  *pQueueHead = 0;
  return(CL_OK);
}
/*****************************************************************************/
