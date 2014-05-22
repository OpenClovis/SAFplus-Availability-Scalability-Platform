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
 * ModuleName  : osal
 * File        : posix.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/
/* INCLUDES */

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clOsalErrors.h>
#include "../osal.h"
#include <clCksmApi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clDbg.h>
#include <clBitApi.h>
#include <clLogApi.h>
#include "clOsalCommon.h"
#include "clCommonCos.h"

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <clEoApi.h>

#ifdef CL_OSAL_DEBUG
#include "clPosixDebug.h"
#endif

#define CL_NODE_REP_STACK_SIZE (512<<10U)

extern int nanosleep (__const struct timespec *__requested_time,
		      struct timespec *__remaining);
extern int kill(pid_t pid, int sig);


/* Global flag to check if COS is already initiliazed */
extern CosTaskControl_t gTaskControl;
extern cosCompCfgInit_t sCosConfig;
extern ClBoolT gIsNodeRepresentative;
    
static void* cosPosixTaskWrapper(void* pArgument);
    
/**************************************************************************/
/* Declaration of helper functions */

static void taskSetScheduling(ClOsalTaskIdT tid, const char *taskName, int policy, int priority );


/* Convert between the different Priority definitions */

static ClUint32T priorityClovis2Posix(ClUint32T priority,int sched)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
  int Pmax = sched_get_priority_max(sched);
  int Pmin = sched_get_priority_min(sched);
  int Prange = Pmax - Pmin;

  int Cmax = ( COS_MAX_PRIORITY ? COS_MAX_PRIORITY : COS_MIN_PRIORITY );
  int Cmin = COS_MIN_PRIORITY;
  int Crange = Cmax - Cmin;

  return (((priority - Cmin) * Prange)/Crange) + Pmin;
#else  
  return (1 + (priority/2));
#endif
}

static ClUint32T priorityPosix2Clovis(ClUint32T priority,int sched)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
  int Pmax = sched_get_priority_max(sched);
  int Pmin = sched_get_priority_min(sched);
  int Prange = Pmax - Pmin;

  int Cmax = COS_MAX_PRIORITY;
  int Cmin = COS_MIN_PRIORITY;
  int Crange = Cmax - Cmin;

  if (Prange) return (((priority - Pmin) * Crange) / Prange) + Cmin;
  else        return Cmin;

#else
    return (priority*2)-2;
#endif
}


/**************************************************************************/

ClRcT
cosPosixFileOpen(ClCharT *file, ClInt32T flags, ClHandleT *fd) 
{
    int rc = 0; 

    nullChkRet(file);

    rc = open((char *)file,(int)flags);
    if(rc < 0) 
    {    
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to open [%s] file. system errorcode [%d]", file, rc);
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }    

    *fd = (ClHandleT)rc;

    return CL_OK;
}

/**************************************************************************/

ClRcT
cosPosixFileClose(ClFdT *fd) 
{
    int rc = 0; 

    nullChkRet(fd);

    rc = close(*fd);
    if(rc < 0) 
    {    
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to close the file. system errorcode [%d]\n", rc);
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }    

    *fd = 0; 

    return CL_OK;
}



ClRcT
cosPosixMmap(ClPtrT start, ClUint32T length, ClInt32T prot, ClInt32T flags, ClHandleT fd, ClHandleT offset, ClPtrT *mmapped)
{
    void *actualMap = NULL;
    
    nullChkRet(mmapped);

    actualMap = mmap((void*)start, (size_t)length, (int)prot, (int)flags, (int)fd, (off_t)offset);
    if(actualMap == MAP_FAILED)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Memory mapping of the file failed with error [%s]", strerror(errno));
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }
        
    *mmapped = actualMap;

    return CL_OK;
}

/**************************************************************************/

ClRcT
cosPosixMunmap(ClPtrT start, ClUint32T length)
{
    int rc = 0;
    
    nullChkRet(start);

    rc = munmap((void *)start, (size_t)length);
    if(rc < 0)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to unmap the mapped memory. system errorcode [%d]", rc);
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }

    return CL_OK;
}

/**************************************************************************/

ClRcT
cosPosixMsync(ClPtrT start, ClUint32T length, ClInt32T flags)
{
    int rc = 0;

    nullChkRet(start);

    rc = msync((void*)start, (size_t)length,(int)flags);
    if(rc < 0)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to sync the file to the mapped memory. system errorcode [%d]", rc);
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }

    return CL_OK;
}

/**************************************************************************/

ClRcT
cosPosixShmOpen(const ClCharT *name, ClInt32T oflag, ClUint32T mode, ClFdT *fd)
{
    int rc = 0;
    ClRcT retCode = CL_OK;

    nullChkRet(name);

    rc = shm_open((const char *)name, (int)oflag, (mode_t)mode);
    if(rc < 0)
    {
        int err = errno;

        if (EEXIST == err)
        {
            clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Shared memory object [%s] already exists. system errorcode [%d]", name, err);
            retCode = CL_ERR_ALREADY_EXIST;
            goto err;
        }

        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to open shared memory object [%s]. system errorcode [%d]", name, errno);
        retCode = CL_OSAL_ERR_OS_ERROR;
        goto err;
    }
    
    *fd = rc;
err:
    return CL_OSAL_RC(retCode);
}

ClRcT
cosPosixShmUnlink(const ClCharT *name)
{
    int rc = 0;

    nullChkRet(name);

    rc = shm_unlink(name);
    if(rc < 0)
    {
        int err = errno;
        ClRcT rc2 = CL_ERR_UNSPECIFIED;
        if (EACCES == err)
            rc2 = CL_ERR_OP_NOT_PERMITTED;

        if (ENOENT == err)
            rc2 = CL_ERR_DOESNT_EXIST;
        else  /* A non-existent shared memory object is a "normal" error -- it just means that the app was programming defensively */
          {
          clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to remove shared memory object. system errorcode [%d]", err);
          }
        return CL_OSAL_RC(rc2);
    }
    
    return CL_OK;
}


/**************************************************************************/

ClRcT
cosPosixFtruncate(ClFdT fd, off_t length)
{
    int rc = 0;
    
    rc = ftruncate((int)fd, (off_t)length);
    if(rc < 0)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Failed to truncate the file. system errorcode [%d]", rc);
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }
    
    return CL_OK;
}

/**************************************************************************/

