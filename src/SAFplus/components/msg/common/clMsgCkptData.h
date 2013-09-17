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
 * File        : clMsgQueueCkpt.h
 *******************************************************************************/

#ifndef __CL_MSG_CKPT_DATA_H__
#define	__CL_MSG_CKPT_DATA_H__

#include <clMsgQueue.h>
#include <clCachedCkpt.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define CL_MSG_QUEUE_MAX_SECTIONS              2048
#define CL_MSG_QUEUE_DATA_SIZE                 (sizeof(ClIocPhysicalAddressT)  \
                                              + sizeof(ClMsgQueueStateFlagT)   \
                                              + sizeof(SaMsgQueueCreationFlagsT))
#define CL_MSG_QUEUE_MAX_SECTION_SIZE          (sizeof(ClCachedCkptDataT)  \
                                              + CL_MSG_QUEUE_DATA_SIZE)
#define CL_MSG_QUEUE_MAX_SECTION_ID_SIZE       CL_MAX_NAME_LENGTH
#define CL_MSG_QUEUE_CKPT_SIZE                 (CL_MSG_QUEUE_MAX_SECTIONS * CL_MSG_QUEUE_MAX_SECTION_SIZE)
#define CL_MSG_QUEUE_RETENTION_DURATION        (0)

#define CL_MSG_QGROUP_MAX_QUEUES               256
#define CL_MSG_QGROUP_MAX_SECTIONS             1024
#define CL_MSG_QGROUP_DATA_SIZE                (sizeof(SaMsgQueueGroupPolicyT) \
                                              + sizeof(ClUint32T) \
                                              + CL_MSG_QGROUP_MAX_QUEUES  * sizeof(SaNameT))
#define CL_MSG_QGROUP_MAX_SECTION_SIZE         (sizeof(ClCachedCkptDataT)  \
                                              + CL_MSG_QGROUP_DATA_SIZE)
#define CL_MSG_QGROUP_MAX_SECTION_ID_SIZE      CL_MAX_NAME_LENGTH
#define CL_MSG_QGROUP_CKPT_SIZE                (CL_MSG_QGROUP_MAX_SECTIONS * CL_MSG_QGROUP_MAX_SECTION_SIZE)
#define CL_MSG_QGROUP_RETENTION_DURATION       (0)

typedef struct {
    /* Message queue name*/
    SaNameT qName;
    /* Component address of a message queue */
    ClIocPhysicalAddressT qAddress;
    /* Ioc address of server that store persistent queue instance */
    ClIocPhysicalAddressT qServerAddress;
    /* Msg queue state */
    ClMsgQueueStateFlagT state;
    /* Msg queue creation flags */
    SaMsgQueueCreationFlagsT creationFlags;
}ClMsgQueueCkptDataT;

typedef struct {
    /* Message queue group name*/
    SaNameT qGroupName;
    /* Component address of a message queue group */
    ClIocPhysicalAddressT qGroupAddress;
    /* Message queue group policy */
    SaMsgQueueGroupPolicyT policy;
    /* Number of message queues */
    ClUint32T numberOfQueues;
    /* Message queue list */
    SaNameT *pQueueList;
}ClMsgQGroupCkptDataT;

//ClRcT clMsgQueueCkptDataMarshal(const SaNameT *qName, 
//                                const ClIocPhysicalAddressT *qAddress,
//                                const ClIocPhysicalAddressT *qServerAddress,
//                                const ClMsgQueueStateFlagT *state,
//                                const SaMsgQueueCreationFlagsT *creationFlags,
//                                 ClCachedCkptDataT *outData);
ClRcT clMsgQueueCkptDataMarshal(ClMsgQueueCkptDataT *qCkptData, ClCachedCkptDataT *outData);
void clMsgQueueCkptDataUnmarshal(ClMsgQueueCkptDataT *qCkptData, const ClCachedCkptDataT *inData);

ClRcT clMsgQGroupCkptDataMarshal(ClMsgQGroupCkptDataT *qCkptData, ClCachedCkptDataT *outData);
void clMsgQGroupCkptHeaderUnmarshal(ClMsgQGroupCkptDataT *qCkptData, const ClCachedCkptDataT *inData);
ClRcT clMsgQGroupCkptDataUnmarshal(ClMsgQGroupCkptDataT *qCkptData, const ClCachedCkptDataT *inData);
void clMsgQGroupCkptDataFree(ClMsgQGroupCkptDataT *qCkptData);
ClBoolT clMsgQGroupCkptQueueExist(ClMsgQGroupCkptDataT *qCkptData, SaNameT *pQueueName, ClUint32T *pPos);

#ifdef	__cplusplus
}
#endif

#endif	/* __CL_MSG_CKPT_DATA_H__ */

