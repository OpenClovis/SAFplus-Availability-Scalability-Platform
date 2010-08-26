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


extern ClUint32T gMsgNumInits;
extern ClHandleDatabaseHandleT gMsgHandleDatabase;
extern ClIdlHandleT gClMsgIdlHandle;


SaAisErrorT clMsgErrorTxlate(ClRcT clError);
#define CL_MSG_SA_RC(x) clErrorToSaf(x)


#define CL_MSG_CLIENT_INIT_CHECK \
    do { \
        if(gMsgNumInits == 0) \
        { \
            ClRcT rc = CL_MSG_RC(CL_ERR_INVALID_HANDLE); \
            clLogError("MSG", "COM", "Message client is not initialized yet. error code [0x%x]. but returning error code [0x%x].", \
                    CL_MSG_RC(CL_ERR_NOT_INITIALIZED), rc); \
            return CL_MSG_SA_RC(rc); \
        } \
    } while(0)


#ifdef __cplusplus
}
#endif

#endif
