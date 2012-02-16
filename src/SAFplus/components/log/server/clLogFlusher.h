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
#ifndef _CL_LOG_FLUSHER_H
#define _CL_LOG_FLUSHER_H
#ifdef __cplusplus
extern "C" {
#endif
    
#include <clLogCommon.h>

#define  CL_LOG_FLUSH_SEND_RETRIES  3    

typedef struct
{
    ClUint32T       numRecords;
    ClTimerHandleT  hTimer;
}ClLogFlushCookieT;
extern void*
clLogFlusherStart(CL_IN  void *pData);

extern ClRcT
clLogFlusherCookieHandleDestroy(ClHandleT  hFlusher, 
                                ClBoolT    timerExpired);

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_FLUSHER_H*/
