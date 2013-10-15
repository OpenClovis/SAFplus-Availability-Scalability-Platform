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
 * File        : clFaultClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file provides the definitions for the faultNotifyRmd,
 * fault report and fault escalation APIs.
 *
 *****************************************************************************/

#undef __SERVER__
#define __CLIENT__
/* system includes */
#include <string.h>	
#include <time.h>

/* ASP includes */
#include <clCommon.h>
#include <clRmdApi.h>
#include <clDebugApi.h> 
#include <clCpmApi.h>
#include <clNameApi.h>

/* fault includes */
#include <clFaultApi.h>
#include "clFaultClientServerCommons.h"
#include "clFaultLog.h"
#include <clFaultClientIpi.h>
#include <clAlarmDefinitions.h>
#include <clIdlApi.h>
#include <clXdrApi.h>
#include <xdrClFaultVersionInfoT.h>
#include <clFaultServerFuncTable.h>
#include <clCorAmf.h>

#define FAULT_LOG_AREA_FAULT	"FLT"
#define FAULT_LOG_CTX_VERSION	"VER"
#define FAULT_LOG_CTX_RMD	"RMD"
#define FAULT_LOG_CTX_ACTION	"ACT"
/**
 *  Determine the supported fault Version.  
 *
 *  This Api checks the version is supported or not. If the version check fails,
 *	the closest version is passed as the out parameter.
 *                                                                        
 *  @param version[IN/OUT] : version value.
 *
 *  @returns CL_OK  - Success<br>
 *    On failure, the error CL_FAULT_ERR_VERSION_UNSUPPORTED is returned with the 
 * 	  closest value of the version supported filled in the version as out parameter.
 */
ClRcT 
clFaultVersionVerify(ClVersionT *version)
{
    ClRcT rc = CL_OK;
	if(version == NULL)
	{
		clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_VERSION,"NULL pointer passed as version \
		           address.with rc: 0x%x", rc);
		return CL_ERR_NULL_POINTER;
	}

    CL_FUNC_ENTER();

	rc = clVersionVerify(&gFaultClientToServerVersionDb,version);
	if(rc != CL_OK)
	{
		clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_VERSION,"The Cor Version is not supported. 0x%x", rc);
		return CL_FAULT_ERR_VERSION_UNSUPPORTED;
	}
    CL_FUNC_EXIT();
    return rc;
}


ClRcT
clFaultReport(SaNameT *compName,
                ClCorMOIdPtrT hMoId,
				ClAlarmStateT alarmState,
 				ClAlarmCategoryTypeT category, 
				ClAlarmSpecificProblemT specificProblem,
				ClAlarmSeverityTypeT severity, 
                ClAlarmProbableCauseT cause,
                void *pData,
                ClUint32T len)
{

	ClRcT rc= CL_OK ;
	ClIocAddressT iocAddress;

	/* have a #ifdef for finding out AMS/FM for faultservice */

	iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
	iocAddress.iocPhyAddress.portId = CL_IOC_FAULT_PORT;
			
	rc = clFaultClientIssueFaultRmd(compName,
									 hMoId,
									 alarmState,
							         category,
							         specificProblem,
							         severity,
							         cause,
							         1, /* doRepairAction */
							         pData,
							         len,
							         iocAddress,
							         CL_FAULT_REPORT_API_HANDLER);
	if (CL_OK != rc )
    {
         clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"clFaultClientIssueFaultRmd failed \
	             with rec:%x\n", rc);
    }
	
	return rc;

}

/**
 * To be used by AMS to inform FM about the fault
 */
/* mahesh. added this function to provide repair interface */

