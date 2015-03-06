/*
 * A scalable timer implementation using red black trees
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "clTimerTestCommon.h"
#include "clTimerTestCommonErrors.h"
#include "clTimerTestRbtree.h"

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

#define CL_TIMER_FREQUENCY (10)  /*10 millisecs*/

typedef struct ClTimer
{
    /*index into the timer tree*/
    ClRbTreeT timerList;
    /*index into the repetitive list*/
    ClListHeadT timerRepList;
    ClTimerTypeT timerType;
    ClTimerContextT timerContext;
    ClTimeT timerTimeOut;
    /*absolute expiry of the timer*/
    ClTimeT timerExpiry;
    ClTimerCallBackT timerCallback;
    ClPtrT timerData;
    ClBoolT timerRunning;

    /* debug data*/
    ClTimeT startTime;
    ClTimeT startRepTime;
    ClTimeT endTime;
}ClTimerT;

typedef struct ClTimerBase
{
#define CL_TIMER_MAX_CALLBACK_TASKS (0x20)
    ClBoolT initialized;
    ClBoolT timerRunning;
    pthread_mutex_t timerListLock;
    ClTimeT now;
    ClRbTreeRootT timerTree;
    pthread_t timerId;
    ClUint32T runningTimers;
    ClTimeT frequency;
}ClTimerBaseT;

static ClBoolT gClTimerDebug = CL_FALSE;
static pthread_mutex_t gClTimerDebugLock;
static ClInt32T clTimerCompare(ClRbTreeT *node, ClRbTreeT *entry);
static ClTimerBaseT gTimerBase = { .timerTree = CL_RBTREE_INITIALIZER(gTimerBase.timerTree, clTimerCompare) };

static ClInt32T clTimerCompare(ClRbTreeT *refTimer, ClRbTreeT *timer)
{
    ClTimerT *timer1  = CL_RBTREE_ENTRY(refTimer, ClTimerT, timerList);
    ClTimerT *timer2 =  CL_RBTREE_ENTRY(timer, ClTimerT, timerList);
    return (ClInt32T) (timer1->timerExpiry - timer2->timerExpiry);
}

static __inline__ void clTimeUpdate(void)
{
    gTimerBase.now = clTimerStopWatchTimeGet();
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
        CL_TIMER_PRINT("Timer create failed: Bad timer handle storage\n");
        goto out;
    }

    if(!timerCallback)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
        CL_TIMER_PRINT("Timer create failed: Null callback function passed\n");
        goto out;
    }

    if(timerType >= CL_TIMER_MAX_TYPE)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
        CL_TIMER_PRINT("Timer create failed: Bad timer type\n");
        goto out;
    }

    if(timerContext >= CL_TIMER_MAX_CONTEXT)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
        CL_TIMER_PRINT("Timer create failed: Bad timer context\n");
        goto out;
    }

    rc = CL_TIMER_RC(CL_ERR_NO_MEMORY);
    pTimer = clHeapCalloc(1,sizeof(*pTimer));
    if(pTimer == NULL)
    {
        CL_TIMER_PRINT("Allocation failed\n");
        goto out;
    }
    expiry = (ClTimeT)((ClTimeT)timeOut.tsSec *1e6 +  timeOut.tsMilliSec*1e3);
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
        CL_TIMER_PRINT("Timer start failed: Bad timer handle storage\n");
        goto out;
    }
    pTimer = (ClTimerT*)timerHandle;


    pthread_mutex_lock(&gTimerBase.timerListLock);

    CL_TIMER_SET_EXPIRY(pTimer);

    /* 
     * add to the rb tree.
     */
    clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);

    if(gClTimerDebug)
        pTimer->startTime = clTimerStopWatchTimeGet();

    pthread_mutex_unlock(&gTimerBase.timerListLock);

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
        CL_TIMER_PRINT("Timer update failed: Bad timer handle storage\n");
        goto out;
    }
    /*
     * First stop the timer.
     */
    clTimerStop(timerHandle);

    pthread_mutex_lock(&gTimerBase.timerListLock);
    pTimer->timerTimeOut = (ClTimeT)((ClTimeT)newTimeOut.tsSec * 1e6 + newTimeOut.tsMilliSec * 1e3);
    CL_TIMER_SET_EXPIRY(pTimer);
    
    clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);

    if(gClTimerDebug)
        pTimer->startTime = clTimerStopWatchTimeGet();

    pthread_mutex_unlock(&gTimerBase.timerListLock);

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
        CL_TIMER_PRINT("Timer stop failed: Bad timer handle storage\n");
        goto out;
    }
    
    pTimer = (ClTimerT*)timerHandle;
    rc = CL_OK;

    pthread_mutex_lock(&gTimerBase.timerListLock);
    /*Reset timer expiry. and rip this guy off from the list.*/
    pTimer->timerExpiry = 0;
    clRbTreeDelete(&gTimerBase.timerTree, &pTimer->timerList);
    pthread_mutex_unlock(&gTimerBase.timerListLock);
    
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
        CL_TIMER_PRINT("Timer delete failed: Bad timer handle storage\n");
        goto out;
    }
    pTimer = (ClTimerT*)*pTimerHandle;

    if(!pTimer)
    {
        rc = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        CL_TIMER_PRINT("Timer delete failed: Bad timer handle\n");
        goto out;
    }
    rc = CL_TIMER_RC(CL_ERR_OP_NOT_PERMITTED);

    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        CL_TIMER_PRINT("Timer delete failed: Volatile auto-delete timer tried to be deleted\n");
        goto out;
    }
    
    pthread_mutex_lock(&gTimerBase.timerListLock);
    clRbTreeDelete(&gTimerBase.timerTree, &pTimer->timerList);
    pthread_mutex_unlock(&gTimerBase.timerListLock);

    rc = CL_OK;
    clHeapFree(pTimer);
    *pTimerHandle = 0;

    out:
    return rc;
}

