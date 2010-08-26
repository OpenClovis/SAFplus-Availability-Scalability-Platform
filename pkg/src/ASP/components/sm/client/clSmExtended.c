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
 * File        : clSmExtended.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains State Manager implementation       
 *************************************************************************/
/** @pkg cl.sm.esm */

/* INCLUDES */
#include <clCommon.h>
#include <clDebugApi.h>

#include <clSmErrors.h>
#include <clSmExtendedApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


static void
dummyCallBack(ClQueueDataT userData)
{
    /*Dont free any thing here */
    return;
}


/**
 *  Creates a new Extended State machine Instance.
 *
 *  This API creates a new Extended State macine Instance of given
 *  state machine type.  The extended state machine shall include
 *  all the regular state machine instance functionalities, plus
 *  additional event queue, history, and lock capabilities.
 *                                                                        
 *  @param sm       State machine type 
 *  @param instance [out] newly created extended state machine instance
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null sm / instance <br/>
 *
 *  @see #clEsmInstanceDelete
 */
ClRcT
clEsmInstanceCreate(ClSmTemplatePtrT sm, 
                  ClExSmInstancePtrT* instance
                  )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(instance);  
  CL_ASSERT(sm);  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Create Extended State Machine Instance"));

  if(sm && instance) 
    {
      /* allocate the instance space */
      *instance = (ClExSmInstancePtrT) mALLOC(sizeof(ClExSmInstanceT));
      if(*instance!=0) 
        {
          memset(*instance, 0, sizeof(ClExSmInstanceT));
          /* call sm create here */
          ret = clSmInstanceCreate(sm, &(*instance)->fsm);
          if(ret == CL_OK)
            {
              ret = clOsalMutexCreate(&(*instance)->lock);
              if (CL_OK != ret)
              {
                  clSmInstanceDelete((*instance)->fsm);
                  mFREE(*instance);
                  ret = SM_ERR_NO_SEMA;
              }
              else
              {
              /* create queue and init */
              ret = SMQ_CREATE((*instance)->q);
              if(ret == CL_OK)
                {
                  /* init log buffer */
                  ESM_LOG_INIT((*instance)->log, ESM_LOG_ENTRIES);
                }
              if(!(*instance)->log.buffer || ret != CL_OK)
                {
                  /* delete the instance */
                  ret = clSmInstanceDelete((*instance)->fsm);
                  /* delete the mutex */
                  clOsalMutexDelete((*instance)->lock);
                  /* check if q init succeeded */
                  if(ret == CL_OK)
                    {
                      /* delete the queue */
                      clQueueDelete(&((*instance)->q));
                    }
                  /* free the instance */
                  mFREE(*instance);
                  ret = CL_SM_RC(CL_ERR_NO_MEMORY);
                }
              }
            }
          
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
 *  Delete Extended State machine Instance.
 *  
 *  API to delete a previously created State macine Instance. Also
 *  frees up the events that are in the Q.
 *                                                                        
 *  @param this Extended State machine Instance to be deleted
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceCreate
 *
 */
ClRcT 
clEsmInstanceDelete(ClExSmInstancePtrT this
                 )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(this);  

  CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Delete Extended State Machine Instance"));

  if(this) 
    {
      ClUint32T sz = 0;

      if(ESM_LOCK(this)!=CL_OK)
        {
          ret = SM_ERR_LOCKED;
          CL_FUNC_EXIT();
          return ret;
        }

      /* free the fsm first */
      ret = clSmInstanceDelete(this->fsm);

      SMQ_SIZE(this->q, sz);
      /* Check if the queue is empty, if not, dequeue and delete them */
      if(sz > 0) 
        {
          ClSmQueueItemPtrT item;
          ClRcT rc;

          rc = SMQ_DEQUEUE(this->q, item);
          while(rc==CL_OK && item)
            {
              mFREE(item);
              rc = SMQ_DEQUEUE(this->q, item);
            }
          CL_DEBUG_PRINT(CL_DEBUG_INFO, ("***Delete: Events are present in Q! Dropped to floor!!! ***"));
        }

      /* delete the queue */
      clQueueDelete(&this->q);

      /* free the history buffer */
      mFREE(this->log.buffer);

      /* unlock it before, so we can delete the mutex */
      ESM_UNLOCK(this);

      /* delete the mutex */
      clOsalMutexDelete(this->lock);

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
 *  Add event to the event q.
 *
 *  API to add event to the state machine instance queue.  The
 *  event properties are copied (a new event is created and 
 *  the contents of the event passed a re copied to the new 
 *  event), but the payload is just referenced and not copied.
 *  
 *  @param this Extended State machine Instance handle
 *  @param msg  Event information
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NO_MEMORY) on memory allocation FAILURE <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceProcessEvent
 *  @see #clEsmInstanceProcessEvents
 *
 */
ClRcT
clEsmInstanceEventAdd(ClExSmInstancePtrT this, 
                    ClSmEventPtrT msg
                    )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();

  CL_ASSERT(this);
  CL_ASSERT(msg);

  if(this && msg)
    {
      ClSmQueueItemPtrT item;

      item = (ClSmQueueItemPtrT) mALLOC(sizeof(ClSmQueueItemT)); 
      if(!item)
        {
          ret = CL_SM_RC(CL_ERR_NO_MEMORY);
        }
      else 
        {
          if(ESM_LOCK(this)!=CL_OK)
            {
              ret = SM_ERR_LOCKED;
              mFREE(item);
              CL_FUNC_EXIT();
              return ret;
            }
          item->event = *msg;
          if (ESM_IS_PAUSED(this) && ESM_IS_DROP_ON_PAUSE(this))
          {
              ret = CL_OK;
              mFREE(item);
              ESM_UNLOCK(this);
              CL_FUNC_EXIT();
              return ret;
          }
          ret = SMQ_ENQUEUE(this->q, item);
          CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Event %d added => ret [%d]",
                                item->event.eventId,
                                ret));
          ESM_UNLOCK(this);
        }
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
 *  Removes the first event from the q and processes it.  Apart
 *  from the event handling thats done at the SMType (Simple
 *  State machine), this API also handles like if this state
 *  machine instance is locked / not.  Also, checks if its in 
 *  history state and if so, then returns back to the previous 
 *  state.  
 *                                                                        
 *  @param this Instance Object
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    SM_ERR_NO_EVENT if there are no events in the q <br/>
 *    SM_ERR_LOCKED if the instance is locked <br/>
 *    SM_ERR_PAUSED if the instance is paused <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceProcessEvents
 */
ClRcT 
clEsmInstanceProcessEvent(ClExSmInstancePtrT this)
{
  ClRcT ret = CL_OK;
  ClSmEventPtrT msg=0;
  ClSmLogInfoPtrT logBuf;

  CL_FUNC_ENTER();

  CL_ASSERT(this);
  if(this && this->fsm && this->fsm->sm && this->fsm->current)
    {
      ClUint32T sz;

      if(ESM_LOCK(this)!=CL_OK)
        {
          ret = SM_ERR_LOCKED;
          CL_FUNC_EXIT();
          return ret;
        }
      SMQ_SIZE(this->q, sz);
      if(sz == 0)
        {
          ret = SM_ERR_NO_EVENT;
        }
      else if(ESM_IS_PAUSED(this))
        {
          ret = SM_ERR_PAUSED;
        }
      else if (ESM_IS_BUSY(this))
        {
            ret = SM_ERR_BUSY;
        }
      else
        {
          ClSmQueueItemPtrT item;
          ClRcT rc;

          /* dequeue the message 
           */

          rc = SMQ_DEQUEUE(this->q, item);
      
          if(rc!=CL_OK || !item) 
            {
              ret = CL_SM_RC(CL_ERR_NULL_POINTER);
            } else 
              {
                ClSmStatePtrT history = this->fsm->current;
                ClSmTransitionPtrT trans = 0;
                
                ESM_SET_BUSY_STATE(this);
                ESM_UNLOCK(this);

                msg = &item->event;

                /* if its in history state, then
                 * need to take care of the next state
                 * (use previous state, if not configured)
                 */
                if(history->type == ESM_HISTORY_STATE)
                  {
                    if(history->maxEventTransitions >(ClUint16T) msg->eventId &&
                       msg->eventId >= 0 &&
                       history->eventTransitionTable[msg->eventId].transition)
                      {
                        trans = history->eventTransitionTable[msg->eventId].transition;
                        
                        if(!trans->nextState)
                          {
                            /* This is not such a good idea, what happens another
                             * instance uses the type, it will reject it as if though
                             * there is a predefined next state and will not go to
                             * history state
                             */
                            trans->nextState = this->previous;
                            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("History State Set as Next State!"));
                          }
                          else 
                            {
                              trans = 0;
                            }
                      }
                    else 
                      {
                        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Unknown Event in History State"));
                      }
                  }
                
#ifdef DEBUG
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("StateMachine [%s] OnEvent [%d]", 
                                      this->fsm->name,
                                      msg->eventId));
#else
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("OnEvent %d", msg->eventId));
#endif

                ret = clSmInstanceOnEvent(this->fsm, msg);
                if(ret == CL_OK) 
                  {
                    /* update the history state */
                    this->previous = history;
                    /* record log */
                    logBuf = ESM_LOG_BUF(this->log);
                    logBuf->eventId = msg->eventId;
                    logBuf->from = this->previous;
                    logBuf->to = this->fsm->current;
                    ESM_LOG_IDX_INCR(this->log);
                  }
                

                /* restore the original state machine, if history
                 * state is set
                 */
                if(trans)
                  {
                    trans->nextState = 0;
                  }
                /* free the dequeued item */
                mFREE(item);
                ESM_LOCK(this);
                ESM_SET_IDL_STATE(this);
              }
        }
        ESM_UNLOCK(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}


