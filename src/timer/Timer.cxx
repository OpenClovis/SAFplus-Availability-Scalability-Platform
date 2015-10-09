/*
 * A scalable timer implementation using red black trees
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "Timer.hxx"
using namespace SAFplus;

static bool gClTimerDebug = CL_FALSE;
static SAFplus::Mutex gClTimerDebugLock;
signed int timerMinParallelThread=5;
signed int timerMaxParallelThread=50;

#define TIMER_INITIALIZED_CHECK(rc,label) do {   \
    if(gTimerBase.initialized == CL_FALSE)          \
    {                                               \
      rc = CL_TIMER_RC(CL_ERR_NOT_INITIALIZED);   \
      goto label;                                 \
    }                                               \
}while(0)

#define TIMER_SET_EXPIRY(timer) do {                         \
    boost::posix_time::ptime currentTime = gTimerBase.now ;                      \
    (timer)->timerExpiry = currentTime + boost::posix_time::microsec((timer)->timerTimeOut); \
}while(0)

#define REPETITIVE_TIMER_SET_EXPIRY(timer) do {      \
    if (TimerTypeT::TIMER_REPETITIVE == (timer)->timerType)      \
    {                                                   \
      boost::posix_time::ptime currentTime = gTimerBase.now;           \
      boost::posix_time::ptime timerExpiry = (timer)->timerExpiry;     \
      if (timerExpiry > currentTime)                  \
      {                                               \
        timerExpiry = currentTime;                   \
      }                                               \
      (timer)->timerExpiry = timerExpiry              \
      + boost::posix_time::microsec((timer)->timerTimeOut); \
    }                                                   \
    else                                                \
    {                                                   \
      TIMER_SET_EXPIRY(timer);                        \
    }                                                   \
}while(0)

SAFplus::TimerBase gTimerBase;


SAFplus::TimerBase::TimerBase(): pool(timerMinParallelThread,timerMaxParallelThread)
{

}
ClRcT SAFplus::TimerBase::TimerBaseInitialize()
{
  ClRcT rc = CL_OK;
  this->frequency = TIMER_FREQUENCY;
  return rc;
}
Timer* SAFplus::TimerBase::get_rbtree_min()
{
  if(timerTree.empty())
  {
    return NULL;
  }
  boost::intrusive::rbtree<Timer>::iterator it = timerTree.begin();
  return &*it;
}
/*
 * Run the sorted timer list expiring timers.
 * Called with the timer list lock held.
 */

