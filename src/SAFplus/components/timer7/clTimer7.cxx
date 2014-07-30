/*
 * A scalable timer implementation using red black trees
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <clTimerApi.h>
#include <clTimerErrors.h>
#include <clList.h>
#include <clRbTree.h>
#include "clTimer7.hxx"

extern ClBoolT gIsNodeRepresentative;
static ClBoolT gClTimerDebug = CL_FALSE;
static ClOsalMutexT gClTimerDebugLock;
static ClHandleT gTimerDebugReg;
ClInt32T timerMinParallelThread=3000;
ClInt32T timerMaxParallelThread=20000;

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


SAFplus::TimerBase gTimerBase;


SAFplus::TimerBase::TimerBase(): pool(timerMinParallelThread,timerMaxParallelThread)
{

}
ClRcT SAFplus::TimerBase::TimerBaseInitialize()
{
    ClRcT rc = CL_OK;
    this->frequency = CL_TIMER_FREQUENCY;
    return rc;
}

/*
 * Run the sorted timer list expiring timers.
 * Called with the timer list lock held.
 */

ClRcT SAFplus::TimerBase::timerRun(void)
{
    ClInt32T recalcInterval = 15; /*recalculate time after expiring this much*/
    ClInt64T timers = 0;
    //ClRbTreeT *iter = NULL;
    //ClRbTreeRootT *root = &this->timerTree;
    //boost::intrusive::rbtree<Timer>::iterator iter();
    ClTimeT now = this->now;
    /*
     * We take the minimum treenode at each iteration since we will drop the lock
     * while invoking the callback and it could so happen that the next
     * entry that we cached gets ripped off thereby forcing us to resort to
     * some dramatics while coding. So lets keep it clean and take the minimum
     * from the tree node.
     */

    while (1)
    {
        Timer *pTimer = this->get_rbtree_min();
        if(!pTimer)
        {
            break;
        }
        ClTimerCallBackT timerCallback;
        ClPtrT timerData;
        ClTimerTypeT timerType;
        ClTimerContextT timerContext;
        ClInt16T callbackTaskIndex = -1;
        if( !(++timers & recalcInterval) )
        {
            this->timeUpdate();
            now = this->now;
        }
        if(pTimer->timerExpiry > now)
        {
            break;
        }
        timerCallback = pTimer->timerCallback;
        timerData = pTimer->timerData;
        timerType = pTimer->timerType;
        timerContext = pTimer->timerContext;
        /*Knock off from this list*/
        this->timerTree.erase(*pTimer);
        if(timerType == CL_TIMER_REPETITIVE && timerContext == CL_TIMER_TASK_CONTEXT)
        {
            CL_REPETITIVE_TIMER_SET_EXPIRY(pTimer);
            this->timerTree.insert_unique(*pTimer);
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
            callbackTaskIndex = pTimer->timerAddCallbackTask();

        /*
         * Drop the lock now.
         */
        clOsalMutexUnlock(&this->timerListLock);

        /*
         * Now fire the sucker up.
         */
        CL_ASSERT(timerCallback != NULL);

        switch(timerContext)
        {
        case CL_TIMER_SEPARATE_CONTEXT:
            {
                this->timerSpawn(pTimer);
            }
            break;
        case CL_TIMER_TASK_CONTEXT:
        default:
            if(gClTimerDebug)
            {
                pTimer->endTime = clOsalStopWatchTimeGet();
                logInfo("TIMER7","RUN", "Timer invoked at [%lld] - [%lld.%lld] usecs. "
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
                delete pTimer;
            }
            break;
        case CL_TIMER_REPETITIVE:
        default:
            break;
        }
        clOsalMutexLock(&this->timerListLock);

        /*
         * In order to avoid a leak when a delete request for a ONE shot/same context
         * comes when we dropped the lock, we double check for a pending delete
         */
        if(timerType != CL_TIMER_VOLATILE && timerContext == CL_TIMER_TASK_CONTEXT)
        {
            if(callbackTaskIndex >= 0)
                pTimer->timerDelCallbackTask(callbackTaskIndex);

            --pTimer->timerRefCnt;

            pTimer->timerFlags &= ~CL_TIMER_RUNNING;
        }
    }
    return CL_OK;
}

ClRcT timerCallbackTask(ClPtrT invocation)
{
    SAFplus::Timer *pTimer = (SAFplus::Timer*)invocation;
    ClTimerTypeT type = pTimer->timerType;
    ClBoolT canFree = CL_FALSE;
    ClInt16T callbackTaskIndex = -1;
    clOsalMutexLock(&gTimerBase.timerListLock);

    if(gClTimerDebug)
    {
        clOsalMutexLock(&gClTimerDebugLock);
        pTimer->endTime = clOsalStopWatchTimeGet();
        logInfo("TIMER7", "CALL", "Timer task invoked at [%lld] - [%lld.%lld] usecs. "
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
        return CL_OK;
    }
    else if( (pTimer->timerFlags & CL_TIMER_STOPPED) )
    {
        --pTimer->timerRefCnt;
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        return CL_OK;
    }

    ++gTimerBase.runningTimers;
    callbackTaskIndex = pTimer->timerAddCallbackTask();

    /*
     * If its a repetitive timer, add back into the list just before callback invocation.
     */
    if(type == CL_TIMER_REPETITIVE)
    {
        gTimerBase.timeUpdate();
        CL_REPETITIVE_TIMER_SET_EXPIRY(pTimer);
        gTimerBase.timerTree.insert_unique(*pTimer);
    }
    clOsalMutexUnlock(&gTimerBase.timerListLock);
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
    clOsalMutexLock(&gTimerBase.timerListLock);

    if(callbackTaskIndex >= 0)
        pTimer->timerDelCallbackTask(callbackTaskIndex);

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
        canFree = CL_TRUE;
    }
    --gTimerBase.runningTimers;

    if(pTimer->timerRefCnt <= 0)
        pTimer->timerFlags &= ~CL_TIMER_RUNNING;

    clOsalMutexUnlock(&gTimerBase.timerListLock);
    return CL_OK;
}

void SAFplus::TimerBase::timerSpawn(Timer *pTimer)
{
    /*
     * Invoke the callback in the task pool context
     */
    this->pool.run(pTimer->timerPool);
}


void SAFplus::TimerBase::timeUpdate(void)
{
    this->now = clOsalStopWatchTimeGet();
}


/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *timerTask(void *pArg)
{
    ClTimerTimeOutT timeOut = {.tsSec = 0, .tsMilliSec = gTimerBase.frequency };
    clOsalMutexLock(&gTimerBase.timerListLock);

    gTimerBase.timeUpdate();

    while(gTimerBase.timerRunning == CL_TRUE)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
        clOsalTaskDelay(timeOut);
        clOsalMutexLock(&gTimerBase.timerListLock);
        gTimerBase.timeUpdate();
        gTimerBase.timerRun();
    }

    /*Unreached actually*/
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    return NULL;
}

void SAFplus::Timer::timerDelCallbackTask(ClInt16T freeIndex)
{
    if(freeIndex >= 0)
    {
        this->callbackTaskIds[freeIndex] = 0;
        this->freeCallbackTaskIndexPool[freeIndex] = this->freeCallbackTaskIndex;
        this->freeCallbackTaskIndex = freeIndex;
    }
}

ClInt16T SAFplus::Timer::timerAddCallbackTask()
{
    ClInt16T nextFreeIndex = 0;
    ClInt16T curFreeIndex = 0;
    if( (curFreeIndex = this->freeCallbackTaskIndex) < 0 )
    {
        logInfo("CALLBACK", "ADD", "Unable to store task id for timer [%p] as current [%d] parallel instances "
                     "of the same running timer exceeds [%d] supported. Timer delete on this timer might deadlock "
                     "if issued from the timer callback context", (ClPtrT)this, this->timerRefCnt,
                     CL_TIMER_MAX_PARALLEL_TASKS);
        return -1;
    }
    nextFreeIndex = this->freeCallbackTaskIndexPool[curFreeIndex];
    if(clOsalSelfTaskIdGet(this->callbackTaskIds + curFreeIndex) != CL_OK)
        return -1;
    this->freeCallbackTaskIndex = nextFreeIndex;
    return curFreeIndex;
}
SAFplus::Timer::~Timer()
{
}
void SAFplus::Timer::timerInitCallbackTask()
{
    ClInt32T i;
    for(i = 0; i < CL_TIMER_MAX_PARALLEL_TASKS-1; ++i)
        this->freeCallbackTaskIndexPool[i] = i+1;
    this->freeCallbackTaskIndexPool[i] = -1;
    this->freeCallbackTaskIndex = 0;
}

ClRcT SAFplus::Timer::timerCreate(ClTimerTimeOutT timeOut,
                         ClTimerTypeT timerType,
                         ClTimerContextT timerContext,
                         ClTimerCallBackT timerCallback,
                         void *timerData)
{
    ClTimeT expiry = 0;
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    CL_TIMER_INITIALIZED_CHECK(rc,out);
    if(!timerCallback)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_NULL_TIMER_CALLBACK);
        logError("TIMER7","CREATE", "Timer create failed: Null callback function passed");
        goto out;
    }

    if(timerType >= CL_TIMER_MAX_TYPE)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_TYPE);
        logError("TIMER7","CREATE", "Timer create failed: Bad timer type");

        goto out;
    }
    this->timerPool = new SAFplus::TimerPoolable(timerCallbackTask, (void*)this);
    if(timerContext >= CL_TIMER_MAX_CONTEXT)
    {
        rc = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_CONTEXT_TYPE);
        logError("TIMER7","CREATE", "Timer create failed: Bad timer context");
        goto out;
    }

    rc = CL_TIMER_RC(CL_ERR_NO_MEMORY);

    expiry = (ClTimeT)((ClTimeT)timeOut.tsSec *1000000 +  timeOut.tsMilliSec*1000);
    /*Convert this expiry to jiffies*/
    this->timerTimeOut = expiry;
    this->timerType = timerType;
    this->timerContext = timerContext;
    this->timerCallback = timerCallback;
    this->timerData = timerData;
    this->timerInitCallbackTask();
    this->timerFlags=CL_TIMER_STOPPED;
    rc = CL_OK;
    out:
    return rc;
}

