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
 * File        : clSmBasic.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Manager implementation       
 *************************************************************************/
/** @pkg cl.sm.fsm */

/* INCLUDES */
#include <string.h>


#include <clCommon.h>
#include <clDebugApi.h>

#include <clSmErrors.h>
#include <clSmBasicApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

extern ClRcT clHsmInstanceOnEvent(ClSmInstancePtrT this, ClSmEventPtrT msg);
extern ClRcT fsmInstanceOnEvent(ClSmInstancePtrT this, ClSmEventPtrT msg);

#ifdef DEBUG
ClRcT smDbgInit()
{
    ClRcT ret= CL_OK ;
    ret= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
    }
    return ret;
}
#endif

/**
 *  Create a new State machine Instance.
 *
 *  API to create a new State macine Instance object from a
 *  given state machine type.
 *                                                                        
 *  @param sm        specifies the SM type the instance should be
 *  @param instance  [out] handle to newly created instance object
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null sm / instance <br/>
 *
 *  @see #clSmInstanceDelete
 */
ClRcT
clSmInstanceCreate(ClSmTemplatePtrT sm, 
                 ClSmInstancePtrT* instance
                 )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(instance);  
  CL_ASSERT(sm);  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Create State Machine Instance"));

  if(sm && instance) 
    {
      /* allocate the instance space */
      *instance = (ClSmInstancePtrT) mALLOC(sizeof(ClSmInstanceT));
      if(*instance!=0) 
        {
          (*instance)->sm = sm;
          if(!sm->init) 
            {
              CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Init State is empty! Setting to first state"));
              sm->init = sm->top[0];
            }          
          (*instance)->current = (ClSmStatePtrT)sm->init; /* default to init */
          /* increment the references in 'sm' type */
          clOsalMutexLock(sm->sem);
          sm->objects++;
          clOsalMutexUnlock(sm->sem);

#ifdef DEBUG
          (*instance)->name[0] = 0;
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
 *  Delete State machine Instance.
 *  
 *  API to delete a previously created State macine Instance.
 *                                                                        
 *  @param this State machine Instance to be deleted
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clSmInstanceCreate
 *
 */
ClRcT 
clSmInstanceDelete(ClSmInstancePtrT this
                 )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Delete State Machine Instance"));

  if(this) 
    {
      /* decrement the references in 'sm' type 
       * NOTE: There is a potential problem here, even if lock acquired
       * reason is type is not accounted with lock and there can be
       * access to type else where in another thread.
       */
      if(this->sm)
      {
          clOsalMutexLock(this->sm->sem);
          this->sm->objects--;
          clOsalMutexUnlock(this->sm->sem);
      }
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
 *  Event Handling Function.
 *
 *  Handles the event at current state. Verifies the state transition
 *  table for possible transition; if present, then first calls the
 *  current state exit function handler, then the transition handler 
 *  and then the next state entry function handler.
 *
 *  If the state machine type is Hierarchical (having composite
 *  states); if the message/event is not handled at current state,
 *  then its passed to the parent and passed till 'top'. If its not
 *  handled, till top, then this method returns ERROR. else, the
 *  transition is handled. Transition, involves all exit method from
 *  bottom till LCA (Least Common Ancestor) and execution of
 *  transition object function, and entry methods from LCA till the
 *  next state.
 *                                                                        
 *  @param this Instance Object
 *  @param msg  Event that needs to be handled
 *
 *  @returns 
 *    CL_OK on CL_OK (successful transition) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance/msg handle <br/>
 *    SM_ERR_EXIT_FAILED if the exit handler returned failure
 *    CL_SM_RC(CL_ERR_INVALID_STATE) unable to handle event at current state
 *
 */
ClRcT 
clSmInstanceOnEvent(ClSmInstancePtrT this, 
                  ClSmEventPtrT msg
                  )
{
  ClRcT ret = CL_OK;
  
  CL_FUNC_ENTER();
  
  CL_ASSERT(this);
  CL_ASSERT(msg);  
  if(!this || !this->sm || !this->current || !msg) 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
    } else 
      if(this->sm->type == HIERARCHICAL_STATE_MACHINE)
        {
          ret = clHsmInstanceOnEvent(this, msg);
        }
      else
        {
          ret = fsmInstanceOnEvent(this, msg);
        }
  
  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Starts state machine instance.
 *
 *  API to start the state machine instance in init state. This API
 *  to be called after creating the state machine instance.  Sets
 *  the current state to init state from the state machine type and
 *  calls the entry function handler, if present.
 *                                                                        
 *  @param this State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 */
ClRcT
clSmInstanceStart(ClSmInstancePtrT this
                )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(this);

  if(!this || !this->sm || !this->current) 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
    } else 
      {
        ClSmStatePtrT curr = this->current;
        
#ifdef DEBUG
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Start [%s]. Set Init state [%d:%s]", 
                              this->name,
                              curr->type,
                              curr->name));
#endif

        /* call the entry function, if present
         */
        if(curr->entry) 
          {
            ret = (*curr->entry)(0, 0, 0);
          }
      }
  
  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Restarts a state machine.
 *
 *  API to Restart the state machine.  The API doesn't do a 
 *  graceful restart, just jumps to the init state and starts 
 *  the state machine.
 *                                                                        
 *  @param this State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 */
