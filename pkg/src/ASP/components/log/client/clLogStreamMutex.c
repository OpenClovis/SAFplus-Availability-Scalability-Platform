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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clLogApi.h>
#include <clLogErrors.h>
#include <clLogClientEo.h>
#include <clLogClientHandle.h>
#include <clLogClientStream.h>
#include <clLogServer.h>
#include <clDebugApi.h>
#include <clCksmApi.h>

/*
 * Dont let the log server flush more than these
 * many batches of max flush limits without pausing
 */
#define CL_LOG_SERVER_FLUSH_BATCH_MAX (10)

#define CL_LOG_SERVER_FLUSH_LIMIT (100)

#define CL_LOG_SERVER_STREAM_FLUSH_RATE(stream) \
    CL_MAX(2, CL_MIN( (stream)->shmSize/((stream)->recordSize?(stream)->recordSize:1)/CL_LOG_SERVER_FLUSH_LIMIT, CL_LOG_SERVER_FLUSH_BATCH_MAX))

extern ClOsalSharedMutexFlagsT gClLogMutexMode;

ClRcT clLogStreamMutexModeSet(ClOsalSharedMutexFlagsT mode)
{
    /*
     * No hybrid stuffs here.
     */
    if( (mode !=  CL_OSAL_SHARED_NORMAL) && (mode != CL_OSAL_SHARED_SYSV_SEM) && (mode != CL_OSAL_SHARED_POSIX_SEM))
    {
        mode = CL_OSAL_SHARED_SYSV_SEM;
    }
    gClLogMutexMode = mode;

    return CL_OK;
}

/*
 * Shared normal mutexes are initialized early by log
 * So this takes care of initializing other lock modes. 
*/
ClRcT clLogClientStreamSharedMutexInit(ClLogClntStreamDataT *pStreamData, ClStringT *pShmName)
{
    return CL_OK;
}

ClRcT clLogServerStreamSharedMutexInit(ClLogSvrStreamDataT *pStreamData, ClStringT *pShmName)
{
    return CL_OK;
}

#ifdef VXWORKS_BUILD

ClRcT clLogSharedSemGet(const ClCharT *pShmName, const ClCharT *pSuffix, ClOsalSemIdT *pSemId)
{
    ClCharT semName[CL_MAX_NAME_LENGTH];
    if(!pSemId || !pShmName || !pSuffix) return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    snprintf(semName, sizeof(semName)-1, "%s%s_%s", 
             pShmName[0] == '/' ? "":"/", pShmName, pSuffix);
    return clOsalSemIdGet((ClUint8T*)semName, pSemId);
}

#endif

ClRcT clLogClientStreamMutexLock(ClLogClntStreamDataT *pClntData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pClntData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL))
    {
#ifndef POSIX_BUILD
        rc = clOsalMutexLock_L(&pClntData->pStreamHeader->shmLock);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexLock_L(&pClntData->pStreamHeader->sharedSem);
#else
        if(!pClntData->sharedSemId && !pClntData->shmName.pValue)
            return rc;
        if(!pClntData->sharedSemId)
        {
            rc = clLogSharedSemGet(pClntData->shmName.pValue, "sharedSem", &pClntData->sharedSemId);
            if(rc != CL_OK) return rc;
            if(!pClntData->sharedSemId)
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemLock(pClntData->sharedSemId);
#endif
    }
    return rc;
}

/*
 * 
 * When using sems, we try throttling or governing the speed of the 
 * log flusher when woken up. This is done by restricting
 * the max batch flush frequency of the flusher beyond
 * which it would pause.
 * If we dont throttle the speed, then we could experience
 * SEMVMX - out of range issues when writer goes extremely fast
 * and the log server stream flusher goes very slow.
 * Or if the log flusher wakes up when the work measure or
 * pending wakeups represented by the number of unlocks
 * could be very high resulting in log server hogging CPU
 */

