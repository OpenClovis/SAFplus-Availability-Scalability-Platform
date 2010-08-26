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
 * ModuleName  : cm
 * File        : clCmMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the Initialization part of the Chassis manager and  
 * the API request handling.                                              
 **************************************************************************/

#undef __CLIENT__
#define __SERVER__

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <oh_utils.h>       /* From OpenHPI package */

#include <clDebugApi.h>
#include <clEoApi.h>
#include <clEoConfigApi.h>
#include <clOsalApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clOsalApi.h>
#include <clEventApi.h>
#include <clCpmApi.h>
#include <clLogApi.h>
#include <include/clCmDefs.h>
#include <ipi/clSAClientSelect.h>
#include <clCmServerFuncTable.h>
#include "clCmHpiDebug.h"

/* Call Backs for EO Config Structure */
static ClRcT clCmComponentInitialize( ClUint32T argc , ClCharT **argv );
static ClRcT cmComponentFinalize(void );
static ClRcT clCMStateChange(ClEoStateT eoState);
static ClRcT chassisManagerPing ( ClEoSchedFeedBackT* schFeedback );

static ClRcT chassisManagerTerminate( ClInvocationT invocation ,
                                      const ClNameT  *compName);

static ClRcT clCMInitialize( ClUint32T argc , ClCharT  *argv[] );

static ClRcT cmServiceInitialize( void );

static ClRcT cmServiceFinalize( void );

static ClRcT clCmServerStateValidate( void );


/* Data structure containing the Context related information for the Chassis
 *  manager component and its assosciated library initializations */
ClCmContextT gClCmChassisMgrContext;

ClEoConfigT clEoConfig = {
    "CHM",
    1,            
    1,           
    CL_IOC_CM_PORT,     
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV,
    clCmComponentInitialize, 
    cmComponentFinalize,    
    clCMStateChange,
    chassisManagerPing,
    NULL
    };


ClUint8T clEoBasicLibs[] = {
    CL_TRUE,            /* osal */
    CL_TRUE,            /* timer */
    CL_TRUE,            /* buffer */
    CL_TRUE,            /* ioc */
    CL_TRUE,            /* rmd */
    CL_TRUE,            /* eo */
    CL_FALSE,            /* om */
    CL_FALSE,            /* hal */
    CL_FALSE,            /* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_TRUE,            /* cor */
    CL_TRUE,            /* cm */
    CL_FALSE,            /* name */
    CL_TRUE,            /* log */
    CL_FALSE,            /* trace */
    CL_FALSE,            /* diag */
    CL_FALSE,            /* txn */
    CL_FALSE,            /* hpi */
    CL_FALSE,            /* cli */
    CL_FALSE,            /* alarm */
    CL_TRUE,            /* debug */
    CL_FALSE            /* gms */
};



static ClRcT clCmComponentInitialize (ClUint32T argc,  ClCharT *argv[])
{

    ClNameT            appName;
    ClCpmCallbacksT    callbacks;
    ClVersionT        version;
    ClRcT            rc = CL_OK;
    ClSvcHandleT    svcHandle;
    ClOsalTaskIdT   taskId;


    /* Do the App intialization */

    /*  Do the CPM client init/Register */
    version.releaseCode = (ClUint8T)'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = chassisManagerTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;
       
    if (( rc = clEoMyEoObjectGet( &(gClCmChassisMgrContext.executionObjHandle))) != CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clEoMyEoObjectGet failed, error [0x%x]", rc);
        return rc;
    }


    if((rc = clCpmClientInitialize(&gClCmChassisMgrContext.cmCompHandle, 
                    &callbacks, 
                    &version
                    ))!= CL_OK)
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCpmClientInitialize failed, error [0x%x]", rc);
        return rc;
    }


    svcHandle.cpmHandle = &gClCmChassisMgrContext.cmCompHandle;
    svcHandle.evtHandle = NULL;
    svcHandle.cpsHandle = NULL;
    svcHandle.gmsHandle = NULL;
    svcHandle.dlsHandle = NULL;
    
    if ((rc = clDispatchThreadCreate(gClCmChassisMgrContext.executionObjHandle, 
                    &taskId,
                    svcHandle)) != CL_OK)
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clDispatchThreadCreate failed, error [0x%x]", rc);
        return rc;
    }


    if((rc = clCpmComponentNameGet(
                    gClCmChassisMgrContext.cmCompHandle , 
                    &appName
                    ))!= CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCpmComponentNameGet failed, error [0x%x]", rc);
        return rc;
    }

           
    /* Block and use the main thread if CL_EO_USE_THREAD_FOR_APP other wise return */
    if((rc = clCMInitialize(argc, argv ))!= CL_OK)
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCMInitialize failed, error [0x%x]", rc);
        return rc;
    }
    if((rc = clCpmComponentRegister(
                    gClCmChassisMgrContext.cmCompHandle , 
                    &appName, 
                    NULL
                    ))!= CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCpmComponentRegister failed, error [0x%x]", rc);
        return rc;
    }
    clLog(CL_LOG_NOTICE, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Chassis manager server fully up");    
    return CL_OK;
}




