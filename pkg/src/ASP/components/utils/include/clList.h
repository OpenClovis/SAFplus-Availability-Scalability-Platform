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

#ifndef _CL_LIST_H_
#define _CL_LIST_H_

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClListHead
{
    struct ClListHead *pNext;
    struct ClListHead *pPrev;
} ClListHeadT;

#define CL_LIST_HEAD_EMPTY(head) ( (head)->pNext == (head) )

#define CL_LIST_HEAD_INITIALIZER(head)          \
    { .pNext = &(head),.pPrev = &(head) }

#define CL_LIST_HEAD_DECLARE(head)                      \
    ClListHeadT head = CL_LIST_HEAD_INITIALIZER(head)

#define CL_LIST_HEAD_INIT(head) do {            \
    (head)->pPrev = (head);                     \
    (head)->pNext = (head);                     \
}while(0)

#define __CL_LIST_ADD(element,prev,next) do {   \
    (element)->pNext = (next);                  \
    (element)->pPrev = (prev);                  \
    (prev)->pNext = (element);                  \
    (next)->pPrev = (element);                  \
}while(0)
    
#define __CL_LIST_DEL(prev,next) do {           \
    (prev)->pNext = (next);                     \
    (next)->pPrev = (prev);                     \
}while(0)


static __inline__ void clListAdd(ClListHeadT *pElement,ClListHeadT *pHead)
{
    ClListHeadT *pNext = pHead->pNext;
    __CL_LIST_ADD(pElement,pHead,pNext);
}

static __inline__ void clListAddTail(ClListHeadT *pElement,ClListHeadT *pHead)
{
    ClListHeadT *pPrev = pHead->pPrev;
    __CL_LIST_ADD(pElement,pPrev,pHead);
}

static __inline__ void clListDel(ClListHeadT *pElement)
{
    ClListHeadT *pPrev = pElement->pPrev;
    ClListHeadT *pNext = pElement->pNext;
    if(!pPrev || !pNext) return;
    __CL_LIST_DEL(pPrev,pNext);
    pElement->pNext = pElement->pPrev = NULL;
}

static __inline__ void clListDelInit(ClListHeadT *pElement)
{
    clListDel(pElement);
    CL_LIST_HEAD_INIT(pElement);
}

static __inline__ void clListMoveInit(ClListHeadT *pSource,
                                      ClListHeadT *pDest)
{
    if(pSource != pDest && !CL_LIST_HEAD_EMPTY(pSource))
    {
        ClListHeadT *pFirst = pSource->pNext;
        ClListHeadT *pLast  = pSource->pPrev;
        pDest->pPrev->pNext = pFirst;
        pFirst->pPrev = pDest->pPrev;
        pLast->pNext = pDest;
        pDest->pPrev = pLast;
        CL_LIST_HEAD_INIT(pSource);
    }
}

#define CL_LIST_ENTRY(element,cast,field)                           \
    (cast*)((ClUint8T*)(element) - (ClWordT)(&((cast*)0)->field))

#define CL_LIST_FOR_EACH(iter,head) \
    for(iter = (head)->pNext ; iter != (head) ; iter = iter->pNext)

#ifdef __cplusplus
}
#endif

#endif /*end of _CL_LIST_H_*/
