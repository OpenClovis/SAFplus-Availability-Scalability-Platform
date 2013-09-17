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
#ifndef _CL_LOG_STREAMOWNER_CKPT_H_
#define _CL_LOG_STREAMOWNER_CKPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCkptApi.h>
#include <clLogStreamOwnerEo.h>
#include <clLogStreamOwner.h>
#include <clLogSvrCommon.h>    

#define  CL_LOG_SO_SEC_SIZE          2 * sizeof(SaNameT) + \
                                     sizeof(ClLogStreamOwnerDataT)  
#define  CL_LOG_SO_SEC_ID_SIZE       sizeof(SaNameT) + sizeof(soSecPrefix) 
/*#define  CL_LOG_CKPT_RETENTION_TIME  (1000L * 1000L * 1000L * 60) */
#define  CL_LOG_MICROS_IN_SEC        (1000L * 1000L)  
                                        /* 5 minutes */    

extern ClRcT
clLogSOLocalCkptGet(CL_IN ClLogSOEoDataT *pSoEoEntry);

extern ClRcT
clLogStreamOwnerLocalCkptCreate(CL_IN ClCkptSvcHdlT  hLibInit); 

extern ClRcT
clLogStreamOwnerGlobalCheckpointRead(void);

extern ClRcT
clLogStreamOwnerLocalCkptDelete(void);

extern ClRcT
clLogStreamOwnerGlobalCkptGet(ClLogSOEoDataT         *pSoEoEntry,
                              ClLogSvrCommonEoDataT  *pSvrCommonEoData, 
                              ClBoolT                *pCreateCkpt);

extern ClRcT
clLogStreamOwnerCkptRead(CL_IN ClHandleT       hLibCkpt, 
                         CL_IN ClLogSOEoDataT  *pSoEoEntry);

extern ClRcT
clLogDsIdMapRecreate(CL_IN ClBitmapHandleT  hDsIdMap,
                     CL_IN ClUint32T        dsId,
                     CL_IN ClPtrT           pCookie);
extern ClRcT 
clLogSOStreamEntryRecreate(ClUint32T  dsId,
                           ClAddrT    pBuffer,
                           ClUint32T  size,
                           ClPtrT     cookie);

extern ClRcT
clLogStreamOwnerLocalCheckpoint(CL_IN ClLogSOEoDataT         *pSoEoData,
                                CL_IN SaNameT                *pStreamName,
                                CL_IN SaNameT                *pStreamScope,
                                CL_IN ClLogStreamOwnerDataT  *pStreamOwnerData);
extern ClRcT
clLogStreamOwnerEntryPack(CL_IN SaNameT                *pStreamName,
                          CL_IN SaNameT                *pStreamScopeNode,
                          CL_IN ClLogStreamOwnerDataT  *pStreamOwnerData,
                          CL_IN ClBufferHandleT        msg);
extern ClRcT
clLogCompTablePack(CL_IN ClCntKeyHandleT   key,
                   CL_IN ClCntDataHandleT  data,
                   CL_IN ClCntArgHandleT   arg,
                   CL_IN ClUint32T         size);
extern ClRcT
clLogStreamOwnerLocalRecreate(ClHandleT       hLibCkpt, 
                              ClLogSOEoDataT  *pSoEoEntry);

extern ClRcT
clLogStreamOwnerCheckpointOpen(ClLogSOEoDataT  *pSoEoEntry,
                               SaNameT         *pCkptName,
                               ClHandleT       *phCkpt);

extern ClRcT
clLogStreamOwnerCheckpoint(CL_IN ClLogSOEoDataT     *pSoEoEntry,
                           CL_IN ClLogStreamScopeT  streamScope,
                           CL_IN ClCntNodeHandleT   hStreamOwnerNode,
                           CL_IN ClLogStreamKeyT    *pStreamKey);
extern ClRcT
clLogStreamOwnerGlobalCheckpoint(CL_IN ClLogSOEoDataT         *pSoEoEntry,
                                 CL_IN SaNameT                *pStreamName,
                                 CL_IN SaNameT                *pStreamScopeNode,
                                 CL_IN ClLogStreamOwnerDataT  *pStreamOwnerData);

extern ClRcT
clLogStreamOwnerCkptUnpack(CL_IN ClLogSvrCommonEoDataT  *pCommonEoData, 
                           CL_IN ClBufferHandleT        msg);

extern ClRcT
clLogCompTableUnpack(CL_IN ClBufferHandleT        msg,
                     CL_IN ClLogStreamOwnerDataT  *pStreamOwnerData);

extern ClRcT
clLogStreamOwnerCkptDelete(CL_IN ClLogSOEoDataT    *pSoEoEntry, 
                           CL_IN ClCkptSectionIdT  *pSecId,
                           CL_IN ClUint32T         dsId);
extern ClRcT
clLogStreamOwnerSecRead(ClLogSOEoDataT            *pSoEoEntry,
                        ClCkptSectionDescriptorT  *pSecDescriptor);

extern ClRcT
clLogStreamOwnerGlobalRecreate(ClLogSOEoDataT  *pSoEoEntry);

extern ClRcT
clLogDsIdMapUnpack(ClBufferHandleT  msg,
                   ClLogSOEoDataT   *pSoEoEntry);

/* May be a common function */
extern ClRcT
clLogCheckpointOpen(CL_IN  ClLogSOEoDataT  *pSoEoEntry,
                    CL_IN  SaNameT         *pCkptName,
                    CL_OUT ClHandleT       *phCkpt);

extern ClRcT
clLogStreamOwnerCkptDsIdGet(CL_IN  ClLogSOEoDataT  *pSoEoEntry, 
                            CL_OUT ClUint32T       *pDsId);

extern ClRcT
clLogStreamOwnerCkptDsIdDelete(CL_IN ClLogSOEoDataT  *pSoEoEntry,
                               CL_IN ClUint32T       dsId) ;

extern ClRcT
clLogStreamOwnerSerialiser(CL_IN ClUint32T  dsId,
                           CL_IN ClAddrT    *pBuffer,
                           CL_IN ClUint32T  *pSize,
                           CL_IN ClPtrT     cookie);
extern ClRcT
clLogStreamOwnerDeserialiser(ClUint32T  dsId,
                             ClAddrT    pBuffer,
                             ClUint32T  size,
                             ClPtrT     cookie);
extern ClRcT
clLogSoDefaultSerializer(CL_IN ClUint32T  dsId,
                         CL_IN ClAddrT    *pBuffer,
                         CL_IN ClUint32T  *pSize,
                         CL_IN ClPtrT     cookie);

extern ClRcT
clLogStreamOwnerFilterInfoPack(ClLogStreamOwnerDataT  *pStreamOwnerData, 
                               ClBufferHandleT        msg);

extern ClRcT
clLogStreamOwnerFilterInfoUnpack(ClBufferHandleT        msg,
                                 ClLogStreamOwnerDataT  *pStreamOwnerData);
extern ClRcT
clLogBitmapUnpack(ClBufferHandleT  msg,
                  ClBitmapHandleT  hBitmap);

extern ClRcT
clLogBitmapPack(ClBitmapHandleT  hBitmap,
                ClBufferHandleT  msg);

extern ClRcT
clLogStreamOwnerEntryRecreate(ClLogSvrCommonEoDataT  *pCommonEoData, 
                              ClAddrT                pBuffer,
                              ClUint32T              size);
extern ClRcT
clLogStreamOwnerGlobalStateRecover(ClIocNodeAddressT  masterAddr, ClBoolT switchover);
#ifdef __cplusplus
}
#endif
#endif /*_CL_LOG_STREAMOWNER_CKPT_H_*/
