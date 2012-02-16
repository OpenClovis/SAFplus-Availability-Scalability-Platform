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
/*******************************************************************************
 * ModuleName  : timer
 * File        : timer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * The Timer Service provides timers that allows a user call back        
 * function to be invoked when a time out happens. The motivation of the 
 * implementation comes from the need to have large number of outstanding
 * timer at any point in time. The timer service library provides a      
 * set of API's which are discussed in the following sections.           
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clQueueApi.h>

#include <clTimerApi.h>
#include <clTimerErrors.h>

#include <clDebugApi.h>
#include <clDbg.h>
/*FIXME: #include <timerLibCompId.h>*/
/*************************************************************************/
#define TIMER_TASK_STACK_SIZE         65535 /* bytes */
/*************************************************************************/

#define CL_TIMER_TICK_USECS (gActiveTimerQueue.tickInMilliSec*1000LL)

#define CL_TIMER_INTERVAL(timeOut) (((timeOut).tsSec*1000000ULL+(timeOut).tsMilliSec*1000ULL)/CL_TIMER_TICK_USECS)

/*
 * time of day drift thresholds.
 */
#define CL_TIMER_DRIFT_THRESHOLD (5U)

typedef enum TsTimerState_e {
    TIMER_ACTIVE=0,
    TIMER_INACTIVE,
    TIMER_FREE
} TsTimerState_e;
/*************************************************************************/
typedef struct TsTimer_t {
    ClTimerTimeOutT          timeOut;
    ClTimerTypeT        type; /* one-shot/repetitive */
    ClUint32T           spawnTask;
    ClTimerCallBackT         fpTimeOutAction;
    void*                pActionArgument;

    TsTimerState_e       state; /* active/inactive/free */
    ClUint64T  timestamp;
    ClUint64T  timeInterval; /*timeinterval in usecs of timer frequency*/
    struct TsTimer_t*    pNextActiveTimer; /* valid only for active timers */
    struct TsTimer_t*    pPreviousActiveTimer; /* valid only for active timers */
} TsTimer_t;
/*************************************************************************/
typedef struct TsActiveTimerQueueHead_t {
    TsTimer_t*   pFirstTimer;
    TsTimer_t*   pLastTimer;

    ClOsalMutexIdT   timerMutex;
    ClOsalTaskIdT    timerTaskId;
    ClOsalTaskIdT    reEnqueueTaskId;
    ClQueueT      reEnqueueQueue;
    ClUint32T   tickInMilliSec;
    ClUint32T   timerServiceInitialized;
} TsActiveTimerQueueHead_t;
/*************************************************************************/
static ClRcT tsFreeTimersPoolCreate (void);
static ClRcT tsFreeTimersPoolDestroy (void);
static ClRcT tsActiveTimersQueueCreate (void);
static ClRcT tsActiveTimersQueueDestroy (void);
static ClRcT tsActiveTimerEnqueue (TsTimer_t* pUserTimer);
static ClRcT tsActiveTimerDequeue (TsTimer_t* pUserTimer);
static void* tsTimerTask (void* pArgument);
static TsTimer_t* tsFreeTimerGet (void);
static void tsFreeTimerReturn (TsTimer_t* pUserTimer);
static void* tsTimerReEnqueue();
void deQueueCallBack(ClQueueDataT userData);
/*************************************************************************/
TsActiveTimerQueueHead_t gActiveTimerQueue;
ClTimerConfigT gTimerConfig = {10,     /* timer resolution */
                                150};   /* timer task priority */
static ClUint64T currentTime;
static ClUint64T skew = 0;
static ClUint64T iteration = 0;

/*************************************************************************/
/*#define	CL_TIMER_RC(ERR_ID)	(CL_RC(CL_COMP_TL, ERR_ID))*/
/*************************************************************************/