ClRcT clLogClientStreamSignalFlusher(ClLogClntStreamDataT *pClntData,
                                     ClOsalCondIdT cond
                                     )
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pClntData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL) )
    {
        if(cond)
            rc = clOsalCondSignal_L(cond);
    }
    else if (gClLogMutexMode & CL_OSAL_SHARED_SYSV_SEM) 
    {
#ifndef VXWORKS_BUILD
        ClInt32T flushRate = CL_LOG_SERVER_STREAM_FLUSH_RATE(pClntData->pStreamHeader);
        rc = clOsalMutexValueSet_L(&pClntData->pStreamHeader->flusherSem, flushRate);
#endif
    }
    else if (gClLogMutexMode & CL_OSAL_SHARED_POSIX_SEM)
    {
        ClInt32T value = 0;
#ifndef VXWORKS_BUILD
        rc = clOsalMutexValueGet(&pClntData->pStreamHeader->flusherSem, &value);
        if(rc == CL_OK)
        {
            if(value < CL_LOG_SERVER_FLUSH_BATCH_MAX) /*control log batch flushes within 10 sem_post*/
                clOsalMutexUnlock_L(&pClntData->pStreamHeader->flusherSem); 
        } 
        else 
            clOsalMutexUnlock_L(&pClntData->pStreamHeader->flusherSem);
#else
        if(!pClntData->flusherSemId && !pClntData->shmName.pValue)
            return rc;
        if(!pClntData->flusherSemId)
        {
            rc = clLogSharedSemGet(pClntData->shmName.pValue, "flusherSem", &pClntData->flusherSemId);
            if(rc != CL_OK) return rc;
            if(!pClntData->flusherSemId)
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemValueGet(pClntData->flusherSemId, &value);
        if(rc == CL_OK)
        {
            if(value < CL_LOG_SERVER_FLUSH_BATCH_MAX)
                clOsalSemUnlock(pClntData->flusherSemId);
        }
        else
            clOsalSemUnlock(pClntData->flusherSemId);
#endif
    }

    return rc;
}

ClRcT clLogClientStreamMutexUnlock(ClLogClntStreamDataT *pClntData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pClntData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL))
    {
#ifndef POSIX_BUILD
        rc = clOsalMutexUnlock_L(&pClntData->pStreamHeader->shmLock);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexUnlock_L(&pClntData->pStreamHeader->sharedSem);
#else
        if(!pClntData->sharedSemId && !pClntData->shmName.pValue)
            return rc;
        if(!pClntData->sharedSemId)
        {
            rc = clLogSharedSemGet(pClntData->shmName.pValue, "sharedSem", &pClntData->sharedSemId);
            if(rc != CL_OK)
                return rc;
            if(!pClntData->sharedSemId) 
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemUnlock(pClntData->sharedSemId);
#endif
    }
    return rc;
}

ClRcT clLogServerStreamMutexLock(ClLogSvrStreamDataT *pSvrData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL))
    {
#ifndef POSIX_BUILD
        rc = clOsalMutexLock_L(&pSvrData->pStreamHeader->shmLock);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexLock_L(&pSvrData->pStreamHeader->sharedSem);
#else
        if(!pSvrData->sharedSemId && !pSvrData->shmName.pValue)
            return rc;
        if(!pSvrData->sharedSemId)
        {
            rc = clLogSharedSemGet(pSvrData->shmName.pValue, "sharedSem", &pSvrData->sharedSemId);
            if(rc != CL_OK)
                return rc;
            if(!pSvrData->sharedSemId)
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemLock(pSvrData->sharedSemId);
#endif
    }
    return rc;
}

ClRcT clLogServerStreamMutexLockFlusher(ClLogSvrStreamDataT *pSvrData)
{
    return clLogServerStreamMutexLock(pSvrData);
}