ClRcT SAFplus::Timer::timerStartInternal(ClTimeT expiry,ClBoolT locked)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    CL_TIMER_INITIALIZED_CHECK(rc,out);
    if(!locked)
    {
        clOsalMutexLock(&gTimerBase.timerListLock);
    }

    this->timerFlags &= ~CL_TIMER_STOPPED;

    if(expiry)
    {
        this->timerExpiry = expiry;
    }
    else
    {
        gTimerBase.timeUpdate();
        CL_TIMER_SET_EXPIRY(this);
    }


    /*
     * add to the rb tree.
     */
    //clRbTreeInsert(&gTimerBase.timerTree, &this->timerList);
    logDebug("TIMER", "START", "INSERT TIMER INTO RBTREE with time expire [%ld]",this->timerExpiry);
    gTimerBase.timerTree.insert_unique(*this);
    if(gClTimerDebug)
        this->startTime = clOsalStopWatchTimeGet();


    if(!locked)
    {
        clOsalMutexUnlock(&gTimerBase.timerListLock);
    }

    rc = CL_OK;

    out:
    return rc;
}

ClRcT SAFplus::Timer::timerStop()
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    CL_TIMER_INITIALIZED_CHECK(rc,out);
    rc = CL_OK;
    clOsalMutexLock(&gTimerBase.timerListLock);
    /*Reset timer expiry. and rip this guy off from the list.*/
    this->timerExpiry = 0;
    this->timerFlags |= CL_TIMER_STOPPED;
    gTimerBase.timerTree.erase(boost::intrusive::rbtree<Timer>::s_iterator_to(*this));
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    out:
    return rc;
}

