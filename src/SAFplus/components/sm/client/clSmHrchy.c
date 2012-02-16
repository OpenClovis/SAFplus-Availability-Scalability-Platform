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
 * File        : clSmHrchy.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Hierarchical State Manager          
 *   implementation                                                      
 *************************************************************************/
/** @pkg cl.sm.hsm */

/* INCLUDES */
#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>

#include <clSmErrors.h>
#include <clSmTemplateApi.h>
#include <clSmBasicApi.h>
#include <clSmHrchyApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

static ClUint16T _findLCA(ClSmInstancePtrT this, ClSmStatePtrT curr, ClSmStatePtrT next);
static ClRcT  _transition(ClSmInstancePtrT this, ClSmTransitionPtrT tO, 
                              ClSmStatePtrT curr, ClSmStatePtrT next,
                              ClSmEventPtrT msg);

/**@#-*/

/**
 *  Event Handling Function.
 *
 *  Handles the event at current state. Handles message at the current
 *  state. If the message/event is not handled at current state, then
 *  its passed to the parent and passed till 'top'. If its not
 *  handled, till top, then this method returns ERROR. else, the
 *  transition is handled. Transition, involves all exit method from
 *  bottom till LCA and execution of transition object function, and
 *  entry methods from LCA till the next state.
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
clHsmInstanceOnEvent(ClSmInstancePtrT this, 
                   ClSmEventPtrT msg
                   )
{
    ClSmStatePtrT curr;
    ClSmTransitionPtrT tO=0;
    ClRcT ret = CL_OK;
    ClRcT retCode = CL_OK;
    
    CL_FUNC_ENTER();
    
    CL_ASSERT(this);
    CL_ASSERT(msg);
    
    if(!this || !this->sm || !this->current || !msg) 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return ret;
      }

    for(curr=this->current;curr && !tO;)
      {
        
        /* check if the event is in event handler table
         */
        if(msg->eventId < (ClSmEventIdT)curr->maxEventTransitions && msg->eventId >= 0)
          {
            tO = curr->eventTransitionTable[msg->eventId].transition;
            break;
          }
        curr=curr->parent;
      }

    if(curr && tO)
      {
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
        retCode = _transition(this, tO, this->current, tO->nextState, msg);
      } 
    else
      {
        ret = CL_SM_RC(CL_ERR_INVALID_STATE);
      }
    
    CL_FUNC_EXIT();
    return ret;
}

/** 
 * Find the LCA index.
 *
 * Finds the Least Common Ancestor between 'curr' state and 'next'
 * state.
 *
 *  @param this    state object handle
 *  @param curr    current state handle
 *  @param next    next state handle
 * 
 *  @returns 
 *    index  no of levels to the common ancestor.
 *
 */
static 
ClUint16T 
_findLCA(ClSmInstancePtrT this, ClSmStatePtrT curr, ClSmStatePtrT next)
{
    ClSmStatePtrT currParents[MAX_DEPTH];
    ClSmStatePtrT nextParents[MAX_DEPTH];
    int i,j;
    ClSmStatePtrT tmp;
    memset(nextParents, 0, sizeof(ClSmStatePtrT)*MAX_DEPTH);
    memset(currParents, 0, sizeof(ClSmStatePtrT)*MAX_DEPTH);
    
    /* some quick performance optimizations here 
     */
    if(curr == next || curr->parent == next->parent)
      {
        return 0;
      }
    
    /* walk till top of curr */
    tmp = curr;
    i=0;
    while(tmp) 
      {
        currParents[i++] = tmp;
        tmp = tmp->parent;
      }
    currParents[i] = tmp;

    /* walk till top of next */
    tmp = next;
    j=0;
    while(tmp) 
      {
        nextParents[j++] = tmp;
        tmp = tmp->parent;
      }
    nextParents[j] = tmp;

    /* now loop from top to find the LCA */
    while(i>-1 && j>-1 && currParents[i]==nextParents[j]) 
      {
        --i;
        --j;
      }
    return ((ClUint16T)i);
}

/** 
 * [Internal] Transition function
 *
 * Transition from current state 'curr' using Transition object (tO)
 * to the 'next' state.
 *
 *  @param this    state machine object handle
 *  @param tO      transition object handle
 *  @param curr    current state handle
 *  @param next    next state handle
 *  @param msg     event message
 * 
 *  @returns 
 *
 */
static
ClRcT  
_transition(ClSmInstancePtrT this, 
            ClSmTransitionPtrT tO, 
            ClSmStatePtrT curr, 
            ClSmStatePtrT next,
            ClSmEventPtrT msg)
{
    ClSmStatePtrT nextParents[MAX_DEPTH];
    ClSmStatePtrT tmp=curr;
    int lvls=-1;
    ClSmStatePtrT pptr = 0;
    ClRcT retCode = 0;
    
    memset(nextParents, 0, sizeof(ClSmStatePtrT)*MAX_DEPTH); 
    if(tO) 
      { 
        lvls =(int)_findLCA(this,curr,next);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("HSM Transition from %d to %d", 
                              curr->type, 
                              next->type)); 
        /* exit till LCA */
        for(;tmp && lvls>-1;lvls--) 
          {
            if(tmp->exit)
            { 
              retCode = (*tmp->exit)(curr, &next, msg);
            }
            tmp = tmp->parent;
          }
        pptr = tmp;
        this->current = next;

        /* run the transition */
        if(tO->transitionHandler) 
          {
            ClSmStatePtrT forced = next;
            retCode = (*tO->transitionHandler)(curr, &forced, msg); 
            /* do something for conditional transition here */
          }

        /* entry from LCA to start */
        lvls=0;
        tmp = next;
        while(tmp && tmp!=pptr) 
          {
            nextParents[lvls++] = tmp;
            tmp = tmp->parent;
          }
        this->current = tO->nextState; 
        /* now run thru the entry states */
        for(;lvls>-1;lvls--) 
          {
            tmp = nextParents[lvls];
            if(tmp && tmp->entry)
            { 
              retCode = (*tmp->entry)(curr, &this->current, msg);
            } 
          }

        /* till its composite state, then set to the init state */
        tmp = next->init;
        while(tmp)
          {
            /* fire the entry */
            if(tmp->entry)
              {
                retCode = (*tmp->entry) (curr, &this->current, msg);
              }
            this->current = tmp;
            tmp=tmp->init;
          }
      } 

  /* note: take care of retCode - if failed !! */
  return retCode;
}

/**@#-*/