static ClRcT 
cosTaskCreate(const ClCharT* pTaskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, 
              ClUint32T stackSize, void* (*fpTaskFunction)(void*),
              void* pTaskFuncArgument,ClOsalTaskIdT* pTaskId)
{
    CosTaskBlock_t *pTaskInfo = NULL;
    pthread_t thread;
    pthread_attr_t	taskAttribute;
    ClUint32T retCode = 0;
    ClTimerTimeOutT delay = { 0,  500 };
    ClInt32T tries = 0;

    /* This posix implementation assumes that the Clovis schedulePolicy enum == 
       the POSIX ones.  I am OK with this because it makes sense to align our 
       layer with Linux POSIX.  But we need to make sure that this compatibility
       is true whenever the code is run.
    */
    CL_ASSERT(CL_OSAL_SCHED_OTHER == SCHED_OTHER);
    CL_ASSERT(CL_OSAL_SCHED_FIFO  == SCHED_FIFO);
    CL_ASSERT(CL_OSAL_SCHED_RR    == SCHED_RR);

    nullChkRet(fpTaskFunction);

    CL_FUNC_ENTER();
    memset(&taskAttribute, 0, sizeof(pthread_attr_t));
    if (stackSize < sCosConfig.cosTaskMinStackSize)
	{
        stackSize = sCosConfig.cosTaskMinStackSize;
	}

#ifndef VXWORKS_BUILD
    if(gIsNodeRepresentative && stackSize < CL_NODE_REP_STACK_SIZE)
    {
        stackSize = CL_NODE_REP_STACK_SIZE;
    }
#endif
        
    /* Check if the priority is with in the limits */
    if (schedulePolicy != CL_OSAL_SCHED_OTHER)
      {
        if ((priority < COS_MIN_PRIORITY) || (priority > COS_MAX_PRIORITY))
          {
            clLogInfo("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"Task creation FAILED");
            retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
            clDbgCodeError(retCode, ("Priority '%d' is invalid, allowed range is %d-%d inclusive", priority, COS_MIN_PRIORITY, COS_MAX_PRIORITY));
            CL_FUNC_EXIT();
            return(retCode);
          }
      }

    if((schedulePolicy == CL_OSAL_SCHED_OTHER)&&(priority != CL_OSAL_THREAD_PRI_NOT_APPLICABLE))
      {
        clDbgCodeError(CL_OSAL_RC(CL_ERR_INVALID_PARAMETER), ("Cannot set the task priority (you must pass CL_OSAL_THREAD_PRI_NOT_APPLICABLE) when the scheduling policy is 'CL_OSAL_SCHED_OTHER'"));
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_ERR_INVALID_PARAMETER));
      }

    priority = priorityClovis2Posix(priority,schedulePolicy);

    /* Initialize with default thread attributes */
    
    retCode = (ClUint32T)pthread_attr_init (&taskAttribute);

    if (0 != retCode)
	{
        clLogInfo("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"\nTask creation FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_ATTRIBUTE_INIT);
        CL_FUNC_EXIT();
        return(retCode);
	}
    retCode = (ClUint32T)pthread_attr_setstacksize (&taskAttribute,stackSize);

    if(0 != retCode)
	{
        clLogInfo("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"\nTask creation FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_STACK_SIZE);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if (pTaskId==NULL)
      {
        /* 
        ** Added following since pthread_detach is not recommended.
        */
        retCode = (ClUint32T) pthread_attr_setdetachstate(&taskAttribute, PTHREAD_CREATE_DETACHED);
    
	if(0 != retCode)
          {
            clLogInfo("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"\nSet detachstate FAILED");
            retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_ATTRIBUTE_SET);
            CL_FUNC_EXIT();
            return(retCode);
          }    
      }

    pTaskInfo = (CosTaskBlock_t*)clHeapAllocate((ClUint32T)sizeof(CosTaskBlock_t));

    if(NULL == pTaskInfo)
    {
        clLogInfo("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"\nTask creation FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }

    pTaskInfo->fpTaskFunction = fpTaskFunction;
    pTaskInfo->pArgument = pTaskFuncArgument;

    if((schedulePolicy != CL_OSAL_SCHED_OTHER) && (
       (getuid() != 0) || 
       (geteuid() != 0)
       )
       ) {
      clLogWarning("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"Need superuser privileges to set CL_OSAL_SCHED_RR/CL_OSAL_SCHED_FIFO; task scheduling will not work as directed.");
    }

    pTaskInfo->schedPolicy = schedulePolicy;
    pTaskInfo->schedPriority = (int)priority;

    if(NULL != pTaskName)
    {
        strncpy(pTaskInfo->taskName, pTaskName, sizeof(pTaskInfo->taskName));
    }
    else
    {
        strncpy(pTaskInfo->taskName, "UNNAMED", sizeof(pTaskInfo->taskName));
    }

    do
    {
        retCode = (ClUint32T)pthread_create (&thread, &taskAttribute, 
                                             cosPosixTaskWrapper, ((void*)pTaskInfo));
    } while( retCode 
             && 
             (retCode == EAGAIN || retCode == ENOMEM)
             &&
             ++tries < 3
             &&
             cosPosixTaskDelay(delay) == CL_OK );

    /* NOTE, thread deletes pTaskInfo, so do not use below this line */

    if (0 != retCode)
	{
        clLogError("OSAL",CL_LOG_CONTEXT_UNSPECIFIED,"Task creation failed with error [%d] - [%s]",retCode,
                                        strerror(retCode));
        clDbgPause();
        clHeapFree(pTaskInfo);
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_CREATE);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if (pTaskId) 
      {
        *pTaskId = (ClOsalTaskIdT)thread;
        /* Set the scheduling priority here and in the task so that it is set by the
           time we return from create.  Otherwise, a quick call to taskPriorityGet may 
           not return the correct value.
        */
        taskSetScheduling(thread, pTaskName ? pTaskName : "UNNAMED", schedulePolicy, priority);
      }

    CL_FUNC_EXIT();
    return (CL_OK);
}



/**************************************************************************/

ClRcT 
cosPosixTaskCreateDetached (const ClCharT* pTaskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, 
                    ClUint32T stackSize, void* (*fpTaskFunction)(void*),
                    void* pTaskFuncArgument)
{
  return cosTaskCreate(pTaskName,schedulePolicy, priority, stackSize, fpTaskFunction, pTaskFuncArgument, NULL);
}

/**************************************************************************/

ClRcT 
cosPosixTaskCreateAttached(const ClCharT* pTaskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, 
                    ClUint32T stackSize, void* (*fpTaskFunction)(void*),
                    void* pTaskFuncArgument,ClOsalTaskIdT* pTaskId)
{
  nullChkRet(pTaskId);
  return cosTaskCreate(pTaskName,schedulePolicy, priority, stackSize, fpTaskFunction, pTaskFuncArgument, pTaskId);
}

/**************************************************************************/

ClRcT 
cosPosixTaskDelete (ClOsalTaskIdT taskId)
{
    ClUint32T retCode = 0;

    ClUint32T isSameTask=1;

    CL_FUNC_ENTER(); 
    if(0 == taskId)
    {
        isSameTask = 0;
        taskId = (ClOsalTaskIdT) pthread_self ();    
    }

    if(0 == isSameTask)
      {
        /* Calling detach to free resources used up by the task*/ 

        retCode = (ClUint32T)pthread_detach (taskId);
        if(0 != retCode)
          {
            clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task delete FAILED, system error code [%d]", retCode);
            retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_DELETE);
            CL_FUNC_EXIT();
            return(retCode);
          }
	    
        pthread_exit ((void*)NULL);        
	    
      }
    else
      {
        /* not the currently executing task. */
	   
        retCode = (ClUint32T)pthread_cancel (taskId);
        if(0 != retCode)
          {
            clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task delete FAILED, system error code [%d]", retCode);
            retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_DELETE);
            CL_FUNC_EXIT();
            return(retCode);
          }

      }
   
    CL_FUNC_EXIT();
    return (CL_OK);
}


