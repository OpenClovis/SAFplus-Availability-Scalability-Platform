#ifndef _CL_GMS_SERVER_FUNC_TABLE_H_
#define _CL_GMS_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clGmsCommon.h>

ClRcT VDECL (cl_gms_cluster_track_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

ClRcT VDECL (cl_gms_cluster_member_get_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

ClRcT VDECL (cl_gms_group_track_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

ClRcT VDECL (cl_gms_cluster_member_eject_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

ClRcT VDECL (cl_gms_group_mcast_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

CL_EO_CALLBACK_TABLE_DECL(gmsCallbackFuncList)[] = 
{
    VSYM_EMPTY(NULL,                                CL_GMS_CLIENT_RESERVED_CALLBACK),             /* 0 */
    VSYM(cl_gms_cluster_track_callback_rmd,         CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK),        /* 1 */
    VSYM(cl_gms_cluster_member_get_callback_rmd,    CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK),   /* 2 */
    VSYM(cl_gms_group_track_callback_rmd ,          CL_GMS_CLIENT_GROUP_TRACK_CALLBACK),          /* 3 */
    VSYM_EMPTY(NULL,                                CL_GMS_CLIENT_GROUP_MEMBER_GET_CALLBACK),     /* 4 */
    VSYM(cl_gms_cluster_member_eject_callback_rmd,  CL_GMS_CLIENT_CLUSTER_MEMBER_EJECT_CALLBACK), /* 5 */
    VSYM_EMPTY(NULL,                                CL_GMS_CLIENT_GROUP_MEMBER_EJECT_CALLBACK),   /* 6 */
    VSYM(cl_gms_group_mcast_callback_rmd,           CL_GMS_CLIENT_GROUP_MCAST_CALLBACK),          /* 7 */
    VSYM_NULL
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, GMS_Client) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_GMS_CLIENT_TABLE_ID, gmsCallbackFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif

