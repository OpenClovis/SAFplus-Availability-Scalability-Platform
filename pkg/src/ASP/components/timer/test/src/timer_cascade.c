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
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include "clTimerTestCommon.h"
#include "clTimerTestCommonErrors.h"
#include "clTimerTestList.h"
#include "clTimerTest.h"

#define CL_TIMER_INITIALIZED_CHECK(rc,label) do {   \
    if(gTimerBase.initialized == CL_FALSE)          \
    {                                               \
        CL_TIMER_PRINT("Timer isnt initialized\n"); \
        rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);   \
        goto label;                                 \
    }                                               \
}while(0)

/*
 * Support 500 active separate context timers.
 * Equivalent to 500 threads running at any point in time.
 */
#define CL_TIMER_MAX_RUNNING (50000)

#define CL_TIMER_LOCK_INIT(mutex) do {          \
        pthread_mutex_init(mutex,NULL);         \
}while(0)

#define CL_TIMER_LOCK_DESTROY(mutex) do {       \
     pthread_mutex_destroy(mutex);              \
}while(0)

#define CL_TIMER_LOCK(mutex) do {               \
    pthread_mutex_lock(mutex);                  \
}while(0)

#define CL_TIMER_UNLOCK(mutex) do {             \
    pthread_mutex_unlock(mutex) ;               \
}while(0)

/*
 * This is the timer granularity expressed in ticks per sec.
*/
#define CL_TIMER_FREQUENCY (100)

#define CL_TIMER_MSEC_JIFFIES  (CL_TIMER_MSEC_PER_SEC/CL_TIMER_FREQUENCY)

#define CL_TIMER_USEC_PER_MSEC_SKEW  (CL_TIMER_MSEC_JIFFIES*CL_TIMER_USEC_PER_MSEC)

#define CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES (CL_TIMER_USEC_PER_MSEC_SKEW - CL_TIMER_USEC_PER_MSEC_SKEW/3)

#define CL_TIMER_MSEC_TO_JIFFIES(msec) ( (msec)/CL_TIMER_MSEC_JIFFIES)

#define CL_TIMER_USEC_TO_JIFFIES(usec) ( (usec)/CL_TIMER_USEC_JIFFIES)

#define CL_TIMER_TIMEOUT CL_TIMER_MSEC_JIFFIES

#define CL_TIMER_NSEC_PER_MSEC_SKEW (CL_TIMER_NSEC_PER_MSEC * CL_TIMER_MSEC_JIFFIES)

/*
 * 200 microsecs per iteration.
 */
#define CL_TIMER_ITERATION_PENALTY (0)

#define CL_TIMER_ITERATION_JIFFY_INCREMENT (CL_TIMER_USEC_PER_MSEC_SKEW/CL_TIMER_ITERATION_PENALTY)

#define CL_TIMER_SET_EXPIRY(timer) do {                                 \
    ClUint64T baseJiffies = gTimerBase.jiffies ;                        \
    (timer)->timerExpiry = baseJiffies + CL_TIMER_MSEC_TO_JIFFIES((timer)->timerTimeOut); \
    if(!(timer)->timerExpiry) (timer)->timerExpiry = 1;                 \
}while(0)


typedef struct ClTimer
{
    /*index into the list head*/
    ClListHeadT timerList;
    /* index into the stop queue*/
    ClListHeadT stopList;
    /* index into the delete queue */
    ClListHeadT deleteList;
    ClCharT timerType;
    ClTimerContextT timerContext;
    /*timeout converted to jiffies*/
    ClUint64T timerTimeOut;
    /*absolute expiry of the timer in jiffies or ticks*/
    ClUint64T timerExpiry;
    /*start jiffy snapshot at which the timer was up*/
    ClUint64T timerStart;
    ClTimerCallBackT timerCallback;
    ClPtrT timerData;
    ClBoolT timerRunning;
    struct timespec startTime;
    struct timespec endTime;
}ClTimerT;

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
    ClBoolT initialized;

    pthread_mutex_t rootVectorLock;
    pthread_cond_t  rootVectorCond;
    ClUint64T jiffies;

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

    pthread_t timerId;
    pthread_t timerJobId;
    ClUint32T runningTimers;
    
    ClBoolT expiryRun;
}ClTimerBaseT;

