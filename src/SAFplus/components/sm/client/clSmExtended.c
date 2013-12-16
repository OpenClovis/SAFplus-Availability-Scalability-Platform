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
#include <clLogUtilApi.h>

#include <clSmErrors.h>
#include <clSmExtendedApi.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

#define ESM_LOG_AREA		"ESM"
#define ESM_LOG_CTX_CREATE	"CRE"
#define ESM_LOG_CTX_DELETE	"DEL"
#define ESM_LOG_CTX_EVENT	"EVT"

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

  clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_CREATE,"Create Extended State Machine Instance");

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
 *  @param smThis Extended State machine Instance to be deleted
 *
 *  @returns 
 *    CL_OK on CL_OK <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceCreate
 *
 */
ClRcT 
clEsmInstanceDelete(ClExSmInstancePtrT smThis
                 )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();
  CL_ASSERT(smThis);  

  clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_DELETE,"Delete Extended State Machine Instance");

  if(smThis) 
    {
      ClUint32T sz = 0;

      if(ESM_LOCK(smThis)!=CL_OK)
        {
          ret = SM_ERR_LOCKED;
          CL_FUNC_EXIT();
          return ret;
        }

      /* free the fsm first */
      ret = clSmInstanceDelete(smThis->fsm);

      SMQ_SIZE(smThis->q, sz);
      /* Check if the queue is empty, if not, dequeue and delete them */
      if(sz > 0) 
        {
          ClSmQueueItemPtrT item;
          ClRcT rc;

          rc = SMQ_DEQUEUE(smThis->q, item);
          while(rc==CL_OK && item)
            {
              mFREE(item);
              rc = SMQ_DEQUEUE(smThis->q, item);
            }
          clLogInfo(ESM_LOG_AREA,ESM_LOG_CTX_DELETE,"***Delete: Events are present in Q! Dropped to floor!!! ***");
        }

      /* delete the queue */
      clQueueDelete(&smThis->q);

      /* free the history buffer */
      mFREE(smThis->log.buffer);

      /* unlock it before, so we can delete the mutex */
      ESM_UNLOCK(smThis);

      /* delete the mutex */
      clOsalMutexDelete(smThis->lock);

      /* free the object */
      mFREE(smThis);
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
 *  @param smThis Extended State machine Instance handle
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
clEsmInstanceEventAdd(ClExSmInstancePtrT smThis, 
                    ClSmEventPtrT msg
                    )
{
  ClRcT ret = CL_OK;

  CL_FUNC_ENTER();

  CL_ASSERT(smThis);
  CL_ASSERT(msg);

  if(smThis && msg)
    {
      ClSmQueueItemPtrT item;

      item = (ClSmQueueItemPtrT) mALLOC(sizeof(ClSmQueueItemT)); 
      if(!item)
        {
          ret = CL_SM_RC(CL_ERR_NO_MEMORY);
        }
      else 
        {
          if(ESM_LOCK(smThis)!=CL_OK)
            {
              ret = SM_ERR_LOCKED;
              mFREE(item);
              CL_FUNC_EXIT();
              return ret;
            }
          item->event = *msg;
          if (ESM_IS_PAUSED(smThis) && ESM_IS_DROP_ON_PAUSE(smThis))
          {
              ret = CL_OK;
              mFREE(item);
              ESM_UNLOCK(smThis);
              CL_FUNC_EXIT();
              return ret;
          }
          ret = SMQ_ENQUEUE(smThis->q, item);
          clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"Event %d added => ret [%d]",
                                item->event.eventId,
                                ret);
          ESM_UNLOCK(smThis);
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
 *  @param smThis Instance Object
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
clEsmInstanceProcessEvent(ClExSmInstancePtrT smThis)
{
  ClRcT ret = CL_OK;
  ClSmEventPtrT msg=0;
  ClSmLogInfoPtrT logBuf;

  CL_FUNC_ENTER();

  CL_ASSERT(smThis);
  if(smThis && smThis->fsm && smThis->fsm->sm && smThis->fsm->current)
    {
      ClUint32T sz;

      if(ESM_LOCK(smThis)!=CL_OK)
        {
          ret = SM_ERR_LOCKED;
          CL_FUNC_EXIT();
          return ret;
        }
      SMQ_SIZE(smThis->q, sz);
      if(sz == 0)
        {
          ret = SM_ERR_NO_EVENT;
        }
      else if(ESM_IS_PAUSED(smThis))
        {
          ret = SM_ERR_PAUSED;
        }
      else if (ESM_IS_BUSY(smThis))
        {
            ret = SM_ERR_BUSY;
        }
      else
        {
          ClSmQueueItemPtrT item;
          ClRcT rc;

          /* dequeue the message 
           */

          rc = SMQ_DEQUEUE(smThis->q, item);
      
          if(rc!=CL_OK || !item) 
            {
              ret = CL_SM_RC(CL_ERR_NULL_POINTER);
            } else 
              {
                ClSmStatePtrT history = smThis->fsm->current;
                ClSmTransitionPtrT trans = 0;
                
                ESM_SET_BUSY_STATE(smThis);
                ESM_UNLOCK(smThis);

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
                            trans->nextState = smThis->previous;
                            clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"History State Set as Next State!");
                          }
                          else 
                            {
                              trans = 0;
                            }
                      }
                    else 
                      {
                        clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"Unknown Event in History State");
                      }
                  }
                
#ifdef DEBUG
                clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"StateMachine [%s] OnEvent [%d]", 
                                      smThis->fsm->name,
                                      msg->eventId);
#else
                clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"OnEvent %d", msg->eventId);