ClRcT SAFplus::Timer::timerStart()
{
    return this->timerStartInternal(0,CL_FALSE);
}
ClRcT SAFplus::Timer::timerUpdate(ClTimerTimeOutT newTimeOut)
{
    ClRcT rc = CL_OK;

    CL_TIMER_INITIALIZED_CHECK(rc, out);

    /*
     * First stop the timer.
     */
    rc = this->timerStop();
    if(rc != CL_OK)
    {
        logError("TIMER7","UPDATE","Timer stop failed");
        goto out;
    }
    clOsalMutexLock(&gTimerBase.timerListLock);

    this->timerFlags &= ~CL_TIMER_STOPPED;
    this->timerTimeOut = (ClTimeT)((ClTimeT)newTimeOut.tsSec * 1000000 + newTimeOut.tsMilliSec * 1000);
    gTimerBase.timeUpdate();
    CL_TIMER_SET_EXPIRY(this);
    gTimerBase.timerTree.insert_unique(*this);



    if(gClTimerDebug)
        this->startTime = clOsalStopWatchTimeGet();

    clOsalMutexUnlock(&gTimerBase.timerListLock);

    out:
    return rc;
}

ClRcT SAFplus::Timer::timerRestart (ClTimerHandleT  timerHandle)
{
    ClRcT rc = 0;

    CL_FUNC_ENTER();

    rc = this->timerStop();

    if (rc != CL_OK)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = this->timerStart();

    if (rc != CL_OK)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT SAFplus::Timer::timerState(ClBoolT flags, ClBoolT *pState)
{
    if(!pState)
        return CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
    *pState = CL_FALSE;
    if((this->timerFlags & flags))
        *pState = CL_TRUE;

    return CL_OK;
}

/*
 * Assumed that the caller has also synchronized his call with his timer start/stop/delete
 */
ClRcT SAFplus::Timer::timerIsStopped(ClBoolT *pState)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gTimerBase.timerListLock);
    rc = this->timerState(CL_TIMER_STOPPED, pState);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    return rc;
}

