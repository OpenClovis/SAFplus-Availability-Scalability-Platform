/*
 * A rewrite of the brain-dead clovis timer implementation.
 * This is a cascading timer implementation thats pretty much
 * similar to what linux kernel uses.A very fast and scalable
 * implementation of cascading timers as compared to the 
 * present implementation of timers.
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
#include <clJobQueue.h>
#include <clLogApi.h>
#include <clDbg.h>

#define CL_TIMER_INITIALIZED_CHECK(rc,label) do {   \
    if(gTimerBase.initialized == CL_FALSE)          \
    {                                               \
        rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);   \
        goto label;                                 \
    }                                               \
}while(0)

/*
 * This is the timer granularity expressed in ticks per sec.
*/
#define CL_TIMER_FREQUENCY (50)
#define CL_TIMER_MSEC_JIFFIES  (CL_TIMER_MSEC_PER_SEC/CL_TIMER_FREQUENCY)
#define CL_TIMER_MSEC_TO_JIFFIES(msec) ( (msec)/CL_TIMER_MSEC_JIFFIES)
#define CL_TIMER_TIMEOUT CL_TIMER_MSEC_JIFFIES
#define CL_TIMER_MSEC_PER_SEC (1000)
#define CL_TIMER_USEC_PER_MSEC (1000)

#if CL_TIMER_MSEC_JIFFIES == 0
#error "JIFFY cannot be 0. Change TIMER frequency"
#endif

#define CL_TIMER_USEC_PER_MSEC_SKEW ( CL_TIMER_USEC_PER_MSEC * CL_TIMER_MSEC_JIFFIES )

#if CL_TIMER_MSEC_JIFFIES == 1
#define CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES CL_TIMER_USEC_PER_MSEC_SKEW
#else
#define CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES (CL_TIMER_USEC_PER_MSEC_SKEW - CL_TIMER_USEC_PER_MSEC_SKEW/3)
#endif

/*
 * A microsec. penalty added to the skew every iteration.
*/
#define CL_TIMER_ITERATION_PENALTY (75)

#define CL_TIMER_SET_EXPIRY(timer) do {                     \
    ClTimeT baseJiffies = gTimerBase.jiffies ;              \
    (timer)->timerExpiry = baseJiffies +                    \
        CL_TIMER_MSEC_TO_JIFFIES((timer)->timerTimeOut);    \
    if(!(timer)->timerExpiry) (timer)->timerExpiry = 1;     \
}while(0)


/*
 * This is a cascading vector representation.
 * The representation of expiry relative to jiffies is as follows.
 * Vector 1 -  ( 2^0 to 2^8)
 * Vector 2 -  ( 2^8 to 2^14)
 * Vector 3 -  ( 2^14 to 2^20)
 * Vector 4 -  ( 2^20 to 2^26)
 * Vector 5 -  ( 2^26 to 2^32)
*/

#define CL_TIMER_ROOT_VECTOR_BITS (0x8)

#define CL_TIMER_INDIRECT_VECTOR_BITS (0x6)

#define CL_TIMER_ROOT_VECTOR_SIZE (1 << CL_TIMER_ROOT_VECTOR_BITS)

#define CL_TIMER_INDIRECT_VECTOR_SIZE ( 1 << CL_TIMER_INDIRECT_VECTOR_BITS)

#define CL_TIMER_ROOT_VECTOR_MASK (CL_TIMER_ROOT_VECTOR_SIZE-1)

#define CL_TIMER_INDIRECT_VECTOR_MASK (CL_TIMER_INDIRECT_VECTOR_SIZE-1)

#define CL_TIMER_INDIRECT_VECTOR_INDEX(jiffies, level)      \
    ( ( (jiffies) >> (CL_TIMER_ROOT_VECTOR_BITS+(level)*    \
                      CL_TIMER_INDIRECT_VECTOR_BITS))       \
      & CL_TIMER_INDIRECT_VECTOR_MASK )

#define CL_TIMER_ROOT_VECTOR_ADD(timer, index) do {                     \
    clListAddTail(&(timer)->timerList, &gTimerBase.rootVector1.list[(index)]); \
}while(0)

