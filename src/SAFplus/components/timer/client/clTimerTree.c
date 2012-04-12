/*
 * A scalable timer implementation using red black trees
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clTimerErrors.h>
#include <clList.h>
#include <clRbTree.h>
#include <clJobQueue.h>
#include <clLogApi.h>
#include <clDbg.h>
#include <clXdrApi.h>
#include <clEoIpi.h>

#define CL_TIMER_INITIALIZED_CHECK(rc,label) do {   \
    if(gTimerBase.initialized == CL_FALSE)          \
    {                                               \
        rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);   \
        goto label;                                 \
    }                                               \
}while(0)

#define CL_TIMER_SET_EXPIRY(timer) do {                         \
    ClTimeT currentTime = gTimerBase.now ;                      \
    (timer)->timerExpiry = currentTime + (timer)->timerTimeOut; \
}while(0)

#define CL_REPETITIVE_TIMER_SET_EXPIRY(timer) do {      \
    if (CL_TIMER_REPETITIVE == (timer)->timerType)      \
    {                                                   \
        ClTimeT currentTime = gTimerBase.now;           \
        ClTimeT timerExpiry = (timer)->timerExpiry;     \
        if (timerExpiry > currentTime)                  \
        {                                               \
           timerExpiry = currentTime;                   \
        }                                               \
        (timer)->timerExpiry = timerExpiry              \
                               + (timer)->timerTimeOut; \
    }                                                   \
    else                                                \
    {                                                   \
        CL_TIMER_SET_EXPIRY(timer);                     \
    }                                                   \
}while(0)

#define CL_TIMER_FREQUENCY (10)  /*10 millisecs*/
#define CL_TIMER_CLUSTER_FREQUENCY_USEC (10000000L)
#define CL_TIMER_CLUSTER_VERSION (0x1)
#ifndef VXWORKS_BUILD
#define CL_TIMER_MAX_PARALLEL_TASKS (0x20)
#else
#define CL_TIMER_MAX_PARALLEL_TASKS (0x2)
#endif

typedef struct ClTimer
{
#define CL_TIMER_RUNNING  (0x1)
#define CL_TIMER_STOPPED  (0x2)
#define CL_TIMER_DELETED  (0x4)
#define CL_TIMER_CLUSTER  (0x8)

    /*index into the timer tree*/
    ClRbTreeT timerList;
    /*index into the cluster list*/
    ClListHeadT timerClusterList;
    ClTimerTypeT timerType;
    ClTimerContextT timerContext;
    ClTimeT timerTimeOut;
    /*absolute expiry of the timer*/
    ClTimeT timerExpiry;
    ClTimerCallBackT timerCallback;
    ClPtrT timerData;
    ClPtrT timerDataPersistent;
    ClUint32T timerDataSize;
    ClBoolT timerFlags;
    ClInt32T timerRefCnt; /*reference count of inflight separate task timers*/
    /* debug data*/
    ClTimeT startTime;
    ClTimeT startRepTime;
    ClTimeT endTime;
    ClOsalTaskIdT callbackTaskIds[CL_TIMER_MAX_PARALLEL_TASKS];
    ClInt16T freeCallbackTaskIndex;
    ClInt16T freeCallbackTaskIndexPool[CL_TIMER_MAX_PARALLEL_TASKS];
}ClTimerT;

typedef struct ClTimerBase
{
#define CL_TIMER_MAX_CALLBACK_TASKS CL_TIMER_MAX_PARALLEL_TASKS
    ClBoolT initialized;
    ClBoolT timerRunning;
    ClOsalMutexT timerListLock;
    ClTimeT now;
    ClRbTreeRootT timerTree;
    ClOsalTaskIdT timerId;
    ClUint32T runningTimers;
    ClJobQueueT timerCallbackQueue;
    ClTimeT frequency;
    ClOsalMutexT clusterListLock;
    ClListHeadT clusterList;
    ClListHeadT clusterSyncList;
    ClUint32T   clusterTimers;
    /*
     * Cluster timer timeout callback method registered
     */
    ClTimerCallBackT  clusterCallback;
    /*
     * Cluster timer replicate method registered
     */
    ClTimerReplicationCallbackT replicationCallback;
    ClJobQueueT clusterJobQueue;
}ClTimerBaseT;

extern ClBoolT gIsNodeRepresentative;
static ClBoolT gClTimerDebug = CL_FALSE;
static ClOsalMutexT gClTimerDebugLock;
static ClHandleT gTimerDebugReg;

static ClInt32T clTimerCompare(ClRbTreeT *node, ClRbTreeT *entry);
static ClTimerBaseT gTimerBase = { 
    .timerTree = CL_RBTREE_INITIALIZER(gTimerBase.timerTree, clTimerCompare),
    .clusterList = CL_LIST_HEAD_INITIALIZER(gTimerBase.clusterList),
    .clusterSyncList = CL_LIST_HEAD_INITIALIZER(gTimerBase.clusterSyncList),
};

static ClInt32T clTimerCompare(ClRbTreeT *refTimer, ClRbTreeT *timer)
{
    ClTimerT *timer1  = CL_RBTREE_ENTRY(refTimer, ClTimerT, timerList);
    ClTimerT *timer2 =  CL_RBTREE_ENTRY(timer, ClTimerT, timerList);
    if(timer1->timerExpiry > timer2->timerExpiry)
        return 1;
    else if(timer1->timerExpiry < timer2->timerExpiry)
        return -1;
    return 0;
}

static __inline__ void clTimeUpdate(void)
{
    gTimerBase.now = clOsalStopWatchTimeGet();
}

static __inline__ void timerFree(ClTimerT *pTimer)
{
    if(pTimer)
    {
        if(pTimer->timerDataPersistent)
            clHeapFree(pTimer->timerDataPersistent);
        clHeapFree(pTimer);
    }
}

/*
 * Called under the timer list lock. for a cluster timer.
 */
