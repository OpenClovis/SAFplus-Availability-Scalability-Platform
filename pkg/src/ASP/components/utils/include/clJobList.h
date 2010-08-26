/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#ifndef _CL_JOB_H_
#define _CL_JOB_H_

#include <clCommon.h>

#ifdef __cplusplus

extern "C" {

#endif

#define CL_JOB_RC(rc) CL_RC(CL_CID_JOB, rc)

typedef struct ClJobList
{
    struct ClJobList *pNext;
    struct ClJobList *pPrev;
    ClPtrT pData;
} ClJobListT;

#define CL_JOB_LIST_EMPTY(head) ( (head)->pNext == (head) )

#define CL_JOB_LIST_INITIALIZER(head)           \
    { .pNext = &(head), .pPrev = &(head) }

#define CL_JOB_LIST_DECLARE(head)                   \
    ClJobListT head = CL_JOB_LIST_INITIALIZER(head)

#define CL_JOB_LIST_INIT(head) do {             \
    (head)->pPrev = (head);                     \
    (head)->pNext = (head);                     \
}while(0)

#define __CL_JOB_ADD(element,prev,next) do {    \
    (element)->pNext = (next);                  \
    (element)->pPrev = (prev);                  \
    (prev)->pNext = (element);                  \
    (next)->pPrev = (element);                  \
}while(0)
    
#define __CL_JOB_DEL(prev,next) do {            \
    (prev)->pNext = (next);                     \
    (next)->pPrev = (prev);                     \
}while(0)


static __inline__ ClRcT clJobAdd(ClPtrT pData,ClJobListT *pHead)
{
    ClJobListT *pJob = (ClJobListT *) clHeapCalloc(1,sizeof(*pJob));
    ClJobListT *pNext = pHead->pNext;
    if(pJob == NULL)
    {
        return CL_JOB_RC(CL_ERR_NO_MEMORY);
    }
    pJob->pData = pData;
    __CL_JOB_ADD(pJob,pHead,pNext);
    return CL_OK;
}

static __inline__ ClRcT clJobAddTail(ClPtrT pData,ClJobListT *pHead)
{
    ClJobListT *pJob = (ClJobListT *) clHeapCalloc(1, sizeof(*pJob));
    ClJobListT *pPrev = pHead->pPrev;
    if(pJob == NULL)
    {
        return CL_JOB_RC(CL_ERR_NO_MEMORY);
    }
    pJob->pData = pData;
    __CL_JOB_ADD(pJob,pPrev,pHead);
    return CL_OK;
}

static __inline__ ClPtrT clJobPop(ClJobListT *pHead)
{
    ClPtrT pData = NULL;
    if(!CL_JOB_LIST_EMPTY(pHead))
    {
        ClJobListT *pJob = pHead->pNext;
        ClJobListT *pNext = pJob->pNext;
        pData = pJob->pData;
        __CL_JOB_DEL(pHead,pNext);
        clHeapFree(pJob);
    }
    return pData;
}

#define CL_JOB_FOR_EACH(iter,head)                                  \
    for(iter = (head)->pNext ; iter != (head) ; iter = iter->pNext)

#ifdef __cplusplus
}
#endif

#endif /*end of _CL_JOB_H_*/
