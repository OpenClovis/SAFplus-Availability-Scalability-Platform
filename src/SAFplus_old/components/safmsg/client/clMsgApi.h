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
 *//*
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


#ifndef __CL_MSG_APIS_H__
#define __CL_MSG_APIS_H__

#include <saMsg.h>
#include <clCommon.h>
#include <clIdlApi.h>
#include <clErrorApi.h>
#include <clJobQueue.h>
#include <clHash.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_MSG_SA_RC(x) clErrorToSaf(x)

typedef struct {
    SaMsgHandleT handle;
    SaMsgCallbacksT callbacks;
    ClHandleT dispatchHandle;
} ClMsgLibInfoT;

typedef enum {
    CL_MSG_QUEUE_OPEN_CALLBACK_TYPE,
    CL_MSG_QUEUE_GROUP_TRACK_CALLBACK_TYPE,
    CL_MSG_MESSAGE_DELIVERED_CALLBACK_TYPE,
    CL_MSG_MESSAGE_RECEIVED_CALLBACK_TYPE
} ClMsgCallbackTypesT;

typedef struct {
    SaMsgHandleT msgHandle;
    SaInvocationT invocation;
    SaMsgQueueHandleT queueHandle;
    SaAisErrorT rc;
} ClMsgAppQueueOpenCallbackParamsT;

extern ClHandleDatabaseHandleT gMsgHandleDatabase;

SaAisErrorT clMsgErrorTxlate(ClRcT clError);

ClRcT clMsgDispatchQueueRegisterInternal(SaMsgHandleT msgHandle,
                                         SaMsgQueueHandleT queueHandle,
                                         SaMsgMessageReceivedCallbackT callback);

ClRcT clMsgDispatchQueueDeregisterInternal(SaMsgQueueHandleT queueHandle);

#ifdef __cplusplus
}
#endif

#endif