ClRcT 
cosPosixTaskKill(ClOsalTaskIdT taskId, ClInt32T sig)
{
    ClUint32T retCode = 0;

    CL_FUNC_ENTER(); 
    if(0 == taskId)
    {
        taskId = (ClOsalTaskIdT) pthread_self ();    
    }

    /* not the currently executing task. */
	   
    retCode = (ClUint32T)pthread_kill (taskId, sig);
    if(0 != retCode)
    {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task delete FAILED, system error code [%d]", retCode);
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_DELETE);
        CL_FUNC_EXIT();
        return(retCode);
    }
   
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixTaskJoin (ClOsalTaskIdT taskId)
{
    ClUint32T retCode = 0;
    ClRcT rc = CL_OK;
    (void) rc;  /* avoid a set but not used error */
    CL_FUNC_ENTER(); 
    /* You can't join to yourself! */
    if ((0 == taskId) || (taskId == (ClOsalTaskIdT) pthread_self ()))
    {
      rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
      clDbgCodeError(rc, ("Tasks cannot 'join' themselves! taskId is %d, I am %d.", (int) taskId, (int) pthread_self()));
    }
    else
      {
	   
        retCode = (ClUint32T)pthread_join (taskId, NULL);
        switch (retCode)
          {
          case 0: break;
          case EINVAL:
            rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
            clDbgCodeError(rc, ("Task %d is not joinable.", (int)taskId));
            break;
          case ESRCH:
            rc = CL_OSAL_RC(CL_ERR_NOT_EXIST);
            clDbgCodeError(rc, ("Task %d is not valid.", (int)taskId));
            break;
          case EDEADLK: 
            rc = CL_OSAL_RC(CL_ERR_BAD_OPERATION);
            clDbgCodeError(rc, ("A deadlock was detected; are both tasks (%d and %d) joining eachother?",(int) taskId,(int) pthread_self()));
            break;
          }
      }
   
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixTaskDetach (ClOsalTaskIdT taskId)
{
    ClUint32T retCode = 0;
    ClRcT rc = CL_OK;
    (void) rc;  /* avoid a set but not used error */

    CL_FUNC_ENTER(); 
    /* You can't join to yourself! */
    if ((0 == taskId) || (taskId == (ClOsalTaskIdT) pthread_self ()))
    {
      rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
      clDbgCodeError(rc, ("Tasks cannot 'join' themselves! taskId is %d, I am %d.", (int) taskId,  (int) pthread_self()));
    }
    else
      {
	   
        retCode = (ClUint32T)pthread_detach (taskId);
        switch (retCode)
          {
          case 0: break;
          case EINVAL:
            rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
            clDbgCodeError(rc, ("Task %d is already detached.", (int) taskId));
            break;
          case ESRCH:
            rc = CL_OSAL_RC(CL_ERR_NOT_EXIST);
            clDbgCodeError(rc, ("Task %d is not valid.", (int) taskId));
            break;
          }
      }
   
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**************************************************************************/

ClRcT 
cosPosixSelfTaskIdGet (ClOsalTaskIdT* pTaskId)
{
    nullChkRet(pTaskId);

    CL_FUNC_ENTER();
    *pTaskId = (ClOsalTaskIdT)pthread_self ();
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixTaskNameGet (ClOsalTaskIdT taskId, ClUint8T** ppTaskName)
{
    ClUint32T retCode = CL_OK;
   
    nullChkRet(ppTaskName);
    CL_FUNC_ENTER();

    if(0 == taskId)
    {
        taskId = (ClOsalTaskIdT)pthread_self();
    }

    if (taskId != (ClOsalTaskIdT)pthread_self())
      {
        clDbgCodeError(CL_OSAL_RC(CL_ERR_INVALID_PARAMETER), ("Invalid parameter, taskid = 0x%llx.  TaskNameGet is only supported for the currently running task (pass 0 to taskId)", taskId) );
        return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
      }

    retCode = clOsalTaskDataGet(gTaskControl.taskNameKey, (ClOsalTaskDataT*)ppTaskName);
    if(CL_OK != retCode)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task Name Get Failed, rc [0x%x]\n", retCode);
    }

    CL_FUNC_EXIT();
    return (retCode);
}

/**************************************************************************/
    
ClRcT 
cosPosixTaskPriorityGet (ClOsalTaskIdT taskId,ClUint32T* pTaskPriority)
{
    struct sched_param scheduleParam = {0};
    int policy = 0;
    ClUint32T retCode = 0;

    nullChkRet(pTaskPriority);
    CL_FUNC_ENTER();
   
    retCode =(ClUint32T) pthread_getschedparam (taskId,&policy,&scheduleParam);

    if(0 != retCode)
    {
        *pTaskPriority = 0; /* Clear out the priority for misbehaving application programs */

        if (retCode == ESRCH)
          {
            clDbgCodeError(CL_OSAL_RC(CL_ERR_INVALID_PARAMETER), ("Invalid parameter, task [%llu] does not exist when attempting to get the task priority", taskId) );
            return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
          }

        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Task [%llu] priority get failed, system error [%s] code [%d]",taskId,strerror(retCode),retCode);
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_OSAL_ERR_TASK_ATTRIBUTE_GET));
    }

    *pTaskPriority = priorityPosix2Clovis((ClUint32T)scheduleParam.sched_priority,policy);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixTaskPrioritySet (ClOsalTaskIdT taskId,ClUint32T taskPriority)
{
    int policy = 0; 
    struct sched_param  scheduleParam = {0};
    ClUint32T retCode = 0;

    CL_FUNC_ENTER();
    if ((taskPriority < COS_MIN_PRIORITY) || 
        (taskPriority > COS_MAX_PRIORITY))
	{
          retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
          clDbgCodeError(retCode, ("Task priority [%d] is out of range [%d] to [%d]", taskPriority, COS_MIN_PRIORITY,COS_MAX_PRIORITY));
          CL_FUNC_EXIT();
          return(retCode);
	}

    retCode = (ClUint32T) pthread_getschedparam (taskId,&policy,&scheduleParam);

    if(0 != retCode)
	{
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task priority set failed.  System error [%d]",retCode);
        retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    taskPriority = priorityClovis2Posix(taskPriority,policy);

    scheduleParam.sched_priority = (ClInt32T)taskPriority;

    if (SCHED_OTHER != policy) {
	    
    retCode = (ClUint32T)pthread_setschedparam (taskId,policy,&scheduleParam);

    if (0 != retCode)
	{
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task priority set failed.  System error [%d]",retCode);
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_ATTRIBUTE_SET);
        CL_FUNC_EXIT();
        return(retCode);
	}
    }
    
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixTaskDelay (ClTimerTimeOutT timer)
{
    ClUint32T       retCode    = 0;
    struct timespec timeOut    = {0};
    struct timespec remainTime = {0};
    CL_FUNC_ENTER();

    memset(&timeOut, 0, sizeof(struct timespec));
    
    timeOut.tv_sec = timer.tsSec + timer.tsMilliSec/1000;
    timeOut.tv_nsec = (timer.tsMilliSec%1000)*1000*1000;
    
    do
    {
        retCode = (ClUint32T) nanosleep (&timeOut, &remainTime);
        if(0 != retCode)
        {
            if( errno == EINTR )
            {
                memcpy(&timeOut, &remainTime, sizeof(timeOut));
                memset(&remainTime, '\0', sizeof(remainTime));
            }
            else
            {
                int err = errno;
                clLogMultiline(CL_LOG_SEV_ERROR, "OSAL", CL_LOG_CONTEXT_UNSPECIFIED, 
                           "Task delay of [%ld] secs and [%ld] nsecs failed\n"
                           "sytem error[%s] code [%d]", (unsigned long)timeOut.tv_sec,
                           timeOut.tv_nsec, strerror(err), err);
                retCode = CL_OSAL_RC(CL_OSAL_ERR_TASK_DELAY);
                CL_FUNC_EXIT();
                return(retCode);
            }
        }
    }while((retCode != 0) && (errno == EINTR));

    CL_FUNC_EXIT();
	return (CL_OK);
}

/**************************************************************************/

ClTimeT
cosPosixStopWatchTimeGet(void)
{
    struct timespec ts;
    int ret;

    memset(&ts, 0, sizeof(ts));

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if(ret != 0)
    {
        switch(ret)
        {
            case EFAULT :
                CL_ASSERT(0);
                break;
            case EINVAL :
                clDbgCodeError(CL_ERR_LIBRARY  ,("CLOCK_MONOTONIC is not supported on this system.  You are either using an incorrect Osal adaption layer (posix)" ));
                break;
            case EPERM :
                CL_ASSERT(0);
                break;
            default: 
                clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Error:Unknow/Undocumented error while calling clock_gettime(). system error code %d.",
                            errno );
                break;
        }
        return -1;
    }

    return (((ClTimeT)ts.tv_sec * 1000000) + (ts.tv_nsec/1000));
}



ClRcT
cosPosixTimeOfDayGet(ClTimerTimeOutT* pTime)
{
    struct timeval tv;
    ClRcT retCode=0;

    nullChkRet(pTime);

    CL_FUNC_ENTER();
    memset(&tv, 0, sizeof(struct timeval)); 

    if(0 != gettimeofday(&tv, NULL)) 
    {
        int err = errno;
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Error getting time of day, system error code %d", err);
        retCode = CL_OSAL_RC(CL_OSAL_ERR_TIME_OF_DAY);
        CL_FUNC_EXIT();
        return(retCode);
    }

    pTime->tsSec = tv.tv_sec;
    pTime->tsMilliSec = (ClUint32T)tv.tv_usec/1000;

    CL_FUNC_EXIT();
    return(CL_OK);
}

ClRcT
cosPosixNanoTimeGet(ClNanoTimeT *pTime)
{
    ClRcT rc = CL_OK;
    
    nullChkRet(pTime);

    sysErrnoChkRet(clock_gettime(CLOCK_REALTIME, (struct timespec *)pTime));

    return CL_OSAL_RC(rc);
}
/**************************************************************************/

static void taskSetScheduling(ClOsalTaskIdT tid, const char *taskName, int policy, int priority )
{
  struct sched_param schedParam = { 0 };

  schedParam.sched_priority = priority;

  if(policy != CL_OSAL_SCHED_OTHER) 
    {      
      int retCode = (ClRcT)pthread_setschedparam(tid,policy,(const struct sched_param*)&schedParam);
      if( 0 != retCode ) 
        { 
          clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task [%s] creation partial failure.  Unable to set scheduling policy.  Error [%s], code                           [%d], when calling pthread_setschedparam", taskName, strerror(retCode), retCode);
        }
    }

}

static void*
cosPosixTaskWrapper(void* pArgument)
{
    ClRcT retCode = CL_OK; 
    void* pTemp;
    CosTaskBlock_t* pTaskInfo = (CosTaskBlock_t*) pArgument;
    CosTaskBlock_t taskInfo;

    CL_FUNC_ENTER();

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task wrapper invoked.");
    taskInfo = *pTaskInfo;
    clHeapFree(pTaskInfo);
    
    retCode = clOsalTaskDataSet(gTaskControl.taskNameKey, taskInfo.taskName);
    if(CL_OK != retCode)
    {
        clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Task [%s] creation partial failure.  Unable to set task data.  Error [0x%x]",                                     taskInfo.taskName, retCode);
        CL_FUNC_EXIT();
        pTemp = NULL;
    }
    taskSetScheduling(pthread_self(), taskInfo.taskName, taskInfo.schedPolicy, taskInfo.schedPriority);

    clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Creating Task [%s]", taskInfo.taskName);

    /* Call the function to be executed */
    pTemp = (void*)(*taskInfo.fpTaskFunction) (taskInfo.pArgument);

   /* Implied by returning from this function pthread_exit(pTemp); */

    /* Note from web: Crashes in __nptl_deallocate_tsd() are symptomatic of invalid dangling thread key cleanup function (the callback to pthread_key_create()). Most probably a library registered a thread-local variable with a callback, then failed to delete the thread-local variable and got unloaded
*/
    return pTemp;
}



/**************************************************************************/
static ClRcT 
cosPosixMutexInitEx (ClOsalMutexT* pMutex, ClOsalMutexAttrT *pAttr)
{
    nullChkRet(pMutex);

    CL_FUNC_ENTER();
    pMutex->flags = CL_OSAL_SHARED_NORMAL;
    sysRetErrChkRet(pthread_mutex_init (&pMutex->shared_lock.mutex, pAttr));  
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixMutexInit (ClOsalMutexT* pMutex)
{
  return cosPosixMutexInitEx(pMutex, NULL);
}

ClRcT
cosPosixMutexErrorCheckInit(ClOsalMutexT *pMutex)
{
    pthread_mutexattr_t attr;
    nullChkRet(pMutex);
    CL_FUNC_ENTER();
    pMutex->flags = (ClOsalSharedMutexFlagsT) (CL_OSAL_SHARED_NORMAL | CL_OSAL_SHARED_ERROR_CHECK);
    sysRetErrChkRet(pthread_mutexattr_init(&attr));
#ifndef POSIX_BUILD
    sysRetErrChkRet(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    sysRetErrChkRet(pthread_mutex_init(&pMutex->shared_lock.mutex, &attr));
    CL_FUNC_EXIT();
    return CL_OK;
}

#ifdef CL_OSAL_DEBUG

ClRcT
cosPosixMutexInitDebug(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    cosPosixMutexPoolAdd(pMutex, file, line);
    return cosPosixMutexInit(pMutex);
}
#endif

ClRcT cosPosixProcessSharedFutexInit(ClOsalMutexT *pMutex)
{
#ifdef VXWORKS_BUILD
    return CL_ERR_NOT_SUPPORTED;
#else
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    pthread_mutexattr_t mattr;

    if (((retCode = (ClUint32T) pthread_mutexattr_init(&mattr)) != 0)
        || ((retCode = (ClUint32T) pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) != 0))
    { /* Unlikely to ever happen... */
        clDbgCodeError(retCode, ("pthread_mutexattr initialization error: %d", retCode));
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }
    pMutex->flags = (ClOsalSharedMutexFlagsT) ( pMutex->flags |  CL_OSAL_SHARED_PROCESS);
    retCode = (ClUint32T) pthread_mutex_init (&pMutex->shared_lock.mutex, &mattr);

    switch (retCode)
    {
    case 0: break;
    case EAGAIN:
        rc = CL_OSAL_RC(CL_ERR_NO_RESOURCE);
        clDbgRootCauseError(rc, ("EAGAIN: Cannot create another mutex"));
        break;
    case ENOMEM:
        rc = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        clDbgRootCauseError(rc, ("ENOMEM: Insufficient memory exists to initialise the mutex."));
        break;
    case EPERM:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(rc,("EPERM: The caller does not have the privileges to initialize the mutex."));
        break;
    case EBUSY:
        rc = CL_OSAL_RC(CL_ERR_INITIALIZED);
        clDbgCodeError(rc,("EBUSY: Mutex already initialized"));
        break;
    case EINVAL:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY); /* This function sets all mutex attributes, so I don't return invalid parameter */
        clDbgCodeError(rc,("EINVAL: Mutex attributes are invalid"));
        break;
    }
    return rc;
#endif
}


/**************************************************************************/
ClRcT 
cosPosixRecursiveMutexInit (ClOsalMutexT* pMutex)
{
    ClUint32T retCode = 0;
    pthread_mutexattr_t mattr;
    ClRcT rc = CL_OK;
  
    nullChkRet(pMutex);
    CL_FUNC_ENTER();  

    pMutex->flags = CL_OSAL_SHARED_NORMAL;

    if (((retCode = (ClUint32T) pthread_mutexattr_init(&mattr)) != 0)
        || ((retCode = (ClUint32T) pthread_mutexattr_settype(&mattr,PTHREAD_MUTEX_RECURSIVE )) != 0))
    { /* Unlikely to ever happen... */
        clDbgCodeError(retCode, ("pthread_mutexattr initialization error: %d", retCode));
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    retCode = (ClUint32T) pthread_mutex_init (&pMutex->shared_lock.mutex, &mattr);

    switch (retCode)
    {
    case 0: break;
    case EAGAIN:
        rc = CL_OSAL_RC(CL_ERR_NO_RESOURCE);
        clDbgRootCauseError(rc, ("EAGAIN: Cannot create another mutex"));
        break;
    case ENOMEM:
        rc = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        clDbgRootCauseError(rc, ("ENOMEM: Insufficient memory exists to initialise the mutex."));
        break;
    case EPERM:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(rc,("EPERM: The caller does not have the privileges to initialize the mutex."));
        break;
    case EBUSY:
        rc = CL_OSAL_RC(CL_ERR_INITIALIZED);
        clDbgCodeError(rc,("EBUSY: Mutex already initialized"));
        break;
    case EINVAL:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY); /* This module sets all mutex attributes, so I don't return invalid parameter */
        clDbgCodeError(rc,("EINVAL: Mutex attributes are invalid"));
        break;
    }

    CL_FUNC_EXIT();
    return (rc);
}

#ifdef CL_OSAL_DEBUG
ClRcT 
cosPosixRecursiveMutexInitDebug(ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line)
{
    ClUint32T retCode = 0;
    pthread_mutexattr_t mattr;
    ClRcT rc = CL_OK;
  
    nullChkRet(pMutex);
    CL_FUNC_ENTER();  

    pMutex->flags = CL_OSAL_SHARED_NORMAL | CL_OSAL_SHARED_RECURSIVE;

    if (((retCode = (ClUint32T) pthread_mutexattr_init(&mattr)) != 0)
        || ((retCode = (ClUint32T) pthread_mutexattr_settype(&mattr,PTHREAD_MUTEX_RECURSIVE )) != 0))
    { /* Unlikely to ever happen... */
        printf("pthread_mutexattr initialization error: %d\n", retCode);
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    retCode = (ClUint32T) pthread_mutex_init (&pMutex->shared_lock.mutex, &mattr);

    switch (retCode)
    {
    case 0: break;
    case EAGAIN:
        rc = CL_OSAL_RC(CL_ERR_NO_RESOURCE);
        printf("EAGAIN: Cannot create another mutex\n");
        break;
    case ENOMEM:
        rc = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        printf("ENOMEM: Insufficient memory exists to initialise the mutex.\n");
        break;
    case EPERM:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        printf("EPERM: The caller does not have the privileges to initialize the mutex.\n");
        break;
    case EBUSY:
        rc = CL_OSAL_RC(CL_ERR_INITIALIZED);
        printf("EBUSY: Mutex already initialized\n");
        break;
    case EINVAL:
        rc = CL_OSAL_RC(CL_ERR_LIBRARY); /* This module sets all mutex attributes, so I don't return invalid parameter */
        printf("EINVAL: Mutex attributes are invalid\n");
        break;
    }

    CL_FUNC_EXIT();
    return (rc);
}
#endif

/**************************************************************************/

ClRcT 
cosPosixMutexAttrInit (ClOsalMutexAttrT* pAttr)
{
    nullChkRet(pAttr);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_mutexattr_init(pAttr));
    CL_FUNC_EXIT();
    return CL_OK;
}
/**************************************************************************/

ClRcT 
cosPosixMutexAttrDestroy (ClOsalMutexAttrT* pAttr)
{
    nullChkRet(pAttr);
 
    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_mutexattr_destroy(pAttr));
    CL_FUNC_EXIT();

    return CL_OK;
}
/**************************************************************************/

ClRcT 
cosPosixMutexAttrPSharedSet (ClOsalMutexAttrT* pAttr, ClOsalSharedTypeT type)
{
#ifdef VXWORKS_BUILD
    return CL_ERR_NOT_SUPPORTED;
#else

  ClRcT rc = CL_OK;

  nullChkRet(pAttr);

  CL_FUNC_ENTER();
  int ret = pthread_mutexattr_setpshared(pAttr, type);
  if (0 != ret)
    {
      if (EINVAL == ret)
        {
          rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
          clDbgCodeError(rc, ("Invalid parameter passed to system call"));
        }
      else
        {
          rc = CL_OSAL_RC(CL_ERR_UNSPECIFIED);
          clDbgCodeError(rc, ("Error code not handled %d (%s)", ret, strerror(ret)));
        }

    }

  return rc;
#endif
}


/**************************************************************************/

ClRcT 
cosPosixMutexCreate (ClOsalMutexIdT* pMutexId)
{
    ClUint32T retCode = 0;
    ClOsalMutexT *pMutex = NULL;
   
    nullChkRet(pMutexId);
    CL_FUNC_ENTER();
    pMutex = (ClOsalMutexT*) clHeapAllocate((ClUint32T)sizeof(ClOsalMutexT));
    if(NULL == pMutex)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex creation failed, out of memory.");
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    retCode =(ClUint32T) cosPosixMutexInit(pMutex);

    if(0 != retCode)
	{
        clHeapFree(pMutex);
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex creation failed, error [0x%x]", retCode);
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_OSAL_ERR_MUTEX_CREATE));
	}

    *pMutexId = ((ClOsalMutexIdT)pMutex);
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT 
cosPosixMutexErrorCheckCreate (ClOsalMutexIdT* pMutexId)
{
    ClUint32T retCode = 0;
    ClOsalMutexT *pMutex = NULL;
   
    nullChkRet(pMutexId);
    CL_FUNC_ENTER();
    pMutex = (ClOsalMutexT*) clHeapAllocate((ClUint32T)sizeof(ClOsalMutexT));
    if(NULL == pMutex)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex creation failed, out of memory.");
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    retCode =(ClUint32T) cosPosixMutexErrorCheckInit(pMutex);

    if(0 != retCode)
	{
        clHeapFree(pMutex);
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex creation failed, error [0x%x]", retCode);
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_OSAL_ERR_MUTEX_CREATE));
	}

    *pMutexId = ((ClOsalMutexIdT)pMutex);
    CL_FUNC_EXIT();
    return (CL_OK);
}