typedef struct ClTimerJobQueue
{
    ClUint64T startJiffies;
    ClUint64T endJiffies;
    ClListHeadT list;
}ClTimerJobQueueT;

static ClTimerBaseT gTimerBase;
static ClListHeadT gClTimerJobQueue = CL_LIST_HEAD_INITIALIZER(gClTimerJobQueue);
static pthread_cond_t gClTimerJobQueueCond;
static pthread_mutex_t gClTimerJobQueueLock;

static ClListHeadT gClTimerDeleteQueue = CL_LIST_HEAD_INITIALIZER(gClTimerDeleteQueue);
static pthread_mutex_t gClTimerDeleteQueueLock;

static ClListHeadT gClTimerStopQueue = CL_LIST_HEAD_INITIALIZER(gClTimerStopQueue);
static pthread_mutex_t gClTimerStopQueueLock;

/*
 * First step. initialize the base vector
 */

static __inline__ void clTimerIndirectVectorInitialize
(ClTimerIndirectVectorT *pVector)
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

/*
 * Boot strapping ourselves first:-)
*/
ClRcT clTimerBaseInitialize(ClTimerBaseT *pBase)
{
    ClRcT rc = CL_OK;
    pBase->jiffies = 0;
    pBase->expiryRun = CL_FALSE;
    CL_TIMER_LOCK_INIT(&pBase->rootVectorLock);
    pthread_cond_init(&pBase->rootVectorCond, NULL);
    clTimerRootVectorInitialize(&pBase->rootVector1);
    clTimerIndirectVectorInitialize(&pBase->indirectVector2);
    clTimerIndirectVectorInitialize(&pBase->indirectVector3);
    clTimerIndirectVectorInitialize(&pBase->indirectVector4);
    clTimerIndirectVectorInitialize(&pBase->indirectVector5);
    return rc;
}

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
    clListMoveInit(pVector->list + index,&tempList);

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
                    ClPtrT timerData,
                    ClTimerHandleT *pTimerHandle)
{
    ClTimerT *pTimer = NULL;
    ClUint64T expiry = 0;
    ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

    CL_TIMER_INITIALIZED_CHECK(rc,out);

    if((timerCallback == NULL)
       ||
       (pTimerHandle == NULL)
       )
    {
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }

    if(timerType >= CL_TIMER_MAX_TYPE)
    {
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }

    if(timerContext >= CL_TIMER_MAX_CONTEXT)
    {
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }

    rc = CL_TIMER_RC(CL_ERR_NO_MEMORY);
    pTimer = calloc(1,sizeof(*pTimer));
    if(pTimer == NULL)
    {
        CL_TIMER_PRINT("Error allocating memory\n");
        goto out;
    }
    expiry = (ClUint64T)((ClUint64T)timeOut.tsSec * CL_TIMER_MSEC_PER_SEC+
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
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }
    pTimer = (ClTimerT*)timerHandle;

    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    rc = CL_TIMER_RC(CL_ERR_INUSE);

    /*
     * Already started.Has to be stopped.
     */
    if(pTimer->timerList.pNext != NULL)
    {
        goto out_unlock;
    }

    pTimer->timerStart  = gTimerBase.jiffies;

    CL_TIMER_SET_EXPIRY(pTimer);
    
    clock_gettime(CLOCK_MONOTONIC, &pTimer->startTime);

    clTimerVectorAdd(pTimer);

    rc = CL_OK;

    out_unlock:

    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

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
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }
    
    pTimer = (ClTimerT*)timerHandle;

    rc = CL_OK;

    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    /*
     * Already there in stop list.
     */
    if(pTimer->stopList.pNext)
    {
        rc = CL_TIMER_RC(CL_ERR_ALREADY_EXIST);
        goto out_unlock;
    }

    if(gTimerBase.expiryRun == CL_TRUE)
    {
        pthread_mutex_lock(&gClTimerStopQueueLock);
        clListAddTail(&pTimer->stopList, &gClTimerStopQueue);
        pthread_mutex_unlock(&gClTimerStopQueueLock);
        goto out_unlock;
    }

    /*Reset timer expiry. and rip this guy off from the list.*/
    pTimer->timerExpiry = 0;

    if(pTimer->timerList.pNext)
        clListDel(&pTimer->timerList);

    out_unlock:
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

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
        CL_TIMER_PRINT("Invalid param\n");
        goto out;
    }
    pTimer = (ClTimerT*)*pTimerHandle;

    rc = CL_TIMER_RC(CL_ERR_OP_NOT_PERMITTED);

    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        goto out;
    }
    
    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    if(pTimer->deleteList.pNext)
    {
        rc = CL_TIMER_RC(CL_ERR_ALREADY_EXIST);
        goto out_unlock;
    }

    pthread_mutex_lock(&gClTimerDeleteQueueLock);
    clListAddTail(&pTimer->deleteList, &gClTimerDeleteQueue);
    pthread_mutex_unlock(&gClTimerDeleteQueueLock);

    rc = CL_OK;
    *pTimerHandle = 0;

    out_unlock:
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

    out:
    return rc;
}