ClRcT SAFplus::TimerBase::timerRun(void)
{
  signed int recalcInterval = 15; /*recalculate time after expiring this much*/
  ClInt64T timers = 0;
  boost::posix_time::ptime now = this->now;
  /*
   * We take the minimum treenode at each iteration since we will drop the lock
   * while invoking the callback and it could so happen that the next
   * entry that we cached gets ripped off thereby forcing us to resort to
   * some dramatics while coding. So lets keep it clean and take the minimum
   * from the tree node.
   */
  while (1)
  {
    Timer *pTimer = NULL;
    pTimer = this->get_rbtree_min();
    if(!pTimer)
    {
      break;
    }
    TimerCallBackT timerCallback;
    ClPtrT timerData;
    TimerTypeT timerType;
    TimerContextT timerContext;
    signed short callbackTaskIndex = -1;
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
    if(timerType == TimerTypeT::TIMER_REPETITIVE && timerContext == TimerContextT::TIMER_TASK_CONTEXT)
    {
      REPETITIVE_TIMER_SET_EXPIRY(pTimer);
      this->timerTree.insert_unique(*pTimer);
    }

    /*
     * Mark it running to prevent a moron from killing us
     * behind our back.
     */
    pTimer->timerFlags |= TIMER_RUNNING;
    /*
     * reference count the timer fired
     */
    ++pTimer->timerRefCnt;

    if(timerType != TimerTypeT::TIMER_VOLATILE && timerContext == TimerContextT::TIMER_TASK_CONTEXT)
      callbackTaskIndex = pTimer->timerAddCallbackTask();

    /*
     * Drop the lock now.
     */
    this->timerListLock.unlock();

    /*
     * Now fire the sucker up.
     */
    CL_ASSERT(timerCallback != NULL);

    switch(timerContext)
    {
      case TimerContextT::TIMER_SEPARATE_CONTEXT:
      {
        this->timerSpawn(pTimer);
      }
      break;
      case TimerContextT::TIMER_TASK_CONTEXT:
      default:
        if(gClTimerDebug)
        {
          pTimer->endTime = boost::posix_time::microsec_clock::local_time();
          boost::posix_time::time_duration diff = pTimer->endTime - pTimer->startTime;
          signed long long duration = diff.total_microseconds();
          logInfo("TIMER7","RUN", "Timer invoked at [%lld] - [%lld.%lld] usecs. Expiry [%lld] usecs",
              duration, (duration/1000000L),
              (duration%1000000L), pTimer->timerTimeOut);

          pTimer->startTime = boost::posix_time::not_a_date_time;
          pTimer->startRepTime = boost::posix_time::not_a_date_time;
          pTimer->endTime = boost::posix_time::not_a_date_time;
        }
        timerCallback(timerData);
        if (gClTimerDebug)
        {
          if (TimerTypeT::TIMER_REPETITIVE == timerType)
          {
            pTimer->startTime = boost::posix_time::microsec_clock::local_time();
          }
        }
        break;
    }

    switch(timerType)
    {
      case TimerTypeT::TIMER_VOLATILE:
        /*
         *Rip this bastard off if not separate context.
         *for separate contexts,thread spawn would do that.
         */
        if(timerContext == TimerContextT::TIMER_TASK_CONTEXT)
        {
          delete pTimer;
        }
        break;
      case TimerTypeT::TIMER_REPETITIVE:
      default:
        break;
    }
    this->timerListLock.lock();

    /*
     * In order to avoid a leak when a delete request for a ONE shot/same context
     * comes when we dropped the lock, we double check for a pending delete
     */
    if(timerType != TimerTypeT::TIMER_VOLATILE && timerContext == TimerContextT::TIMER_TASK_CONTEXT)
    {
      if(callbackTaskIndex >= 0)
        pTimer->timerDelCallbackTask(callbackTaskIndex);
      --pTimer->timerRefCnt;
      pTimer->timerFlags &= ~TIMER_RUNNING;
    }
  }
  return CL_OK;
}

