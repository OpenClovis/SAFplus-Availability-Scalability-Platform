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
 * File        : clSmTemplateApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Machine template definitions 
 *************************************************************************/

#ifndef _CL_SM_TEMPLATE_API_H
#define _CL_SM_TEMPLATE_API_H

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.sm */

/* INCLUDES */
#include <stdlib.h>
#include <clOsalApi.h>

/* DEFINES */

#define mALLOC(sz)       clHeapCalloc(1,sz)
#define mFREE(ptr)       clHeapFree(ptr)

#define MAX_STR_NAME     64


/* ENUM */

/* forward declarations */
typedef struct SMState                  ClSmStateT;
typedef ClSmStateT*                      ClSmStatePtrT;
typedef ClUint16T                      ClSmStateIdT;
typedef ClInt16T                       ClSmStateTypeT;
typedef ClInt16T                       ClSmEventIdT;

/**
 * Event Structure.
 * 
 * An occurrence that may trigger a state
 * transition. Event types include an explicit signal from outside the
 * system, an invocation from inside the system, the passage of a
 * designated period of time, or a designated condition becoming true
 */
typedef struct SMEvent
{
  ClSmEventIdT      eventId;                 /**< event id */
  ClPtrT            payload;                 /**< user payload */
} ClSmEventT;

typedef ClSmEventT*       ClSmEventPtrT;
typedef ClRcT (*ClSmFuncPtrT)(ClSmStatePtrT prev, ClSmStatePtrT* next, ClSmEventPtrT event);

/**
 * Transition Object.
 * 
 * Captures the next state and the transition handler to use.
 */
typedef struct SMTransition
{
  ClSmStatePtrT              nextState;               /**< next state */
  ClSmFuncPtrT            transitionHandler;       /**< transition handler */

#ifdef DEBUG
  char                   transitionName[MAX_STR_NAME]; /**< Transition Name */
#endif

} ClSmTransitionT;

typedef ClSmTransitionT*      ClSmTransitionPtrT;


/**
 * Event transition Object.
 *
 * Captures Event Transition (event id and transition object to use
 * for transition).
 */
typedef struct SMEventTransition
{
  ClSmTransitionPtrT         transition;              /**< Transition Object */

#ifdef DEBUG
  char                   name[MAX_STR_NAME];      /**< Event Name */
#endif

} ClSmEventTransitionT;  

typedef ClSmEventTransitionT*     ClSmEventTransitionPtrT;


/**
 * State Object.
 *
 * A condition during the life of an object in which it satisfies some
 * condition, performs some action, or waits for some event.
 *
 */
struct SMState 
{
  ClSmStateTypeT          type;                    /**< State type */
  ClSmEventTransitionPtrT    eventTransitionTable;    /**< Event Transition Table */
  ClUint16T             maxEventTransitions;     /**< max entries */
  ClSmFuncPtrT            entry;                   /**< state entry handler */
  ClSmFuncPtrT            exit;                    /**< state exit handler */
  ClSmStatePtrT              parent;                  /**< parent state,for HSM */  
  ClSmStatePtrT              init;                    /**< init state,for HSM */

#ifdef DEBUG
  char                   name[MAX_STR_NAME];      /**< State Name */
  char                   entryFun[MAX_STR_NAME];  /**< Entry function Name */
  char                   exitFun[MAX_STR_NAME];   /**< Exit function Name */
#endif

};

typedef enum
  {
    SIMPLE_STATE_MACHINE,
    HIERARCHICAL_STATE_MACHINE
  }
ClSmTypeT;

/**
 * State Machine Object.  
 *
 * Describes the possible sequences of states and actions through
 * which the element instances can proceed during its lifetime as a
 * result of reacting to discrete events.  Once the state machine type
 * is created, use @link SMInstance @endlink to create a state machine
 * instance.
 */
typedef struct SMTemplate
{
  ClSmStatePtrT*             top;                     /**< states */
  ClUint16T             maxStates;               /**< Max States */
  ClSmTypeT               type;                    /**< Type of state machine */
  ClSmStatePtrT              init;                    /**< initial state */

#ifdef DEBUG
  char                   name[MAX_STR_NAME];      /**< State Machine Name */
#endif
  ClUint32T             objects;                 /**< Objects of this type alive*/
  ClOsalMutexIdT            sem;

} ClSmTemplateT;

typedef ClSmTemplateT*        ClSmTemplatePtrT;

/**
 * State Machine Log Object.  
 *
 * State transition Log info. Captures the event, from, to
 * information.
 */
typedef struct SMLogInfo
{
  ClSmEventIdT            eventId;                 /**< event id */
  ClSmStatePtrT              from;                    /**< 'From' State */
  ClSmStatePtrT              to;                      /**< 'to' State */
} ClSmLogInfoT;

typedef ClSmLogInfoT*     ClSmLogInfoPtrT;

/** ret - return value (ClRcT)
 * thisInst - State machine Type Handle (ClSmTemplatePtrT)
 * nTrans - number of transitions 
 * state_h - State Handle  (ClSmStatePtrT)
 * entry - entry  Handler (ClSmFuncPtrT)
 * exit - exit  Handler (ClSmFuncPtrT)
 */
