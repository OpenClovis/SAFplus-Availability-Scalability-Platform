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
 * File        : clFaultClientIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains fault client related IPIs
 *
 *
 *****************************************************************************/

#ifndef _CL_FAULT_CLIENT_IPI_H_
#define _CL_FAULT_CLIENT_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>

/* fault related includes */
#include <clFaultDefinitions.h> 
#include <clFaultErrorId.h>


/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 * The function ID used by the fault client library to make an RMD
 * call to notify the fault server about the fault.
 */
#define CL_FAULT_REPORT_API_HANDLER CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1) 

/**
 * The function ID used by the fault client library to make an RMD
 * call to notify the fault server about the fault.
 */
#define CL_FAULT_REPAIR_API_HANDLER CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2) 
/******************************************************************************
 *  Data Types
 *****************************************************************************/
                                                                                           
/*****************************************************************************
 *  Functions
 *****************************************************************************/

/* forward declarations */
ClRcT clFaultClientIssueFaultRmd(ClNameT *compName,
                        ClCorMOIdPtrT hMoId,
						ClAlarmStateT alarmState,
						ClAlarmCategoryTypeT category, 
						ClAlarmSpecificProblemT specificProblem,
						ClAlarmSeverityTypeT severity, 
                        ClAlarmProbableCauseT cause,
                        ClUint32T doRepairAction,
                        void *pData,
                        ClUint32T len,
                        ClIocAddressT iocAddress,
                        ClUint32T apiId);


/* externs */


#ifdef __cplusplus
}
#endif


                                                                                
#endif /* _CL_FAULT_CLIENT_IPI_H_ */
