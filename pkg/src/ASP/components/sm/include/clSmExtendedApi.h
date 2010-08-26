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
 * ModuleName  : sm
 * File        : clSmExtendedApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Extended State Machine Defn        
 *************************************************************************/

#ifndef _CL_SM_EXTENDED_API_H
#define _CL_SM_EXTENDED_API_H

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.sm.esm */

/* INCLUDES */
#include <clQueueApi.h>

#include <clSmTemplateApi.h>
#include <clSmBasicApi.h>
#include <string.h>

/* DEFINES */


#define ESM_HISTORY_STATE     -1
#define ESM_LOG_ENTRIES       20

#define ESM_LOCK(thisInst)        clOsalMutexLock((thisInst)->lock)
#define ESM_UNLOCK(thisInst)      clOsalMutexUnlock((thisInst)->lock)

#define ESM_IS_PAUSED(thisInst)         ((thisInst)->pause==1)
#define ESM_PAUSE(thisInst)             ((thisInst)->pause=1)
#define ESM_CONTINUE(thisInst)          ((thisInst)->pause=0)

#define ESM_IS_BUSY(thisInst)           ((thisInst)->busy==1)
#define ESM_SET_BUSY_STATE(thisInst)    ((thisInst)->busy=1)
#define ESM_SET_IDL_STATE(thisInst)     ((thisInst)->busy=0)

#define ESM_IS_DROP_ON_PAUSE(thisInst)  ((thisInst)->dropOrQ == DROP_ON_PAUSE)
#define ESM_DROP_ON_PAUSE(thisInst)     ((thisInst)->dropOrQ = DROP_ON_PAUSE)
#define ESM_Q_ON_PAUSE(thisInst)        ((thisInst)->dropOrQ = QUEUE_ON_PAUSE)

#define SMQ_CREATE(q)         clQueueCreate(0, dummyCallBack, dummyCallBack, &(q))
#define SMQ_ENQUEUE(q,item)   clQueueNodeInsert((q), (ClQueueDataT)(item))
#define SMQ_DEQUEUE(q,item)   clQueueNodeDelete((q), (ClQueueDataT*)(&(item)))
#define SMQ_SIZE(q, sz)       clQueueSizeGet((q), &(sz))

typedef ClQueueT               ClSmQueueT;

struct SMCircularIdx
{
  ClUint16T   idx;
  ClUint16T   max;
};
typedef struct SMCircularIdx ClSmCircularIdxT;

#define SM_IDX_INIT(cidx,n) ((cidx).idx=0,(cidx).max=n)
#define SM_LOG_IDX(cidx,i)  ((cidx).idx+(i))%((cidx).max)
#define SM_LOG_BKIDX(cidx,i) ((cidx).idx+(cidx).max-(i))%((cidx).max)
#define SM_LOG_IDX_INCR(cidx) (cidx).idx=SM_LOG_IDX((cidx),1)

#define ESM_LOG_SZ           sizeof(ClSmLogInfoT)

typedef struct ESMLog
{
  ClSmCircularIdxT     idx;               /**< log index */
  ClUint8T*          buffer;            /**< log buffer */
} ClExSmLogT;

#define ESM_LOG_INIT(log,n) \
 (SM_IDX_INIT((log).idx,n), \
  (log).buffer=(ClUint8T *) memset(mALLOC(ESM_LOG_SZ*n),0, (ESM_LOG_SZ*n)))
#define ESM_LOG_BACK_GET(log,i)  \
 (ClSmLogInfoPtrT)((log).buffer+(SM_LOG_BKIDX((log).idx, i)* ESM_LOG_SZ))
#define ESM_LOG_GET(log,i)  \
 (ClSmLogInfoPtrT)((log).buffer+(SM_LOG_IDX((log).idx, i)* ESM_LOG_SZ))
#define ESM_LOG_BUF(log)      ESM_LOG_GET(log,0)
#define ESM_LOG_IDX_INCR(log) SM_LOG_IDX_INCR((log).idx)


/* ENUM */

