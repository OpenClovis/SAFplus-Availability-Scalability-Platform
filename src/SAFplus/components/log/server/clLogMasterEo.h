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
#ifndef _CL_LOG_MASTER_EO_H_
#define _CL_LOG_MASTER_EO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmapApi.h>
#include <clCkptApi.h>
#include <clLogCommon.h>
#include <clLogSvrCommon.h>

#define CL_LOG_MASTER_START_MCAST_ADDR    0x00000000000000FF
#define CL_LOG_MASTER_MAX_FILES           1024
#define CL_LOG_MASTER_MAX_SECTIONID_SIZE  1024
#define CL_LOG_MASTER_COMPID_START        32
#define CL_LOG_MASTER_STREAMID_START        32

#define CL_LOG_MASTER_EO_ENTRY_KEY        4

typedef struct
{
    ClOsalMutexId_LT        masterFileTableLock;
    ClCntHandleT            hMasterFileTable; 
    ClUint16T               nextStreamId;
    ClOsalMutexId_LT        masterCompTableLock;
    ClCntHandleT            hCompTable;
    ClUint32T               nextCompId;
    ClUint32T               numComps;
    ClIocMulticastAddressT  startMcastAddr;
    ClUint32T               numMcastAddr;
    ClBitmapHandleT         hAllocedAddrMap;
    ClUint32T               maxFiles;
    ClUint32T               sectionSize;
    ClUint32T               sectionIdSize;
    ClCkptHdlT              hCkpt;
} ClLogMasterEoDataT;

ClRcT
clLogMasterEoDataInit(CL_OUT ClLogMasterEoDataT  **ppMasterEoEntry);


ClRcT
clLogMasterEoDataFinalize(CL_IN ClLogMasterEoDataT *pMasterEoEntry);

ClRcT
clLogMasterEoEntryGet(CL_OUT ClLogMasterEoDataT  **ppMasterEoEntry,
                      CL_OUT ClLogSvrCommonEoDataT  **ppCommonEoEntry);

ClRcT
clLogMasterEoEntrySet(CL_IN ClLogMasterEoDataT  *pMasterEoEntry);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_MASTER_EO_H_ */