/**
 *  Process all pending events.
 *
 *  Handle all pending events in the q. All the events
 *  that are pending in the event q are processed. If
 *  there is an error in the processing, then it returns
 *  back with the error code, even if there are more
 *  events in the q.
 *                                                                        
 *  @param this Instance Object handle
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    SM_ERR_NO_EVENT if there are no events in the q <br/>
 *    SM_ERR_LOCKED if the instance is locked <br/>
 *    SM_ERR_PAUSED if the instance is paused <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceProcessEvent
 */
ClRcT 
clEsmInstanceProcessEvents(ClExSmInstancePtrT this)
{
  ClRcT ret = CL_OK;
  int k=0;

  CL_FUNC_ENTER();
  CL_ASSERT(this);

  if(this)
    {
      while(ret == CL_OK)
        {
          ret = clEsmInstanceProcessEvent(this);
          if(ret==CL_OK) k++;
        }
      
      /* if more than one event, then return back
       * status OK
       */
      if(k>0 && ret==SM_ERR_NO_EVENT)
        {
          ret = CL_OK;
        }

      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Processed %d events",k));

    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
          
}

/**
 *  Starts the extended state machine instance.
 *
 *  Please refer to the FSM start API.
 *
 *  @param this State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clSmInstanceStart
 */
ClRcT
clEsmInstanceStart(ClExSmInstancePtrT this
                )
{
    CL_ASSERT(this);
    if(this)
      {
        return clSmInstanceStart(this->fsm);
      }

    return CL_SM_RC(CL_ERR_NULL_POINTER);
}


/**
 *  ReStarts the extended state machine instance.
 *
 *  Please refer to the FSM Restart API.
 *
 *  @param this State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clSmInstanceStart
 */
ClRcT
clEsmInstanceRestart(ClExSmInstancePtrT this
                  )
{
    CL_ASSERT(this);
    if(this)
      {
        return clSmInstanceRestart(this->fsm);
      }
    
    return CL_SM_RC(CL_ERR_NULL_POINTER);
}

/**
 *  Pause the extended state machine instance.
 *
 *  Stops the state machine instance execution of events. Any 
 *  event addition is just queued and no processing is
 *  allowed if the state machine is paused.  To put the
 *  state machine instance back in regular mode, use API
 *  clEsmInstanceContinue.
 *
 *  @param this     State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceContinue
 */
ClRcT
clEsmInstancePause(ClExSmInstancePtrT this
                 )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(this);

  if(this && this->fsm)
    {
      if(ESM_IS_PAUSED(this))
      {
        ret = SM_ERR_PAUSED;
        CL_FUNC_EXIT();
        return ret;
      }
#ifdef DEBUG
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Pause [%s]", this->fsm->name));
#endif
      ESM_PAUSE(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }

  CL_FUNC_EXIT();
  return ret;
}

/**
 *  Continue the extended state machine instance.
 *
 *  This API to be called if the state machine is paused
 *  and events are just being queued and not being 
 *  processed.  This API puts the state machine instance
 *  back in regular processing mode.
 *
 *  @param this State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstancePause
 */
ClRcT
clEsmInstanceContinue(ClExSmInstancePtrT this
                    )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(this);

  if(this && this->fsm)
    {
      if(!(ESM_IS_PAUSED(this)))
      {
        ret = SM_ERR_NOT_PAUSED;
        CL_FUNC_EXIT();
        return ret;
      }
#ifdef DEBUG
      CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Continue [%s]", this->fsm->name));
#endif
      ESM_CONTINUE(this);
    } else 
      {
        ret = CL_SM_RC(CL_ERR_NULL_POINTER);
      }
  
  CL_FUNC_EXIT();
  return ret;

}

