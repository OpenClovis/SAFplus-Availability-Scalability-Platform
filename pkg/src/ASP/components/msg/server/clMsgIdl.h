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
 * File        : clMsgIdl.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *****************************************************************************/


#ifndef __CL_MSG_IDL_H__
#define __CL_MSG_IDL_H__

#include <clCommon.h>
#include <clIdlApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clMsgSender.h>
#include <saMsg.h>


#ifdef __cplusplus
extern "C" {
#endif

ClRcT clMsgSrvIdlInitialize(void);
ClRcT clMsgSrvIdlFinalize(void);

ClRcT clMsgSendMessage_idl(ClMsgMessageSendTypeT sendType, ClIocNodeAddressT nodeAddr, ClNameT *pName,
        SaMsgMessageT *pMessage, SaTimeT sendTime, ClHandleT senderHandle, SaMsgAckFlagsT ackFlag, SaTimeT timeout);
ClRcT clMsgGetDatabasesThroughIdl(ClIocNodeAddressT masterAddr, ClUint8T **ppData, ClUint32T *pSize);

ClRcT clMsgQUpdateSendThroughIdl(ClMsgSyncActionT syncAct, ClNameT *pQName, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr);
ClRcT clMsgGroupUpdateSendThroughIdl(ClMsgSyncActionT syncAct, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy);
ClRcT clMsgGroupMembershipChangeInfoSendThroughIdl(ClMsgSyncActionT syncAct, ClNameT *pGroupName, ClNameT *pQueueName);
ClRcT clMsgCallClientsTrackCallback(ClIocPhysicalAddressT compAddr, ClNameT *pGroupName, SaMsgHandleT appHandle,
        SaMsgQueueGroupNotificationBufferT *pData);
ClRcT clMsgCallClientsMessageReceiveCallback(ClIocPhysicalAddressT *pCompAddr, SaMsgHandleT clientHandle, SaMsgQueueHandleT qHandle);
ClRcT clMsgQueueInfoGetThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pName, SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pQMvInfo);
ClRcT clMsgQueueMoveMessagesThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pName, SaMsgQueueOpenFlagsT openFlags, ClIocPhysicalAddressT *pCompAddr);
ClRcT clMsgQueueAllocateThroughIdl( ClIocNodeAddressT toNode, ClBoolT newQ, ClNameT *pQName,
        ClIocPhysicalAddressT *pCompAddress, SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttrs, SaMsgQueueHandleT *pQHandle);
ClRcT clMsgFailoverQMovedInfoUpdateThroughIdl(ClIocNodeAddressT destNode, ClNameT *pQName);
ClRcT clMsgQueueUnlinkThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pQName);
ClRcT clMsgQueueStatusGetThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pQName, SaMsgQueueStatusT *pQueueStatus);
ClRcT msgQUpdateSend_unicastSync_idl(ClIocNodeAddressT node, ClMsgSyncActionT syncAct, ClNameT *pQName, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr);

#ifdef __cplusplus
}
#endif

#endif