ClRcT
clSmInstanceRestart(ClSmInstancePtrT this
                  )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(this);

  if(!this || !this->sm || !this->current) 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
    } else 
      {
        this->current = this->sm->init;
        ret = clSmInstanceStart(this);
      }
  

  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Get current state.
 *
 *  API to return the current state handle.  
 *                                                                        
 *  @param this  State Machine Object
 *  @param state [OUT] current state handle
 *
 *  @returns 
 *    CL_OK on CL_OK (state handle points to current state) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on null instance handle <br/>
 *
 */
ClRcT
clSmInstanceCurrentStateGet(ClSmInstancePtrT this,
                          ClSmStatePtrT* state)
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(this);
  if(this && state)
  {
    *state = this->current;
  } else 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
     }

  CL_FUNC_EXIT();
  return ret;
}

/* ======== private functions go here  ========= */

/**@#-*/

#ifdef DEBUG

void
smInstanceNameSet(ClSmInstancePtrT this, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(this && name ) 
    {
      strncpy(this->name, name, MAX_STR_NAME);
    }
}

#endif


/**
 *  Event Handling Function.
 *
 *  Handles the event at current state. Verifies the state transition
 *  table for possible transition; if present, then first calls the
 *  current state exit function handler, then the transition handler 
 *  and then the next state entry function handler.
 *                                                                        
 *  @param this Instance Object
 *  @param msg  Event that needs to be handled
 *
 *  @returns 
 *    CL_OK on CL_OK (successful transition) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance/msg handle <br/>
 *    SM_ERR_EXIT_FAILED if the exit handler returned failure
 *    CL_SM_RC(CL_ERR_INVALID_STATE) unable to handle event at current state
 *
 */
ClRcT 
fsmInstanceOnEvent(ClSmInstancePtrT this, 
                   ClSmEventPtrT msg
                   )
{
  ClSmStatePtrT curr;
  ClSmStatePtrT next;
  ClSmTransitionPtrT tO=0;
  ClRcT ret = CL_OK;
  
  CL_FUNC_ENTER();
  
  CL_ASSERT(this);
  CL_ASSERT(msg);
  
  if(!this || !this->sm || !this->current || !msg) 
    {
      ret = CL_SM_RC(CL_ERR_NULL_POINTER);
    } else 
      {
        curr = this->current;
        
#ifdef DEBUG
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("StateMachine [%s] OnEvent [%d] in State [%d:%s]", 
                              this->name,
                              msg->eventId,
                              curr->type,
                              curr->name));
#else
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("OnEvent %d in state %d", 
                              msg->eventId,
                              curr->type));
#endif
        
        /* check if the event is in event handler table
         */
        if(msg->eventId < (ClSmEventIdT)curr->maxEventTransitions && msg->eventId >= 0)
          {
            tO = curr->eventTransitionTable[msg->eventId].transition;
          }
        
        if(tO) 
          {
            next = tO->nextState;
            
            /* run the state transition 
             *  1. call the exit function 
             *  2. call the TO handler 
             *  3. call the entry function
             *  - Entry and exit functions can be null
             *  - The transition may be self, in that case
             *     do not call exit and entry
             *  - if exit fails, then the transition is aborted and
             *    remains in the same state.
             */
            if(curr!=next && curr->exit) 
              {
                ret = (*curr->exit)(curr, &next, msg);
                if(ret != CL_OK) 
                  {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, (SM_ERR_STR(SM_ERR_EXIT_FAILED) 
                                          "Event %d in state %d Aborted!", 
                                          msg->eventId,
                                          curr->type));
                    ret = SM_ERR_EXIT_FAILED;
                  }
              }
            
            if(ret == CL_OK) 
              {
                this->current = next; 
                if(tO->transitionHandler) 
                  {
                    ClSmStatePtrT forced = next;
                    
                    ret=(*tO->transitionHandler)(curr, &forced, msg);
                    if(ret == SM_ERR_FORCE_STATE)
                      {
                        ret = CL_OK;
                        
                        CL_DEBUG_PRINT(CL_DEBUG_TRACE, (SM_ERR_STR(SM_ERR_FORCE_STATE) 
                                              "Event %d in state %d => [%d]!", 
                                              msg->eventId,
                                              curr->type,
                                              forced?forced->type:-1));
                        /*
                         * now look into the next state and force the
                         * state machine to that state.
                         * note: catch is, if its a self transition
                         * then need to call the exit, so that
                         * its consistent.
                         */
                         if(forced != next && next == curr && curr->exit)
                           {
                             /* call exit of curr */
                             ret = (*curr->exit) (curr, &forced, msg);
                           }
                         this->current = forced;
                      }
                  }
                if(curr!=next && this->current && this->current->entry) 
                  {
                    ret = (*(this->current)->entry)(curr, &this->current, msg);
                  }
              }
            
          } else 
            {
              ret = CL_SM_RC(CL_ERR_INVALID_STATE);
            }
      }
  
  CL_FUNC_EXIT();
  return ret;
}


void
smInstanceShow(ClSmInstancePtrT this
               )
{
  if(this)
    {
#ifdef DEBUG
      clOsalPrintf("\n[SM] Instance [%s] of type [%s]. Current State is [%s].\n", 
               this->name,
               this->sm?this->sm->name:"",
               this->current?this->current->name:"");
#else
      clOsalPrintf("\n[SM] Current State is [%d].\n", 
               this->current?this->current->type:-1);
#endif
    }

}

/**@#-*/
