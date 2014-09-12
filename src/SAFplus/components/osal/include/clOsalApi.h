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

/**
 * \file
 * \brief Operating System Abstraction Layer API
 * \ingroup osal_apis
 *
 */

/**
 * \addtogroup osal_apis
 * \{
 */

/*********************************************************************************/
/************************************* OSAL  APIs ********************************/
/*********************************************************************************/
/*										                                         */
/* clOsalInitialize                                                              */
/* clOsalFinalize                                                                */
/* clOsalTaskCreate                                                              */
/* clOsalTaskDelete                                                              */
/* clOsalSelfTaskIdGet                                                           */
/* clOsalTaskNameGet                                                             */
/* clOsalTaskPriorityGet                                                         */
/* clOsalTaskPrioritySet                                                         */
/* clOsalTaskDelay                                                               */
/* clOsalTimeOfDayGet                                                            */
/* clOsalMutexCreate                                                             */
/* clOsalMutexCreateAndLock                                                      */
/* clOsalMutexLock                                                               */
/* clOsalMutexUnlock                                                             */
/* clOsalMutexDelete                                                             */
/* clOsalNumMemoryAllocGet                                                       */
/* clOsalNumMemoryDeallocedGet                                                   */
/* clOsalCondCreate                                                              */
/* clOsalCondDelete                                                              */
/* clOsalCondWait                                                                */
/* clOsalCondBroadcast                                                           */
/* clOsalCondSignal                                                              */
/* clOsalTaskKeyCreate                                                           */
/* clOsalTaskKeyDelete                                                           */
/* clOsalTaskDataSet                                                             */
/* clOsalTaskDataGet                                                             */
/* clOsalPrintf                                                                  */
/* clOsalPrintf                                                                  */
/* clOsalSemCreate                                                               */
/* clOsalSemIdGet                                                                */
/* clOsalSemLock                                                                 */
/* clOsalSemTryLock                                                              */
/* clOsalSemUnlock                                                               */
/* clOsalSemValueGet                                                             */
/* clOsalSemDelete                                                               */
/* clOsalProcessCreate                                                           */
/* cosProcessResume                                                              */
/* cosProcessSuspend                                                             */
/* clOsalProcessDelete                                                           */
/* clOsalProcessWait                                                             */
/* clOsalProcessSelfIdGet                                                        */
/* clOsalShmCreate                                                               */
/* clOsalShmIdGet                                                                */
/* clOsalShmDelete                                                               */
/* clOsalShmAttach                                                               */
/* clOsalShmDetach                                                               */
/* clOsalShmSecurityModeSet                                                      */
/* clOsalShmSecurityModeGet                                                      */
/* clOsalShmSizeGet                                                              */
/* clOsalSigHandlerInitialize                                                    */
/* clOsalErrorReportHandler                                                      */
/*										                                         */
/*********************************************************************************/

#ifndef _CL_OSAL_API_H_
#define _CL_OSAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __KERNEL__
#include <stdlib.h>
#include <pthread.h>
#endif


#include <clCommon.h>
#include <clTimerApi.h>
#include <clHeapApi.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif /* DMALLOC */

#ifdef CL_OSAL_DEBUG
#include <clList.h>
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/



/**
 * This is required while Thread or Task creation.
 */
#define CL_OSAL_MIN_STACK_SIZE  196608

/**
 * The maximum length of thread/task names
 */
#define CL_OSAL_NAME_MAX 32
/* Uncomment following line to enable signal handler feature */
/*#define CL_OSAL_SIGNAL_HANDLER 1 */

#ifdef __KERNEL__
#define clOsalPrintf    printk
#endif /* __KERNEL__ */
/* FIXME: This doesn't look clean */
/* Remove the #if 0 below to enable memleak detection tool in kernel space */
#if 0
#define MEM_LEAK_DETECT
#endif
#ifdef MEM_LEAK_DETECT
#   define kmalloc(x,y)    clHeapAllocate(x)
#   define kfree           clHeapFree
#endif /* MEM_LEAK_DETECT */

/******************************************************************************
 *  Data Types
 *****************************************************************************/

#ifndef __KERNEL__

#include <clArchHeaders.h>
#include <semaphore.h>

/**
 *  The mutex type to be initialized
 */
typedef enum ClOsalSharedMutexFlags
{
    CL_OSAL_SHARED_INVALID   = 0x0,
    CL_OSAL_SHARED_NORMAL    = 0x1,
    CL_OSAL_SHARED_SYSV_SEM  = 0x2,
    CL_OSAL_SHARED_POSIX_SEM = 0x4,
    CL_OSAL_SHARED_RECURSIVE = 0x8,
    CL_OSAL_SHARED_PROCESS   = 0x10,
    CL_OSAL_SHARED_ERROR_CHECK = 0x20,
}ClOsalSharedMutexFlagsT;

typedef struct ClOsalMutex
{
#ifdef CL_OSAL_DEBUG
#define CL_OSAL_DEBUG_MAGIC 0xDEADBEEF
    ClUint32T magic;
    pthread_t tid;
    const ClCharT *creatorFile;
    ClInt32T creatorLine;
    const ClCharT *ownerFile;
    ClInt32T ownerLine;
    ClListHeadT list; /*entry into the mutex pool*/
#endif
    ClOsalSharedMutexFlagsT flags;
    union shared_lock
    {
        pthread_mutex_t mutex;
        struct ClSem
        {
            sem_t posSem;
            int semId;
            ClInt32T numSems;
        }sem;
    }shared_lock;
}ClOsalMutexT;

typedef pthread_mutexattr_t ClOsalMutexAttrT;
typedef pthread_condattr_t ClOsalCondAttrT;
typedef enum ClOsalSharedType
{
    CL_OSAL_PROCESS_PRIVATE = 0, /*default*/
    CL_OSAL_PROCESS_SHARED = 1
}ClOsalSharedTypeT;

/**
 *  The thread condition type wrapped
 */
typedef pthread_cond_t      ClOsalCondT;
#endif

/**
 * The type of an identifier to the OSAL Task ID.
 */
typedef ClUint64T           ClOsalTaskIdT;

/**
 * The type of an identifier to the OSAL Mutex ID.
 */
#ifndef __KERNEL__
typedef ClOsalMutexT*       ClOsalMutexIdT;
#else
typedef ClPtrT              ClOsalMutexIdT;
#endif



/**
 * The type of an identifier to the OSAL condition ID.
 */
#ifndef __KERNEL__
typedef ClOsalCondT*        ClOsalCondIdT;
#else
typedef ClPtrT              ClOsalCondIdT; // Mynk NTC
#endif
/**
 * The type of an identifier to the OSAL Semaphore ID.
 */
#ifndef POSIX_BUILD
typedef ClHandleT           ClOsalSemIdT;
#else
typedef sem_t*              ClOsalSemIdT;
#endif

/**
 * The type of an identifier to the OSAL Shared Memory ID.
 */
typedef ClUint32T           ClOsalShmIdT;

/**
 * The type of an identifier to the OSAL Process ID.
 */
typedef ClUint32T           ClOsalPidT;

/**
 * The type of an identifier to the OSAL Task Data type.
 */
typedef ClPtrT              ClOsalTaskDataT;

typedef struct timespec ClNanoTimeT;

/**
 * CAllback
 type of callback function invoked when a process is created.
 */
typedef void (*ClOsalProcessFuncT)(void*);

/**
 * CAllback
 type of callback function invoked when the task specific private key is destruyed.
 */

typedef void (*ClOsalTaskKeyDeleteCallBackT)(void*);

/* ENUM */

/**
 * The following enumeration type contains schedule policy of the tasks that will be created.
 * The values of the ClOsalSchedulePolicyT enumeration type have the following interpretation:
 * other is the default scheduling policy
 * fifo & rr are used for real time threads (you must be running with superuser priviledges).
 */
typedef enum {  /* DO NOT CHANGE the order of this enum, since it must be aligned with the Posix ordering */

    /** Default scheduling mechanism */
    CL_OSAL_SCHED_OTHER = SCHED_OTHER, 

    /** First-in-first-out */
    CL_OSAL_SCHED_FIFO = SCHED_FIFO,      
    
    /** Roundrobin */
    CL_OSAL_SCHED_RR = SCHED_RR        
} ClOsalSchedulePolicyT;

