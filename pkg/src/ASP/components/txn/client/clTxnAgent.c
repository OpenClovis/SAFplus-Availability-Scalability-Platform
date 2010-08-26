/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : txn                                                           
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/txn/client/clTxnAgent.c $
 * $Author: deepak $
 * $Date: 2007/03/14 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains transaction agent implementation. Agent is             
 * resonsible for responding to transaction mgr requests/commands and invoke   
 * appropriate actions on the application component (EO) for completing        
 * transaction.                                                                
 *
 *
 *****************************************************************************/
#define __SERVER__
#include <string.h>

#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clVersionApi.h>
#include <clCntApi.h>
#include <clBufferApi.h>
#include <clVersion.h>
#include <clCpmApi.h>

#include <clTxnLog.h>
#include <clTxnAgentApi.h>

#include "clTxnAgentIpi.h"
#include <clTxnStreamIpi.h>
#include <clTxnCommonDefn.h>

#include <xdrClTxnMessageHeaderIDLT.h>
#include <xdrClTxnCmdT.h>
#include <clTxnXdrCommon.h>


/**
 * List of supported version (transaction agent)
 */
static ClVersionT   clTxnMgmtVersionSupported[] = {
    { 'B', 1, 1 }
};
/*
* List of Job/Txn Status strings.
*/
 static char*    gCliTxnStatusStr[] = {
    "",
    "CL_TXN_TRANSACTION_PRE_INIT",
    "CL_TXN_TRANSACTION_ACTIVE",
    "CL_TXN_TRANSACTION_PREPARING",
    "CL_TXN_TRANSACTION_PREPARED",
    "CL_TXN_TRANSACTION_COMMITTING",
    "CL_TXN_TRANSACTION_COMMITTED",
    "CL_TXN_TRANSACTION_MARKED_ROLLBACK",
    "CL_TXN_TRANSACTION_ROLLING_BACK",
    "CL_TXN_TRANSACTION_ROLLED_BACK",
    "CL_TXN_TRANSACTION_RESTORED",
    "CL_TXN_TRANSACTION_UNKNOWN",
    };

/**
 * Read-only Version Database. 
 */
static ClVersionDatabaseT clTxnMgmtVersionDb = {
    sizeof(clTxnMgmtVersionSupported) / sizeof(ClVersionT), 
    clTxnMgmtVersionSupported
};

/* TODO: Deepak: Needs to be locked */
ClTxnAgentCfgT  *clTxnAgntCfg = NULL;

/* Forward Declarations */
static ClRcT 
VDECL(clTxnAgentCntrlMsgReceive)(
        CL_IN   ClEoDataT               eoArg, 
        CL_IN   ClBufferHandleT  inMsg, 
        CL_OUT  ClBufferHandleT  outMsg);

static ClRcT 
VDECL(clTxnAgentDebugMsgReceive)(
        CL_IN   ClEoDataT               eoArg, 
        CL_IN   ClBufferHandleT  inMsg, 
        CL_OUT  ClBufferHandleT  outMsg);

#include "clTxnAgentFuncList.h"

/* Internal Functions */
/**
 * This is a hash-fn used to store comp-service in a hash-map
 */
ClUint32T _clTxnCmpServiceHashFn(ClCntKeyHandleT key);

/**
 * Key compare function for services hosted in a component (transaction-agent)
 */
ClInt32T _clTxnCmpServiceKeyCompare(CL_IN ClCntKeyHandleT key1,
                                    CL_IN ClCntKeyHandleT key2);

/**
 * Callback function to delete service-registration information at agent
 */
void _clTxnCompServiceDelete(CL_IN ClCntKeyHandleT userKey, 
                          CL_IN ClCntDataHandleT userData);

/**
 * Internal function to process commands received from txn-mgr
 */
ClRcT _clTxnAgentProcessMgrCmd(
        CL_IN   ClBufferHandleT  inMsgHandle,
        CL_IN   ClBufferHandleT  outMsgHandle,
        CL_IN   ClTxnMessageHeaderT     *pMsgHdr);

/**
 * COR SPECIFIC
 */
ClRcT _clTxnAgentProcessClientCmd(
        CL_IN   ClBufferHandleT  inMsgHandle,
        CL_OUT   ClBufferHandleT  outMsgHandle,
        CL_IN   ClTxnMessageHeaderT     *pMsgHdr);
/*
 * Internal Function to pack the txn/job state and service details
   */
static ClRcT _clTxnAgentAppendActiveJob(CL_IN   ClBufferHandleT  outMsg);

static ClRcT _clTxnAgentAppendServices(CL_IN   ClBufferHandleT  outMsg);
static ClCharT* _clTxnCmdNameGet(CL_IN ClUint32T cmd);

/**
 * TxnAgent Library Initialize used with EO Infrastructure
 */