#ifdef CL_OSAL_DEBUG
ClRcT 
cosPosixMutexCreateDebug (ClOsalMutexIdT* pMutexId, const ClCharT *file, ClInt32T line)
{
    ClUint32T retCode = 0;
    ClOsalMutexT *pMutex = NULL;
   
    nullChkRet(pMutexId);
    CL_FUNC_ENTER();
    pMutex = (ClOsalMutexT*) calloc(1, (ClUint32T)sizeof(ClOsalMutexT));
    if(NULL == pMutex)
    {
        printf("Mutex creation failed, out of memory.\n");
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    retCode =(ClUint32T) cosPosixMutexInitDebug(pMutex, file, line);

    if(0 != retCode)
	{
        free(pMutex);
        printf("Mutex creation failed, error [0x%x]\n", retCode);
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_OSAL_ERR_MUTEX_CREATE));
	}

    *pMutexId = ((ClOsalMutexIdT)pMutex);
    CL_FUNC_EXIT();
    return (CL_OK);
}
#endif

/**************************************************************************/

ClRcT
cosPosixCondInit(ClOsalCondT *pCond)
{
    pthread_condattr_t attr;
    nullChkRet(pCond);
    
    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_condattr_init(&attr));
#ifdef __linux__
#if __GNUC__ > 3
    sysRetErrChkRet(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
#endif
#endif
    sysRetErrChkRet(pthread_cond_init(pCond, &attr));
    CL_FUNC_EXIT();

    return (CL_OK);
}

