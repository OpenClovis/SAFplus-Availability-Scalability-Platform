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
#include <clMsgCommon.h>
#include <saMsg.h>

#include <msgCltSrvClientCallsFromClientToClientServerClient.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClIdlHandleObjT gIdlUcastObj;
extern ClIdlHandleObjT gIdlBcastObj;
extern ClIdlHandleT gIdlUcastHandle;
extern ClIdlHandleT gIdlBcastHandle;
    
ClRcT clMsgCommIdlInitialize(void);
void  clMsgCommIdlFinalize(void);

ClRcT clMsgSendMessage_idl(ClMsgMessageSendTypeT sendType,
        ClIocPhysicalAddressT compAddr,
        ClNameT *pName,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout,
        ClBoolT isSync,
        SaMsgAckFlagsT ackFlag,
        MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
        void *cookie);

ClRcT clMsgSendMessage_IocSend(ClMsgMessageSendTypeT sendType,
        ClIocPhysicalAddressT compAddr,
        ClNameT *pName,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout,
        ClBoolT isSync,
        SaMsgAckFlagsT ackFlag,
        MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
        void *cookie);

ClRcT clMsgQueueUnlinkToServer(ClNameT *pQName);

ClRcT clMsgCallClientsTrackCallback(ClIocPhysicalAddressT compAddr,
        ClNameT *pGroupName,
        SaMsgHandleT appHandle,
        SaMsgQueueGroupNotificationBufferT *pData);

ClRcT clMsgQueueAllocateThroughIdl(
        ClIocPhysicalAddressT destNode,
        ClNameT *pQName,
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttrs,
        SaMsgQueueHandleT *pQHandle);

ClRcT clMsgMessageGet_Idl(
        ClIocPhysicalAddressT destNode,
        ClNameT *pQName,
        SaTimeT timeout);

#ifdef __cplusplus
}
#endif

#endif
