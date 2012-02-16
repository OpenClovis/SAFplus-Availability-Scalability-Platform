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
/*Thread mgmt. interface*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <clThreadPool.h>
#include <clEoQueueStats.h>

#define CL_THREAD_POOL_UNUSED_THREADS (0x20)
#define CL_THREAD_POOL_UNUSED_IDLE_TIME {.tsSec=0,.tsMilliSec=500}
#define CL_THREAD_POOL_JOB_IDLE_TIME CL_THREAD_POOL_UNUSED_IDLE_TIME
#define CL_THREAD_POOL_DELAY (100)
#define DEFAULT_THREADS_NORMAL (0x10)
#define DEFAULT_THREADS_EXCLUSIVE (1)

#if 0
static ClEoQueueT gClThreadPoolUnused = {                           \
    .queue = CL_JOB_LIST_INITIALIZER(gClThreadPoolUnused.queue),    \
};

static ClInt32T gClThreadPoolDefaultThreads[CL_THREAD_POOL_MODE_MAX] = {
    [CL_THREAD_POOL_MODE_NORMAL] = DEFAULT_THREADS_NORMAL,
    [CL_THREAD_POOL_MODE_EXCLUSIVE] = DEFAULT_THREADS_EXCLUSIVE,
};

typedef struct ClThreadPoolCtrl
{
    ClBoolT initialized;
    ClInt32T maxUnusedThreads;
    ClInt32T unusedThreads;
    ClOsalMutexT unusedThreadsMutex ;
    ClTimerTimeOutT idleTime;
    ClTimerTimeOutT jobIdleTime;
    ClBoolT unusedPoolExit;
} ClThreadPoolCtrlT;

static ClThreadPoolCtrlT gClThreadPoolCtrl =  {
    .initialized = CL_FALSE,
    .maxUnusedThreads = CL_THREAD_POOL_UNUSED_THREADS,
    .idleTime = CL_THREAD_POOL_UNUSED_IDLE_TIME,
    .jobIdleTime = CL_THREAD_POOL_JOB_IDLE_TIME,
    .unusedPoolExit = CL_FALSE,
};


ClRcT clThreadPoolInitialize(void)
{
    ClRcT rc = CL_OK;
    if(gClThreadPoolCtrl.initialized == CL_TRUE)
        return rc;
    rc = clOsalMutexInit(&gClThreadPoolUnused.mutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&gClThreadPoolUnused.cond);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClThreadPoolCtrl.unusedThreadsMutex);
    CL_ASSERT(rc == CL_OK);
    gClThreadPoolCtrl.initialized = CL_TRUE;
    return rc;
}

/*
 * Dequeue a pool from the unused pool list.
 */
static ClThreadPoolT *clThreadPoolDequeuePool(ClThreadPoolT *pThreadPool)
{
    ClThreadPoolT *pPool = NULL;
    ClTimerTimeOutT unusedIdleTimeOut = gClThreadPoolCtrl.idleTime;
    ClRcT rc = CL_OK;
    ClBoolT unusedUpdate = CL_FALSE;

    CL_EO_QUEUE_LOCK(&gClThreadPoolUnused.mutex);

    if(CL_JOB_LIST_EMPTY(&gClThreadPoolUnused.queue))
    {
        CL_EO_QUEUE_LOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
        if(gClThreadPoolCtrl.unusedThreads >= gClThreadPoolCtrl.maxUnusedThreads
           && 
           gClThreadPoolCtrl.maxUnusedThreads != -1)
        {
            CL_EO_QUEUE_UNLOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Max unused threads limit reached(%d).\n",gClThreadPoolCtrl.maxUnusedThreads));
            goto out_unlock;
        }
        ++gClThreadPoolCtrl.unusedThreads;
        unusedUpdate=CL_TRUE;
        CL_EO_QUEUE_UNLOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
    
        ++gClThreadPoolUnused.numThreadsWaiting;
        while(gClThreadPoolCtrl.unusedPoolExit == CL_FALSE 
              && 
              CL_JOB_LIST_EMPTY(&gClThreadPoolUnused.queue)
              )
        {
            rc = clOsalCondWait(&gClThreadPoolUnused.cond,
                                &gClThreadPoolUnused.mutex,
                                unusedIdleTimeOut);
            if(rc == CL_OK 
               && 
               unusedIdleTimeOut.tsSec == 0 
               && 
               unusedIdleTimeOut.tsMilliSec == 0)
            {
                continue;
            }

            --gClThreadPoolUnused.numThreadsWaiting;

            if(CL_JOB_LIST_EMPTY(&gClThreadPoolUnused.queue))
            {
                goto out_unlock;
            }

            goto out_dequeue;
        }
        --gClThreadPoolUnused.numThreadsWaiting;
    }

    out_dequeue:

    if(gClThreadPoolCtrl.unusedPoolExit == CL_FALSE)
    {
        CL_EO_QUEUE_POP(&gClThreadPoolUnused,ClThreadPoolT,pPool);
        CL_ASSERT(pPool != NULL);
    }

    out_unlock:
    CL_EO_QUEUE_UNLOCK(&gClThreadPoolUnused.mutex);

    if(unusedUpdate == CL_TRUE)
    {
        CL_EO_QUEUE_LOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
        --gClThreadPoolCtrl.unusedThreads;
        CL_EO_QUEUE_UNLOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
    }
    return pPool;
}


/*
 *Dequeue a job from the thread pool priority queue.
 *Called with queue lock held
*/

static ClEoJobT *clThreadPoolDequeueJob(ClThreadPoolT *pThreadPool)
{
    ClEoJobT *pJob = NULL;
    ClRcT rc = CL_OK;
    ClTimerTimeOutT jobIdleTimeOut = gClThreadPoolCtrl.jobIdleTime;

    if(CL_JOB_LIST_EMPTY(&pThreadPool->pQueue->queue))
    {
        /*
         *Since its a dedicated one, wait till a job arrives or threadpool
         is freed.
        */
        if(pThreadPool->mode == CL_THREAD_POOL_MODE_EXCLUSIVE)
            jobIdleTimeOut.tsSec = 0, jobIdleTimeOut.tsMilliSec = 0;

        ++pThreadPool->pQueue->numThreadsWaiting;
        while(
              CL_JOB_LIST_EMPTY(&pThreadPool->pQueue->queue)
              &&
              pThreadPool->running == CL_TRUE
              )
        {
            ClTimerTimeOutT timeout = jobIdleTimeOut;

            rc = clOsalCondWait(&pThreadPool->pQueue->cond,
                                &pThreadPool->pQueue->mutex,
                                timeout);
            if(rc == CL_OK
               &&
               jobIdleTimeOut.tsSec == 0
               &&
               jobIdleTimeOut.tsMilliSec == 0
               )
                continue;

            --pThreadPool->pQueue->numThreadsWaiting;
            if(CL_JOB_LIST_EMPTY(&pThreadPool->pQueue->queue))
            {
                goto out;
            }
            goto out_dequeue;
        }
        --pThreadPool->pQueue->numThreadsWaiting;
    }

    out_dequeue:

    if(pThreadPool->running == CL_TRUE)
    {
        CL_EO_QUEUE_POP(pThreadPool->pQueue,
                        ClEoJobT,
                        pJob);
        CL_ASSERT(pJob != NULL);
    }

    out:
    return pJob;
}

static void *clThreadPoolProxyThread(void *pArg)
{
    ClThreadPoolT *pThreadPool = (ClThreadPoolT*)pArg;
    
    CL_ASSERT(pThreadPool->pQueue != NULL);

    CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
    while(pThreadPool->running == CL_TRUE)
    {
        ClEoJobT *pJob = clThreadPoolDequeueJob(pThreadPool);
        if(pJob)
        {
            ClRcT rc = CL_OK;
            ClUint32T queueSize = pThreadPool->pQueue->numElements + 1; 
            ClEoQueueStatsT queueStats = {0};
            ++pThreadPool->pQueue->refCnt;
            CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
            CL_ASSERT(pThreadPool->pJobHandler != NULL);

            clEoQueueStatsStart(queueSize, pJob, &queueStats);
            rc = pThreadPool->pJobHandler(pJob, pThreadPool->pUserData);
            clEoQueueStatsStop(queueSize, pJob, &queueStats);

            clHeapFree(pJob);
            pJob = NULL;
            CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
            --pThreadPool->pQueue->refCnt;
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Error processing job.rc=0x%x\n",rc));
            }
        }
        else
        {
            if(pThreadPool->running == CL_FALSE)
            {
                break;
            }
            /*
             *Didnt find a job within the specified time.
             *Wait for a new pool request to reuse this thread.
             */
            --pThreadPool->numThreads;
            CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
            pThreadPool = clThreadPoolDequeuePool(pThreadPool);
            if(pThreadPool == NULL)
            {
                break;
            }
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Thread reused for pool %p\n", (ClPtrT)pThreadPool));
            CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
        }
    }
    if(pThreadPool)
    {
        --pThreadPool->numThreads;
        CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
    }
    return NULL;
}