ClRcT 
cosPosixProcessSharedCondInit (ClOsalCondT* ptr)
{
#ifdef VXWORKS_BUILD
    return CL_ERR_NOT_SUPPORTED;
#else
  ClUint32T retCode = 0;
  pthread_condattr_t mattr;
  ClRcT rc = CL_OK;
  
  nullChkRet(ptr);
  CL_FUNC_ENTER();  
  memset(&mattr, 0, sizeof(mattr));
  if (((retCode = (ClUint32T) pthread_condattr_init(&mattr)) != 0)
      || ((retCode = (ClUint32T) pthread_condattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) != 0))
    { /* Unlikely to ever happen... */
      clDbgCodeError(retCode, ("pthread_condattr initialization error: %d", retCode));
      return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

#ifdef __linux__
#if __GNUC__ > 3
  sysRetErrChkRet(pthread_condattr_setclock(&mattr, CLOCK_MONOTONIC));
#endif
#endif

  retCode = (ClUint32T) pthread_cond_init (ptr, &mattr);

  switch (retCode)
    {
    case 0: break;
    case EAGAIN:
      rc = CL_OSAL_RC(CL_ERR_NO_RESOURCE);
      clDbgRootCauseError(rc, ("EAGAIN: Cannot create another task condition"));
      break;
    case ENOMEM:
      rc = CL_OSAL_RC(CL_ERR_NO_MEMORY);
      clDbgRootCauseError(rc, ("ENOMEM: Insufficient memory exists to initialise the task condition."));
      break;
    case EPERM:
      rc = CL_OSAL_RC(CL_ERR_LIBRARY);
      clDbgCodeError(rc,("EPERM: The caller does not have the privileges to initialize the task condition."));
      break;
    case EBUSY:
      rc = CL_OSAL_RC(CL_ERR_INITIALIZED);
      clDbgCodeError(rc,("EBUSY: Task condition already initialized"));
      break;
    case EINVAL:
      rc = CL_OSAL_RC(CL_ERR_LIBRARY); /* This function sets all condition attributes, so I don't return invalid parameter */
      clDbgCodeError(rc,("EINVAL: Task condition attributes are invalid"));
      break;
    }

  CL_FUNC_EXIT();
  return (rc);
#endif
}


/**************************************************************************/
ClRcT 
cosPosixCondInitEx (ClOsalCondT* pCond, ClOsalCondAttrT *pAttr)
{
    nullChkRet(pCond);
    nullChkRet(pAttr);

    CL_FUNC_ENTER();
#ifdef __linux__
#if __GNUC__ > 3
    sysRetErrChkRet(pthread_condattr_setclock(pAttr, CLOCK_MONOTONIC));
#endif
#endif    
    sysRetErrChkRet(pthread_cond_init(pCond, pAttr));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixCondAttrInit (ClOsalCondAttrT* pAttr)
{
    nullChkRet(pAttr);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_condattr_init(pAttr));
    CL_FUNC_EXIT();

    return CL_OK;
}
/**************************************************************************/

ClRcT 
cosPosixCondAttrDestroy (ClOsalCondAttrT* pAttr)
{
    nullChkRet(pAttr);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_condattr_destroy(pAttr));
    CL_FUNC_EXIT();

    return CL_OK;
}
/**************************************************************************/