static ClRcT timerClusterPack(ClTimerT *pTimer, ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClTimeT expiry = 0;

    /*
     * Skip delete pending timers.
     */
    CL_ASSERT(pTimer->timerFlags & CL_TIMER_CLUSTER);

    if(pTimer->timerExpiry)
        expiry = CL_MAX(pTimer->timerExpiry - gTimerBase.now, 0);

    rc = clXdrMarshallClInt64T(&pTimer->timerTimeOut, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    rc = clXdrMarshallClUint32T(&pTimer->timerType, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    rc = clXdrMarshallClUint32T(&pTimer->timerContext, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    rc = clXdrMarshallClInt64T(&expiry, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    rc = clXdrMarshallClUint32T(&pTimer->timerDataSize, msg, 0);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        goto out;
    }

    if(pTimer->timerDataSize > 0)
    {
        rc = clXdrMarshallArrayClUint8T((ClUint8T*)pTimer->timerDataPersistent, pTimer->timerDataSize, msg, 0);
        if(rc != CL_OK)
        {
            goto out;
        }
    }
 
   out:
    return rc;
}

/*
 * Called with clusterListLock held.
 */
static ClRcT timerClusterPackAll(ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClUint8T clusterTimerVersion = CL_TIMER_CLUSTER_VERSION;
    register ClListHeadT *iter =  NULL;
    ClInt32T timers = 0;
    ClInt32T recalcInterval = 31;

    rc = clXdrMarshallClUint8T(&clusterTimerVersion, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    rc = clXdrMarshallClUint32T(&gTimerBase.clusterTimers, msg, 0);
    if(rc != CL_OK)
    {
        goto out;
    }

    clTimeUpdate();

    clOsalMutexLock(&gTimerBase.timerListLock);
    CL_LIST_FOR_EACH(iter, &gTimerBase.clusterList)
    {
        ClTimerT *pTimer = CL_LIST_ENTRY(iter, ClTimerT, timerClusterList);
        /*
         * Skip delete pending timers.
         */
        if(pTimer->timerFlags & CL_TIMER_DELETED)
            return CL_TIMER_RC(CL_ERR_INVALID_STATE);

        if( (rc = timerClusterPack(pTimer, msg)) != CL_OK)
            goto out_unlock;

        ++timers;

        if((timers & recalcInterval))
        {
            clTimeUpdate();
        }
    }

    out_unlock:
    clOsalMutexUnlock(&gTimerBase.timerListLock);

    out:
    return rc;
}

/*
 * The timer cluster task calls out the registered replication callback to 
 * replicate the cluster list
 */
static ClRcT timerClusterTask(ClPtrT pThrottle)
{
    ClTimerTimeOutT frequency = {.tsSec = 10, .tsMilliSec = 0};
    ClBufferHandleT msg = 0;
    static ClTimeT lastRunTime;
    ClTimeT currentTime = 0;
    ClRcT rc = CL_OK;

    clOsalMutexLock(&gTimerBase.clusterListLock);
    if(CL_LIST_HEAD_EMPTY(&gTimerBase.clusterList))
    {
        goto out_unlock;
    }

    currentTime = clOsalStopWatchTimeGet();

    if(pThrottle && lastRunTime && 
       (currentTime - lastRunTime) < CL_TIMER_CLUSTER_FREQUENCY_USEC)
    {
        ClTimeT remainingTime = CL_TIMER_CLUSTER_FREQUENCY_USEC - (currentTime - lastRunTime);
        remainingTime /= 1000;
        if(remainingTime > 0)
        {
            frequency.tsSec = remainingTime/1000;
            frequency.tsMilliSec = remainingTime%1000;
            clOsalMutexUnlock(&gTimerBase.clusterListLock);
            clOsalTaskDelay(frequency);
            clOsalMutexLock(&gTimerBase.clusterListLock);
        }
    }

    /*
     * Gather the clustered timer list.
     */
    rc = clBufferCreate(&msg);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "REPLICATE", "Skipping replication of cluster timers as buffer create returned [%#x]", rc);
        goto out_unlock;
    }
    rc = timerClusterPackAll(msg);
    if(rc != CL_OK)
    {
        goto out_unlock;
    }

    /*
     * Invoke the replication callback registered for the timer.
     */
    if(gTimerBase.replicationCallback)
    {
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        rc = gTimerBase.replicationCallback(msg);
        clOsalMutexLock(&gTimerBase.clusterListLock);
    }

    out_unlock:
    currentTime = clOsalStopWatchTimeGet();
    lastRunTime = currentTime;
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    if(msg)
        clBufferDelete(&msg);

    return rc;
}

/*
 * Called under the lock.
 */
static ClRcT timerClusterTaskSchedule(ClTimerT *pTimer, ClBoolT throttle)
{
    ClPtrT arg = NULL;
    if(throttle)
        arg = (ClPtrT)(ClWordT)1;
    if(gTimerBase.clusterJobQueue.flags && gTimerBase.replicationCallback)
        return clJobQueuePush(&gTimerBase.clusterJobQueue, timerClusterTask, arg);
    return CL_TIMER_RC(CL_ERR_INVALID_STATE);
}

/*
 * Called under the lock.
 */
static ClRcT timerClusterDel(ClTimerT *pTimer)
{
    ClRcT rc = CL_OK;
    if(!(pTimer->timerFlags & CL_TIMER_CLUSTER)
       ||
       !gTimerBase.replicationCallback)
        return rc;
    --gTimerBase.clusterTimers;
    CL_ASSERT(gTimerBase.clusterTimers >= 0);
    clListDel(&pTimer->timerClusterList);
    if(gTimerBase.clusterJobQueue.flags)
    {
        rc = clJobQueuePush(&gTimerBase.clusterJobQueue, timerClusterTask, NULL);
    }
    return rc;
}

static ClRcT timerClusterAdd(ClTimerT *pTimer, ClBoolT clusterSync, ClBoolT locked)
{
    ClRcT rc = CL_OK;
    if(!(pTimer->timerFlags & CL_TIMER_CLUSTER)
       ||
       !gTimerBase.replicationCallback)
        return rc;
    if(!locked)
    {
        clOsalMutexLock(&gTimerBase.clusterListLock);
    }
    ++gTimerBase.clusterTimers;
    clListAddTail(&pTimer->timerClusterList, &gTimerBase.clusterList);
    if(!gTimerBase.clusterJobQueue.flags)
    {
        /*
         * Initialize the cluster job queue.
         */
        rc = clJobQueueInit(&gTimerBase.clusterJobQueue, 0, 1);
        if(rc != CL_OK)
        {
            clLogError("TIMER", "CLUSTER", "Timer add to cluster list returned [%#x]", rc);
            goto out_unlock;
        }
    }
    if(clusterSync)
    {
        rc = clJobQueuePushIfEmpty(&gTimerBase.clusterJobQueue, timerClusterTask, (ClPtrT)(ClWordT)1);
        if(rc != CL_OK)
        {
            clLogError("TIMER", "CLUSTER", "Timer cluster list queue add returned [%#x]", rc);
        }
    }
    out_unlock:
    if(!locked)
    {
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
    }
    return rc;
}

static void timerInitCallbackTask(ClTimerT *pTimer)
{
    ClInt32T i;
    for(i = 0; i < CL_TIMER_MAX_PARALLEL_TASKS-1; ++i)
        pTimer->freeCallbackTaskIndexPool[i] = i+1;
    pTimer->freeCallbackTaskIndexPool[i] = -1;
    pTimer->freeCallbackTaskIndex = 0;
}

static ClBoolT timerMatchCallbackTask(ClTimerT *pTimer, ClOsalTaskIdT *pSelfId)
{
    ClInt32T i;
    ClOsalTaskIdT selfId = 0;
    if(!pSelfId)
        pSelfId = &selfId;
    if(!*pSelfId && clOsalSelfTaskIdGet(pSelfId) != CL_OK)
        return CL_FALSE;
    selfId = *pSelfId;
    for(i = 0; i < CL_TIMER_MAX_PARALLEL_TASKS; ++i)
    {
        if(selfId == pTimer->callbackTaskIds[i])
            return CL_TRUE;
    }
    return CL_FALSE;
}

static __inline__ ClInt16T timerAddCallbackTask(ClTimerT *pTimer)
{
    ClInt16T nextFreeIndex = 0;
    ClInt16T curFreeIndex = 0;
    if( (curFreeIndex = pTimer->freeCallbackTaskIndex) < 0 )
    {
        clLogWarning("CALLBACK", "ADD", "Unable to store task id for timer [%p] as current [%d] parallel instances "
                     "of the same running timer exceeds [%d] supported. Timer delete on this timer might deadlock "
                     "if issued from the timer callback context", (ClPtrT)pTimer, pTimer->timerRefCnt,
                     CL_TIMER_MAX_PARALLEL_TASKS);
        return -1;
    }
    nextFreeIndex = pTimer->freeCallbackTaskIndexPool[curFreeIndex];
    if(clOsalSelfTaskIdGet(pTimer->callbackTaskIds + curFreeIndex) != CL_OK)
        return -1;
    pTimer->freeCallbackTaskIndex = nextFreeIndex;
    return curFreeIndex;
}

static __inline__ void timerDelCallbackTask(ClTimerT *pTimer, ClInt16T freeIndex)
{
    if(freeIndex >= 0)
    {
        pTimer->callbackTaskIds[freeIndex] = 0;
        pTimer->freeCallbackTaskIndexPool[freeIndex] = pTimer->freeCallbackTaskIndex;
        pTimer->freeCallbackTaskIndex = freeIndex;
    }
}

static ClRcT timerCreate(ClTimerTimeOutT timeOut, 
                         ClTimerTypeT timerType,
                         ClTimerContextT timerContext,
                         ClTimerCallBackT timerCallback,
                         void *timerData,
                         ClUint32T timerDataSize,
                         ClBoolT timerCluster,
                         ClTimerHandleT *pTimerHandle)
{
    ClTimerT *pTimer = NULL;
    ClTimeT expiry = 0;
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(pTimerHandle == NULL)
    {
        clDbgCodeError(rc, ("Timer create failed: Bad timer handle storage"));
        goto out;
    }

    if(!timerCallback)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_NULL_TIMER_CALLBACK);
        clDbgCodeError(rc, ("Timer create failed: Null callback function passed"));
        goto out;
    }

    if(timerType >= CL_TIMER_MAX_TYPE)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_TYPE);
        clDbgCodeError(rc, ("Timer create failed: Bad timer type"));
        goto out;
    }

    if(timerContext >= CL_TIMER_MAX_CONTEXT)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_CONTEXT_TYPE);
        clDbgCodeError(rc, ("Timer create failed: Bad timer context"));
        goto out;
    }

    rc = CL_TIMER_RC(CL_ERR_NO_MEMORY);
    pTimer = clHeapCalloc(1,sizeof(*pTimer));
    if(pTimer == NULL)
    {
        clLogError("TIMER", "CREATE", "Allocation failed");
        goto out;
    }
    expiry = (ClTimeT)((ClTimeT)timeOut.tsSec *1000000 +  timeOut.tsMilliSec*1000);
    /*Convert this expiry to jiffies*/
    pTimer->timerTimeOut = expiry;
    pTimer->timerType = timerType;
    pTimer->timerContext = timerContext;
    pTimer->timerCallback = timerCallback;
    pTimer->timerData = timerData;
    pTimer->timerDataPersistent = NULL;
    pTimer->timerDataSize = timerDataSize;
    timerInitCallbackTask(pTimer);
    if(timerDataSize > 0)
    {
        /*
         * Use a persistent store for replication. which is unaffected if timer users 
         * corrupt the timer data on callbacks.
         */
        pTimer->timerDataPersistent = clHeapCalloc(1, timerDataSize);
        CL_ASSERT(pTimer->timerDataPersistent != NULL);
        if(pTimer->timerData)
            memcpy(pTimer->timerDataPersistent, pTimer->timerData, timerDataSize);
    }
    if(timerCluster)
        pTimer->timerFlags = CL_TIMER_CLUSTER;
    else
        pTimer->timerFlags = 0;
    *pTimerHandle = (ClTimerHandleT)pTimer;

    rc = CL_OK;

    out:
    return rc;
}

