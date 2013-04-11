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
 * File        : clMsgSender.h
 *******************************************************************************/

#ifndef __CL_MSG_SENDER_H__
#define __CL_MSG_SENDER_H__

#include <clCommon.h>
#include <clCommonErrors.h>
#include <saMsg.h>
#include <clMsgApiExt.h>

#include <msgCltSrvClientCallsFromClientToClientServerClient.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    ClOsalCondIdT condVar;
    ClOsalMutexT mutex;
    SaMsgMessageT *pRecvMessage;
    SaTimeT *pReplierSendTime;
    ClRcT replierRc;
}ClMsgWaitingSenderDetailsT;

extern ClHandleDatabaseHandleT gMsgSenderDatabase;


ClRcT clMsgSenderDatabaseInit(void);
void clMsgSenderDatabaseFin(void);

ClRcT clMsgClientMessageSend(ClIocAddressT *pDestAddr, 
                             ClIocAddressT *pServerAddr, 
                             ClNameT *pDest, 
                             ClMsgMessageIovecT *pMessage,
                             SaTimeT sendTime, 
                             SaTimeT timeout,
                             ClBoolT isSync,
                             SaMsgAckFlagsT ackFlag,
                             MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
                             void *cookie);
ClRcT clMsgClientMessageSendReceive(ClIocAddressT * pDestAddr,
                             ClIocAddressT * pServerAddr,
                             ClNameT *pDest,
                             SaMsgMessageT *pSendMessage,
                             SaTimeT sendTime,
                             SaMsgMessageT *pRecvMessage,
                             SaTimeT *pReplierSendTime,
                             SaTimeT timeout);
ClRcT clMsgClientMessageReply(SaMsgMessageT *pMessage,
                             SaTimeT sendTime, 
                             ClHandleT senderId, 
                             SaTimeT timeout,
                             ClBoolT isSync,
                             SaMsgAckFlagsT ackFlag,
                             MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
                             void *cookie);


#ifdef __cplusplus
}
#endif

#endif