ClRcT clTxnLibInitialize(void)
{
    ClRcT               rc      = CL_OK;
    ClEoExecutionObjT   *pEOObj;
    ClVersionT          txnVersion = {
        .releaseCode    =   'B',
        .majorVersion   =   1,
        .minorVersion   =   1
    };
    CL_FUNC_ENTER();

    rc = clEoMyEoObjectGet(&pEOObj);
    if (CL_OK != rc)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = clTxnAgentInitialize(pEOObj, &txnVersion);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * TxnAgent Library Finalize used with EO Infrastructure
 */
ClRcT clTxnLibFinalize(void)
{
    ClRcT       rc = CL_OK;
    CL_FUNC_ENTER();

    rc = clTxnAgentFinalize();

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnVersionVerify(ClVersionT     *pVersion)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    if (NULL == pVersion)
    {
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    rc = clVersionVerify (&clTxnMgmtVersionDb, pVersion);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Un-supported version. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This is the agent initilaization API called during EO initialization.
 * 
 * Agent acts as intermediate layer during an active transaction. It receives
 * job-definition and commands from transaction manager and takes specific
 * action.
 */
ClRcT clTxnAgentInitialize(CL_IN ClEoExecutionObjT  *appEoObj, 
                           CL_IN ClVersionT         *pTxnVersion)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if ( (NULL == appEoObj) || (NULL == pTxnVersion) )
    {
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    /* Do version negotiation  (later part of RC-1 or RC-2) */
    if (clTxnVersionVerify(pTxnVersion) != CL_OK)
    {
        CL_FUNC_EXIT();
        return (CL_TXN_RC(CL_ERR_VERSION_MISMATCH));
    }

    clTxnAgntCfg = (ClTxnAgentCfgT *) clHeapAllocate( sizeof(ClTxnAgentCfgT));
    CL_TXN_NULL_CHECK_RETURN(clTxnAgntCfg, CL_ERR_NO_MEMORY, 
                             ("Failed to allocate memory\n"));
    memset(clTxnAgntCfg, 0, sizeof(ClTxnAgentCfgT));


    /* If possible, instead of statically registring RMD functions, txn-agent 
       could register EO client table here.
    */

    /* Create hash-maps for active-txn component-services */
    rc = clTxnDbInit( &(clTxnAgntCfg->activeTxnMap) );
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to allocate hash-map for active-txn rc:0x%x\n", rc));

    rc = clCntThreadSafeHashtblCreate(CL_TXN_NUM_BUCKETS, 
                            _clTxnCmpServiceKeyCompare, 
                            _clTxnCmpServiceHashFn, 
                            _clTxnCompServiceDelete, _clTxnCompServiceDelete, 
                            CL_CNT_UNIQUE_KEY,
                            (ClCntHandleT *) &clTxnAgntCfg->compServiceMap);
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to allocate hash-map for comp-service rc:0x%x\n", rc));

    clTxnMutexCreateAndLock(&clTxnAgntCfg->actMtx);
    /* No registered service */
    clTxnAgntCfg->agentCapability = CL_TXN_AGENT_NO_SERVICE_REGD;

    clTxnMutexUnlock(clTxnAgntCfg->actMtx);

    clLogInfo("AGT", NULL,
            "Installing function table");
    rc = clEoClientInstallTables(appEoObj, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXNAgent));
    if (CL_OK == rc)
    {
        rc = clTxnCommIfcInit(&(clTxnMgmtVersionSupported[0]));
        if(CL_OK != rc)
        {
            clLogError("AGT", NULL, 
                    "Error in initiazing communication interface, rc [0x%x]", rc);
            clEoClientUninstallTables(appEoObj, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXNAgent));
            return rc;
        }
    }

    if(CL_OK == rc)
    {
        rc = clTxnAgentTableRegister(appEoObj);
        if(CL_OK != rc)
        {
            clLogError("AGT", NULL, 
                    "Error in table registration, rc [0x%x]", rc);
            clEoClientUninstallTables(appEoObj, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXNAgent));
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This is a finalization API for transaction agent. This API must be called
 * during finalization of EO.
 */
ClRcT clTxnAgentFinalize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT   *pEOObj;


    CL_FUNC_ENTER();
    /* - Release all allocated memeory 
       FIXME: Check for active transaction, and try to consolidate
    */

    if (clTxnAgntCfg != NULL)
    {
        clTxnDbFini(clTxnAgntCfg->activeTxnMap);

        clCntDelete(clTxnAgntCfg->compServiceMap);
        clTxnMutexDelete(clTxnAgntCfg->actMtx);
        clHeapFree(clTxnAgntCfg);

        clTxnCommIfcFini();

        rc = clEoMyEoObjectGet(&pEOObj);
        clEoClientUninstallTables(pEOObj, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXNAgent));

        clTxnAgntCfg = NULL;
    }
    CL_TXN_RETURN_RC(rc, ("Failed to finalize transaction-agent rc:0x%x\n", rc));
}

/**
 * API to register a service with transaction agent
 */
ClRcT clTxnAgentServiceRegister(
        CL_IN   ClInt32T                    serviceId, 
        CL_IN   ClTxnAgentCallbacksT        tCallback, 
        CL_OUT  ClTxnAgentServiceHandleT    *pHandle)
{
    /*
       This is registration request from a service hosted in this component.
       (There could be multiple such services).
       Store these callbacks in an appropriate data-structure indexed with 
       service-id (identication of service under considered).
    */
    ClRcT                       rc                  = CL_OK;
    ClUint8T                    serviceCapability   = 0xFF;
    ClTxnAgentCompServiceInfoT  *pNewCompService    = NULL;

    CL_FUNC_ENTER();

    CL_TXN_NULL_CHECK_RETURN(pHandle, CL_ERR_NULL_POINTER, ("Invalid handle\n"));
    if (NULL == clTxnAgntCfg)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_TXN_AGENT_LIB,
                   CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Agent library is not initialized. clTxnAgntCfg is NULL\n"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    /* Validate callback function */
    if( (tCallback.fpTxnAgentJobPrepare == NULL) && 
        (tCallback.fpTxnAgentJobRollback == NULL) &&
        (tCallback.fpTxnAgentJobCommit != NULL) )
    {
        serviceCapability = CL_TXN_AGENT_SERVICE_1PC;
        clTxnMutexLock(clTxnAgntCfg->actMtx);
        /* Check if this is not the first one to declare as 1-PC Capable service */
        if ( (clTxnAgntCfg->agentCapability & 
                CL_TXN_AGENT_SERVICE_1PC) == CL_TXN_AGENT_SERVICE_1PC)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Txn-Agent does not allow more than one service of 1-PC Type"));
            CL_FUNC_EXIT();
            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
            return CL_TXN_RC(CL_ERR_INVALID_PARAMETER);
        }
        clTxnMutexUnlock(clTxnAgntCfg->actMtx);
    }
    else if ( (tCallback.fpTxnAgentJobPrepare != NULL) &&
              (tCallback.fpTxnAgentJobCommit != NULL)  &&
              (tCallback.fpTxnAgentJobRollback != NULL) )
    {
        serviceCapability = CL_TXN_AGENT_SERVICE_2PC;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid parameter - callback function not defined properly"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_INVALID_PARAMETER);
    }

    /* Check if entry already exists or not */
    rc = clCntDataForKeyGet( (ClCntHandleT) clTxnAgntCfg->compServiceMap, 
                              (ClCntKeyHandleT) &serviceId, 
                              (ClCntDataHandleT *) &pNewCompService);
    if ( CL_OK == rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Component-service already registered\n"));
        CL_FUNC_EXIT();
        return (CL_TXN_RC(CL_ERR_DUPLICATE));
    }

    pNewCompService = (ClTxnAgentCompServiceInfoT *) clHeapAllocate(sizeof(ClTxnAgentCompServiceInfoT));
    CL_TXN_NULL_CHECK_RETURN(pNewCompService, CL_ERR_NO_MEMORY, 
                             ("Failed to allocate memory\n"));

    memset(pNewCompService, 0, sizeof(ClTxnAgentCompServiceInfoT));
    pNewCompService->serviceType = serviceId;
    pNewCompService->serviceCapability = serviceCapability;

    pNewCompService->pCompCallbacks = 
        (ClTxnAgentCallbacksT *) clHeapAllocate(sizeof(ClTxnAgentCallbacksT));
    
    
    CL_TXN_NULL_CHECK_RETURN(pNewCompService->pCompCallbacks, CL_ERR_NO_MEMORY,
                             ("Failed to allocate memory\n"));
    memcpy(pNewCompService->pCompCallbacks, &tCallback, sizeof(ClTxnAgentCallbacksT));

    /* Put the entry into hash-map with service-id to be the key */
    rc = clCntNodeAdd(clTxnAgntCfg->compServiceMap, 
                       (ClCntKeyHandleT) &(pNewCompService->serviceType), 
                       (ClCntDataHandleT)pNewCompService, NULL);

    if (CL_OK != rc)
    {
        clHeapFree(pNewCompService->pCompCallbacks);
        clHeapFree(pNewCompService);
    }
    else 
    {
        /* Update agent-cfg */
        if (serviceCapability == CL_TXN_AGENT_SERVICE_1PC)
        {
#ifdef CL_TXN_DEBUG
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Service registered is 1pc\n"));
#endif
            clTxnAgntCfg->agentCapability |= CL_TXN_AGENT_SERVICE_1PC;
        }
        else
            clTxnAgntCfg->agentCapability |= CL_TXN_AGENT_SERVICE_2PC;

        *pHandle = (ClTxnAgentServiceHandleT )pNewCompService;
        clLogNotice("AGT", "INI",
                "Registered [%s] service successfully having serviceId [%d]", 
                (serviceCapability == CL_TXN_AGENT_SERVICE_1PC)?"READ": "2P",
                serviceId);
    }
    CL_TXN_RETURN_RC(rc, ("Failed to register new component-service rc:0x%x\n", rc));
}