ClRcT clTimerCreate(ClTimerTimeOutT timeOut, 
                    ClTimerTypeT timerType,
                    ClTimerContextT timerContext,
                    ClTimerCallBackT timerCallback,
                    void *timerData,
                    ClTimerHandleT *pTimerHandle)
{
    return timerCreate(timeOut, timerType, timerContext, timerCallback, timerData, 0, CL_FALSE, pTimerHandle);
}

ClRcT clTimerCreateCluster(ClTimerTimeOutT timeOut, 
                           ClTimerTypeT timerType,
                           ClTimerContextT timerContext,
                           void *timerData,
                           ClUint32T timerDataSize,
                           ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;

    if(!gTimerBase.clusterCallback)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_STATE);
        clLogError("TIMER", "CREATE", "Cluster timer callbacks are not registered with clTimerClusterRegister");
        goto out;
    }
 
    rc = timerCreate(timeOut, timerType, timerContext, gTimerBase.clusterCallback,
                     timerData, timerDataSize, CL_TRUE, pTimerHandle);

    if(rc != CL_OK)
        goto out;

    rc = timerClusterAdd((ClTimerT*)*pTimerHandle, CL_TRUE, CL_FALSE);

    out:
    return rc;
}

ClRcT clTimerCreateAndStart(ClTimerTimeOutT timeOut, 
                            ClTimerTypeT timerType,
                            ClTimerContextT timerContext,
                            ClTimerCallBackT timerCallback,
                            void *timerData,
                            ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;

    rc = clTimerCreate(timeOut, timerType, timerContext, timerCallback, timerData, pTimerHandle);

    if(rc != CL_OK)
        goto out;

    rc = clTimerStart(*pTimerHandle);

    out:
    return rc;
}

ClRcT clTimerCreateAndStartCluster(ClTimerTimeOutT timeOut, 
                                   ClTimerTypeT timerType,
                                   ClTimerContextT timerContext,
                                   void *timerData,
                                   ClUint32T timerDataSize,
                                   ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;

    rc = clTimerCreateCluster(timeOut, timerType, timerContext, timerData, timerDataSize, pTimerHandle);

    if(rc != CL_OK)
        goto out;

    rc = clTimerStart(*pTimerHandle);

    out:
    return rc;
}

/*
 * Start the timer.
*/