#define CL_TIMER_INDIRECT_VECTOR_ADD(timer,index,vector) do {           \
    clListAddTail(&(timer)->timerList,                                  \
                  &gTimerBase.indirectVector##vector.list[(index)]);    \
}while(0)

/*
 * Cascade all the timers downwards to the root Vector
*/

#define CL_TIMER_CASCADE(jiffies,index)            do {             \
    if(!(index)                                                     \
      &&                                                            \
       !clTimerCascade(&gTimerBase.indirectVector2,                 \
                       CL_TIMER_INDIRECT_VECTOR_INDEX(jiffies, 0))  \
       &&                                                           \
       !clTimerCascade(&gTimerBase.indirectVector3,                 \
                       CL_TIMER_INDIRECT_VECTOR_INDEX(jiffies, 1))  \
       &&                                                           \
       !clTimerCascade(&gTimerBase.indirectVector4,                 \
                       CL_TIMER_INDIRECT_VECTOR_INDEX(jiffies, 2))  \
       )                                                            \
    {                                                               \
        clTimerCascade(&gTimerBase.indirectVector5,                 \
                       CL_TIMER_INDIRECT_VECTOR_INDEX(jiffies, 3)); \
    }                                                               \
}while(0)

typedef struct ClTimer
{
    /*index into the list head*/
    ClListHeadT timerList;
    /* index into the stop queue*/
    ClListHeadT stopList;
    /* index into the delete queue */
    ClListHeadT deleteList;
    ClTimerTypeT timerType;
    ClTimerContextT timerContext;
    /*timeout converted to jiffies*/
    ClTimeT timerTimeOut;
    /*absolute expiry of the timer in jiffies or ticks*/
    ClTimeT timerExpiry;
    /*start jiffy snapshot at which the timer was up*/
    ClTimerCallBackT timerCallback;
    ClPtrT timerData;
    ClBoolT timerRunning;
    ClTimeT startTime;
    ClTimeT startRepTime;
    ClTimeT endTime;
}ClTimerT;

typedef struct ClTimerRootVector
{
    ClListHeadT list[CL_TIMER_ROOT_VECTOR_SIZE];
}ClTimerRootVectorT;

typedef struct ClTimerIndirectVector
{
    ClListHeadT list[CL_TIMER_INDIRECT_VECTOR_SIZE];
}ClTimerIndirectVectorT;

typedef struct ClTimerBase
{
#define CL_TIMER_MAX_PARALLEL_TASKS (0x1)
#define CL_TIMER_MAX_CALLBACK_TASKS (0x20)
#define CL_TIMER_MAX_FUNCTION_TASKS (0x3)
    ClBoolT initialized;
    ClOsalMutexT rootVectorLock;
    ClOsalCondT  rootVectorCond;
    ClTimeT jiffies;
    /*2^0 to 2^8*/
    ClTimerRootVectorT rootVector1;
    /*2^8 to 2^14*/
    ClTimerIndirectVectorT indirectVector2;
    /*2^14 to 2^20*/
    ClTimerIndirectVectorT indirectVector3;
    /*2^20 to 2^26*/
    ClTimerIndirectVectorT indirectVector4;
    /*2^26 to 2^32 - max expiry boundary is 2^32-1*/
    ClTimerIndirectVectorT indirectVector5;
    ClOsalTaskIdT timerId;
    ClUint32T runningTimers;
    ClBoolT expiryRun;
    ClBoolT timerRunning;
    ClListHeadT timerStopQueue;
    ClListHeadT timerDeleteQueue;
    ClJobQueueT *timerJobQueue;
    ClTaskPoolHandleT timerCallbackPool;
    ClTaskPoolHandleT timerFunctionsPool;
}ClTimerBaseT;

typedef struct ClTimerJob
{
    ClTimeT startJiffies;
    ClTimeT endJiffies;
}ClTimerJobT;

static ClBoolT gClTimerDebug = CL_FALSE;
static ClOsalMutexT gClTimerDebugLock;
static ClTimerBaseT gTimerBase;

/*
 * Add the timer to the vector.
 * Should be called with root Vector Lock held.
 * Much much faster than a sorted add that the current timer
 * uses.
*/