ClRcT timerCallbackTask(ClPtrT invocation)
{
  SAFplus::Timer *pTimer = (SAFplus::Timer*)invocation;
  TimerTypeT type = pTimer->timerType;
  bool canFree = CL_FALSE;
  signed short callbackTaskIndex = -1;
  gTimerBase.timerListLock.lock();

  if(gClTimerDebug)
  {
    gClTimerDebugLock.lock();
    pTimer->endTime = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = pTimer->endTime - pTimer->startTime;
    signed long long duration = diff.total_microseconds();
    logInfo("TIMER7", "CALL", "Timer task invoked at [%lld] - [%lld.%lld] usecs. "
        "Expiry [%lld] usecs with flags [%#x]", duration, (duration/1000000L),
        (duration% 1000000L), pTimer->timerTimeOut, pTimer->timerFlags);
    pTimer->startTime = boost::posix_time::not_a_date_time;
    pTimer->startRepTime = boost::posix_time::not_a_date_time;
    pTimer->endTime = boost::posix_time::not_a_date_time;
    gClTimerDebugLock.unlock();
  }

  /*
   * Check if we had a delete or stop while we were getting conceived.
   */
  if( (pTimer->timerFlags & TIMER_DELETED ) )
  {
    if(--pTimer->timerRefCnt <= 0)
    {
      canFree = CL_TRUE;
      pTimer->timerFlags &= ~TIMER_RUNNING;
    }
    gTimerBase.timerListLock.unlock();
    return CL_OK;
  }
  else if( (pTimer->timerFlags & TIMER_STOPPED) )
  {
    --pTimer->timerRefCnt;
    gTimerBase.timerListLock.unlock();
    return CL_OK;
  }

  ++gTimerBase.runningTimers;
  callbackTaskIndex = pTimer->timerAddCallbackTask();

  /*
   * If its a repetitive timer, add back into the list just before callback invocation.
   */
  if(type == TimerTypeT::TIMER_REPETITIVE)
  {
    gTimerBase.timeUpdate();
    REPETITIVE_TIMER_SET_EXPIRY(pTimer);
    gTimerBase.timerTree.insert_unique(*pTimer);
  }
  gTimerBase.timerListLock.unlock();
  pTimer->timerCallback(pTimer->timerData);
  if (gClTimerDebug)
  {
    if (TimerTypeT::TIMER_REPETITIVE == type)
    {
      gClTimerDebugLock.lock();
      pTimer->startTime = boost::posix_time::microsec_clock::local_time();
      gClTimerDebugLock.unlock();
    }
  }
  canFree = CL_FALSE;
  gTimerBase.timerListLock.lock();

  if(callbackTaskIndex >= 0)
    pTimer->timerDelCallbackTask(callbackTaskIndex);

  --pTimer->timerRefCnt;

  /*
   * Recheck for a pending delete request.
   */
  if( (pTimer->timerFlags & TIMER_DELETED ) )
  {
    if(pTimer->timerRefCnt <= 0)
      canFree = CL_TRUE;
  }
  else if(type == TimerTypeT::TIMER_VOLATILE)
  {
    canFree = CL_TRUE;
  }
  --gTimerBase.runningTimers;
  if(pTimer->timerRefCnt <= 0)
    pTimer->timerFlags &= ~TIMER_RUNNING;

  gTimerBase.timerListLock.unlock();
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
  this->now = boost::posix_time::microsec_clock::local_time();
}


/*
 * Here comes the mother routine.
 * The actual timer.
 */

void *timerTask(void *pArg)
{
  gTimerBase.timerListLock.lock();
  gTimerBase.timeUpdate();
  while(gTimerBase.timerRunning == CL_TRUE)
  {
    gTimerBase.timerListLock.unlock();
    boost::this_thread::sleep(boost::posix_time::millisec(gTimerBase.frequency));
    gTimerBase.timerListLock.lock();
    gTimerBase.timeUpdate();
    gTimerBase.timerRun();
  }

  /*Unreached actually*/
  gTimerBase.timerListLock.unlock();
  return NULL;
}

void SAFplus::Timer::timerDelCallbackTask(signed short freeIndex)
{
  if(freeIndex >= 0)
  {
    this->callbackTaskIds[freeIndex] = 0;
    this->freeCallbackTaskIndexPool[freeIndex] = this->freeCallbackTaskIndex;
    this->freeCallbackTaskIndex = freeIndex;
  }
}

signed short SAFplus::Timer::timerAddCallbackTask()
{
  signed short nextFreeIndex = 0;
  signed short curFreeIndex = 0;
  if( (curFreeIndex = this->freeCallbackTaskIndex) < 0 )
  {
    logInfo("CALLBACK", "ADD", "Unable to store task id for timer [%p] as current [%d] parallel instances "
        "of the same running timer exceeds [%d] supported. Timer delete on this timer might deadlock "
        "if issued from the timer callback context", (ClPtrT)this, this->timerRefCnt,
        TIMER_MAX_PARALLEL_TASKS);
    return -1;
  }
  nextFreeIndex = this->freeCallbackTaskIndexPool[curFreeIndex];
  this->callbackTaskIds[curFreeIndex] = pthread_self();
  this->freeCallbackTaskIndex = nextFreeIndex;
  return curFreeIndex;
}
SAFplus::Timer::~Timer()
{
  delete timerPool;
}
void SAFplus::Timer::timerInitCallbackTask()
{
  signed int i;
  for(i = 0; i < TIMER_MAX_PARALLEL_TASKS-1; ++i)
    this->freeCallbackTaskIndexPool[i] = i+1;
  this->freeCallbackTaskIndexPool[i] = -1;
  this->freeCallbackTaskIndex = 0;
}