#define SM_TYPE_STATE_CREATE(ret, thisInst, nTrans, state_h, stateId, entry, exit) \
  do \
  { \
    ret = clSmStateCreate((nTrans), &(state_h)); \
    if(ret==CL_OK) \
      ret = clSmStateSet((state_h), (stateId), (entry), (exit)); \
    if(ret==CL_OK) \
      ret = clSmTypeStateAdd(thisInst, (stateId), (state_h)); \
  } \
  while(0)

#define SM_TYPE_HRCHY_STATE_CREATE(ret, thisInst, nTrans, state_h, parentState, stateId, entry, exit)\
do \
{ \
    SM_TYPE_STATE_CREATE(ret, thisInst, nTrans, state_h, stateId, entry, exit); \
    if (ret == CL_OK) {                          \
        state_h->parent = parentState;              \
        thisInst->type = HIERARCHICAL_STATE_MACHINE;    \
    }                                               \
} \
while (0)

#define SM_TYPE_HRCHY_INIT_SET(state_h) (state_h->parent->init = state_h)

/* ret - return value (ClRcT)
 * thisInst - State Handle (State_h)
 * stateId - State Identifier (ClSmStateIdT)
 * event - Event Identifier  (ClSmEventIdT)
 * nextStateId - Next State Identifier (ClSmStateIdT)
 * transition - Transition Handler (ClSmFuncPtrT)
 * to - transition object (ClSmTransitionPtrT)
 */
#define SM_STATE_TRANSITION_CREATE(ret,thisInst,stateId,event,nextStateId,transition,to) \
  do \
  { \
    ClSmStatePtrT tmpStateHandle; \
    ClSmStatePtrT nxtStateHandle; \
    ret = clSmTypeStateGet(thisInst, stateId, &tmpStateHandle); \
    if(ret==CL_OK) \
      ret = clSmTypeStateGet(thisInst, nextStateId, &nxtStateHandle); \
    if(ret==CL_OK) \
      ret = clSmTransitionCreate(nxtStateHandle, transition, &to); \
    if(ret==CL_OK) \
      ret = clSmStateTransitionAdd(tmpStateHandle, event, to); \
  } \
  while(0)

/**@#-*/

/* -------------- Transition Object API's ------------------ */

ClRcT                   clSmTransitionCreate(CL_IN ClSmStatePtrT next, 
                                            CL_IN ClSmFuncPtrT func,
                                            CL_OUT ClSmTransitionPtrT* transObj);
ClRcT                   clSmTransitionDelete(ClSmTransitionPtrT thisInst);

/* -------------- Type API's ------------------ */
ClRcT                   clSmTypeCreate(CL_IN ClUint16T maxStates, CL_OUT ClSmTemplatePtrT* sm);
ClRcT                   clSmTypeDelete(CL_IN ClSmTemplatePtrT thisInst);

ClRcT                   clSmTypeInitStateSet(CL_IN ClSmTemplatePtrT thisInst, 
                                            CL_IN ClSmStateIdT initStateId);
ClRcT                   clSmTypeStateAdd(CL_IN ClSmTemplatePtrT thisInst, 
                                        CL_IN ClSmStateIdT sId, 
                                        CL_IN ClSmStatePtrT state);
ClRcT                   clSmTypeStateRemove(CL_IN ClSmTemplatePtrT thisInst, 
                                           CL_IN ClSmStateIdT sId);
ClRcT                   clSmTypeStateGet(CL_IN ClSmTemplatePtrT thisInst, 
                                        CL_IN ClSmStateIdT sId,
                                        CL_OUT ClSmStatePtrT* sm);

/* -------------- State API's ------------------ */
ClRcT                   clSmStateCreate(CL_IN ClUint16T maxTOs, CL_OUT ClSmStatePtrT* state);
ClRcT                   clSmStateDelete(CL_IN ClSmStatePtrT thisInst);

ClRcT                   clSmStateSet(CL_IN ClSmStatePtrT thisInst, 
                                    CL_IN ClSmStateTypeT sType,
                                    CL_IN ClSmFuncPtrT sEntry,
                                    CL_IN ClSmFuncPtrT sExit);
ClRcT                   clSmStateTransitionAdd(CL_IN ClSmStatePtrT thisInst, 
                                              CL_IN ClSmEventIdT eventId, 
                                              CL_IN ClSmTransitionPtrT to);
ClRcT                   clSmStateEntryInstall(CL_IN ClSmStatePtrT thisInst, CL_IN ClSmFuncPtrT entry);
ClRcT                   clSmStateExitInstall(CL_IN ClSmStatePtrT thisInst, CL_IN ClSmFuncPtrT exit);

/* -------------- Show API's ------------------ */

void                     smTransitionShow(CL_IN ClSmTransitionPtrT thisInst);
void                     smTypeShow(CL_IN ClSmTemplatePtrT thisInst);
void                     smStateShow(CL_IN ClSmStatePtrT thisInst);

#ifdef DEBUG

void                     smStateNameSet(CL_IN ClSmStatePtrT thisInst, 
                                        CL_IN char* name,
                                        CL_IN char* entry,
                                        CL_IN char* exit);
void                     smTypeNameSet(CL_IN ClSmTemplatePtrT thisInst, CL_IN char* name);
void                     smEventNameSet(CL_IN ClSmEventTransitionPtrT thisInst, CL_IN char* name);
void                     smTransitionNameSet(CL_IN ClSmTransitionPtrT thisInst, CL_IN char* name);

#endif

/**@#+*/


#ifdef __cplusplus
}
#endif

#endif  /*  _CL_SM_TEMPLATE_API_H */