/**
 * API to un-register a service with transaction-agent
 */
ClRcT clTxnAgentServiceUnRegister(CL_IN ClTxnAgentServiceHandleT tHandle)
{
    ClRcT                       rc = CL_OK;
    ClUint8T                    twoPC = 0;
    ClTxnAgentCompServiceInfoT  *pCompService = NULL;


    CL_FUNC_ENTER();

    if (tHandle == 0x0)
    {
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_INVALID_HANDLE);
    }
    /*
       This is a request from service hosted in the component to unregister
       from the transaction-management.
       Remove the entry from the data-structure and invalidate the handle

       FIXME: Before actually deleting it, check to see if this service is 
              part of any active txn or not
    */

    rc = clCntDataForKeyGet(clTxnAgntCfg->compServiceMap, 
                            (ClCntKeyHandleT) &(((ClTxnAgentCompServiceInfoT *)tHandle)->serviceType),
                            (ClCntDataHandleT *)&pCompService);
    if (CL_OK == rc)
    {
        ClUint32T   srvCount;
        if (pCompService->serviceCapability == CL_TXN_AGENT_SERVICE_1PC)
        {
            twoPC = 0;
            clTxnAgntCfg->agentCapability &= ~(CL_TXN_AGENT_SERVICE_1PC);
        }
        else if(pCompService->serviceCapability == CL_TXN_AGENT_SERVICE_2PC)
        {
            twoPC = 1;
        }

        rc = clCntAllNodesForKeyDelete(clTxnAgntCfg->compServiceMap,
                                      (ClCntKeyHandleT) &(((ClTxnAgentCompServiceInfoT *)tHandle)->serviceType));
        if(CL_OK != rc)
        {
            clLogError("AGT", NULL,
                    "Failed to delete node from compServiceMap corresponding to service[%d]", 
                    ((ClTxnAgentCompServiceInfoT *)tHandle)->serviceType);
            return rc;
        }
                                        
        /* Reset agent capability, if necessary */
        rc = clCntSizeGet(clTxnAgntCfg->compServiceMap, &srvCount);
        if ( (CL_OK == rc) && (srvCount == 0x0) )
        {
            clTxnAgntCfg->agentCapability = CL_TXN_AGENT_NO_SERVICE_REGD;
        } 
        else if ( (CL_OK == rc) && (srvCount == 0x1) && 
                  ( (clTxnAgntCfg->agentCapability & CL_TXN_AGENT_SERVICE_1PC) == CL_TXN_AGENT_SERVICE_1PC) )
        {
            clTxnAgntCfg->agentCapability = CL_TXN_AGENT_SERVICE_1PC;
        }
        else if ( CL_OK != rc )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while reading number of service registered. rc:0x%x", rc));
        }
    }
    if(CL_OK == rc)
        clLogNotice("AGT", "FIN",
                "Unregistering [%s] service successfull", twoPC ? "2PC":"READ"); 
    CL_TXN_RETURN_RC(rc, ("Failed to unregister component-service rc:0x%x\n", rc));
}

