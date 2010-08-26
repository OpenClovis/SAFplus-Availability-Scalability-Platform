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
 * ModuleName  : txn                                                           
 * File        : clTxnService.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * File        : clTxnService.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
#define __SERVER__
#include <string.h>

#include <clDebugApi.h>
#include <clBufferApi.h>
#include <clVersionApi.h>
#include <clXdrApi.h>
#include <clVersion.h>

#include <clTxnErrors.h>
#include <clTxnLog.h>
#include <clTxnStreamIpi.h>
#include <xdrClTxnMessageHeaderIDLT.h>
#include <xdrClTxnCmdT.h>
#include <clTxnXdrCommon.h>
#include <clTxnApi.h>

#include "clTxnServiceIpi.h"

#include "clTxnEngine.h"

#ifdef RECORD_TXN

#include <clTxnCommonDefn.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#endif
/**
 * List of supported version (transaction management)
 */
ClVersionT   clTxnMgmtVersionSupported[] = {
    { 'B', 1, 1 }
};

/**
 * Version Database
 */
ClVersionDatabaseT clTxnMgmtVersionDb = {
    sizeof(clTxnMgmtVersionSupported) / sizeof(ClVersionT), 
    clTxnMgmtVersionSupported
};


#ifdef RECORD_TXN

ClTxnDbgT gTxnDbg = {0}; /* Use to record txn */

ClRcT   _clTxnDumpBuff(ClBufferHandleT buff); 
#endif

/* Forward Declarations (RMD Receive Functions) */

/* Process client request, used by cli and txn client*/
ClRcT 
VDECL(clTxnServiceClientReqRecv)(
        CL_IN   ClEoDataT        cData, 
        CL_IN   ClBufferHandleT  inMsgHandle, 
        CL_OUT  ClBufferHandleT  outMsgHandle);

ClRcT _clTxnServiceTransactionExecute(CL_IN ClTxnDefnT *pTxnDefn, 
                                      CL_IN ClUint32T flag);

static ClRcT   
_clTxnReadConfig(void);
ClTxnServiceT       *clTxnServiceCfg = NULL;

#undef __CLIENT__
#include "clTxnServerFuncTable.h"

/**
 * Initializes transaction service by installing RMD callout functions and initializing
 * internal data-structure and protocol handlers. Additional parameter of eoClientId is
 * passed to differentiate between the case where transaction-service could be native or
 * additional service.
 */
ClRcT clTxnServiceInitialize(
        CL_IN ClEoExecutionObjT *pEoObj, 
        CL_IN ClEoClientIdT eoClientId, 
        CL_OUT ClTxnServiceHandleT *pTxnSrvcHandle)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();


    if (clTxnServiceCfg != NULL)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Duplicate initializing of txn-service"));
        CL_FUNC_EXIT();
        return CL_ERR_INITIALIZED;
    }

    if ( NULL == pTxnSrvcHandle )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguement\n"));
        rc = CL_ERR_NULL_POINTER;
        goto fatal_error;
    }
    
    /* Call Data-structure initialization routines.  */
    clTxnServiceCfg = (ClTxnServiceT *) clHeapAllocate(sizeof(ClTxnServiceT));
    if (NULL == clTxnServiceCfg)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_ERR_NO_MEMORY;
        goto fatal_error;
    }
    memset(clTxnServiceCfg, 0, sizeof(ClTxnServiceT));

    /* Enable checkpoint by default */
    clTxnServiceCfg->dsblCkpt = CL_FALSE;
    /* Initialize communication interface */
    rc = clTxnCommIfcInit(&(clTxnMgmtVersionSupported[0]));

    if (CL_OK == rc)
    {
        /* Initialize txnServiceMutex for protecting txnIdCounter */
        rc = clOsalMutexCreateAndLock ( &(clTxnServiceCfg->txnServiceMutex) );
        clTxnServiceCfg->txnIdCounter = 0x0U;
    }

    /* Enable or disable ckpt */

    if(CL_OK == rc)
    {
        rc = _clTxnReadConfig();
    }
    /* Initialize transaction service ckpt impl */
    if (CL_OK == rc)
    {
        rc = clTxnServiceCkptInitialize(); 
    }

    if (CL_OK == rc)
    {
        if(clTxnServiceCfg->retVal)
        {
            /* Restore the previously known state */
            rc = clTxnServiceCkptAppStateRestore();
        }
        else
            clLogDebug("SER", NULL,
                    "Checkpoint does not exist");
    }

    /* Initialize the back-end transaction engine */
    if (CL_OK == rc)
    { 
        rc = clTxnEngineInitialize();
    }

    if (CL_OK != rc)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_2_SERVICE_START_FAILED, "Transaction", rc);
        clOsalMutexUnlock (clTxnServiceCfg->txnServiceMutex);
        clOsalMutexDelete (clTxnServiceCfg->txnServiceMutex);
        clHeapFree(clTxnServiceCfg);
        clTxnServiceCfg = NULL;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Txn-Service Initialization done\n"));
        *pTxnSrvcHandle = (ClTxnServiceHandleT ) clTxnServiceCfg;
        clOsalMutexUnlock (clTxnServiceCfg->txnServiceMutex);
    }