ClRcT clFaultRepairAction(ClIocAddressT iocAddress,ClAlarmHandleT alarmHandle,ClUint32T recoveryActionTaken)
{
	ClRcT rc= CL_OK ;
	ClFaultVersionInfoT *faultVersionInfo;
    ClBufferHandleT inMsgHandle;
    ClRmdOptionsT opts = CL_RMD_DEFAULT_OPTIONS;

	faultVersionInfo = clHeapAllocate((ClUint32T)sizeof(ClFaultVersionInfoT));
	if(faultVersionInfo == NULL)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_ACTION,"Memory allocation failed \n");
        return CL_ERR_NO_MEMORY;
    }
	CL_FAULT_VERSION_SET(faultVersionInfo->version);
	faultVersionInfo->alarmHandle = alarmHandle;
	faultVersionInfo->recoveryAction = recoveryActionTaken;

	/*
	   This is a check being made for the validity of the alarm handle.
	 */
	if(alarmHandle == 0)
	{
        clHeapFree(faultVersionInfo);
        return CL_FAULT_RC(CL_ERR_INVALID_HANDLE);
	}

	iocAddress.iocPhyAddress.portId = CL_IOC_FAULT_PORT;

	rc = clBufferCreate(&inMsgHandle);
	if (rc != CL_OK )
    {
         clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_ACTION,"Could not create buffer for \
		     message\n");
		 clHeapFree(faultVersionInfo);
         return rc;
    }
	
	//rc = clBufferNBytesWrite(inMsgHandle,(void*)&alarmversionInfo,sizeof(ClAlarmVersionInfoT));
    rc = VDECL_VER(clXdrMarshallClFaultVersionInfoT, 4, 0, 0)(faultVersionInfo,inMsgHandle,0);
    if (CL_OK != rc)
    {
        /* Free buffer allocated above */
        clBufferDelete(&inMsgHandle);
		clHeapFree(faultVersionInfo);
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_ACTION,"Could not write into Buffer\n");
        return rc;
    }
	
    opts.retries  = CL_FAULT_RMDCALL_RETRIES;
    opts.timeout  = CL_FAULT_RMDCALL_TIMEOUT;
    opts.priority = CL_RMD_DEFAULT_PRIORITY;

   /* rmd call to fm server */
	rc = clRmdWithMsg (iocAddress, 
		               CL_FAULT_REPAIR_API_HANDLER,
		               inMsgHandle,
		               (ClBufferHandleT)NULL,
	  		 	       CL_RMD_CALL_ATMOST_ONCE|CL_RMD_CALL_ASYNC,
					   &opts,	
					   NULL );
	if(rc != CL_OK)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"\n RMD call to FM server failed with rc:0x%x",rc);
	}
	
	clBufferDelete(&inMsgHandle);	
	clHeapFree(faultVersionInfo);
	return rc;

}

ClRcT clFaultRepairNotification(ClAmsNotificationDescriptorT *notification,
                                ClIocAddressT iocAddress,
                                ClAlarmHandleT alarmHandle,
                                ClUint32T recovery)
{
    ClCorMOIdT moId;

    if(!notification) return CL_FAULT_RC(CL_ERR_INVALID_PARAMETER);

    return clCorAmfMoIdGet((const ClCharT*)notification->entityName.value, notification->entityType, &moId);
}