static void *clTimerCallbackThread(void *pArg)
{
    ClTimerT *pTimer = pArg;
#if 0
    if(gTimerBase.runningTimers >= CL_TIMER_MAX_RUNNING)
    {
        fprintf(stderr,"Total running timers in the system exceeds max threshold of %d.Skipping timer start\n",CL_TIMER_MAX_RUNNING);
        return NULL;
    }
#endif
    ++gTimerBase.runningTimers;
    pTimer->timerRunning = CL_TRUE;
    pTimer->timerCallback(pTimer->timerData);
    --gTimerBase.runningTimers;
    pTimer->timerRunning = CL_FALSE;

    /*
     * Rip this guy if volatile.
     */
    if(pTimer->timerType == CL_TIMER_VOLATILE)
    {
        free(pTimer);
    }
    return NULL;
}

static __inline__ void clTimerSpawn(ClTimerT *pTimer)
{
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    CL_ASSERT(pthread_create(&id,&attr,clTimerCallbackThread,(void*)pTimer)==0);
}

/*
* Okay reque repetitive timers.
*/
static void clTimerStartRepetitive(ClListHeadT *pHead)
{
    ClTimerBaseT *pBase = &gTimerBase;

    if(CL_LIST_HEAD_EMPTY(pHead))
        return;

    CL_TIMER_LOCK(&pBase->rootVectorLock);

    while(pBase->expiryRun == CL_TRUE)
    {
        pthread_cond_wait(&pBase->rootVectorCond, &pBase->rootVectorLock);
    }

    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pTimerListEntry = pHead->pNext;

        ClTimerT *pTimer = CL_LIST_ENTRY(pTimerListEntry,ClTimerT,timerList);
        
        clListDel(pTimerListEntry);

        CL_TIMER_SET_EXPIRY(pTimer);
        
        clock_gettime(CLOCK_MONOTONIC, &pTimer->startTime);

        pTimer->timerStart = pBase->jiffies ? pBase->jiffies : 1;

        clTimerVectorAdd(pTimer);
    }

    CL_TIMER_UNLOCK(&pBase->rootVectorLock);
}

/*
 * Stop the timers from the async stop queue.
 */
static void clTimerRunStopQueue(void)
{
    ClListHeadT stopBatchQueue = CL_LIST_HEAD_INITIALIZER(stopBatchQueue);

    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    pthread_mutex_lock(&gClTimerStopQueueLock);
    clListMoveInit(&gClTimerStopQueue, &stopBatchQueue);
    pthread_mutex_unlock(&gClTimerStopQueueLock);

    if(!CL_LIST_HEAD_EMPTY(&stopBatchQueue))
    {
        while(gTimerBase.expiryRun == CL_TRUE)
        {
            pthread_cond_wait(&gTimerBase.rootVectorCond, &gTimerBase.rootVectorLock);
        }
        while(!CL_LIST_HEAD_EMPTY(&stopBatchQueue))
        {
            ClTimerT *pTimer = CL_LIST_ENTRY(stopBatchQueue.pNext, ClTimerT, stopList);
            pTimer->timerExpiry = 0;
            clListDel(&pTimer->stopList);
            /*
             * Delete from the timer list.
             */
            if(pTimer->timerList.pNext)
                clListDel(&pTimer->timerList);
        }
    }
    
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);
}