static ClRcT timerStart(ClTimerHandleT timerHandle, ClTimeT expiry, ClBoolT clusterSync, ClBoolT locked)
{
    ClTimerT *pTimer = NULL;
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(timerHandle == 0)
    {
        clDbgCodeError(rc, ("Timer start failed: Bad timer handle storage"));
        goto out;
    }

    pTimer = (ClTimerT*)timerHandle;

    if(!locked)
    {
        clOsalMutexLock(&gTimerBase.clusterListLock);
        clOsalMutexLock(&gTimerBase.timerListLock);
        pTimer = (ClTimerT*)timerHandle;
        if(!pTimer)
        {
            clOsalMutexUnlock(&gTimerBase.timerListLock);
            clOsalMutexUnlock(&gTimerBase.clusterListLock);
            goto out;
        }
    }

    pTimer->timerFlags &= ~CL_TIMER_STOPPED;

    if(expiry)
    {
        pTimer->timerExpiry = expiry;
    }
    else
    {
        clTimeUpdate();
        CL_TIMER_SET_EXPIRY(pTimer);
    }
    

    /* 
     * add to the rb tree.
     */
    clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);

    if(gClTimerDebug)
        pTimer->startTime = clOsalStopWatchTimeGet();

    if((pTimer->timerFlags & CL_TIMER_CLUSTER)
       &&
       clusterSync)
        timerClusterTaskSchedule(pTimer, CL_FALSE);

    if(!locked)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
    }

    rc = CL_OK;

    out:
    return rc;
}

ClRcT clTimerStart(ClTimerHandleT timerHandle)
{
    return timerStart(timerHandle, 0, CL_TRUE, CL_FALSE);
}

static ClRcT timerStop(ClTimerHandleT timerHandle, ClBoolT clusterSync)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerT *pTimer = NULL;

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(timerHandle == 0)
    {
        clDbgCodeError(rc, ("Timer stop failed: Bad timer handle storage"));
        goto out;
    }
    
    rc = CL_OK;

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    pTimer = (ClTimerT*)timerHandle;

    if(!pTimer)
    {
        /*
         * Timer was deleted behind our back. Back out.
         */
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        goto out;
    }

    /*Reset timer expiry. and rip this guy off from the list.*/
    pTimer->timerExpiry = 0;
    pTimer->timerFlags |= CL_TIMER_STOPPED;
    clRbTreeDelete(&gTimerBase.timerTree, &pTimer->timerList);

    if((pTimer->timerFlags & CL_TIMER_CLUSTER) && clusterSync)
        timerClusterTaskSchedule(pTimer, CL_FALSE);

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    out:
    return rc;
}

ClRcT clTimerStop(ClTimerHandleT timerHandle)
{
    return timerStop(timerHandle, CL_TRUE);
}

ClRcT clTimerUpdate(ClTimerHandleT timerHandle, ClTimerTimeOutT newTimeOut)
{
    ClRcT rc = CL_OK;
    ClTimerT *pTimer = NULL;

    CL_TIMER_INITIALIZED_CHECK(rc, out);

    if(!timerHandle)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(rc, ("Timer update failed: Bad timer handle storage"));
        goto out;
    }
    /*
     * First stop the timer.
     */
    rc = timerStop(timerHandle, CL_FALSE);
    if(rc != CL_OK)
    {
        clDbgCodeError(rc, ("Timer stop failed"));
        goto out;
    }
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);

    pTimer = (ClTimerT*)timerHandle;
    if(!pTimer)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        rc = CL_TIMER_RC(CL_ERR_NOT_EXIST);
        goto out;
    }

    pTimer->timerFlags &= ~CL_TIMER_STOPPED;
    pTimer->timerTimeOut = (ClTimeT)((ClTimeT)newTimeOut.tsSec * 1000000 + newTimeOut.tsMilliSec * 1000);
    clTimeUpdate();
    CL_TIMER_SET_EXPIRY(pTimer);
    
    clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);

    if(gClTimerDebug)
        pTimer->startTime = clOsalStopWatchTimeGet();

    if(pTimer->timerFlags & CL_TIMER_CLUSTER)
        timerClusterTaskSchedule(pTimer, CL_FALSE);

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    out:
    return rc;
}

ClRcT
clTimerRestart (ClTimerHandleT  timerHandle)
{
    ClRcT rc = 0;

    CL_FUNC_ENTER();

    rc = timerStop (timerHandle, CL_FALSE);

    if (rc != CL_OK) 
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = clTimerStart (timerHandle);

    if (rc != CL_OK) 
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 * CANNOT BE INVOKED FROM THE timer callback thats RUNNING.
 */
static ClRcT timerDelete(ClTimerHandleT *pTimerHandle, ClBoolT asyncFlag)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerT *pTimer = NULL;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 50 };
    ClInt32T runCounter = 0;
    ClOsalTaskIdT selfId = 0;

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(pTimerHandle == NULL)
    {
        clDbgCodeError(rc, ("Timer delete failed: Bad timer handle storage"));
        goto out;
    }

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);

    pTimer = (ClTimerT*)*pTimerHandle;

    if(!pTimer)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        rc = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(rc, ("Timer delete failed: Bad timer handle"));
        goto out;
    }
    rc = CL_TIMER_RC(CL_ERR_OP_NOT_PERMITTED);

    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        clDbgCodeError(rc, ("Timer delete failed: Volatile auto-delete timer tried to be deleted"));
        goto out;
    }

    clRbTreeDelete(&gTimerBase.timerTree, &pTimer->timerList);

    if(pTimer->timerFlags & CL_TIMER_CLUSTER)
    {
        timerClusterDel(pTimer);
    }

    *pTimerHandle = 0;

    clOsalSelfTaskIdGet(&selfId);

    if((pTimer->timerFlags & CL_TIMER_RUNNING) 
       &&
       pTimer->timerRefCnt > 0)
    {
        if(asyncFlag || timerMatchCallbackTask(pTimer, &selfId))
        {
            rc = CL_OK;
            pTimer->timerFlags |= CL_TIMER_DELETED;
            clOsalMutexUnlock(&gTimerBase.timerListLock);
            clOsalMutexUnlock(&gTimerBase.clusterListLock);
            goto out;
        }
        /* 
         * If we get a DELETE request for a running timer outside callback thread context,
         * we would block till the timer is not running.
         */
        while((pTimer->timerFlags & CL_TIMER_RUNNING)
              &&
              pTimer->timerRefCnt > 0)
        {
            clOsalMutexUnlock(&gTimerBase.timerListLock);
            clOsalMutexUnlock(&gTimerBase.clusterListLock);
            if(runCounter 
               && 
               !(runCounter & 255))
            {
                clLogWarning("TIMER", "DEL", "Task [%#llx] waiting for timer callback task to exit",
                             selfId);
            }
            clOsalTaskDelay(delay);
            ++runCounter;
            runCounter &= 32767;
            clOsalMutexLock(&gTimerBase.clusterListLock);
            clOsalMutexLock(&gTimerBase.timerListLock);
        }
    }

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    
    timerFree(pTimer);

    rc = CL_OK;

    out:
    return rc;
}

/*
 * Dont call it under a callback.
 */
ClRcT clTimerDelete(ClTimerHandleT *pTimerHandle)
{
    return timerDelete(pTimerHandle, CL_FALSE);
}

ClRcT clTimerDeleteAsync(ClTimerHandleT *pTimerHandle)
{
    return timerDelete(pTimerHandle, CL_TRUE);
}

static ClRcT timerState(ClTimerHandleT timerHandle, ClBoolT flags, ClBoolT *pState)
{
    ClTimerT *pTimer = (ClTimerT*)timerHandle;
    if(!pTimer || !pState)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    *pState = CL_FALSE;
    if((pTimer->timerFlags & flags))
        *pState = CL_TRUE;
    return CL_OK;
}

