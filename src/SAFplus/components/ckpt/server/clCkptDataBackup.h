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

/*******************************************************************************
 * ModuleName  : ckpt                                                          
 * File        : clCkptDataBackup.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * related to checkpointing the metadata and created checkpoints
 * (writing/reading from persistent
 * memory).
 *****************************************************************************/
#ifndef _CL_CKPT_DATA_BACK_UP_H_
#define _CL_CKPT_DATA_BACK_UP_H_

#ifdef __cplusplus
extern "C" {
#endif


extern ClRcT ckptMasterDatabasePack(ClBufferHandleT  outMsg);
extern ClRcT ckptDbPack(ClBufferHandleT   *pOutMsg,ClIocNodeAddressT        peerAddr);


ClRcT ckptMetaDataSerializer(ClUint32T dataSetID, ClAddrT* ppData, ClUint32T* pDataLen, ClPtrT pCookie);

ClRcT ckptMetaDataDeserializer(ClUint32T dataSetID, ClAddrT pData, ClUint32T dataLen, ClPtrT pCookie);

ClRcT ckptCheckpointsSerializer(ClUint32T dataSetID, ClAddrT* ppData, ClUint32T* pDataLen, ClPtrT pCookie);

ClRcT ckptCheckpointsDeserializer(ClUint32T dataSetID, ClAddrT pData, ClUint32T dataLen, ClPtrT pCookie);

ClRcT ckptDataPeriodicSave();

ClRcT ckptDataBackupInitialize(ClUint8T *pFlag);

#ifdef __cplusplus
 }
#endif

#endif
