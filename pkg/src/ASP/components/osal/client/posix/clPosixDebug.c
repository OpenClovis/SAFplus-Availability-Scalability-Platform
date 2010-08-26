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
 * ModuleName  : osal
 * File        : clPosixDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/

#include "clPosixDebug.h"

#ifdef CL_OSAL_DEBUG

#undef clLogCritical
#define clLogCritical(area, context, ...) do { printf("%s-%s: ", area, context); printf(__VA_ARGS__); printf("\n"); fflush(stdout); fflush(stdout); fflush(stdout); sleep(1); } while(0)

static CL_LIST_HEAD_DECLARE(gClMutexPoolList);
static ClUint32T gClMutexPoolCount;
static pthread_mutex_t gClMutexPoolMutex = PTHREAD_MUTEX_INITIALIZER;

void cosPosixMutexPoolAdd(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    if(!pMutex) return;
    pthread_mutex_lock(&gClMutexPoolMutex);
    pMutex->magic = CL_OSAL_DEBUG_MAGIC;
    pMutex->tid = 0;
    pMutex->creatorFile = file;
    pMutex->creatorLine = line;
    pMutex->ownerFile = NULL;
    pMutex->ownerLine = 0;
    pMutex->list.pNext = NULL;
    pMutex->list.pPrev = NULL;
    pthread_mutex_unlock(&gClMutexPoolMutex);
}

static ClRcT cosPosixMutexPoolLockInternal(ClOsalMutexT *pMutex, 
                                           const ClCharT *file, 
                                           ClInt32T line,
                                           ClBoolT grabLock)
{
    if(!pMutex) return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);

    pthread_mutex_lock(&gClMutexPoolMutex);
    /*
     * Check if the mutex is valid/created first.
     */
    if(pMutex->magic != CL_OSAL_DEBUG_MAGIC)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to lock an invalid mutex from file [%s], line [%d]", 
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }

    if(!pMutex->creatorFile)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to lock an uninitialized mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }
    /*
     * Now check if the mutex already has a owner within the same thread context.
     * Double lock or deadlock. 
     */
    if(grabLock)
    {
        if(pMutex->ownerFile) 
        {
            if(pthread_self() == pMutex->tid)
            {
                clLogCritical("OSAL", "DEBUG", "Deadlock detected for thread [%lld] while trying to lock an "
                              "already locked mutex. Mutex trying to be locked by file [%s], line [%d] "
                              "is already owned by file [%s], line [%d]", (ClUint64T)pMutex->tid, 
                              file, line, pMutex->ownerFile, pMutex->ownerLine);
                pthread_mutex_unlock(&gClMutexPoolMutex);
                CL_ASSERT(0);
                exit(1);
            }
        }
        pthread_mutex_unlock(&gClMutexPoolMutex);
        sysRetErrChkRet(pthread_mutex_lock(&pMutex->shared_lock.mutex));
        pthread_mutex_lock(&gClMutexPoolMutex);
    }

    pMutex->ownerFile = file;
    pMutex->ownerLine = line;
    pMutex->tid = pthread_self();
    /*
     * Add it to the mutex pool in the reverse order.
     */
    if(!pMutex->list.pNext)
    {
        clListAdd(&pMutex->list, &gClMutexPoolList);
        ++gClMutexPoolCount;
    }
    pthread_mutex_unlock(&gClMutexPoolMutex);
    return CL_OK;
}

ClRcT cosPosixMutexPoolLock(ClOsalMutexT *pMutex, 
                            const ClCharT *file, 
                            ClInt32T line)
{
    return cosPosixMutexPoolLockInternal(pMutex, file, line, CL_TRUE);
}

ClRcT cosPosixMutexPoolLockSoft(ClOsalMutexT *pMutex, 
                                const ClCharT *file, 
                                ClInt32T line)
{
    return cosPosixMutexPoolLockInternal(pMutex, file, line, CL_FALSE);
}

void cosPosixMutexPoolUnlock(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    if(!pMutex) return;

    pthread_mutex_lock(&gClMutexPoolMutex);
    if(pMutex->magic != CL_OSAL_DEBUG_MAGIC)
    {
        clLogCritical("OSAL","DEBUG", "Trying to unlock an invalid mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }
    if(!pMutex->creatorFile)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to unlock an uninitialized mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }
    if(!pMutex->ownerFile)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to unlock an already unlocked mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }

    pMutex->ownerFile = NULL;
    pMutex->ownerLine = 0;
    pMutex->tid = 0;
    /*
     * Unlink the mutex from the pool.
     */
    if(pMutex->list.pNext)
    {
        clListDel(&pMutex->list);
        pMutex->list.pNext = NULL;
        pMutex->list.pPrev = NULL;
        --gClMutexPoolCount;
    }

    pthread_mutex_unlock(&gClMutexPoolMutex);

}

void cosPosixMutexPoolDelete(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    if(!pMutex) return;
    pthread_mutex_lock(&gClMutexPoolMutex);
    if(pMutex->magic != CL_OSAL_DEBUG_MAGIC)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to delete an invalid mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }
    if(!pMutex->creatorFile)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to delete an uninitialized mutex from file [%s], line [%d]",
                      file, line);
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }

    if(pMutex->ownerFile)
    {
        clLogCritical("OSAL", "DEBUG", "Trying to delete an already locked mutex at file [%s], line [%d], tid [%lld] "
                      "from file [%s], line [%d], tid [%lld]\n",
                      pMutex->ownerFile, pMutex->ownerLine, (ClUint64T)pMutex->tid,
                      file, line, (ClUint64T)pthread_self());
        pthread_mutex_unlock(&gClMutexPoolMutex);
        CL_ASSERT(0);
        exit(1);
    }

    pMutex->magic = 0;
    pMutex->creatorFile = NULL;
    pMutex->creatorLine = 0;
    pthread_mutex_unlock(&gClMutexPoolMutex);
    return;
}

/*
 * Just unlock all the locks in the mutex pool
 */
void cosPosixMutexPoolBustLocks(void)
{
    pthread_mutex_lock(&gClMutexPoolMutex);
    while(!CL_LIST_HEAD_EMPTY(&gClMutexPoolList))
    {
        ClListHeadT *head = gClMutexPoolList.pNext;
        ClOsalMutexT *pMutex = CL_LIST_ENTRY(head, ClOsalMutexT, list);
        CL_ASSERT(pMutex->magic == CL_OSAL_DEBUG_MAGIC);
        CL_ASSERT(pMutex->creatorFile != NULL);
        CL_ASSERT(pMutex->ownerFile != NULL);
        clListDel(&pMutex->list);
        pthread_mutex_unlock(&pMutex->shared_lock.mutex);
        pMutex->ownerFile = NULL;
        pMutex->ownerLine = 0;
    }
    gClMutexPoolCount = 0;
    pthread_mutex_unlock(&gClMutexPoolMutex);
}

#endif