ClRcT SAFplus::Timer::timerCreate(TimerTimeOutT timeOut,
    TimerTypeT timerType,
    TimerContextT timerContext,
    TimerCallBackT timerCallback,
    void *timerData)
{
  signed long long expiry = 0;
  ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

  TIMER_INITIALIZED_CHECK(rc,out);
  if(!timerCallback)
  {
    rc = CL_TIMER_RC(CL_TIMER_ERR_NULL_TIMER_CALLBACK);
    logError("TIMER7","CREATE", "Timer create failed: Null callback function passed");
    goto out;
  }

  if(timerType >= TimerTypeT::TIMER_MAX_TYPE)
  {
    rc = CL_TIMER_RC(CL_TIMER_ERR_INVALID_TIMER_TYPE);
    logError("TIMER7","CREATE", "Timer create failed: Bad timer type");

    goto out;
  }
  this->timerPool = new SAFplus::TimerPoolable(timerCallbackTask, (void*)this);
  if(timerContext >= TimerContextT::TIMER_MAX_CONTEXT)
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
  this->timerFlags=0;
  rc = CL_OK;
  out:
  return rc;
}

SAFplus::Timer::Timer(TimerTimeOutT timeOut,
    TimerTypeT timerType,
    TimerContextT timerContext,
    TimerCallBackT timerCallback,
    void *timerData)
{
  timerCreate(timeOut,timerType,timerContext,timerCallback,timerData);
}
ClRcT SAFplus::Timer::timerStartInternal(boost::posix_time::ptime expiry,bool locked)
{
  ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);

  TIMER_INITIALIZED_CHECK(rc,out);
  if(!locked)
  {
    gTimerBase.timerListLock.lock();
  }

  this->timerFlags &= ~TIMER_STOPPED;

  if(expiry!=boost::posix_time::not_a_date_time)
  {
    this->timerExpiry = expiry;
  }
  else
  {
    gTimerBase.timeUpdate();
    TIMER_SET_EXPIRY(this);
  }


  /*
   * add to the rb tree.
   */
  //TODO logDebug("TIMER", "START", "INSERT TIMER INTO RBTREE with time expire [%ld]",this->timerExpiry);
  if(this->timerCallback==NULL)
  {
    logDebug("TIMER", "START", "callback null in start function");
  }
  gTimerBase.timerTree.insert_unique(*this);
  if(gClTimerDebug)
    this->startTime = boost::posix_time::microsec_clock::local_time();


  if(!locked)
  {
    gTimerBase.timerListLock.unlock();
  }

  rc = CL_OK;

  out:
  return rc;
}

ClRcT SAFplus::Timer::timerStop()
{
  ClRcT rc = CL_TIMER_RC(CL_ERR_INVALID_PARAMETER);
  TIMER_INITIALIZED_CHECK(rc,out);
  rc = CL_OK;
  gTimerBase.timerListLock.lock();
  /*Reset timer expiry. and rip this guy off from the list.*/
  this->timerExpiry = boost::posix_time::not_a_date_time;
  this->timerFlags |= TIMER_STOPPED;
  if(gTimerBase.timerTree.find(*this)!=gTimerBase.timerTree.end())
  {
    gTimerBase.timerTree.erase(boost::intrusive::rbtree<Timer>::s_iterator_to(*this));
  }
  gTimerBase.timerListLock.unlock();
  out:
  return rc;
}