static void clTimerVectorAdd(ClTimerT *pTimer)
{
    ClUint64T index;
    ClUint64T timerExpiry = pTimer->timerExpiry;
    ClTimerBaseT *pBase = &gTimerBase;

    index = timerExpiry - pBase->jiffies;
    /*Add into the right place based on level of indirection*/
    if(index < CL_TIMER_ROOT_VECTOR_SIZE)
    {
        index =  timerExpiry & CL_TIMER_ROOT_VECTOR_MASK;
        CL_TIMER_ROOT_VECTOR_ADD(pTimer,index);
    }
    else if(index < 
            (1 << (CL_TIMER_ROOT_VECTOR_BITS+CL_TIMER_INDIRECT_VECTOR_BITS))
            )
    {
        index = (timerExpiry >> CL_TIMER_ROOT_VECTOR_BITS) & CL_TIMER_INDIRECT_VECTOR_MASK;
        CL_TIMER_INDIRECT_VECTOR_ADD(pTimer,index,2);
    }
    else if(index < 
            (1 << (CL_TIMER_ROOT_VECTOR_BITS+2*CL_TIMER_INDIRECT_VECTOR_BITS))
            )
    {
        index = (timerExpiry >> (CL_TIMER_ROOT_VECTOR_BITS+CL_TIMER_INDIRECT_VECTOR_BITS)) & CL_TIMER_INDIRECT_VECTOR_MASK;
        CL_TIMER_INDIRECT_VECTOR_ADD(pTimer,index,3);
    }
    else if(index < 
            (1 << (CL_TIMER_ROOT_VECTOR_BITS+3*CL_TIMER_INDIRECT_VECTOR_BITS))
            )
    {
        index = (timerExpiry >> (CL_TIMER_ROOT_VECTOR_BITS+2*CL_TIMER_INDIRECT_VECTOR_BITS)) & CL_TIMER_INDIRECT_VECTOR_MASK;
        CL_TIMER_INDIRECT_VECTOR_ADD(pTimer,index,4);
    }
    else if( (ClInt64T)index < 0 )
    {
        
        /*
         * expiry moving in the past.
         * Just add it to the root vector for getting processed
         * immediately
         */
        CL_TIMER_ROOT_VECTOR_ADD(pTimer,pBase->jiffies&CL_TIMER_ROOT_VECTOR_MASK);
    }
    else
    {
        /*
         * We go to the overflowing last vector for max expiry
         */
        if(index > 0xffffffffUL)
        {
            /*
             * Scale it down to the max. expiry.
             */
            index = 0xffffffffUL;
            timerExpiry = pBase->jiffies + index;
        }
        index = (timerExpiry >> (CL_TIMER_ROOT_VECTOR_BITS+3*CL_TIMER_INDIRECT_VECTOR_BITS)) & CL_TIMER_INDIRECT_VECTOR_MASK;
        CL_TIMER_INDIRECT_VECTOR_ADD(pTimer,index,5);
    }
}

/*
 * Cascade timers down to the lower end.
 * Should be called with root vector lock held.
 */

static ClUint32T clTimerCascade(ClTimerIndirectVectorT *pVector,
                                ClUint32T index
                                )
{
    ClListHeadT tempList = CL_LIST_HEAD_INITIALIZER(tempList);
    ClListHeadT *pNext,*pTemp;
    /*
     * move the elements off. this vector list.
     */
    clListMoveInit(pVector->list + index, &tempList);

    for(pTemp = tempList.pNext; pTemp != &tempList; pTemp = pNext)
    {
        ClTimerT *pTimer = CL_LIST_ENTRY(pTemp,ClTimerT,timerList);
        pNext = pTemp->pNext;
        /*
         * Add the timer to the vector which would effect a
         * revectorization.
         */
        clTimerVectorAdd(pTimer);
    }
    return index;
}

