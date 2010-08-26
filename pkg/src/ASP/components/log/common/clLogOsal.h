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
#ifndef _CL_LOG_OSAL_H_
#define _CL_LOG_OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/mman.h>
#include <pthread.h>
#include <clTimerApi.h>
#include <clOsalApi.h>

typedef enum
{
    CL_OSAL_PROCESS_PRIVATE_L = PTHREAD_PROCESS_PRIVATE, /*default*/
    CL_OSAL_PROCESS_SHARED_L = PTHREAD_PROCESS_SHARED
}ClOsalSharedType_LT;

typedef ClOsalMutexT          ClOsalMutexId_LT;
typedef ClOsalCondT           ClOsalCondId_LT;
typedef ClOsalMutexAttrT      ClOsalMutexAttr_LT;
typedef ClOsalCondAttrT       ClOsalCondAttr_LT;

#define  clOsalMutexLock_L(arg)    clOsalMutexLock((ClOsalMutexIdT)  (arg))
#define  clOsalMutexUnlock_L(arg)  clOsalMutexUnlock((ClOsalMutexIdT) (arg))
#define  clOsalMutexValueSet_L(arg, v) clOsalMutexValueSet( (ClOsalMutexIdT)(arg), (v))
#define  clOsalMutexValueGet_L(arg, v) clOsalMutexValueGet((ClOsalMutexIdT)(arg), (v))
#define clOsalShmOpen_L        clOsalShmOpen

#define clOsalShmClose_L(arg)  clOsalShmClose(&(arg))

#define clOsalShmUnlink_L      clOsalShmUnlink

#define clOsalMmap_L           clOsalMmap

#define clOsalMunmap_L         clOsalMunmap

#define clOsalMsync_L          clOsalMsync

#define clOsalPageSizeGet_L    clOsalPageSizeGet

#define clOsalCondSignal_L     clOsalCondSignal 

#define clOsalMutexInitEx_L    clOsalMutexInitEx

#define clOsalMutexInit_L      clOsalMutexInit

#define clOsalMutexDestroy_L   clOsalMutexDestroy

#define clOsalFtruncate_L      clOsalFtruncate

#define clOsalMutexAttrInit_L        clOsalMutexAttrInit

#define clOsalMutexAttrPSharedSet_L  clOsalMutexAttrPSharedSet

#define clOsalMutexAttrDestroy_L     clOsalMutexAttrDestroy

#define clOsalMutexTryLock_L         clOsalMutexTryLock

#define clOsalCondAttrInit_L         clOsalCondAttrInit

#define clOsalCondAttrPSharedSet_L   clOsalCondAttrPSharedSet

#define clOsalCondAttrDestroy_L      clOsalCondAttrDestroy

#define clOsalCondInitEx_L           clOsalCondInitEx

#define clOsalCondInit_L             clOsalCondInit

#define clOsalCondInit_L             clOsalCondInit

#define clOsalCondWait_L             clOsalCondWait

#define clOsalCondBroadcast_L        clOsalCondBroadcast

#define clOsalCondDestroy_L          clOsalCondDestroy

ClRcT
clOsalNanoTimeGet_L(ClTimeT  *pTime);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_OSAL_H_ */