/*
 * Assumed that the caller has also synchronized his call with his timer start/stop/delete
 */
ClRcT clTimerIsStopped(ClTimerHandleT timerHandle, ClBoolT *pState)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    rc = timerState(timerHandle, CL_TIMER_STOPPED, pState);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return rc;
}

/*
 * Assumed that the caller has also synchronized his call with his timer start/stop/delete.
 */
ClRcT clTimerIsRunning(ClTimerHandleT timerHandle, ClBoolT *pState)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    rc = timerState(timerHandle, CL_TIMER_RUNNING, pState);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return rc;
}

static ClRcT clTimerCallbackTask(ClPtrT invocation)
{
    ClTimerT *pTimer = invocation;
    ClTimerTypeT type = pTimer->timerType;
    ClBoolT canFree = CL_FALSE;
    ClInt16T callbackTaskIndex = -1;
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    
    if(gClTimerDebug)
    {
        clOsalMutexLock(&gClTimerDebugLock);
        pTimer->endTime = clOsalStopWatchTimeGet();
        clLogNotice("TIMER", "CALL", "Timer task invoked at [%lld] - [%lld.%lld] usecs. "
                    "Expiry [%lld] usecs with flags [%#x]", pTimer->endTime - pTimer->startTime,
                    (ClTimeT)((pTimer->endTime - pTimer->startTime)/1000000L),
                    (ClTimeT)((pTimer->endTime - pTimer->startTime) % 1000000L),
                    pTimer->timerTimeOut, pTimer->timerFlags);
        pTimer->startTime = 0;
        pTimer->startRepTime = 0;
        pTimer->endTime = 0;
        clOsalMutexUnlock(&gClTimerDebugLock);
    }

    /*
     * Check if we had a delete or stop while we were getting conceived.
     */
    if( (pTimer->timerFlags & CL_TIMER_DELETED ) )
    {
        if(--pTimer->timerRefCnt <= 0)
        {
            canFree = CL_TRUE;
            pTimer->timerFlags &= ~CL_TIMER_RUNNING;
        }
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        if(canFree == CL_TRUE)
        {
            timerFree(pTimer);
        }
        return CL_OK;
    }
    else if( (pTimer->timerFlags & CL_TIMER_STOPPED) )
    {
        --pTimer->timerRefCnt;
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        return CL_OK;
    }

    ++gTimerBase.runningTimers;
    callbackTaskIndex = timerAddCallbackTask(pTimer);

    /*
     * If its a repetitive timer, add back into the list just before callback invocation.
     */
    if(type == CL_TIMER_REPETITIVE)
    {
        clTimeUpdate();
        CL_REPETITIVE_TIMER_SET_EXPIRY(pTimer);
        clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);
    }

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    pTimer->timerCallback(pTimer->timerData);
    if (gClTimerDebug)
    {
        if (CL_TIMER_REPETITIVE == type)
        {
            clOsalMutexLock(&gClTimerDebugLock);
            pTimer->startTime = clOsalStopWatchTimeGet();
            clOsalMutexUnlock(&gClTimerDebugLock);
        }
    }

    canFree = CL_FALSE;

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);

    if(callbackTaskIndex >= 0)
        timerDelCallbackTask(pTimer, callbackTaskIndex);

    --pTimer->timerRefCnt;

    /*
     * Recheck for a pending delete request.
     */
    if( (pTimer->timerFlags & CL_TIMER_DELETED ) )
    {
        if(pTimer->timerRefCnt <= 0)
            canFree = CL_TRUE;
    }
    else if(type == CL_TIMER_VOLATILE)
    {
        if(pTimer->timerFlags & CL_TIMER_CLUSTER)
        {
            timerClusterDel(pTimer);
        }
        canFree = CL_TRUE;
    }

    --gTimerBase.runningTimers;

    if(pTimer->timerRefCnt <= 0)
        pTimer->timerFlags &= ~CL_TIMER_RUNNING;
    
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    if(canFree == CL_TRUE)
    {
        timerFree(pTimer);
    }
    return CL_OK;
}

static __inline__ void clTimerSpawn(ClTimerT *pTimer)
{
    /*
     * Invoke the callback in the task pool context
     */
    clJobQueuePush(&gTimerBase.timerCallbackQueue, clTimerCallbackTask, pTimer);
}

/*
 * Run the sorted timer list expiring timers. 
 * Called with the timer list lock held.
 */