/*********************************************************************************
            Following are internal/private function implementation
*********************************************************************************/
/**
  This function recieves the debug CLI command through the transaction manager
  and send back the job status and registered service details. 
*/
static ClRcT 
VDECL(clTxnAgentDebugMsgReceive)(
        CL_IN   ClEoDataT               eoArg, 
        CL_IN   ClBufferHandleT  inMsg, 
        CL_IN   ClBufferHandleT  outMsg)
{
    ClRcT                   rc = CL_OK;

    CL_FUNC_ENTER();
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_NEWLINE,
                                        CL_TXN_AGT_CLI_DISP_NEWLINE_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *) CL_TXN_AGT_CLI_DISP_JOB_LIST_HEADER,
                                        CL_TXN_AGT_CLI_DISP_JOB_LIST_HEADER_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_SHORT_SEP,
                                        CL_TXN_AGT_CLI_DISP_SHORT_SEP_SIZE,
                                        outMsg,0);
    
    rc = _clTxnAgentAppendActiveJob(outMsg);
    if(CL_OK != rc)
    {
        clLogError("AGT", "CLI",
                "Appending active job failed with error [0x%x]. Failed to handle Txn Mgr debug cmd", rc);
        return rc;
    }
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_SHORT_SEP,
                                        CL_TXN_AGT_CLI_DISP_SHORT_SEP_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_NEWLINE,
                                        CL_TXN_AGT_CLI_DISP_NEWLINE_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *) CL_TXN_AGT_CLI_DISP_SRC_INFO_HEADER,
                                        CL_TXN_AGT_CLI_DISP_SRC_INFO_HEADER_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_SHORT_SEP,
                                        CL_TXN_AGT_CLI_DISP_SHORT_SEP_SIZE,
                                        outMsg,0);

    rc = _clTxnAgentAppendServices(outMsg);
    if(CL_OK != rc)
    {
        clLogError("AGT", "CLI",
                "Appending services failed with error [0x%x]. Failed to handle Txn Mgr debug cmd", rc);
        return rc;
    }
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_SHORT_SEP,
                                        CL_TXN_AGT_CLI_DISP_SHORT_SEP_SIZE,
                                        outMsg,0);
    
    rc = clXdrMarshallArrayClUint8T((ClUint8T *)CL_TXN_AGT_CLI_DISP_NEWLINE,
                                        CL_TXN_AGT_CLI_DISP_NEWLINE_SIZE,
                                        outMsg,0);
    CL_FUNC_EXIT();
    return(rc);
}    