ClRcT clTimerCreate(ClTimerTimeOutT timeOut, 
                    ClTimerTypeT timerType,
                    ClTimerContextT timerContext,
                    ClTimerCallBackT timerCallback,
                    void *timerData,
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
    expiry = (ClTimeT)((ClTimeT)timeOut.tsSec * CL_TIMER_MSEC_PER_SEC+
                         timeOut.tsMilliSec);
    /*Convert this expiry to jiffies*/
    pTimer->timerTimeOut = expiry;
    pTimer->timerType = timerType;
    pTimer->timerContext = timerContext;
    pTimer->timerCallback = timerCallback;
    pTimer->timerData = timerData;
    *pTimerHandle = (ClTimerHandleT)pTimer;
    rc = CL_OK;

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

/*
 * Start the timer.
*/

ClRcT clTimerStart(ClTimerHandleT timerHandle)
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

    clOsalMutexLock(&gTimerBase.rootVectorLock);

    CL_TIMER_SET_EXPIRY(pTimer);

    clTimerVectorAdd(pTimer);

    if(gClTimerDebug)
        pTimer->startTime = clOsalStopWatchTimeGet();

    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    rc = CL_OK;

    out:
    return rc;
}

ClRcT clTimerUpdate(ClTimerHandleT timerHandle, ClTimerTimeOutT newTimeOut)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerT *pTimer = (ClTimerT*)timerHandle;

    CL_TIMER_INITIALIZED_CHECK(rc, out);

    if(!timerHandle)
    {
        clDbgCodeError(rc, ("Timer update failed: Bad timer handle storage"));
        goto out;
    }
    /*
     * First stop the timer.
     */
    clTimerStop(timerHandle);

    clOsalMutexLock(&gTimerBase.rootVectorLock);
    pTimer->timerTimeOut = (ClTimeT)((ClTimeT)newTimeOut.tsSec * CL_TIMER_MSEC_PER_SEC +
                                     newTimeOut.tsMilliSec);
    /*pTimer->timerTimeOut = CL_TIMER_MSEC_TO_JIFFIES(pTimer->timerTimeOut);*/
    CL_TIMER_SET_EXPIRY(pTimer);
    clTimerVectorAdd(pTimer);

    if(gClTimerDebug)
        pTimer->startTime = clOsalStopWatchTimeGet();

    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    out:
    return rc;
}

ClRcT clTimerStop(ClTimerHandleT timerHandle)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerT *pTimer = NULL;

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(timerHandle == 0)
    {
        clDbgCodeError(rc, ("Timer stop failed: Bad timer handle storage"));
        goto out;
    }
    
    pTimer = (ClTimerT*)timerHandle;

    rc = CL_OK;

    clOsalMutexLock(&gTimerBase.rootVectorLock);

    /*
     * Already there in stop list.
     */
    if(pTimer->stopList.pNext)
    {
        rc = CL_TIMER_RC(CL_ERR_ALREADY_EXIST);
        clDbgCodeError(rc, ("Timer stop failed: Timer already stopped"));
        goto out_unlock;
    }

    if(gTimerBase.expiryRun == CL_TRUE)
    {
        clListAddTail(&pTimer->stopList, &gTimerBase.timerStopQueue);
        goto out_unlock;
    }

    /*Reset timer expiry. and rip this guy off from the list.*/
    pTimer->timerExpiry = 0;

    clListDel(&pTimer->timerList);

    out_unlock:
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    out:
    return rc;
}

/*
 * Dont call it under a callback.
 */
ClRcT clTimerDelete(ClTimerHandleT *pTimerHandle)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerT *pTimer = NULL;
    
    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if(pTimerHandle == NULL)
    {
        clDbgCodeError(rc, ("Timer delete failed: Bad timer handle storage"));
        goto out;
    }
    pTimer = (ClTimerT*)*pTimerHandle;

    if(!pTimer)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(rc, ("Timer delete failed: Bad timer handle"));
        goto out;
    }
    rc = CL_TIMER_RC(CL_ERR_OP_NOT_PERMITTED);

    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        clDbgCodeError(rc, ("Timer delete failed: Volatile auto-delete timer tried to be deleted"));
        goto out;
    }
    
    clOsalMutexLock(&gTimerBase.rootVectorLock);

    if(pTimer->deleteList.pNext)
    {
        rc = CL_TIMER_RC(CL_ERR_ALREADY_EXIST);
        goto out_unlock;
    }

    /*
     * See if this can be deleted inline.
     */
    if(gTimerBase.expiryRun == CL_FALSE 
       &&
       pTimer->timerRunning == CL_FALSE)
    {
        clListDel(&pTimer->stopList);
        clListDel(&pTimer->timerList);
        clHeapFree(pTimer);
    }
    else
    {
        clListAddTail(&pTimer->deleteList, &gTimerBase.timerDeleteQueue);
    }

    rc = CL_OK;
    *pTimerHandle = 0;

    out_unlock:
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    out:
    return rc;
}