#ifdef RECORD_TXN

    {
#define CL_TXN_FILE_NAME    "txnRecords"
        ClCharT *env = NULL;
        ClCharT  loc[256] = {0};
        ClCharT file[256] = {0}; 

        
        env = getenv("ASP_DBDIR");
        if(!env)
        {
            clLogError("SER", "DBG",
                    "Error in getting value of ASP_DBDIR");
            return rc;
        }

        snprintf(loc, sizeof(loc) - 1, "%s/txn", env);
        
        /* create dir to store the txn records */
        rc = mkdir (loc, 0755);
        if ((-1 == rc) && (EEXIST != errno))
        {
            clLogError("INT", "DBP", 
                    "Failed while creating dir[%s] - [%s]", 
                    loc, strerror(errno));
            return rc;
        }
        snprintf(file, sizeof(file) - 1, "%s/%s", loc, CL_TXN_FILE_NAME);
        clLogInfo("SER", "DBG",
                "File [%s] stores txn records", file);

        /* Open file in append mode */
        gTxnDbg.fd = fopen(file, "a+");
        if(!gTxnDbg.fd)
        {
            clLogError("SER", "DBG",
                    "Error in opening file txnhistory, error [%s]", strerror(errno));
            return CL_ERR_NOT_EXIST;
        }
        
        clTxnMutexCreate(&gTxnDbg.mtx); /* mutex used to protect the record file */
    }

#endif
    rc = clEoClientInstallTables(pEoObj,
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXN));
    if(CL_OK != rc)
    {
        clLogError("SER", "INT", "Error in installing function table, rc [0x%x]", rc);
        return rc;
    }

    /*This is required since txn client initization does not happen through clEoClientLibs */

    {
        ClVersionT  version = {'B', 0x1, 0x1};
        ClTxnClientHandleT txnHandle;

        rc = clTxnClientInitialize(&version, NULL, &txnHandle);
    }
    
fatal_error:
    CL_TXN_RETURN_RC(rc, ("Failed to initialize transaction-service rc:0x%x\n", rc));
}

/**
 * Finalize transaction service by terminating all on-going transaction processing.
 */
ClRcT clTxnServiceFinalize(CL_IN ClTxnServiceHandleT txnSrvcHandle,
                           CL_IN ClEoClientIdT eoClientId)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT   *pEoObj = NULL;
    CL_FUNC_ENTER();

    /*
      - Remove RMD callouts from the EO Client Table
      - Free any memory allocated (via containers)
    */

    rc = clEoMyEoObjectGet(&pEoObj);
    if (CL_OK == rc) 
    {
        clEoClientUninstallTables(pEoObj,
                                  CL_EO_SERVER_SYM_MOD(gAspFuncTable, TXN));
    }
    rc = clTxnEngineFinalize();

    if (CL_OK == rc)
    {
        rc = clTxnServiceCkptFinalize();
    }

    clTxnCommIfcFini();

    clOsalMutexDelete (clTxnServiceCfg->txnServiceMutex);

    clHeapFree(clTxnServiceCfg);
    clTxnServiceCfg = NULL;

#ifdef RECORD_TXN

    fclose(gTxnDbg.fd);
    
    clTxnMutexDelete(gTxnDbg.mtx); /* mutex used to protect the record file */

#endif
    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 * RMD Receive Function. This function invokes transaction execution for 
 * a fully defined transaction with job-details.
 */