/*
 * Assumed that the caller has also synchronized his call with his timer start/stop/delete.
 */
ClRcT SAFplus::Timer::timerIsRunning(ClTimerHandleT timerHandle, ClBoolT *pState)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gTimerBase.timerListLock);
    rc = this->timerState(CL_TIMER_RUNNING, pState);
    clOsalMutexUnlock(&gTimerBase.timerListLock);
    return rc;
}

ClRcT SAFplus::Timer::timerCreateAndStart(ClTimerTimeOutT timeOut,
                            ClTimerTypeT timerType,
                            ClTimerContextT timerContext,
                            ClTimerCallBackT timerCallback,
                            void *timerData)
{
    ClRcT rc = CL_OK;

    rc = this->timerCreate(timeOut, timerType, timerContext, timerCallback, timerData);

    if(rc != CL_OK)
        goto out;

    rc = this->timerStart();

    out:
    return rc;
}


/*
 * Dont call it under a callback.
 */

ClRcT SAFplus::timerInitialize(ClPtrT config, ClInt32T maxTimer)
{
    timerMinParallelThread=maxTimer;
    logDebug("TIMER", "START", "Init timer with [%ld] thread pools",maxTimer);
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

    rc = gTimerBase.TimerBaseInitialize();
    if(rc != CL_OK)
    {
        goto out;
    }

    gTimerBase.timerRunning = CL_TRUE;
    gTimerBase.timeUpdate();
    rc = clOsalTaskCreateAttached("TIMER-TASK", CL_OSAL_SCHED_OTHER, 0, 0, timerTask, NULL,
                                  &gTimerBase.timerId);
    if(rc != CL_OK)
    {
        gTimerBase.timerRunning = CL_FALSE;
        logError("TIMER7", "INIT", "Timer task create returned [%#x]", rc);
        goto out_free;
    }

    gTimerBase.initialized = CL_TRUE;

    return rc;

    out_free:

    if(gTimerBase.timerId)
    {
        gTimerBase.timerRunning = CL_FALSE;
        clOsalTaskJoin(gTimerBase.timerId);
    }

    out:
    return rc;
}

ClRcT SAFplus::timerFinalize(void)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);

    if(gTimerBase.initialized == CL_FALSE)
    {
        goto out;
    }

    gTimerBase.initialized = CL_FALSE;

    if(gTimerBase.timerRunning == CL_TRUE)
    {
        clOsalMutexLock(&gTimerBase.timerListLock);
        gTimerBase.timerRunning = CL_FALSE;
        clOsalMutexUnlock(&gTimerBase.timerListLock);

        if(gTimerBase.timerId)
        {
            clOsalTaskJoin(gTimerBase.timerId);
        }
    }
    gTimerBase.pool.stop();
    rc = CL_OK;
    out:
    return rc;
}