static ClRcT clTimerCallbackTask(ClPtrT invocation)
{
    ClTimerT *pTimer = invocation;
    if(gClTimerDebug)
    {
        clOsalMutexLock(&gClTimerDebugLock);
        pTimer->endTime = clOsalStopWatchTimeGet();
        clLogNotice("TIMER", "CALL", "Timer task invoked at [%lld] - [%lld.%lld] usecs. "
                    "Expiry [%lld] jiffies", pTimer->endTime - pTimer->startTime,
                    (pTimer->endTime - pTimer->startTime)/CL_TIMER_USEC_PER_MSEC,
                    (pTimer->endTime - pTimer->startTime) % CL_TIMER_USEC_PER_MSEC,
                    pTimer->timerTimeOut);
        pTimer->startTime = pTimer->startRepTime;
        pTimer->startRepTime = 0;
        pTimer->endTime = 0;
        clOsalMutexUnlock(&gClTimerDebugLock);
    }
    ++gTimerBase.runningTimers;
    pTimer->timerCallback(pTimer->timerData);
    --gTimerBase.runningTimers;
    pTimer->timerRunning = CL_FALSE;

    /*
     * Rip this guy if volatile.
     */
    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        clHeapFree(pTimer);
    }
    return CL_OK;
}

static __inline__ void clTimerSpawn(ClTimerT *pTimer)
{
    /*
     * Invoke the callback in the task pool context
     */
    pTimer->timerRunning = CL_TRUE;
    clTaskPoolRun(gTimerBase.timerCallbackPool, clTimerCallbackTask, pTimer);
}

/*
* Okay reque repetitive timers.
*/
static ClRcT clTimerRepetitiveTask(ClPtrT invocation)
{
    ClListHeadT *pHead = invocation;
    ClTimerBaseT *pBase = &gTimerBase;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0};

    clOsalMutexLock(&pBase->rootVectorLock);
    while(pBase->expiryRun == CL_TRUE)
    {
        clOsalCondWait(&pBase->rootVectorCond, &pBase->rootVectorLock, delay);
    }
    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pTimerListEntry = pHead->pNext;

        ClTimerT *pTimer = CL_LIST_ENTRY(pTimerListEntry,ClTimerT,timerList);
        
        clListDel(pTimerListEntry);

        CL_TIMER_SET_EXPIRY(pTimer);
        
        clTimerVectorAdd(pTimer);

        if(gClTimerDebug)
        {
            clOsalMutexLock(&gClTimerDebugLock);
            /*
             * Required for debugging if the separate spawn for the 
             * callback landed late than the requeue of the repetitive timer.
             *
             */
            if(pTimer->startTime)
            {
                pTimer->startRepTime = clOsalStopWatchTimeGet();
            }
            else
            {
                pTimer->startTime = clOsalStopWatchTimeGet();
            }
            clOsalMutexUnlock(&gClTimerDebugLock);
        }

    }
    clOsalMutexUnlock(&pBase->rootVectorLock);
    clHeapFree(pHead);
    return CL_OK;
}

/*
 * Stop the timers from the async stop queue.
 */
static ClRcT clTimerStopTask(ClPtrT unused)
{
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0 };

    clOsalMutexLock(&gTimerBase.rootVectorLock);
    if(!CL_LIST_HEAD_EMPTY(&gTimerBase.timerStopQueue))
    {
        while(gTimerBase.expiryRun == CL_TRUE)
        {
            clOsalCondWait(&gTimerBase.rootVectorCond, &gTimerBase.rootVectorLock, delay);
        }
        while(!CL_LIST_HEAD_EMPTY(&gTimerBase.timerStopQueue))
        {
            ClTimerT *pTimer = CL_LIST_ENTRY(gTimerBase.timerStopQueue.pNext, ClTimerT, stopList);
            clListDel(&pTimer->stopList);
            /*
             * Delete from the timer list.
             */
            clListDel(&pTimer->timerList);
        }
    }
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);
    return CL_OK;
}