/* Initialize the chassis manager component */ 
static ClRcT clCMInitialize( ClUint32T argc , ClCharT *argv[])
{
    ClRcT rc =      CL_OK;

    /* Initialize the Chassis manager ServiceInterface */ 
    if((rc = cmServiceInitialize ())!= CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "cmServiceInitialize failed, error [0x%x]", rc);
        return rc;
    } 

    /* Event Related Initialization */ 
    if((rc = clCmEventInterfaceInitialize())!= CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCmEventInterfaceInitialize failed, error [0x%x]", rc);
        cmServiceFinalize();
        return rc;
    }

    /* HPI related Initialization */ 
    if((rc = clCmHpiInterfaceInitialize())!= CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clCmHpiInterfaceInitialize failed, error [0x%x]", rc);
        clCmEventInterfaceFinalize();
        cmServiceFinalize();
        return rc;
    }

    /* Register with the debug server */ 
    if((rc =cmDebugRegister(gClCmChassisMgrContext.executionObjHandle))!= CL_OK)
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "cmDebugRegister failed, error [0x%x]", rc);
        clCmHpiInterfaceFinalize();
        clCmEventInterfaceFinalize();
        cmServiceFinalize();
        return rc;
    }

    gClCmChassisMgrContext.cmInitDone = CL_TRUE;
    return rc;
}

static ClRcT cmServiceInitialize( void )
{

    ClRcT rc = CL_OK;

    memset((void *)&(gClCmChassisMgrContext.executionObjHandle), 0,
                        sizeof(gClCmChassisMgrContext.executionObjHandle));

    if((rc = clEoMyEoObjectGet(
                    &gClCmChassisMgrContext.executionObjHandle 
                    ))!= SA_OK ){
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clEoMyEoObjectGet failed, error [0x%x]", rc);
        return rc;
    }

    /* Install the native client table in the EO */ 
    rc = clEoClientInstallTables( 
                                 gClCmChassisMgrContext.executionObjHandle,
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, CM));
            
    if(rc != CL_OK )
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "clEoClientInstall failed, error [0x%x]", rc);
        return rc;
    }

    return CL_OK;                           
}


static ClRcT cmServiceFinalize( void )
{

    /* Deinstall the Native service client Table*/
    clEoClientUninstallTables( 
            gClCmChassisMgrContext.executionObjHandle, 
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, CM));
    
    return CL_OK;
}


/* End or Terminate the Chassis manager component */ 
static ClRcT chassisManagerTerminate( ClInvocationT invocation , 
                                      const ClNameT  *compName)
{

    ClRcT rc = CL_OK;

    clCmHpiInterfaceFinalize  ();
    clCmEventInterfaceFinalize();
    cmDebugDeregister( gClCmChassisMgrContext.executionObjHandle );
    cmServiceFinalize();
    /* Unregister with the component manager */ 
    clCpmComponentUnregister( gClCmChassisMgrContext.cmCompHandle , 
                              compName , NULL);
    /* Finalize the cpm client library in the Chassis manager */
    clCpmClientFinalize(gClCmChassisMgrContext.cmCompHandle);

    clCpmResponse(gClCmChassisMgrContext.cmCompHandle, invocation, CL_OK);

    return rc;
}    


static ClRcT cmComponentFinalize ( )
{
    return CL_OK;
}



/* Set the state of the Chassis manager component */
static ClRcT   clCMStateChange(ClEoStateT eoState)
{
    return CL_OK;
}



/*Responds  the Health Ping from the Component manager */
static ClRcT chassisManagerPing ( ClEoSchedFeedBackT* schFeedback )
{
    ClRcT rc = CL_OK;
    return rc;
}

