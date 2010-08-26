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
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#ifndef __CL_MSG_GROUP_DATABASE_H__
#define __CL_MSG_GROUP_DATABASE_H__

#include <clCommon.h>
#include <clMsgDatabase.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    struct hashStruct hash;
    ClNameT name;
    SaMsgQueueGroupPolicyT policy;
    ClIocPhysicalAddressT rrNext;
    ClListHeadT *pRRNext;
    ClListHeadT qList;
    ClListHeadT trackList;
    ClOsalMutexT groupLock;
}ClMsgGroupRecordT;


typedef struct {
    ClListHeadT list;
    ClListHeadT clientList;
    ClUint8T trackType;
    ClMsgGroupRecordT *pMsgGroup;
    SaMsgHandleT appHandle;
    SaMsgHandleT clientHandle;
    ClIocPhysicalAddressT  compAddr;
} ClMsgGroupTrackListEntryT;


typedef struct {
    ClListHeadT list;
    ClMsgGroupRecordT *pGroupDetails;
}ClMsgQueuesGroupDetailsT;


typedef struct {
    ClListHeadT list;
    ClMsgQueueRecordT *pQueueDetails;
    SaMsgQueueGroupChangesT change;
}ClMsgGroupsQueueDetailsT;


extern ClOsalMutexT gClGroupDbLock;

ClBoolT clMsgGroupEntryExists(const ClNameT *pQGroupName, ClMsgGroupRecordT **ppQGroupEntry);
ClBoolT clMsgDoesQExistInGroup(ClMsgGroupRecordT *pQGroupEntry, const ClNameT *pQName,
        ClMsgQueuesGroupDetailsT **ppGListEntry, ClMsgGroupsQueueDetailsT **ppQListEntry);

ClRcT clMsgNodeGroupEntriesGet(ClUint8T **ppData, ClUint32T *pCount);

ClRcT clMsgAddQueueToGroup(ClNameT *pGroupName, ClNameT *pQueueName);
ClRcT clMsgNewGroupAddInternal(ClMsgGroupSyncupRecordT *pGroupInfo, ClMsgGroupRecordT **ppGroupEntry);
ClRcT clMsgGroupInfoUpdateSend(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy);

ClRcT clMsgGroupMembershipInfoSend(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName);

ClRcT clMsgRRNextMemberGet(ClMsgGroupRecordT *pMsgGroup, ClMsgQueueRecordT **pQueue);
ClRcT clMsgGroupLocalBestNextMemberGet(ClMsgGroupRecordT *pMsgGroup, ClMsgQueueRecordT **pQueue, SaUint8T priority);

ClRcT clMsgAllQueuesOfGroupGet(ClMsgGroupRecordT *pMsgGroup, SaMsgQueueGroupNotificationT **ppData, ClUint32T *pCount);

void clMsgDeleteAllGroupsOfQueue(ClMsgQueueRecordT *pQueue);


ClRcT clMsgGroupDatabasePack(ClBufferHandleT message);
ClRcT clMsgGroupDatabaseSyncup(ClBufferHandleT message);

void clMsgGroupDatabaseShow(void);


/*****************************************************************************************/
/* As of now I am putting the "clMsgGroupCalls.c"s declarations in this file. */

#include <clMsgEo.h>
ClRcT clMsgQueueGroupTrackStopInternal(ClMsgClientDetailsT *pClient, SaMsgHandleT msgHandle, const ClNameT *pGroupName);

#ifdef __cplusplus
}
#endif


#endif
