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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgDebugInternal.h
 *******************************************************************************/

/*******************************************************************************
 * Description : This file contains macros and declarations that help debugging
 *               messaging server.
 *****************************************************************************/


#ifndef __CL_MSG_DEBUG_INTERNAL_H__
#define __CL_MSG_DEBUG_INTERNAL_H__

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>


#ifdef __cplusplus
extern "C" {
#endif



/* The following are to enalbe/dessable the assertion. */
#ifdef __MSG_ENABLE_ASSERTS__

#define CL_MSG_ASSERT(condition)    CL_ASSERT(condition)

#else

#define CL_MSG_ASSERT(condition) \
    do { \
        if(!(condtion)) \
            clLogEmergency("MSG", "ASSERT", "Assertion failed. Please report this error. \"assert(%s)\"", #condition); \
    } while(0)

#endif



/* The following are for debug the locking and unlocking of mutexs in messaging service. */
#ifdef __MSG_DEBUG_LOCKS__

#define CL_OSAL_MUTEX_LOCK(lock)    \
    do{ \
        clOsalMutexLock((lock)); \
        clOsalPrintf("MSG_DEBUG : [%s : %s() : %d]. LOCK [%s].\n", __FILE__, __FUNCTION__, __LINE__, #lock); \
    } while(0)
#define CL_OSAL_MUTEX_UNLOCK(lock)    \
    do{ \
        clOsalMutexUnlock((lock)); \
        clOsalPrintf("MSG_DEBUG : [%s : %s() : %d]. UNLOCK [%s].\n", __FILE__, __FUNCTION__, __LINE__, #lock); \
    } while(0)

#else

#define CL_OSAL_MUTEX_LOCK(lock)      clOsalMutexLock((lock))
#define CL_OSAL_MUTEX_UNLOCK(lock)    clOsalMutexUnlock((lock))

#endif

#ifdef __cplusplus
}
#endif


#endif