static ClRcT clCmServerStateValidate(void)
{
    /* Server is in the process of Initialization ask the client to resend the
     *  request */
    if( gClCmChassisMgrContext.cmInitDone == CL_FALSE ){
        clLog(CL_LOG_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Client call received before initialization completed. Asking to retry");
        return CL_CM_ERROR(CL_ERR_TRY_AGAIN);
    }

    /* Reject any service request if the discovery is in progress */
    if( gClCmChassisMgrContext.platformInfo.discoveryInProgress == CL_TRUE ){
        clLog(CL_LOG_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Client call received during HPI discovery. Asking to retry");
        return CL_CM_ERROR ( CL_ERR_TRY_AGAIN );
    }

    return CL_OK;
}



/* API serving loooop handles the Client API requests one by one in an
 *  iterative mode */
ClRcT  VDECL(clCmServerApiFruStateGet)( ClEoDataT data, 
                                        ClBufferHandleT  inMsg,
                                        ClBufferHandleT  outMsg )
{

    ClRcT rc  = CL_OK;
    /*ClUint32T  physicalSlot;*/
    ClUint32T  size = (ClUint32T)sizeof(ClCmFruAddressT);
    SaErrorT         error = SA_OK; 
    SaHpiResourceIdT resourceId=0;
    SaHpiHsStateT    frustate;
    ClCmFruAddressT fruSlotInfo;


    rc= clCmServerStateValidate();
    if(rc != CL_OK)
    {
        clLog(CL_LOG_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Not ready to handle clCmServerApiFruStateGet. Asking client to retry");
        return rc;
    }

    clBufferNBytesRead( inMsg, (ClUint8T*)&fruSlotInfo, &size);
    clCmClientToServerVersionValidate(fruSlotInfo.version);


    rc= clCmGetResourceIdFromSlotId(0,fruSlotInfo.physicalSlot,&resourceId);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Unable to provide ResourceId from slotId, error [0x%x]", rc);
        return rc;
    }

    if((error = _saHpiHotSwapStateGet (
                    gClCmChassisMgrContext.platformInfo.hpiSessionId,
                    resourceId,
                    &frustate
                    ))!= SA_OK){
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "saHpihotswapstateGet failed :%s", oh_lookup_error(error));
        return CL_CM_ERROR ( CL_ERR_CM_HPI_ERROR );
     }

     clBufferNBytesWrite (outMsg,
             (ClUint8T*)&frustate,
             (ClUint32T)sizeof(SaHpiHsStateT)
             );


    return rc;
}


ClRcT  VDECL(clCmServerOperationRequest)(ClEoDataT data, 
                                         ClBufferHandleT  inMsg,
                                         ClBufferHandleT  outMsg )
{

    ClRcT rc  = CL_OK;
    ClCmOpReqT opRequest;
    ClUint32T  size = (ClUint32T)sizeof(opRequest);
    SaErrorT         error = SA_OK; 
    SaHpiResourceIdT resourceId=SAHPI_UNSPECIFIED_RESOURCE_ID;
    char * pEnv=NULL;
    

    memset((void*)&opRequest, 0 , sizeof(opRequest));

    rc= clCmServerStateValidate();
    if(rc != CL_OK)
    {
        clLog(CL_LOG_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Not ready to handle clCmServerOperationRequest. Asking client to retry");
        return rc;
    }

    clBufferNBytesRead( inMsg, (ClUint8T*)&opRequest, &size);
    clCmClientToServerVersionValidate(opRequest.version);

    /* If simulation environment , CPM will set the env variable in it's own
     * context and it will be copied to CM's process context, in case if running
     * from bash shell , set this env varaible in the bash */
    pEnv=getenv("CL_HW_PLATFORM");
    /* If simulation environment return */
    if((NULL!=pEnv)&&(strncasecmp(pEnv,"SIMULATION",strlen(pEnv))==0))
    {
        clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Simulation mode: chassisId=%u, slotId=%u, request=%u",
            opRequest.chassisId,
            opRequest.physicalSlot,
            opRequest.request);
        return CL_OK;
    }

    clLog(CL_LOG_DEBUG, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
        "clCmServerOperationRequest received: chassisId=%u, slotId=%u, request=%u",
        opRequest.chassisId,
        opRequest.physicalSlot,
        opRequest.request);

    rc= clCmGetResourceIdFromSlotId(opRequest.chassisId,opRequest.physicalSlot,&resourceId);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
        "clCmServerOperationRequest: unable to get resourceId from slotId, error [0x%x]", rc);
        return rc;
    }

    /* i) operation can be one of the states 
       ii) power reset or shutdown of the card . 
     */

     /* Check if its a hotswap request */ 
     if( opRequest.request== CL_CM_INSERT_REQUEST ||
             opRequest.request == CL_CM_EXTRACT_REQUEST)
     {

        SaHpiHsActionT  hotSwapAction;

         if(opRequest.request == CL_CM_INSERT_REQUEST) 
             hotSwapAction = SAHPI_HS_ACTION_INSERTION;
         else 
             hotSwapAction = SAHPI_HS_ACTION_EXTRACTION;


         if((error = _saHpiHotSwapActionRequest( 
                        gClCmChassisMgrContext.platformInfo.hpiSessionId,
                        resourceId,
                        hotSwapAction 
                        ))!= SA_OK)
        {
            clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiHotSwapActionRequest failed: %s", oh_lookup_error(error));
        }
        return CL_OK;

    }

     /* Check if its a power request */ 
     else if( opRequest.request == CL_CM_POWERON_REQUEST
             || opRequest.request ==CL_CM_POWEROFF_REQUEST
             || opRequest.request == CL_CM_POWER_CYCLE_REQUEST)
    {

        SaHpiPowerStateT  powerstate;

        if(opRequest.request == CL_CM_POWERON_REQUEST) 
            powerstate = SAHPI_POWER_ON;
        else if(opRequest.request ==CL_CM_POWEROFF_REQUEST)
            powerstate = SAHPI_POWER_OFF;
        else 
            powerstate = SAHPI_POWER_CYCLE;

        if((error = _saHpiResourcePowerStateSet( 
                        gClCmChassisMgrContext.platformInfo.hpiSessionId,
                        resourceId,
                        powerstate
                        ))!= SA_OK ){
            clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiResourcePowerStateSet failed: %s", oh_lookup_error(error));
        }
        return CL_OK;
    }
    else if( opRequest.request == CL_CM_RESET_REQUEST
             || opRequest.request ==CL_CM_WARM_RESET_REQUEST
             || opRequest.request == CL_CM_RESET_ASSERT_REQUEST
             || opRequest.request ==CL_CM_RESET_DEASSERT_REQUEST)
    {

        SaHpiResetActionT  resetAction;

        if(opRequest.request == CL_CM_RESET_REQUEST) 
            resetAction = SAHPI_COLD_RESET;
        else if(opRequest.request ==CL_CM_WARM_RESET_REQUEST)
            resetAction = SAHPI_WARM_RESET;
        else if (opRequest.request ==CL_CM_RESET_ASSERT_REQUEST) 
            resetAction = SAHPI_RESET_ASSERT;
        else
            resetAction = SAHPI_RESET_DEASSERT;

        if((error = _saHpiResourceResetStateSet( 
                        gClCmChassisMgrContext.platformInfo.hpiSessionId,
                        resourceId,
                        resetAction))!= SA_OK )
        {
            clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                "saHpiResourceResetStateSet failed: %s", oh_lookup_error(error));
        }

        return CL_OK;
    }
    else 
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Invalid operation on the FRU requested");
        return CL_CM_ERROR ( CL_ERR_INVALID_PARAMETER );
    }

    return rc;
}

