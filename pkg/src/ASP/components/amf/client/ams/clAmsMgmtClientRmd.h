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
 * ModuleName  : amf
 * File        : clAmsMgmtClientRmd.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * High-level RMD functions, used by the Ams API to talk to the server.
 * This should be input to an IDL compiler that autogenerates the implementation
 * code.
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_CLIENT_RMD_H_
#define _CL_AMS_MGMT_CLIENT_RMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>

#include <clAmsMgmtCommon.h>
#include <clAmsXdrHeaderFiles.h>
/******************************************************************************
 * Constants and Functions to receive async call from the server
 *****************************************************************************/

#define CL_AMS_CLIENT_CALLBACKS   5 /* Number of registered callbacks + 1 */

extern ClEoPayloadWithReplyCallbackT
        cl_Ams_client_callback_list[CL_AMS_CLIENT_CALLBACKS];
    
extern ClRcT cl_ams_mgmt_initialize(
        CL_IN   clAmsMgmtInitializeRequestT   *req,
        CL_OUT  clAmsMgmtInitializeResponseT **res);

extern ClRcT cl_ams_mgmt_finalize(
        CL_IN   clAmsMgmtFinalizeRequestT   *req,
        CL_OUT  clAmsMgmtFinalizeResponseT **res);

extern ClRcT cl_ams_mgmt_entity_instantiate(
        CL_IN   clAmsMgmtEntityInstantiateRequestT   *req,
        CL_OUT  clAmsMgmtEntityInstantiateResponseT **res);

extern ClRcT cl_ams_mgmt_entity_terminate(
        CL_IN   clAmsMgmtEntityTerminateRequestT   *req,
        CL_OUT  clAmsMgmtEntityTerminateResponseT **res);

extern ClRcT cl_ams_mgmt_entity_set_config(
        CL_IN   clAmsMgmtEntitySetConfigRequestT   *req,
        CL_OUT  clAmsMgmtEntitySetConfigResponseT **res);

extern ClRcT cl_ams_mgmt_entity_set_alpha_factor(
        CL_IN   clAmsMgmtEntitySetAlphaFactorRequestT   *req,
        CL_OUT  clAmsMgmtEntitySetAlphaFactorResponseT **res);

extern ClRcT cl_ams_mgmt_entity_set_beta_factor(
        CL_IN   clAmsMgmtEntitySetBetaFactorRequestT   *req,
        CL_OUT  clAmsMgmtEntitySetBetaFactorResponseT **res);

extern ClRcT cl_ams_mgmt_entity_lock_assignment(
        CL_IN   clAmsMgmtEntityLockAssignmentRequestT   *req,
        CL_OUT  clAmsMgmtEntityLockAssignmentResponseT **res);

extern ClRcT cl_ams_mgmt_entity_force_lock(
        CL_IN   clAmsMgmtEntityForceLockRequestT   *req,
        CL_OUT  clAmsMgmtEntityForceLockResponseT **res);

extern ClRcT cl_ams_mgmt_entity_lock_instantiation(
        CL_IN   clAmsMgmtEntityLockInstantiationRequestT   *req,
        CL_IN   ClBoolT forceFlag,
        CL_OUT  clAmsMgmtEntityLockInstantiationResponseT **res);

extern ClRcT cl_ams_mgmt_entity_unlock(
        CL_IN   clAmsMgmtEntityUnlockRequestT   *req,
        CL_OUT  clAmsMgmtEntityUnlockResponseT **res);

extern ClRcT cl_ams_mgmt_entity_shutdown(
        CL_IN   clAmsMgmtEntityShutdownRequestT   *req,
        CL_OUT  clAmsMgmtEntityShutdownResponseT **res);

extern ClRcT cl_ams_mgmt_entity_shutdown_with_restart(
        CL_IN   clAmsMgmtEntityShutdownRequestT   *req,
        CL_OUT  clAmsMgmtEntityShutdownResponseT **res);

extern ClRcT cl_ams_mgmt_entity_restart(
        CL_IN   clAmsMgmtEntityRestartRequestT   *req,
        CL_OUT  clAmsMgmtEntityRestartResponseT **res);

extern ClRcT cl_ams_mgmt_entity_repaired(
        CL_IN   clAmsMgmtEntityRepairedRequestT   *req,
        CL_OUT  clAmsMgmtEntityRepairedResponseT **res);

extern ClRcT cl_ams_mgmt_sg_adjust_preference(
        CL_IN   clAmsMgmtSGAdjustPreferenceRequestT   *req,
        CL_OUT  clAmsMgmtSGAdjustPreferenceResponseT **res);

extern ClRcT cl_ams_mgmt_si_swap (
        CL_IN   clAmsMgmtSISwapRequestT   *req,
        CL_OUT  clAmsMgmtSISwapResponseT **res);