/**
 * Extended State Machine APIs.  
 *
 * <h1>ESM Overview</h1>
 * 
 * Clovis State Machine library contains API's to create state
 * machines and their run time instances.  
 *
 * A FiniteStateMachine (FSM) is a representation of an event driven
 * system (a system that is driven by external events that are input
 * into the system). In an event-driven system, the system makes a
 * transition from one state (previous state) to another prescribed
 * new state (new state), provided that the condition that defines the
 * change is satisfied.  In other words, a Finite-state machine is a
 * pre-determined network which has a finite number of states that are
 * interconnected based on the external events that trigger the state
 * transition.
 *
 * A Finite-State machine contains a finite number of
 * states, events and user call-back routines. Simple state machines
 * can be represented by networking a set of states and transition
 * events, and we refer to it as Finite-State Machine (FSM) in this
 * document. 
 * 
 * A Simple FSM just captures the states, transitions
 * and events that trigger them. In some cases, the state machine
 * need to support conditional transitions, history, event queues
 * and instance locking capabilities.  These are special cases
 * and for efficiency purposes these functionalities are
 * encapsulated in ESM (Extended State Machine). An ESM includes
 * all capabilites of simple FSM, plus the ones mentioned above.
 *
 * <h1>Interaction with other components</h1>
 * none
 *
 * <h1>Configuration</h1>
 * none
 *
 * <h1>ESM Usage Scenario(s)</h1>
 *
 * Following code block shall explain the usage of extended
 * state machine instance
 *
 * <blockquote>
 * <pre>
 *@@    /@ creation of state machine type uses the same
 *@@       API's as regular state machine @/
 *@@ 
 *@@    /@ API to create extended state machine instance @/
 *@@    ClExSmInstancePtrT instance;
 *@@    ClRcT        ret;
 *@@ 
 *@@    ret = clEsmInstanceCreate(sm, &instance);
 *@@    /@ Start the State machine @/
 *@@    if(ret == CL_OK)
 *@@      clEsmInstanceStart(instance);
 * </pre>
 * </blockquote>
 * Following code block would add events to the instance event Q.
 * <blockquote>
 * <pre>
 *@@    /@ API to add events to the instance Q @/
 *@@    ret = smInstanceEventAdd(instance, event);
 * </pre>
 * </blockquote>
 * Following code block would process the events from the event Q
 * <blockquote>
 * <pre>
 *@@    /@ API to handle events from the instance Q @/
 *@@    ret = clEsmInstanceProcessEvents(instance);
 * </pre>
 * </blockquote>
 *
 * @pkgdoc  cl.sm.esm
 * @pkgdoctid Extended State Machine Library
 *
 */

typedef enum {
    DROP_ON_PAUSE,
    QUEUE_ON_PAUSE
} ClSmPauseT;


/**
 * Extended State Machine Instance Object.
 *
 * Holds additional information that are required to run the extended
 * state machine like, previous state, lock, paused or not, queue
 * datastructures to hold the events.
 */
typedef struct ESMInstance
{
  ClSmInstancePtrT        fsm;               /**< State machine Instance */
  ClSmStatePtrT           previous;          /**< previous state */
  ClOsalMutexIdT          lock;              /**< locked/not */
  ClInt8T            pause;             /**< Paused / not */
  ClInt8T                busy;              /** busy or not */
  ClSmPauseT           dropOrQ;           /**< Drop or Q on pause */
  ClSmQueueT           q;                 /**< Event Queue */
  ClExSmLogT            log;               /**< event, transition log */
} ClExSmInstanceT;

typedef ClExSmInstanceT*     ClExSmInstancePtrT;

/**
 * Extended State Machine Q Node.
 *
 * Event Q Data structure
 */
typedef struct SMQueueItem
{
  ClSmEventT           event;
} ClSmQueueItemT;
  
typedef ClSmQueueItemT* ClSmQueueItemPtrT;

/**@#-*/

/* Function Prototypes */

/* constructor/destructor functions */

ClRcT                clEsmInstanceCreate(CL_IN ClSmTemplatePtrT sm, 
                                        CL_OUT ClExSmInstancePtrT* instance);
ClRcT                clEsmInstanceDelete(CL_IN ClExSmInstancePtrT thisInst);

/* public functions */
ClRcT                clEsmInstanceEventAdd(CL_IN ClExSmInstancePtrT thisInst, CL_IN ClSmEventPtrT msg);
ClRcT                clEsmInstanceProcessEvent(CL_IN ClExSmInstancePtrT thisInst);
ClRcT                clEsmInstanceProcessEvents(CL_IN ClExSmInstancePtrT thisInst);

ClRcT                clEsmInstanceStart(CL_IN ClExSmInstancePtrT thisInst);
ClRcT                clEsmInstanceRestart(CL_IN ClExSmInstancePtrT thisInst);
ClRcT                clEsmInstancePause(CL_IN ClExSmInstancePtrT thisInst);
ClRcT                clEsmInstanceContinue(CL_IN ClExSmInstancePtrT thisInst);

void                  clEsmInstanceShow(CL_IN ClExSmInstancePtrT thisInst);
void                  clEsmInstanceLogShow(CL_IN ClExSmInstancePtrT thisInst, CL_IN ClUint16T items);

#ifdef DEBUG
void                  esmInstanceNameSet(CL_IN ClExSmInstancePtrT thisInst, CL_IN char* name);
#endif


/**@#+*/

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_SM_EXTENDED_API_H */
