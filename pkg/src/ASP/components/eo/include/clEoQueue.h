/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#ifndef _CL_EO_QUEUE_H_
#define _CL_EO_QUEUE_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clBufferApi.h>
#include <clIocApi.h>
#include <clJobList.h>
#include <clTaskPool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_EO_QUEUE_LOCK(lock) do {             \
        ClRcT retCode = clOsalMutexLock(lock);  \
        CL_ASSERT(retCode == CL_OK);            \
}while(0)

#define CL_EO_QUEUE_UNLOCK(lock) do {               \
        ClRcT retCode = clOsalMutexUnlock(lock);    \
        CL_ASSERT(retCode == CL_OK);                \
}while(0)

#ifndef VXWORKS_BUILD
#define CL_EO_RECV_QUEUE_PRI(_rec_param)                                \
    ( ( (_rec_param).protoType == CL_IOC_RMD_ORDERED_PROTO ) ? CL_IOC_ORDERED_PRIORITY : \
      ( ( (_rec_param).protoType == CL_IOC_PORT_NOTIFICATION_PROTO ) ? CL_IOC_NOTIFICATION_PRIORITY : \
        ( ( (_rec_param).priority >= CL_IOC_MAX_PRIORITIES ) ? CL_IOC_DEFAULT_PRIORITY : (_rec_param).priority ) ) )
#else
#define CL_EO_RECV_QUEUE_PRI(_rec_param)                                \
    ( ( (_rec_param).protoType == CL_IOC_RMD_ORDERED_PROTO ) ? CL_IOC_ORDERED_PRIORITY : \
      ( ( (_rec_param).protoType == CL_IOC_PORT_NOTIFICATION_PROTO ) ? CL_IOC_HIGH_PRIORITY : \
        ( ( (_rec_param).priority == CL_IOC_HIGH_PRIORITY ) ? CL_IOC_HIGH_PRIORITY : CL_IOC_DEFAULT_PRIORITY ) ) )
#endif

#ifndef CL_EO_RC
#define CL_EO_RC(rc) CL_RC(CL_CID_EO,rc)
#endif

#define CL_EO_SCHED_POLICY (CL_OSAL_SCHED_OTHER)

typedef enum ClEoQueueState
{
    CL_EO_QUEUE_STATE_ACTIVE = 0x1,

    CL_EO_QUEUE_STATE_QUIESCED = 0x2,
}ClEoQueueStateT;

typedef enum ClThreadPoolMode
{
    CL_THREAD_POOL_MODE_NORMAL=1,
    CL_THREAD_POOL_MODE_EXCLUSIVE,
    CL_THREAD_POOL_MODE_MAX,
}ClThreadPoolModeT;

#define CL_EO_ALIGN(v,a) ( ((v) + (a)-1) & ~((a)-1) )

#define CL_EO_QUEUE_LEN(queue) ((queue)->numElements - (queue)->numThreadsWaiting)

#define CL_EO_QUEUE_POP(eoQueue,cast,data) do {     \
    data = NULL;                                    \
    if(!CL_JOB_LIST_EMPTY(&(eoQueue)->queue))       \
    {                                               \
        data = (cast*)clJobPop(&(eoQueue)->queue);  \
        CL_ASSERT(data != NULL);                    \
        --(eoQueue)->numElements;                   \
        CL_ASSERT((eoQueue)->numElements>=0);       \
    }                                               \
}while(0)

#define CL_EO_QUEUE_ADD_HEAD(eoQueue,element) do {          \
        ClRcT retCode = CL_OK;                              \
        retCode = clJobAdd((element),&(eoQueue)->queue);    \
        CL_ASSERT(retCode == CL_OK);                        \
        ++(eoQueue)->numElements;                           \
}while(0)

#define CL_EO_QUEUE_ADD(eoQueue,element) do {                   \
        ClRcT retCode = CL_OK;                                  \
        retCode = clJobAddTail((element),&(eoQueue)->queue);    \
        CL_ASSERT(retCode == CL_OK);                            \
        ++(eoQueue)->numElements;                               \
}while(0)

#define CL_EO_DELAY(delay) do {                 \
        ClTimerTimeOutT timeout = {0};          \
        if((delay)==0) (delay) = 1000;          \
        timeout.tsSec =(delay)/1000;            \
        timeout.tsMilliSec=(delay)%1000;        \
        clOsalTaskDelay(timeout);               \
}while(0)

typedef struct ClEoJob
{
    ClBufferHandleT msg;
    ClIocRecvParamT msgParam;
}ClEoJobT;

typedef ClRcT (*ClEoJobCallBackT)(ClEoJobT *pJob, ClPtrT pUserData);

struct ClThreadPool;

typedef struct ClEoQueue
{
    ClIocPriorityT priority;
    ClJobListT queue;
    struct ClThreadPool *pThreadPool;
    ClInt32T numElements;
    ClInt32T numThreadsWaiting;
    ClInt32T refCnt;
    ClEoQueueStateT state;
    ClOsalMutexT mutex;
    ClOsalCondT  cond;
}ClEoQueueT;

typedef ClPtrT ClEoQueueHandleT;


ClRcT clEoQueueInitialize(void);

ClRcT clEoQueueCreate(ClEoQueueHandleT *pHandle,
                      ClIocPriorityT priority,
                      ClThreadPoolModeT mode,
                      ClInt32T maxThreads,
                      ClInt32T threadPriority,
                      ClEoJobCallBackT pJobHandler,
                      ClPtrT pUserData
                      );

ClRcT clEoQueueQuiesce(ClIocPriorityT priority,
                       ClBoolT stopThreadPool);

void clEoQueuesQuiesce(void);

void clEoQueuesUnquiesce(void);

ClRcT clEoQueueDelete(ClIocPriorityT priority);

ClRcT clEoQueueDeleteSync(ClIocPriorityT priority, ClBoolT force);

ClRcT clEoQueueJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam);

ClRcT clEoQueueFinalize(void);

ClRcT clEoQueueFinalizeSync(ClBoolT force);

#ifdef __cplusplus
}
#endif

#endif