ClRcT SAFplus::Timer::timerStart()
{
  return this->timerStartInternal(boost::posix_time::not_a_date_time,CL_FALSE);
}
ClRcT SAFplus::Timer::timerUpdate(TimerTimeOutT newTimeOut)
{
  ClRcT rc = CL_OK;

  TIMER_INITIALIZED_CHECK(rc, out);

  /*
   * First stop the timer.
   */
  rc = this->timerStop();
  if(rc != CL_OK)
  {
    logError("TIMER7","UPDATE","Timer stop failed");
    goto out;
  }
  gTimerBase.timerListLock.lock();

  this->timerFlags &= TIMER_STOPPED;
  this->timerTimeOut = (ClTimeT)((ClTimeT)newTimeOut.tsSec * 1000000 + newTimeOut.tsMilliSec * 1000);
  gTimerBase.timeUpdate();
  TIMER_SET_EXPIRY(this);
  gTimerBase.timerTree.insert_unique(*this);



  if(gClTimerDebug)
    this->startTime = boost::posix_time::microsec_clock::local_time();

  gTimerBase.timerListLock.unlock();

  out:
  return rc;
}

ClRcT SAFplus::Timer::timerRestart (TimerHandleT  timerHandle)
{
  ClRcT rc = 0;
  rc = this->timerStop();
  if (rc != CL_OK)
  {
    return (rc);
  }

  rc = this->timerStart();

  if (rc != CL_OK)
  {
    return (rc);
  }
  return (CL_OK);
}

ClRcT SAFplus::Timer::timerState(bool flags, bool *pState)
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
bool SAFplus::Timer::timerIsStopped()
{
  if((this->timerFlags & TIMER_STOPPED))
    return true;
  return false;
}

/*
 * Assumed that the caller has also synchronized his call with his timer start/stop/delete.
 */
bool SAFplus::Timer::timerIsRunning()
{
  if((this->timerFlags & TIMER_RUNNING))
    return true;
  return false;
}

ClRcT SAFplus::Timer::timerCreateAndStart(TimerTimeOutT timeOut,
    TimerTypeT timerType,
    TimerContextT timerContext,
    TimerCallBackT timerCallback,
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

ClRcT SAFplus::timerInitialize(ClPtrT config, signed int maxTimer)
{
  // timerMinParallelThread=maxTimer;
  logDebug("TIMER", "START", "Init timer with [%d] thread pools",maxTimer);
  ClRcT rc = CL_TIMER_RC(CL_ERR_INITIALIZED);
  if(gTimerBase.initialized == CL_TRUE)
  {
    goto out;
  }

  if(clParseEnvBoolean("CL_TIMER_DEBUG") == CL_TRUE)
  {
    gClTimerDebug = CL_TRUE;
    /* crash if used outside debug context*/
  }

  rc = gTimerBase.TimerBaseInitialize();
  if(rc != CL_OK)
  {
    goto out;
  }

  gTimerBase.timerRunning = CL_TRUE;
  gTimerBase.timeUpdate();
  logInfo("TIMER7", "INIT", "create thread to handle timer");
  pthread_create(&gTimerBase.timerId, NULL,timerTask, NULL);
  if(rc != CL_OK)
  {
    gTimerBase.timerRunning = CL_FALSE;
    logError("TIMER7", "INIT", "Timer task create returned [%#x]", rc);
    goto out_free;
  }
  logInfo("TIMER7", "INIT", "create thread to handle timer success");
  gTimerBase.initialized = CL_TRUE;
  return rc;
  out_free:
  if(gTimerBase.timerId)
  {
    gTimerBase.timerRunning = CL_FALSE;
    pthread_join(gTimerBase.timerId, NULL);

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
    gTimerBase.timerListLock.lock();
    gTimerBase.timerRunning = CL_FALSE;
    gTimerBase.timerListLock.unlock();

    if(gTimerBase.timerId)
    {
      pthread_join(gTimerBase.timerId,NULL);
    }
  }
  gTimerBase.pool.stop();
  rc = CL_OK;
  out:
  return rc;
}
