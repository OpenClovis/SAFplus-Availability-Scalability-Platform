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

#ifndef _CL_LOG_TST_H_
#define _CL_LOG_TST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clLogApi.h>   

typedef struct
{
    ClLogHandleT  hLog;
    ClVersionT    version;
    ClWaterMarkT  waterMark;
} ClTcLogParamsT;

typedef enum 
{
    CL_LOG_TST_BUFFER = 0,
    CL_LOG_TST_ASCII  = 1,
    CL_LOG_TST_TLV    = 2
}ClLogWriteTypeT;

typedef enum 
{
    CL_TST_LOG_LOCAL  = 1,
    CL_TST_LOG_MASTER = 2                      
} ClLogFileTypeT;

typedef struct
{
    ClLogStreamHandleT  hStream;
    ClTimeT             time_us; 
    ClTimeT             time_ms;
}ClLogTstDataT;

typedef struct
{
    ClUint32T             fileSize;
    ClUint32T             recordSize;
    ClLogFileFullActionT  fileAction;
    ClUint32T             maxFiles;

}ClLogOpenParamsT;

ClRcT
clTcLogSvcInit(ClLogHandleT *phLog);

ClRcT
clTcLogStreamOpen(ClHandleT         hLog,
                  ClCharT           *pStreamName,
                  ClLogStreamScopeT streamScope, 
                  ClCharT           *fileName,
                  ClCharT           *fileLocation,
                  ClLogOpenParamsT  *pOpenParams,
                  ClUint32T         flushFreq,
                  ClLogTstDataT     *pLogTestData);
ClRcT
clLogTestStreamAttributesInit(const ClCharT                 *fileName,
                              const ClCharT                 *fileLocation,
                              ClUint32T               fileUnitSize,
                              ClUint32T               recordSize,
                              ClLogFileFullActionT    fileFullAction,
                              ClUint32T               maxFilesRotated,
                              ClUint32T               flushFreq,
                              ClTimeT                 flushInterval,
                              ClWaterMarkT            waterMark,
                              ClLogStreamAttributesT  *pStreamAttr);
ClRcT
clTcLogStreamClose(ClLogTstDataT  *pTestData);

ClRcT
clTcLogSvcFinalize(ClHandleT hLog);

ClRcT
clTcLogWrite(ClLogTstDataT    *pLogTestData, 
             ClLogWriteTypeT  writeType,
             ClLogSeverityT   severity, 
             ClCharT          *pLogStr);
#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_CLIENT_H_*/
