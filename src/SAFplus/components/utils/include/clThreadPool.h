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
#ifndef _CL_THREAD_POOL_H_
#define _CL_THREAD_POOL_H_

#include <clEoQueue.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if 0
#define CL_THREADPOOL_RC(rc) CL_RC(CL_CID_THREADPOOL, rc)

struct ClThreadPool
{
    ClInt32T numThreads;
    ClInt32T maxThreads;
    ClThreadPoolModeT mode;
    ClInt32T priority;
    ClBoolT running;
    ClBoolT userQueue;
    ClEoQueueT *pQueue;
    ClEoJobCallBackT pJobHandler;
    ClPtrT pUserData;
};

typedef struct ClThreadPool ClThreadPoolT;
typedef ClPtrT ClThreadPoolHandleT;

ClRcT clThreadPoolInitialize(void);

ClRcT clThreadPoolCreate(ClThreadPoolHandleT *pHandle,
                         ClEoQueueHandleT queueHandle,
                         ClThreadPoolModeT mode,
                         ClInt32T maxThreads,
                         ClInt32T threadPriority,
                         ClEoJobCallBackT pJobHandler,
                         ClPtrT pUserData
                         );

ClRcT clThreadPoolDelete(ClThreadPoolHandleT handle);                         

ClRcT clThreadPoolDeleteSync(ClThreadPoolHandleT handle, ClBoolT force);                         

ClRcT clThreadPoolPush(ClThreadPoolHandleT handle, ClEoJobT *pJob);

ClRcT clThreadPoolStop(ClThreadPoolHandleT handle);

ClRcT clThreadPoolResume(ClThreadPoolHandleT handle);

ClRcT clThreadPoolFreeUnused(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
