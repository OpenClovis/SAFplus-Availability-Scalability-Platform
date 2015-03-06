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
 * ModuleName  : fault
 * File        : clFaultDebug.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains extern declarations of the Api's which
 * are going to interact with the debug component
 *
 *
 *****************************************************************************/
#ifndef _CL_FAULT_DEBUG_H_
#define _CL_FAULT_DEBUG_H_
                                                                                                                             
#ifdef __cplusplus
extern "C" {
#endif

	
#include <clCorMetaData.h>
		                                                                                                                             
	extern ClRcT clFaultDebugRegister(ClEoExecutionObjT* pEoObj);
	extern ClRcT clFaultDebugDeregister(ClEoExecutionObjT* pEoObj);
	extern ClRcT clFaultCliDebugGenerateFault(ClUint32T argc, ClCharT **argv, ClCharT** ret);
	extern ClRcT clFaultCliDebugHistoryShow(ClUint32T argc, ClCharT **argv, ClCharT** ret);
	extern ClRcT clFaultCliDebugCompleteHistoryShow(ClUint32T argc, ClCharT **argv, ClCharT** ret);
	extern ClRcT clFaultXlateMOPath(ClCharT *path, ClCorMOIdPtrT cAddr);
	                                                                                                                             
#ifdef __cplusplus
}
#endif
                                                                                                                             
                                                                                                                             
#endif /* _CL_FAULT_DEBUG_H_ */