ClRcT clFaultClientIssueFaultRmd(SaNameT *compName,
	                    ClCorMOIdPtrT hMoId,
						ClAlarmStateT alarmState,
				 		ClAlarmCategoryTypeT category, 
		                ClAlarmSpecificProblemT specificProblem,
                        ClAlarmSeverityTypeT severity, 
                        ClAlarmProbableCauseT cause,
		                ClUint32T recoveryActionTaken,
                        void *pData,
                        ClUint32T len,
                        ClIocAddressT iocAddress,
                        ClUint32T      apiId)
{

	ClRcT rc= CL_OK ;
    ClBufferHandleT inMsgHandle;
    ClRmdOptionsT opts = CL_RMD_DEFAULT_OPTIONS;
	ClFaultEventPtr hFaultEvent;	
	
	/* 
	 *  This rmd message string now will make use of
	 *  format specifiers to include component name 
	 *  in the log write message
	 *  A default max value is being provided to 
	 *  store this message
	 */
	/*ClCharT *rmdmsg=clHeapAllocate(CL_FAULT_LOG_1_RMD_MSG*sizeof(ClCharT));*/
		
	if (hMoId == NULL)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"clFmFaultPublish ERROR: MOID is NULL for the fault \r\n");
		return CL_FAULT_RC(CL_FAULT_ERR_MOID_NULL);
    }
	
	if (compName == NULL)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"clFmFaultPublish ERROR: compname\
			being passed to fault is NULL\r\n");
		return CL_FAULT_RC(CL_FAULT_ERR_COMPNAME_NULL);
    }
	
	rc = clFaultValidateCategory(category);
    if (rc != CL_OK)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"clFmFaultPublish ERROR: Invalid Category for\
					the fault:0x%x\r\n",category);
		return CL_FAULT_RC(CL_FAULT_ERR_INVALID_CATEGORY);
    }
	
	rc = clFaultValidateSeverity(severity);
    if (rc != CL_OK)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"clFmFaultPublish ERROR: Invalid severity for \
					the fault:0x%x\r\n",severity);
		return CL_FAULT_RC(CL_FAULT_ERR_INVALID_SEVERITY);
    }
    opts.retries  = CL_FAULT_RMDCALL_RETRIES;
    opts.timeout  = CL_FAULT_RMDCALL_TIMEOUT;
    opts.priority = CL_RMD_DEFAULT_PRIORITY;
	
	if((hFaultEvent = (ClFaultEventPtr)clHeapAllocate(sizeof(ClFaultEventT)+len  ))== NULL)
	{
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"Memory allocation failed \n");
        return CL_ERR_NO_MEMORY;
    }
	memset(hFaultEvent,0,sizeof(ClFaultEventT)+len);
    
	/* copy the event attributes into the member variable event of 
	   the fault structure */
	hFaultEvent->compName   = *compName;
	hFaultEvent->moId       = *hMoId;
    hFaultEvent->alarmState = alarmState;
    hFaultEvent->category   = category;
    hFaultEvent->specificProblem= specificProblem;	
    hFaultEvent->severity   = severity;
    hFaultEvent->cause      = cause;
    hFaultEvent->recoveryActionTaken = recoveryActionTaken;


	hFaultEvent->addInfoLen = ((pData == NULL) ? 0 :len);
    if (pData)
	    memcpy( hFaultEvent->additionalInfo , pData , len );	   

	rc = clBufferCreate(&inMsgHandle);
	if (rc != CL_OK )
    {
         clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"Could not create buffer for \
					 message\n");
         return rc;
    }
	
	rc = clBufferNBytesWrite(inMsgHandle,(void *)hFaultEvent,sizeof(ClFaultEventT)+len);
    if (CL_OK != rc)
    {
        /* Free buffer allocated above */
        clBufferDelete(&inMsgHandle);
        clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"Could not write into Buffer\n");
        return rc;
    }
	
	
	rc = clRmdWithMsg (iocAddress, 
		               apiId,
		               inMsgHandle,
		               (ClBufferHandleT)NULL,
                       CL_RMD_CALL_ATMOST_ONCE,
                       &opts,	
                       NULL );
	if(rc != CL_OK)
	{
		clLogError(FAULT_LOG_AREA_FAULT,FAULT_LOG_CTX_RMD,"\nRMD Call to FM server failed with rc:0x%x",rc);
		/*
		sprintf(rmdmsg,"Fault Manager Local/Global depending on the context \
				when trying to restart %s",compName->value);
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_SEV_ERROR,CL_FAULT_CLIENT_LIB,CL_FAULT_LOG_1_RMD,rmdmsg);*/
	}
	
	clBufferDelete(&inMsgHandle);	
	clHeapFree(hFaultEvent);	
	/*clHeapFree(rmdmsg);*/
	return rc;

}


ClRcT clFaultSvcLibInitialize(void)
{
	ClRcT rc = CL_OK;	
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, FLT), CL_IOC_FAULT_PORT);
    if(CL_OK != rc)
    {
        clLogError("FLT", NULL, 
                "Error in initializing fault library, rc [0x%x]", rc);
        return rc;
    }
	return rc;
}
ClRcT clFaultSvcLibFinalize(void)
{
	return CL_OK;
}

