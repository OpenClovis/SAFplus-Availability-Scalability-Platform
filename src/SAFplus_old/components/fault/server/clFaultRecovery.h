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
 * File        : clFaultRecovery.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file has the definitions for the fault recovery                      
 *                                                                           
 *****************************************************************************/
#ifndef _CL_FAULT_RECOVERY_H_
#define _CL_FAULT_RECOVERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clClistApi.h>
#include <clFaultDefinitions.h>

ClRcT 
clFaultRecordShow(ClFaultRecordPtr hRec);

extern ClRcT
clFaultEventHistoryAdd(ClFaultRecordPtr pFE);

/* Returns one of the 5 categories, else INVALID */
extern ClCharT*    
_clFaultCatStr(ClUint32T    category);

/* Returns one of the 7 severities defined, else INVALID */
extern ClCharT*    
_clFaultSevStr(ClUint32T    severity);

/* Returns one of the 57 probable causes, else INVALID */
extern ClCharT*    
_clFaultProbCauseStr(ClUint32T probCause);

#ifdef __cplusplus
}
#endif

#endif	/*	 _CL_FAULT_RECOVERY_H_ */