/* 
Description:
------------
This call handles the response from the CPMG to the query made by the Chassis
manager for admission/ extraction of the FRU . This RMD call is to be called
by the CPMG in an asynchronous way . The function handles the response YES/NO
and sets the resource to ACTIVE/INACTIVE depending on the response. 
 */

ClRcT VDECL(clCmServerCpmResponseHandle) ( ClEoDataT  data, 
                                           ClBufferHandleT  inMsgHandle, 
                                           ClBufferHandleT  outMsgHandle)

{
    ClRcT rc  = CL_OK;
    ClUint32T  size = (ClUint32T)sizeof(ClCpmCmMsgT);
    ClCpmCmMsgT      cpm2CmMsg; 
    
    memset((void*)&cpm2CmMsg,0, sizeof(ClCpmCmMsgT));

    rc= clCmServerStateValidate();
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Not ready to handle clCmServerCpmResponseHandle. Asking client to retry");
        return rc;
    }

    rc= clBufferNBytesRead(inMsgHandle,
                                  (ClUint8T*)&cpm2CmMsg,
                                  &size);
    if(CL_OK !=rc)
    {
        clLog(CL_LOG_ERROR, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
            "Failed to decode request buffer, error [0x%x]", rc);
        return rc;
    }

    /* Handle the response from the CPMG */
    clCmHPICpmResponseHandle(&cpm2CmMsg);
    return rc;
}