static ClRcT clTimerRun(void)
{
    ClInt32T recalcInterval = 15; /*recalculate time after expiring this much*/
    ClInt64T timers = 0;
    ClRbTreeT *iter = NULL;
    ClRbTreeRootT *root = &gTimerBase.timerTree;
    ClTimeT now = gTimerBase.now;
    ClBoolT clusterSync = CL_FALSE;
    /*
     * We take the minimum treenode at each iteration since we will drop the lock
     * while invoking the callback and it could so happen that the next
     * entry that we cached gets ripped off thereby forcing us to resort to
     * some dramatics while coding. So lets keep it clean and take the minimum
     * from the tree node.
     */

    while ( (iter = clRbTreeMin(root)) )
    {
        ClTimerT *pTimer = NULL; 
        ClTimerCallBackT timerCallback;
        ClPtrT timerData;
        ClTimerTypeT timerType;
        ClTimerContextT timerContext;
        ClInt16T callbackTaskIndex = -1;

        if( !(++timers & recalcInterval) )
        {
            clTimeUpdate();
            now = gTimerBase.now;
        }
        pTimer = CL_RBTREE_ENTRY(iter, ClTimerT, timerList);
        if(pTimer->timerExpiry > now) break;

        timerCallback = pTimer->timerCallback;
        timerData = pTimer->timerData;
        timerType = pTimer->timerType;
        timerContext = pTimer->timerContext;
        /*Knock off from this list*/
        clRbTreeDelete(root, iter);
        
        if(timerType == CL_TIMER_REPETITIVE && timerContext == CL_TIMER_TASK_CONTEXT)
        {
            CL_REPETITIVE_TIMER_SET_EXPIRY(pTimer);
            clRbTreeInsert(root, iter);
        }

        /* 
         * Mark it running to prevent a moron from killing us
         * behind our back.
         */
        pTimer->timerFlags |= CL_TIMER_RUNNING;
        /*
         * reference count the timer fired
         */
        ++pTimer->timerRefCnt;

        if(timerType != CL_TIMER_VOLATILE && timerContext == CL_TIMER_TASK_CONTEXT)
            callbackTaskIndex = timerAddCallbackTask(pTimer);
        
        if( (pTimer->timerFlags & CL_TIMER_CLUSTER) )
            clusterSync = CL_TRUE;

        /*
         * Drop the lock now.
         */
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);

        /*
         * Now fire the sucker up.
         */
        CL_ASSERT(timerCallback != NULL);
        
        switch(timerContext)
        {
        case CL_TIMER_SEPARATE_CONTEXT:
            {
                clTimerSpawn(pTimer);
            }
            break;
        case CL_TIMER_TASK_CONTEXT:
        default:
            if(gClTimerDebug)
            {
                pTimer->endTime = clOsalStopWatchTimeGet();
                clLogNotice("TIMER", "CALL", "Timer invoked at [%lld] - [%lld.%lld] usecs. "
                            "Expiry [%lld] usecs", pTimer->endTime - pTimer->startTime,
                            (ClTimeT)((pTimer->endTime - pTimer->startTime)/1000000L),
                            (ClTimeT)((pTimer->endTime - pTimer->startTime)%1000000L),
                            pTimer->timerTimeOut);
                pTimer->startTime = 0;
                pTimer->startRepTime = 0;
                pTimer->endTime = 0;
            }
            timerCallback(timerData);
            if (gClTimerDebug)
            {
               if (CL_TIMER_REPETITIVE == timerType)
               {
                   pTimer->startTime = clOsalStopWatchTimeGet();
               }
            }
            break;
        }

        switch(timerType)
        {
        case CL_TIMER_VOLATILE:
            /*
             *Rip this bastard off if not separate context.
             *for separate contexts,thread spawn would do that.
             */
            if(timerContext == CL_TIMER_TASK_CONTEXT)
            {
                clOsalMutexLock(&gTimerBase.clusterListLock);
                if(pTimer->timerFlags & CL_TIMER_CLUSTER)
                    timerClusterDel(pTimer);
                clOsalMutexUnlock(&gTimerBase.clusterListLock);
                clHeapFree(pTimer);
            }
            break;
        case CL_TIMER_REPETITIVE:
        default:
            break;
        }

        clOsalMutexLock(&gTimerBase.clusterListLock);
        clOsalMutexLock(&gTimerBase.timerListLock);

        /*
         * In order to avoid a leak when a delete request for a ONE shot/same context
         * comes when we dropped the lock, we double check for a pending delete
         */
        if(timerType != CL_TIMER_VOLATILE && timerContext == CL_TIMER_TASK_CONTEXT)
        {
            if(callbackTaskIndex >= 0)
                timerDelCallbackTask(pTimer, callbackTaskIndex);

            --pTimer->timerRefCnt;

            pTimer->timerFlags &= ~CL_TIMER_RUNNING;
            if( (pTimer->timerFlags & CL_TIMER_DELETED) )
            {
                clOsalMutexUnlock(&gTimerBase.timerListLock);
                clOsalMutexUnlock(&gTimerBase.clusterListLock);
                timerFree(pTimer);
                clOsalMutexLock(&gTimerBase.clusterListLock);
                clOsalMutexLock(&gTimerBase.timerListLock);
            }
        }
    }

    if(clusterSync)
    {
        timerClusterTaskSchedule(NULL, CL_FALSE);
    }
    return CL_OK;
}

/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *clTimerTask(void *pArg)
{
    ClTimerTimeOutT timeOut = {.tsSec = 0, .tsMilliSec = gTimerBase.frequency };

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);

    clTimeUpdate();

    while(gTimerBase.timerRunning == CL_TRUE)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);

        clOsalTaskDelay(timeOut);

        clOsalMutexLock(&gTimerBase.clusterListLock);
        clOsalMutexLock(&gTimerBase.timerListLock);

        clTimeUpdate();
        
        clTimerRun();
    }

    /*Unreached actually*/
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    return NULL;
}

static ClRcT clTimerBaseInitialize(ClTimerBaseT *pBase)
{
    ClRcT rc = CL_OK;
    pBase->timerCallbackQueue.flags = 0;
    pBase->frequency = CL_TIMER_FREQUENCY;
    clOsalMutexInit(&pBase->timerListLock);
    clOsalMutexInit(&pBase->clusterListLock);
    gTimerBase.clusterTimers = 0;
    CL_LIST_HEAD_INIT(&pBase->clusterList);
    CL_LIST_HEAD_INIT(&pBase->clusterSyncList);
    return rc;
}

static ClRcT cliTimersShow(int argc, char **argv, char **ret)
{
    ClTimerStatsT *pStats = NULL;
    ClDebugPrintHandleT msgHandle = 0;
    ClUint32T numTimers = 0;
    ClRcT rc = CL_OK;
    ClUint32T i;

    if(argc != 1) return CL_ERR_INVALID_PARAMETER;
    *ret = NULL;
    if((rc = clDebugPrintInitialize(&msgHandle)) != CL_OK)
        return rc;
    if( (rc = clTimerStatsGet(&pStats, &numTimers)) != CL_OK)
    {
        goto out_free;
    }
    clDebugPrint(msgHandle, "%-10s%-12s%-10s%-17s%-17s\n",
                 "INDEX", "TYPE", "CONTEXT", "TIMEOUT (usecs)", "EXPIRY (usecs)");
    clDebugPrint(msgHandle, "%.*s\n", 68,
                 "------------------------------------------------------------------------");
    for(i = 0; i < numTimers; ++i)
    {
        clDebugPrint(msgHandle, "%-10d%-12s%-10s%-17lld%-17lld\n", 
                     i+1, CL_TIMER_TYPE_STR(pStats[i].type), CL_TIMER_CONTEXT_STR(pStats[i].context),
                     pStats[i].timeOut, pStats[i].expiry);
    }

    out_free:
    clDebugPrintFinalize(&msgHandle, ret);

    if(pStats)
        clHeapFree(pStats);
    return CL_OK;
}

static ClDebugFuncEntryT gClTimerCliTab[] = {
    {(ClDebugCallbackT) cliTimersShow, "timersShow", "Show started timers"} ,
};

ClRcT clTimerDebugRegister(void)
{
    ClRcT rc = CL_OK;
    if(!gTimerDebugReg)
    {
        rc = clDebugRegister(gClTimerCliTab,
                             sizeof(gClTimerCliTab) / sizeof(gClTimerCliTab[0]), 
                             &gTimerDebugReg);
    }
    return rc;
}

ClRcT clTimerDebugDeregister()
{
    ClRcT rc = CL_OK;
    if(gTimerDebugReg)
    {
        rc = clDebugDeregister(gTimerDebugReg);
        gTimerDebugReg = CL_HANDLE_INVALID_VALUE;
    }
    return rc;
}

