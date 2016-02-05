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
 * File        : clMsgReceiver.h
 *******************************************************************************/


#ifndef __CL_MSG_RECEIVER_H__
#define __CL_MSG_RECEIVER_H__

#include <clCommon.h>
#include <clIocApi.h>
#include <clEoApi.h>
#include <clMsgCommon.h>
#include <clMsgQueue.h>
#include <clMsgApiExt.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    ClIocPhysicalAddressT senderAddr;
    ClHandleT senderHandle;
    SaTimeT timeout;
    ClTimerHandleT timerHandle;
    ClHandleT myOwnHandle;
} ClMsgReplyDetailsT;

typedef struct {
    ClHandleT senderHandle;
    ClUint32T messageId;
}ClMsgKeyT;

typedef struct {
    SaMsgMessageT *pMessage;
    SaTimeT sendTime;
    ClHandleT replyId;
} ClMsgReceivedMessageDetailsT;

extern ClHandleDatabaseHandleT gMsgReplyDetailsDb;

ClRcT clMsgReceiverDatabaseInit(void);
void clMsgReceiverDatabaseFin(void);
ClRcT clMsgQueueTheLocalMessage(
        ClMsgMessageSendTypeT sendType,
        SaMsgQueueHandleT qHandle,
        ClMsgMessageIovecT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout);
ClRcT clMsgQueueGetMessagesAndMove(
        ClMsgQueueInfoT *pQInfo,
        ClIocPhysicalAddressT destCompAddr);
ClRcT clMsgReplyReceived(ClMsgMessageIovecT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout);


#ifdef __cplusplus
}
#endif


#endif
