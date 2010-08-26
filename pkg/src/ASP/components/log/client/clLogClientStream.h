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
#ifndef _CL_LOG_CLIENT_STREAM_H_
#define _CL_LOG_CLIENT_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

#include <clCntApi.h>
#include <clBitmapApi.h>
#include <clLogCommon.h>

struct ClLogClntStreamData
{
    ClLogStreamHeaderT  *pStreamHeader;
    ClUint8T            *pStreamRecords;
    ClStringT           shmName;
    ClBitmapHandleT     hStreamBitmap;
#ifdef VXWORKS_BUILD
    /*
     * cache the named semaphore ids since only named semaphores with '/' prefixed names have RTP shared access
     */
    ClOsalSemIdT        sharedSemId; 
    ClOsalSemIdT        flusherSemId; 
#endif
};

typedef struct ClLogClntStreamData ClLogClntStreamDataT;

ClRcT
clLogClntStreamOpen(ClLogHandleT        hLog,
                    ClNameT             *pStreamName,
                    ClNameT             *pNodeName,
                    ClStringT           *pShmName,
                    ClUint32T           shmSize,
                    ClLogStreamHandleT  *phStream);
void
clLogClntStreamDeleteCb(ClCntKeyHandleT   userKey,
                        ClCntDataHandleT  userData);
ClRcT
clLogClntStreamDataCleanup(ClLogClntStreamDataT *pData);

ClRcT
clLogClntStreamEntryGet(ClLogClntEoDataT  *pClntEoEntry,
                        ClNameT           *pStreamName,
                        ClNameT           *pNodeName,
                        ClStringT         *pShmName,
                        ClUint32T         shmSize,
                        ClCntNodeHandleT  *phStreamNode,
                        ClBoolT           *pAddedEntry);
ClRcT
clLogClntStreamEntryAdd(ClCntHandleT       hClntTable,
                        ClLogStreamKeyT    *pStreamKey,
                        ClStringT          *pShmName,
                        ClUint32T          shmSize,
                        ClCntNodeHandleT   *phStreamNode);
ClRcT
clLogClntStreamWrite(ClLogClntEoDataT    *pClntEoEntry,
                     ClLogSeverityT      severity,
                     ClUint16T           serviceId,
                     ClUint16T           msgId,
                     va_list             args,
                     ClCntNodeHandleT    hClntStreamNode);

ClRcT
clLogClntStreamWriteWithHeader(ClLogClntEoDataT    *pClntEoEntry,
                               ClLogSeverityT      severity,
                               ClUint16T           serviceId,
                               ClUint16T           msgId,
                               ClCharT             *pMsgHeader,
                               va_list             args,
                               ClCntNodeHandleT    hClntStreamNode);

ClRcT
clLogClientMsgWrite(ClLogSeverityT     severity,
                    ClUint16T          streamId,
                    ClUint16T          serviceId,
                    ClUint16T          msgId,
                    ClUint32T          compId,
                    va_list            args,
                    ClUint32T          recSize,
                    ClUint8T           *pRecord);

ClRcT
clLogClientMsgWriteWithHeader(ClLogSeverityT     severity,
                              ClUint16T          streamId,
                              ClUint16T          serviceId,
                              ClUint16T          msgId,
                              ClUint32T          compId,
                              ClUint64T          sequenceNum,
                              ClCharT            *pMsgHeader,
                              va_list            args,
                              ClUint32T          recSize,
                              ClUint8T           *pRecord);

ClRcT
clLogClntStreamClose(ClCntNodeHandleT    hClntStreamNode,
                     ClLogStreamHandleT  hStream,
                     ClBoolT             notifySvr);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_CLIENT_STREAM_H_ */