static void clTimerRunDeleteQueue(void)
{
    ClListHeadT deleteBatchQueue = CL_LIST_HEAD_INITIALIZER(deleteBatchQueue);

    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    pthread_mutex_lock(&gClTimerDeleteQueueLock);
    clListMoveInit(&gClTimerDeleteQueue, &deleteBatchQueue);
    pthread_mutex_unlock(&gClTimerDeleteQueueLock);

    if(!CL_LIST_HEAD_EMPTY(&deleteBatchQueue))
    {
        while(gTimerBase.expiryRun == CL_TRUE)
        {
            pthread_cond_wait(&gTimerBase.rootVectorCond, &gTimerBase.rootVectorLock);
        }
        while(!CL_LIST_HEAD_EMPTY(&deleteBatchQueue))
        {
            ClTimerT *pTimer = CL_LIST_ENTRY(deleteBatchQueue.pNext, ClTimerT, deleteList);
            clListDel(&pTimer->deleteList);
            if(pTimer->stopList.pNext)
                clListDel(&pTimer->stopList);
            if(pTimer->timerList.pNext)
                clListDel(&pTimer->timerList);
            pTimer->timerExpiry = 0;
            free(pTimer);
        }
    }
    
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);
}

/*
 * Run the timers through cascading.
 */
static void clTimerRun(ClUint64T startJiffies, ClUint64T endJiffies)
{
    ClUint32T index;
    ClTimerBaseT *pBase = &gTimerBase;
    ClListHeadT repetitiveList = CL_LIST_HEAD_INITIALIZER(repetitiveList);
    
    if(startJiffies) ++startJiffies;
    /*
     * Process pending stops. first
     */
    clTimerRunStopQueue();

    CL_TIMER_LOCK(&pBase->rootVectorLock);

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
        CL_TIMER_UNLOCK(&pBase->rootVectorLock);

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
            ClUint64T timeDiff;

            /*Knock off from this list*/
            clListDel(pTimerListEntry);

            clock_gettime(CLOCK_MONOTONIC, &pTimer->endTime);
            
            timeDiff = (pTimer->endTime.tv_sec*CL_TIMER_USEC_PER_SEC + pTimer->endTime.tv_nsec/CL_TIMER_NSEC_PER_USEC);
            timeDiff -= (pTimer->startTime.tv_sec*CL_TIMER_USEC_PER_SEC + pTimer->startTime.tv_nsec/CL_TIMER_NSEC_PER_USEC);
            /*CL_TIMER_PRINT("Timer invoked at [%lld] - [%lld.%lld] usecs." \
                           "Expiry [%lld] jiffies\n", timeDiff,
                           timeDiff/CL_TIMER_USEC_PER_MSEC,
                           timeDiff % CL_TIMER_USEC_PER_MSEC,
                           pTimer->timerTimeOut);*/

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
                    free(pTimer);
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
        CL_TIMER_LOCK(&pBase->rootVectorLock);
    }
    /*
     * Now run the delete timer list.
     */
    gTimerBase.expiryRun = CL_FALSE;
    pthread_cond_broadcast(&gTimerBase.rootVectorCond);
    CL_TIMER_UNLOCK(&pBase->rootVectorLock);

    clTimerRunDeleteQueue();
    clTimerStartRepetitive(&repetitiveList);
}

static void *clTimerJobThread(void *arg)
{
    ClListHeadT batchQueue = CL_LIST_HEAD_INITIALIZER(batchQueue);
    pthread_mutex_lock(&gClTimerJobQueueLock);
    for(;;)
    {
        while(CL_LIST_HEAD_EMPTY(&gClTimerJobQueue))
        {
            pthread_cond_wait(&gClTimerJobQueueCond, &gClTimerJobQueueLock);
        }
        clListMoveInit(&gClTimerJobQueue, &batchQueue);
        pthread_mutex_unlock(&gClTimerJobQueueLock);
        while(!CL_LIST_HEAD_EMPTY(&batchQueue))
        {
            ClTimerJobQueueT *pJob = CL_LIST_ENTRY(batchQueue.pNext, ClTimerJobQueueT, list);
            ClUint64T startJiffies, endJiffies;
            clListDel(&pJob->list);
            startJiffies = pJob->startJiffies;
            endJiffies = pJob->endJiffies;
            free(pJob);
            clTimerRun(startJiffies, endJiffies);
        }
        pthread_mutex_lock(&gClTimerJobQueueLock);
    }
    pthread_mutex_unlock(&gClTimerJobQueueLock);
    return NULL;
}

