#ifndef _CL_GMS_SERVER_FUNC_TABLE_H_
#define _CL_GMS_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clGmsEoFunc.h>
#include <clGmsCommon.h>

CL_EO_CALLBACK_TABLE_DECL(gmsEOFuncList)[] =
{

    VSYM_EMPTY(NULL,                          CL_GMS_CLIENT_RESERVED),                 /* 0  */

    /* Session APIs  function pointers for client library */

    VSYM(cl_gms_initialize_rmd,               CL_GMS_CLIENT_INITIALIZE),               /* 1  */
    VSYM(cl_gms_finalize_rmd,                 CL_GMS_CLIENT_FINALIZE),                 /* 2  */

    /* PAssive Cluster/Group View function pointers fpr client library */

    VSYM(cl_gms_cluster_track_rmd,            CL_GMS_CLIENT_CLUSTER_TRACK),            /* 3  */
    VSYM(cl_gms_cluster_track_stop_rmd,       CL_GMS_CLIENT_CLUSTER_TRACK_STOP),       /* 4  */
    VSYM(cl_gms_cluster_member_get_rmd,       CL_GMS_CLIENT_CLUSTER_MEMBER_GET),       /* 5  */
    VSYM(cl_gms_cluster_member_get_async_rmd, CL_GMS_CLIENT_CLUSTER_MEMBER_GET_ASYNC), /* 6  */
    VSYM(cl_gms_group_list_get_rmd,           CL_GMS_CLIENT_GROUP_LIST_GET),           /* 7  */
    VSYM(cl_gms_group_track_rmd,              CL_GMS_CLIENT_GROUP_TRACK),              /* 8  */
    VSYM(cl_gms_group_track_stop_rmd,         CL_GMS_CLIENT_GROUP_TRACK_STOP),         /* 9  */
    VSYM(cl_gms_group_member_get_rmd,         CL_GMS_CLIENT_GROUP_MEMBER_GET),         /* 10 */
    VSYM(cl_gms_group_member_get_async_rmd,   CL_GMS_CLIENT_GROUP_MEMBER_GET_ASYNC),   /* 11 */

    /* Cluster Manage APIs  function pointers for client library */

    VSYM(cl_gms_cluster_join_rmd,             CL_GMS_CLIENT_CLUSTER_JOIN),             /* 12 */
    VSYM(cl_gms_cluster_leave_rmd,            CL_GMS_CLIENT_CLUSTER_LEAVE),            /* 13 */
    VSYM(cl_gms_cluster_leader_elect_rmd,     CL_GMS_CLIENT_CLUSTER_LEADER_ELECT),     /* 14 */
    VSYM(cl_gms_cluster_member_eject_rmd,     CL_GMS_CLIENT_CLUSTER_MEMBER_EJECT),     /* 15 */

    /* Group Management function pointers for client library */

    VSYM(cl_gms_group_create_rmd,             CL_GMS_CLIENT_GROUP_CREATE),             /* 16 */
    VSYM(cl_gms_group_destroy_rmd,            CL_GMS_CLIENT_GROUP_DESTROY),            /* 17 */
    VSYM(cl_gms_group_join_rmd,               CL_GMS_CLIENT_GROUP_JOIN),               /* 18 */
    VSYM(cl_gms_group_leave_rmd,              CL_GMS_CLIENT_GROUP_LEAVE),              /* 19 */
    VSYM(cl_gms_group_leader_elect_rmd,       CL_GMS_CLIENT_GROUP_LEADER_ELECT),       /* 20 */
    VSYM(cl_gms_group_member_eject_rmd,       CL_GMS_CLIENT_GROUP_MEMBER_EJECT),       /* 21 */

    /* Cluster Management function pointers for peer(server) to peer
     * messaging */

    VSYM(cl_gms_server_receieve_rmd,          CL_GMS_SERVER_TOKEN),                    /* 22 */
    VSYM(cl_gms_server_receieve_rmd,          CL_GMS_SERVER_MCAST),                    /* 23 */
    VSYM(cl_gms_group_info_get_rmd,           CL_GMS_CLIENT_GROUP_INFO_GET),           /* 24 */
    VSYM(cl_gms_comp_up_notify_rmd,           CL_GMS_COMP_UP_NOTIFY),                  /* 25 */
    VSYM(cl_gms_group_mcast_send_rmd,         CL_GMS_CLIENT_GROUP_MCAST_SEND),         /* 26 */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, GMS) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, gmsEOFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}   
#endif

#endif

