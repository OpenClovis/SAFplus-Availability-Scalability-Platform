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
/*
 * CLOVIS EO request handling prototype
*/
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <clHeapApi.h>
#include <clEoQueue.h>
#include <clThreadPool.h>
#include <clEoQueueStats.h>

static ClOsalMutexT gClEoQueueMutex;
static ClEoQueueT *gpClEoQueues[CL_IOC_MAX_PRIORITIES];

static ClInt32T gClEoThreadPriorityMap[CL_IOC_MAX_PRIORITIES];
extern ClRcT clEoJobHandler(ClEoJobT *pJob, ClPtrT pUserData);

#if defined(EO_QUEUE_STATS)

static ClEoQueueStatsT gClEoQueueStats[CL_IOC_MAX_PRIORITIES][CL_IOC_NUM_PROTOS];
static ClOsalMutexT gClEoQueueStatsMutex;
static ClBoolT gClEoQueueStatsMutexInitialized = CL_FALSE;

static const ClCharT *clEoProtoName(ClUint32T proto)
{
    static const ClCharT *const protos[] = { "SYNC_REQUEST","SYNC_REPLY","ASYNC_REQUEST","ASYNC_REPLY","NOTIFICATION","SYSLOG","ACK","ORDERED" };
    static ClUint32T numProtos = (ClUint32T) sizeof(protos)/sizeof(protos[0]);
    static ClUint32T protoBase = CL_IOC_INTERNAL_PROTO_END+1;
    ClInt32T protoType = proto - protoBase;
    if(protoType < 0 || protoType >= protoBase+numProtos)
        return "UNKNOWN";
    return protos[protoType];
}

static void clEoQueueStatsUpdate(ClEoQueueStatsT *pStats)
{
    ClUint8T proto = pStats->proto;
    ClUint8T priority = pStats->priority;
    ClEoQueueStatsT *pStatsGlobal = NULL;
    /* 
     * Cant do this in eoProtoInit as its invoked
     * before osal init.
     */
    if( gClEoQueueStatsMutexInitialized == CL_FALSE )
    {
        ClRcT rc = CL_OK;
        gClEoQueueStatsMutexInitialized = CL_TRUE;
        rc = clOsalMutexInit(&gClEoQueueStatsMutex);
        CL_ASSERT(rc == CL_OK);
    }

    CL_EO_QUEUE_LOCK(&gClEoQueueStatsMutex);

    pStatsGlobal = &gClEoQueueStats[priority][proto];
    pStatsGlobal->used = CL_TRUE;
    pStatsGlobal->totalQueueSize += pStats->totalQueueSize;
    ++pStatsGlobal->count;

    if(!pStatsGlobal->minQueueSize) pStatsGlobal->minQueueSize = pStats->totalQueueSize;
    else if(pStats->totalQueueSize < pStatsGlobal->minQueueSize) pStatsGlobal->minQueueSize = pStats->totalQueueSize;

    if(!pStatsGlobal->maxQueueSize) pStatsGlobal->maxQueueSize = pStats->totalQueueSize;
    else if(pStats->totalQueueSize > pStatsGlobal->maxQueueSize) pStatsGlobal->maxQueueSize = pStats->totalQueueSize;

    pStatsGlobal->totalTime += pStats->totalTime;
    if(!pStatsGlobal->minTime) pStatsGlobal->minTime = pStats->totalTime;
    else if(pStats->totalTime < pStatsGlobal->minTime) pStatsGlobal->minTime = pStats->totalTime;
    
    if(!pStatsGlobal->maxTime) pStatsGlobal->maxTime = pStats->totalTime;
    else if(pStats->totalTime > pStatsGlobal->maxTime) pStatsGlobal->maxTime = pStats->totalTime;

    CL_EO_QUEUE_UNLOCK(&gClEoQueueStatsMutex);

    memset(pStats, 0, sizeof(*pStats));
}