static void *clTimerCallbackTask(ClPtrT invocation)
{
    ClTimerT *pTimer = invocation;
    if(gClTimerDebug)
    {
        pthread_mutex_lock(&gClTimerDebugLock);
        pTimer->endTime = clTimerStopWatchTimeGet();
        CL_TIMER_PRINT("Timer task invoked at [%lld] - [%lld.%lld] usecs. "
                       "Expiry [%lld] usecs\n", pTimer->endTime - pTimer->startTime,
                       (pTimer->endTime - pTimer->startTime)/1000000L,
                       (ClTimeT)((pTimer->endTime - pTimer->startTime) % 1000000L),
                       pTimer->timerTimeOut);
        pTimer->startTime = pTimer->startRepTime;
        pTimer->startRepTime = 0;
        pTimer->endTime = 0;
        pthread_mutex_unlock(&gClTimerDebugLock);
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
    return NULL;
}

static __inline__ void clTimerSpawn(ClTimerT *pTimer)
{
    pthread_attr_t attr;
    pthread_t id;
    CL_ASSERT(pthread_attr_init(&attr) == 0);
    CL_ASSERT(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
    pTimer->timerRunning = CL_TRUE;
    CL_ASSERT(pthread_create(&id, &attr, clTimerCallbackTask, (void*)pTimer) == 0);
}

/*
* Okay reque repetitive timers. Called with list lock held.
*/
static ClRcT clTimerRepetitiveTask(ClPtrT invocation)
{
    ClListHeadT *pHead = invocation;

    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pTimerListEntry = pHead->pNext;

        ClTimerT *pTimer = CL_LIST_ENTRY(pTimerListEntry,ClTimerT,timerRepList);
        
        clListDel(pTimerListEntry);

        CL_TIMER_SET_EXPIRY(pTimer);
        
        clRbTreeInsert(&gTimerBase.timerTree, &pTimer->timerList);

        if(gClTimerDebug)
        {
            pthread_mutex_lock(&gClTimerDebugLock);
            /*
             * Required for debugging if the separate spawn for the 
             * callback landed late than the requeue of the repetitive timer.
             *
             */
            if(pTimer->startTime)
            {
                pTimer->startRepTime = clTimerStopWatchTimeGet();
            }
            else
            {
                pTimer->startTime = clTimerStopWatchTimeGet();
            }
            pthread_mutex_unlock(&gClTimerDebugLock);
        }

    }

    clHeapFree(pHead);
    return CL_OK;
}

static __inline__ void clTimerRunRepetitiveQueue(ClListHeadT *list)
{
    if(!CL_LIST_HEAD_EMPTY(list))
    {
        ClListHeadT *repetitiveList = clHeapCalloc(1, sizeof(*repetitiveList));
        if(!repetitiveList) 
        {
            CL_TIMER_PRINT("Repetitive list allocation failed\n");
            return;
        }
        CL_LIST_HEAD_INIT(repetitiveList);
        clListMoveInit(list, repetitiveList);
        clTimerRepetitiveTask(repetitiveList);
    }
}

/*
 * Run the sorted timer list expiring timers. 
 * Called with the timer list lock held.
 */
static ClRcT clTimerRun(void)
{
    ClListHeadT repetitiveList = CL_LIST_HEAD_INITIALIZER(repetitiveList);
    ClInt32T recalcInterval = 63; /*recalculate time after expiring this much*/
    ClInt64T timers = 0;
    ClRbTreeT *iter = NULL;
    ClRbTreeT *next = NULL;
    ClRbTreeRootT *root = &gTimerBase.timerTree;
    ClTimeT now = gTimerBase.now;

    for(iter = clRbTreeMin(root); iter; iter = next)
    {
        ClTimerT *pTimer = NULL; 
        ClTimerCallBackT timerCallback;
        ClPtrT timerData;
        ClTimerTypeT timerType;
        ClTimerContextT timerContext;

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

        next = clRbTreeNext(root, iter);

        /*Knock off from this list*/
        clRbTreeDelete(root, iter);

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
                pTimer->endTime = clTimerStopWatchTimeGet();
                CL_TIMER_PRINT("Timer invoked at [%lld] - [%lld.%lld] usecs. "
                               "Expiry [%lld] usecs\n", pTimer->endTime - pTimer->startTime,
                               (pTimer->endTime - pTimer->startTime)/1000000L,
                               (pTimer->endTime - pTimer->startTime)%1000000L,
                               pTimer->timerTimeOut);
                pTimer->startTime = 0;
                pTimer->startRepTime = 0;
                pTimer->endTime = 0;
            }
            pTimer->timerRunning = CL_TRUE;
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
            clListAddTail(&pTimer->timerRepList, &repetitiveList);
            break;
        case CL_TIMER_ONE_SHOT:
        default:
            break;
        }
    }
    clTimerRunRepetitiveQueue(&repetitiveList);
    return CL_OK;
}