/**
 * This function receives control message from transaction-manager for coordinating
 * for a given transaction-job. Parameters identify transaction-job and the control
 * information necessary to execute the next step.
 */
ClRcT 
VDECL(clTxnAgentCntrlMsgReceive)(
        CL_IN   ClEoDataT               eoArg, 
        CL_IN   ClBufferHandleT  inMsg, 
        CL_IN   ClBufferHandleT  outMsg)
{
    ClRcT                   rc = CL_OK;
    ClTxnMessageHeaderT     msgHeader = {{0}};
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}};
    
    CL_FUNC_ENTER();

    rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(inMsg, &msgHeadIDL);
    if(CL_OK != rc)
    {
        clLogError("AGT", NULL,
                "Error in unmarshalling msgHeadIDL, rc [0x%x]", rc);
        return rc;
    }

    _clTxnCopyIDLToMsgHead(&msgHeader, &msgHeadIDL);
    CL_ASSERT(msgHeader.msgCount);

    /* Check/Validate server version */
    rc = clVersionVerify (&clTxnMgmtVersionDb, &(msgHeader.version));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Un-supported version. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (CL_GET_ERROR_CODE(rc));
    }

    clLogDebug("AGT", NULL, "Receiving control-msg from txn-manager");

    /* 
       Based on msg-type, 
       - If this is TXN_DEFN, then extract the txn and job-defn and do initialize.
         Respond CL_OK, if everything is fine.

       - If this is TXN_CMD, then for each txn and job (if mentioned), execute 
         and intended operation

       - If TXN_STATUS_CHANGE, take action accordingly
    */

    switch (msgHeader.msgType)
    {
        case CL_TXN_MSG_MGR_CMD:
            rc = _clTxnAgentProcessMgrCmd(inMsg, outMsg, &msgHeader);
            break;

        case CL_TXN_MSG_CLIENT_REQ_TO_AGNT:
            rc = _clTxnAgentProcessClientCmd(inMsg, outMsg, &msgHeader);
            break;
        
        case CL_TXN_MSG_COMP_STATUS_UPDATE:
            rc = CL_ERR_NOT_IMPLEMENTED;
            break;

        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Invalid msg-type received 0x%x\n", msgHeader.msgType));
            rc = CL_ERR_INVALID_PARAMETER;
            break;
    }
/* This is to be replaced by clTxnCommIfcReadMessage(). clTxnCommIfcReadMessage()
 * writes to the outMsgHdl which is either received by the client or server depending
 * on where the async request came from 
 */
#if 0
    rc = clTxnCommIfcAllSessionClose();

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to process received msg. rc:0x%x\n", rc));
        /* FIXME: Do logging */
        rc = CL_GET_ERROR_CODE(rc);
    }
#endif

    CL_FUNC_EXIT();
    return (rc);
}

/******************************************************************************
 *                          Internal Functions                                *
 ******************************************************************************/

