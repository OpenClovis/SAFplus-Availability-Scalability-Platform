#ifndef _CL_GMS_CLIENT_EO_FUNC_H_
#define _CL_GMS_CLIENT_EO_FUNC_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>


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
        CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

ClRcT VDECL (cl_gms_group_mcast_callback_rmd) (
        CL_IN   ClEoDataT        c_data,     /* Unused */
        CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
        CL_OUT  ClBufferHandleT  out_buffer);/* Never used */

#ifdef __cplusplus
}
#endif

#endif