ClRcT clTimerInitialize(ClPtrT config)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INITIALIZED);
    ClUint32T callbackTasks = CL_TIMER_MAX_CALLBACK_TASKS;

    if(gTimerBase.initialized == CL_TRUE)
    {
        goto out;
    }

    if(clParseEnvBoolean("CL_TIMER_DEBUG") == CL_TRUE)
    {
        gClTimerDebug = CL_TRUE;
        /* crash if used outside debug context*/
        clOsalMutexInit(&gClTimerDebugLock);
    }

    rc = clTimerBaseInitialize(&gTimerBase);
    if(rc != CL_OK)
    {
        goto out;
    }
    if(!gIsNodeRepresentative)
    {
        callbackTasks = CL_IOC_RESERVED_PRIORITY+1;
        if(callbackTasks > CL_TIMER_MAX_CALLBACK_TASKS)
            callbackTasks = CL_TIMER_MAX_CALLBACK_TASKS;
        clLogNotice("CALLBACK", "TASKS", "Setting timer callback pool to [%d] tasks", callbackTasks);
    }
    rc = clJobQueueInit(&gTimerBase.timerCallbackQueue, 0, callbackTasks);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "INI", "Timer callback jobqueue create returned [%#x]", rc);
        goto out_free;
    }

    gTimerBase.timerRunning = CL_TRUE;

    clTimeUpdate();

    rc = clOsalTaskCreateAttached("TIMER-TASK", CL_OSAL_SCHED_OTHER, 0, 0, clTimerTask, NULL, 
                                  &gTimerBase.timerId);
    if(rc != CL_OK)
    {
        gTimerBase.timerRunning = CL_FALSE;
        clLogError("TIMER", "INIT", "Timer task create returned [%#x]", rc);
        goto out_free;
    }
    
    gTimerBase.initialized = CL_TRUE;

    return rc;

    out_free:

    if(gTimerBase.timerCallbackQueue.flags)
        clJobQueueDelete(&gTimerBase.timerCallbackQueue);
    
    if(gTimerBase.timerId)
    {
        gTimerBase.timerRunning = CL_FALSE;
        clOsalTaskJoin(gTimerBase.timerId);
    }

    out:
    return rc;
}

ClRcT clTimerFinalize(void)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);

    if(gTimerBase.initialized == CL_FALSE)
    {
        goto out;
    }

    gTimerBase.initialized = CL_FALSE;

    if(gTimerBase.timerRunning == CL_TRUE)
    {
        clOsalMutexLock(&gTimerBase.clusterListLock);
        clOsalMutexLock(&gTimerBase.timerListLock);
        gTimerBase.timerRunning = CL_FALSE;
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalMutexUnlock(&gTimerBase.clusterListLock);

        if(gTimerBase.timerId)
        {
            clOsalTaskJoin(gTimerBase.timerId);
        }
    }
    
    if(gTimerBase.timerCallbackQueue.flags)
        clJobQueueDelete(&gTimerBase.timerCallbackQueue);

    clOsalMutexLock(&gTimerBase.clusterListLock);
    if(gTimerBase.clusterTimers && gTimerBase.clusterJobQueue.flags)
        clJobQueueDelete(&gTimerBase.clusterJobQueue);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clTimerClusterRegister(ClTimerCallBackT clusterCallback,
                             ClTimerReplicationCallbackT replicationCallback)
{
    if(gTimerBase.initialized == CL_FALSE)
        return CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);
    clOsalMutexLock(&gTimerBase.clusterListLock);
    if(gTimerBase.clusterCallback || gTimerBase.replicationCallback)
    {
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        return CL_TIMER_RC(CL_ERR_ALREADY_EXIST);
    }
    if(!clusterCallback)
    {
        clOsalMutexUnlock(&gTimerBase.clusterListLock);
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    }
    gTimerBase.clusterCallback = clusterCallback;
    gTimerBase.replicationCallback = replicationCallback;
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return CL_OK;
}

/*
 * Sync up the cluster timers.
 */
ClRcT clTimerClusterSync(void)
{
    ClRcT rc = CL_OK;
    if(gTimerBase.initialized == CL_FALSE)
        return CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);
    clOsalMutexLock(&gTimerBase.clusterListLock);
    if(gTimerBase.clusterTimers)
        rc = timerClusterTaskSchedule(NULL, CL_FALSE);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return rc;
}     

static ClRcT timerClusterUnpack(ClBufferHandleT msg, ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;
    ClTimerT *pTimer = NULL;

    if(!pTimerHandle)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    pTimer = (ClTimerT*)*pTimerHandle;
    if(!pTimer)
    {
        pTimer = clHeapCalloc(1, sizeof(*pTimer));
        CL_ASSERT(pTimer != NULL);
    }
    else
    {
        if(pTimer->timerDataPersistent)
            clHeapFree(pTimer->timerDataPersistent);
        memset(pTimer, 0, sizeof(*pTimer));
    }

    rc = clXdrUnmarshallClInt64T(msg, &pTimer->timerTimeOut);
    if(rc != CL_OK)
        goto out_free;
    rc = clXdrUnmarshallClUint32T(msg, &pTimer->timerType);
    if(rc != CL_OK)
        goto out_free;
    rc = clXdrUnmarshallClUint32T(msg, &pTimer->timerContext);
    if(rc != CL_OK)
        goto out_free;
    rc = clXdrUnmarshallClInt64T(msg, &pTimer->timerExpiry);
    if(rc != CL_OK)
        goto out_free;

    if(pTimer->timerExpiry)
    {
        pTimer->timerExpiry += clOsalStopWatchTimeGet(); /*set absolute timer expiry*/
    }
    rc = clXdrUnmarshallClUint32T(msg, &pTimer->timerDataSize);
    if(rc != CL_OK)
        goto out_free;
    pTimer->timerDataPersistent = NULL;
    if(pTimer->timerDataSize > 0)
    {
        pTimer->timerDataPersistent = clHeapCalloc(pTimer->timerDataSize, sizeof(ClUint8T));
        CL_ASSERT(pTimer->timerDataPersistent != NULL);
        rc = clXdrUnmarshallArrayClUint8T(msg, pTimer->timerDataPersistent, pTimer->timerDataSize);
        if(rc != CL_OK)
        {
            goto out_free;
        }
    }

    if(!*pTimerHandle)
        *pTimerHandle = (ClTimerHandleT)pTimer;

    return CL_OK;

    out_free:
    *pTimerHandle = 0;
    if(pTimer->timerDataPersistent)
        clHeapFree(pTimer->timerDataPersistent);
    clHeapFree(pTimer);
    return rc;
}

static ClRcT timerClusterUnpackAll(ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClUint32T clusterTimers = 0;
    ClTimerT *pClusterTimers = NULL;
    ClTimerT *pFreeTimer = NULL;
    CL_LIST_HEAD_DECLARE(lastSyncList);
    ClInt32T i = 0;
    rc = clXdrUnmarshallClUint32T(msg, &clusterTimers);
    if(rc != CL_OK)
        goto out;
    if(!clusterTimers)
        goto out;
    pClusterTimers = clHeapCalloc(clusterTimers, sizeof(*pClusterTimers));
    CL_ASSERT(pClusterTimers != NULL);
    for(i = 0; i < clusterTimers; ++i)
    {
        ClTimerHandleT timer = (ClTimerHandleT)&pClusterTimers[i];
        rc = timerClusterUnpack(msg, &timer);
        if(rc != CL_OK)
            goto out_free;
    }
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clListMoveInit(&gTimerBase.clusterSyncList, &lastSyncList);
    for(i = 0; i < clusterTimers; ++i)
    {
        clListAddTail(&pClusterTimers[i].timerClusterList, &gTimerBase.clusterSyncList);
    }
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    while(!CL_LIST_HEAD_EMPTY(&lastSyncList))
    {
        ClListHeadT *head = lastSyncList.pNext;
        ClTimerT *pTimer = CL_LIST_ENTRY(head, ClTimerT, timerClusterList);
        clListDel(&pTimer->timerClusterList);
        if(pTimer->timerDataPersistent)
            clHeapFree(pTimer->timerDataPersistent);
        if(!pFreeTimer) 
            pFreeTimer = pTimer; /*mark the first entry*/
    }
    if(pFreeTimer)
        clHeapFree(pFreeTimer);
    return CL_OK;

    out_free:
    for(; pClusterTimers && i >= 0; --i)
    {
        if(pClusterTimers[i].timerDataPersistent)
            clHeapFree(pClusterTimers[i].timerDataPersistent);
    }
    if(pClusterTimers)
        clHeapFree(pClusterTimers);
    out:
    return rc;
}