extern ClRcT cl_ams_mgmt_sg_adjust (
        CL_IN   clAmsMgmtSGAdjustPreferenceRequestT *req,
        CL_OUT  clAmsMgmtSGAdjustPreferenceResponseT **res);

extern ClRcT cl_ams_mgmt_entity_list_entity_ref_add (
        CL_IN   clAmsMgmtEntityListEntityRefAddRequestT   *req,
        CL_OUT  clAmsMgmtEntityListEntityRefAddResponseT **res);

extern ClRcT _clAmsMgmtReadTLV(
            CL_IN   ClBufferHandleT     buf,
            CL_IN   ClAmsTLVT                  *tlv );

extern ClRcT
cl_ams_mgmt_entity_set_ref(
        CL_IN   clAmsMgmtEntitySetRefRequestT   *req,
        CL_OUT  clAmsMgmtEntitySetRefResponseT **res);

extern ClRcT
cl_ams_mgmt_csi_set_nvp(
        CL_IN   clAmsMgmtCSISetNVPRequestT   *req,
        CL_OUT  clAmsMgmtCSISetNVPResponseT **res);

extern ClRcT
cl_ams_mgmt_debug_enable(
        CL_IN   clAmsMgmtDebugEnableRequestT   *req,
        CL_OUT  clAmsMgmtDebugEnableResponseT **res);

extern ClRcT
cl_ams_mgmt_debug_disable(
        CL_IN   clAmsMgmtDebugDisableRequestT   *req,
        CL_OUT  clAmsMgmtDebugDisableResponseT **res);

extern ClRcT
cl_ams_mgmt_debug_get(
        CL_IN   clAmsMgmtDebugGetRequestT   *req,
        CL_OUT  clAmsMgmtDebugGetResponseT **res);

extern ClRcT
cl_ams_mgmt_debug_enable_log_to_console(
        CL_IN   clAmsMgmtDebugEnableLogToConsoleRequestT   *req,
        CL_OUT  clAmsMgmtDebugEnableLogToConsoleResponseT **res);

