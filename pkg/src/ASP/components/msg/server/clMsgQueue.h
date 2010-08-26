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


#ifndef __CL_MSG_QUEUE_H__
#define __CL_MSG_QUEUE_H__

#include <clCommon.h>
#include <clList.h>
#include <clHandleApi.h>
#include <saAis.h>
#include <saMsg.h>
#include <clMsgDatabase.h>
#include <clVersion.h>


#ifdef __cplusplus
extern "C" {
#endif


#define CL_MSG_QUEUE_PRIORITIES  (SA_MSG_MESSAGE_LOWEST_PRIORITY + 1)

typedef enum {
    CL_MSG_QUEUE_CREATED      = 0,
    CL_MSG_QUEUE_OPEN         = 1,
    CL_MSG_QUEUE_CLOSED       = 2,
} ClMsgQueueStateFlagT;

typedef struct {
    struct hashStruct hash;
    ClListHeadT qList;
    ClListHeadT trackList;
    SaMsgHandleT cltHandle;
    SaMsgHandleT svrHandle;
    ClIocPhysicalAddressT address;
} ClMsgClientDetailsT;


typedef struct {
    ClListHeadT list;
    SaMsgQueueHandleT qHandle;
}ClMsgClientQueueDetailsT;


typedef struct {
    ClMsgClientDetailsT *pOwner;
    ClMsgQueueRecordT *pQHashEntry;
    ClMsgClientQueueDetailsT *pQInfoAtClt;
    /* configuration */
    SaMsgQueueOpenFlagsT openFlags;
    SaMsgQueueCreationFlagsT creationFlags;
    SaSizeT size[CL_MSG_QUEUE_PRIORITIES];
    SaTimeT retentionTime;
    /* present status */
    ClMsgQueueStateFlagT state;
    SaSizeT usedSize[CL_MSG_QUEUE_PRIORITIES];
    ClCntHandleT pPriorityContainer[CL_MSG_QUEUE_PRIORITIES];
    SaUint32T numberOfMessages[CL_MSG_QUEUE_PRIORITIES];
    ClBoolT unlinkFlag;
    SaTimeT closeTime;
    /* helper-datastructures */
    ClOsalMutexT qLock;
    ClOsalCondIdT qCondVar;
    ClUint32T numThreadsBlocked;
    ClTimerHandleT timerHandle;
} ClMsgQueueInfoT;

extern ClHandleDatabaseHandleT gClMsgQDatabase; 
extern ClOsalMutexT gClLocalQsLock;

ClRcT clMsgQueueInitialize(void);
ClRcT clMsgQueueFinalize(void);
ClRcT VDECL_VER(clMsgClientQueueClose, 4, 0, 0)(SaMsgQueueHandleT queueHandle);
void clMsgFailoverClosedQMove(ClMsgQueueInfoT *pQInfo, ClIocNodeAddressT destNode);
void clMsgQueueFree(ClBoolT qMoved, ClMsgQueueInfoT *pQInfo);

#ifdef __cplusplus
}
#endif

#endif
