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
 * ModuleName  : alarm
 * File        : clAlarmMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module is the main file of Alarm.
*************************************************************************/

#define __SERVER__

#include <string.h>
#include <unistd.h>

#include <clEventApi.h>
#include <clEventExtApi.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clHalApi.h>
#include <clCorApi.h>
#include <clCpmApi.h>
#if 0
#include <ipi/clSAClientSelect.h>
#endif

#include <clAlarmDebug.h>
#include <clAlarmServer.h>
#include <clAlarmCommons.h>
#include <clAlarmServerAlarmUtil.h>
#include <clAlarmInfoCnt.h>
#include <clAlarmPayloadCont.h>
#include <xdrClAlarmInfoIDLT.h>
#include <clAlarmServerFuncTable.h>

/********************************** Global *****************************/
ClEventInitHandleT        gAlarmInitHandle;
ClEventChannelHandleT     gAlarmEvtChannelHandle;
ClCpmHandleT              gAlarmCpmHandle;

/**********************************************************************/

ClRcT   alarmSvcTerminate(ClInvocationT invocation,
                            const ClNameT  *compName)
{
    ClRcT rc =CL_OK;

    rc = clAlarmAlarmInfoCntDestroy();/* Destroy the alarm Info container */
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n container destroy failed \n"));
        return rc;
    }    

    rc = clAlarmPayloadCntDeleteAll();/* Destroy the alarm Info container */
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n container destroy failed \n"));
        return rc;
    }    


    rc = clCpmComponentUnregister(gAlarmCpmHandle,compName, NULL);
    if(rc != CL_OK )
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_CRITICAL,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_DEREGISTER,
                   "CPM");
        return rc;
    }
    rc = clCpmClientFinalize(gAlarmCpmHandle);
    if(rc != CL_OK )
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_CRITICAL,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_FINALIZATION,
                   "Cpm Client");
        return rc;
    }
    clCpmResponse(gAlarmCpmHandle, invocation, CL_OK);
    return CL_OK;
}


ClRcT alarmServerInitialize (ClUint32T argc, ClCharT *argv[])
   
{
    ClEoExecutionObjT*  pEOObj  =   NULL;
    ClRcT               rc      =   CL_OK;    
    ClCpmCallbacksT     callbacks;
    ClNameT             appName;
    ClVersionT          version =   CL_EVENT_VERSION;

    CL_FUNC_ENTER();

    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = alarmSvcTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    rc = clCpmClientInitialize(&gAlarmCpmHandle, &callbacks, &version);
    if(CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_CRITICAL,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_INITIALIZATION,
                   "Cpm Client");
        return rc;
    }

    rc = clCpmComponentNameGet(gAlarmCpmHandle, &appName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clCpmComponentNameGet failed \n"));
        return rc;
    }    

    rc = clCpmComponentRegister(gAlarmCpmHandle, &appName, NULL);
    if(CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_CRITICAL,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_REGISTER,
                   "CPM");
        return rc;
    }            

    rc = clEoMyEoObjectGet(&pEOObj);
    if(CL_OK != rc)
    {
        clLogError("ALM", "INI", "Failed to get the EO object. rc [0x%x]", rc);
        return rc;
    }
                                                                                                  
    rc = clAlarmClientTableRegister();
    if (rc != CL_OK)
    {
        clLogError("ALM", "INI", "Failed to register the client function table. rc [0x%x]", rc);
        return rc;
    }

    rc = clEoClientInstallTables(pEOObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, ALM)); 
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n AlarmServer: Native clEoClientInstall failed \n"));
        return rc;
    }
    
    rc = clAlarmServerAlarmChannelInit();
    if (CL_OK != rc)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("clAlarmServerAlarmChannelInit failed with rc 0x%x ", rc));
          return (rc);
    }

    rc = clAlarmDebugRegister(pEOObj);
    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_ERROR,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_REGISTER,
                   "Debug");
        return (rc);
    }    
        
    rc = clAlarmAlarmInfoCntCreate(); /* Create the alarm info container */ 
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n container creation failed \n"));
        return rc;
    }    

    rc = clAlarmPayloadCntListCreate(); /* Create the alarm info container */ 
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n container creation failed \n"));
        return rc;
    }    

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, CL_ALARM_SERVER_LIB,
            CL_LOG_MESSAGE_1_SERVICE_STARTED, "Alarm" );

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT clAlarmServerAlarmChannelInit()
{
    ClRcT rc = CL_OK;
    ClVersionT emVersion = CL_EVENT_VERSION; 
    ClNameT channelName = {strlen(ClAlarmEventName), ClAlarmEventName};

    if(CL_OK!=( rc =  clEventInitialize( &gAlarmInitHandle, NULL, &emVersion)))
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_CRITICAL,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_INITIALIZATION,
                   "event");
         /* Over my head. Just pass the error */
         return rc;
    }    

    rc = clEventChannelOpen(gAlarmInitHandle, &channelName, CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL, -1, &gAlarmEvtChannelHandle);
    if (CL_OK != rc)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("clEventChannelOpen failed with rc 0x%x ", rc));
          return (rc);
    }        

    return rc;
}


ClRcT VDECL(clAlarmHandleToInfoGet) (ClEoDataT data, 
                              ClBufferHandleT  inMsgHandle, 
                              ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T inLen = 0; 
    ClAlarmInfoT* pAlarmInfo = NULL;  /* allocate from heap ??*/
    ClAlarmInfoIDLT alarmInfoIDL;
    ClAlarmHandleT alarmHandle;

    clBufferLengthGet(inMsgHandle, &inLen);

    if(inLen !=  sizeof(ClAlarmHandleT))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n AlarmServer: Invalid Buffer Passed \n"));
        CL_FUNC_EXIT();
        return CL_ALARM_RC(CL_ERR_INVALID_BUFFER);
    }

    rc = clXdrUnmarshallClUint32T(inMsgHandle, &alarmHandle);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AIG", "Failed while decoding the alarm handle into host format. rc[0x%x]", rc);
        return rc;
    }
        
    clLogTrace("AMN", "AIG", "The alarm information request reached for alarm handle [%d]", alarmHandle);

    rc = clAlarmAlarmInfoGet(alarmHandle,&pAlarmInfo);
    if (CL_OK != rc)
    {
        clLogInfo("AMN", "AIG", "Failed while getting the alarm information for the \
                        alarm handle [%d]. rc[0x%x] ", alarmHandle, rc);
        return rc;
    }
    
    alarmInfoIDL.probCause = pAlarmInfo->probCause;
    clNameCopy(&alarmInfoIDL.compName, &pAlarmInfo->compName);
    alarmInfoIDL.moId = pAlarmInfo->moId;
    alarmInfoIDL.alarmState = pAlarmInfo->alarmState;
    alarmInfoIDL.category = pAlarmInfo->category;
    alarmInfoIDL.specificProblem = pAlarmInfo->specificProblem;
    alarmInfoIDL.severity = pAlarmInfo->severity;
    alarmInfoIDL.eventTime = pAlarmInfo->eventTime;
    alarmInfoIDL.len = pAlarmInfo->len;
    alarmInfoIDL.buff = pAlarmInfo->buff;

    rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)(&alarmInfoIDL,outMsgHandle, 0);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AIG", "Failed while marshalling the alarm information for the alarm handle [%d]. rc[0x%x]", 
                alarmHandle, rc);
        return rc;
    }

    return rc;
}

ClRcT VDECL(clAlarmAddProcess) (ClEoDataT data, ClBufferHandleT  inMsgHandle,
        ClBufferHandleT  outMsgHandle)
{

    ClRcT rc = CL_OK;
    ClAlarmVersionInfoT* pAlarmVersionInfo;  
    ClUint32T inLen = 0; 
    ClCorObjectHandleT hMSOObj;

    CL_ALARM_FUNC_ENTER();

    clBufferLengthGet(inMsgHandle, &inLen);

    if(inLen >=  sizeof(ClAlarmVersionInfoT))
    {
        pAlarmVersionInfo = (ClAlarmVersionInfoT *)clHeapAllocate(inLen);
    }
    else
    {
        clLogCritical("AMN", "AAD", "AlarmServer: Invalid Buffer Passed \n");
        CL_ALARM_FUNC_EXIT();
        return CL_ALARM_RC(CL_ERR_INVALID_BUFFER);
    }
                                                                                     
    if(pAlarmVersionInfo == NULL)
    {
        clLogCritical("AMN", "AAD", "Version Info is NULL");
        CL_ALARM_FUNC_EXIT();
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    clBufferNBytesRead(inMsgHandle,(ClUint8T*)pAlarmVersionInfo,&inLen);

    clAlarmClientToServerVersionValidate(pAlarmVersionInfo->version);

    rc = clCorObjectHandleGet(&(pAlarmVersionInfo->alarmInfo.moId), &hMSOObj);

    if (CL_OK != rc)
    {
        clLogCritical("AMN", "AAD", "Failed while getting the object \
                handle from MoId. rc[0x%x] ", rc);
        clHeapFree(pAlarmVersionInfo);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }

    rc = clAlarmServerEngineAlarmProcess(hMSOObj, &pAlarmVersionInfo->alarmInfo,outMsgHandle);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AAD", "Failed while processing the alarm at server. rc[0x:%x]", rc);
        clHeapFree(pAlarmVersionInfo);
        CL_ALARM_FUNC_EXIT();
        return rc;
    }
    /* 
     * The pAlarmInfo is not freed as it will mean freeing the data residing
     * within the container.
     */
    clHeapFree(pAlarmVersionInfo); 

    CL_ALARM_FUNC_EXIT();

    return rc;
}