/**
 * The following enumeration type contains the various thread priorities.
 * The values of the ClOsalProcessFlagT enumeration type have the following interpretation:
 */
typedef enum
{
/**
 * When the scheduling is CL_OSAL_SCHED_OTHER, priority is not used
 */
    CL_OSAL_THREAD_PRI_NOT_APPLICABLE = 0,

/**
 * Highest thread priority.
 */
    CL_OSAL_THREAD_PRI_HIGH = 160,  /* COS_MAX_PRIORITY, */

/**
 * Medium thread priority.
 */
    CL_OSAL_THREAD_PRI_MEDIUM = 80, /* (COS_MAX_PRIORITY - COS_MIN_PRIORITY)/2, */

/**
 * Lowest thread priority.
 */
    CL_OSAL_THREAD_PRI_LOW = 1,     /* COS_MIN_PRIORITY */
} ClOsalThreadPriorityT;

/**
 *  Process creation flags.
 *  The values of the ClOsalProcessFlagT enumeration type have the following interpretation:
 *
 */
typedef enum {
    /** This will create a process with new session. */
    CL_OSAL_PROCESS_WITH_NEW_SESSION    = CL_BIT(0),
    /** This would create a new process group. */
    CL_OSAL_PROCESS_WITH_NEW_GROUP      = CL_BIT(1)
} ClOsalProcessFlagT;

#if 0
/**
 *  Shared memory creation flags.
 *
 *  \c CL_OSAL_SHM_PRIVATE: The shared memory created is private for this process.
 *  \c CL_OSAL_SHM_PUBLIC: The shared memory created can be shared across all process.
 *
 */
typedef enum{

/**
 *  The shared memory created is private for this process.
 */
              CL_OSAL_SHM_PRIVATE = BIT0,

/**
 *  The shared memory created can be shared across all process.
 */
              CL_OSAL_SHM_PUBLIC = BIT1
            } CosShmFlag_e;
#endif

  /** Shared memory security options */
typedef enum {
    /** read-only by this user */
    CL_OSAL_SHM_MODE_READ_USER      = 0x0100,
    /** read-only by the group */
    CL_OSAL_SHM_MODE_READ_GROUP     = 0x0020,
    /** read-only by everyone */
    CL_OSAL_SHM_MODE_READ_OTHERS    = 0x0004,
    /** read/write by this user*/
    CL_OSAL_SHM_MODE_WRITE_USER     = 0x0080,
    /** read/write by the group */
    CL_OSAL_SHM_MODE_WRITE_GROUP    = 0x0010,
    /** read/write by everyone */
    CL_OSAL_SHM_MODE_WRITE_OTHERS   = 0x0002
} ClOsalShmSecurityModeFlagT;

/**
 * Shared memory area definition.
 */
#define CL_OSAL_SHM_EXCEPTION_LENGTH     2048
typedef struct {
/**
 * Place to store the component Exception information.
 */
        ClCharT       exceptionInfo[CL_OSAL_SHM_EXCEPTION_LENGTH];
}ClOsalShmAreaDefT;

/**
 * Shared memory associated with the component.
 */
extern ClOsalShmAreaDefT *gpClShmArea;
/**
 * Shared memory ID associated with the component.
 */
extern ClOsalShmIdT gClCompUniqueShmId;

/*****************************************************************************
 *  OSAL APIs
 *****************************************************************************/

/**
 ************************************
 *  \brief Initializes the Operating System Abstraction Layer (OSAL).
 *
 *  \par Header File:
 *  clOsalApi.h
 *
 *  \par Library File:
 *  libClOsal
 *
 *  \retval CL_RC_OK This API executed successfully.
 *  \retval CL_ERR_NO_MEM On memory allocation failure.
 *  \retval CL_ERR_MUTEX_CREATE On failure in creating a mutex.
 *
 *  \par Description:
 *  This API is used to initialize the OSAL. This should be the first
 *  API to be called before any of the other OSAL APIs are invoked.
 *
 *  \sa clOsalFinalize()
 *
 */
ClRcT clOsalInitialize(const ClPtrT pConfig);

/**
 ****************************************
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_CL_OSAL_CLEANUP On failure to clean up OSAL. 
 * \retval CL_ERR_MUTEX_DELETE On failure in deleting mutex. 
 * 
 * \par Description
 * This API is used to clean-up OSAL. This is last OSAL API that is
 *    invoked, typically this must be called during system shutdown. 
 * 
 * \sa clOsalInitialize() 
 */
ClRcT clOsalFinalize(void);

/* Tasking API's */