static ClRcT clThreadPoolThreadSpawn(ClThreadPoolT *pThreadPool, ClOsalTaskIdT *pId)
{
    ClRcT rc = CL_OK;
    
    if(!pId)
    {
        return CL_THREADPOOL_RC(CL_ERR_INVALID_PARAMETER);
    }
   
#ifdef SOLARIS_BUILD
    rc = clOsalTaskCreateAttached("ProxyThread", CL_OSAL_SCHED_OTHER,
                                  CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
                                  0, clThreadPoolProxyThread,
                                  (ClPtrT)pThreadPool, pId);
#else
    rc = clOsalTaskCreateAttached("ProxyThread", CL_OSAL_SCHED_OTHER, 
                                  pThreadPool->priority,
                                  0, clThreadPoolProxyThread, 
                                  (ClPtrT)pThreadPool, pId);
#endif

    if(rc != CL_OK )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error spawning thread.errno=%d\n",errno));
        goto out;
    }
    
    clOsalTaskDetach(*pId);

    out:
    return rc;
}

/*Called with thread pool queue mutex held*/
ClRcT clThreadPoolStart(ClThreadPoolT *pThreadPool, ClOsalTaskIdT *pId)
{
    ClRcT rc = CL_THREADPOOL_RC(CL_ERR_OP_NOT_PERMITTED);

    /*
     * Check if people are waiting on the unused threadpool queue:
     * If yes, add this pool to the unused list 
     */
    CL_EO_QUEUE_LOCK(&gClThreadPoolUnused.mutex);
    if(CL_EO_QUEUE_LEN(&gClThreadPoolUnused) < 0 )
    {
        /*Waiting threads on the unused queue*/
        CL_EO_QUEUE_ADD(&gClThreadPoolUnused,pThreadPool);
        clOsalCondSignal(&gClThreadPoolUnused.cond);
        CL_EO_QUEUE_UNLOCK(&gClThreadPoolUnused.mutex);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Adding pool %p into unused idle pool\n", (ClPtrT)pThreadPool));
        rc = CL_OK;
        goto out_update;
    }
    CL_EO_QUEUE_UNLOCK(&gClThreadPoolUnused.mutex);

    if(pThreadPool->numThreads >= pThreadPool->maxThreads 
       && 
       pThreadPool->maxThreads != -1)
    {
        ClInt32T delay = CL_THREAD_POOL_DELAY;
        CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
        /*Give a second chance to see if some existing ones drain out*/
        CL_EO_DELAY(delay);
        CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
        if(pThreadPool->numThreads >= pThreadPool->maxThreads)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Thread pool %p,max thread(%d) limit reached\n", (ClPtrT)pThreadPool,pThreadPool->maxThreads));
            goto out;
        }
    }

    /*We spawn a thread for the pool*/
    rc = clThreadPoolThreadSpawn(pThreadPool,pId);
    if(rc!=CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error spawning thread.rc=0x%x\n",rc));
        goto out;
    }
    out_update:
    ++pThreadPool->numThreads;
    out:
    return rc;
}

