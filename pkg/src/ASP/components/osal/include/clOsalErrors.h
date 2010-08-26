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
 * ModuleName  : osal
 * File        : clOsalErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Error codes returned by Clovis OS Abstraction Layer
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Error codes returned by Clovis OS Abstraction Layer
 *  \ingroup osal_apis
 */

/**
 *  \addtogroup osal_apis
 *  \{
 */

#ifndef _CL_OSAL_ERRORS_H_
#define _CL_OSAL_ERRORS_H_
  
#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

   
#define CL_OSAL_AREA "OSA"


/***************************************************************************** 
 * ERROR CODES RETURNED BY OSAL LAYER 
 *****************************************************************************/
/**
 * Error returned when the osal library is not initialized 
 */
#define CL_OSAL_ERR_OS_ERROR                 0x100

/**
 * Error returned when creation of mutex fails. 
 */
#define CL_OSAL_ERR_CREATE_MUTEX             0x101

/**
 * Error returned when initialization of task attribute fails. 
 */
#define CL_OSAL_ERR_TASK_ATTRIBUTE_INIT      0x102

/**
 * Error returned when creation of task fails.
 */
#define CL_OSAL_ERR_TASK_CREATE              0x103

/**
 * Error returned when deletion of task fails.
 */
#define CL_OSAL_ERR_TASK_DELETE              0x104

/**
 * Error returned when task attribute cannot be set. 
 */
#define CL_OSAL_ERR_TASK_ATTRIBUTE_SET       0x105

/**
 * Error returned when delaying a task fails.
 */
#define CL_OSAL_ERR_TASK_DELAY               0x106

/**
 * Error returned when creation of mutex fails. 
 */
#define CL_OSAL_ERR_MUTEX_CREATE             0x107

/**
 * Error returned when locking of mutex fails. 
 */
#define CL_OSAL_ERR_MUTEX_LOCK               0x108

/**
 * Error returned when unlocking of mutex fails. 
 */
#define CL_OSAL_ERR_MUTEX_UNLOCK             0x109

/**
 * Error returned when deletion of mutex fails. 
 */
#define CL_OSAL_ERR_MUTEX_DELETE             0x10a

/**
 * Error returned when initializing condition variable fails.
 */
#define CL_OSAL_ERR_CONDITION_CREATE         0x10b

/**
 *  Error returned when destroying condition variable fails.
 */
#define CL_OSAL_ERR_CONDITION_DELETE         0x10c

/**
 * Error returned when waiting on a condition variable fails.
 */
#define CL_OSAL_ERR_CONDITION_WAIT           0x10d

/**
 * Error returned when unable to restart all threads waiting on a condition
 * variable.
 */
#define CL_OSAL_ERR_CONDITION_BROADCAST      0x10e

/**
 * Error returned when unable to restart a thread waiting on a condition
 * variable 
 */
#define CL_OSAL_ERR_CONDITION_SIGNAL         0x10f

/**
 * Not used 
 */
#define CL_OSAL_ERR_SCHEDULE_POLICY          0x110

/**
 * Error returned when finalization of osal library fails. 
 */
#define CL_OSAL_ERR_COS_CLEANUP              0x111

/**
 * Error returned when task attribute cannot be retrieved. 
 */
#define CL_OSAL_ERR_TASK_ATTRIBUTE_GET       0x112

/**
 * Error returned when the task referred to does not exist. 
 */
#define CL_OSAL_ERR_NO_TASK_EXIST            0x113

/**
 * Error returned when stack size of thread creation attribute cannot be set
 */
#define CL_OSAL_ERR_TASK_STACK_SIZE          0x114

/**
 * Error returned when time of day cannot be obtained
 */
#define CL_OSAL_ERR_TIME_OF_DAY              0x115

/**
 * Error returned when creation of semaphore fails. 
 */
#define CL_OSAL_ERR_SEM_CREATE               0x116

/**
 * Error returned when the semaphore ID cannot be retrieved. 
 */
#define CL_OSAL_ERR_SEM_ID_GET               0x117

/**
 * Error returned when locking of a semaphore fails. 
 */
#define CL_OSAL_ERR_SEM_LOCK                 0x118

/**
 * Error returned when unlocking of a semaphore fails. 
 */
#define CL_OSAL_ERR_SEM_UNLOCK               0x119

/**
 * Error returned when the value of semaphore cannot be obtained.
 */
#define CL_OSAL_ERR_SEM_GET_VALUE            0x11a

/**
 * Error returned when deletion of a semaphore fails.
 */
#define CL_OSAL_ERR_SEM_DELETE               0x11b

/**
 * Error returned when process creation fails. 
 */
#define CL_OSAL_ERR_PROCESS_CREATE           0x11c

/**
 * Error returned when deletion fails. 
 */
#define CL_OSAL_ERR_PROCESS_DELETE           0x11d

/**
 * Error returned when waiting on a child process fails.
 */
#define CL_OSAL_ERR_PROCESS_WAIT             0x11e

/**
 * Error returned when creation of shared memory fails. 
 */
#define CL_OSAL_ERR_SHM_CREATE               0x11f

/**
 * Error returned when the ID of the shared memory cannot be retrieved. 
 */
#define CL_OSAL_ERR_SHM_ID_GET               0x120

/**
 * Error returned when deletion of shared memory fails. 
 */
#define CL_OSAL_ERR_SHM_DELETE               0x121

/**
 * On failure in attaching a shared memory. 
 */
#define CL_OSAL_ERR_SHM_ATTACH               0x122

/**
 * On failure in detaching a shared memory. 
 */
#define CL_OSAL_ERR_SHM_DETACH               0x123

/**
 * On failure in setting permissions to a shared memory. 
 */
#define CL_OSAL_ERR_SHM_MODE_SET             0x124

/**
 * On failure in retrieving permissions of a shared memory. 
 */
#define CL_OSAL_ERR_SHM_MODE_GET             0x125

/**
 * On failure in retrieving size of a shared memory. 
 */
#define CL_OSAL_ERR_SHM_SIZE                 0x126

/**
 * Error returned when initialization of osal library failed.
 */
#define CL_OSAL_ERR_COS_INIT                 0x127

/**
 * Error returned when creation of memory pool fails. 
 */
#define CL_OSAL_ERR_MEM_POOL_CREATE          0x128

/**
 * Error returned when detaching of memory pool fails. 
 */
#define CL_OSAL_ERR_MEM_POOL_DETACH          0x129

/**
 * Error returned when deleting of memory pool fails. 
 */
#define CL_OSAL_ERR_MEM_POOL_DELETE          0x130

/**
 * Error returned when the name given in semaphore creation is greater than 20 
 */
#define CL_OSAL_ERR_NAME_TOO_LONG            0x131

/**
 * Error returned when the thread is not signalled within a time period
 * specified
 */
#define CL_OSAL_ERR_CONDITION_TIMEDOUT       0x132


/******************************************************************************
 * ERROR/RETURN CODE HANDLING MACROS
 *****************************************************************************/
#define CL_OSAL_RC(ERROR_CODE)  CL_RC(CL_CID_OSAL, (ERROR_CODE))


#ifdef __cplusplus
}
#endif

#endif /* _CL_OSAL_ERRORS_H_ */
    

/** \} */