static ClRcT clTimerDeleteTask(ClPtrT unused)
{
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0};
    /*
     * There could be timers pending to be spawned. So we requeue deletes.
     */
    ClListHeadT runningQueue = CL_LIST_HEAD_INITIALIZER(runningQueue);
    clOsalMutexLock(&gTimerBase.rootVectorLock);
    if(!CL_LIST_HEAD_EMPTY(&gTimerBase.timerDeleteQueue))
    {
        while(gTimerBase.expiryRun == CL_TRUE)
        {
            clOsalCondWait(&gTimerBase.rootVectorCond, &gTimerBase.rootVectorLock, delay);
        }
        while(!CL_LIST_HEAD_EMPTY(&gTimerBase.timerDeleteQueue))
        {
            ClTimerT *pTimer = CL_LIST_ENTRY(gTimerBase.timerDeleteQueue.pNext, ClTimerT, deleteList);
            clListDel(&pTimer->deleteList);
            clListDel(&pTimer->stopList);
            clListDel(&pTimer->timerList);
            /*
             * Requeue running timers.
             */
            if(pTimer->timerRunning == CL_TRUE)
            {
                clListAddTail(&pTimer->deleteList, &runningQueue);
                continue;
            }
            clHeapFree(pTimer);
        }
        clListMoveInit(&runningQueue, &gTimerBase.timerDeleteQueue);
    }
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);
    return CL_OK;
}

static __inline__ void clTimerRunDeleteQueue(void)
{
    clTaskPoolRun(gTimerBase.timerFunctionsPool, clTimerDeleteTask, 0);
}

static __inline__ void clTimerRunStopQueue(void)
{
    clTaskPoolRun(gTimerBase.timerFunctionsPool, clTimerStopTask, 0);
}

static __inline__ void clTimerRunRepetitiveQueue(ClListHeadT *list)
{
    if(!CL_LIST_HEAD_EMPTY(list))
    {
        ClListHeadT *repetitiveList = clHeapCalloc(1, sizeof(*repetitiveList));
        if(!repetitiveList) 
        {
            clLogError("TIMER", "TASK", "Repetitive list allocation failed");
            return;
        }
        CL_LIST_HEAD_INIT(repetitiveList);
        clListMoveInit(list, repetitiveList);
        clTaskPoolRun(gTimerBase.timerFunctionsPool, clTimerRepetitiveTask, 
                      repetitiveList);
    }
}

/*
 * Run the timers through cascading.
 */
static ClRcT clTimerRun(ClPtrT invocation)
{
    ClUint32T index;
    ClTimerBaseT *pBase = &gTimerBase;
    ClListHeadT repetitiveList = CL_LIST_HEAD_INITIALIZER(repetitiveList);
    ClTimeT startJiffies;
    ClTimeT endJiffies;

    startJiffies = ((ClTimerJobT*)invocation)->startJiffies;
    endJiffies = ((ClTimerJobT*)invocation)->endJiffies;

    clHeapFree(invocation);
    /*
     * Account for the last run
     */
    if(startJiffies) ++startJiffies;

    /*
     * Process pending stops/deletes. first
     */
    clTimerRunStopQueue();
    clTimerRunDeleteQueue();

    clOsalMutexLock(&pBase->rootVectorLock);

    gTimerBase.expiryRun = CL_TRUE;

    while(startJiffies <= endJiffies)
    {
        ClListHeadT expiredList = CL_LIST_HEAD_INITIALIZER(expiredList);

        index = startJiffies & CL_TIMER_ROOT_VECTOR_MASK;
        /*
         * cascade the timers if possible.
         * Done once in CL_TIMER_ROOT_VECTOR_SIZE interval.
         */
        if(startJiffies)
        {
            CL_TIMER_CASCADE(startJiffies, index);
        }

        /*
         * We knock the expired list off and drop the lock.
         */
        clListMoveInit(&pBase->rootVector1.list[index], &expiredList);
        clOsalMutexUnlock(&pBase->rootVectorLock);

        /*
         * We iterate this list with root vector lock held and process
         * expired timers.
         */

        while(!CL_LIST_HEAD_EMPTY(&expiredList))
        {
            ClListHeadT *pTimerListEntry = expiredList.pNext;
            ClTimerT *pTimer = CL_LIST_ENTRY(pTimerListEntry,ClTimerT,timerList);
            ClTimerCallBackT timerCallback = pTimer->timerCallback;
            ClPtrT timerData = pTimer->timerData;
            ClTimerTypeT timerType = pTimer->timerType;
            ClTimerContextT timerContext = pTimer->timerContext;

            /*Knock off from this list*/
            clListDel(pTimerListEntry);
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
                pTimer->timerRunning = CL_TRUE;
                if(gClTimerDebug)
                {
                    pTimer->endTime = clOsalStopWatchTimeGet();
                    clLogNotice("TIMER", "CALL", "Timer invoked at [%lld] - [%lld.%lld] usecs. "
                                "Expiry [%lld] jiffies", pTimer->endTime - pTimer->startTime,
                                (pTimer->endTime - pTimer->startTime)/CL_TIMER_USEC_PER_MSEC,
                                (pTimer->endTime - pTimer->startTime) % CL_TIMER_USEC_PER_MSEC,
                                pTimer->timerTimeOut);
                    pTimer->startTime = 0;
                    pTimer->startRepTime = 0;
                    pTimer->endTime = 0;
                }
                timerCallback(timerData);
                pTimer->timerRunning = CL_FALSE;
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
                    clHeapFree(pTimer);
                }
                break;
            case CL_TIMER_REPETITIVE:
                clListAddTail(&pTimer->timerList, &repetitiveList);
                break;
            case CL_TIMER_ONE_SHOT:
            default:
                break;
            }
        }
        ++startJiffies;
        clOsalMutexLock(&pBase->rootVectorLock);
    }
    /*
     * Now run the delete timer list.
     */
    gTimerBase.expiryRun = CL_FALSE;
    clOsalCondBroadcast(&gTimerBase.rootVectorCond);
    clOsalMutexUnlock(&pBase->rootVectorLock);

    clTimerRunRepetitiveQueue(&repetitiveList);
    return CL_OK;
}

