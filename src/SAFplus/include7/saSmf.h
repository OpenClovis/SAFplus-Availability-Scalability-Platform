/*******************************************************************************
**
** FILE:
**   saSmf.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Software Management Framework (SMF). It contains  
**   all the prototypes and type definitions required for SMF. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-SMF-A.01.01
**
** DATE: 
**   Mon  Sept   5  2007
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2006 by the Service Availability Forum. All rights reserved.
** 
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
** 
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/


#ifndef _SA_SMF_H
#define _SA_SMF_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaSmfHandleT;
typedef SaUint32T SaSmfCallbackScopeIdT;

typedef enum{
	SA_SMF_UPGRADE = 1,
	SA_SMF_ROLLBACK = 2
} SaSmfPhaseT;

typedef enum{
	SA_SMF_ROLLING = 1,
	SA_SMF_SINGLE_STEP = 2
} SaSmfUpgrMethodT;

typedef enum {							
   SA_SMF_CMPG_INITIAL =1,
   SA_SMF_CMPG_EXECUTING = 2,
   SA_SMF_CMPG_SUSPENDING_EXECUTION = 3,
   SA_SMF_CMPG_EXECUTION_SUSPENDED = 4,
   SA_SMF_CMPG_EXECUTION_COMPLETED = 5,
   SA_SMF_CMPG_CAMPAIGN_COMMITTED = 6,
   SA_SMF_CMPG_ERROR_DETECTED = 7,
   SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED = 8,
   SA_SMF_CMPG_ERROR_DETECTED_IN_SUSPENDING = 9,
   SA_SMF_CMPG_EXECUTION_FAILED = 10,
   SA_SMF_CMPG_ROLLING_BACK = 11,
   SA_SMF_CMPG_SUSPENDING_ROLLBACK = 12,
   SA_SMF_CMPG_ROLLBACK_SUSPENDED = 13,
   SA_SMF_CMPG_ROLLBACK_COMPLETED = 14,
   SA_SMF_CMPG_ROLLBACK_COMMITTED = 15,
   SA_SMF_CMPG_ROLLBACK_FAILED = 16
} SaSmfCmpgStateT;

typedef enum {							
   SA_SMF_PROC_INITIAL =1,
   SA_SMF_PROC_EXECUTING = 2,
   SA_SMF_PROC_SUSPENDED = 3,
   SA_SMF_PROC_COMPLETED = 4,
   SA_SMF_PROC_STEP_UNDONE = 5,
   SA_SMF_PROC_FAILED = 6,
   SA_SMF_PROC_ROLLING_BACK = 7,
   SA_SMF_PROC_ROLLBACK_SUSPENDED = 8,
   SA_SMF_PROC_ROLLED_BACK = 9,
   SA_SMF_PROC_ROLLBACK_FAILED = 10
} SaSmfProcStateT;

typedef enum {							
   SA_SMF_STEP_INITIAL =1,
   SA_SMF_STEP_EXECUTING = 2,
   SA_SMF_STEP_UNDOING = 3,
   SA_SMF_STEP_COMPLETED = 4,
   SA_SMF_STEP_UNDONE = 5,
   SA_SMF_STEP_FAILED = 6,
   SA_SMF_STEP_ROLLING_BACK = 7,
   SA_SMF_STEP_UNDOING_ROLLBACK = 8,
   SA_SMF_STEP_ROLLED_BACK = 9,
   SA_SMF_STEP_ROLLBACK_UNDONE = 10,
   SA_SMF_STEP_ROLLBACK_FAILED = 11
} SaSmfStepStateT;

typedef enum {							
   SA_SMF_CAMPAIGN_STATE =1,
   SA_SMF_PROCEDURE_STATE = 2,
   SA_SMF_STEP_STATE = 3
} SaSmfStateT;

typedef enum {
	SA_SMF_ENTITY_NAME = 1
} SaSmfEntityInfoT;

typedef enum {
	SA_SMF_PREFIX_FILTER = 1,
	SA_SMF_SUFFIX_FILTER = 2,
	SA_SMF_EXACT_FILTER = 3,
	SA_SMF_PASS_ALL_FILTER = 4
} SaSmfLabelFilterTypeT;

typedef struct {
	SaSizeT labelSize;
	SaUint8T *label;
} SaSmfCallbackLabelT;

typedef struct {
	SaSmfLabelFilterTypeT filterType;
	SaSmfCallbackLabelT filter;
} SaSmfLabelFilterT;

typedef struct {
	SaSizeT filtersNumber;
	SaSmfLabelFilterT *filters;
} SaSmfLabelFilterArrayT;


typedef void (*SaSmfCampaignCallbackT) (
		SaSmfHandleT smfHandle,
		SaInvocationT invocation,
		SaSmfCallbackScopeIdT scopeId,
		const SaNameT *objectName,
		SaSmfPhaseT phase,
		const SaSmfCallbackLabelT *callbackLabel,
		const SaStringT params);

typedef struct {
	SaSmfCampaignCallbackT saSmfCampaignCallback;
} SaSmfCallbacksT;


/*************************************************/
/************ Defs for SMF Admin API *************/
/*************************************************/

typedef enum {      
   SA_SMF_ADMIN_EXECUTE = 1,
   SA_SMF_ADMIN_ROLLBACK = 2,
   SA_SMF_ADMIN_SUSPEND = 3,
   SA_SMF_ADMIN_COMMIT = 4
} SaSmfAdminOperationIdT;


/*************************************************/
/******** SMF API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saSmfInitialize(
	SaSmfHandleT *smfHandle,
	const SaSmfCallbacksT *smfCallbacks,
	SaVersionT *version);

extern SaAisErrorT 
saSmfSelectionObjectGet(
	SaSmfHandleT smfHandle,
	SaSelectionObjectT *selectionObject);

extern SaAisErrorT
saSmfDispatch(
	SaSmfHandleT smfHandle,
	SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saSmfFinalize(
	SaSmfHandleT smfHandle);

extern SaAisErrorT 
saSmfCallbackScopeRegister(
	SaSmfHandleT smfHandle,
	SaSmfCallbackScopeIdT scopeId,
	const SaSmfLabelFilterArrayT *scopeOfInterest);

extern SaAisErrorT 
saSmfCallbackScopeUnregister(
	SaSmfHandleT smfHandle,
	SaSmfCallbackScopeIdT scopeId);

extern SaAisErrorT 
saSmfResponse(
	SaSmfHandleT smfHandle,
	SaInvocationT invocation,
	SaAisErrorT error);


#ifdef  __cplusplus
}
#endif

#endif  /* _SA_SMF_H */
