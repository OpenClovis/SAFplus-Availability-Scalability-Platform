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
 * ModuleName  : gms
 * File        : clGmsEngine.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Header file for the GMS engine which contains the synchronization protocol
 * for GMS.
 *
 *
 *****************************************************************************/

#ifndef _CL_GMS_ENGINE_H_
#define _CL_GMS_ENGINE_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clGmsCommon.h>

extern ClRcT
_clGmsEngineLeaderElect(
        CL_IN   ClGmsGroupIdT groupId ,
        CL_IN   ClGmsViewNodeT          *node ,
        CL_IN   ClGmsGroupChangesT       cond,
        CL_OUT  ClGmsNodeIdT            *leaderNodeId ,
        CL_OUT  ClGmsNodeIdT            *deputyNodeId 
        );

extern ClRcT _clGmsEngineStart();

ClRcT _clGmsEngineClusterJoin(
        CL_IN   const ClGmsGroupIdT   groupId,
        CL_IN   const ClGmsNodeIdT    nodeId,
        CL_IN   ClGmsViewNodeT* node);

ClRcT _clGmsEngineClusterLeave(
        CL_IN   const ClGmsGroupIdT   groupId,
        CL_IN   const ClGmsNodeIdT    nodeId);

ClRcT _clGmsEngineClusterLeaveExtended(
        CL_IN   const ClGmsGroupIdT   groupId,
        CL_IN   const ClGmsNodeIdT    nodeId,
        CL_IN   ClBoolT viewCache);

ClRcT   _clGmsEngineGroupCreate(
        CL_IN    ClGmsGroupNameT     groupName,
        CL_IN    ClGmsGroupParamsT   groupParams,
        CL_IN    ClUint64T           contextHandle,
        CL_IN    ClInt32T            isLocalMsg);

ClRcT   _clGmsEngineGroupDestroy(
        CL_IN   ClGmsGroupIdT       groupId,
        CL_IN   ClGmsGroupNameT     groupName,
        CL_IN   ClUint64T           contextHandle,
        CL_IN    ClInt32T            isLocalMsg);

ClRcT   _clGmsEngineGroupJoin(
        CL_IN     ClGmsGroupIdT       groupId,
        CL_IN     ClGmsViewNodeT*     node,
        CL_IN     ClUint64T           contextHandle,
        CL_IN    ClInt32T            isLocalMsg);

ClRcT   _clGmsEngineGroupLeave(
        CL_IN    ClGmsGroupIdT       groupId,
        CL_IN    ClGmsMemberIdT      memberId,
        CL_IN    ClUint64T           contextHandle,
        CL_IN    ClInt32T            isLocalMsg);

ClRcT   _clGmsRemoveAllMembersOnShutdown (
        CL_IN   ClGmsNodeIdT   nodeId);

ClRcT   _clGmsRemoveMemberOnCompDeath(
        CL_IN   ClGmsMemberIdT     memberId);

ClRcT
_clGmsEnginePreferredLeaderElect(
        CL_IN   ClGmsClusterMemberT  preferredLeaderNode,
        /* Suppressing coverity warning for pass by value with below comment */
        // coverity[pass_by_value]
        CL_IN   ClUint64T            contextHandle,
        CL_IN   ClInt32T             isLocalMsg);
    
ClRcT
    _clGmsEngineGroupInfoSync(ClGmsGroupSyncNotificationT *syncNotification);

extern int clGmsSendSyncMsg (ClGmsGroupSyncNotificationT *syncNotification);

extern ClUint32T   gmsOpenAisEnable;

extern int  aisexec_main(int, char**);

extern void  totempg_finalize(void);

extern ClRcT
    clGmsGroupLeaveHandler(
            CL_IN   ClGmsGroupLeaveRequestT                   *req,
            CL_OUT  ClGmsGroupLeaveResponseT                  *res);

ClRcT _clGmsEngineClusterLeaveWrapper(
        CL_IN   ClGmsGroupIdT   groupId,
        CL_IN   ClCharT*        nodeIp);
extern ClRcT   
    _clGmsEngineMcastMessageHandler(ClGmsGroupMemberT *groupMember,
                                    ClGmsGroupInfoT   *groupData,
                                    ClUint32T          userDataSize,
                                    ClPtrT             data);
#ifdef  __cplusplus
}
#endif
            
#endif /* _CL_GMS_ENGINE_H_ */
