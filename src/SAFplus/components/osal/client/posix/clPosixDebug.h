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
 * File        : clPosixDebug.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/

#ifndef _CL_POSIX_DEBUG_H_
#define _CL_POSIX_DEBUG_H_

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include "clOsalCommon.h"
#include <clList.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void cosPosixMutexPoolAdd(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
extern ClRcT cosPosixMutexPoolLock(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
extern ClRcT cosPosixMutexPoolLockSoft(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
extern void cosPosixMutexPoolUnlock(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
extern void cosPosixMutexPoolDelete(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
extern void cosPosixMutexPoolBustLocks(void);

#ifdef __cplusplus
}
#endif

#endif