static void clTimerJobQueue(ClTimeT startJiffies, ClTimeT endJiffies)
{
    ClTimerJobT *pJob = NULL;
    pJob = clHeapCalloc(1, sizeof(*pJob));
    if(!pJob) return;
    pJob->startJiffies = startJiffies;
    pJob->endJiffies = endJiffies;
    clJobQueuePush(gTimerBase.timerJobQueue, clTimerRun, pJob);
}


/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *clTimerTask(void *pArg)
{
    ClTimeT diff = 0;
    ClTimerTimeOutT timeOut = {.tsSec = 0, .tsMilliSec = CL_TIMER_TIMEOUT };
    static ClTimeT skew = 0;
    ClTimeT startTime = 0;
    ClTimeT endTime = 0;

    clOsalMutexLock(&gTimerBase.rootVectorLock);

    startTime = clOsalStopWatchTimeGet();

    while(gTimerBase.timerRunning == CL_TRUE)
    {
        ClTimeT startJiffies = 0;
        ClTimeT endJiffies = 0;

        clOsalMutexUnlock(&gTimerBase.rootVectorLock);

        clOsalTaskDelay(timeOut);

        endTime = clOsalStopWatchTimeGet();

        diff = endTime - startTime;

        /*
         * Start the countdown for the next iteration.
         */
        startTime = clOsalStopWatchTimeGet();

        clOsalMutexLock(&gTimerBase.rootVectorLock);
        startJiffies = gTimerBase.jiffies;
        gTimerBase.jiffies += CL_TIMER_MSEC_TO_JIFFIES(diff/CL_TIMER_USEC_PER_MSEC);
        skew += (diff % CL_TIMER_USEC_PER_MSEC);
        skew += CL_TIMER_ITERATION_PENALTY;
        /*
         * Account for a tick for the nearest millisec
         */
        if(skew >= CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES)
        {
            gTimerBase.jiffies += skew/CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES;
            skew -= (skew/CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES * CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES);
        }
        endJiffies = gTimerBase.jiffies;

        clOsalMutexUnlock(&gTimerBase.rootVectorLock);

        clTimerJobQueue(startJiffies, endJiffies);
    }

    /*Unreached actually*/
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    return NULL;
}

/*
 * First step. initialize the base vector
 */