static ClInt64T clTimerTimeDiff(struct timeval *pStart,
                                struct timeval *pEnd,
                                ClUint32T thresholdSecs)
{
    ClInt64T diff = 0;
    pEnd->tv_sec -= pStart->tv_sec;
    pEnd->tv_usec -= pStart->tv_usec;

    /*Possible Time shift*/
    if(pEnd->tv_sec <= 0 )
    {
        if(pEnd->tv_sec < 0 || pEnd->tv_usec <= 0)
        {
            return 0ULL;
        }
    }
    if(pEnd->tv_usec < 0 )
    {
        pEnd->tv_usec += 1000000L;
        --pEnd->tv_sec;
    }
    diff = pEnd->tv_sec*1000000LL + pEnd->tv_usec;
    /*
     * Assume forward time shift
     */
    if(thresholdSecs && (pEnd->tv_sec > (int) thresholdSecs))
    {
        diff = 0;
    }
    return diff;
}

ClRcT clTimerConfigInitialize(void* pConfigData)
{
    ClTimerConfigT* pTimerConfig = NULL;
    ClRcT errorCode = 0;

    pTimerConfig = (ClTimerConfigT *)pConfigData;
  
    if(pConfigData == NULL) {
        errorCode = CL_TIMER_RC(CL_ERR_NULL_POINTER);
        return(errorCode);
    }

    if( (pTimerConfig->timerResolution < 10) || (pTimerConfig->timerTaskPriority <1) || (pTimerConfig->timerTaskPriority > 160) ) {
        errorCode = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
        return(errorCode);
    }

    gTimerConfig.timerResolution = pTimerConfig->timerResolution;
    gTimerConfig.timerTaskPriority = pTimerConfig->timerTaskPriority;

    return(CL_OK);

}
/*************************************************************************/
ClRcT 
clTimerInitialize (ClPtrT pConfig)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;

    CL_FUNC_ENTER();
    /* TBD: check whether the OSAL has been initialized before going ahead with Timer init */
    returnCode = clOsalInitialize(NULL);
    if(CL_OK != returnCode) {
        CL_FUNC_EXIT();
        return(returnCode);
    }

    if (gActiveTimerQueue.timerServiceInitialized == 1) {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT (CL_DEBUG_INFO,("\nTimer already initialized"));
        return (CL_OK);
    }

#ifdef DEBUG
    returnCode= dbgAddComponent (COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != returnCode)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
        return (returnCode);
    }
