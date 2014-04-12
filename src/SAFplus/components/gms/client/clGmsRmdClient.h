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
 * ModuleName  : gms
 * File        : clGmsRmdClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * High-level RMD functions, used by the GMS API to talk to the server.
 * This should be input to an IDL compiler that autogenerates the implementation
 * code.
 *
 *
 *****************************************************************************/

#ifndef _CL_GMS_RMD_CLIENT_H_
#define _CL_GMS_RMD_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>

#include <clGmsCommon.h>

/******************************************************************************
 * Constants and Functions to receive async call from the server
 *****************************************************************************/
#define CL_GMS_CLIENT_CALLBACKS   7 /* Number of registered callbacks + 1 */
extern ClEoPayloadWithReplyCallbackT
                        cl_gms_client_callback_list[CL_GMS_CLIENT_CALLBACKS];

/******************************************************************************
 * High Level RMD Functions to initiate RMD calls to server
 *****************************************************************************/
extern ClRcT cl_gms_cluster_track_rmd(
        CL_IN   ClGmsClusterTrackRequestT            *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterTrackResponseT          **res);

extern ClRcT cl_gms_cluster_track_stop_rmd(
        CL_IN   ClGmsClusterTrackStopRequestT        *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterTrackStopResponseT      **res);

extern ClRcT cl_gms_cluster_member_get_rmd(
        CL_IN   ClGmsClusterMemberGetRequestT        *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterMemberGetResponseT      **res);

extern ClRcT cl_gms_cluster_member_get_async_rmd(
        CL_IN   ClGmsClusterMemberGetAsyncRequestT   *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterMemberGetAsyncResponseT **res);
/*---------------------------------------------------------------------------*/
extern ClRcT cl_gms_group_track_rmd(
        CL_IN   ClGmsGroupTrackRequestT              *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupTrackResponseT            **res);

extern ClRcT cl_gms_group_track_stop_rmd(
        CL_IN   ClGmsGroupTrackStopRequestT          *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupTrackStopResponseT        **res);

extern ClRcT cl_gms_group_member_get_rmd(
        CL_IN   ClGmsGroupMemberGetRequestT          *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupMemberGetResponseT        **res);

extern ClRcT cl_gms_group_member_get_async_rmd(
        CL_IN   ClGmsGroupMemberGetAsyncRequestT     *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupMemberGetAsyncResponseT   **res);
/*---------------------------------------------------------------------------*/
extern ClRcT cl_gms_cluster_join_rmd(
        CL_IN   ClGmsClusterJoinRequestT             *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterJoinResponseT           **res);

extern ClRcT cl_gms_cluster_leave_rmd(
        CL_IN   ClGmsClusterLeaveRequestT            *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterLeaveResponseT          **res);

extern ClRcT cl_gms_cluster_leave_rmd_native(
        CL_IN   ClGmsClusterLeaveRequestT            *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterLeaveResponseT          **res);

extern ClRcT cl_gms_cluster_leader_elect_rmd(
        CL_IN   ClGmsClusterLeaderElectRequestT      *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterLeaderElectResponseT    **res);

extern ClRcT cl_gms_cluster_member_eject_rmd(
        CL_IN   ClGmsClusterMemberEjectRequestT      *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsClusterMemberEjectResponseT    **res);
/*---------------------------------------------------------------------------*/
extern ClRcT cl_gms_group_create_rmd(
        CL_IN   ClGmsGroupCreateRequestT               *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupCreateResponseT             **res);

extern ClRcT cl_gms_group_destroy_rmd(
        CL_IN   ClGmsGroupDestroyRequestT              *req,
        CL_IN   ClUint32T                               timeout, /* [ms] */
        CL_OUT  ClGmsGroupDestroyResponseT            **res);

extern ClRcT cl_gms_group_join_rmd(
        CL_IN   ClGmsGroupJoinRequestT               *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupJoinResponseT             **res);

extern ClRcT cl_gms_group_leave_rmd(
        CL_IN   ClGmsGroupLeaveRequestT              *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupLeaveResponseT            **res);

extern ClRcT cl_gms_group_list_get_rmd(
        CL_IN   ClGmsGroupsInfoListGetRequestT       *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupsInfoListGetResponseT     **res);

extern ClRcT cl_gms_group_info_get_rmd(
        CL_IN   ClGmsGroupInfoGetRequestT          *req,
        CL_IN   ClUint32T                           timeout, /* [ms] */
        CL_OUT  ClGmsGroupInfoGetResponseT        **res);
    
extern ClRcT cl_gms_group_mcast_send_rmd(
        CL_IN   ClGmsGroupMcastRequestT          *req,
        CL_IN   ClUint32T                         timeout, /* [ms] */
        CL_OUT  ClGmsGroupMcastResponseT        **res);

extern ClRcT cl_gms_comp_up_notify_rmd(
        CL_IN   ClGmsCompUpNotifyRequestT          *req,
        CL_IN   ClUint32T                           timeout, /* [ms] */
        CL_OUT  ClGmsCompUpNotifyResponseT        **res);

extern ClRcT cl_gms_group_leader_elect_rmd(
        CL_IN   ClGmsGroupLeaderElectRequestT        *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupLeaderElectResponseT      **res);

extern ClRcT cl_gms_group_member_eject_rmd(
        CL_IN   ClGmsGroupMemberEjectRequestT        *req,
        CL_IN   ClUint32T                             timeout, /* [ms] */
        CL_OUT  ClGmsGroupMemberEjectResponseT      **res);
extern ClRcT 
    cl_gms_clientlib_initialize_rmd ( 
            CL_IN   ClGmsClientInitRequestT *req,  /* Input data from server */
            CL_IN   ClUint32T               timeout,     /* Unused */
            CL_OUT  ClGmsClientInitResponseT **res /* Never used in callbacks */
            );

#ifdef __cplusplus
}
#endif

#endif /* _CL_GMS_RMD_CLIENT_H_ */
