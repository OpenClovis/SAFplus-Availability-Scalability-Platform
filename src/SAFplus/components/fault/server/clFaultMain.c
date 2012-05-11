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
 * File        : clFaultMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides the definitions for the creation of a fault
 *                   manager EO
 *****************************************************************************/
#define __SERVER__                          
/* system includes */
#include <string.h>
#include <unistd.h>

/* ASP includes */
#include <clCommon.h>
#include <clCpmApi.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clRmdApi.h>
#include <clDebugApi.h>
#include <clVersion.h>

#include <clCorApi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clCorNotifyApi.h>
#include <clAlarmIpi.h>

/* fault includes */
#include <clFaultDebug.h>
#include <clFaultApi.h>
#include <clFaultServerIpi.h>
#include <clFaultClientServerCommons.h>
#include <clFaultLog.h>
#include <clFaultRepairHandlers.h>
#include <clIdlApi.h>
#include <clXdrApi.h>
#include <../idl/xdr/xdrClFaultVersionInfoT.h>
#include <clFaultClientIpi.h>

/* globals */
ClFaultSeqTblT ***faultactiveSeqTbls;
ClEoExecutionObjT gFmEoObj;

ClUint32T clFaultLocalProbationPeriod=10;

ClFaultSeqTblT  **fmSeqTbls[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};


/* static */
static ClNameT sFaultCompName;
static ClUint32T sFaultCompId;
static ClCpmHandleT cpmHandle;
extern ClFaultRepairHandlerTableT gRepairHdlrList[];

static ClRcT 
VDECL (clFaultReportProcess)(
        ClEoDataT         data,
        ClBufferHandleT   inMsgHdl,
        ClBufferHandleT   outMsgHdl);

static ClRcT
VDECL (clFaultServerRepairAction)(
        ClEoDataT         data,
        ClBufferHandleT   inMsgHdl,
        ClBufferHandleT   outMsgHdl);


#undef __CLIENT__
#include <clFaultServerFuncTable.h>

/******************************************************************
 * Description:  This receives the fault event attirbutes and this 
 *                  can be used by AM/CPM/CM ro report the faults.
 *                 This API will make an RMD call to FM or AMS based 
 *                 on name service query.
 * Arguments: The arguments are as defined in the RMD receive calls
 *
 *
 ******************************************************************/
static ClRcT
VDECL (clFaultReportProcess)(
    ClEoDataT         data,
    ClBufferHandleT   inMsgHdl,
    ClBufferHandleT   outMsgHdl)
{
    ClRcT          rc = CL_OK;
    ClUint32T      length = 0;
    ClFaultEventPtr      hFaultEvent = NULL;
    ClFaultRecordPtr   hFaultRecord = NULL;
	
    clLogDebug("SER", NULL, "Received report request");
    hFaultRecord  = (ClFaultRecordT*)clHeapAllocate(sizeof(ClFaultRecordT));
	if(NULL == hFaultRecord)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while allocating the memory. rc[0x%x]", CL_ERR_NO_MEMORY));
		return CL_ERR_NO_MEMORY;
	}

    rc = clBufferLengthGet(inMsgHdl, &length);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nException occurred in getting the \
                    lenght of incoming message\n"));
        return rc;
    }          

    /* need to consider the size of structure and the buffer data associated with it. */
    hFaultEvent   = (ClFaultEventT*)clHeapAllocate(length);
	if(NULL == hFaultEvent)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while allocating the memory. rc[0x%x]", CL_ERR_NO_MEMORY));
		return CL_ERR_NO_MEMORY;
	}

    rc = clBufferNBytesRead(inMsgHdl, (ClUint8T *)hFaultEvent, &length);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not read from the incoming message\n"));
        return rc;
    }
    hFaultRecord->event             = *hFaultEvent ;

    (hFaultRecord->event).category=
        clFaultCategory2InternalTranslate((hFaultRecord->event).category);
    (hFaultRecord->event).severity=
        clFaultSeverity2InternalTranslate((hFaultRecord->event.severity));
    
    rc = clFaultRepairProcess(hFaultRecord);
    if(CL_OK != rc)
    {
        clLogError("FLT", NULL,
                "Error in reporting fault, rc [0x%x]",
                rc);
    }
	clHeapFree(hFaultEvent);
	clHeapFree(hFaultRecord);
    return rc;

}

