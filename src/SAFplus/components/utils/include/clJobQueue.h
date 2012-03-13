#ifndef _CL_JOB_QUEUE_API_H_
#define _CL_JOB_QUEUE_API_H_

#include "clOsalApi.h"
#include "clQueueApi.h"
#include "clTaskPool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  ClOsalMutexT      mutex;
  ClQueueT          queue;
  ClTaskPoolT*      pool; 
  ClUint32T         flags;
} ClJobQueueT;

typedef struct ClJobQueueUsage
{
    ClUint32T numMsgs;
    ClTaskPoolUsageT taskPoolUsage;
}ClJobQueueUsageT;

typedef ClRcT (*ClJobQueueWalkCallbackT)(ClCallbackT cb, ClPtrT data, ClPtrT arg);

extern ClRcT clJobQueueInit(ClJobQueueT* handle, ClUint32T maxJobs, ClUint32T maxTasks);

extern ClRcT clJobQueueCreate(ClJobQueueT** handle, ClUint32T maxJobs, ClUint32T maxTasks);

/* An API that lets you "hook" a queue to a task pool:
ClRcT clJobQueueCreate2(ClJobQueueT** handle, ClQueueT* queue, ClTaskPool* pool);
*/

extern ClRcT clJobQueueDelete(ClJobQueueT* handle);

extern ClRcT clJobQueuePush(ClJobQueueT* handle, ClCallbackT job, ClPtrT data);

extern ClRcT clJobQueuePushIfEmpty(ClJobQueueT* handle, ClCallbackT job, ClPtrT data);

extern ClRcT clJobQueueStop(ClJobQueueT* handle);
extern ClRcT clJobQueueQuiesce(ClJobQueueT* handle);

  /* Not yet implemented */
extern ClRcT clJobQueueResume(ClJobQueueT* handle);

extern ClRcT clJobQueueMonitorStart(ClJobQueueT *handle, ClTimerTimeOutT monitorThreshold, ClTaskPoolMonitorCallbackT monitorCallback);

extern ClRcT clJobQueueMonitorStop(ClJobQueueT *handle);

extern ClRcT clJobQueueMonitorDelete(ClJobQueueT *handle);

extern ClRcT clJobQueueStatsGet(ClJobQueueT *handle, ClJobQueueWalkCallbackT cb, ClPtrT arg, 
                                ClJobQueueUsageT *pJobQueueUsage);

extern ClRcT clJobQueueWalk(ClJobQueueT *handle, ClJobQueueWalkCallbackT cb, ClPtrT arg);

#ifdef __cplusplus
}
#endif

#endif