ClRcT VDECL(clTxnServiceClientReqRecv)(
        CL_IN   ClEoDataT        cData, 
        CL_IN   ClBufferHandleT  inMsgHandle, 
        CL_OUT  ClBufferHandleT  outMsgHandle)
{
    ClRcT                   rc = CL_OK;
    ClTxnMessageHeaderT     msgHeader;
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}}; 

    CL_FUNC_ENTER();
    /* The incoming buffer contains the configuration information regarding
       newly formed transaction, job-ids and involved component-addresses.

       Extract the configuration details and start the transaction for each job.
    */
    memset(&msgHeader, 0, sizeof(ClTxnMessageHeaderT));
    clLogDebug("SER", "CTM",
            "Received client request");
    if (NULL == clTxnServiceCfg)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL, CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Either Txn-Server is not initialized or has been terminated\n"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

#ifdef RECORD_TXN
    clTxnMutexLock(gTxnDbg.mtx);

    rc = _clTxnDumpBuff(inMsgHandle); 
    if(CL_OK != rc)
    {
        clLogError("SER", "DBG",
                "Error in dumping msg header, rc [0x%x]", rc);
        clTxnMutexUnlock(gTxnDbg.mtx);
        return rc;
    }
    clTxnMutexUnlock(gTxnDbg.mtx);

#endif
    /* Extract the header */
    if ( (rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(inMsgHandle, &msgHeadIDL) ) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to read header-informatino from incoming msg. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return CL_TXN_RC(rc);
    }

    _clTxnCopyIDLToMsgHead(&msgHeader, &msgHeadIDL) ;
    CL_ASSERT(msgHeader.msgCount);

    /* Check/Validate server version */
    rc = clVersionVerify (&clTxnMgmtVersionDb, &(msgHeader.version));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Un-supported version. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (CL_TXN_RC(rc));
    }


    switch (msgHeader.msgType)
    {
        case CL_TXN_MSG_CLIENT_REQ:
            rc = _clTxnServiceClientReqProcess(msgHeader, inMsgHandle, outMsgHandle, 0);
            break;

        case CL_TXN_MSG_COMP_STATUS_UPDATE:
            rc = CL_ERR_NOT_IMPLEMENTED;
            break;

        default:
            rc = CL_ERR_INVALID_PARAMETER;
            break;
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to process client-request. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}



/**********************************************************************************
 *                              Internal Functions                                *
 **********************************************************************************/

/**
 * Internal API
 *
 * This function receives new transaction definition from client and prepares
 * for its processing
 */
ClRcT _clTxnServiceClientReqProcess(
        CL_IN   ClTxnMessageHeaderT     msgHeader,
        CL_IN   ClBufferHandleT  inMsgHandle,
        CL_IN   ClBufferHandleT  outMsgHandle, 
        CL_IN   ClUint8T         cli)
{
    ClRcT           rc  =   CL_OK;


    CL_FUNC_ENTER();

    clLogDebug("CTM", NULL, 
            "To processing [0x%x] messages.", msgHeader.msgCount);

    while ( (CL_OK == rc) && (msgHeader.msgCount > 0) )
    {
        ClTxnCmdT       tCmd;
        ClTxnDefnT      *pTxnDefn;

        msgHeader.msgCount--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(inMsgHandle, &tCmd);

        clLogDebug("CTM", NULL, 
                "Received cmd[0x%x] for txnId [0x%x:0x%x]", 
                tCmd.cmd, tCmd.txnId.txnMgrNodeAddress, tCmd.txnId.txnId);

        if (CL_OK == rc)
        {
            switch (tCmd.cmd)
            {
                case CL_TXN_CMD_PROCESS_TXN:
                    // continue...
                    break;

                case CL_TXN_CMD_CANCEL_TXN:
                    tCmd.resp = rc = CL_ERR_NOT_IMPLEMENTED;    /* FIXME */
                    break;

                default:
                    tCmd.resp = rc = CL_ERR_INVALID_PARAMETER;
                    break;

            }
        }

        if (CL_OK != rc)
        { 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to process incoming message cmd[0x%x]. rc:0x%x", tCmd.cmd, rc));
            continue;
        }

        rc = clTxnStreamTxnCfgInfoUnpack(inMsgHandle, &pTxnDefn);
        if (CL_OK == rc)
        {
            rc = clOsalMutexLock (clTxnServiceCfg->txnServiceMutex);
            if (rc != CL_OK) clLogError("SER", "CTM", "Failed to lock mutex: rc [0x%x]", rc);
            CL_ASSERT(CL_OK == rc);

            pTxnDefn->serverTxnId.txnId = clTxnServiceCfg->txnIdCounter++;
            pTxnDefn->serverTxnId.txnMgrNodeAddress = clIocLocalAddressGet();

            rc = clOsalMutexUnlock (clTxnServiceCfg->txnServiceMutex);
            CL_ASSERT(CL_OK == rc);            

        }

        if (CL_OK == rc)
        {
            /* Checkpoing txn-service app-state */
            rc = clTxnServiceCkptAppStateCheckpoint();
        }

        /* Call internal function to execute the transaction */
        if (CL_OK == rc)
        {
            rc = clTxnEngineNewTxnReceive(pTxnDefn, msgHeader.srcAddr, CL_FALSE, outMsgHandle, cli);
        }

        if (CL_OK != rc)
        {
            clLogError("SER", "CTM",
                    "Failed to queue transaction with txnId [0x%x:0x%x] received from [0x%x:0x%x] for processing, rc [0x%x]", 
                    tCmd.txnId.txnMgrNodeAddress,
                    tCmd.txnId.txnId,
                    msgHeader.srcAddr.nodeAddress,
                    msgHeader.srcAddr.portId,
                    rc);
            if (pTxnDefn != NULL)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                           CL_TXN_LOG_1_TRANSACTION_FAILED, pTxnDefn->serverTxnId.txnId);
            }
            else
            {
                /* FIXME: Provide further log message for error while reading transaction definition */
            }

            /* Prepare to send the response back to client */
            rc = CL_TXN_RC(rc);
        }
    }

    CL_FUNC_EXIT();
    return rc;
}

