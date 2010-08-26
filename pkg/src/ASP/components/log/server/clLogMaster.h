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
    ClNameT   *pNodeName;
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
                          CL_INOUT ClNameT                 *pStreamName,
                          CL_INOUT ClLogStreamScopeT       *pStreamScope,
                          CL_INOUT ClNameT                 *pStreamScopeNode,
                          CL_OUT   ClUint16T               *pStreamId,
                          CL_OUT   ClIocMulticastAddressT  *pStreamMcastAddr);
ClRcT
clLogMasterCompEntryUpdate(ClNameT    *pCompName,
                           ClUint32T  *pClientId,
                           ClBoolT    restart);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_MASTER_H_ */
