/*******************************************************************************
**
** FILE:
**   SaMsg.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Message Service (MSG). It contains all of 
**   the prototypes and type definitions required for MSG. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-MSG-B.03.01
**
** DATE: 
**   Mon  Dec   18  2006  
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2006 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/

#ifndef _SA_MSG_H
#define _SA_MSG_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaMsgHandleT;
typedef SaUint64T SaMsgQueueHandleT;
typedef SaUint64T SaMsgSenderIdT;

#define SA_MSG_MESSAGE_DELIVERED_ACK 0x1
typedef SaUint32T SaMsgAckFlagsT;

#define SA_MSG_QUEUE_PERSISTENT  0x1
typedef SaUint32T SaMsgQueueCreationFlagsT;

#define SA_MSG_MESSAGE_HIGHEST_PRIORITY 0
#define SA_MSG_MESSAGE_LOWEST_PRIORITY 3

typedef struct {
    SaMsgQueueCreationFlagsT creationFlags;
    SaSizeT                  size[SA_MSG_MESSAGE_LOWEST_PRIORITY+1];
    SaTimeT                  retentionTime;
} SaMsgQueueCreationAttributesT;

#define SA_MSG_QUEUE_CREATE 0x1
#define SA_MSG_QUEUE_RECEIVE_CALLBACK 0x2
#define SA_MSG_QUEUE_EMPTY 0x4
typedef SaUint32T SaMsgQueueOpenFlagsT;

typedef struct {
    SaUint32T queueSize;
    SaSizeT   queueUsed;
    SaUint32T numberOfMessages;
} SaMsgQueueUsageT;

typedef struct {
    SaMsgQueueCreationFlagsT creationFlags;
    SaTimeT                  retentionTime;
    SaTimeT                  closeTime;
    SaMsgQueueUsageT         saMsgQueueUsage[SA_MSG_MESSAGE_LOWEST_PRIORITY+1];
} SaMsgQueueStatusT;

typedef enum {
    SA_MSG_QUEUE_GROUP_ROUND_ROBIN = 1,
    SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN = 2,
    SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE = 3,
    SA_MSG_QUEUE_GROUP_BROADCAST = 4
} SaMsgQueueGroupPolicyT;

typedef enum {
    SA_MSG_QUEUE_GROUP_NO_CHANGE = 1,
    SA_MSG_QUEUE_GROUP_ADDED = 2,
    SA_MSG_QUEUE_GROUP_REMOVED = 3,
    SA_MSG_QUEUE_GROUP_STATE_CHANGED = 4
} SaMsgQueueGroupChangesT;

typedef struct {
    SaNameT queueName;
} SaMsgQueueGroupMemberT;

typedef struct {
    SaMsgQueueGroupMemberT  member;
    SaMsgQueueGroupChangesT change;
} SaMsgQueueGroupNotificationT;

typedef struct {
    SaUint32T                     numberOfItems;
    SaMsgQueueGroupNotificationT *notification;
    SaMsgQueueGroupPolicyT        queueGroupPolicy;
} SaMsgQueueGroupNotificationBufferT;

typedef struct {
    SaUint32T type;
    SaUint32T version;
    SaSizeT   size;
    SaNameT  *senderName;
    void     *data;
    SaUint8T  priority;
} SaMsgMessageT;
     
typedef enum {
    SA_MSG_QUEUE_CAPACITY_REACHED = 1,
    SA_MSG_QUEUE_AVAILABLE = 2,
    SA_MSG_QUEUE_GROUP_CAPACITY_REACHED = 3,
    SA_MSG_QUEUE_GROUP_AVAILABLE = 4
} saMsgMessageCapacityStatusT;
     
typedef struct {
    SaSizeT capacityReached[SA_MSG_MESSAGE_LOWEST_PRIORITY+1];
    SaSizeT capacityAvailable[SA_MSG_MESSAGE_LOWEST_PRIORITY+1];
} SaMsgQueueThresholdsT;

typedef enum {
    SA_MSG_DEST_CAPACITY_STATUS = 1
} SaMsgStateT;

typedef enum {
    SA_MSG_MAX_PRIORITY_AREA_SIZE_ID = 1,
    SA_MSG_MAX_QUEUE_SIZE_ID = 2,
    SA_MSG_MAX_NUM_QUEUES_ID = 3,
    SA_MSG_MAX_NUM_QUEUE_GROUPS_ID = 4,
    SA_MSG_MAX_NUM_QUEUES_PER_GROUP_ID = 5,
    SA_MSG_MAX_MESSAGE_SIZE_ID = 6,
    SA_MSG_MAX_REPLY_SIZE_ID = 7,
} SaMsgLimitIdT;

typedef void
(*SaMsgQueueOpenCallbackT)(SaInvocationT invocation,
                           SaMsgQueueHandleT queueHandle,
                           SaAisErrorT error);

typedef void
(*SaMsgQueueGroupTrackCallbackT) (
                   const SaNameT *queueGroupName,
                   const SaMsgQueueGroupNotificationBufferT *notificationBuffer,
                   SaUint32T numberOfMembers,
                   SaAisErrorT error);

typedef void 
(*SaMsgMessageDeliveredCallbackT)(SaInvocationT invocation,
                                  SaAisErrorT error);

typedef void 
(*SaMsgMessageReceivedCallbackT)(SaMsgQueueHandleT queueHandle);

typedef struct {
    SaMsgQueueOpenCallbackT        saMsgQueueOpenCallback;
    SaMsgQueueGroupTrackCallbackT  saMsgQueueGroupTrackCallback;
    SaMsgMessageDeliveredCallbackT saMsgMessageDeliveredCallback;
    SaMsgMessageReceivedCallbackT  saMsgMessageReceivedCallback; 
} SaMsgCallbacksT;

/*************************************************/
/******** MSG API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saMsgInitialize(
    SaMsgHandleT *msgHandle, 
    const SaMsgCallbacksT *msgCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saMsgSelectionObjectGet(
    SaMsgHandleT msgHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saMsgDispatch(
    SaMsgHandleT msgHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saMsgFinalize(
    SaMsgHandleT msgHandle);

extern SaAisErrorT 
saMsgQueueOpen(
    SaMsgHandleT msgHandle,
    const SaNameT *queueName,
    const SaMsgQueueCreationAttributesT *creationAttributes,
    SaMsgQueueOpenFlagsT openFlags,
    SaTimeT timeout,
    SaMsgQueueHandleT *queueHandle);

extern SaAisErrorT 
saMsgQueueOpenAsync(
    SaMsgHandleT msgHandle,
    SaInvocationT invocation,
    const SaNameT *queueName,
    const SaMsgQueueCreationAttributesT *creationAttributes,
    SaMsgQueueOpenFlagsT openFlags);

extern SaAisErrorT 
saMsgQueueClose(
    SaMsgQueueHandleT queueHandle);

extern SaAisErrorT 
saMsgQueueStatusGet(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueName, 
    SaMsgQueueStatusT *queueStatus);

extern SaAisErrorT 
saMsgQueueRetentionTimeSet(
    SaMsgQueueHandleT queueHandle,
    SaTimeT *retentionTime);

extern SaAisErrorT 
saMsgQueueUnlink(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueName);

extern SaAisErrorT
saMsgQueueGroupCreate(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueGroupName,
    SaMsgQueueGroupPolicyT queueGroupPolicy);

extern SaAisErrorT 
saMsgQueueGroupDelete(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueGroupName);

extern SaAisErrorT 
saMsgQueueGroupInsert(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueGroupName,
    const SaNameT *queueName);

extern SaAisErrorT 
saMsgQueueGroupRemove(
    SaMsgHandleT msgHandle, 
    const SaNameT *queueGroupName, 
    const SaNameT *queueName);

extern SaAisErrorT 
saMsgQueueGroupTrack(
    SaMsgHandleT msgHandle,
    const SaNameT *queueGroupName,
    SaUint8T trackFlags,
    SaMsgQueueGroupNotificationBufferT *notificationBuffer);

extern SaAisErrorT 
saMsgQueueGroupTrackStop(
    SaMsgHandleT msgHandle,
    const SaNameT *queueGroupName);

extern SaAisErrorT 
saMsgQueueGroupNotificationFree(
    SaMsgHandleT msgHandle,
    SaMsgQueueGroupNotificationT *notification);

extern SaAisErrorT 
saMsgMessageSend(
    SaMsgHandleT msgHandle,
    const SaNameT *destination,
    const SaMsgMessageT *message,
    SaTimeT timeout);

extern SaAisErrorT 
saMsgMessageSendAsync(
    SaMsgHandleT msgHandle,
    SaInvocationT invocation,
    const SaNameT *destination,
    const SaMsgMessageT *message,
    SaMsgAckFlagsT ackFlags);

extern SaAisErrorT 
saMsgMessageGet(
    SaMsgQueueHandleT queueHandle,
    SaMsgMessageT *message,
    SaTimeT *sendTime,
    SaMsgSenderIdT *senderId,
    SaTimeT timeout);

extern SaAisErrorT 
saMsgMessageDataFree(
    SaMsgHandleT msgHandle,
    void *data);

extern SaAisErrorT 
saMsgMessageCancel(
    SaMsgQueueHandleT queueHandle);

extern SaAisErrorT 
saMsgMessageSendReceive(
    SaMsgHandleT msgHandle,
    const SaNameT *destination,
    const SaMsgMessageT *sendMessage,
    SaMsgMessageT *receiveMessage,
    SaTimeT *replySendTime,
    SaTimeT timeout);

extern SaAisErrorT 
saMsgMessageReply(
    SaMsgHandleT msgHandle,
    const SaMsgMessageT *replyMessage,
    const SaMsgSenderIdT *senderId,
    SaTimeT timeout);

extern SaAisErrorT 
saMsgMessageReplyAsync(
    SaMsgHandleT msgHandle,
    SaInvocationT invocation,
    const SaMsgMessageT *replyMessage,
    const SaMsgSenderIdT *senderId,
    SaMsgAckFlagsT ackFlags);

extern SaAisErrorT 
saMsgQueueCapacityThresholdsSet(
    SaMsgQueueHandleT queueHandle,
    const SaMsgQueueThresholdsT *thresholds);

extern SaAisErrorT 
saMsgQueueCapacityThresholdsGet(
    SaMsgQueueHandleT queueHandle,
    SaMsgQueueThresholdsT *thresholds);

extern SaAisErrorT 
saMsgMetadataSizeGet(
    SaMsgHandleT msgHandle,
    SaUint32T *metadataSize);

extern SaAisErrorT 
saMsgLimitGet(
    SaMsgHandleT msgHandle,
    SaMsgLimitIdT limitId,
    SaLimitValueT *limitValue);

#ifdef  __cplusplus
}

#endif

#endif  /* _SA_MSG_H */



