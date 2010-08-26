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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#ifndef __CL_MSG_DATABASE_H__
#define __CL_MSG_DATABASE_H__

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clList.h>
#include <clBufferApi.h>
#include <clIocApi.h>
#include <saMsg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CL_MSG_DATA_ADD = 1,
    CL_MSG_DATA_DEL,
    CL_MSG_DATA_UPD
}ClMsgSyncActionT;


typedef struct {
    ClNameT name;
    SaMsgQueueGroupPolicyT policy;
}ClMsgGroupSyncupRecordT;


typedef struct {
    ClNameT qName;
    ClIocPhysicalAddressT compAddr;
    ClUint32T numGroups;
}ClMsgQueueSyncupRecordT;

typedef struct {
    struct hashStruct qNodeHash;
    struct hashStruct qNameHash;
    ClListHeadT groupList;
    ClNameT qName;
    SaMsgQueueHandleT qHandle;
    ClIocPhysicalAddressT compAddr;
}ClMsgQueueRecordT;

#define CL_MSG_NODE_Q_BUCKET_BITS  (6)
#define CL_MSG_NODE_Q_BUCKETS      (1<<CL_MSG_NODE_Q_BUCKET_BITS)
#define CL_MSG_NODE_Q_MASK         (CL_MSG_NODE_Q_BUCKETS - 1)

extern ClOsalMutexT gClQueueDbLock;
extern struct hashStruct *ppMsgNodeQHashTable[];

ClRcT msgQueueInfoUpdateSend_internal(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle,
        ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue);
ClRcT msgQueueInfoUpdateSend_SyncUnicast(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle,
        ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue);
ClRcT clMsgQueueInfoUpdateSend(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle,
        ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue);
ClRcT clMsgNodeQEntriesGet(ClIocNodeAddressT nodeAddr, ClUint8T **ppData, ClUint32T *pCount);
ClBoolT clMsgQNameEntryExists(const ClNameT *pQName, ClMsgQueueRecordT **ppQNameEntry);
ClRcT clMsgUpdateDatabases(ClUint8T *pData, ClUint32T size);
void clMsgNodeLeftCleanup(ClIocAddressT *pAddr);
void clMsgCompLeftCleanup(ClIocAddressT *pAddr); 
void clMsgQueueDatabaseShow(void);


static __inline__ ClUint32T clMsgNodeQHash(ClIocNodeAddressT nodeAddress)
{
    return ((ClUint32T) (nodeAddress & CL_MSG_NODE_Q_MASK));
}


#ifdef __cplusplus
}
#endif


#endif