/**
 *  Show Instance Details.
 *
 *  API to print the instance details.
 *
 *  @param this State Machine Object
 *
 *  @returns 
 *    void
 *
 *  @see #clEsmInstanceLogShow
 */
void
clEsmInstanceShow(ClExSmInstancePtrT this
                )
{
  if(this)
    {
      ClUint32T sz;

      SMQ_SIZE(this->q, sz);
      clOsalPrintf("\n[ESM] Instance: Lock [%d] Pause [%d] Q-Items [%d]", 
               this->lock,
               this->pause,
               sz);
      smInstanceShow(this->fsm);
    }

}

/**
 *  Show Log Details.
 *
 *  API to print the history of the state machine instance..
 *
 *  @param this State Machine Object
 *
 *  @returns 
 *    void
 *
 *  @see #clEsmInstanceLogShow
 */
void
clEsmInstanceLogShow(ClExSmInstancePtrT this,
                   ClUint16T  items
                   )
{
  if(this)
    {
      int i;
      ClSmLogInfoPtrT logBuf;

#ifdef DEBUG
      clOsalPrintf("\n[ESM] Instance: %s Last [%d] entries:",
               this->fsm->name,
               items);
#else
      clOsalPrintf("\n[ESM] Instance: Last [%d] entries:",
               (int)items);
#endif
      clOsalPrintf("\n------------------------------------------");
      clOsalPrintf("\n Event   From-State       To-State");
      clOsalPrintf("\n------------------------------------------");
      /* show last 'items' log information */
      for(i=1;i<= (int)items && i<=ESM_LOG_ENTRIES;i++)
        {
          logBuf = ESM_LOG_BACK_GET(this->log, i);
          if(logBuf && logBuf->from && logBuf->to)
            {
              clOsalPrintf("\n  %02d  %02d/%-15s %02d/%s",
                       logBuf->eventId,
#ifdef DEBUG
                       logBuf->from->type,
                       logBuf->from->name,
                       logBuf->to->type,
                       logBuf->to->name);
#else
                       logBuf->from->type,
                       "",
                       logBuf->to->type,
                       "");
#endif
            }
        }
      clOsalPrintf("\n------------------------------------------\n");
    }

}

/* ======== private functions go here  ========= */

/**@#-*/

#ifdef DEBUG

void
esmInstanceNameSet(ClExSmInstancePtrT this, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(this) 
    {
      smInstanceNameSet(this->fsm, name);
    }
}

#endif


/**@#-*/
