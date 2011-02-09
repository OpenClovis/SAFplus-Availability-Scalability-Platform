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
 * File        : clAmsXdrHeaderFiles.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * header files for xdr marshalling and unmarshalling functions
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_XDR_HEADER_FILES_H
#define _CL_AMS_XDR_HEADER_FILES_H 

#ifdef __cplusplus
extern "C" {
#endif

#include <xdrClAmsMgmtEntitySetConfigRequestT.h>
#include <xdrClAmsMgmtEntityUnlockRequestT.h>
#include <xdrClAmsMgmtEntityRepairedRequestT.h>
#include <xdrClAmsMgmtEntityRestartRequestT.h>
#include <xdrClAmsMgmtEntityShutdownRequestT.h>
#include <xdrClAmsMgmtEntityLockInstantiationRequestT.h>
#include <xdrClAmsMgmtEntityLockAssignmentRequestT.h>
#include <xdrClAmsMgmtSISwapRequestT.h>
#include <xdrClAmsMgmtSGAdjustPreferenceRequestT.h>
#include <xdrClAmsMgmtInitializeRequestT.h>
#include <xdrClAmsMgmtFinalizeRequestT.h>
#include <xdrClAmsMgmtDebugEnableRequestT.h>
#include <xdrClAmsMgmtDebugDisableRequestT.h>
#include <xdrClAmsMgmtDebugGetRequestT.h>
#include <xdrClAmsMgmtDebugGetResponseT.h>
#include <xdrClAmsMgmtDebugEnableLogToConsoleRequestT.h>
#include <xdrClAmsMgmtDebugDisableLogToConsoleRequestT.h>
#include <xdrClAmsMgmtCCBInitializeRequestT.h>
#include <xdrClAmsMgmtCCBInitializeResponseT.h>
#include <xdrClAmsMgmtCCBFinalizeRequestT.h>
#include <xdrClAmsMgmtCCBFinalizeResponseT.h>
#include <xdrClAmsMgmtCCBEntitySetConfigRequestT.h>
#include <xdrClAmsMgmtCCBEntitySetConfigResponseT.h>
#include <xdrClAmsNodeConfigT.h>
#include <xdrClAmsSGConfigT.h>
#include <xdrClAmsSUConfigT.h>
#include <xdrClAmsSIConfigT.h>
#include <xdrClAmsCompConfigT.h>
#include <xdrClAmsCSIConfigT.h>
#include <xdrClAmsCSINVPT.h>
#include <xdrClAmsMgmtCCBCSISetNVPRequestT.h>
#include <xdrClAmsMgmtCCBCSISetNVPResponseT.h>
#include <xdrClAmsMgmtCCBSetNodeDependencyRequestT.h>
#include <xdrClAmsMgmtCCBSetNodeDependencyResponseT.h>
#include <xdrClAmsMgmtCCBSetNodeSUListRequestT.h>
#include <xdrClAmsMgmtCCBSetNodeSUListResponseT.h>
#include <xdrClAmsMgmtCCBSetSGSUListRequestT.h>
#include <xdrClAmsMgmtCCBSetSGSUListResponseT.h>
#include <xdrClAmsMgmtCCBSetSGSIListRequestT.h>
#include <xdrClAmsMgmtCCBSetSGSIListResponseT.h>
#include <xdrClAmsMgmtCCBSetSUCompListRequestT.h>
#include <xdrClAmsMgmtCCBSetSUCompListResponseT.h>
#include <xdrClAmsMgmtCCBSetSISURankListRequestT.h>
#include <xdrClAmsMgmtCCBSetSISURankListResponseT.h>
#include <xdrClAmsMgmtCCBSetSISIDependencyRequestT.h>
#include <xdrClAmsMgmtCCBSetSISIDependencyResponseT.h>
#include <xdrClAmsMgmtCCBSetCSICSIDependencyRequestT.h>
#include <xdrClAmsMgmtCCBSetCSICSIDependencyResponseT.h>
#include <xdrClAmsMgmtCCBSetSICSIListRequestT.h>
#include <xdrClAmsMgmtCCBSetSICSIListResponseT.h>
#include <xdrClAmsMgmtCCBEnableEntityRequestT.h>
#include <xdrClAmsMgmtCCBEnableEntityResponseT.h>
#include <xdrClAmsMgmtCCBDisableEntityRequestT.h>
#include <xdrClAmsMgmtCCBDisableEntityResponseT.h>
#include <xdrClAmsMgmtCCBEntityCreateRequestT.h>
#include <xdrClAmsMgmtCCBEntityCreateResponseT.h>
#include <xdrClAmsMgmtCCBEntityDeleteRequestT.h>
#include <xdrClAmsMgmtCCBEntityDeleteResponseT.h>
#include <xdrClAmsMgmtCCBCommitRequestT.h>
#include <xdrClAmsMgmtCCBCommitResponseT.h>
#include <xdrClAmsMgmtEntityGetRequestT.h>
#include <xdrClAmsMgmtEntityGetResponseT.h>
#include <xdrClAmsNodeT.h>
#include <xdrClAmsSGT.h>
#include <xdrClAmsSUT.h>
#include <xdrClAmsSIT.h>
#include <xdrClAmsCompT.h>
#include <xdrClAmsCSIT.h>
#include <xdrClAmsMgmtEntityGetConfigRequestT.h>
#include <xdrClAmsMgmtEntityGetConfigResponseT.h>
#include <xdrClAmsMgmtEntityGetStatusRequestT.h>
#include <xdrClAmsMgmtEntityGetStatusResponseT.h>
#include <xdrClAmsMgmtGetCSINVPListRequestT.h>
#include <xdrClAmsMgmtGetEntityListRequestT.h>
#include <xdrClAmsMgmtGetOLEntityListRequestT.h>
#include <xdrClAmsSUSIRefT.h>
#include <xdrClAmsSUSIExtendedRefT.h>
#include <xdrClAmsSISURefT.h>
#include <xdrClAmsSISUExtendedRefT.h>
#include <xdrClAmsCompCSIRefT.h>
#include <xdrClAmsCSICompRefT.h>
#include <xdrClAmsNotificationDescriptorT.h>
#include <xdrClAmsMgmtInitializeRequestT.h>
#include <xdrClAmsMgmtInitializeResponseT.h>
#include <xdrClAmsMgmtEntityAdminResponseT.h>
#include <xdrClAmsMgmtEntitySetAlphaFactorRequestT.h>
#include <xdrClAmsMgmtEntitySetBetaFactorRequestT.h>
#include <xdrClAmsEntityBufferT.h>
#include <xdrClAmsMgmtMigrateRequestT.h>
#include <xdrClAmsMgmtMigrateListT.h>
#include <xdrClAmsMgmtMigrateResponseT.h>
#include <xdrClAmsMgmtSIAssignSUCustomRequestT.h>
#include <xdrClAmsInvocationIDLT.h>

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_XDR_HEADER_FILES_H */