/**
 ****************************************
 * \brief Creates a task. 
 * 
 * \param taskName Name of the task. This must be a valid string. NULL is regarded as
 *    invalid but the task creation does not fail. 
 * \param schedulePolicy Schedule policy can be set as one of the following: 
 *   \arg CL_OSAL_SCHED_RR For this, you must be logged in as super-user. It supports
 *    priority-based realtime round-robin scheduling. 
 *   \arg CL_OSAL_SCHED_FIFO For this, you must be logged in as super-user. It supports priority
 *    based pre-emptive scheduling. This parameter is ignored in case of
 *    VxWorks. 
 *   \arg CL_OSAL_SCHED_OTHER 
 * \param priority Priority at which the tasks is executed. The priority must be between
 *    1 and 160 (1 - lowest priority 160 - highest priority). Any other
 *    value is regarded as wrong and task creation fails. 
 * \param stackSize Size (in bytes) of the user stack that must be created when the task
 *    is executed. The stack size must be a positive integer. If zero is
 *    mentioned, then the default stack size of 4096 bytes would be
 *    allocated. If the stack size mentioned is less than the default
 *    stack size then the default stack size is assigned. 
 * \param fpTaskFunction Entry point of the task. This function is executed when the task is
 *    executed. This is a function pointer and NULL argument would result
 *    in error. 
 * \param pTaskFuncArgument The argument to be passed to \e fpTaskFuncArgument when the function
 *    is executed. This must be a valid pointer and if there are no
 *    arguments to be passed then NULL can be passed. Any other value will
 *    be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_INVLD_PARAM On passing invalid parameters. 
 * \retval CL_ERR_TASK_ATTRIBUTE_INIT On failure in task attribute initialization. 
 * \retval CL_ERR_TASK_CREATE On failure to create task. 
 * \retval CL_ERR_TASK_ATTRIBUTE_SET On failure in setting task attributes. 
 * 
 * \par Description
 * This API is used to spawn a new task that runs concurrently with the
 *    calling task. The new task invokes the user API \e fpTaskFunction
 *    with the argument \e pTaskFuncArgument to the function. The new task
 *    terminates explicitly by calling clOsalTaskDelete or implicitly, by
 *    returning from the \e fpTaskFunction. 
 * 
 * \sa clOsalTaskDelete(), clOsalSelfTaskIdGet(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskCreateDetached (const ClCharT *taskName,
        ClOsalSchedulePolicyT schedulePolicy,
        ClUint32T priority,
        ClUint32T stackSize,
        void* (*fpTaskFunction)(void*),
        void* pTaskFuncArgument);

/**
 ****************************************
 * \brief Creates a task. 
 * 
 * \param taskName Name of the task. This must be a valid string. NULL is regarded as
 *    invalid but the task creation does not fail. 
 * \param schedulePolicy Schedule policy can be set as one of the following: 
 *   \arg CL_OSAL_SCHED_RR For this, you must be logged in as super-user. It supports
 *    priority-based realtime round-robin scheduling. 
 *   \arg CL_OSAL_SCHED_FIFO For this, you must be logged in as super-user. It supports priority
 *    based pre-emptive scheduling. This parameter is ignored in case of
 *    VxWorks. 
 *   \arg CL_OSAL_SCHED_OTHER 
 * \param priority Priority at which the tasks is executed. The priority must be between
 *    1 and 160 (1 - lowest priority 160 - highest priority). Any other
 *    value is regarded as wrong and task creation fails. 
 * \param stackSize Size (in bytes) of the user stack that must be created when the task
 *    is executed. The stack size must be a positive integer. If zero is
 *    mentioned, then the default stack size of 4096 bytes would be
 *    allocated. If the stack size mentioned is less than the default
 *    stack size then the default stack size is assigned. 
 * \param fpTaskFunction Entry point of the task. This function is executed when the task is
 *    executed. This is a function pointer and NULL argument would result
 *    in error. 
 * \param pTaskFuncArgument The argument to be passed to \e fpTaskFuncArgument when the function
 *    is executed. This must be a valid pointer and if there are no
 *    arguments to be passed then NULL can be passed. Any other value will
 *    be invalid. 
 * \param pTaskId (out)The identifier of the task started is stored here. This shall be
 *    any valid pointer, NULL will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_INVLD_PARAM On passing invalid parameters. 
 * \retval CL_ERR_TASK_ATTRIBUTE_INIT On failure in task attribute initialization. 
 * \retval CL_ERR_TASK_CREATE On failure to create task. 
 * \retval CL_ERR_TASK_ATTRIBUTE_SET On failure in setting task attributes. 
 * 
 * \par Description
 * This API is used to spawn a new task that runs concurrently with the
 *    calling task. The new task invokes the user API \e fpTaskFunction
 *    with the argument \e pTaskFuncArgument to the function. The new task
 *    terminates explicitly by calling clOsalTaskDelete or implicitly, by
 *    returning from the \e fpTaskFunction.  Since the task it "attached",
 *    it can be manipulated using the task handle returned in the \e pTaskId.
 *    parameter.
 * 
 * \sa clOsalTaskDelete(), clOsalSelfTaskIdGet(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskCreateAttached(const ClCharT *taskName,
        ClOsalSchedulePolicyT schedulePolicy,
        ClUint32T priority,
        ClUint32T stackSize,
        void* (*fpTaskFunction)(void*),
        void* pTaskFuncArgument,
        ClOsalTaskIdT* pTaskId);

/**
 **************************************** 
 * \brief Joins a task
 * \param taskId Identifier of the task to be joined. The taskId must be same as what
 *    was returned when the task was created. All other values will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. (Not applicable for deleting the
 *    currently executing task) 
 * \retval CL_ERR_NO_TASK_EXIST On deleting a task which does not exist. 
 * \retval CL_ERR_TASK_DELETE On failure to delete a task. 
 * 
 * \par Description
 * This API is used to join a task.  Joining a task means that the calling task
 * will wait until the specified task is completed.  Joining a task also cleans
 * 
 * \sa clOsalTaskCreate(), clOsalSelfTaskIdGet(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskJoin (ClOsalTaskIdT taskId);

/**
 ****************************************
 * \brief Deletes a task. 
 * 
 * \param taskId Identifier of the task to be deleted. The taskId must be same as what
 *    was returned when the task was created. All other values will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. (Not applicable for deleting the
 *    currently executing task) 
 * \retval CL_ERR_NO_TASK_EXIST On deleting a task which does not exist. 
 * \retval CL_ERR_TASK_DELETE On failure to delete a task. 
 * 
 * \par Description
 * This API is used to delete a task. 
 * 
 * \sa clOsalTaskCreate(), clOsalSelfTaskIdGet(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskDelete (ClOsalTaskIdT taskId);


/**
 ****************************************
 * \brief Kills a task  by sending a signal
 * 
 * \param taskId Identifier of the task to be killed. The taskId must be same as what
 *    was returned when the task was created. All other values will be
 *    invalid. 
 * \param sig Signal to be sent to the task
 * 
 * \retval CL_RC_OK This API executed successfully. (Not applicable for killing the
 *    currently executing task) 
 * \retval CL_ERR_NO_TASK_EXIST On killing a task which does not exist. 
 * \retval CL_ERR_TASK_DELETE On failure to killing a task. 
 * 
 * \par Description
 * This API is used to kill a task by sending a signal identified by 
 * the sig parameter.
 * 
 * \sa clOsalTaskCreate(), ClOsalTaskDelete(), clOsalSelfTaskIdGet(), 
 */

ClRcT clOsalTaskKill (ClOsalTaskIdT taskId, ClInt32T sig);


/**
 ****************************************
 * \brief No longer interested in the task's alive/dead state 
 * 
 * \param taskId Identifier of the task to be detached. The taskId must be same as
 *    what was returned when the task was created. All other values will
 *    be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. (Not applicable for deleting the
 *    currently executing task) 
 * \retval CL_ERR_NO_TASK_EXIST On deleting a task which does not exist. 
 * \retval CL_ERR_TASK_DELETE On failure to delete a task. 
 * 
 * \par Description
 * This API is used to tell the platform that this task should run
 *    independently of all other tasks. Its return code, or state will not
 *    be accessible by the program. 
 * 
 * \sa clOsalTaskCreate(), clOsalSelfTaskIdGet(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskDetach (ClOsalTaskIdT taskId);



/**
 ****************************************
 * \brief Retrieves task id. 
 * 
 * \param pTaskId (out) Task ID of the calling task is stored here. This must be a
 *    valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * 
 * \par Description
 * This API is used to obtain the task Id of the calling task. 
 * 
 * \sa clOsalTaskCreate(), clOsalTaskDelete(), clOsalTaskNameGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */
ClRcT clOsalSelfTaskIdGet (ClOsalTaskIdT* pTaskId);




/**
 ****************************************
 * \brief Retrieves task name. 
 * 
 * \param taskId Task ID of the task for the which the name is to be found. If the
 *    task ID is zero then the name of the current task is found. 
 * \param ppTaskName (out) Name of the task is stored here. This must be a valid pointer
 *    and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_NO_TASK_EXIST On requesting for a task name which does not exist. 
 * 
 * \par Description
 * This API is used to obtain the name of the task. 
 * 
 * \sa clOsalTaskCreate(), clOsalTaskDelete(), clOsalSelfTaskIdGet(),
 *    clOsalTaskPriorityGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */
ClRcT clOsalTaskNameGet (ClOsalTaskIdT taskId, ClCharT** ppTaskName);




/**
 ****************************************
 * \brief Retrieves the priority of the task. 
 * 
 * \param taskId Task ID of the task for which the priority is to obtained. The task
 *    ID must be same as the one that was returned when the task was
 *    created. Any other value will be invalid. 
 * \param pTaskPriority (out) Priority of the task is stored here. This must be a valid
 *    pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_TASK_ATTRIBUTE_GET On failure in retrieving the priority of the task. 
 * 
 * \par Description
 * This API is used to obtain the priority of a task. 
 * 
 * \sa clOsalTaskCreate(), clOsalTaskDelete(), clOsalSelfTaskIdGet(),
 *    clOsalTaskNameGet(), clOsalTaskPrioritySet(), clOsalTaskDelay() 
 */
ClRcT clOsalTaskPriorityGet (ClOsalTaskIdT taskId,
        ClUint32T* pTaskPriority);




/**
 ****************************************
 * \brief Sets the priority of the task. 
 * 
 * \param taskId Task ID of the task for which the priority is to be set. The task ID
 *    must be same as the one that was returned when the task was created.
 *    Any other value will be invalid. 
 * \param pTaskPriority New priority of the task. This must be between 1 and 160. All other
 *    values are invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_TASK_ATTRIBUTE_SET On failure in setting the priority of the task. 
 * 
 * \par Description
 * This API is used to set the priority of a task. 
 * 
 * \sa clOsalTaskCreate(), clOsalTaskDelete(), clOsalSelfTaskIdGet(),
 *    clOsalTaskNameGet(), clOsalTaskPriorityGet(), clOsalTaskDelay() 
 */

ClRcT clOsalTaskPrioritySet (ClOsalTaskIdT taskId,
        ClUint32T taskPriority);




/**
 ****************************************
 * \brief Delays a task. 
 * 
 * \param time Time duration for which the task is delayed. This value is in
 *    milliseconds. Any positive value will be valid. 
 * 
 * \retval CL_RC_OK The API executed successfully. 
 * \retval CL_ERR_TASK_DELAY On failure in delaying a task. 
 * 
 * \par Description
 * This API causes the calling task to relinquish the CPU for the
 *    duration specified. For certain OSes like Linux the taks may not get
 *    scheduled back after the timedelay mentioned. It may be scheduled at
 *    a later time by the scheduler based on the scheduling time interval,
 *    also it can be interrupted by signals received by the process and
 *    could return before the delay time period. 
 * 
 * \sa clOsalTaskCreate(), clOsalTaskDelete(), clOsalSelfTaskIdGet(),
 *    clOsalTaskNameGet(), clOsalTaskPriorityGet(),
 *    clOsalTaskPrioritySet() 
 */