ClRcT 
cosPosixCondAttrPSharedSet (ClOsalCondAttrT* pAttr, ClOsalSharedTypeT type)
{
#ifdef VXWORKS_BUILD
    return CL_ERR_NOT_SUPPORTED;
#else
    nullChkRet(pAttr);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_condattr_setpshared(pAttr, type));
    CL_FUNC_EXIT();

    return CL_OK;
#endif
}
/**************************************************************************/


ClRcT
cosPosixCondCreate(ClOsalCondIdT *pConditionId)
{
    pthread_cond_t* pConditionVar = NULL;
    ClRcT retCode = CL_OK;

    nullChkRet(pConditionId);
    CL_FUNC_ENTER();

    pConditionVar = (pthread_cond_t*) clHeapAllocate ((ClUint32T)sizeof (pthread_cond_t));

    if(NULL == pConditionVar)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Condition create failed: out of memory");
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }

    retCode = cosPosixCondInit(pConditionVar);

    if(CL_OK != retCode)
	{
        clHeapFree(pConditionVar);
        return(retCode);
	}

    *pConditionId = ((ClOsalCondIdT)pConditionVar);
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT
cosPosixCondDestroy(ClOsalCondT *pCond)
{
    nullChkRet(pCond);

    CL_FUNC_ENTER();

    sysRetErrChkRet(pthread_cond_destroy(pCond));

    CL_FUNC_EXIT();
    return CL_OK;
}