#endif
    
    /* create a pool of free timers */
    returnCode = tsFreeTimersPoolCreate ();

    if (returnCode != CL_OK) {
        
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,("\nTimer Init : NOT DONE"));
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    /* create the active timers queue */
    returnCode = tsActiveTimersQueueCreate ();

    if (returnCode != CL_OK) {
        /* debug message */
        returnCode = tsFreeTimersPoolDestroy ();
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,("\nTimer Init : NOT DONE"));
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    /* create re-enqueue Queue */
    returnCode = clQueueCreate(0,
              deQueueCallBack,
              deQueueCallBack,
              &(gActiveTimerQueue.reEnqueueQueue));

    if (returnCode != CL_OK) {
        /* debug message */
        returnCode = tsFreeTimersPoolDestroy ();
        returnCode = tsActiveTimersQueueDestroy ();
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,("\nTimer Init : NOT DONE"));
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    CL_DEBUG_PRINT (CL_DEBUG_INFO,("\nTimer Init : DONE"));
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerFinalize (void)
{
    ClRcT retCode = 0;
    CL_FUNC_ENTER();
    /* kill the timer-task */
    retCode = clOsalTaskDelete (gActiveTimerQueue.timerTaskId);

    retCode = tsActiveTimersQueueDestroy ();

    retCode = tsFreeTimersPoolDestroy ();
    
    /* create re-enqueue Queue */
    retCode = clQueueDelete(&(gActiveTimerQueue.reEnqueueQueue));

    CL_DEBUG_PRINT (CL_DEBUG_INFO,("\nTimer Cleanup : DONE"));
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerCreate (ClTimerTimeOutT      timeOut,        /* the timeout, in clockticks */
               ClTimerTypeT    type,           /* one shot or repetitive */
               ClTimerContextT timerTaskSpawn, /* whether to spawn off the timer function
                                                 * as a separate task or invoke it in the
                                                 * same context as the timer-task */
               ClTimerCallBackT     fpAction,       /* the function to be called on timeout */
               void*            pActionArgument,/* the argument to the function called on timeout */
               ClTimerHandleT* pTimerHandle)   /* The pointer to the timer handle */		  
{
    TsTimer_t* pUserTimer = NULL;
    ClRcT errorCode;
    
    CL_FUNC_ENTER();
    if (fpAction == NULL) {
        errorCode = CL_TIMER_RC(CL_TIMER_ERR_NULL_TIMER_CALLBACK);
        clDbgCodeError(errorCode, ("Timer create failed: Null callback function passed"));
        CL_FUNC_EXIT();
        return (errorCode);
    }

    if (pTimerHandle == NULL) {
        errorCode = CL_TIMER_RC(CL_ERR_NULL_POINTER);
        clDbgCodeError(returnCode, ("Bad timer handle"));
        CL_FUNC_EXIT();
        return (errorCode);
    }

    switch (type) {
        case CL_TIMER_ONE_SHOT:
        case CL_TIMER_REPETITIVE:
        break;
        default:
            errorCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_TYPE);
            clDbgCodeError(errorCode,("Timer create failed: Invalid timer type"));
            CL_FUNC_EXIT();
            return (errorCode);
    }

    switch (timerTaskSpawn) {
        case CL_TIMER_TASK_CONTEXT:
        case CL_TIMER_SEPARATE_CONTEXT:
        break;
        default:
            errorCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_CONTEXT_TYPE);
            clDbgCodeError(errorCode,("Timer create failed: Invalid context type"));
            CL_FUNC_EXIT();
            return (errorCode);
    }

    /* TBD: ensure that user-timeout is more than timer resolution */

    /* allocate a timer from the free pool */
    pUserTimer = tsFreeTimerGet ();

    if (pUserTimer == NULL) {
        /* debug message */
        errorCode = CL_TIMER_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT (CL_DEBUG_WARN,
                  ("\nTimer create failed"));
        CL_FUNC_EXIT();
        return (errorCode);
    }

    timeOut.tsSec = timeOut.tsSec + (timeOut.tsMilliSec/1000);
    timeOut.tsMilliSec = timeOut.tsMilliSec % 1000;
    
    /* fill in the appropriate values */
    pUserTimer->timeOut.tsSec = timeOut.tsSec;
    pUserTimer->timeOut.tsMilliSec = timeOut.tsMilliSec;
    pUserTimer->timeInterval = CL_TIMER_INTERVAL(timeOut);

    pUserTimer->type = type; /* one-shot/repetitive */
    pUserTimer->spawnTask = timerTaskSpawn;
    pUserTimer->fpTimeOutAction = fpAction;
    pUserTimer->pActionArgument = pActionArgument;
    pUserTimer->state = TIMER_INACTIVE; /* active/inactive/free */
    pUserTimer->pNextActiveTimer = NULL; /* valid only for active timers */
    pUserTimer->pPreviousActiveTimer = NULL; /* valid only for active timers */
    
    /* return handle to the user */
    *pTimerHandle = pUserTimer;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerDelete (ClTimerHandleT*  pTimerHandle)
{
    TsTimer_t* pUserTimer = NULL;
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;

    CL_FUNC_ENTER();
    if (NULL == pTimerHandle) {
        returnCode = CL_TIMER_RC(CL_ERR_NULL_POINTER);
        clDbgCodeError(returnCode, ("Bad timer handle storage"));
        CL_FUNC_EXIT();
        return (returnCode);
    }

    pUserTimer = (TsTimer_t*) *pTimerHandle;
    if (pUserTimer == NULL) {
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(returnCode, ("Bad timer handle"));
        CL_FUNC_EXIT();
        return (returnCode);
    }
    if (pUserTimer->state == TIMER_FREE) {
        returnCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER);
        clDbgCodeError(returnCode, ("Double delete of a timer")); 
        CL_FUNC_EXIT();
        return (returnCode);
    }

    /* if timer is active, remove it from the active-timers queue */
    returnCode = clTimerStop (*pTimerHandle);

    if (returnCode != CL_OK) {
        CL_DEBUG_PRINT (CL_DEBUG_WARN, ("\nTimer delete failed"));
        CL_FUNC_EXIT();
        return (returnCode);
    }

    /* null out all the values */
    pUserTimer->fpTimeOutAction = NULL;
    pUserTimer->pActionArgument = NULL;
    pUserTimer->state = TIMER_FREE;
    pUserTimer->pNextActiveTimer = NULL; /* valid only for active timers */
    pUserTimer->pPreviousActiveTimer = NULL; /* valid only for active timers */
    
    /* return timer to the free pool */
    tsFreeTimerReturn (pUserTimer);

    *pTimerHandle = NULL;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerStart (ClTimerHandleT  timerHandle)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    TsTimer_t* pUserTimer = NULL;
    CL_FUNC_ENTER();
    /* make sure the timer actually exists */
    pUserTimer = (TsTimer_t*) timerHandle;
    if (pUserTimer == NULL) {
        /* debug message */
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(returnCode, ("Bad timer handle"));
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pUserTimer->state == TIMER_FREE) {
        returnCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER);
        clDbgCodeError(returnCode, ("Attempt to start deleted timer.")); 
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pUserTimer->fpTimeOutAction == NULL) {
        /* user has probably deleted the timer! otherwise there's some other corruption! */
        returnCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER);
        clDbgCodeError(returnCode, ("Attempt to start deleted or corrupt timer.")); 
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pUserTimer->state == TIMER_ACTIVE) {
        returnCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER);
        clDbgCodeError(returnCode, ("Attempt to start an already started timer.")); 
        CL_FUNC_EXIT();
        return (returnCode);
    }

    /* mark timer as active */
    pUserTimer->state = TIMER_ACTIVE;

    returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);
    if (returnCode != CL_OK) {
        /* debug message */
        CL_FUNC_EXIT();
        return (returnCode);
    }
    /* put it on the active timer's queue */
    pUserTimer->timestamp = currentTime+pUserTimer->timeInterval;
    pUserTimer->timestamp += skew/(iteration?iteration:1)/CL_TIMER_TICK_USECS;
    returnCode = tsActiveTimerEnqueue (pUserTimer);
    if (returnCode != CL_OK) {
        /* debug message */
        if (CL_OK != clOsalMutexUnlock (gActiveTimerQueue.timerMutex)) {
            CL_FUNC_EXIT();
            return(returnCode);
        }
        CL_FUNC_EXIT();
        return (returnCode);
    }
    returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
    if (returnCode != CL_OK) {
        /* debug message */
        CL_FUNC_EXIT();
        return (returnCode);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerStop (ClTimerHandleT  timerHandle)
{
    /* make sure the timer actually exists */
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    TsTimer_t* pUserTimer = NULL;

    CL_FUNC_ENTER();
    pUserTimer = (TsTimer_t*) timerHandle;

    if (pUserTimer == NULL) {
        /* debug message */
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(returnCode, ("Bad timer handle"));
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pUserTimer->state == TIMER_FREE) {
        returnCode = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER);
        clDbgCodeError(returnCode, ("Attempt to stop a deleted timer.")); 
        CL_FUNC_EXIT();
        return (returnCode);
    }

    /* if the timer is active, remove it from the active-timers queue */
    if (pUserTimer->state == TIMER_ACTIVE) {
        returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);
        if (returnCode != CL_OK) {
            CL_FUNC_EXIT();
            return (returnCode);
        }

        returnCode = tsActiveTimerDequeue (pUserTimer);
        if (returnCode != CL_OK) {
            if(CL_OK != clOsalMutexUnlock (gActiveTimerQueue.timerMutex)) {
                CL_FUNC_EXIT();
                return(returnCode);
            }
            CL_FUNC_EXIT();
            return (returnCode);
        }

        returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
        if (returnCode != CL_OK) {
            CL_FUNC_EXIT();
            return (returnCode);
        }
    }

    /* mark the timer as inactive */
    pUserTimer->state = TIMER_INACTIVE;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerRestart (ClTimerHandleT  timerHandle)
{
    ClRcT returnCode = 0;

    CL_FUNC_ENTER();
    returnCode = clTimerStop (timerHandle);

    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }

    returnCode = clTimerStart (timerHandle);

    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerCreateAndStart (ClTimerTimeOutT      timeOut,        /* the timeout, in clockticks */
                    ClTimerTypeT    type,           /* one shot or repetitive */
                    ClTimerContextT timerTaskSpawn, /* whether to spawn off the timer function
                                                 * as a separate task or invoke it in the
                                                 * same context as the timer-task
                                                 */
                    ClTimerCallBackT     fpAction,       /* the function to be called on timeout */
                    void*            actionArgument, /* the argument to the function called on timeout */
                    ClTimerHandleT* pTimerHandle)
{
    ClRcT returnCode = 0;

    CL_FUNC_ENTER();
    returnCode = clTimerCreate (timeOut,
                                type,
                                timerTaskSpawn,
                                fpAction,
                                actionArgument,
                                pTimerHandle);

    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }

    returnCode = clTimerStart (*pTimerHandle); 

    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
