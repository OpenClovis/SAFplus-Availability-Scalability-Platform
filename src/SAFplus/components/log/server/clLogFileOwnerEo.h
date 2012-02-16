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
#ifndef _CL_LOG_STREAM_HDLR_EO_H_
#define _CL_LOG_STREAM_HDLR_EO_H_  

#ifdef __cplusplus
extern "C" {
#endif

#include <clHandleApi.h>
#include <clCntApi.h>
#include <clLogApi.h>
#include <clLogCommon.h>    

#define  CL_LOG_STREAM_HDLR_EO_ENTRY_KEY  5    

typedef enum
{
    CL_LOG_FILEOWNER_STATE_INACTIVE = 1,
    CL_LOG_FILEOWNER_STATE_ACTIVE   = 2,                                   
    CL_LOG_FILEOWNER_STATE_CLOSED   = 3,
} ClLogFileOwnerStateT;

typedef struct
{
    ClHandleDatabaseHandleT  hStreamDB;
    ClOsalMutexId_LT         fileTableLock;
    ClCntHandleT             hFileTable;
    ClLogHandleT             hLog;
    ClNameT                  nodeName;
    ClLogFileOwnerStateT     status;
    ClUint32T                activeCnt;
    ClBoolT                  terminate;
}ClLogFileOwnerEoDataT;
    
extern ClRcT
clLogFileOwnerBootup(ClBoolT  logRestart);

extern ClRcT
clLogFileOwnerShutdown(void);

extern ClRcT
clLogFileOwnerEoDataFinalize(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry);

extern ClRcT
clLogFileOwnerEoDataFree(void);

extern ClRcT
clLogFileOwnerEoEntryGet(ClLogFileOwnerEoDataT **ppFileOwnerEoEntry);

extern ClRcT
clLogFileOwnerEoEntryUpdate(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry);

extern ClRcT
clLogFileOwnerEoDataInit(ClLogFileOwnerEoDataT  **ppFileOwnerEoEntry);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_COMMON_H_ */
