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
 * File        : clGmsEoFunc.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Contains internal global data structures and function prototypes for the 
 * GMS component.
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_EO_FUNC_H_
#define _CL_GMS_EO_FUNC_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clEoApi.h>

/******************************************************************************
 * RMD functions
 *****************************************************************************/

ClRcT
VDECL (cl_gms_initialize_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_finalize_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_track_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_track_stop_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_member_get_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_member_get_async_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_list_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_info_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_comp_up_notify_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_track_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_track_stop_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_member_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_member_get_async_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_join_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_leave_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_leader_elect_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_cluster_member_eject_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_create_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_destroy_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_join_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_leave_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_leader_elect_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_member_eject_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_server_receieve_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

ClRcT
VDECL (cl_gms_group_mcast_send_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer); /* Never used in callbacks */

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_EO_FUNC_H_ */