ClRcT
clTimerTypeGet (ClTimerHandleT timerHandle,
                ClUint32T* pTimerType)
{
    /* make sure the timer actually exists */
    TsTimer_t* pUserTimer = NULL;
    ClRcT returnCode;

    CL_FUNC_ENTER();
    pUserTimer = (TsTimer_t*) timerHandle;
    if (pUserTimer == NULL) {
        /* debug message */
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(returnCode, ("Bad timer handle"));
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pTimerType == NULL) {
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    /* extract the timer-type from the timer */
    *pTimerType = pUserTimer->type;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
static ClRcT
tsFreeTimersPoolCreate (void)
{
    /*
     * this function will eventually be implemented to create a pool of free timers.
     * thenceforth, when-ever the user requests for a new timer, the next-free-timer
     * from this pool would be returned. If this pool runs out of free-timers, new
     * free-timers would be created in the pool.
     */
    return (CL_OK);
}
/*************************************************************************/
static ClRcT
tsFreeTimersPoolDestroy (void)
{
    /*
     * this function will eventually be implemented to destroy a pool of free timers.
     */
    return (CL_OK);
}
/*************************************************************************/
static ClRcT
tsActiveTimersQueueCreate (void)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    CL_FUNC_ENTER();

    gActiveTimerQueue.pFirstTimer = NULL;
    gActiveTimerQueue.pLastTimer = NULL;

    
    returnCode = clOsalMutexCreate (&gActiveTimerQueue.timerMutex);
    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }    

    gActiveTimerQueue.tickInMilliSec = gTimerConfig.timerResolution;

    /* spawn off the timer-task */
    returnCode = clOsalTaskCreateAttached("TIMER TASK",
                                CL_OSAL_SCHED_OTHER,
                                CL_OSAL_THREAD_PRI_NOT_APPLICABLE,  /* gTimerConfig.timerTaskPriority: Normally it would be this, but OTHER scheduling has no priority */
                                TIMER_TASK_STACK_SIZE,
                                tsTimerTask,
                                NULL,
                                &gActiveTimerQueue.timerTaskId);

    if (returnCode != CL_OK) {
        if(CL_OK != clOsalMutexDelete (gActiveTimerQueue.timerMutex)) {
            CL_FUNC_EXIT();
            return(returnCode);
        }
        CL_FUNC_EXIT();
        return (returnCode);
    }

    gActiveTimerQueue.timerServiceInitialized = 1;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
static ClRcT
tsActiveTimersQueueDestroy (void)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    TsTimer_t* pUserTimer = NULL;
    TsTimer_t* pNextUserTimer = NULL;

    CL_FUNC_ENTER();
    returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);

    if (returnCode != CL_OK) {
        CL_FUNC_EXIT();
        return (returnCode);
    }

    pNextUserTimer = NULL;

    for (pUserTimer = gActiveTimerQueue.pFirstTimer;
	 pUserTimer != NULL;
	 pUserTimer = pNextUserTimer) {

        pNextUserTimer = pUserTimer->pNextActiveTimer;

        returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
	
        returnCode = clTimerDelete ((ClTimerHandleT*)&pUserTimer);
	
        returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);
    }

    gActiveTimerQueue.timerServiceInitialized = 0;

    returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);

    returnCode = clOsalMutexDelete (gActiveTimerQueue.timerMutex);

    CL_FUNC_EXIT();
    return (returnCode);
}
/*************************************************************************/
static ClRcT
tsActiveTimerEnqueue (TsTimer_t* pUserTimer)
{
    TsTimer_t* pTimerOnQueue = NULL;

    CL_FUNC_ENTER();
    if (pUserTimer == NULL) {
        /* invalid timer */
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    if (pUserTimer->state != TIMER_ACTIVE) {
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    if (pUserTimer->fpTimeOutAction == NULL) {
        CL_FUNC_EXIT();
        return (CL_ERR_UNSPECIFIED);
    }

    if (gActiveTimerQueue.pFirstTimer == NULL) {
        /* this is the first timer in the list */
        pUserTimer->pNextActiveTimer = NULL;
        pUserTimer->pPreviousActiveTimer = NULL;
        gActiveTimerQueue.pFirstTimer = pUserTimer;
        gActiveTimerQueue.pLastTimer = pUserTimer;

        CL_FUNC_EXIT();
        return (CL_OK);
    }

    for (pTimerOnQueue = gActiveTimerQueue.pFirstTimer;
         pTimerOnQueue != NULL;
         pTimerOnQueue = pTimerOnQueue->pNextActiveTimer) {
     
        if( (pTimerOnQueue->timestamp <= pUserTimer->timestamp))
	    {
            continue;
	    }
        else
	    {
            TsTimer_t* pPreviousActiveTimer;
		
            pPreviousActiveTimer = pTimerOnQueue->pPreviousActiveTimer;
		
            pUserTimer->pNextActiveTimer = pTimerOnQueue;
            pUserTimer->pPreviousActiveTimer = pPreviousActiveTimer;
            pTimerOnQueue->pPreviousActiveTimer = pUserTimer;
		
            if (pPreviousActiveTimer) {
                pPreviousActiveTimer->pNextActiveTimer = pUserTimer;
            }
		
            if (gActiveTimerQueue.pFirstTimer == pTimerOnQueue) {
                gActiveTimerQueue.pFirstTimer = pUserTimer;
            }
		
            CL_FUNC_EXIT();
            return(CL_OK);
	    }
	
    }
    
    /* so the new node we are adding is the last node! */
    pUserTimer->pPreviousActiveTimer = gActiveTimerQueue.pLastTimer;
    pUserTimer->pNextActiveTimer = NULL;
    gActiveTimerQueue.pLastTimer->pNextActiveTimer = pUserTimer;
    gActiveTimerQueue.pLastTimer = pUserTimer;
    
    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
static ClRcT
tsActiveTimerDequeue (TsTimer_t* pUserTimer)
{
    TsTimer_t* pPreviousTimer = NULL;
    TsTimer_t* pNextTimer = NULL;

    CL_FUNC_ENTER();
    if (pUserTimer == NULL) {
	/* invalid timer */
        CL_FUNC_EXIT();
	return (CL_ERR_UNSPECIFIED);
    }

    pNextTimer = pUserTimer->pNextActiveTimer;
    pPreviousTimer = pUserTimer->pPreviousActiveTimer;

    if (pPreviousTimer) {
	    pPreviousTimer->pNextActiveTimer = pNextTimer;
    }
    if (pNextTimer) {
        pNextTimer->pPreviousActiveTimer = pPreviousTimer;
    }

    if (gActiveTimerQueue.pFirstTimer == pUserTimer) {
        gActiveTimerQueue.pFirstTimer = pNextTimer;
    }

    if (gActiveTimerQueue.pLastTimer == pUserTimer) {
        gActiveTimerQueue.pLastTimer = pPreviousTimer;
    }

    pUserTimer->pNextActiveTimer = NULL;
    pUserTimer->pPreviousActiveTimer = NULL;

    CL_FUNC_EXIT();
    return (CL_OK);
}
/*************************************************************************/
static void*
tsTimerTask (void* pArgument)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    ClTimerTimeOutT sleepTime = {0,0};
    ClTimerTypeT type ;
    TsTimer_t* pUserTimer = NULL;
    
    sleepTime.tsSec = 0;
    sleepTime.tsMilliSec = gActiveTimerQueue.tickInMilliSec;

    while (1) {
        /* run forever */
        ClInt64T diff;
        ClUint32T skewTicks;
        ClUint32T skewTicksTemp;
        struct timeval start;
        struct timeval end;
        gettimeofday(&start,NULL);
        returnCode = clOsalTaskDelay (sleepTime);
        gettimeofday(&end,NULL);
        /* 
         *  If this ever return CL_OSAL_ERR_OS_ERROR, it means that gOsalFunction 
         *  is un-initialized or been finalized. In case of finalized we can 
         *  simply return from this thread. Ideally clOsalTaskDelay should 
         *  not be used here. a better approach would be pthread_cond_timedwait. 
         *  but right now this is the temporary stuff which will work
         */
        if(CL_GET_ERROR_CODE(returnCode) == CL_OSAL_ERR_OS_ERROR && 
           CL_GET_CID(returnCode) == CL_CID_OSAL)
        {
            break;
        }
    
        diff = clTimerTimeDiff(&start,&end,CL_TIMER_DRIFT_THRESHOLD);

        if(diff < CL_TIMER_TICK_USECS)
        {
            skewTicks = 1;
            diff = 0;
        }
        else
        {
            skewTicks = diff/CL_TIMER_TICK_USECS;
        }

        returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);

        if (returnCode != CL_OK) {
            continue;
        }    

        skewTicksTemp = skew/(iteration+1);

        skew += skewTicksTemp % CL_TIMER_TICK_USECS;
        
        skewTicksTemp/=CL_TIMER_TICK_USECS;

        skew += diff % CL_TIMER_TICK_USECS;

        skewTicks += skewTicksTemp;

        ++iteration;

        if(currentTime > currentTime+iteration+skewTicks)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Timer overflow detected.Exiting timer\n"));
            clOsalMutexUnlock(gActiveTimerQueue.timerMutex);
            break;
        }

        currentTime += skewTicks;
    
        if (gActiveTimerQueue.pFirstTimer == NULL) {
            returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
            continue;
        }
    
        if( (gActiveTimerQueue.pFirstTimer->timestamp > currentTime))
        {
        
        }
        else {
            gettimeofday(&start,NULL);
            for (pUserTimer = gActiveTimerQueue.pFirstTimer;
                 (pUserTimer != NULL) && (pUserTimer->timestamp <= currentTime);
                 pUserTimer = pUserTimer->pNextActiveTimer) {
	    
                /* first remove the timer from the active queue */
                returnCode = tsActiveTimerDequeue (pUserTimer);
		
                /* check that the timer we are about to execute is valid */
                if ((pUserTimer->state != TIMER_ACTIVE) ||
                    (pUserTimer->fpTimeOutAction == NULL)) {
                    continue;
                } /* end of check to ensure that the timer we are about to execute is valid */
        
                if((type = pUserTimer->type) == CL_TIMER_ONE_SHOT)
                {
                    /* make the timer state as inactive if its a one-shot timer */
                    pUserTimer->state = TIMER_INACTIVE; 
                }
                switch (pUserTimer->spawnTask) {
                case CL_TIMER_SEPARATE_CONTEXT:
                    /* spawn off a task to run the timer function */
                    returnCode = clOsalTaskCreateDetached ("USER TIMER TASK",
                                                   CL_OSAL_SCHED_OTHER,
                                                   CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
                                                   TIMER_TASK_STACK_SIZE,
                                                   (void* (*) (void*)) pUserTimer->fpTimeOutAction,
                                                   pUserTimer->pActionArgument);
		    
                    if (returnCode != CL_OK) {
                        /* do the appropriate thing here */
                        break;
                    }
                    break;
		    
                case CL_TIMER_TASK_CONTEXT:
                    returnCode = pUserTimer->fpTimeOutAction (pUserTimer->pActionArgument);
                    break;
		    
                default:
                    /* this should never happen! we should have verified this
                     * in create/newTimerActivate
                     */
                    break;
                }

                if (type == CL_TIMER_REPETITIVE) {
		    
                    /* put the current time into this timer's timestamp */
                    pUserTimer->timestamp = currentTime+pUserTimer->timeInterval;
		    
                    /* This timer needs to fire again. So put into re-enqueue Queue */
                    returnCode = clQueueNodeInsert(gActiveTimerQueue.reEnqueueQueue, 
                                                   pUserTimer);
		    
                    if (returnCode != CL_OK) {
                        /* do the appropriate thing here */
                        break;
                    }
		    
                }
		
            } /* end of for (all timers with 0 expiry time) loop */    
            gettimeofday(&end,NULL);

            skew += clTimerTimeDiff(&start,&end,CL_TIMER_DRIFT_THRESHOLD);

        } /* end of else: so there are expired timers */
    
        tsTimerReEnqueue();
    
        returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
    
    } /* end of while (1) */
    
    return (NULL);
}
/*************************************************************************/
static TsTimer_t*
tsFreeTimerGet (void)
{
    return ((TsTimer_t*) clHeapAllocate ((ClUint32T)sizeof (TsTimer_t)));
}
/*************************************************************************/
static void
tsFreeTimerReturn (TsTimer_t* pUserTimer)
{
    if (pUserTimer) {
        clHeapFree (pUserTimer);
    }
}
/*************************************************************************/
void
tsAllActiveTimersPrint (void)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    ClUint32T count = 0;
    TsTimer_t* pUserTimer = NULL;

    returnCode = clOsalMutexLock (gActiveTimerQueue.timerMutex);

    if (returnCode != CL_OK) {
        return;
    }

    if (gActiveTimerQueue.pFirstTimer == NULL) {
        /* no elements to display */
        returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
        return;
    }

    for (pUserTimer = gActiveTimerQueue.pFirstTimer, count = 0;
     pUserTimer != NULL;
	 pUserTimer = pUserTimer->pNextActiveTimer, count++) {
	 clOsalPrintf ("(%d) Timer timeout = %us, timestamp->sec = %ld, timestamp->Microsec = %ld.\n",
		(ClInt32T)count,
		pUserTimer->timeOut.tsSec,
        pUserTimer->timestamp/1000000ULL,
        pUserTimer->timestamp%1000000ULL);
    }

    returnCode = clOsalMutexUnlock (gActiveTimerQueue.timerMutex);
}
/*************************************************************************/
ClRcT
clTimerUpdate(ClTimerHandleT timerHandle,ClTimerTimeOutT newTimeout)
{
    ClRcT returnCode = CL_ERR_INVALID_HANDLE;
    ClRcT retCode = CL_ERR_INVALID_HANDLE;
    TsTimer_t* pUserTimer = NULL;
    TsTimerState_e currentState;
    pUserTimer = (TsTimer_t*) timerHandle;

    CL_FUNC_ENTER();

    if (pUserTimer == NULL) {
        /* debug message */
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        CL_FUNC_EXIT();
        return (returnCode);
    }

    if (pUserTimer->state == TIMER_FREE) {
        /* debug message */
        returnCode = CL_TIMER_RC(CL_ERR_INVALID_HANDLE);
        CL_FUNC_EXIT();
        return (returnCode);
    }

    currentState = pUserTimer->state;

    if(TIMER_ACTIVE == currentState) {
        returnCode = clTimerStop(timerHandle);
        if(CL_OK != returnCode) {
            CL_FUNC_EXIT();
            return(returnCode);
        }
    }

    pUserTimer->timeOut.tsSec = newTimeout.tsSec + (newTimeout.tsMilliSec/1000);
    pUserTimer->timeOut.tsMilliSec = newTimeout.tsMilliSec % 1000;

    if(TIMER_ACTIVE == currentState) {
        timerHandle = pUserTimer;
        retCode = clTimerStart(timerHandle);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}
/*************************************************************************/
static void*
tsTimerReEnqueue()
{
    ClRcT returnCode = CL_OK;
    /*ClUint32T queueSize = 0;
    ClUint32T i = 0;*/
    TsTimer_t* pUserTimer = NULL;
    ClRcT retCode = 0;
    
    /* For each of the timers in the reEnqueue Queue, insert into active timer list
     *  and dequeue from the reEnqueue Queue 
     */
    while(returnCode == CL_OK){
		returnCode = clQueueNodeDelete(gActiveTimerQueue.reEnqueueQueue, (ClQueueDataT*)&pUserTimer);
		if(CL_OK != returnCode) {
		    break;
		}
		retCode = tsActiveTimerEnqueue (pUserTimer);
    }
	
    /* EL - This portion and the return code handling is suspicious */
    returnCode = CL_OK;
    return(0);
}
/*************************************************************************/
void deQueueCallBack(ClQueueDataT userData)
{
    
}