ClRcT _clTxnAgentAppendActiveJob(CL_IN   ClBufferHandleT  outMsg)
{
    ClRcT                   rc = CL_OK;
    ClTxnDefnT              *pTxnDefn;
    ClCntNodeHandleT        currTxnNode;
    ClCharT                 pOutMsg[1024];
    
    CL_FUNC_ENTER();
   
    rc = clCntFirstNodeGet(clTxnAgntCfg->activeTxnMap, &currTxnNode);
    
    while (rc == CL_OK)
    {
        ClCntNodeHandleT    nodeTxn;
        ClCntNodeHandleT    currJobNode;
        ClTxnAppJobDefnT    *pNewTxnJob;
        
        /* Retrieve data-node for node */
        rc = clCntNodeUserDataGet(clTxnAgntCfg->activeTxnMap, currTxnNode,
                                           (ClCntDataHandleT *)&pTxnDefn);
        if(CL_OK != rc)
        {
            clLogError("AGT", "CLI",
                    "User data get failed with error [0x%x]", rc);
            
            return rc;
        }
        
        rc = clCntFirstNodeGet(pTxnDefn->jobList, &currJobNode);
        
        while (rc == CL_OK)
        {
            ClCntNodeHandleT        nodeJob;
        
            /* Retrieve data-node for node */
            rc = clCntNodeUserDataGet(pTxnDefn->jobList, currJobNode,
                                           (ClCntDataHandleT *)&pNewTxnJob);
            pOutMsg[0] = '\0';
            sprintf(pOutMsg,
                    "   0x%x:0x%x\t|   0x%x\t|   %30s \n",
                    pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId,
                    pNewTxnJob->jobId.jobId, gCliTxnStatusStr[pNewTxnJob->currentState]);
 
            clXdrMarshallArrayClUint8T(&pOutMsg, strlen((char*)pOutMsg), outMsg, 0);
  
            nodeJob = currJobNode;
            rc = clCntNextNodeGet(pTxnDefn->jobList, nodeJob, &currJobNode);
        }
        
        nodeTxn = currTxnNode;
        rc = clCntNextNodeGet(clTxnAgntCfg->activeTxnMap, nodeTxn, &currTxnNode);
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        rc = CL_OK;

    CL_FUNC_EXIT();
    return(rc);
}

ClRcT _clTxnAgentAppendServices(CL_IN   ClBufferHandleT  outMsg)
{
    ClRcT                   rc = CL_OK;
    ClTxnAgentCompServiceInfoT  *pCompInstance;
    ClCntNodeHandleT        currNode;
    ClCharT                 pOutMsg[1024];
    
    CL_FUNC_ENTER();
   
    rc = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);
    
    while (rc == CL_OK)
    {
        ClCntNodeHandleT        node;
        /* Retrieve data-node for node */
        rc = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                       (ClCntDataHandleT *)&pCompInstance);
        if(CL_OK != rc)
        {
            clLogError("AGT", "CLI",
                    "User data get failed with error [0x%x]", rc);
            return rc;
        }

         pOutMsg[0] = '\0';

        if(pCompInstance->serviceCapability == CL_TXN_AGENT_SERVICE_2PC)
        {
            sprintf(pOutMsg,
                    "   %6d\t|   2PC Capable  \n",
                    pCompInstance->serviceType);
        }
        else if(pCompInstance->serviceCapability == CL_TXN_AGENT_SERVICE_1PC)
        {
            sprintf(pOutMsg,
                    "   %6d\t|   1PC Capable  \n",
                    pCompInstance->serviceType);
        }

        clXdrMarshallArrayClUint8T(&pOutMsg, strlen((char*)pOutMsg), outMsg, 0);
        
        node = currNode;
        rc = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        rc = CL_OK;
    
    CL_FUNC_EXIT();
    return(rc);
}

