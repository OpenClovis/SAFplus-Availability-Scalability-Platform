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
#include <clMsgCommon.h>
#include <clMsgGeneral.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    struct hashStruct hash;
    ClNameT name;
    SaMsgQueueGroupPolicyT policy;
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
    ClNameT qName;
    SaMsgQueueGroupChangesT change;
}ClMsgGroupsQueueDetailsT;


extern ClOsalMutexT gClGroupDbLock;

ClBoolT clMsgGroupEntryExists(const ClNameT *pQGroupName, ClMsgGroupRecordT **ppQGroupEntry);
ClBoolT clMsgDoesQExistInGroup(ClMsgGroupRecordT *pQGroupEntry, const ClNameT *pQName,
        ClMsgGroupsQueueDetailsT **ppQListEntry);

ClRcT clMsgNodeGroupEntriesGet(ClUint8T **ppData, ClUint32T *pCount);

ClRcT clMsgAddQueueToGroup(ClNameT *pGroupName, ClNameT *pQueueName);
ClRcT clMsgGroupInfoUpdate(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy);

ClRcT clMsgGroupMembershipInfoSend(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName);

ClRcT clMsgAllQueuesOfGroupGet(ClMsgGroupRecordT *pMsgGroup, SaMsgQueueGroupNotificationT **ppData, ClUint32T *pCount);

void clMsgGroupDatabaseShow(void);


/*****************************************************************************************/
/* As of now I am putting the "clMsgGroupCalls.c"s declarations in this file. */

#include <clMsgEo.h>
ClRcT clMsgQueueGroupTrackStopInternal(ClMsgClientDetailsT *pClient, SaMsgHandleT msgHandle, const ClNameT *pGroupName);

#ifdef __cplusplus
}
#endif


#endif