static void clEoQueueStatsDump(void)
{
    register ClInt32T i,j;
    ClCharT *pBuffer = NULL;
    ClUint32T bytes = 0;
    ClUint32T totalBytes=0;
    ClUint32T nbytes=0;
    ClInt32T remaining = 0;

    for(i = 0; i < CL_IOC_MAX_PRIORITIES;++i)
    {
        for(j = 0; j < CL_IOC_NUM_PROTOS;++j)
        {
            ClEoQueueStatsT *pStats = &gClEoQueueStats[i][j];
            ClUint64T avgTime = 0;
            ClUint32T avgQueueSize=0;
            if(pStats->used == CL_FALSE) continue;
            if(!remaining)
            {
                remaining = 1024;
                totalBytes = bytes+remaining;
                totalBytes &= ~1023;
                pBuffer = realloc(pBuffer,totalBytes);
                if(!pBuffer) return;
            }
            avgTime = pStats->totalTime/(pStats->count?pStats->count:1);
            avgQueueSize= pStats->totalQueueSize/(pStats->count ? pStats->count:1);
            nbytes = snprintf(pBuffer+bytes,totalBytes-bytes,
                    "Pri:%d, Proto:%s, Count:%d, MinSize:%d, MaxSize:%d, AvgSize:%d, MinTime:%lld, MaxTime:%lld, AvgTime:%lld\n", i,clEoProtoName((ClUint32T)j),pStats->count,pStats->minQueueSize,pStats->maxQueueSize,avgQueueSize,pStats->minTime,pStats->maxTime,avgTime);
            bytes += nbytes;
            remaining -= nbytes;
            if(remaining < nbytes )
                remaining = 0;
        }
    }
    if(pBuffer)
    {
        ClCharT filename[0xff+1];
        ClCharT *aspLogDir = getenv("ASP_LOGDIR");
        ClIocNodeAddressT nodeAddress = clIocLocalAddressGet();
        snprintf(filename,
                 sizeof(filename),
                 "%s/%s_queue_stats_%d.txt",
                 !aspLogDir ? "/tmp" : aspLogDir,
                 CL_EO_NAME,
                 nodeAddress);
        int fd = open(filename, O_CREAT|O_RDWR|O_APPEND,0666);
        if(fd < 0 )
        {
            perror("open:");
            free(pBuffer);
            return;
        }
        if(write(fd,pBuffer,bytes) != bytes)
        {
            perror("Write:");
        }
        else
        {
            printf(" EO Queue Stats written to %s\n",filename);
        }
        free(pBuffer);
    }
}

static void sigusr2_handler(int sig)
{
    clEoQueueStatsDump();
}

void clEoQueueStatsStart(ClUint32T queueSize, ClEoJobT *pJob, ClEoQueueStatsT *pStats)
{
    ClUint8T proto = pJob->msgParam.protoType;
    ClUint8T priority = CL_EO_RECV_QUEUE_PRI(pJob->msgParam);
    pStats->priority = priority;
    pStats->proto = proto;
    pStats->totalQueueSize = queueSize;
    gettimeofday(&pStats->start,NULL);
}

void clEoQueueStatsStop(ClUint32T queueSize, ClEoJobT *pJob, ClEoQueueStatsT *pStats)
{
    ClUint8T proto = pJob->msgParam.protoType;
    ClUint8T priority = CL_EO_RECV_QUEUE_PRI(pJob->msgParam);
    ClUint64T timeDiff=0;
    gettimeofday(&pStats->end, NULL);
    CL_ASSERT(pStats->priority == priority);
    CL_ASSERT(pStats->proto == proto);
    pStats->end.tv_sec -= pStats->start.tv_sec;
    pStats->end.tv_usec -= pStats->start.tv_usec;
    if(pStats->end.tv_sec < 0)
        pStats->end.tv_sec = 0, pStats->end.tv_usec = 0;
    else if(pStats->end.tv_usec < 0)
    {
        --pStats->end.tv_sec;
        pStats->end.tv_usec += 1000000L;
    }
    timeDiff = (ClUint64T)pStats->end.tv_sec*1000000L + pStats->end.tv_usec;
    pStats->totalTime = timeDiff;
    clEoQueueStatsUpdate(pStats);
}

#endif

#if 0
/*Push job into the thread pool*/
ClRcT clEoQueueJob(ClBufferHandleT recvMsg, 
                   ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    ClEoJobT *pJob = NULL;
    ClEoQueueT *pQueue = NULL; 
    ClUint32T priority ;

    if(recvMsg == CL_HANDLE_INVALID_VALUE
       ||
       !pRecvParam)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid param\n"));
        goto out;
    }

    rc = CL_EO_RC(CL_ERR_NO_MEMORY);
    pJob = clHeapCalloc(1, sizeof(*pJob));
    if(pJob == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Allocation error\n"));
        goto out;
    }
    pJob->msg = recvMsg;
    memcpy(&pJob->msgParam, pRecvParam, sizeof(pJob->msgParam));

    priority = CL_EO_RECV_QUEUE_PRI(pJob->msgParam);
    
    if(priority >= CL_IOC_MAX_PRIORITIES)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid priority [%d]\n", priority));
        goto out_free;
    }

    rc = CL_EO_RC(CL_ERR_NOT_EXIST);

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if(!(pQueue = gpClEoQueues[priority]))
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("No priority queue associated with priority [%d]\n",
                        priority));
    }
    
    if( (pQueue->state & CL_EO_QUEUE_STATE_QUIESCED) )
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        return CL_EO_RC(CL_ERR_OP_NOT_PERMITTED);
    }

    if(pQueue->pThreadPool == NULL)
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Thread pool associated with queue deleted.\n"));
        goto out_free;
    }
    /*
     * Bump up the queue refcnt and drop the lock.
     */
    ++pQueue->refCnt;

    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    rc = clThreadPoolPush((ClThreadPoolHandleT)pQueue->pThreadPool, pJob);

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    --pQueue->refCnt;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    if(rc != CL_OK)
    {
        goto out_free;
    }

    goto out;

    out_free:
    clHeapFree(pJob);
    out:
    return rc;
}
#endif