ClRcT clOsalTaskDelay (ClTimerTimeOutT timeOut);


/**
 ****************************************
 * \brief Retrieves the current time. 
 * 
 * \param pTime Pointer to variable of type ClTimerTimeOutT, in which the time is
 *    returned. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_TIME_OF_DAY On failure in retrieving the time of day. 
 * 
 * \par Description
 * This API is used to return the current time (number of seconds and
 *    milliseconds) since the Epoch. 
 */
ClRcT clOsalTimeOfDayGet (ClTimerTimeOutT* pTime);

/**
 ************************************
 *  \brief Returns the time since Epoch, with a best resolution of 1 nanosecond.  Actual resolution is hardware dependent.
 *
 *  \param pTime Current time
 *
 *  \retval CL_OK  This API executed successfully.
 *  \retval CL_OSAL_ERR_OS_ERROR The OS abstraction layer has not been
 *  initialized. In other words, clOsalInitialize() has not been called.
 *  \retval CL_ERR_NULL_PTR 'pTime' is NULL 
 *  \retval CL_ERR_UNSPECIFIED An unknown error has occured in fetching the
 *  current time. This will not be returned under normal circumstances.
 *
 *  \sa
 *  clOsalTimeOfDayGet()
 *
 */
ClRcT clOsalNanoTimeGet (ClNanoTimeT* pTime);

/**
 ****************************************
 * \brief Retrieves the time since the machine is up. 
 * 
 * \param pTime Pointer to variable of type ClTimerTimeOutT, in which the time is
 *    returned. 
 * 
 * \retval -1 if there is some system error.
 * \retval on success returns the time in microseconds.
 * 
 * \par Description
 * This API is used to return the current time, since the machine is up, in microseconds. 
 */
ClTimeT clOsalStopWatchTimeGet(void);


/* Mutex */


/**
 ****************************************
 * \brief Initializes a mutex. 
 * 
 * \param pMutex (out) Mutex initialization data is stored here. This must be a valid
 *    pointer. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_MUTEX_CREATE On failure in initializing a Mutex. 
 * 
 * \par Description
 * This API is used to initialize a mutual exclusion semaphore.  It differs from
 * clOsalMutexCreate in that "Create" allocates memory while this function uses
 * the ClOsalMutexT object's memory.  To clean up use clOsalMutexDestroy() NOT
 *  clOsalMutexDelete() 
 * 
 * \sa clOsalMutexCreate(), clOsalRecursiveMutexInit(),
 *    clOsalMutexCreateAndLock(), clOsalMutexDestroy() 
 */
#ifndef __KERNEL__

#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexInit (ClOsalMutexT* pMutex);

#else

extern ClRcT clOsalMutexInitDebug(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
#define clOsalMutexInit(mutex) clOsalMutexInitDebug(mutex, __FILE__, __LINE__)

#endif

ClRcT clOsalMutexErrorCheckInit (ClOsalMutexT* pMutex);

ClRcT clOsalMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value);

ClRcT clOsalMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue);

#endif


/**
 ****************************************
 * \brief Initializes a process-shared mutex.
 * 
 * \param pMutex (out) Mutex initialization data is stored here. This must be a valid
 *    pointer to shared memory.
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_MUTEX_CREATE On failure in initializing a Mutex. 
 * 
 * \par Description
 * This API is used to initialize a mutual exclusion semaphore that can be
 * used to synchronize threads running in different processes
 * 
 * \sa clOsalMutexInit(), clOsalRecursiveMutexInit(), clOsalMutexDelete() 
 */
#ifndef __KERNEL__
ClRcT clOsalProcessSharedMutexInit (ClOsalMutexT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);

ClRcT clOsalSharedMutexCreate (ClOsalMutexIdT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);
#endif

#define clOsalMutexIdGet(pClOsalMutexT) ((ClOsalMutexIdT)(pClOsalMutexT))

/**
 ****************************************
 * \brief Initializes a mutex. 
 * 
 * \param pMutex (out) Mutex initialization data is stored here. This must be a valid
 *    pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_MUTEX_CREATE On failure in initializing a Mutex. 
 * 
 * \par Description
 * This API is used to initialize a mutual exclusion semaphore. The
 *    mutex is recursive -- i.e. same thread can 'lock' it multiple times,
 *    so long as it 'unlocks' it the same # of times. 
 * 
 * \sa clOsalMutexCreate(), clOsalRecursiveMutexInit(),
 *    clOsalMutexCreateAndLock(), clOsalMutexDelete() 
 */
#ifndef __KERNEL__

#ifndef CL_OSAL_DEBUG

ClRcT clOsalRecursiveMutexInit (ClOsalMutexT* pMutex);

#else

