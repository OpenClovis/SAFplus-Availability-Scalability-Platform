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


#ifndef __CL_MSG_RECEIVER_H__
#define __CL_MSG_RECEIVER_H__

#include <clCommon.h>
#include <clIocApi.h>
#include <clEoApi.h>
#include <clMsgDatabase.h>
#include <clMsgSender.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    ClIocNodeAddressT senderNode;
    ClHandleT senderHandle;
    SaTimeT timeout;
    ClTimerHandleT timerHandle;
    ClHandleT myOwnHandle;
} ClMsgReplyDetailsT;

extern ClHandleDatabaseHandleT gMsgReplyDetailsDb;

ClRcT clMsgReceiverDatabaseInit(void);
void clMsgReceiverDatabaseFin(void);
ClRcT clMsgQueueTheLocalMessage(
        ClMsgMessageSendTypeT sendType,
        SaMsgQueueHandleT qHandle,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout);
ClRcT clMsgQueueGetMessagesAndMove(
        ClMsgQueueInfoT *pQInfo,
        ClIocNodeAddressT destNodeAddr
        );
ClRcT clMsgReplyReceived(SaMsgMessageT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout);


#ifdef __cplusplus
}
#endif


#endif
