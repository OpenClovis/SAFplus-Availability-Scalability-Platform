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
 * File        : clFaultRepairHandlers.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file provides the definitions for the fault 
 * repair handling.
 *
 *
 ******************************************************************************/

/**
 *  \file
 *  \brief Header file of Definitions for the Fault Repair Handling.
 *  \ingroup fault_apis
 */

/**
 *  \addtogroup fault_apis
 *  \{
 */

#ifndef _CL_FAULT_REPAIR_HANDLERS_H_
#define _CL_FAULT_REPAIR_HANDLERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clClistApi.h>
#include <clFaultDefinitions.h>
#include <saAmf.h>


/**
 * This is the function pointer type for repair handlers.
 */
typedef ClRcT(*ClFaultRepairHandlerT)(ClAlarmInfoT *pAlarmInfo,
		SaAmfRecommendedRecoveryT recoveryAction);

/**
 * The ClFaultRepairHandlerTableT structure is used to capture the repair handlers per
 * MoClassType.
 */

typedef struct ClFaultRepairHandlerTable
{
    ClCorClassTypeT MoClassType;
    ClFaultRepairHandlerT repairHandler;
}ClFaultRepairHandlerTableT;



#ifdef __cplusplus
}
#endif

#endif	/*	 _CL_FAULT_REPAIR_HANDLERS_H_ */

/** \} */