static void clTimerJobQueue(ClUint64T startJiffies, ClUint64T endJiffies)
{
    ClTimerJobQueueT *pJob = NULL;
    pJob = calloc(1, sizeof(*pJob));
    CL_ASSERT(pJob != NULL);
    pJob->startJiffies = startJiffies;
    pJob->endJiffies = endJiffies;
    pthread_mutex_lock(&gClTimerJobQueueLock);
    clListAddTail(&pJob->list, &gClTimerJobQueue);
    pthread_cond_signal(&gClTimerJobQueueCond);
    pthread_mutex_unlock(&gClTimerJobQueueLock);
}


/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *clTimerTask(void *pArg)
{
    ClInt64T diff = 0;
    /*ClInt64T iterations = 0;*/
    ClUint64T timerTimeOut = CL_TIMER_TIMEOUT * CL_TIMER_USEC_PER_MSEC;
    static ClUint64T skew = 0;
    static struct timespec start = {0};
    
    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);

    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_testcancel();

    for(;;)
    {
        struct timespec  end = {0};
        ClUint64T startJiffies = 0, endJiffies = 0;
        CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

        CL_TIMER_DELAY(timerTimeOut);
        pthread_testcancel();

        clock_gettime(CLOCK_MONOTONIC, &end);

        diff = (end.tv_sec * CL_TIMER_NSEC_PER_SEC + end.tv_nsec) - (start.tv_sec * CL_TIMER_NSEC_PER_SEC + start.tv_nsec);
        diff /= CL_TIMER_NSEC_PER_USEC;

        clock_gettime(CLOCK_MONOTONIC, &start);

        CL_TIMER_LOCK(&gTimerBase.rootVectorLock);
        startJiffies = gTimerBase.jiffies;
        gTimerBase.jiffies += CL_TIMER_MSEC_TO_JIFFIES(diff/CL_TIMER_USEC_PER_MSEC);
        skew += (diff % CL_TIMER_USEC_PER_MSEC);
        /*
         * Account for a tick for the nearest millisec
         */
        /*CL_TIMER_PRINT("Skew set at [%lld], iterations [%lld]\n", skew, ++iterations);*/
        skew += CL_TIMER_ITERATION_PENALTY;
        if(skew >= CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES)
        {
            gTimerBase.jiffies += skew/CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES;
            skew -= (skew/CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES * CL_TIMER_USEC_PER_MSEC_SKEW_JIFFIES);
        }

        endJiffies = gTimerBase.jiffies;

        CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

        clTimerJobQueue(startJiffies, endJiffies);

    }

    /*Unreached actually*/
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

    return NULL;
}

ClRcT clTimerInitialize(void)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_INITIALIZED);
    int ret;
    pthread_attr_t attr;

    if(gTimerBase.initialized == CL_TRUE)
    {
        CL_TIMER_PRINT("Timer already initialized\n");
        goto out;
    }
    rc = clTimerBaseInitialize(&gTimerBase);
    if(rc != CL_OK)
    {
        CL_TIMER_PRINT("Error initializing the timer base\n");
        goto out;
    }
    pthread_mutex_init(&gClTimerJobQueueLock, NULL);
    pthread_mutex_init(&gClTimerStopQueueLock, NULL);
    pthread_mutex_init(&gClTimerDeleteQueueLock, NULL);
    
    pthread_cond_init(&gClTimerJobQueueCond, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&gTimerBase.timerId,&attr,clTimerTask,NULL);
    CL_ASSERT(ret==0);
    ret = pthread_create(&gTimerBase.timerJobId, &attr, clTimerJobThread, NULL);
    CL_ASSERT(ret == 0);
    gTimerBase.initialized = CL_TRUE;

    out:
    return rc;
}

ClRcT clTimerFinalize(void)
{
    ClRcT rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);

    if(gTimerBase.initialized == CL_FALSE)
    {
        CL_TIMER_PRINT("Timer isnt initialized\n");
        goto out;
    }

    CL_TIMER_LOCK(&gTimerBase.rootVectorLock);
    gTimerBase.initialized = CL_FALSE;
    gTimerBase.expiryRun = CL_FALSE;
    CL_TIMER_UNLOCK(&gTimerBase.rootVectorLock);

    /*
     * We shouldnt care about deleting pending timers
     * as finalize of individual guys should take care.
     * Can be done, but with another list head which I dont think its required.
     */
    pthread_cancel(gTimerBase.timerId);

    clTimerRunDeleteQueue();

    rc = CL_OK;

    out:
    return rc;
}