/*
 * Unpack the cluster timers received on a cluster sync.
 * Called on the slaves.
 */
ClRcT clTimerClusterUnpackAll(ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClUint8T clusterTimerVersion = 0;
    
    rc = clXdrUnmarshallClUint8T(msg, &clusterTimerVersion);
    if(rc != CL_OK)
    {
        goto out;
    }
    switch(clusterTimerVersion)
    {
    case CL_TIMER_CLUSTER_VERSION:
        timerClusterUnpackAll(msg);
        break;
    default:
        clLogError("TIMER", "CLUSTER", "Cluster timer unpack version mismatch."
                   "Expected [%d]. Got [%d]", CL_TIMER_CLUSTER_VERSION, clusterTimerVersion);
        rc = CL_TIMER_RC(CL_ERR_VERSION_MISMATCH);
        goto out;
    }

    out:
    return rc;
}

ClRcT clTimerClusterUnpack(ClBufferHandleT msg, ClTimerHandleT *pTimerHandle)
{
    if(!pTimerHandle)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    return timerClusterUnpack(msg, pTimerHandle);
}
 
ClRcT clTimerClusterPackAll(ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    if(!gTimerBase.initialized)
        return CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);
    clOsalMutexLock(&gTimerBase.clusterListLock);
    rc = timerClusterPackAll(msg);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return rc;
}                       

ClRcT clTimerClusterPack(ClTimerHandleT timer, ClBufferHandleT msg)
{
    ClRcT rc = CL_OK;
    ClTimerT *pTimer = NULL;
    if(!gTimerBase.initialized)
        return CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);
    if(!timer)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    pTimer = (ClTimerT*)timer;
    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    clTimeUpdate();
    rc = timerClusterPack(pTimer, msg);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    return rc;
}                       

ClRcT clTimerClusterFree(ClTimerHandleT *pHandle)
{
    ClTimerT *pTimer = NULL;
    if(!pHandle || !*pHandle)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    pTimer = (ClTimerT*)*pHandle;
    if(pTimer->timerDataPersistent)
        clHeapFree(pTimer->timerDataPersistent);
    clHeapFree(pTimer);
    *pHandle = 0;
    return CL_OK;
}

/*
 * Called with lock held.
 */
static ClRcT timerClusterConfigure(ClTimerT *pSyncTimer, ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeout = {0};
    ClTimerHandleT timerHandle = 0;

    timeout.tsSec = 0;
    timeout.tsMilliSec = pSyncTimer->timerTimeOut/1000;
    rc = timerCreate(timeout, pSyncTimer->timerType, pSyncTimer->timerContext, gTimerBase.clusterCallback,
                     pSyncTimer->timerDataPersistent, pSyncTimer->timerDataSize, CL_TRUE, &timerHandle);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "CLUSTER", "Cluster timer reconfig create returned with [%#x]", rc);
        goto out;
    }
    timerClusterAdd((ClTimerT*)timerHandle, CL_FALSE, CL_TRUE);
    if(pSyncTimer->timerExpiry > 0)
    {
        ClTimeT now = clOsalStopWatchTimeGet();
        if(pSyncTimer->timerExpiry > now)
        {
            rc = timerStart(timerHandle, pSyncTimer->timerExpiry, CL_FALSE, CL_TRUE);
            if(rc != CL_OK)
            {
                clLogError("TIMER", "CLUSTER", "Cluster timer reconfig start returned with [%#x]", rc);
                goto out;
            }
        }
    }

    if(pTimerHandle)
        *pTimerHandle = timerHandle;

    out:
    return rc;
}

ClRcT clTimerClusterConfigureAll(void)
{
    ClTimerT *pFreeTimer = NULL;

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    while(!CL_LIST_HEAD_EMPTY(&gTimerBase.clusterSyncList))
    {
        ClListHeadT *head = gTimerBase.clusterSyncList.pNext;
        ClTimerT *pTimer = CL_LIST_ENTRY(head, ClTimerT, timerClusterList);
        if(!pFreeTimer)
            pFreeTimer = pTimer;
        clListDel(&pTimer->timerClusterList);
        timerClusterConfigure(pTimer, NULL);
    }
    timerClusterTaskSchedule(NULL, CL_FALSE);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);
    
    if(pFreeTimer)
        clHeapFree(pFreeTimer);

    return CL_OK;
}

ClRcT clTimerClusterConfigure(ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_OK;
    ClTimerHandleT timerHandle = 0;

    if(!pTimerHandle || !*pTimerHandle)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    
    timerHandle = *pTimerHandle;

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    
    rc = timerClusterConfigure((ClTimerT*)timerHandle, pTimerHandle);

    timerClusterTaskSchedule((ClTimerT*)*pTimerHandle, CL_FALSE);

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    clHeapFree((ClTimerT*)timerHandle);

    return rc;
}

ClRcT clTimerStatsGet(ClTimerStatsT **ppStats, ClUint32T *pNumTimers)
{
    ClUint32T numTimers = 0;
    ClRbTreeT *iter;
    ClTimerStatsT *pStats = NULL;

    if(!ppStats || !pNumTimers)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    
    pStats = clHeapCalloc(16, sizeof(*pStats));
    CL_ASSERT(pStats != NULL);

    clOsalMutexLock(&gTimerBase.clusterListLock);
    clOsalMutexLock(&gTimerBase.timerListLock);
    clTimeUpdate();
    CL_RBTREE_FOR_EACH(iter, &gTimerBase.timerTree)
    {
        ClTimerT *entry = CL_RBTREE_ENTRY(iter, ClTimerT, timerList);
        pStats[numTimers].timeOut = entry->timerTimeOut;
        if(gTimerBase.now >= entry->timerExpiry)
            pStats[numTimers].expiry = 0;
        else
            pStats[numTimers].expiry =  entry->timerExpiry - gTimerBase.now;
        pStats[numTimers].type = entry->timerType;
        pStats[numTimers].context = entry->timerContext;
        ++numTimers;
        if(!(numTimers & 15))
        {
            pStats = clHeapRealloc(pStats, sizeof(*pStats) * (numTimers+16));
            CL_ASSERT(pStats != NULL);
        }
    }
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    clOsalMutexUnlock(&gTimerBase.clusterListLock);

    *pNumTimers = numTimers;
    *ppStats = pStats;
    return CL_OK;
}
