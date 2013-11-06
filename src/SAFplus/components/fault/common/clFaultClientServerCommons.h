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
 * File        : clFaultClientServerCommons.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides the structure of the fault record
 *				and fault event
 *
 *
 *****************************************************************************/
						

#ifndef _CL_FAULT_CLIENT_SERVER_COMMONS_H_
#define _CL_FAULT_CLIENT_SERVER_COMMONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <../../alarm/include/clAlarmDefinitions.h>
#include <clVersionApi.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
/**
 * RMD call timeout.
 */
#define CL_FAULT_RMDCALL_TIMEOUT 500

/**
 * The number of retries for making the RMD call.
 */
#define CL_FAULT_RMDCALL_RETRIES 2

/* 
 * Structure used for passing alarmhandle information along 
 * with the version information from the fault client to 
 * fault server.
 */

typedef struct{
	ClVersionT version;
	ClAlarmHandleT alarmHandle;
	ClUint32T recoveryAction;
}ClFaultVersionInfoT;

/*
 * Internal Version related definitions
 */
extern ClVersionDatabaseT gFaultClientToServerVersionDb;

#define clFaultClientToServerVersionValidate(version) \
		do\
		{ \
			if(clVersionVerify(&gFaultClientToServerVersionDb, (&version) ) != CL_OK) \
			{ \
			    clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"Failed to validate the client version ."); \
				return CL_FAULT_ERR_VERSION_UNSUPPORTED;  \
			} \
		}while(0) 


#define CL_FAULT_VERSION				{'B', 0x1, 0x1}
#define CL_FAULT_VERSION_SET(version)     \
		do { 							\
			version.releaseCode = 'B' ; \
			version.majorVersion = 0x1; \
			version.minorVersion = 0x1; \
		}while(0)


/* 
 *  extern declaration for validation of fault category and
 *  severity 
 */
extern ClRcT clFaultValidateCategory(ClAlarmCategoryTypeT category);
extern ClRcT clFaultValidateSeverity(ClAlarmSeverityTypeT severity);

/* 
 *  extern declaration for translation of fault category and
 *  severity to internal values
 */
extern ClRcT 
clFaultSeverity2InternalTranslate(ClAlarmSeverityTypeT severity);
extern ClRcT 
clFaultCategory2InternalTranslate(ClAlarmCategoryTypeT category);
	
extern ClRcT 
clFaultCategory2IndexTranslate(ClAlarmCategoryTypeT category, ClUint8T *pCatIndex);
extern ClRcT 
clFaultSeverity2IndexTranslate(ClAlarmSeverityTypeT severity, ClUint8T *pSevIndex);
extern ClRcT 
clFaultInternal2CategoryTranslate(ClAlarmCategoryTypeT category);
extern ClRcT 
clFaultInternal2SeverityTranslate(ClAlarmSeverityTypeT severity);
extern ClRcT 
clFaultValidateMoId(ClCorMOIdPtrT hmoId);

extern ClCharT *clFaultCategoryString[];
extern ClCharT *clFaultSeverityString[];
extern ClCharT *clFaultProbableCauseString[];
#ifdef __cplusplus
}
#endif

#endif		/* _CL_FAULT_CLIENT_SERVER_COMMONS_H_ */