ClRcT _clTxnAgentProcessMgrCmd(
        CL_IN   ClBufferHandleT     inMsgHandle,
        CL_IN   ClBufferHandleT     outMsgHandle,
        CL_IN   ClTxnMessageHeaderT *pMsgHdr)
{
    ClRcT   rc  = CL_OK;
    ClTxnCmdT   tCmd = {.resp = CL_OK };
    ClTxnCommHandleT commHandle = NULL;
    ClUint32T   mCount = pMsgHdr->msgCount;
    ClTimeT t1, t2;

    
    CL_FUNC_ENTER();

    clLogDebug("AGT", NULL,
            "Received processing cmd from TM. Message count [%d]", 
            pMsgHdr->msgCount);
    t1 = clOsalStopWatchTimeGet();

    while ( (CL_OK == tCmd.resp) && (pMsgHdr->msgCount > 0) )
    {

        pMsgHdr->msgCount--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(inMsgHandle, &tCmd);
        switch (tCmd.cmd)
        {
            case CL_TXN_CMD_INIT:
                rc = clTxnAgentTxnDefnReceive(tCmd, inMsgHandle); 
                if(CL_OK != rc)
                {
                    clLogError("AGT", "MTA",
                            "Failed to process init command from manager, rc=[0x%x]",
                            rc);
                    /* Construct payload to send response back to server */
                    clTxnMutexLock(clTxnAgntCfg->actMtx);
                    
                   /* tCmd.resp = rc; */

                    rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                                      pMsgHdr->srcAddr, 
                                                      CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                                      NULL, CL_TXN_RMD_DFLT_TIMEOUT,  
                                                      CL_TXN_COMMON_ID, /* Value is 0x1 */
                                                      &commHandle);
                    if (CL_OK == rc)
                    {
                        rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &tCmd);

                        if (CL_OK != rc)
                        {
                            clLogError("AGT", NULL,
                                    "Failed to append cmd in the response with error [0x%x]",
                                    rc); 
                            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
                            break;
                        }
                        rc = clTxnCommIfcSessionRelease(commHandle);
                        if (CL_OK != rc)
                        {
                            clLogError("AGT", NULL,
                                    "Failed to release session with error[0x%x]",
                                    rc); 
                            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
                            break;
                        }
                        clTxnMutexUnlock(clTxnAgntCfg->actMtx);
                    }
                    else
                        clLogError("AGT", "ATM", 
                                "Failed to create new session for key [Node:0x%x,Port:0x%x]", 
                                pMsgHdr->srcAddr.nodeAddress, pMsgHdr->srcAddr.portId);
                    break;

                }
                /* Request for the first time */
                if(mCount == (pMsgHdr->msgCount + 1) )
                {
                    rc = _clTxnAgentTxnStart(tCmd);
                    if(CL_OK != rc)
                    {
                        /*tCmd.resp = rc; */
                        clLogError("AGT", "MTA",
                                "Failed to start transaction[0x%x:0x%x], rc=[0x%x]",
                                tCmd.txnId.txnMgrNodeAddress,
                                tCmd.txnId.txnId,
                                rc);
                        break;
                    }
                    clLogDebug("AGT", "MTA",
                            "Transaction[0x%x:0x%x] started",
                            tCmd.txnId.txnMgrNodeAddress,
                            tCmd.txnId.txnId);
                }
                break;

            case CL_TXN_CMD_PREPARE:
            case CL_TXN_CMD_1PC_COMMIT:
            case CL_TXN_CMD_2PC_COMMIT:
            case CL_TXN_CMD_ROLLBACK:
                rc = clTxnAgentProcessJob(pMsgHdr, tCmd, outMsgHandle, &commHandle);
                if(CL_OK != rc)
                {
                  /*  tCmd.resp = rc; */
                    clLog(CL_LOG_ERROR, "AGT", NULL,
                            "Error in processing cmd [%s] from server. rc [0x%x]", 
                            _clTxnCmdNameGet(tCmd.cmd), rc);
                }
                if(!pMsgHdr->msgCount && 
                    ( (tCmd.cmd == CL_TXN_CMD_ROLLBACK) || 
                      (tCmd.cmd == CL_TXN_CMD_2PC_COMMIT) ) 
                  )
                {
                    rc = _clTxnAgentTxnStop(tCmd);
                    if(CL_OK != rc)
                    {
                       /* tCmd.resp = rc; */
                        clLogError("AGT", "MTA",
                                "Failed to stop transaction[0x%x:0x%x], rc=[0x%x]",
                                tCmd.txnId.txnMgrNodeAddress,
                                tCmd.txnId.txnId, rc);
                    }
                    else
                        clLogDebug("AGT", "MTA",
                                "Transaction[0x%x:0x%x] stopped",
                                tCmd.txnId.txnMgrNodeAddress,
                                tCmd.txnId.txnId);
                }
                
                /* Remove the joblist when ROLLBACK or COMMIT is complete */
                if( (tCmd.cmd == CL_TXN_CMD_ROLLBACK) || 
                    (tCmd.cmd == CL_TXN_CMD_2PC_COMMIT) ) 
                {
                    ClTxnDefnPtrT pTxnDefn = NULL;
                    clLogDebug("AGT", NULL,
                            "Received remove cmd from server");
                    rc = clTxnDbTxnDefnGet(clTxnAgntCfg->activeTxnMap, 
                                    tCmd.txnId, &pTxnDefn);
                    if(CL_OK == rc)
                    {
                        if(1 < pTxnDefn->jobCount)
                        {
                            rc = clTxnAppJobRemove(pTxnDefn, tCmd.jobId);
                            if(CL_OK != rc)
                            {
                                ClNameT name = {0};
                                clCpmComponentNameGet(0, &name);
                                clLog(CL_LOG_ERROR, "AGT", NULL,
                                     "REMOVE cmd received. Error in removing the job information for component [%s] rc [0x%x]", 
                                     name.value, rc);
                                return rc;
                            }

                        }
                        else if(1 == pTxnDefn->jobCount) /* This is the last job, delete the entire txn list */
                        {
                            ClCntNodeHandleT nodeHandle;
                            rc  = clCntNodeFind(
                                    clTxnAgntCfg->activeTxnMap, 
                                    (ClCntKeyHandleT )&(tCmd.txnId), 
                                    &nodeHandle);
                            if(rc == CL_OK)
                            {
                                rc = clCntNodeDelete(clTxnAgntCfg->activeTxnMap, nodeHandle);
                                if(CL_OK != rc)
                                {
                                    clLog(CL_LOG_ERROR, "AGT", NULL, 
                                         "REMOVE cmd received. Error in deleting txn defn for txnId [0x%x] rc [0x%x]", 
                                         tCmd.txnId.txnId, rc);
                                    return rc;
                                }
                            }

                        }
                        else
                        {
                            clLog(CL_LOG_ERROR, "AGT", NULL,
                            "Remove cmd received. There is no txndefn corresponding to txnId [0x%x]. Jobcount [%d]\n",
                             tCmd.txnId.txnId, pTxnDefn->jobCount);
                        }
                    }
                }
                
                break;
                default:
                    clLog(CL_LOG_ERROR, "AGT", NULL, 
                        "Invalid command received from TM [0x%x]", tCmd.cmd);
                    rc = CL_ERR_INVALID_PARAMETER;
                    break;
        }
    }
    if((tCmd.cmd != CL_TXN_CMD_REMOVE_JOB) && commHandle)
    {
        rc = clTxnCommIfcReadMessage(commHandle, outMsgHandle);
        if(CL_OK != rc)
        {
            clLogError("AGT", NULL,
                    "Failed to write the response with error [0x%x]",
                    rc);
        }
        else
            clLogDebug("AGT", "ATM",
                    "Successfully sent response back");
        t2 = clOsalStopWatchTimeGet();
        clLogDebug("AGT", NULL,
                "Time taken to complete command[%d], [%lld]usecs",
                tCmd.cmd, (t2-t1) );
    }

    if (CL_OK != rc)
    {
            rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}


