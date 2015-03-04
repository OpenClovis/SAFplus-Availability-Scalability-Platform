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
 * ModuleName  : msg                                                          
 * File        : clMsgCkptServer.h
 *******************************************************************************/

#ifndef __CL_MSG_CKPT_SERVER_H__
#define	__CL_MSG_CKPT_SERVER_H__

#include <clMsgCommon.h>
#include <clMsgCkptData.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern ClCachedCkptSvcInfoT gMsgQCkptServer;
extern ClCachedCkptSvcInfoT gMsgQGroupCkptServer;

ClRcT clMsgQCkptInitialize(void);
ClRcT clMsgQCkptFinalize(void);
ClRcT clMsgQCkptSynch(void);
ClBoolT clMsgQCkptExists(const SaNameT *pQName, ClMsgQueueCkptDataT *pQueueData);
ClBoolT clMsgQGroupCkptExists(const SaNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData);
ClRcT clMsgQGroupCkptDataGet(const SaNameT *pQGroupName, ClMsgQGroupCkptDataT *pQGroupData);

ClRcT clMsgQCkptDataUpdate(ClMsgSyncActionT syncupType, ClMsgQueueCkptDataT *pQueueData, ClBoolT updateCkpt);
ClRcT clMsgQGroupCkptDataUpdate(ClMsgSyncActionT syncupType, SaNameT *pGroupName, SaMsgQueueGroupPolicyT policy, ClIocPhysicalAddressT qGroupAddress, ClBoolT updateCkpt);
ClRcT clMsgQGroupMembershipCkptDataUpdate(ClMsgSyncActionT syncupType, SaNameT *pGroupName, SaNameT *pQueueName, ClBoolT updateCkpt);

void clMsgQCkptCompDown(ClIocAddressT *pAddr);
void clMsgQCkptNodeDown(ClIocAddressT *pAddr);
void clMsgQGroupCkptNodeDown(ClIocAddressT *pAddr);
void clMsgFailoverQueuesMove(ClIocNodeAddressT destNode, ClUint32T *pNumOfOpenQs);
#ifdef	__cplusplus
}
#endif

#endif	/* __CL_MSG_CKPT_SERVER_H__ */

