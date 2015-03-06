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
 * File        : clMsgQueue.h
 *******************************************************************************/

#ifndef __CL_MSG_QUEUE_H__
#define __CL_MSG_QUEUE_H__

#include <clCommon.h>
#include <clList.h>
#include <clHandleApi.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clIocApi.h>
#include <saAis.h>
#include <saMsg.h>

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
    struct hashStruct qNameHash;
    SaNameT qName;
    SaMsgQueueHandleT qHandle;
}ClMsgQueueRecordT;

typedef struct {
    ClMsgQueueRecordT *pQueueEntry;
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
    /* helper-data structures */
    ClOsalMutexT qLock;
    ClOsalCondIdT qCondVar;
    ClUint32T numThreadsBlocked;
    ClTimerHandleT timerHandle;
} ClMsgQueueInfoT;

typedef struct {
    ClListHeadT list;
    SaNameT groupName;
}ClMsgQueuesGroupDetailsT;

extern ClHandleDatabaseHandleT gClMsgQDatabase; 
extern ClOsalMutexT gClLocalQsLock;
extern ClOsalMutexT gClQueueDbLock;

/*
 * Queue info IPIs
 */
ClRcT clMsgQueueInitialize(void);
ClRcT clMsgQueueFinalize(void);
void  clMsgQueueEmpty(ClMsgQueueInfoT *pQInfo);
void  clMsgQueueFree(ClMsgQueueInfoT *pQInfo);
ClRcT clMsgQueueFreeByHandle(SaMsgQueueHandleT qHandle);
ClRcT clMsgQueueStatusGet(SaMsgQueueHandleT qHandle, SaMsgQueueStatusT *pQueueStatus);
ClRcT clMsgQueueRetentionTimeSet(SaMsgQueueHandleT qHandle, SaTimeT *pRetenTime);
ClRcT clMsgMessageCancel(SaMsgQueueHandleT qHandle);
ClRcT clMsgQueueAllocate(
        SaNameT *pQName, 
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueHandleT *pQueueHandle);
ClRcT clMsgQueueOpen(SaMsgQueueHandleT qHandle,
        SaMsgQueueOpenFlagsT openFlags);
ClRcT clMsgQueueDestroy(SaNameT * pQName);
/*
 * Queue entry IPIs
 */
ClBoolT clMsgQNameEntryExists(const SaNameT *pQName, ClMsgQueueRecordT **ppQNameEntry);
ClRcT clMsgQEntryAdd(SaNameT *pName, SaMsgQueueHandleT queueHandle, ClMsgQueueRecordT **ppMsgQEntry);
void clMsgQEntryDel(SaNameT *pQName);
ClRcT clMsgToDestQueueMove(ClIocNodeAddressT destNode, SaNameT *pQName);
ClRcT clMsgToLocalQueueMove(ClIocPhysicalAddressT srcAddr, SaNameT * pQName, ClBoolT qDelete);

ClRcT clMsgQueueStatusGet_4_0_0( SaNameT *pQName, SaMsgQueueStatusT *pQueueStatus);
ClRcT clMsgQueueMoveMessages_4_0_0( SaNameT *pQName, SaMsgQueueOpenFlagsT openFlags, ClBoolT qDelete);
ClRcT clMsgQueueInfoGet_4_0_0( SaNameT *pQName, SaMsgQueueCreationAttributesT *pQNewAttrs);


#ifdef __cplusplus
}
#endif

#endif