/******************************************************************
 * Description:  This receives the fault event attirbutes and calls 
 *                  the fault's repair process
 * Arguments: The arguments are as defined in the RMD receive calls
 *
 *
 ******************************************************************/
static ClRcT 
VDECL (clFaultServerRepairAction)( 
        ClEoDataT         data,
        ClBufferHandleT   inMsgHdl,
        ClBufferHandleT   outMsgHdl)
{
    ClRcT          rc = CL_OK;
#if 0    // Fault manager deprecated
    ClAlarmInfoT  *pAlarmInfo=NULL;
    ClCorClassTypeT type=(ClCorClassTypeT )0;
    ClUint32T      index=0;
    ClUint8T bFound =CL_FALSE;
    ClFaultVersionInfoT *faultVersionInfo = NULL;
    
    faultVersionInfo = clHeapAllocate(sizeof(ClFaultVersionInfoT));
    if (NULL == faultVersionInfo)
    {
        clLogError("RAC", "EOC", "Failed to allocate the memory for the fault version info");
        return CL_FAULT_RC(CL_ERR_NO_MEMORY);
    }
    
    rc = VDECL_VER(clXdrUnmarshallClFaultVersionInfoT, 4, 0, 0)(inMsgHdl,faultVersionInfo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nException occurred in getting the \
                    lenght of incoming message\n"));
        clHeapFree(faultVersionInfo);
        return rc;
    }          

    clFaultClientToServerVersionValidate(faultVersionInfo->version);
        
    rc = clAlarmClientResourceInfoGet(faultVersionInfo->alarmHandle, &pAlarmInfo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO,("clFaultAlarmHandleInfoGet was not \
					successful,rc : %0x \n",rc));
        clHeapFree(pAlarmInfo);
        clHeapFree(faultVersionInfo);
        return rc;
    }

    rc=clCorMoIdToClassGet(&(pAlarmInfo->moId),CL_COR_MO_CLASS_GET,&type);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not malloc memory for alarmInfo\n"));
        clHeapFree(pAlarmInfo);
        clHeapFree(faultVersionInfo);
        return rc;
    }

    /* call the repair handler */

    clLogTrace("REP", "EXP", "Found the class type as [0x%x] ", type);

    for(index=0;(gRepairHdlrList[index].MoClassType != 0) ;index++)
    {
        if(type == gRepairHdlrList[index].MoClassType)
        {
            bFound= CL_TRUE;
            if(gRepairHdlrList[index].repairHandler != NULL)
            {
                rc = gRepairHdlrList[index].repairHandler(pAlarmInfo,faultVersionInfo->recoveryAction);
                if (CL_OK != rc)
                    clLogError("REP", NULL, 
                            "Failure in the repair-action callback function of index [%d], rc[0x%x]", index, rc);
            }
            break;
        }
    }

    if( CL_FALSE==bFound)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultServerRepairAction: Could not find appropriate repair handler\n"));
    }

    clHeapFree(pAlarmInfo);
    clHeapFree(faultVersionInfo);
#endif
    return rc;
}


/*************************************************************************/
/******************** Functions needed by EO infrastructure **************/
/*************************************************************************/
                                                                                                                             
ClRcT   clFaultServiceTerminate(ClInvocationT invocation,
                    const ClNameT  *compName)
{
    ClRcT rc=CL_OK;
    
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_DEREGISTER,"CPM");
        return rc;
    }
    rc = clCpmClientFinalize(cpmHandle);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_FINALIZATION,"Cpm Client");
        return rc;
    }
    clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;
}
                                                                                                                             
                                                                                                                             
ClRcT   clFaultCpmRegister()
{
    ClCpmCallbacksT callbacks;
    ClVersionT  version = CL_EVENT_VERSION;
    ClIocPortT  iocPort;
    ClRcT       rc = CL_OK;

    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = clFaultServiceTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;
                                                                                                                             
    clEoMyEoIocPortGet(&iocPort);
    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_INITIALIZATION,"Cpm Client");
        return rc;
    }

    rc = clCpmComponentNameGet(cpmHandle, &sFaultCompName);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCpmComponentNameGet failed [%x]",rc));
        return rc;
    }    
    
    rc = clCpmComponentRegister(cpmHandle, &sFaultCompName, NULL);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_REGISTER,"CPM");
        return rc;
    }    
    
    /* get the compId for Fault */

    rc = clCpmComponentIdGet(cpmHandle, &sFaultCompName, &sFaultCompId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCpmComponentIdGet failed [%x]",rc));
        return rc;
    }

    return rc;
}



