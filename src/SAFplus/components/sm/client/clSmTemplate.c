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
 * ModuleName  : sm
 * File        : clSmTemplate.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Manager Transition Object    
 *               implementation                                          
 *************************************************************************/

/** @pkg cl.sm */

/* INCLUDES */
#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>

#include <clSmErrors.h>
#include <clSmTemplateApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

#define SM_LOG_AREA		"SM"
#define SM_LOG_CTX_CREATE	"CRE"
#define SM_LOG_CTX_DELETE	"DEL"
#define SM_LOG_CTX_TRANSITION	"TRANS"
#define SM_LOG_CTX_ADD		"ADD"
#define SM_LOG_CTX_SET		"SET"
#define SM_LOG_CTX_GET		"GET"
/**
 *  Create a new State machine type.
 *
 *  API to create a new State macine Type. Maximum states in the
 *  state machine is passed in the parameter. Each state is 
 *  identified by a state id (index) and captures the transition
 *  table at each table.
 *                                                                        
 *  @param maxStates maximum states in State Machine 
 *  @param sm        [out] handle to the newly created state machine type
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state machine handle <br/>
 *
 *  @see #clSmTypeDelete
 */
ClRcT
clSmTypeCreate(ClUint16T maxStates,
             ClSmTemplatePtrT* sm
             )
{
  ClRcT ret = CL_OK;
  int sz = 0;

  CL_FUNC_ENTER();
  CL_ASSERT(sm);  

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_CREATE,"Create State Machine Type [%d]", maxStates);

  if(sm && (maxStates > 0)) {
    /* allocate the states and assign to top */
    *sm = (ClSmTemplatePtrT) mALLOC(sizeof(ClSmTemplateT));
    if(*sm!=0) 
      {
        sz = sizeof(ClSmStatePtrT)*(int)maxStates;
        /* allocate space to hold maxStates handles  */
        (*sm)->top = (ClSmStatePtrT*) mALLOC((size_t)sz);
        if((*sm)->top) 
          {
            (*sm)->maxStates = maxStates;
            (*sm)->objects = 0;
            /* Introduced a semaphore for handling concurrent
             * SM Instance creation/deletion
             */
            ret = clOsalMutexCreate( &((*sm)->sem));
            if (CL_OK != ret)
            {
                mFREE((*sm)->top);
                mFREE(*sm);
                ret = SM_ERR_NO_SEMA;
            }
            else
            {
                /* 
                 * default initial state to first state
                 */
                (*sm)->init = (*sm)->top[0];
                (*sm)->type = SIMPLE_STATE_MACHINE;
#ifdef DEBUG
                (*sm)->name[0] = 0;
#endif
            }
          }
        else
          {
            mFREE(*sm);
          }
      }
    if(!(*sm) || !(*sm)->top)
      {
        ret = CL_SM_RC(CL_ERR_NO_MEMORY);
      }
  } else 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
    }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Delete State machine type.
 *
 *  API to delete a previously created state machine type. All the
 *  instances that are created are NOT deleted and their access to 
 *  the type will have unpredictable results. This API to be called 
 *  cautiously and be sure that the state machine type is no longer
 *  referenced.
 *                                                                        
 *  @param this State Machine type to be deleted
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on NULL sm handle <br/>
 *
 *  @see #clSmTypeCreate
 */