ClRcT _clTxnAgentProcessClientCmd(
        CL_IN   ClBufferHandleT  inMsgHandle,
        CL_OUT   ClBufferHandleT  outMsgHandle,
        CL_IN   ClTxnMessageHeaderT     *pMsgHdr)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           mCount = pMsgHdr->msgCount;
    ClTxnStartStopT     startstop = CL_TXN_DEFAULT;
    ClTxnCommHandleT    commHandle;
    
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("To processing %d messages", pMsgHdr->msgCount));
        
    rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP_TO_CLIENT, 
                                      pMsgHdr->srcAddr, 
                                      CL_TXN_CLIENT_MGR_RESP_RECV, 
                                      NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
                                      CL_TXN_COMMON_ID,
                                      &commHandle);
    clLogTrace("AGT", "RDT", 
            "[%d] Message(s) received from client [0x%x:0x%x]", 
            pMsgHdr->msgCount,
            pMsgHdr->srcAddr.nodeAddress,
            pMsgHdr->srcAddr.portId);
    while ( (CL_OK == rc) && (pMsgHdr->msgCount > 0) )
    {
        ClTxnCmdT   tCmd;

        pMsgHdr->msgCount--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(inMsgHandle, &tCmd);
        switch (tCmd.cmd)
        {
            case CL_TXN_CMD_READ_JOB:
                if(mCount == (pMsgHdr->msgCount + 1) )
                {
                    startstop = CL_TXN_START;
                }
                if(!pMsgHdr->msgCount)
                {
                    startstop |= CL_TXN_STOP;
                }
                clLogDebug("AGT", NULL,
                        "Processing stop, startstop[%d], mCount[%d], msgCount[%d]",
                        startstop, mCount, pMsgHdr->msgCount);
                rc = clTxnAgentReadJob(tCmd, inMsgHandle, commHandle, startstop);
                if(CL_OK != rc)
                {
                    clLogError("AGT", NULL,
                            "Failed to process read job, rc=[0x%x]", rc);
                }
                startstop = CL_TXN_PHASE;

                break;
            
            default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid comamnd received 0x%x", tCmd.cmd));
                rc = CL_ERR_INVALID_PARAMETER;
                break;
        }
    }

    rc = clTxnCommIfcSessionRelease(commHandle);
    rc = clTxnCommIfcReadMessage(commHandle, outMsgHandle);

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to process all txn-cmds. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Key compare function for managing various services of a component
 */
ClInt32T _clTxnCmpServiceKeyCompare(CL_IN ClCntKeyHandleT key1, 
                             CL_IN ClCntKeyHandleT key2)
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();

    return ( *((ClUint32T *)key1) - *((ClUint32T *)key2) );
}

/**
 * Hash function used for managing hash-map of services of a component.
 */
ClUint32T _clTxnCmpServiceHashFn(CL_IN ClCntKeyHandleT key)
{
    CL_FUNC_ENTER();

    CL_FUNC_EXIT();
    return ( *((ClUint32T *)key) % CL_TXN_NUM_BUCKETS );
}

/**
 * Callback function to delete service-registration information at agent
 */
void _clTxnCompServiceDelete(CL_IN ClCntKeyHandleT userKey, 
                             CL_IN ClCntDataHandleT userData)
{
    ClTxnAgentCompServiceInfoT   *pCompService = NULL;
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting an entry for txn-agent component-service\n"));
    pCompService = (ClTxnAgentCompServiceInfoT *) userData;

    if(pCompService)
    {
        clHeapFree(pCompService->pCompCallbacks);
        clHeapFree(pCompService);
    }
}
static ClCharT* _clTxnCmdNameGet(ClUint32T cmd)
{
    switch(cmd)
    {
        case CL_TXN_CMD_PREPARE:
            return "PREPARE";
        case CL_TXN_CMD_1PC_COMMIT:
            return "1PC COMMIT";
        case CL_TXN_CMD_2PC_COMMIT:
            return "2PC COMMIT";
        case CL_TXN_CMD_ROLLBACK:
            return "ROLLBACK";
        default:
            clLog(CL_DEBUG_ERROR, "AGT", NULL,
                    "Cannot find valid cmd corresponding to [%d]", 
                    cmd);
            return "INVALID";

    }
}