ClRcT clThreadPoolCreate(ClThreadPoolHandleT *pHandle,
                         ClEoQueueHandleT queueHandle,
                         ClThreadPoolModeT mode,
                         ClInt32T maxThreads,
                         ClInt32T priority,
                         ClEoJobCallBackT pJobHandler,
                         ClPtrT pUserData
                         )
{
    ClThreadPoolT *pThreadPool=NULL;
    ClEoQueueT *pQueue = (ClEoQueueT*)queueHandle;
    ClRcT rc = CL_THREADPOOL_RC(CL_ERR_NOT_INITIALIZED);
    ClOsalTaskIdT *pIds = NULL;

    if(gClThreadPoolCtrl.initialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Thread pool uninitialized\n"));
        goto out;
    }
    rc = CL_THREADPOOL_RC(CL_ERR_INVALID_PARAMETER);
    if(pJobHandler == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid parameter\n"));
        goto out;
    }

    if(mode < CL_THREAD_POOL_MODE_NORMAL 
       ||
       mode >= CL_THREAD_POOL_MODE_MAX
       )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid mode:%d\n",mode));
        goto out;
    }

    /*
     * Infinite limit doesnt make sense in EXCLUSIVE MODE.
     */
    if(mode == CL_THREAD_POOL_MODE_EXCLUSIVE && maxThreads == -1)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Max threads cannot be infinite in "\
                                        "EXCLUSIVE mode\n"));
        goto out;
    }

    /*
     * If maxThreads is specified as 0, use defaults
     */

    if(!maxThreads) 
    {
        maxThreads = gClThreadPoolDefaultThreads[mode];
    }

    rc = CL_THREADPOOL_RC(CL_ERR_NO_MEMORY);
    if(mode == CL_THREAD_POOL_MODE_EXCLUSIVE && maxThreads != -1)
    {
        pIds = clHeapCalloc(maxThreads, sizeof(ClOsalTaskIdT));
        if(pIds == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
            goto out;
        }
    }

    pThreadPool = clHeapCalloc(1, sizeof(*pThreadPool));
    if(pThreadPool == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out_free2;
    }
    
    pThreadPool->userQueue = CL_TRUE;
    if(pQueue==NULL)
    {
        pQueue = clHeapCalloc(1,sizeof(*pQueue));
        if(pQueue == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
            goto out_free;
        }
        rc = clOsalMutexInit(&pQueue->mutex);
        CL_ASSERT(rc == CL_OK);
        rc = clOsalCondInit(&pQueue->cond);
        CL_ASSERT(rc == CL_OK);
        CL_JOB_LIST_INIT(&pQueue->queue);
        pQueue->pThreadPool = pThreadPool;
        pThreadPool->userQueue = CL_FALSE;
    }
    pThreadPool->pQueue = pQueue;
    pThreadPool->mode = mode;
    pThreadPool->maxThreads = maxThreads;
    pThreadPool->pQueue = pQueue;
    pThreadPool->priority = priority;
    pThreadPool->running = CL_TRUE;
    pThreadPool->pJobHandler = pJobHandler;
    pThreadPool->pUserData = pUserData;
    
    if(pIds)
    {
        register ClInt32T i = 0;
        CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
        while(i < pThreadPool->maxThreads)
        {
            rc = clThreadPoolStart(pThreadPool,pIds+i);
            if(rc != CL_OK)
            {
                register ClInt32T j;
                for(j = 0; j < i; ++j)
                {
                    if(pIds[j])
                    {
                        /*
                         * Just rip them off. Dont care for graceful joins
                         */
                        clOsalTaskKill(pIds[j], SIGKILL);
                    }
                }
                CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
                goto out_free;
            }
            ++i;
        }
        CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
    }
    *pHandle = (ClThreadPoolHandleT) pThreadPool;
    rc = CL_OK;
    goto out_free2;

    out_free:
    if(pThreadPool)
    {
        if(pThreadPool->userQueue == CL_FALSE)
        {
            clHeapFree(pThreadPool->pQueue);
        }
        clHeapFree(pThreadPool);
    }
    out_free2:
    if(pIds)
    {
        clHeapFree(pIds);
    }
    out:
    return rc;
}

/*
 * Push a job into the threadpool
 */
ClRcT clThreadPoolPush(ClThreadPoolHandleT handle,
                       ClEoJobT *pJob
                       )
{
    ClThreadPoolT *pThreadPool = (ClThreadPoolT*)handle;
    ClRcT rc = CL_THREADPOOL_RC(CL_ERR_NOT_INITIALIZED);
    
    if(gClThreadPoolCtrl.initialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Threadpool uninitialized\n"));
        goto out;
    }
    
    rc = CL_THREADPOOL_RC(CL_ERR_INVALID_PARAMETER);

    if(pThreadPool == NULL || pJob == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid param\n"));
        goto out;
    }

    CL_ASSERT(pThreadPool->pQueue != NULL);

    CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
    
    if(pThreadPool->running == CL_FALSE)
    {
        CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
        rc = CL_THREADPOOL_RC(CL_ERR_OP_NOT_PERMITTED);
        goto out;
    }

    if(CL_EO_QUEUE_LEN(pThreadPool->pQueue) >= 0 
       && 
       pThreadPool->mode == CL_THREAD_POOL_MODE_NORMAL )
    {
        ClOsalTaskIdT id;
        /*Spawn a thread to handle the job*/
        rc = clThreadPoolStart(pThreadPool,&id);
        if(rc != CL_OK)
        { 
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Error starting thread.rc=0x%x\n",rc));
            goto out_unlock;
        }
    }
    /*Queue the job and signal on the mutex*/
    CL_EO_QUEUE_ADD(pThreadPool->pQueue, pJob);

    if(pThreadPool->pQueue->numThreadsWaiting > 0 )
    {
        clOsalCondSignal(&pThreadPool->pQueue->cond);
    }
    rc = CL_OK;

    out_unlock:
    CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
    out:
    return rc;
}