#if 0
ClRcT clEoQueueCreate(ClEoQueueHandleT *pHandle,
                      ClIocPriorityT priority,
                      ClThreadPoolModeT mode,
                      ClInt32T maxThreads,
                      ClInt32T threadPriority,
                      ClEoJobCallBackT pJobHandler,
                      ClPtrT pUserData
                      )
{
    ClEoQueueT *pQueue =NULL;
    ClThreadPoolHandleT threadPool=0;
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    if(pHandle == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid parameter\n"));
        goto out;
    }

    if( priority >= CL_IOC_MAX_PRIORITIES )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid priority %d\n",priority));
        goto out;
    }

    rc = CL_EO_RC(CL_ERR_OP_NOT_PERMITTED);

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if(gpClEoQueues[priority])
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Queue with priority:%d already active\n",priority));
        goto out;
    }
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    if(!threadPriority)
    {
        threadPriority = gClEoThreadPriorityMap[priority];
    }

    rc = CL_EO_RC(CL_ERR_NO_MEMORY);
#if 0
    pQueue = clHeapCalloc(1,sizeof(*pQueue));
    if(pQueue == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out;
    }
#endif

    CL_JOB_LIST_INIT(&pQueue->queue);
    rc = clOsalMutexInit(&pQueue->mutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&pQueue->cond);
    CL_ASSERT(rc == CL_OK);

    if(pJobHandler == NULL)
    {
        /*Default handler for jobs*/
        pJobHandler = clEoJobHandler;
    }

#if 0 /* GAS */
    rc = clThreadPoolCreate(&threadPool,(ClEoQueueHandleT)pQueue,
                            mode,maxThreads,threadPriority,pJobHandler,
                            pUserData);
#endif

    rc = clJobQueueInit(&gEoJobQueue, 0, maxThreads);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in thread pool create.rc=0x%x\n",rc));
        goto out_free;
    }

#if 0
    pQueue->pThreadPool = (ClThreadPoolT*)threadPool;
    pQueue->priority = priority;
    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if(gpClEoQueues[priority])
    {
        /*Someone beat us. So back out*/
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        rc = CL_EO_RC(CL_ERR_OP_NOT_PERMITTED);
        clThreadPoolDelete((ClThreadPoolHandleT) threadPool);
        goto out_free;
    }
    pQueue->state = CL_EO_QUEUE_STATE_ACTIVE;
    gpClEoQueues[priority] = pQueue;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
    *pHandle = (ClEoQueueHandleT)pQueue;
#endif

    rc = CL_OK;
    goto out;

    out_free:
#if 0
    clOsalMutexDestroy(&pQueue->mutex);
    clOsalCondDestroy(&pQueue->cond);
    clHeapFree(pQueue);
#endif

    out:
    return rc;
}
#endif

#if 0
ClRcT clEoQueueDeleteSync(ClIocPriorityT priority, ClBoolT force)
{
    ClEoQueueT *pQueue = NULL;
    ClRcT rc = CL_EO_RC(CL_ERR_INVALID_PARAMETER);

    if(priority >= CL_IOC_MAX_PRIORITIES)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid parameter\n"));
        goto out;
    }

    rc = CL_EO_RC(CL_ERR_NOT_EXIST);

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if(!(pQueue = gpClEoQueues[priority]))
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        CL_DEBUG_PRINT(CL_DEBUG_WARN, ("Queue priority:%d doesnt exist\n",priority));
        goto out;
    }
    gpClEoQueues[priority] = NULL;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    /*Quiesce the queue*/
    pQueue->state = CL_EO_QUEUE_STATE_QUIESCED;

    /*Delete the thread pool associated with the queue*/
    rc = CL_OK;
    if(pQueue->pThreadPool)
    {
        rc = clThreadPoolDeleteSync((ClThreadPoolHandleT)pQueue->pThreadPool, 
                                    force);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error deleting thread pool.rc=0x%x\n",rc));
        }
    }
    clOsalMutexDestroy(&pQueue->mutex);
    clOsalCondDestroy(&pQueue->cond);
    clHeapFree(pQueue);
    out:
    return rc;
} 