ClRcT 
clSmTypeDelete(ClSmTemplatePtrT this
             )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_DELETE,"Delete State Machine Type");

  if(this && this->top)
    {
      int i;

      clOsalMutexLock(this->sem);
      if(this->objects > 0)
        {
            clOsalMutexUnlock(this->sem);
          ret = SM_ERR_OBJ_PRESENT;
          CL_FUNC_EXIT();
          return ret;
        }
      clOsalMutexUnlock(this->sem);
      clOsalMutexDelete(this->sem);

      /* free the member objects - state table and then this object */
      for(i=0;i<this->maxStates;i++)
        {
          ClSmStatePtrT stateh = this->top[i];
          /* Bad design, only the pointer was given, why do you want to
           * free the memory associated to it ?
           */ 
          if(stateh)
            {
              clSmStateDelete(stateh);
            }
        }
      mFREE(this->top);
      mFREE(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }
  
  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Create a new State.
 *
 *  API to create a new State Object. State object captures the
 *  the possible transitions from it.  The maximum transitions 
 *  in the state is specified by maxTOs.  The API returns 
 *  a handle to the newly created state with empty transitions
 *  in it.
 *                                                                        
 *  @param maxTOs  maximum transition objects allowed in State
 *  @param state   [out] newly created state handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state handle <br/>
 *
 *  @see #clSmStateDelete
 */
ClRcT
clSmStateCreate(ClUint16T maxTOs, 
              ClSmStatePtrT* state
              )
{
  ClRcT ret = CL_OK;
  int sz = 0; 
  CL_FUNC_ENTER();
  CL_ASSERT(state);

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_CREATE,"Create State with [%d] Transitions", 
                        maxTOs);

  if(state)
    {
      /* allocate the state  */
      *state = (ClSmStatePtrT) mALLOC(sizeof(ClSmStateT));
      if(*state) 
        {
          /* allocate event transition table */
          memset((*state), 0, sizeof(ClSmStateT));
          if(maxTOs>0) 
            {
              sz = sizeof(ClSmEventTransitionT)*(int)maxTOs;
              
              (*state)->eventTransitionTable = 
                (ClSmEventTransitionPtrT) mALLOC(sizeof(ClSmEventTransitionT)*maxTOs);
              if((*state)->eventTransitionTable)
                {
                  memset((*state)->eventTransitionTable, 0, (size_t)sz);
                } else 
                  {
                    /* free the state handle */
                    mFREE(*state);
                    
                    ret = CL_SM_RC(CL_ERR_NO_MEMORY);
                    CL_FUNC_EXIT();
                    return ret;
                  }
            }
          (*state)->maxEventTransitions = maxTOs;
        } else 
          {
            ret = CL_SM_RC(CL_ERR_NO_MEMORY);
          }
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Delete State object from State Machine Type.
 *
 *  API to delete a previously created state. All the transitions that
 *  are referring are NOT deleted and their access will have
 *  unpredictable results. This API to be called cautiously and be
 *  sure that the state deleted is no longer referenced.
 *                                                                        
 *  @param this State to be deleted
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on NULL sm handle <br/>
 *
 *  @see #clSmStateCreate
 */
ClRcT
clSmStateDelete(ClSmStatePtrT this
              )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_DELETE,"Delete State");

  if(this) 
    {
      /* free the state object */
      if(this->eventTransitionTable) 
        {
          int i;
          /* another bad design, since no body is there to free the
           * event transition table, ending up here 
           */
          for(i=0;i<this->maxEventTransitions;i++)
            {
              ClSmTransitionPtrT to = this->eventTransitionTable[i].transition ;
              if(to)
                {
                  clSmTransitionDelete(to);
                }
            }
          mFREE(this->eventTransitionTable);
        }
      mFREE(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Create a new Transition Object.
 *
 *  API to create a new state transition object. State transition
 *  captures the next state the transition leads to and the function
 *  to call (transition handler).  The function pointer can be empty
 *  (null).
 *                                                                        
 *  @param next Next state object handle
 *  @param func Transition function
 *  @param to   [out] newly created transition object
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on null handle <br/>
 *
 *  @see #clSmTransitionDelete
 */
ClRcT
clSmTransitionCreate(ClSmStatePtrT next,
                   ClSmFuncPtrT func,
                   ClSmTransitionPtrT* to
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(next);  
  CL_ASSERT(to);  

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_TRANSITION,"Create State Transition ");

  if(next && to) 
    {

      /* allocate the TO  */
      *to = (ClSmTransitionPtrT) mALLOC(sizeof(ClSmTransitionT));
      if(*to) 
        {
          (*to)->nextState = next;
          (*to)->transitionHandler = func;

#ifdef DEBUG
          (*to)->transitionName[0] = 0;
#endif

        } else
          {
            ret = CL_SM_RC(CL_ERR_NO_MEMORY);
          }

    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }
  
  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Delete Transition Object.
 *
 *  API to delete a previously created transition object. All the
 *  references to this transition object should be removed by the
 *  caller, otherwise their access would have unpredictable
 *  results. This API to be called cautiously and be sure that the
 *  transition object is no longer referenced.
 *
 *  @param this Transition object handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on NULL sm handle <br/>
 *
 *  @see #clSmTransitionCreate
 */
ClRcT
clSmTransitionDelete(ClSmTransitionPtrT this
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_TRANSITION,"Delete State Transition");

  if(this) 
    {
      /* free the object */
      mFREE(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Set State Information.
 *
 *  API to set the properties of a state, which includes type, entry
 *  and exit functions.  
 *                                                                        
 *  @param this    Previously created state object handle
 *  @param sType   State Type
 *  @param sEntry  Entry function pointer (called when the state is 
 *                 activated/entered)
 *  @param sExit   Exit function pointer (called when the state is 
 *                 de-activated/exited)
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state handle <br/>
 *
 *  @see #clSmStateEntryInstall
 *  @see #clSmStateExitInstall
 *
 */
ClRcT
clSmStateSet(ClSmStatePtrT this,
           ClSmStateTypeT sType,
           ClSmFuncPtrT   sEntry,
           ClSmFuncPtrT   sExit
           )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  /*CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Set State Info [%d, >>%p %p<< ]", sType, sEntry, sExit));*/

  if(this) 
    {
      this->type = sType;
      this->entry = sEntry;
      this->exit = sExit;
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
  
}


/**
 *  Add Transition Object to State.
 *
 *  This API Adds/Copies contents of the Transition object to the
 *  State Object.
 *                                                                        
 *  @param this State Object to which Transition Object to be added
 *  @param event Event that triggers the transition
 *  @param to Transition Object that needs to be triggered on the event
 *
 *  @returns 
 *    CL_OK on successful addition <br/>
 *    ERROR to indicate failure
 *
 */
ClRcT
clSmStateTransitionAdd(ClSmStatePtrT this, 
                     ClSmEventIdT eventId, 
                     ClSmTransitionPtrT to
                     )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  
  CL_ASSERT(to); 

  clLogTrace(SM_LOG_CTX_TRANSITION,SM_LOG_CTX_ADD,"State Transition Add [%d]",
                        eventId);

  if(this && this->eventTransitionTable && to) 
    {
      /* see if there is more space */
      if(eventId >= (ClSmEventIdT)this->maxEventTransitions || eventId < 0) 
        {
          ret = SM_ERR_OUT_OF_RANGE;
        } else 
          {
            this->eventTransitionTable[eventId].transition = to;
          }
    } else
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Install State Entry function.
 *
 *  API to install the state entry function. 
 *                                                                        
 *  @param this    Previously created state object handle
 *  @param sEntry  Entry function pointer (called when the state is 
 *                 activated/entered)
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state handle <br/>
 *
 *  @see #clSmStateExitInstall
 *  @see #clSmStateSet
 *
 */
ClRcT
clSmStateEntryInstall(ClSmStatePtrT this,
                    ClSmFuncPtrT   sEntry
                    )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  if(this) 
    {
      /*CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Install Entry state [%d, >>%p ]", this->type, sEntry));*/
      this->entry = sEntry;
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
  
}

/**
 *  Install State Exit function.
 *
 *  API to install the state exit function. 
 *                                                                        
 *  @param this    Previously created state object handle
 *  @param sExit   Exit function pointer (called when the state is 
 *                 exitted)
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state handle <br/>
 *
 *  @see #clSmStateExitInstall
 *  @see #clSmStateSet
 *
 */
ClRcT
clSmStateExitInstall(ClSmStatePtrT this,
                   ClSmFuncPtrT sExit
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  if(this) 
    {
      /*CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Install Exit state [%d, %p<< ]", this->type, sExit));*/
      this->exit = sExit;
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
  
}


/**
 *  Set initial State of the state machine.
 *
 *  API to set the initial state of the state machine.  Any 
 *  instance created for the state machine type will have its
 *  initial state set pointed to sId (initial state).
 *                                                                        
 *  @param this  State machine type object
 *  @param sId   Initial state the state machine to take when
 *               instantiated.
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state handle <br/>
 *    SM_ERR_OUT_OF_RANGE if sId is more than the states the 
 *             state machine type holds
 *
 */
ClRcT 
clSmTypeInitStateSet(ClSmTemplatePtrT this,
                   ClSmStateIdT sId
                   )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);

  clLogTrace(SM_LOG_AREA,SM_LOG_CTX_SET,"Set Init State [%d]", 
                        sId);

  /* initialize current, temporarily to the init of top */
  /* This just sets the init state and doesn't start anything */
  if(this && this->top) 
    {
      /* see if its a valid state */
      if(sId >= this->maxStates) 
        {

          ret = SM_ERR_OUT_OF_RANGE;
        } else 
          {
            this->init = this->top[sId];
          }

    } else
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Adds State to State Machine.
 *
 *  Adds/Copies contents of the state object to the SM Type object.
 *                                                                        
 *  @param this   State Machine Object
 *  @param sId    State Id (index identifying the slot the state object
 *                to be added)
 *  @param state  State Object handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state and/or type handles <br/>
 *    SM_ERR_OUT_OF_RANGE if sId is out of range 
 *
 */
ClRcT
clSmTypeStateAdd(ClSmTemplatePtrT this, 
               ClSmStateIdT sId, 
               ClSmStatePtrT state
               )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(state); 
  CL_ASSERT(this);

  if(this && this->top && state) 
    {

      clLogTrace(SM_LOG_AREA,SM_LOG_CTX_ADD,"Add State [%d] Type [%d]", 
                            sId, 
                            state->type);

      /* see if there is more space */
      if(sId >= this->maxStates) 
        {
          ret = SM_ERR_OUT_OF_RANGE;
        } else 
          {
            this->top[sId] = state;
          }
    } else
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }
  
  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Removes State from State Machine.
 *
 *  Removes state from state machine type.
 *                                                                        
 *  @param this   State Machine Object
 *  @param sId    State Id (index whose state has to be removed)
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state <br/>
 *    SM_ERR_OUT_OF_RANGE if sId is out of range 
 *
 */
ClRcT
clSmTypeStateRemove(ClSmTemplatePtrT this, 
                  ClSmStateIdT sId
                  )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);

  if(this && this->top) 
    {

      clLogTrace(SM_LOG_AREA,SM_LOG_CTX_DELETE,"Remove State [%d]", 
                            sId);

      /* see if its a valid state */
      if(sId >= this->maxStates) 
        {

          ret = SM_ERR_OUT_OF_RANGE;
        } else 
          {
            /* another place of bad design.  Here, there is a potential
             * memory leak and the caller is supposed to free the state handle.
             */
            this->top[sId] = 0;
          }
    } else
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  
  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Get the state handle.
 *
 *  API to retrieve the state object for a given state identifier.
 *                                                                        
 *  @param this   State Machine Type
 *  @param sId    State Id (index identifying the state object)
 *  @param sm     [out] State Object handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null state <br/>
 *    SM_ERR_OUT_OF_RANGE if sId is out of range 
 *
 */
ClRcT
clSmTypeStateGet(ClSmTemplatePtrT this, 
               ClSmStateIdT sId,
               ClSmStatePtrT* sm
               )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(sm); 
  CL_ASSERT(this);

  if(this && this->top && sm) 
    {

      clLogTrace(SM_LOG_AREA,SM_LOG_CTX_GET,"Get State [%d]", sId);

      /* see if its a valid state */
      if(sId >= this->maxStates) 
        {

          ret = SM_ERR_OUT_OF_RANGE;
        } else 
          {
            *sm = this->top[sId];
          }
    } else
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }
  
  CL_FUNC_EXIT();
  return ret;

}

/* ======== private functions go here  ========= */
/**@#-*/

void
smTransitionShow(ClSmTransitionPtrT this)
{ 
  if(this)
    {
      if(this->nextState) 
        clOsalPrintf("Next: %02d/%-10s ", this->nextState->type,
#ifdef DEBUG
        this->nextState->name
#else
        ""
#endif
	);
      else
        clOsalPrintf("Next: %-13s ","NULL");
      clOsalPrintf(" %p:%s \n", this->transitionHandler, 
#ifdef DEBUG
        this->transitionName
#else
        ""
#endif
	);
    }
}


void
smStateShow(ClSmStatePtrT this)
{ 
  int i=0;
  
  if(this)
    {
      clOsalPrintf("  State : %d/%s\n  Entry : %p/%s\n  Exit  : %p/%s\n  Parent: %d/%s\n  Init  : %d/%s\n",
               this->type,
#ifdef DEBUG
               this->name,
               this->entry,
               this->entryFun,
               this->exit,
               this->exitFun,
               this->parent?this->parent->type:-1,
               this->parent?this->parent->name:"",
               this->init?this->init->type:-1,
               this->init?this->init->name:""
#else
               "",
               this->entry,
               "",
               this->exit,
               "",
               this->parent?this->parent->type:-1,
               "",
               this->init?this->init->type:-1,
               ""
#endif
               );
      /* loop thru the transition table and print */
      clOsalPrintf("  Transition Table:\n");
      for(i=0;this->eventTransitionTable && i < (int)this->maxEventTransitions;i++) 
        {
          if(this->eventTransitionTable[i].transition)
            {
              clOsalPrintf("   Event: %02d/%-10s ",
                       i,
#ifdef DEBUG
                       this->eventTransitionTable[i].name
#else
                       ""
#endif
                       );
              smTransitionShow(this->eventTransitionTable[i].transition);
            }
        }
      clOsalPrintf("\n");
    }
}


void
smTypeShow(ClSmTemplatePtrT this)
{
  int i=0;
  
  if(this)
    {
      
      clOsalPrintf("\nState Machine [%s] Details: \n", 
#ifdef DEBUG
         this->name
#else
         ""
#endif
         );
      clOsalPrintf("---------------------------------------------------------------\n");
      
      for(i=0;i<(int)this->maxStates;i++) 
        {
          ClSmStatePtrT tmp = this->top[i];
          
          if(tmp)
            {
              smStateShow(tmp);
            }
        }
      clOsalPrintf("---------------------------------------------------------------\n");
    }
}

#ifdef DEBUG

void
smStateNameSet(ClSmStatePtrT this, char* name, char* entry, char* exit)
{
  /* go ahead and set the name if its not null
   */
  if(this)
    {
      if(name)
        strncpy(this->name, name, MAX_STR_NAME);
      if(entry)
        strncpy(this->entryFun, entry, MAX_STR_NAME);
      if(exit)
        strncpy(this->exitFun, exit, MAX_STR_NAME);
    }
}

void
smTypeNameSet(ClSmTemplatePtrT this, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(this && name ) 
    {
      strncpy(this->name, name, MAX_STR_NAME);
    }
}


void
smEventNameSet(ClSmEventTransitionPtrT this, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(this && name ) 
    {
      strncpy(this->name, name, MAX_STR_NAME);
    }
}

void
smTransitionNameSet(ClSmTransitionPtrT this, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(this && name ) 
    {
      strncpy(this->transitionName, name, MAX_STR_NAME);
    }
}

#endif


/**@#-*/