static ClRcT _clTxnReadConfig(void)
{
    ClRcT   rc = CL_OK;

    if (clParseEnvBoolean("CL_TXN_DSBLE_CKPT") == CL_TRUE)
    {
        clLogNotice("SER", NULL, 
                "Checkpoint is disabled");
        clTxnServiceCfg->dsblCkpt = CL_TRUE;
    }
    else
    {
        clLogNotice("SER", NULL, 
                "Checkpoint is enabled");
        clTxnServiceCfg->dsblCkpt = CL_FALSE;
    }

    return rc;
}
#ifdef RECORD_TXN
ClRcT   _clTxnDumpBuff(ClBufferHandleT buff) 
{
    ClRcT   rc = CL_OK;
    ClTimeT             tstamp = 0; /* Time stamp */
    ClUint8T            *flatbuff = NULL;
    ClBufferHandleT     copybuff = NULL;
    ClUint32T           buflen = 0;

    rc = clBufferCreate(&copybuff);
    if(CL_OK != rc)
    {
        clLogError("SER", "DBG",
                "Error in creating copy buffer handle, rc [0x%x]", rc);
        return rc;
    }
    rc = clBufferLengthGet(buff, &buflen);
    if(CL_OK != rc)
    {
        clLogError("SER", "DBG",
                "Error in getting length of the buff, rc [0x%x]", 
                 rc);
        clBufferDelete(&copybuff);
        return rc;
    }

    rc = clBufferToBufferCopy(buff, 0, copybuff, buflen);
    if(CL_OK != rc)
    {
        clLogError("SER", "DBG",
                "Error in buffer to buffer copy of length [%d] bytes, rc [0x%x]", 
                buflen, rc);
        clBufferDelete(&copybuff);
        return rc;
    }

    clLogInfo("SER", "DBG", "Length of buffer handle to write [%d] bytes", buflen);

    /* Dump incoming buffer */
    fseek(gTxnDbg.fd, 0L, SEEK_END);
    
    /* Dump timestamp */
    
    tstamp = clOsalStopWatchTimeGet(); 

    clLogInfo("CLI", "DBG",
            "time stamp received[%lld]", tstamp);
    
    /* Write timestamp */
    clLogInfo("SER", "DBG", "Dumped time stamp [%zd] bytes into txnRecords ", fwrite(&tstamp, 1, sizeof(tstamp), gTxnDbg.fd));

    /*Write buff length */
    clLogInfo("SER", "DBG", "Dumped msg length of [%zd] bytes into txnRecords ", fwrite(&buflen, 1, sizeof(buflen), gTxnDbg.fd));

    /* Flatten buffer handle to write into file */
    rc = clBufferFlatten(copybuff, &flatbuff);
    if(CL_OK != rc)
    {
        clLogError("SER", "DBG",
                "Error while flattening buffer, rc [0x%x]", rc);
        clBufferDelete(&copybuff);
        return rc;
    }

    clLogInfo("SER", "DBG", "Dumped msgHeader of length [%zd] into file", fwrite(flatbuff, 1, buflen, gTxnDbg.fd));

    clHeapFree(flatbuff);
    clBufferDelete(&copybuff);
    return rc;
}
#endif



