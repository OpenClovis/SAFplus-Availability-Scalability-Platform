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
 * ModuleName  : name
 * File        : clNameLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains the information about log messages internal
 * Name Service.
 *
 *
 *****************************************************************************/

#ifndef _CL_NAME_LOG_H_
#define _CL_NAME_LOG_H_
                                                                                                                             
#ifdef __cplusplus
extern "C" {
#endif

/**
 * "NS Context Creation failed, rc=0x%x"
 */
#define CL_NS_LOG_1_CONTEXT_CREATION_FAILED   clNameLogMsg[0]

/**
 * "Not permitted to delete default NS contexts"
 */
#define CL_NS_LOG_0_OPERATION_NOT_PERMITTED   clNameLogMsg[1]

/**
 * "NS Context deletion failed, rc=0x%x"
 */
#define CL_NS_LOG_1_CONTEXT_DELETION_FAILED   clNameLogMsg[2]

/**
 * "NS Registration failed, rc=0x%x"
 */
#define CL_NS_LOG_1_NS_REGISTRATION_FAILED    clNameLogMsg[3]

/**
 * "NS Query Database failed, rc=0x%x"
 */
#define CL_NS_LOG_1_NS_QUERY_FAILED           clNameLogMsg[4]

/**
 * "NS Display request failed, rc=0x%x"
 */
#define CL_NS_LOG_1_NS_DISPLAY_FAILED         clNameLogMsg[5]

/**
 * "NS Service Deregistration failed, rc=0x%x
 */
#define CL_NS_LOG_1_SERVICE_DEREGIS_FAILED    clNameLogMsg[6]

/**
 * "NS Component Deregistration failed, rc=0x%x" 
 */
#define CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED  clNameLogMsg[7]

/**
 * "NS syncup failed, rc=0x%x"
 */
#define CL_NS_LOG_1_SYNCUP_FAILED             clNameLogMsg[8]

/**
 * "NS Initialization  failed, rc=0x%x"
 */
#define CL_NS_LOG_1_NS_INIT_FAILED            clNameLogMsg[9]  

/**
 * "NS Entry Cleanup failed, rc=0x%x"
 */
#define CL_NS_LOG_1_NS_ENTRY_CLEANUP_FAILED   clNameLogMsg[10] 

#define CL_NS_LOG_1_NS_DUPLICATE_ENTRY        clNameLogMsg[11]

#define CL_NS_LOG_1_NS_REGISTER_OVERWRITE_FAILED  clNameLogMsg[12] 
 
                                                                                                                             
#ifdef __cplusplus
}
#endif
                                                                                                                             
                                                                                                                             
#endif /* _CL_NAME_LOG_H_ */
