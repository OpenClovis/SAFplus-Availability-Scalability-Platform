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
 * ModuleName  : fault
 * File        : clFaultLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides the definitions for predefined log messages
 *				 for fault component
 *				
 *
 *
 *****************************************************************************/

#ifndef _CL_FAULT_LOG_H_
#define _CL_FAULT_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>
#define CL_FAULT_LOG_MAX_MSG 6	
#define CL_FAULT_LOG_1_RMD_MSG 100	

#define CL_FAULT_CLIENT_LIB     "fault_client"
#define CL_FAULT_SERVER_LIB    	NULL 

extern ClCharT *clFaultLogMsg[];

#define	CL_FAULT_LOG_1_RMD 			clFaultLogMsg[0] /* Fault Log messages for
														RMD failure */
#define	CL_FAULT_LOG_1_MUTEX 			clFaultLogMsg[1] /* Fault log messages
														for error in mutex
														creation/lock/unlock
														*/
#define	CL_FAULT_LOG_1_REGISTER 		clFaultLogMsg[2] /* Fault log messages
														when registration
														failed */
#define	CL_FAULT_LOG_1_DEREGISTER 	clFaultLogMsg[3] /* Fault log messages
														when deregistration
														failed */
#define	CL_FAULT_LOG_1_INITIALIZATION clFaultLogMsg[4] /* Fault log messages
														when initialization
														failed */
#define	CL_FAULT_LOG_1_FINALIZATION 	clFaultLogMsg[5] /* Fault log messages
														when finalization
														failed */
	
#ifdef __cplusplus
}
#endif


#endif /* _CL_FAULT_LOG_H_ */