ClRcT clFaultInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT* eoObj = NULL;            

    CL_FUNC_ENTER();

    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultInitialize:  clEoMyEoObjectGet  failed rc:%x.... \n", rc));
        return rc;    
    }

    rc = clFaultCpmRegister();
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clFaultCpmRegister failed with rc:%x \n", rc));
        return rc;
    }

    rc = clFaultNativeClientTableInit(eoObj);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clFaultNativeClientTableInit failed with rc:%x \n", rc));
        return rc;
    }

    rc = clFaultDebugRegister(eoObj);
    if(CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_ERROR,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_REGISTER,"Debug");
        return rc;
    }    

    clFaultHistoryInit(clFaultLocalProbationPeriod);
    faultactiveSeqTbls=fmSeqTbls;
    
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, CL_FAULT_SERVER_LIB,
            CL_LOG_MESSAGE_1_SERVICE_STARTED, "Fault" );

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT clFaultNativeClientTableInit(ClEoExecutionObjT* eoObj)
{

    ClRcT rc = CL_OK;

    memcpy(&gFmEoObj, eoObj, sizeof(ClEoExecutionObjT));

    if (gFmEoObj.rmdObj == 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("****** RMD obj is NULL \n"));
                                                                                          
    rc = clEoClientInstallTables(&gFmEoObj, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, FLT));
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n fmGlobal: Native clEoClientInstall failed \n"));
    }

    rc = clFaultSvcLibInitialize();
    if(CL_OK != rc)
    {
        clLogError("SER", NULL,
                "Error in fault client initialization, rc [0x%x]", rc);
    }

    return rc;
}

ClRcT clFaultFinalize()
{
    ClRcT rc =CL_OK;
    ClEoExecutionObjT *pEOObj = NULL;

    rc = clEoMyEoObjectGet(&pEOObj);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoMyEoObjectGet failed rc :[%x] \n", rc));
        return rc;
    }        
            
    rc = clFaultDebugDeregister(pEOObj);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_ERROR,CL_FAULT_SERVER_LIB,CL_FAULT_LOG_1_DEREGISTER,"Debug");
        return rc;
    }

    rc = clEoClientUninstall(pEOObj, CL_EO_NATIVE_COMPONENT_TABLE_ID);
    if (CL_OK != rc)
    {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoClientUninstall failed rc :[%x] \n", rc));
       return rc;
    }    
    
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE,
            CL_FAULT_SERVER_LIB,CL_LOG_MESSAGE_1_SERVICE_STOPPED, "Fault" );
    
    return rc;
}


ClRcT   clFaultStateChange(ClEoStateT eoState)
{
   return CL_OK;
}

ClRcT   clFaultHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
   schFeedback->freq = CL_EO_DEFAULT_POLL;
   schFeedback->status = 0;
   return CL_OK;
}

ClEoConfigT clEoConfig = {
    "FLT",          /* EO Name*/
    1,              /* EO Thread Priority */
    1,              /* No of EO thread needed */
    CL_IOC_FAULT_PORT,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    clFaultInitialize,  /* Function CallBack  to initialize the Application */
    clFaultFinalize,    /* Function Callback to Terminate the Application */ 
    clFaultStateChange, /* Function Callback to change the Application state */
    clFaultHealthCheck, /* Function Callback to change the Application state */
    NULL
    };

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,            /*  osal   */
    CL_TRUE,            /*  timer  */
    CL_TRUE,            /*  buffer */
    CL_TRUE,            /*  ioc    */
    CL_TRUE,            /*  rmd    */
    CL_TRUE,            /*  eo     */
    CL_FALSE,           /*  om     */
    CL_FALSE,           /*  hal    */
    CL_FALSE,           /*  dbal   */
};
  
ClUint8T clEoClientLibs[] = {
    CL_FALSE,           /*  cor   */
    CL_FALSE,           /*  cm    */
    CL_FALSE,           /*  name  */
    CL_TRUE,            /*  log   */
    CL_FALSE,           /*  trace */
    CL_FALSE,           /*  diag  */
    CL_FALSE,           /*  txn   */
    CL_FALSE,           /*  hpi   */
    CL_FALSE,           /*  cli   */
    CL_FALSE,           /*  alarm */
    CL_TRUE,            /*  debug */
    CL_FALSE,           /*  gms   */
    CL_FALSE,           /* pm */
};
