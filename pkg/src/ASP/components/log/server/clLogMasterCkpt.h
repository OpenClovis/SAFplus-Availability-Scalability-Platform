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
#ifndef _CL_LOG_MASTER_CKPT_H_
#define _CL_LOG_MASTER_CKPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogCommon.h>
#include <clLogMasterEo.h>
#include <clIocApi.h>
#include <xdrClLogCompDataT.h>

#define CL_LOG_MASTER_RETENTION_TIME  1000 * 1000L * 1000L 

#define CL_LOG_COMP_SEC_OFFSET  (CL_MAX_NAME_LENGTH + (3 * sizeof(ClUint32T)))
    
typedef struct
{
    ClUint16T               nextStreamId;
    ClIocMulticastAddressT  startMcastAddr;
    ClUint32T               numMcastAddr;
    ClUint32T               maxFiles;
    ClUint32T               maxStreamsInFile;
} ClLogMasterCkptEoDataT;


ClRcT
clLogMasterCkptGet(void);

ClRcT
clLogMasterCkptDestroy(void);

ClRcT
clLogMasterCkptDelete(void);

ClRcT
clLogMasterDataCheckpoint(CL_IN ClLogMasterEoDataT  *pMasterEoEntry,
                          CL_IN ClCntNodeHandleT     hFileNode,
                          CL_IN ClBoolT              addCkptEntry);

ClRcT
clLogMasterStateRecover(CL_IN ClLogSvrCommonEoDataT  *pCommonEoEntry,
            			CL_IN ClLogMasterEoDataT     *pMasterEoEntry,
                        CL_IN ClBoolT switchover);
extern ClRcT
clLogMasterGlobalCkptRead(ClLogSvrCommonEoDataT  *pCommonEoEntry, ClBoolT switchover);

extern ClRcT
clLogMasterCompDataCheckpoint(ClLogMasterEoDataT   *pMasterEoEntry, 
                              ClLogCompDataT       *pCompData);
extern ClRcT
clLogMasterCompTableStateRecover(ClLogMasterEoDataT  *pMasterEoEntry, 
                                 ClUint8T            *pBuffer, 
                                 ClUint32T           dataSize);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_MASTER_CKPT_H_ */