#endif

                ret = clSmInstanceOnEvent(smThis->fsm, msg);
                if(ret == CL_OK) 
                  {
                    /* update the history state */
                    smThis->previous = history;
                    /* record log */
                    logBuf = ESM_LOG_BUF(smThis->log);
                    logBuf->eventId = msg->eventId;
                    logBuf->from = smThis->previous;
                    logBuf->to = smThis->fsm->current;
                    ESM_LOG_IDX_INCR(smThis->log);
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
                ESM_LOCK(smThis);
                ESM_SET_IDL_STATE(smThis);
              }
        }
        ESM_UNLOCK(smThis);
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
 *  @param smThis Instance Object handle
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
clEsmInstanceProcessEvents(ClExSmInstancePtrT smThis)
{
  ClRcT ret = CL_OK;
  int k=0;

  CL_FUNC_ENTER();
  CL_ASSERT(smThis);

  if(smThis)
    {
      while(ret == CL_OK)
        {
          ret = clEsmInstanceProcessEvent(smThis);
          if(ret==CL_OK) k++;
        }
      
      /* if more than one event, then return back
       * status OK
       */
      if(k>0 && ret==SM_ERR_NO_EVENT)
        {
          ret = CL_OK;
        }

      clLogTrace(ESM_LOG_AREA,ESM_LOG_CTX_EVENT,"Processed %d events",k);

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
 *  @param smThis State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clSmInstanceStart
 */
ClRcT
clEsmInstanceStart(ClExSmInstancePtrT smThis
                )
{
    CL_ASSERT(smThis);
    if(smThis)
      {
        return clSmInstanceStart(smThis->fsm);
      }

    return CL_SM_RC(CL_ERR_NULL_POINTER);
}


/**
 *  ReStarts the extended state machine instance.
 *
 *  Please refer to the FSM Restart API.
 *
 *  @param smThis State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clSmInstanceStart
 */
ClRcT
clEsmInstanceRestart(ClExSmInstancePtrT smThis
                  )
{
    CL_ASSERT(smThis);
    if(smThis)
      {
        return clSmInstanceRestart(smThis->fsm);
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
 *  @param smThis     State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstanceContinue
 */
ClRcT
clEsmInstancePause(ClExSmInstancePtrT smThis
                 )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(smThis);

  if(smThis && smThis->fsm)
    {
      if(ESM_IS_PAUSED(smThis))
      {
        ret = SM_ERR_PAUSED;
        CL_FUNC_EXIT();
        return ret;
      }
#ifdef DEBUG
      clLogTrace(ESM_LOG_AREA,CL_LOG_CONTEXT_UNSPECIFIED,"Pause [%s]", smThis->fsm->name);
#endif
      ESM_PAUSE(smThis);
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
 *  @param smThis State Machine Object
 *
 *  @returns 
 *    CL_OK on CL_OK (successful start) <br/>
 *    CL_SM_RC(CL_ERR_NULL_POINTER) on invalid/null instance handle <br/>
 *
 *  @see #clEsmInstancePause
 */
ClRcT
clEsmInstanceContinue(ClExSmInstancePtrT smThis
                    )
{
  ClRcT ret = CL_OK;
  CL_FUNC_ENTER();

  CL_ASSERT(smThis);

  if(smThis && smThis->fsm)
    {
      if(!(ESM_IS_PAUSED(smThis)))
      {
        ret = SM_ERR_NOT_PAUSED;
        CL_FUNC_EXIT();
        return ret;
      }
#ifdef DEBUG
      clLogTrace(ESM_LOG_AREA,CL_LOG_CONTEXT_UNSPECIFIED,"Continue [%s]", smThis->fsm->name);
#endif
      ESM_CONTINUE(smThis);
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
 *  @param smThis State Machine Object
 *
 *  @returns 
 *    void
 *
 *  @see #clEsmInstanceLogShow
 */
void
clEsmInstanceShow(ClExSmInstancePtrT smThis
                )
{
  if(smThis)
    {
      ClUint32T sz;

      SMQ_SIZE(smThis->q, sz);
      clOsalPrintf("\n[ESM] Instance: Lock [%d] Pause [%d] Q-Items [%d]", 
               smThis->lock,
               smThis->pause,
               sz);
      smInstanceShow(smThis->fsm);
    }

}

/**
 *  Show Log Details.
 *
 *  API to print the history of the state machine instance..
 *
 *  @param smThis State Machine Object
 *
 *  @returns 
 *    void
 *
 *  @see #clEsmInstanceLogShow
 */
void
clEsmInstanceLogShow(ClExSmInstancePtrT smThis,
                   ClUint16T  items
                   )
{
  if(smThis)
    {
      int i;
      ClSmLogInfoPtrT logBuf;

#ifdef DEBUG
      clOsalPrintf("\n[ESM] Instance: %s Last [%d] entries:",
               smThis->fsm->name,
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
          logBuf = ESM_LOG_BACK_GET(smThis->log, i);
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
esmInstanceNameSet(ClExSmInstancePtrT smThis, char* name)
{
  /* go ahead and set the name if its not null
   */
  if(smThis) 
    {
      smInstanceNameSet(smThis->fsm, name);
    }
}

#endif


/**@#-*/
