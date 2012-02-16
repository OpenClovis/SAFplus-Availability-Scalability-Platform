#ifndef _CL_TASK_POOL_H_
#define _CL_TASK_POOL_H_

#include <clEoQueue.h>
#include <clTimerApi.h>
#include <clMetricApi.h>
#include <clList.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Keep it slightly more than the uninterruptible task hang timeout in linux kernel (120 secs)
 */
#define CL_TASKPOOL_MONITOR_INTERVAL (150)

#define CL_TASKPOOL_RC(rc) CL_RC(CL_CID_TASKPOOL, rc)

#define CL_TASKPOOL_DEFAULT_MONITOR_INTERVAL (30) /* in seconds*/

#ifdef __linux__
#define DECL_TASK_ID(taskId)  ClOsalTaskIdT taskId;
#else
#define DECL_TASK_ID(taskId) 
#endif

typedef ClRcT (*ClTaskPoolMonitorCallbackT)(ClOsalTaskIdT tid, ClTimeT currentInterval, ClTimeT thresholdInterval);

typedef struct ClTaskPoolStats
{
    ClOsalTaskIdT tId;
    DECL_TASK_ID(taskId)
    ClTimeT       startTime;
    ClTimeT       iocSendTime;
    ClBoolT       heartbeatDisabled;
}ClTaskPoolStatsT;

typedef struct ClTaskPoolUsage
{
    ClInt32T numTasks;
    ClInt32T maxTasks;
    ClInt32T numIdleTasks;
}ClTaskPoolUsageT;

struct ClTaskPool
{
    ClMetric2T         numTasks;
    ClMetric2T         maxTasks;
    ClMetric2T         numIdleTasks;
    ClInt32T          priority;
    ClUint32T         flags;
    ClBoolT           userQueue;
    ClTimerHandleT    monitorTimer;
    ClTimerTimeOutT   monitorThreshold;
    ClTaskPoolMonitorCallbackT monitorCallback;
    ClBoolT monitorActive;
    ClTaskPoolStatsT   *pStats;
    ClOsalMutexT      mutex;
    ClOsalCondT       cond;
    ClCallbackT       preIdleFn;
    ClPtrT     preIdleCookie;
    ClCallbackT       onDeckFn;
    ClPtrT     onDeckCookie;
    ClInt32T pendingJobs;
};



typedef struct ClTaskPool ClTaskPoolT;
typedef struct ClTaskPool* ClTaskPoolHandleT;

ClRcT clTaskPoolInitialize(void);

ClRcT clTaskPoolFinalize(void);

/* preIdleFunc can call clTaskPoolRun to skip idling and add another job */
ClRcT clTaskPoolCreate(ClTaskPoolHandleT *pHandle,ClInt32T maxTasks, ClCallbackT preIdleFunc, ClPtrT preIdleCookie);

ClRcT clTaskPoolDelete(ClTaskPoolHandleT handle);

ClRcT clTaskPoolRun(ClTaskPoolHandleT handle, ClCallbackT func, ClPtrT cookie);

/* Ensures that there is at least 1 thread running in the pool, and wakes any idle threads.
   All idle threads will call the "pre-idle callback" if one has been registered before going
   back into idle.   
 */ 
ClRcT clTaskPoolWake(ClTaskPoolHandleT handle);

ClRcT clTaskPoolStopAsync(ClTaskPoolHandleT handle);
ClRcT clTaskPoolStop(ClTaskPoolHandleT handle);
ClRcT clTaskPoolStart(ClTaskPoolHandleT handle);
ClRcT clTaskPoolQuiesce(ClTaskPoolHandleT handle);

ClRcT clTaskPoolResume(ClTaskPoolHandleT handle);

ClRcT clTaskPoolFreeUnused(void);

ClRcT clTaskPoolQuiesce(ClTaskPoolHandleT handle);

ClRcT clTaskPoolMonitorStart(ClTaskPoolHandleT handle, ClTimerTimeOutT monitorThreshold, ClTaskPoolMonitorCallbackT monitorCallback);

ClRcT clTaskPoolMonitorStop(ClTaskPoolHandleT handle);

ClRcT clTaskPoolMonitorDelete(ClTaskPoolHandleT handle);

ClRcT clTaskPoolMonitorDisable(void);

ClRcT clTaskPoolMonitorEnable(void);

ClRcT clTaskPoolStatsGet(ClTaskPoolHandleT handle, ClTaskPoolUsageT *pStats);

ClRcT clTaskPoolRecordIOCSend(ClBoolT start);

#ifdef __cplusplus
}
#endif

#endif