static __inline__ void clTimerIndirectVectorInitialize(ClTimerIndirectVectorT *pVector)
{
    register ClInt32T i;
    for(i = 0 ; i < CL_TIMER_INDIRECT_VECTOR_SIZE;++i)
    {
        ClListHeadT *pHead = pVector->list+i;
        CL_LIST_HEAD_INIT(pHead);
    }
}

static __inline__ void clTimerRootVectorInitialize(ClTimerRootVectorT *pVector)
{
    register ClInt32T i;
    for(i = 0; i < CL_TIMER_ROOT_VECTOR_SIZE;++i)
    {
        ClListHeadT *pHead = pVector->list + i;
        CL_LIST_HEAD_INIT(pHead);
    }
}

ClRcT clTimerBaseInitialize(ClTimerBaseT *pBase)
{
    ClRcT rc = CL_OK;
    pBase->jiffies = 0;
    pBase->expiryRun = CL_FALSE;
    pBase->timerCallbackPool = 0;
    pBase->timerJobQueue = 0;
    pBase->timerFunctionsPool = 0;

    clOsalMutexInit(&pBase->rootVectorLock);
    rc = clOsalCondInit(&pBase->rootVectorCond);
    if(rc != CL_OK)
    {
        clOsalMutexDestroy(&pBase->rootVectorLock);
        return rc;
    }
    CL_LIST_HEAD_INIT(&pBase->timerDeleteQueue);
    CL_LIST_HEAD_INIT(&pBase->timerStopQueue);
    clTimerRootVectorInitialize(&pBase->rootVector1);
    clTimerIndirectVectorInitialize(&pBase->indirectVector2);
    clTimerIndirectVectorInitialize(&pBase->indirectVector3);
    clTimerIndirectVectorInitialize(&pBase->indirectVector4);
    clTimerIndirectVectorInitialize(&pBase->indirectVector5);
    return rc;
}

ClRcT clTimerInitialize(ClPtrT config)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INITIALIZED);

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
    rc = clJobQueueCreate(&gTimerBase.timerJobQueue, 0, CL_TIMER_MAX_PARALLEL_TASKS);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "INI", "Timer job queue create returned [%#x]", rc);
        return rc;
    }
    clTaskPoolInitialize();
    rc = clTaskPoolCreate(&gTimerBase.timerCallbackPool, CL_TIMER_MAX_CALLBACK_TASKS, 0, 0);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "INI", "Timer callback pool create returned [%#x]", rc);
        goto out_free;
    }
    rc = clTaskPoolCreate(&gTimerBase.timerFunctionsPool, CL_TIMER_MAX_FUNCTION_TASKS, 0, 0);
    if(rc != CL_OK)
    {
        clLogError("TIMER", "INI", "Timer functions pool create returned [%#x]", rc);
        goto out_free;
    }

    gTimerBase.timerRunning = CL_TRUE;
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

    if(gTimerBase.timerJobQueue)
        clJobQueueDelete(gTimerBase.timerJobQueue);
    if(gTimerBase.timerCallbackPool)
        clTaskPoolDelete(gTimerBase.timerCallbackPool);
    if(gTimerBase.timerFunctionsPool)
        clTaskPoolDelete(gTimerBase.timerFunctionsPool);
    
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

    clOsalMutexLock(&gTimerBase.rootVectorLock);
    gTimerBase.initialized = CL_FALSE;
    gTimerBase.expiryRun = CL_FALSE;
    clOsalMutexUnlock(&gTimerBase.rootVectorLock);

    /*
     * We shouldnt care about deleting pending timers
     * as finalize of individual guys should take care.
     * Can be done, but with another list head which I dont think its required.
     */
    if(gTimerBase.timerRunning == CL_TRUE)
    {
        gTimerBase.timerRunning = CL_FALSE;
        if(gTimerBase.timerId)
            clOsalTaskJoin(gTimerBase.timerId);
    }

    clOsalCondBroadcast(&gTimerBase.rootVectorCond);

    if(gTimerBase.timerJobQueue)
        clJobQueueDelete(gTimerBase.timerJobQueue);
    if(gTimerBase.timerCallbackPool)
        clTaskPoolDelete(gTimerBase.timerCallbackPool);
    if(gTimerBase.timerFunctionsPool)
        clTaskPoolDelete(gTimerBase.timerFunctionsPool);

    rc = CL_OK;

    out:
    return rc;
}