ClRcT VDECL(clAlarmInfoToAlarmHandleGet) (ClEoDataT data, 
                              ClBufferHandleT  inMsgHandle, 
                              ClBufferHandleT  outMsgHandle)
{
    ClRcT   rc = CL_OK;
    ClAlarmInfoIDLT alarmInfoIdl = {0};
    ClAlarmInfoT    alarmInfo = {0};
    ClAlarmHandleT  alarmHandle = CL_HANDLE_INVALID_VALUE;

    clLogTrace("AMN", "AHG", 
            "Inside the function to get the alarm handle from alarm info.");

    rc = VDECL_VER(clXdrUnmarshallClAlarmInfoIDLT, 4, 0, 0)(inMsgHandle, &alarmInfoIdl);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AHG", 
                "Failed to unmarshal the alarm information from input buffer. rc[0x%x]", rc);
        return rc;
    }

    alarmInfo.moId = alarmInfoIdl.moId;
    alarmInfo.probCause = alarmInfoIdl.probCause;
    alarmInfo.specificProblem = alarmInfoIdl.specificProblem;

    rc = clAlarmAlarmInfoToHandleGet(&alarmInfo, &alarmHandle);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AHG", "Failed to get the alam handle given the alarm info. rc[0x%x]", rc);
        return rc;
    }

    clLogDebug ("AMN", "AHG", "Got the alarm handle as [%d]", alarmHandle);

    rc = clXdrMarshallClUint32T(&alarmHandle, outMsgHandle, 0);
    if (CL_OK != rc)
    {
        clLogError("AMN", "AHG", 
                "Failed to marshal the alarm handle in the output buffer. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}
/*****************************************************************************/

/* This function clean up the information which is it has 
1. Deregister with CPM.
2. Deregister with GMS.
3. Remove all the subscription information.
4. Send deregistration request to EM/PM.
5. Remove all the associated information with ECH (Masking/Retention/User of the ECH).
6. Delete mutex for ECH/L & ECH/G
7. Delete DB of ECH/L & ECH/G.

*/
ClRcT alarmServerFinalize()
{
    ClRcT rc =CL_OK;
    ClEoExecutionObjT *pEOObj = NULL;
    /* Uninstall the native table */

    rc = clEoMyEoObjectGet(&pEOObj);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoMyEoObjectGet failed rc :[%x] \n", rc));
        return rc;
    }        

    rc = clAlarmDebugDeregister(pEOObj);
       if (CL_OK != rc)
       {
        clLogWrite(CL_LOG_HANDLE_APP, 
                   CL_LOG_ERROR,    
                   CL_ALARM_SERVER_LIB,
                   CL_ALARM_LOG_1_DEREGISTER,
                   "Debug");
           return rc;
       }
    rc = clEventChannelClose(gAlarmEvtChannelHandle);
       if (CL_OK != rc)
       {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEventChannelClose failed rc :[%x] \n", rc));
           return rc;
       }    
    
    rc = clEventFinalize(gAlarmInitHandle);
       if (CL_OK != rc)
       {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEvent finalize failed rc :[%x] \n", rc));
           return rc;
       }    
    
       rc = clEoClientUninstallTables(pEOObj, 
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, ALM));
       if (CL_OK != rc)
       {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoClientUninstall failed rc :[%x] \n", rc));
           return rc;
       }
    
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, CL_ALARM_SERVER_LIB,
            CL_LOG_MESSAGE_1_SERVICE_STOPPED, "Alarm" );
    
    CL_FUNC_EXIT();
    return rc;
}
/*****************************************************************************/



ClRcT   alarmServerStateChange(ClEoStateT eoState)
{
       return CL_OK;
}

ClRcT   alarmServerHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
       schFeedback->freq = CL_EO_DEFAULT_POLL;
       schFeedback->status = 0;
       return CL_OK;
}

ClEoConfigT clEoConfig = {
    "ALM",          /* EO Name*/
    1,              /* EO Thread Priority */
    3,              /* No of EO thread needed */
    CL_IOC_ALARM_PORT,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    alarmServerInitialize,  /* Function CallBack  to initialize the Application */
    alarmServerFinalize,    /* Function Callback to Terminate the Application */ 
    alarmServerStateChange, /* Function Callback to change the Application state */
    alarmServerHealthCheck, /* Function Callback to change the Application state */
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
    CL_TRUE,            /*  om     */
    CL_FALSE,           /*  hal    */
    CL_FALSE,           /*  dbal   */
};
  
ClUint8T clEoClientLibs[] = {
    CL_TRUE,           /*  cor    */
    CL_FALSE,           /*  cm     */
    CL_FALSE,           /*  name   */
    CL_TRUE,            /*  log    */
    CL_FALSE,           /*  trace  */
    CL_FALSE,           /*  diag   */
    CL_FALSE,           /*  txn    */
    CL_FALSE,           /*  hpi    */
    CL_FALSE,           /*  cli    */
    CL_FALSE,           /*  alarm  */
    CL_TRUE,            /*  debug  */
    CL_FALSE,            /*  gms    */
    CL_FALSE,            /* PM */
};


