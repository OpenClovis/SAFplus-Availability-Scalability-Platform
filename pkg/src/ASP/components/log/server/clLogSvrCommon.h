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
#ifndef _CL_LOG_SVR_COMMON_H_
#define _CL_LOG_SVR_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <xdrClLogStreamAttrIDLT.h>    
#include <clCkptApi.h>    
#include <clIdlApi.h>
#include <clLogCommon.h>
#include <clEventApi.h>
    
#define CL_LOG_SERVER_COMMON_EO_ENTRY_KEY   1
#define CL_LOG_INVALID_DSID                 0
#define CL_LOG_DSID_START                   1
#define CL_LOG_SVR_DEFAULT_TIMEOUT          5000
#define CL_LOG_SVR_DEFAULT_RETRY            0
#define CL_LOG_MAX_PRECREATED_STREAMS       32

#define CL_LOG_AREA_SVR                     "SVR"    
#define CL_LOG_AREA_FILE_OWNER              "FOW"    
#define CL_LOG_AREA_STREAM_OWNER            "SOW"
#define CL_LOG_AREA_MASTER                  "MAS"
    
#define CL_LOG_CTX_CKPT_READ                "CPR"
#define CL_LOG_CTX_BOOTUP                "BOO"
#define CL_LOG_CTX_SVR_INIT                "SVI"
#define CL_LOG_CTX_FO_INIT                  "FOI"
#define CL_LOG_CTX_SO_INIT                  "SOI"    
#define CL_LOG_CTX_SO_FILTERSET             "SFS"    
#define CL_LOG_CTX_MAS_INIT                 "MOI"    

#define  CL_LOG_DEFAULT_COMPID              0xFFFFFFFE

extern const ClVersionT  gCkptVersion;
extern const ClVersionT  gLogVersion;
     
typedef struct
{
    ClIocNodeAddressT      masterAddr; 
    ClIocNodeAddressT      deputyAddr;
    ClCkptSvcHdlT          hSvrCkpt;
    ClCkptSvcHdlT          hLibCkpt;
    ClUint32T              maxStreams;
    ClUint32T              maxComponents;
    ClUint32T              maxMsgs;
    ClUint32T              numShmPages;
    ClEventHandleT         hEvtFileCreated;
    ClEventHandleT         hEvtFileClosed;
    ClEventHandleT         hEvtFileUnitFull;
    ClEventHandleT         hEvtFileHWMarkCrossed;
    ClEventHandleT         hStreamCreationEvt;
    ClEventHandleT         hStreamCloseEvt;
    ClEventHandleT         hCompAddEvt;
    ClEventInitHandleT     hEventInitHandle;
    ClEventChannelHandleT  hLogEvtChannel;
} ClLogSvrCommonEoDataT;

typedef struct
{
    ClNameT              streamName;
    ClLogStreamScopeT    streamScope;
    ClLogStreamAttrIDLT  streamAttr;
} ClLogStreamDataT;


extern ClRcT 
clLogSvrMutexModeSet(void);

extern ClRcT
clLogSvrCommonDataInit();

extern ClRcT
clLogSvrCommonDataFinalize(void);

extern ClRcT
clLogSvrCkptFinalize(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry);

extern ClRcT
clLogAttributesMatch(CL_IN ClLogStreamAttrIDLT  *pStoredAttr,
                     CL_IN ClLogStreamAttrIDLT  *pPassedAttr,
                     CL_IN ClBoolT              compareFilePath);

extern ClRcT
clLogStreamAttributesCopy(ClLogStreamAttrIDLT  *pStoredAttr,
                          ClLogStreamAttrIDLT  *pPassedAttr, 
                          ClBoolT              copyFilePath);
extern ClRcT
clLogIdlHandleInitialize(ClIocAddressT  destAddr, 
                         ClIdlHandleT   *phLogIdl);

extern ClRcT
clLogServerSerialiser(ClUint32T  dsId,
                      ClAddrT    *pBuffer,
                      ClUint32T  *pSize,
                      ClPtrT     cookie);
extern ClRcT
clLogAddressForLocationGet(ClCharT        *pStr, 
                           ClIocAddressT  *pDestAddr,
                           ClBoolT        *pIsLogical);

extern ClRcT 
clLogPrecreatedStreamsDataGet(ClLogStreamDataT     *pStreamAttr[],
                              ClUint32T            *pNumStreams);
extern ClIocNodeAddressT 
clLogFileOwnerAddressFetch(ClStringT  *pFileLocation);

extern ClRcT
clLogFileOwnerStreamCreateEvent(ClNameT              *pStreamName,
                                ClLogStreamScopeT    streamScope,
                                ClNameT              *pStreamScopeNode,
                                ClUint16T            streamId,
                                ClLogStreamAttrIDLT  *pStreamAttr,
                                ClBoolT              doHandlerRegister);

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_SVR_COMMON_H_*/