/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *clTimerTask(void *pArg)
{
    ClTimerTimeOutT timeOut = {.tsSec = 0, .tsMilliSec = gTimerBase.frequency };
    ClTimeT delay = timeOut.tsSec * CL_TIMER_USEC_PER_SEC + timeOut.tsMilliSec * CL_TIMER_USEC_PER_MSEC;
    pthread_mutex_lock(&gTimerBase.timerListLock);

    clTimeUpdate();

    while(gTimerBase.timerRunning == CL_TRUE)
    {
        pthread_mutex_unlock(&gTimerBase.timerListLock);

        CL_TIMER_DELAY(delay);

        pthread_mutex_lock(&gTimerBase.timerListLock);

        clTimeUpdate();
        
        clTimerRun();
    }

    /*Unreached actually*/
    pthread_mutex_unlock(&gTimerBase.timerListLock);

    return NULL;
}

static ClRcT clTimerBaseInitialize(ClTimerBaseT *pBase)
{
    ClRcT rc = CL_OK;
    pBase->frequency = CL_TIMER_FREQUENCY;
    pthread_mutex_init(&pBase->timerListLock, NULL);
    return rc;
}

ClRcT clTimerInitialize(void)
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
        pthread_mutex_init(&gClTimerDebugLock, NULL);
    }

    rc = clTimerBaseInitialize(&gTimerBase);
    if(rc != CL_OK)
    {
        goto out;
    }

    gTimerBase.timerRunning = CL_TRUE;

    clTimeUpdate();

    rc = (ClRcT)pthread_create(&gTimerBase.timerId, NULL, clTimerTask, NULL);

    if(rc != CL_OK)
    {
        gTimerBase.timerRunning = CL_FALSE;
        CL_TIMER_PRINT("Timer task create returned [%#x]\n", rc);
        goto out_free;
    }

    gTimerBase.initialized = CL_TRUE;

    return rc;

    out_free:

    if(gTimerBase.timerId)
    {
        gTimerBase.timerRunning = CL_FALSE;
        pthread_join(gTimerBase.timerId, NULL);
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
        pthread_mutex_lock(&gTimerBase.timerListLock);
        gTimerBase.timerRunning = CL_FALSE;
        pthread_mutex_unlock(&gTimerBase.timerListLock);

        if(gTimerBase.timerId)
        {
            pthread_join(gTimerBase.timerId, NULL);
        }
    }
    
    rc = CL_OK;

    out:
    return rc;
}

