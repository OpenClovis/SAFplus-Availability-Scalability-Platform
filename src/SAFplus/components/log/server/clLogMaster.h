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
#ifndef _CL_LOG_MASTER_H_
#define _CL_LOG_MASTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogCommon.h>
#include <clCntApi.h>
#include <xdrClLogStreamAttrIDLT.h>

typedef struct
{
    ClStringT  fileName;
    ClStringT  fileLocation;
    ClUint32T  hash;
} ClLogFileKeyT;

typedef struct
{
    ClCharT    *pCompPrefix;
    ClUint32T  hash;
} ClLogMasterCompKeyT;

typedef struct
{
    ClCntHandleT         hStreamTable;
    ClLogStreamAttrIDLT  streamAttr;
    ClUint32T            nActiveStreams;
} ClLogFileDataT;

typedef struct
{
    ClIocMulticastAddressT  streamMcastAddr;
    ClUint16T               streamId;
} ClLogMasterStreamDataT;

typedef struct
{
    SaNameT   *pNodeName;
    ClBoolT   flag;
    ClUint32T numStreams;
}ClLogMasterWalkArgT;

ClRcT
clLogMasterBootup(void);

ClRcT
clLogMasterShutdown(void);

ClInt32T
clLogFileKeyCompare(CL_IN ClCntKeyHandleT  key1,
                    CL_IN ClCntKeyHandleT  key2);


ClUint32T
clLogFileKeyHashFn(CL_IN ClCntKeyHandleT  key);

void
clLogMasterFileEntryDeleteCb(CL_IN ClCntKeyHandleT   key,
                             CL_IN ClCntDataHandleT  data);
void
clLogFileKeyDestroy(CL_IN ClLogFileKeyT  *pKey);

ClRcT
clLogFileKeyCreate(CL_IN  ClStringT      *pFileName,
                   CL_IN  ClStringT      *pFileLocation,
                   CL_IN  ClUint32T      maxFiles,
                   CL_OUT ClLogFileKeyT  **ppKey);

void
clLogMasterStreamEntryDeleteCb(CL_IN ClCntKeyHandleT   key,
                               CL_IN ClCntDataHandleT  data);

ClRcT
clLogMasterAttrVerifyNGet(CL_IN    ClLogStreamAttrIDLT     *pStreamAttr,
                          CL_INOUT SaNameT                 *pStreamName,
                          CL_INOUT ClLogStreamScopeT       *pStreamScope,
                          CL_INOUT SaNameT                 *pStreamScopeNode,
                          CL_OUT   ClUint16T               *pStreamId,
                          CL_OUT   ClIocMulticastAddressT  *pStreamMcastAddr);
ClRcT
clLogMasterCompEntryUpdate(SaNameT    *pCompName,
                           ClUint32T  *pClientId,
                           ClBoolT    restart);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_MASTER_H_ */