ClRcT clLogServerStreamMutexUnlock(ClLogSvrStreamDataT *pSvrData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL) )
    {
#ifndef POSIX_BUILD
        rc = clOsalMutexUnlock_L(&pSvrData->pStreamHeader->shmLock);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexUnlock_L(&pSvrData->pStreamHeader->sharedSem);
#else
        if(!pSvrData->sharedSemId && !pSvrData->shmName.pValue)
            return rc;
        if(!pSvrData->sharedSemId)
        {
            rc = clLogSharedSemGet(pSvrData->shmName.pValue, "sharedSem", &pSvrData->sharedSemId);
            if(rc != CL_OK)
                return rc;
            if(!pSvrData->sharedSemId)
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemUnlock(pSvrData->sharedSemId);
#endif
    }
    return rc;
}

ClRcT clLogServerStreamCondWait(ClLogSvrStreamDataT *pSvrData,
                                ClOsalCondIdT cond,
                                ClTimerTimeOutT timeout
                                )
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL))
    {
#ifndef POSIX_BUILD
        rc = clOsalCondWait_L(cond,&pSvrData->pStreamHeader->shmLock,timeout);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexUnlock_L(&pSvrData->pStreamHeader->sharedSem);
        rc |= clOsalMutexLock_L(&pSvrData->pStreamHeader->flusherSem);
        rc |= clOsalMutexLock_L(&pSvrData->pStreamHeader->sharedSem);
#else
        if(!pSvrData->sharedSemId && !pSvrData->shmName.pValue)
            return rc;
        if(!pSvrData->flusherSemId && !pSvrData->shmName.pValue)
            return rc;
        if(!pSvrData->sharedSemId)
        {
            rc = clLogSharedSemGet(pSvrData->shmName.pValue, "sharedSem", &pSvrData->sharedSemId);
        }
        if(!pSvrData->flusherSemId)
        {
            rc = clLogSharedSemGet(pSvrData->shmName.pValue, "flusherSem", &pSvrData->flusherSemId);
        }
        if(!pSvrData->sharedSemId || !pSvrData->flusherSemId)
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        rc =  clOsalSemUnlock(pSvrData->sharedSemId);
        rc |= clOsalSemLock(pSvrData->flusherSemId);
        rc |= clOsalSemLock(pSvrData->sharedSemId);
#endif
    }
    return rc;
}

ClRcT clLogServerStreamSignalFlusher(ClLogSvrStreamDataT *pSvrData,
                                     ClOsalCondIdT cond)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData)
    {
        return rc;
    }
    if( (gClLogMutexMode & CL_OSAL_SHARED_NORMAL) )
    {
        if(cond) 
            rc = clOsalCondSignal_L(cond);
    }
    else if (gClLogMutexMode & CL_OSAL_SHARED_POSIX_SEM)
    {
        ClInt32T value = 0;
#ifndef VXWORKS_BUILD
        rc = clOsalMutexValueGet(&pSvrData->pStreamHeader->flusherSem, &value);
        if(rc == CL_OK)
        {
            if(value < CL_LOG_SERVER_FLUSH_BATCH_MAX) /*control log batch flushes within 10 sem_post*/
                clOsalMutexUnlock_L(&pSvrData->pStreamHeader->flusherSem); 
        } 
        else 
            rc = clOsalMutexUnlock_L(&pSvrData->pStreamHeader->flusherSem);
#else
        if(!pSvrData->flusherSemId && !pSvrData->shmName.pValue)
            return rc;
        if(!pSvrData->flusherSemId)
        {
            rc = clLogSharedSemGet(pSvrData->shmName.pValue, "flusherSem", &pSvrData->flusherSemId);
            if(rc != CL_OK)
                return rc;
            if(!pSvrData->flusherSemId)
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        rc = clOsalSemValueGet(pSvrData->flusherSemId, &value);
        if(rc == CL_OK)
        {
            if(value < CL_LOG_SERVER_FLUSH_BATCH_MAX)
                clOsalSemUnlock(pSvrData->flusherSemId);
        }
        else
            rc = clOsalSemUnlock(pSvrData->flusherSemId);
#endif
    }
    else
    {
#ifndef VXWORKS_BUILD
        rc = clOsalMutexUnlock_L(&pSvrData->pStreamHeader->flusherSem);
#endif
    }

    return rc;
}