/**************************************************************************/

ClRcT
cosPosixCondDelete(ClOsalCondIdT conditionId)
{
    pthread_cond_t * pConditionVar = (pthread_cond_t*) conditionId;
    ClRcT retCode = CL_OK;
    nullChkRet(pConditionVar);
    CL_FUNC_ENTER();

    retCode = cosPosixCondDestroy(pConditionVar);
	
    if(CL_OK != retCode) return(retCode);
   
    clHeapFree(pConditionVar);
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixCondWait (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer)
{
    pthread_cond_t* pConditionVar = (pthread_cond_t*) conditionId;
    ClOsalMutexT *pMutexId = (ClOsalMutexT*) mutexId;
    ClUint64T nanoSecTime = 0;
    struct timespec timeOut;
    
    nullChkRet(pConditionVar);
    nullChkRet(pMutexId);

    CL_FUNC_ENTER();
    
    memset(&timeOut, 0, sizeof(timeOut));
    
    if( (pMutexId->flags & CL_OSAL_SHARED_NORMAL) )
    {
        if ((0 == timer.tsSec) && (0 == timer.tsMilliSec))
        {
            sysRetErrChkRet(pthread_cond_wait (pConditionVar, &pMutexId->shared_lock.mutex));
        }
        else
        {
            struct timespec current = {0};
            int result = 0;
            clockid_t clock = CLOCK_REALTIME;
#ifdef __linux__
#if __GNUC__ > 3
            clock = CLOCK_MONOTONIC;
#endif
#endif
            sysRetErrChkRet(clock_gettime(clock,  &current));
            nanoSecTime = 	((ClUint64T)current.tv_nsec) + ((ClUint64T)timer.tsMilliSec * 1000LL * 1000LL);
	    
            timeOut.tv_nsec = (nanoSecTime % 1000000000);
            timeOut.tv_sec = (current.tv_sec + timer.tsSec + (nanoSecTime / 1000000000));

            result = pthread_cond_timedwait (pConditionVar, &pMutexId->shared_lock.mutex, &timeOut);
            if(result == ETIMEDOUT )
            {
                return CL_OSAL_RC(CL_ERR_TIMEOUT);
            }

            sysRetErrChkRet(result);
        }
    }
    else
    {
        /*
          clDbgCodeError(CL_ERR_LIBRARY,("condwait not supported for sems\n"));
        */
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }
    CL_FUNC_EXIT();
    return (CL_OK);
}

#ifdef CL_OSAL_DEBUG

ClRcT
cosPosixCondWaitDebug (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer,
                       const ClCharT *file, ClInt32T line)
{
    pthread_cond_t* pConditionVar = (pthread_cond_t*) conditionId;
    ClOsalMutexT *pMutexId = (ClOsalMutexT*) mutexId;
    ClUint64T nanoSecTime = 0;
    struct timespec timeOut;
    int result = 0;
    nullChkRet(pConditionVar);
    nullChkRet(pMutexId);

    CL_FUNC_ENTER();
    
    memset(&timeOut, 0, sizeof(timeOut));
    
    if( (pMutexId->flags & CL_OSAL_SHARED_NORMAL) )
    {
        if ((0 == timer.tsSec) && (0 == timer.tsMilliSec))
        {
            if(!(pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                cosPosixMutexPoolUnlock(pMutexId, file, line);

           
            result = pthread_cond_wait (pConditionVar, &pMutexId->shared_lock.mutex);
            if(result)
            {
                if(! (pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                    cosPosixMutexPoolLockSoft(pMutexId, file, line);
                sysRetErrChkRet(result);
            }
                
            if(!(pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                cosPosixMutexPoolLockSoft(pMutexId, file, line);
        }
        else
        {
            struct timespec current = {0};
            int result = 0;
            clockid_t clock = CLOCK_REALTIME;
#ifdef __linux__
#if __GNUC__ > 3
            clock = CLOCK_MONOTONIC;
#endif
#endif
            sysRetErrChkRet(clock_gettime(clock,  &current));
            nanoSecTime = 	((ClUint64T)current.tv_nsec) + ((ClUint64T)timer.tsMilliSec * 1000LL * 1000LL);
	    
            timeOut.tv_nsec = (nanoSecTime % 1000000000);
            timeOut.tv_sec = (current.tv_sec + timer.tsSec + (nanoSecTime / 1000000000));
            
            if(!(pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                cosPosixMutexPoolUnlock(pMutexId, file, line);

            result = pthread_cond_timedwait (pConditionVar, &pMutexId->shared_lock.mutex, &timeOut);
            if(result)
            {
                if(!(pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                    cosPosixMutexPoolLockSoft(pMutexId, file, line);
            }
            if(result == ETIMEDOUT )
            {
                return CL_OSAL_RC(CL_ERR_TIMEOUT);
            }
            sysRetErrChkRet(result);
            if(!(pMutexId->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS)))
                cosPosixMutexPoolLockSoft(pMutexId, file, line);
        }
    }
    else
    {
        /*
          clDbgCodeError(CL_ERR_LIBRARY,("condwait not supported for sems\n"));
        */
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }
    CL_FUNC_EXIT();
    return (CL_OK);
}

#endif

/**************************************************************************/
ClRcT
cosPosixCondBroadcast (ClOsalCondIdT conditionId)
{
    pthread_cond_t* pConditionId = (pthread_cond_t*) conditionId;

    nullChkRet(pConditionId);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_cond_broadcast (pConditionId));
    CL_FUNC_EXIT();

    return (CL_OK);
}


/**************************************************************************/

ClRcT
cosPosixCondSignal (ClOsalCondIdT conditionId)
{
    pthread_cond_t* pConditionId = (pthread_cond_t*) conditionId;

    nullChkRet(pConditionId);

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_cond_signal (pConditionId));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixTaskDataSet(ClUint32T key, ClOsalTaskDataT threadData)
{
    void* tmp = (void *)threadData;

    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_setspecific ((pthread_key_t)key, tmp));
    CL_FUNC_EXIT();

    return(CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixTaskDataGet(ClUint32T key, ClOsalTaskDataT* pThreadData)
{
  void* tmp = NULL;

  nullChkRet(pThreadData);

  CL_FUNC_ENTER();

  tmp = (void*)pthread_getspecific(key);
  if (tmp == NULL)
    {
      int retCode  = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
      *pThreadData = 0; /* Clear it out in case the app is misbehaved */
      //clLogWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Access of non-existent task-specific data key [0x%x]", key);
      clDbgCodeError(retCode, ("Access of non-existent task-specific data key [0x%x]", key));
      CL_FUNC_EXIT();
      return(retCode);
    }

  *pThreadData = tmp; 
  CL_FUNC_EXIT();
  return (CL_OK);

}

/**************************************************************************/

ClRcT 
cosPosixTaskKeyCreate(ClUint32T* pKey,  void(*ClOsalTaskKeyDeleteCallBackT)(void*))
{
    pthread_key_t key = 0;

    nullChkRet(pKey);

    CL_FUNC_ENTER();

    sysRetErrChkRet(pthread_key_create (&key, ClOsalTaskKeyDeleteCallBackT));
 
    *pKey = (ClUint32T)key;
    CL_FUNC_EXIT();
    return(CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixTaskKeyDelete(ClUint32T key)
{
    CL_FUNC_ENTER();
    sysRetErrChkRet(pthread_key_delete(key));
    CL_FUNC_EXIT();

    return(CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixProcessDelete(ClOsalPidT processId)
{
    ClInt32T retCode = CL_OK;
   
    CL_FUNC_ENTER();
    if((processId ==0)||((int)processId < 0))
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess Delete: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_PROCESS_DELETE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    retCode = kill ((pid_t)processId, SIGKILL);   

    if(0 != retCode)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess Delete: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_PROCESS_DELETE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess Delete: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixProcessWait(ClOsalPidT processId)
{
    ClInt32T retCode = CL_OK;
   
    CL_FUNC_ENTER();
#ifdef SOLARIS_BUILD
    retCode = waitpid ((pid_t)processId, NULL, 0);
#else
    retCode = waitpid ((__pid_t)processId, NULL, 0);   
#endif

    if(retCode < 0)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess wait: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_PROCESS_WAIT);
        CL_FUNC_EXIT();
        return(retCode);
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess wait: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixProcessSelfIdGet(ClOsalPidT* pProcessId)
{
    ClRcT retCode = 0;
    CL_FUNC_ENTER();
    if(NULL == pProcessId)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess ID Get: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    *pProcessId = (ClOsalPidT)getpid();

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nProcess ID Get: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixMaxPathGet(const ClCharT* path, ClInt32T* pLength)
{
    ClRcT rc = CL_OK;
    long len;

    CL_FUNC_ENTER();

    if(NULL == pLength || NULL == path)
    {
        rc = CL_ERR_NULL_POINTER;
        goto err;
    }
    

    len = pathconf(path, _PC_PATH_MAX);
    if(len < 0)
    {
        if(EINVAL == errno)
        {
            rc = CL_ERR_NOT_SUPPORTED;
            goto err;
        }

        if(0 == errno)
        {
            rc = CL_ERR_DOESNT_EXIST;
            goto err;
        }

        rc = CL_ERR_UNSPECIFIED;
        goto err;
    }

    len--; /*TODO: Able to create a file of 'len-1' only, not len*/

    *pLength = len;
    CL_FUNC_EXIT();
    return CL_OK;
   
err:
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nFetching maximum pathname size: FAILED");
    CL_FUNC_EXIT();
    return CL_OSAL_RC(rc);
}

ClRcT
cosPosixPageSizeGet(ClInt32T* pSize)
{
    ClRcT rc = CL_OK;
    long size;

    CL_FUNC_ENTER();

    if(NULL == pSize)
    {
        rc = CL_ERR_NULL_POINTER;
        goto err;
    }
    

    size = sysconf(_SC_PAGESIZE);
    if(size < 0)
    {
        if(EINVAL == errno)
        {
            rc = CL_ERR_NOT_SUPPORTED;
            goto err;
        }

        rc = CL_ERR_UNSPECIFIED;
        goto err;
    }

    *pSize = size;
    CL_FUNC_EXIT();
    return CL_OK;
   
err:
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nFetching page size: FAILED");
    CL_FUNC_EXIT();
    return CL_OSAL_RC(rc);
}

/**************************************************************************/



void clOsalErrorReportHandler(void *info)
{
    return;
}
