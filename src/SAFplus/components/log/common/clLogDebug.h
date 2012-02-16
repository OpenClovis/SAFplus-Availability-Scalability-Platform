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
/******************************************************************************
 * Description :
 * This file contains the debug macros we are defining for the log. Major
 * functionality we are adding:
 * - We can read the log level at the start time
 * We also define macros
 * - For locking/unlocking
 * - For check in/check out
 *
 * TODO/FIXME:
 * - put pthread_self and getpid under OSAL wrapper
 *****************************************************************************/

#ifndef _CL_LOG_DEBUG_H_
#define _CL_LOG_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <clCommon.h>
#include <clDebugApi.h>

/*
 * Defining the macros with different levels:
 */
#if defined(CL_DEBUG) && defined(CL_DEBUG_START)

extern void
clLogDebugLevelSet(void);

extern ClUint32T  clLogDebugLevel;

#define this_error_indicates_missing_parans_around_string(...) __VA_ARGS__
#define CL_DEBUG_VERBOSE           (1 << 5)

#define CL_LOG_DEBUG_CRITICAL(arg) CL_LOG_DEBUG_MSG(CL_DEBUG_CRITICAL, arg)
#define CL_LOG_DEBUG_ERROR(arg)    CL_LOG_DEBUG_MSG(CL_DEBUG_ERROR, arg)
#define CL_LOG_DEBUG_WARN(arg)     CL_LOG_DEBUG_MSG(CL_DEBUG_WARN, arg)
#define CL_LOG_DEBUG_INFO(arg)     CL_LOG_DEBUG_MSG(CL_DEBUG_INFO, arg)
#define CL_LOG_DEBUG_TRACE(arg)    CL_LOG_DEBUG_MSG(CL_DEBUG_TRACE, arg)
#define CL_LOG_DEBUG_VERBOSE(arg)  CL_LOG_DEBUG_MSG(CL_DEBUG_VERBOSE, arg)

#define CL_LOG_DEBUG_MSG(levelSpecified, arg)       \
do{                                                 \
    if( clLogDebugLevel >= levelSpecified )         \
    {                                               \
        clLogDeferred(levelSpecified, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, this_error_indicates_missing_parans_around_string arg);   } \
}while(0)

#else

#define CL_LOG_DEBUG_CRITICAL(arg)
#define CL_LOG_DEBUG_ERROR(arg)
#define CL_LOG_DEBUG_WARN(arg)
#define CL_LOG_DEBUG_INFO(arg)
#define CL_LOG_DEBUG_TRACE(arg)
#define CL_LOG_DEBUG_VERBOSE(arg)

#endif

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_DEBUG_H_*/