extern ClRcT clOsalRecursiveMutexInitDebug(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
#define clOsalRecursiveMutexInit(mutex) clOsalRecursiveMutexInitDebug(mutex, __FILE__, __LINE__)

#endif

#endif

/**
 ****************************************
 * \brief Creates a mutex. 
 * 
 * \param pMutexId (out) Identifier of the mutex created is stored here. This must be a
 *    valid pointer (cannot be NULL). 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_NO_MEM On memory allocation failure. 
 * \retval CL_ERR_MUTEX_CREATE On failure in creating a Mutex. 
 * 
 * \par Description
 * This API is used to create and initialize a mutual exclusion
 *    semaphore. 
 * 
 * \sa clOsalMutexCreateAndLock(), clOsalMutexDelete() 
 */
#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexCreate (ClOsalMutexIdT* pMutexId);

#else

extern ClRcT clOsalMutexCreateDebug(ClOsalMutexIdT *pMutexId, const ClCharT *file, ClInt32T line);
#define clOsalMutexCreate(mutexId) clOsalMutexCreateDebug(mutexId, __FILE__, __LINE__)

#endif

ClRcT clOsalMutexErrorCheckCreate (ClOsalMutexIdT* pMutexId);


/**
 ****************************************
 * \brief Creates a mutex in locked state. 
 * 
 * \param pMutexId (out) Identifier of the mutex created is stored here. This must be a
 *    valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_NO_MEM On memory allocation failure. 
 * \retval CL_ERR_MUTEX_CREATE On failure in creating a mutex. 
 * \retval CL_ERR_MUTEX_LOCK On failure in locking a mutex. 
 * 
 * \par Description
 * This API is used to create and initialize a mutual exclusion
 *    semaphore in locked state. 
 * 
 * \sa clOsalMutexCreate(), clOsalMutexDelete() 
 */
#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexCreateAndLock (ClOsalMutexIdT* pMutexId);

#else

extern ClRcT clOsalMutexCreateAndLockDebug(ClOsalMutexIdT *pMutexId, const ClCharT *file, ClInt32T line);
#define clOsalMutexCreateAndLock(mutexId) clOsalMutexCreateAndLockDebug(mutexId, __FILE__, __LINE__)

#endif

/**
 ****************************************
 * \brief Locks a mutex. 
 * 
 * \param mutexId Identifier of the mutex to be locked. The mutex id must be same as
 *    one returned when the mutex was created. All other values will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_MUTEX_LOCK On failure in locking a mutex. 
 * 
 * \par Description
 * This API is used to lock a mutex. 
 * 
 * \sa clOsalMutexUnlock 
 */
#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexLock (ClOsalMutexIdT mutexId);

#else

extern ClRcT clOsalMutexLockDebug(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
#define clOsalMutexLock(mutexId) clOsalMutexLockDebug(mutexId, __FILE__, __LINE__)

#endif

ClRcT clOsalMutexLockSilent (ClOsalMutexIdT mutexId);

ClRcT
clOsalMutexUnlockNonDebug(ClOsalMutexIdT mutexId);

ClRcT
clOsalMutexTryLock(ClOsalMutexIdT mutexId);

/**
 ****************************************
 * \brief Unlocks a mutex. 
 * 
 * \param mutexId Identifier of the mutex to be unlocked. The mutex ID must be same as
 *    one returned when the mutex was created. All other values will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_MUTEX_UNLOCK On failure in unlocking a mutex. 
 * 
 * \par Description
 * This API is used to unlock a mutex. 
 * 
 * \sa clOsalMutexLock() 
 */
#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexUnlock (ClOsalMutexIdT mutexId);

#else

extern ClRcT clOsalMutexUnlockDebug(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
#define clOsalMutexUnlock(mutexId) clOsalMutexUnlockDebug(mutexId, __FILE__, __LINE__)

#endif

ClRcT clOsalMutexUnlockSilent (ClOsalMutexIdT mutexId);

/**
 ****************************************
 * \brief Deletes a mutex. 
 * 
 * \param mutexId Identifier of the mutex to be deleted. The mutex id must be same as
 *    one returned when the mutex was created. All other values will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_MUTEX_DELETE On failure in deleting a mutex. 
 * 
 * \par Description
 * This API is used to delete a mutex. 
 * 
 * \sa clOsalMutexCreateAndLock(), clOsalMutexCreate() 
 */
#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexDelete (ClOsalMutexIdT mutexId);

#else

extern ClRcT clOsalMutexDeleteDebug(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
#define clOsalMutexDelete(mutexId) clOsalMutexDeleteDebug(mutexId, __FILE__, __LINE__)

#endif

/**
 ****************************************
 * \brief Destroys a mutex. 
 * 
 * \param mutexId Mutex to be destroyed. The mutex must be same as one returned when
 *    the mutex was initialized. All other values will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_MUTEX_DELETE On failure in destroying the mutex. 
 * 
 * \par Description
 * This API is used to destroy a mutex. 
 * 
 * \sa clOsalMutexInit(), clOsalMutexCreateAndLock(), clOsalMutexCreate() 
 */

#ifndef __KERNEL__

#ifndef CL_OSAL_DEBUG

ClRcT clOsalMutexDestroy (ClOsalMutexT *pMutex);

#else

extern ClRcT clOsalMutexDestroyDebug(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
#define clOsalMutexDestroy(mutex) clOsalMutexDestroyDebug(mutex, __FILE__, __LINE__)

extern ClRcT clOsalBustLocks(void);

#endif
#endif

#ifndef __KERNEL__
/**
 ****************************************
 * \brief Initializes a condition variable. 
 * 
 * \param pCond (out) Identifier of the condition variable initialized is stored
 *    here. This must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_CONDITION_INIT On failure in creating a condition variable. 
 * 
 * \par Description
 * This API is used to initialize a condition variable. 
 * Use clOsalCondDelete() to delete it, NOT clOsalCondDestroy()
 * 
 * \sa clOsalCondDelete(), clOsalCondWait(), clOsalCondBroadcast(), clOsalCondSignal() 
 */
ClRcT clOsalCondInit (ClOsalCondT* pCond);
#endif

#ifndef __KERNEL__
/**
 ****************************************
 * \brief Initializes a condition variable that can be used in multiple processes.
 * 
 * \param pCond (out) Identifier of the condition variable initialized is stored
 *    here. This must be a valid pointer to shared memory.
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_CONDITION_INIT On failure in creating a condition variable. 
 * 
 * \par Description
 * This API is used to initialize a condition variable. 
 * Use clOsalCondDestroy() to delete it, NOT clOsalCondDelete()
 * 
 * \sa clOsalCondDestroy(), clOsalCondWait(), clOsalCondBroadcast(), clOsalCondSignal() 
 */
ClRcT clOsalProcessSharedCondInit (ClOsalCondT* pCond);

#endif

/**
 ****************************************
 * \brief Creates a condition variable. 
 * 
 * \param pConditionId (out) Identifier of the condition variable created is stored here.
 *    This must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_NO_MEM On memory allocation failure. 
 * \retval CL_ERR_CONDITION_CREATE On failure in creating a condition variable. 
 * 
 * \par Description
 * This API is used to create a condition variable.  The condition variable is 
 * allocated on the heap.  Use clOsalCondDelete() to destroy it. 
 * 
 * \sa clOsalCondDelete(), clOsalCondWait(), clOsalCondBroadcast(),
 *    clOsalCondSignal() 
 */
ClRcT clOsalCondCreate (ClOsalCondIdT* pConditionId);



/**
 ****************************************
 * \brief Deletes a condition variable. 
 * 
 * \param conditionId Identifier of the condition variable to be deleted. No task must be
 *    waiting on this condition variable when delete is invoked. This must
 *    be the same as one returned when the condition variable was created.
 *    Any other value will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_CONDITION_TIMEDOUT On condition timedout 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_CONDITION_DELETE On failure in deleting the condition variable. 
 * 
 * \par Description
 * This API is used to delete a condition variable, and free associated memory.
 * It is paired with clOsalCondCreate()
 * \sa clOsalCondCreate(), clOsalCondWait(), clOsalCondBroadcast(),
 *    clOsalCondSignal() 
 */
ClRcT clOsalCondDelete (ClOsalCondIdT conditionId);


/**
 ****************************************
 * \brief Destroys a condition variable. 
 * 
 * \param conditionId Identifier of the condition variable to be destroyed. No task must be
 *    waiting on this condition variable when destroy is invoked. This
 *    must be the same as one returned when the condition variable was
 *    initialized. Any other value will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_CONDITION_TIMEDOUT On condition timedout 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_CONDITION_DELETE On failure in deleting the condition variable. 
 * 
 * \par Description
 * This API is used to delete a condition variable. It is paired with
 * clOsalCondInit(), NOT clOsalCondCreate().
 * 
 * \sa clOsalCondInit(), clOsalCondWait(), clOsalCondBroadcast(),
 *    clOsalCondSignal() 
 */
#ifndef __KERNEL__
ClRcT clOsalCondDestroy (ClOsalCondT *pCond);
#endif


/**
 ****************************************
 * \brief Waits for a condition. 
 * 
 * \param conditionId Identifier of the condition variable. This must be the same as one
 *    returned when the condition variable was created. Any other value
 *    can will be invalid. 
 * \param mutexId Identifier of the mutex. This must be the same as one returned when
 *    the mutex was created. Any other value will be invalid. 
 * \param time Duration for which the task waits for a condition. The time structure
 *    must be filled in with valid values. If the time out is zero then
 *    the task waits indefinitely for the signal. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_CONDITION_WAIT On failure to wait for a condition. 
 * 
 * \par Description
 * This API is used to unlock a mutex and wait on a condition variable
 *    to be signalled as the task execution is suspended. The mutex must
 *    be locked before entering into wait. \arg If the time out value is
 *    specified then the task would wait for the specified time. \arg If
 *    the condition is not signalled within the specified time then an
 *    error is returned. \arg If the timeout specified is zero then the
 *    task waits indefinitely until it receives a signal. 
 * 
 * \sa clOsalCondCreate(), clOsalCondDelete(), clOsalCondBroadcast(),
 *    clOsalCondSignal() 
 */
#ifndef CL_OSAL_DEBUG
ClRcT clOsalCondWait (ClOsalCondIdT conditionId,
        ClOsalMutexIdT mutexId,
        ClTimerTimeOutT time);
#else
extern ClRcT clOsalCondWaitDebug (ClOsalCondIdT conditionId,
                                  ClOsalMutexIdT mutexId,
                                  ClTimerTimeOutT time,
                                  const ClCharT *file, ClInt32T line);
#define clOsalCondWait(cond, mutex, time) clOsalCondWaitDebug(cond, mutex, time, __FILE__, __LINE__)
#endif

/**
 ****************************************
 * \brief Broadcasts a condition. 
 * 
 * \param conditionId Identifier of the condition variable. This must be the same as one
 *    returned when the condition variable was created. Any other value
 *    will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_CONDITION_BROADCAST On failure to broadcast the condition. 
 * 
 * \par Description
 * This API is used to resume all the tasks that are waiting on the
 *    condition variable. If no tasks are waiting on that condition then
 *    nothing happens. 
 * 
 * \sa clOsalCondCreate(), clOsalCondDelete(), clOsalCondWait(),
 *    clOsalCondSignal() 
 */
ClRcT clOsalCondBroadcast (ClOsalCondIdT conditionId);

/**
 ****************************************
 * \brief Signals a condition. 
 * 
 * \param conditionId Identifier of the condition variable. This must be the same as one
 *    returned when the condition variable was created. Any other value
 *    will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_CONDITION_SIGNAL On failure to signal a condition. 
 * 
 * \par Description
 * This API is used to resume one of the tasks that are waiting on the
 *    condition variable. If no tasks are waiting on that condition then
 *    nothing happens. If there are several threads waiting on the
 *    condition then exactly one task is resumed. 
 * 
 * \sa clOsalCondCreate(), clOsalCondDelete(), clOsalCondWait(),
 *    clOsalCondBroadcast() 
 */
ClRcT clOsalCondSignal (ClOsalCondIdT conditionId);


/**
 ****************************************
 * \brief Creates a key for thread-specific data. 
 * 
 * \param pKey (out) Pointer to ClUint32T, in which the created key is returned. 
 * \param pCallbackFunc Callback function, which is called when the key is deleted using
 *    clOsalTaskKeyDelete API. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_OS_ERROR On failure to create a key for thread-specific data. 
 * 
 * \par Description
 * This API is used to create a key for thread-specific data. 
 * 
 * \sa clOsalTaskKeyDelete(), clOsalTaskDataSet(), clOsalTaskDataGet() 
 */
ClRcT clOsalTaskKeyCreate(ClUint32T* pKey,
        ClOsalTaskKeyDeleteCallBackT pCallbackFunc);




/**
 ****************************************
 * \brief Deletes the key for thread-specific data. 
 * 
 * \param key Key to be deleted. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_OS_ERROR On failure in deleting the key for thread-specific data. 
 * 
 * \par Description
 * This API is used to delete the key for thread-specific data, which is
 *    returned by clOsalTaskKeyCreate. 
 * 
 * \sa clOsalTaskKeyCreate(), clOsalTaskDataSet(), clOsalTaskDataGet() 
 */
ClRcT clOsalTaskKeyDelete(ClUint32T key);


/**
 ****************************************
 * \brief Sets the thread-specific data. 
 * 
 * \param key Key for the thread-specific data returned by clOsalTaskKeyCreate API. 
 * \param threadData ThreadData to be assigned to the calling task. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_OS_ERROR On failure to set thread-specific data. 
 * 
 * \par Description
 * This API is used to set the thread-specific data for the calling
 *    task. 
 * 
 * \sa clOsalTaskKeyCreate(), clOsalTaskKeyDelete(), clOsalTaskDataGet() 
 */
ClRcT clOsalTaskDataSet(ClUint32T key,
        ClOsalTaskDataT threadData);



/**
 ****************************************
 * \brief Retrieves the thread-specific data. 
 * 
 * \param key Key for the thread-specific data returned by clOsalTaskKeyCreate API. 
 * \param pThreadData (out) Pointer to ClUint32T in which the thread-specific data is
 *    returned on success. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_OS_ERROR On failure in retrieving the thread-specific data. 
 * 
 * \par Description
 * This API is used to return the thread-specific data of the calling
 *    task. 
 * 
 * \sa clOsalTaskKeyCreate(), clOsalTaskKeyDelete(), clOsalTaskDataSet() 
 */
ClRcT clOsalTaskDataGet(ClUint32T key,
        ClOsalTaskDataT* pThreadData);



/**
 ****************************************
 * \brief Prints to the standard output. 
 * 
 * \param fmt Format string to be printed. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_OS_ERROR On failure to print to standard output. 
 * 
 * \par Description
 * This API is used to print to the standard output. 
 */
#ifndef __KERNEL__
ClRcT clOsalPrintf(const ClCharT* fmt, ...);
#endif /* __KERNEL__ */

/* Semaphores */
/**
 ****************************************
 * \brief Creates a semaphore. 
 * 
 * \param pName Name of the semaphore to be created. If the same name is specified
 *    then the same semaphore ID is returned. Name must not be more than
 *    20 characters and cannot be NULL. 
 * \param value Value of the semaphore. This is required to set the semaphore before
 *    it is used first time. This must be a positive integer and must be
 *    less than \c CL_SEM_MAX_VALUE. Zero will be invalid. 
 * \param pSemId (out) Identifier of the semaphore created is stored in this location.
 *    This must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_CREATE On failure in creating a semaphore. 
 * \retval CL_ERR_NAME_TOO_LONG If semaphore name is too long. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * 
 * \par Description
 * This API is used to create an counting semaphore. If the semaphore
 *    with the same name exists then the exisiting semaphore is obtained
 *    else a new semaphore is created. 
 * 
 * \sa clOsalSemIdGet(), clOsalSemLock(), clOsalSemTryLock(),
 *    clOsalSemUnlock(), clOsalSemValueGet(), clOsalSemDelete() 
 */
ClRcT clOsalSemCreate(ClUint8T* pName,
        ClUint32T value,
        ClOsalSemIdT* pSemId);


/**
 ****************************************
 * \brief Retrieves the semaphore id. 
 * 
 * \param pName Name of the semaphore for the which the ID is required.NULL is not
 *    valid. 
 * \param pSemId (out) A memory location to store the ID.
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SEM_ID_GET On failure in retrieving the semaphore ID. 
 * 
 * \par Description
 * This API is used to obtain the ID of the semaphore if the name is
 *    specified. 
 * 
 * \sa clOsalSemCreate(),clOsalSemLock(), clOsalSemTryLock(),
 *    clOsalSemUnlock(), clOsalSemValueGet(), clOsalSemDelete() 
 */
ClRcT clOsalSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId);


/**
 ****************************************
 * \brief Locks a semaphore. 
 * 
 * \param semId Identifier of the semaphore to be locked. This must be the same as
 *    what was returned when the semaphore was created. Any other value
 *    will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_LOCK On failure in locking a semaphore. 
 * 
 * \par Description
 * This API is used to lock a semaphore. If the semaphore is already
 *    locked (unavailable) then trying to lock the same semaphore suspends
 *    the execution of the process until the semaphore is available. The
 *    value of the semaphore is decremented. 
 * 
 * \sa clOsalSemCreate(), clOsalSemIdGet(), clOsalSemTryLock(),
 *    clOsalSemUnlock(), clOsalSemValueGet(), clOsalSemDelete() 
 */
ClRcT clOsalSemLock(ClOsalSemIdT semId);


/**
 ****************************************
 * \brief Locks a semaphore if it is available. 
 * 
 * \param semId Identifier of the semaphore to be locked. This must be the same as
 *    one returned when the semaphore was created. Any other value will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_LOCK On failure in try-locking a semaphore. 
 * 
 * \par Description
 * This API is used to lock a semaphore if the semaphore is available.
 *    If the semaphore is unavailable then the call just returns without
 *    blocking. The call returns immediately (non-blocking) on both the
 *    cases. If the semaphore is available,then semaphore is locked and
 *    will remain locked until it is unlocked explicitly. The value of the
 *    semaphore is decremented. 
 * 
 * \sa clOsalSemCreate(), clOsalSemIdGet(), clOsalSemLock(),
 *    clOsalSemUnlock(), clOsalSemValueGet(), clOsalSemDelete() 
 */
ClRcT clOsalSemTryLock(ClOsalSemIdT semId);



/**
 ****************************************
 * \brief Unlocks a semaphore. 
 * 
 * \param semId Identifier of the semaphore to be locked. This must be the same as
 *    one returned when the semaphore was created. Any other value will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_UNLOCK On failure in unlocking a semaphore. 
 * 
 * \par Description
 * This API is used to unlock a semaphore.Any process that is blocked on
 *    the semaphore resumes execution. If no process is blocked then the
 *    semaphore is made available for the others. The value of the
 *    semaphore is incremented. 
 * 
 * \sa clOsalSemCreate(), clOsalSemIdGet(), clOsalSemLock(),
 *    clOsalSemTryLock(), clOsalSemValueGet(), clOsalSemDelete() 
 */
ClRcT clOsalSemUnlock(ClOsalSemIdT semId);



/**
 ****************************************
 * \brief Retrieves the value of a semaphore. 
 * 
 * \param semId Identifier of the semaphore to be locked. This must be the same as
 *    one returned when the semaphore was created. Any other value will be
 *    invalid. 
 * \param pSemValue (out) Value of the semaphore is stored in the location specified.
 *    This must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_GET_VALUE On failure in retrieving the value of the semaphore. 
 * 
 * \par Description
 * This API is used to return the value of a semaphore. 
 * 
 * \sa clOsalSemCreate(), clOsalSemIdGet(), clOsalSemLock(),
 *    clOsalSemTryLock(), clOsalSemUnlock(), clOsalSemDelete() 
 */
ClRcT clOsalSemValueGet(ClOsalSemIdT semId,
        ClUint32T* pSemValue);



/**
 ****************************************
 * \brief Deletes a semaphore. 
 * 
 * \param semId Identifier of the semaphore to be locked. This must be the same as
 *    one returned when the semaphore was created. Any other value will be
 *    invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SEM_DELETE On failure in deleting a semaphore. 
 * 
 * \par Description
 * This API is used to destroy a semaphore. No process must be waiting
 *    for the semaphore while the semaphore is being destroyed. If one or
 *    more processes are waiting on the semaphore then the semaphore does
 *    not get destroyed. 
 * 
 * \sa clOsalSemCreate(), clOsalSemIdGet(), clOsalSemLock(),
 *    clOsalSemTryLock(), clOsalSemUnlock(), clOsalSemValueGet(), 
 */
ClRcT clOsalSemDelete(ClOsalSemIdT semId);


/* Process */


/**
 ****************************************
 * \brief Creates a process. 
 * 
 * \param fpFunction Function to be invoked when the process is created. This must be a
 *    valid pointer and cannot be NULL. 
 * \param functionArg Argument passed to the \e fpFunction that is created along with the
 *    cretionof the the process. This can be a valid pointer or NULL. 
 * \param creationFlags Flags that control certain properties including creation of new
 *    session, process group and creating the process in a suspended
 *    state. 
 * \param pProcessId (out) Id of the process created is stored in this location. This must
 *    be a valid pointer cand cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_PROCESS_CREATE On failure to create a process. 
 * 
 * \par Description
 * This API is used to create a process. 
 * 
 * \sa clOsalProcessDelete(), clOsalProcessWait(), clOsalProcessSelfIdGet() 
 */
ClRcT clOsalProcessCreate(ClOsalProcessFuncT fpFunction,
        void* functionArg,
        ClOsalProcessFlagT creationFlags,
        ClOsalPidT* pProcessId);

/**
 ****************************************
 * \brief Deletes a process. 
 * 
 * \param processId Id of the process to be deleted. This must be the same as one
 *    returned when the process was created. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_PROCESS_DELETE On failure in deleting a process. 
 * 
 * \par Description
 * This API is used to delete a process. 
 * 
 * \sa clOsalProcessCreate(), clOsalProcessWait(), clOsalProcessSelfIdGet() 
 */
ClRcT clOsalProcessDelete(ClOsalPidT processId);


/**
 ****************************************
 * \brief Waits for a process to exit. 
 * 
 * \param processId Id of the process for which the calling process needs to wait. This
 *    must be the same as one returned when the process was created. If
 *    zero is passed then the calling process waits for all the processes,
 *    that it created, to exit. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_PROCESS_WAIT On failure to wait for a process to exit. 
 * 
 * \par Description
 * This API is used to suspend the execution of a process until the the
 *    processes created from with in the process has exited. 
 * 
 * \sa clOsalProcessCreate(), clOsalProcessDelete(),
 *    clOsalProcessSelfIdGet() 
 */
ClRcT clOsalProcessWait(ClOsalPidT processId);



/**
 ****************************************
 * \brief Retrieves the processId. 
 * 
 * \param pProcessId (out) ProcessId of the caller is stored in this location. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * 
 * \par Description
 * This API is used to obtain the processId of the caller. 
 * 
 * \sa clOsalProcessCreate(), clOsalProcessDelete(), clOsalProcessWait(), 
 */
ClRcT clOsalProcessSelfIdGet(ClOsalPidT* pProcessId);


/* Shared Memory */


/**
 ****************************************
 * \brief Creates a shared memory. 
 * 
 * \param pName Name for the shared memory region to be created. The same name must
 *    be specified if the same shared memory is to be used from some other
 *    process. This must be a valid pointer and cannot be NULL. Name must
 *    not be more than 20 characters. 
 * \param size Size of the shared memory to be created. This must be a positive
 *    integer. 
 * \param pShmId Identifier of the shared memory created is stored in this location.
 *    This must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_NAME_TOO_LONG If the name specified is too long. 
 * \retval CL_ERR_SHM_ID_GET On failure to retrieve the ID of the shared memory. 
 * \retval CL_ERR_SHM_CREATE On failure to create a shared memory. 
 * 
 * \par Description
 * This API is used to create a shared memory region. If the shared
 *    memory region with the same name exists then the existing shared
 *    memory is obtained, else a new shared memory region is created. 
 * 
 * \sa clOsalShmIdGet(), clOsalShmDelete(), clOsalShmAttach(),
 *    clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), clOsalShmSizeGet() 
 */
ClRcT clOsalShmCreate(ClUint8T* pName,
            ClUint32T       size,
            ClOsalShmIdT    *pShmId);



/**
 ****************************************
 * \brief Retrieves a shared memory Id. 
 * 
 * \param pName Name of the shared memory region for which the Id is to be obtained.
 *    This must be a valid pointer and cannot be NULL. 
 * \param pShmId Id of the shared memory is copied into this location. NULL will be
 *    invalid. 
 * 
 * \retval CL_RC_OK The API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SHM_ID_GET On failure in obtaining the shared memory ID. 
 * 
 * \par Description
 * This API is used to obtain the shared memory Id. 
 * 
 * \sa clOsalShmCreate(), clOsalShmDelete(), clOsalShmAttach(),
 *    clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), clOsalShmSizeGet() 
 */
ClRcT clOsalShmIdGet(ClUint8T    *pName,
        ClOsalShmIdT    *pShmId);




/**
 ****************************************
 * \brief Deletes a shared memory. 
 * 
 * \param shmId Id of the shared memory to be deleted. This must be the same as the
 *    one that was passed when the shared memory was created. Any other
 *    value will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SHM_DELETE On failure in deleting the shared memory. 
 * 
 * \par Description
 * This API is used to delete a shared memory region. If more than one
 *    process has attached this shared memory then the destroy would just
 *    mark it for destroy. The shared memory is actually destroyed only
 *    after the last detach. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(), clOsalShmAttach(),
 *    clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), clOsalShmSizeGet() 
 */
ClRcT clOsalShmDelete(ClOsalShmIdT shmId);



/**
 ****************************************
 * \brief Attaches a shared memory. 
 * 
 * \param shmId Identifier of the shared memory region to be attached. This must be
 *    same as one returned the shared memory was created. All other values
 *    will be invalid. 
 * \param pInMem Pointer to the shared memory region to be attached. 
 *   \arg NULL 
 *   \arg address 
 * \param ppOutMem (out) Attached shared memory region is copied to this location. This
 *    must be a valid pointer and cannot be NULL. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SHM_ATTACH On failure in attaching the shared memory. 
 * 
 * \par Description
 * This API is used to attach a shared memory region. The attach occurs
 *    at the address equal to the address specified by you rounded to the
 *    nearest multiple of the page size. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(),
 *    clOsalShmDelete(),clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), clOsalShmSizeGet() 
 */
ClRcT clOsalShmAttach(ClOsalShmIdT shmId,
        void* pInMem,
        void** ppOutMem);




/**
 ****************************************
 * \brief Detaches a shared memory. 
 * 
 * \param pMem Pointer to the shared memory region to be detached. The memory must
 *    have been attached to the process already. NULL will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SHM_DETACH On failure in detaching shared memory. 
 * 
 * \par Description
 * This API is used to detach a shared memory region from the address
 *    space of the calling process. The memory segment to be detached must
 *    have been attached to the process. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(), clOsalShmDelete(),
 *    clOsalShmAttach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), clOsalShmSizeGet() 
 */
ClRcT clOsalShmDetach(void* pMem);




/**
 ****************************************
 * \brief Sets permissions to shared memory. 
 * 
 * \param shmId Identifier of the shared memory region. This must be same as one
 *    returned when the shared memory was created. All other values will
 *    be invalid. 
 * \param mode Permission to be set. This can be any of the flags specified in \e
 *    ClOsalShmSecurityModeFlagT. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_SHM_MODE_SET On failure in setting permissions to shared memory. 
 * 
 * \par Description
 * This API is used to set permissions for a shared memory region.This
 *    allows only the access modes to be changed. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(), clOsalShmDelete(),
 *    clOsalShmAttach(), clOsalShmDetach(), clOsalShmSecurityModeGet(),
 *    clOsalShmSizeGet() 
 */
ClRcT clOsalShmSecurityModeSet(ClOsalShmIdT shmId,
        ClUint32T mode);


/**
 ****************************************
 * \brief Retrieves permissions of shared memory. 
 * 
 * \param shmId Identifier of the shared memory region. This must be same as one
 *    returned when the shared memory was created. All other values will
 *    be invalid. 
 * \param pMode (out) Permission of the shared memory will be copied to this
 *    location. NULL will be invalid. 
 * 
 * \retval CL_RC_OK This API executed successfully. 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SHM_MODE_GET On failure in retrieving permissions of the shared memory. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(), clOsalShmDelete(),
 *    clOsalShmAttach(), clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSizeGet() 
 */
ClRcT clOsalShmSecurityModeGet(ClOsalShmIdT shmId,
        ClUint32T* pMode);



/**
 ****************************************
 * \brief Retrieves the size of shared memory. 
 * 
 * \param shmId Identifier of the shared memory region. This must be same as one
 *    returned when the shared memory was created. All other values will
 *    be invalid. 
 * \param pSize Size of the shared memory is copied into the specified location. 
 * 
 * \retval CL_RC_OK This API executed successfully.: 
 * \retval CL_ERR_INVLD_PARAM On passing an invalid parameter. 
 * \retval CL_ERR_NULL_PTR On passing a NULL pointer. 
 * \retval CL_ERR_SHM_SIZE On failure in retrieving size of the shared memory. 
 * 
 * \par Description
 * This API is used to obtain the size of the shared memory that was
 *    allocated. 
 * 
 * \sa clOsalShmCreate(), clOsalShmIdGet(), clOsalShmDelete(),
 *    clOsalShmAttach(), clOsalShmDetach(), clOsalShmSecurityModeSet(),
 *    clOsalShmSecurityModeGet(), 
 */
ClRcT clOsalShmSizeGet(ClOsalShmIdT shmId,
        ClUint32T* pSize);

#if 0 /* Deprecated R4.0.  Use the standard C library routines */
/**
 ****************************************
 * \brief Closes a file 
 * 
 * \param fd Pointer to the file descriptor (fd is zeroed upon successful close to help avoid bugs)
 * 
 * \retval CL_RC_OK             This API executed successfully.: 
 * \retval CL_OSAL_ERR_OS_ERROR Unrecoverable operating system level error (view the logs for more information)
 *
 * \par Description
 * Closes a file, just like the standard close() function.
 * 
 * \sa clOsalFileOpen()
 */
ClRcT clOsalFileClose (ClFdT *fd);
#endif


ClRcT clOsalMmap(ClPtrT start, ClUint32T length, ClInt32T prot, ClInt32T flags,
        ClHandleT fd, ClHandleT offset, ClPtrT *mmapped);


ClRcT clOsalMunmap(ClPtrT start, ClUint32T length);


ClRcT clOsalMsync (ClPtrT start, ClUint32T length, ClInt32T flags);


ClRcT clOsalFtruncate (ClFdT fd, off_t length);

ClRcT clOsalShmOpen (const ClCharT *name, ClInt32T oflag, ClUint32T mode, ClFdT *fd);
ClRcT clOsalShmClose (ClFdT *fd);
    
ClRcT clOsalShmUnlink (const ClCharT *name);

/**
 ****************************************
 * \brief Installs and initializes the signal handler. 
 * initialized. In other words, clOsalInitialize() has not been called.
 * \retval CL_ERR_NULL_POINTER 'name' is NULL
 * \retval CL_ERR_OP_NOT_PERMITTED Permission denied to unlink the POSIX shared
 * memory object.
 * \retval CL_ERR_DOESNT_EXIST  A POSIX shared memory object by name 'name'
 * does not exist
 * \retval CL_ERR_UNSPECIFIED: An unknown error was returned by pathconf()
 */

void clOsalSigHandlerInitialize(void);


/*
 *  \brief Returns the maximum allowed length of a relative pathname when 'path' is the working directory.
 *
 *  \param path: Relative or absolute path of a directory
 *
 *  \param pLength: The returned length
 *
 *  \retval CL_OK:  This API executed successfully.
 *  \retval CL_OSAL_ERR_OS_ERROR: The OS abstraction layer has not been
 *  initialized. In other words, clOsalInitialize() has not been called.
 *  \retval CL_ERR_NULL_PTR: Either 'path' or 'pLength' is NULL 
 *  \retval CL_ERR_NOT_SUPPORTED: This limit cannot be queried at run-time.
 *  \retval CL_ERR_DOESNT_EXIST: There is no definite limit.
 *  \retval CL_ERR_UNSPECIFIED: An unknown error was returned by pathconf()
 *
 *  \par Description:
 *  This function is a wrapper for pathconf().
 *
 */
ClRcT clOsalMaxPathGet(const ClCharT* path, ClInt32T* pLength);

/**
 ************************************
 *
 *  \par Synopsis:
 *  Returns the size of a page in bytes
 *
 *  \par Header File:
 *  clOsalApi.h
 *  clOsalErrors.h
 *
 *  \par Library File:
 *  libClOsal.a
 *  libClOsal.so
 *
 *  \param pSize The returned length
 *
 *  \retval CL_OK  This API executed successfully.
 *  \retval CL_OSAL_ERR_OS_ERROR The OS abstraction layer has not been
 *  initialized. In other words, clOsalInitialize() has not been called.
 *  \retval CL_ERR_NULL_PTR 'pSize' is NULL 
 *  \retval CL_ERR_NOT_SUPPORTED This limit cannot be queried at run-time.
 *  \retval CL_ERR_UNSPECIFIED An unknown error was returned by sysconf()
 *
 *  \par Description:
 *  None
 *
 *  \par Related APIs:
 *  None
 *
 */
ClRcT clOsalPageSizeGet(ClInt32T* pSize);


/* Deprecated functions */

void clOsalErrorReportHandler(void *info) CL_DEPRECATED;

ClPtrT clOsalMalloc(ClUint32T size) CL_DEPRECATED;

ClPtrT clOsalCalloc(ClUint32T size) CL_DEPRECATED;

void clOsalFree(ClPtrT pAddress) CL_DEPRECATED;

#ifndef __KERNEL__
 
ClRcT clOsalMutexAttrInit(ClOsalMutexAttrT *pAttr) CL_DEPRECATED;

ClRcT clOsalMutexAttrDestroy(ClOsalMutexAttrT *pAttr) CL_DEPRECATED;

ClRcT clOsalMutexAttrPSharedSet(ClOsalMutexAttrT *pAttr, ClOsalSharedTypeT type) CL_DEPRECATED;

ClRcT clOsalMutexInitEx(ClOsalMutexT *pMutex, ClOsalMutexAttrT *pAttr) CL_DEPRECATED;


ClRcT clOsalCondInitEx(ClOsalCondT *pCond, ClOsalCondAttrT *pAttr) CL_DEPRECATED;

ClRcT clOsalCondAttrInit(ClOsalCondAttrT *pAttr) CL_DEPRECATED;

ClRcT clOsalCondAttrDestroy(ClOsalCondAttrT *pAttr) CL_DEPRECATED;

ClRcT clOsalCondAttrPSharedSet(ClOsalCondAttrT *pAttr, ClOsalSharedTypeT type) CL_DEPRECATED;
#endif



#ifdef __cplusplus
}
#endif


/**
 * \}
 */


#endif /* _CL_OSAL_API_H_ */