ClRcT clThreadPoolDeleteSync(ClThreadPoolHandleT handle, ClBoolT force)
{
    ClThreadPoolT *pThreadPool = (ClThreadPoolT*)handle;
    ClEoQueueT *pQueue = NULL;
    ClInt32T delay = 500;
    ClRcT rc= CL_OK;
    ClInt32T threadLimit = 0;
    ClInt32T refCntLimit = 0;

    if(gClThreadPoolCtrl.initialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Threadpool uninitialized\n"));
        return CL_THREADPOOL_RC(CL_ERR_NOT_INITIALIZED);
    }

    if(force == CL_TRUE)
    {
        /*
         * We could be in delete from a thread context itself.
         */
        threadLimit = 1;
        refCntLimit = 1;
    }

    CL_ASSERT(pThreadPool->pQueue != NULL);

    CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);

    pThreadPool->running = CL_FALSE;

    if(pThreadPool->numThreads)
    {
        clOsalCondBroadcast(&pThreadPool->pQueue->cond);
        /*
         * Faking a thread join here since its been seen with certain
         * distros - namely windriver that pthread_join core-dumps on a join
         * of a thread id that doesnt exist which is actually a join bug.
         * So playing it safe. and moreover these are detached threads.
         */
        while(pThreadPool->numThreads > threadLimit 
              || 
              pThreadPool->pQueue->refCnt > refCntLimit)
        {
            CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
            CL_EO_DELAY(delay);
            CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
        }
        CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);
        CL_EO_DELAY(delay);
        CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
    }
    pThreadPool->pQueue->pThreadPool = NULL;
    pQueue = pThreadPool->pQueue;
    pThreadPool->pQueue = NULL;
    CL_EO_QUEUE_UNLOCK(&pQueue->mutex);
    if(pThreadPool->userQueue == CL_FALSE)
    {
        clHeapFree(pQueue);
    }
    clHeapFree(pThreadPool);
    return rc;
}

ClRcT clThreadPoolStop(ClThreadPoolHandleT handle)
{
    ClThreadPoolT *pThreadPool = (ClThreadPoolT*)handle;
    
    if(!pThreadPool)
    {
        return CL_THREADPOOL_RC(CL_ERR_INVALID_PARAMETER);
    }
    CL_ASSERT(pThreadPool->pQueue != NULL);

    CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
    pThreadPool->running = CL_FALSE;
    clOsalCondBroadcast(&pThreadPool->pQueue->cond);
    CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);

    return CL_OK;
}

ClRcT clThreadPoolResume(ClThreadPoolHandleT handle)
{
    ClThreadPoolT *pThreadPool = (ClThreadPoolT*)handle;

    if(!pThreadPool)
    {
        return CL_THREADPOOL_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_ASSERT(pThreadPool->pQueue != NULL);

    CL_EO_QUEUE_LOCK(&pThreadPool->pQueue->mutex);
    pThreadPool->running = CL_TRUE;
    CL_EO_QUEUE_UNLOCK(&pThreadPool->pQueue->mutex);

    return CL_OK;
}

ClRcT clThreadPoolDelete(ClThreadPoolHandleT handle)
{
    return clThreadPoolDeleteSync(handle, CL_TRUE);
}

/*Rip off guys in the global pool*/
ClRcT clThreadPoolFreeUnused(void)
{
    ClRcT rc = CL_OK;
    ClInt32T delay = 500;

    if(gClThreadPoolCtrl.initialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Threadpool uninitialized\n"));
        return CL_THREADPOOL_RC(CL_ERR_NOT_INITIALIZED);
    }

    CL_EO_QUEUE_LOCK(&gClThreadPoolUnused.mutex);
    CL_EO_QUEUE_LOCK(&gClThreadPoolCtrl.unusedThreadsMutex);

    if(gClThreadPoolCtrl.unusedThreads > 0)
    {
        gClThreadPoolCtrl.unusedPoolExit = CL_TRUE;
        clOsalCondBroadcast(&gClThreadPoolUnused.cond);
    }

    CL_EO_QUEUE_UNLOCK(&gClThreadPoolCtrl.unusedThreadsMutex);
    while( gClThreadPoolUnused.numThreadsWaiting > 0)
    {
        CL_EO_QUEUE_UNLOCK(&gClThreadPoolUnused.mutex);
        CL_EO_DELAY(delay);
        CL_EO_QUEUE_LOCK(&gClThreadPoolUnused.mutex);
    }
    CL_EO_QUEUE_UNLOCK(&gClThreadPoolUnused.mutex);
    return rc;
}

#endif
