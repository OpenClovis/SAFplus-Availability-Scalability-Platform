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
 * ModuleName  : amf
 * File        : clAmsSAClientRmd.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * High-level RMD functions, used by the Ams API to talk to the server.
 * This should be input to an IDL compiler that autogenerates the implementation
 * code.
 *****************************************************************************/

#ifndef _CL_AMS_SA_CLIENT_RMD_H_
#define _CL_AMS_SA_CLIENT_RMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>

#include <clAmsSACommon.h>

/******************************************************************************
 * Constants and Functions to receive async call from the server
 *****************************************************************************/

extern ClRcT cl_ams_client_initialize(
        CL_IN   clAmsClientInitializeRequestT   *req,
        CL_OUT  clAmsClientInitializeResponseT **res);

extern ClRcT cl_ams_client_finalize(
        CL_IN   clAmsClientFinalizeRequestT   *req,
        CL_OUT  clAmsClientFinalizeResponseT **res);

extern ClRcT cl_ams_client_csi_ha_state_get(
        CL_IN   clAmsClientCSIHAStateGetRequestT   *req,
        CL_OUT  clAmsClientCSIHAStateGetResponseT **res);

extern ClRcT cl_ams_client_csi_quiescing_complete(
        CL_IN   clAmsClientCSIQuiescingCompleteRequestT   *req,
        CL_OUT  clAmsClientCSIQuiescingCompleteResponseT **res);

extern ClRcT cl_ams_client_pg_track(
        CL_IN   clAmsClientPGTrackRequestT   *req,
        CL_OUT  clAmsClientPGTrackResponseT **res);

extern ClRcT cl_ams_client_pg_track_stop(
        CL_IN   clAmsClientPGTrackStopRequestT   *req,
        CL_OUT  clAmsClientPGTrackStopResponseT **res);

extern ClRcT cl_ams_client_response(
        CL_IN   clAmsClientResponseRequestT   *req,
        CL_OUT  clAmsClientResponseResponseT **res);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _CL_AMS_SA_CLIENT_RMD_H_