extern ClRcT
cl_ams_mgmt_debug_disable_log_to_console(
        CL_IN   clAmsMgmtDebugDisableLogToConsoleRequestT   *req,
        CL_OUT  clAmsMgmtDebugDisableLogToConsoleResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_initialize(
        CL_IN   clAmsMgmtCCBInitializeRequestT   *req,
        CL_OUT  clAmsMgmtCCBInitializeResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_finalize(
        CL_IN   clAmsMgmtCCBFinalizeRequestT *req,
        CL_OUT  clAmsMgmtCCBFinalizeResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_entity_set_config(
        CL_IN   clAmsMgmtCCBEntitySetConfigRequestT   *req,
        CL_OUT  clAmsMgmtCCBEntitySetConfigResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_csi_set_nvp(
        CL_IN   clAmsMgmtCCBCSISetNVPRequestT   *req,
        CL_OUT  clAmsMgmtCCBCSISetNVPResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_csi_delete_nvp(
        CL_IN   clAmsMgmtCCBCSIDeleteNVPRequestT   *req,
        CL_OUT  clAmsMgmtCCBCSIDeleteNVPResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_node_dependency(
        CL_IN   clAmsMgmtCCBSetNodeDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetNodeDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_node_dependency(
        CL_IN   clAmsMgmtCCBSetNodeDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetNodeDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_node_su_list(
        CL_IN   clAmsMgmtCCBSetNodeSUListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetNodeSUListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_node_su_list(
        CL_IN   clAmsMgmtCCBSetNodeSUListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetNodeSUListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_sg_su_list(
        CL_IN   clAmsMgmtCCBSetSGSUListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSGSUListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_sg_su_list(
        CL_IN   clAmsMgmtCCBSetSGSUListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSGSUListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_sg_si_list(
        CL_IN   clAmsMgmtCCBSetSGSIListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSGSIListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_sg_si_list(
        CL_IN   clAmsMgmtCCBSetSGSIListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSGSIListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_su_comp_list(
        CL_IN   clAmsMgmtCCBSetSUCompListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSUCompListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_su_comp_list(
        CL_IN   clAmsMgmtCCBSetSUCompListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSUCompListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_si_su_rank_list(
        CL_IN   clAmsMgmtCCBSetSISURankListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSISURankListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_si_su_rank_list(
        CL_IN   clAmsMgmtCCBSetSISURankListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSISURankListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_si_si_dependency(
        CL_IN   clAmsMgmtCCBSetSISIDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSISIDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_csi_csi_dependency(
        CL_IN   clAmsMgmtCCBSetCSICSIDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetCSICSIDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_si_si_dependency(
        CL_IN   clAmsMgmtCCBSetSISIDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSISIDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_csi_csi_dependency(
        CL_IN   clAmsMgmtCCBSetCSICSIDependencyRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetCSICSIDependencyResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_set_si_csi_list(
        CL_IN   clAmsMgmtCCBSetSICSIListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSICSIListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_delete_si_csi_list(
        CL_IN   clAmsMgmtCCBSetSICSIListRequestT   *req,
        CL_OUT  clAmsMgmtCCBSetSICSIListResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_enable_entity(
        CL_IN   clAmsMgmtCCBEnableEntityRequestT   *req,
        CL_OUT  clAmsMgmtCCBEnableEntityResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_disable_entity(
        CL_IN   clAmsMgmtCCBDisableEntityRequestT   *req,
        CL_OUT  clAmsMgmtCCBDisableEntityResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_entity_create(
        CL_IN   clAmsMgmtCCBEntityCreateRequestT   *req,
        CL_OUT  clAmsMgmtCCBEntityCreateResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_entity_delete(
        CL_IN   clAmsMgmtCCBEntityDeleteRequestT   *req,
        CL_OUT  clAmsMgmtCCBEntityDeleteResponseT **res);

extern ClRcT
cl_ams_mgmt_ccb_commit(
        CL_IN   clAmsMgmtCCBCommitRequestT   *req,
        CL_OUT  clAmsMgmtCCBCommitResponseT **res);

extern ClRcT
cl_ams_mgmt_entity_get(
        CL_IN   clAmsMgmtEntityGetRequestT   *req,
        CL_OUT  clAmsMgmtEntityGetResponseT **res);


extern ClRcT
cl_ams_mgmt_entity_get_config(
        CL_IN   clAmsMgmtEntityGetConfigRequestT   *req,
        CL_OUT  clAmsMgmtEntityGetConfigResponseT **res);

extern ClRcT
cl_ams_mgmt_entity_get_status(
        CL_IN   clAmsMgmtEntityGetStatusRequestT   *req,
        CL_OUT  clAmsMgmtEntityGetStatusResponseT **res);

extern ClRcT
cl_ams_mgmt_get_csi_nvp_list(
        CL_IN   clAmsMgmtGetCSINVPListRequestT   *req,
        CL_OUT  clAmsMgmtGetCSINVPListResponseT **res);

extern ClRcT
cl_ams_mgmt_get_entity_list(
        CL_IN   clAmsMgmtGetEntityListRequestT   *req,
        CL_OUT  clAmsMgmtGetEntityListResponseT **res);

extern ClRcT
cl_ams_mgmt_get_ol_entity_list(
        CL_IN   clAmsMgmtGetOLEntityListRequestT   *req,
        CL_OUT  clAmsMgmtGetOLEntityListResponseT **res);

extern ClRcT
clAmsGetEntitySize(
        CL_IN  ClAmsEntityTypeT  *entityType,
        CL_OUT  ClUint32T  *configSize,
        CL_OUT  ClUint32T  *statusSize,
        CL_OUT  ClUint32T  *entitySize );


extern ClRcT
cl_ams_mgmt_migrate_sg(ClAmsMgmtMigrateRequestT *request,
                       ClAmsMgmtMigrateResponseT *response);

extern ClRcT
cl_ams_mgmt_user_data_set(ClAmsMgmtUserDataSetRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_setkey(ClAmsMgmtUserDataSetRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_get(ClAmsMgmtUserDataGetRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_getkey(ClAmsMgmtUserDataGetRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_delete(ClAmsMgmtUserDataDeleteRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_deletekey(ClAmsMgmtUserDataDeleteRequestT *req);

extern ClRcT
cl_ams_mgmt_user_data_deleteall(ClAmsMgmtUserDataDeleteRequestT *req);

extern ClRcT 
cl_ams_mgmt_si_assign_su_custom(ClAmsMgmtSIAssignSUCustomRequestT *req);

extern ClRcT
cl_ams_mgmt_db_get(ClAmsMgmtDBGetResponseT *res);

extern ClRcT
cl_ams_mgmt_cas_get(CL_OUT ClAmsMgmtCASGetRequestT *req);

extern ClRcT
__marshalClAmsMgmtCCBEntitySetConfig(
                                     CL_IN  ClPtrT  ptr,
                                     CL_INOUT  ClBufferHandleT  buf,
                                     ClUint32T versionCode);

extern
ClRcT cl_ams_ccb_batch_rmd(
                           CL_IN ClUint32T  fn_id,  
                           CL_IN ClBufferHandleT buffer,
                           CL_IN ClUint32T versionCode);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_MGMT_CLIENT_RMD_H_ */