ClRcT clLogClientStreamMutexDestroy(ClLogClntStreamDataT *pClntData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pClntData || !pClntData->pStreamHeader) return rc;
#ifndef VXWORKS_BUILD
    return clOsalMutexDestroy(&pClntData->pStreamHeader->sharedSem);
#else
    if(!pClntData->sharedSemId && !pClntData->shmName.pValue)
        return rc;
    if(!pClntData->sharedSemId)
    {
        rc = clLogSharedSemGet(pClntData->shmName.pValue, "sharedSem", &pClntData->sharedSemId);
        if(rc != CL_OK) return rc;
        if(!pClntData->sharedSemId)
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    rc = clOsalSemDelete(pClntData->sharedSemId);
    pClntData->sharedSemId = 0;
    return rc;
#endif
}

ClRcT clLogClientStreamFlusherMutexDestroy(ClLogClntStreamDataT *pClntData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pClntData || !pClntData->pStreamHeader) return rc;
#ifndef VXWORKS_BUILD
    return clOsalMutexDestroy(&pClntData->pStreamHeader->flusherSem);
#else
    if(!pClntData->flusherSemId && !pClntData->shmName.pValue)
        return rc;
    if(!pClntData->flusherSemId)
    {
        rc = clLogSharedSemGet(pClntData->shmName.pValue, "flusherSem", &pClntData->flusherSemId);
        if(rc != CL_OK) return rc;
        if(!pClntData->flusherSemId)
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    rc = clOsalSemDelete(pClntData->flusherSemId);
    pClntData->flusherSemId = 0;
    return rc;
#endif
}

ClRcT clLogServerStreamMutexDestroy(ClLogSvrStreamDataT *pSvrData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData || !pSvrData->pStreamHeader) return rc;
#ifndef VXWORKS_BUILD
    return clOsalMutexDestroy(&pSvrData->pStreamHeader->sharedSem);
#else
    if(!pSvrData->sharedSemId && !pSvrData->shmName.pValue)
        return rc;
    if(!pSvrData->sharedSemId)
    {
        rc = clLogSharedSemGet(pSvrData->shmName.pValue, "sharedSem", &pSvrData->sharedSemId);
        if(rc != CL_OK) return rc;
        if(!pSvrData->sharedSemId)
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    rc = clOsalSemDelete(pSvrData->sharedSemId);
    pSvrData->sharedSemId = 0;
    return rc;
#endif
}

ClRcT clLogServerStreamFlusherMutexDestroy(ClLogSvrStreamDataT *pSvrData)
{
    ClRcT rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    if(!pSvrData) return rc;
#ifndef VXWORKS_BUILD
    return clOsalMutexDestroy(&pSvrData->pStreamHeader->flusherSem);
#else
    if(!pSvrData->flusherSemId && !pSvrData->shmName.pValue)
        return rc;
    if(!pSvrData->flusherSemId)
    {
        rc = clLogSharedSemGet(pSvrData->shmName.pValue, "flusherSem", &pSvrData->flusherSemId);
        if(rc != CL_OK) return rc;
        if(!pSvrData->flusherSemId)
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    rc = clOsalSemDelete(pSvrData->flusherSemId);
    pSvrData->flusherSemId = 0;
    return rc;
#endif
}

/*
 * This thread will be created only the lockMode is sem. Through this we are
 * acheiving flushInterval facility.
 */
ClLogLockModeT 
clLogLockModeGet(void)
{
    if( (gClLogMutexMode & CL_OSAL_SHARED_SYSV_SEM) || (gClLogMutexMode & CL_OSAL_SHARED_POSIX_SEM) )
    {
        /* its semaphore */
        return CL_LOG_SEM_MODE;
    }
    return CL_LOG_MUTEX_MODE;
}