ClRcT clEoQueueDelete(ClIocPriorityT priority)
{
    return clEoQueueDeleteSync(priority, CL_TRUE);
}
#endif

#if 0
/*
 * Setup the priority map
*/
ClRcT clEoQueueInitialize(void)
{
    ClInt32T minPriority = sched_get_priority_min(CL_EO_SCHED_POLICY);
    ClInt32T maxPriority = sched_get_priority_max(CL_EO_SCHED_POLICY);
    ClInt32T priority=maxPriority;
    ClInt32T decr = 10;
    ClRcT rc = CL_OK;
    register ClInt32T i;

    if(minPriority < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error getting minPriority\n"));
        minPriority = 0;
    }
    if(maxPriority < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error getting maxPriority\n"));
        maxPriority = 0;
    }

    if(!priority) decr = 0;

    for(i = CL_IOC_HIGH_PRIORITY; i <= CL_IOC_LOW_PRIORITY; ++i)
    {
        gClEoThreadPriorityMap[i] = CL_MAX(priority, 0);
        priority -= decr;
    }

    gClEoThreadPriorityMap[CL_IOC_DEFAULT_PRIORITY] = CL_MAX(priority, 0);
    for(i = CL_IOC_RESERVED_PRIORITY ; i < CL_IOC_MAX_PRIORITIES ; ++i)
        gClEoThreadPriorityMap[i] = CL_MAX(priority, 0);

    rc = clOsalMutexInit(&gClEoQueueMutex);
    CL_ASSERT(rc == CL_OK);

#if defined (EO_QUEUE_STATS)
    signal(SIGUSR2, sigusr2_handler);
#endif

    return rc;
}
#endif

#if 0
/*
 * Quiesce the EO queue. Dont accept any jobs
 * Wait for pending jobs to drain.
 * If thread pool associated with queue has to be stopped,
 * then stop all the threads.
 */

ClRcT clEoQueueQuiesce(ClIocPriorityT priority, ClBoolT stopThreadPool)
{
    ClRcT rc = CL_OK;
    ClEoQueueT *pQueue = NULL;

    if(priority >= CL_IOC_MAX_PRIORITIES)
    {
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if( !(pQueue = gpClEoQueues[priority] ) )
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        return CL_EO_RC(CL_ERR_DOESNT_EXIST);
    }
    pQueue->state &= ~CL_EO_QUEUE_STATE_ACTIVE;
    pQueue->state |= CL_EO_QUEUE_STATE_QUIESCED;

    if(stopThreadPool == CL_FALSE)
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        return CL_OK;
    }
    ++pQueue->refCnt;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
    
    clThreadPoolStop((ClThreadPoolHandleT)pQueue->pThreadPool);
    
    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    --pQueue->refCnt;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    return rc;
}

ClRcT clEoQueueUnquiesce(ClIocPriorityT priority)
{
    ClRcT rc = CL_OK;
    ClEoQueueT *pQueue = NULL;

    if(priority >= CL_IOC_MAX_PRIORITIES)
    {
        return CL_EO_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    if( !(pQueue = gpClEoQueues[priority]))
    {
        CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);
        return CL_EO_RC(CL_ERR_NOT_EXIST);
    }
    pQueue->state &= ~CL_EO_QUEUE_STATE_QUIESCED;
    ++pQueue->refCnt;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    clThreadPoolResume((ClThreadPoolHandleT)pQueue->pThreadPool);

    CL_EO_QUEUE_LOCK(&gClEoQueueMutex);
    --pQueue->refCnt;
    CL_EO_QUEUE_UNLOCK(&gClEoQueueMutex);

    return rc;
}
#endif

#if 0
}
void clEoQueuesUnquiesce(void)
{
    ClInt32T i;
    for(i = 0; i < CL_IOC_MAX_PRIORITIES; ++i)
    {
        clEoQueueUnquiesce((ClIocPriorityT)i);
    }
}
#endif

ClRcT clEoQueueFinalizeSync(ClBoolT force)
{
    register ClInt32T i;
    ClRcT rc = CL_OK;
    for(i = CL_IOC_DEFAULT_PRIORITY ; i < CL_IOC_MAX_PRIORITIES; ++i)
    {
        clEoQueueDeleteSync((ClIocPriorityT)i, force);
    }
    /*Remove the unused thread pool*/
    rc = clThreadPoolFreeUnused();
    return rc;
}

ClRcT clEoQueueFinalize(void)
{
    return clEoQueueFinalizeSync(CL_TRUE);
}
