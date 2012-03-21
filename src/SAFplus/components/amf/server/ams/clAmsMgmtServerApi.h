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
 * File        : clAmsMgmtServerApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS management
 * API.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_SERVER_API_H_
#define _CL_AMS_MGMT_SERVER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/
#include <clEoApi.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtHooks.h>
#include <clAmsInvocation.h>

extern ClRcT
VDECL(_clAmsMgmtInitialize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtFinalize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntityCreate)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntityDelete)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 4, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 4, 1, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 5, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntitySetAlphaFactor)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntitySetBetaFactor)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntityLockAssignment)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntityForceLock)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT ClBufferHandleT  out_buffer);

extern ClRcT
VDECL_VER(_clAmsMgmtEntityForceLockInstantiation, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT ClBufferHandleT  out_buffer);

extern ClRcT
VDECL_VER(_clAmsMgmtCCBBatchCommit, 5, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT ClBufferHandleT  out_buffer);

extern ClRcT
VDECL(_clAmsMgmtEntityLockInstantiation)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in_buffer,
        CL_OUT  ClBufferHandleT  out_buffer);

extern ClRcT 
VDECL(_clAmsMgmtEntityUnlock)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtEntityShutdown)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtEntityRestart)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtEntityRepaired)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtSGAdjustPreference)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtSGAssignSUtoSG)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtSISwap)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtEntityListEntityRefAdd)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtEntitySetRef)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT 
VDECL(_clAmsMgmtCSISetNVP)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);


extern ClRcT
VDECL(_clAmsMgmtEntityGetEntityList)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtDebugEnable)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtDebugDisable)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtDebugGet)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtDebugEnableLogToConsole)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtDebugDisableLogToConsole)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out);

extern ClRcT
VDECL(_clAmsMgmtCCBInitialize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL(_clAmsMgmtCCBFinalize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL(_clAmsMgmtCCBEntityCreate)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL(_clAmsMgmtCCBEntityDelete)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 4, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 4, 1, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 5, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL(_clAmsMgmtCCBCSISetNVP)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL(_clAmsMgmtCCBCSIDeleteNVP)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetNodeDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );


extern ClRcT 
VDECL(_clAmsMgmtCCBSetNodeSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSGSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSGSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSUCompList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSISURankList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSISIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetCSICSIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBSetSICSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteNodeDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteNodeSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSGSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSGSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSUCompList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSISURankList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSISIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteCSICSIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBDeleteSICSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtCCBCommit)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT clAmsMgmtCommitCCBOperations(
        CL_IN  ClCntHandleT opListHandle );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 5, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 5, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtGetCSINVPList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtGetEntityList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtGetOLEntityList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );


extern ClRcT 
VDECL(_clAmsMgmtMigrateSG)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataSet)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataSetKey)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataGet)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataGetKey)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataDelete)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtEntityUserDataDeleteKey)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT 
VDECL(_clAmsMgmtSIAssignSUCustom)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL_VER(_clAmsMgmtDBGet, 5, 1, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

extern ClRcT
VDECL_VER(_clAmsMgmtComputedAdminStateGet, 5, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out );

#ifdef __cplusplus
}
#endif

#endif							/* _CL_AMS_MGMT_SERVER_API_H_ */ 
