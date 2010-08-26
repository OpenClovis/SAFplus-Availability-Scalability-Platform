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
#ifndef _CL_LOG_FILE_EVT_H_
#define _CL_LOG_FILE_EVT_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <clCommon.h>
#include <clEventApi.h>
#include <clLogSvrCommon.h>    
#include <xdrClLogStreamInfoIDLT.h>    
    
/* Log Events associated filters */
#define CL_LOG_STREAM_CREATED                 0x1
#define CL_LOG_STREAM_CLOSED                  0x2    
#define CL_LOG_FILE_CREATED                   0x3
#define CL_LOG_FILE_CLOSED                    0x4    
#define CL_LOG_FILE_HIGH_WATERMARK_CROSSED    0x5
#define CL_LOG_FILE_UNIT_FULL                 0x6
#define CL_LOG_COMP_ADD                       0x7

typedef enum 
{
    CL_LOG_STREAMCREAT_EVT_SUBID = 1,
    CL_LOG_STREAMCLOSE_EVT_SUBID = 2,
    CL_LOG_NODEDOWN_EVT_SUBID = 3,
    CL_LOG_COMPDOWN_EVT_SUBID = 4,
    CL_LOG_COMPADD_EVT_SUBID = 5
}ClLogEvtSubcriberIdT;

typedef struct
{
    ClCharT  *fileName;
    ClCharT  *fileLocation;
} ClLogEvtFileInfoT;
    
extern ClRcT
clLogEventInitialize(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry);

extern ClRcT
clLogEvtFinalize(ClBoolT  terminatePath);
/* Functions for publishing events */
extern ClRcT
clLogFileCreationEvent(ClStringT * fileName,
                       ClStringT * fileLocation);
extern ClRcT
clLogFileClosureEvent(ClStringT * fileName,
                      ClStringT * fileLocation);
extern ClRcT
clLogFileUnitFullEvent(ClStringT * fileName,
                       ClStringT * fileLocation);
extern ClRcT
clLogHighWatermarkEvent(ClStringT * fileName,
                        ClStringT * fileLocation);
extern ClRcT    
clLogFilePublishEvent(ClStringT    *fileName,
                      ClStringT    *fileLocation,
                      ClUint32T  patternType);

extern ClRcT
clLogEventIntialize(ClLogSvrCommonEoDataT *ppSvrCommonEoEntry);

extern ClRcT
clLogEvtSubscribe(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry);

extern ClRcT
clLogCompDownSubscribe(ClEventInitHandleT  hEvtSvcInit, 
                       ClHandleT           *phCompChl);
extern ClRcT
clLogNodeDownSubscribe(ClEventInitHandleT  hEvtSvcInit,
                       ClEventHandleT      *phNodeChl);
extern ClRcT
clLogStreamCreationEventDataExtract(ClUint8T   *pBuffer,
                                    ClUint32T  size);
extern ClRcT
clLogStreamCloseEventDataExtract(ClUint8T   *pBuffer,
                                 ClUint32T  size);
extern ClRcT
clLogStreamCreationEvtPublish(ClLogStreamInfoIDLT  *pLogStreamInfo);

extern ClRcT
clLogStreamCloseEvtPublish(ClNameT            *pStreamName,
                           ClLogStreamScopeT  streamScope,
                           ClNameT            *pStreamScopeNode);
extern ClRcT
clLogFileOwnerEventSubscribe(void);

extern ClRcT
clLogCompAddEvtPublish(ClNameT    *pCompName,
                       ClUint32T  clientId);
extern ClRcT
clLogCompAddEventDataExtract(ClUint8T   *pBuffer,
                             ClUint32T  size);
extern ClRcT
clLogNodeDownMasterDBUpdate(ClNameT  nodeName);
    
#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_FILE_EVT_H_ */
    
