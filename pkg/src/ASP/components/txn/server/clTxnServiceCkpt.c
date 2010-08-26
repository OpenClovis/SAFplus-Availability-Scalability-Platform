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
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/txn/server/clTxnServiceCkpt.c $
 * $Author: anil $
 * $Date: 2007/04/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This source file contains integration with checkpoint service for restart
 * capabilities
 *
 *
 *****************************************************************************/

#include <string.h>

#include <clDebugApi.h>
#include <clBufferApi.h>
#include <clCkptExtApi.h>
#include <clXdrApi.h>

#include <clTxnErrors.h>
#include <clTxnStreamIpi.h>

#include "clTxnServiceIpi.h"
#include "clTxnRecovery.h"

extern ClTxnServiceT       *clTxnServiceCfg;

/* Forward Declarations (Private Functions for checkpointing) */
/* Serializer function for Transaction Service Data-Set */
static ClRcT _clTxnServiceCkptAppStatePack(
        CL_IN   ClUint32T   dataSetId, 
        CL_OUT  ClAddrT     *ppData, 
        CL_OUT  ClUint32T   *pDataLen, 
        CL_IN   ClPtrT      cookie);

/* Deserializer function for Transaction Service Data-Set */
static ClRcT _clTxnServiceCkptAppStateUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen, 
        CL_IN   ClPtrT      cookie);

/* Serializer function for Transaction Definition Data-Set */
static ClRcT _clTxnServiceCkptTxnPack(
        CL_IN   ClUint32T   dataSetId, 
        CL_OUT  ClAddrT     *ppData, 
        CL_OUT  ClUint32T   *pDataLen, 
        CL_IN   ClPtrT      cookie);

/* Deserializer function for Transaction Definition Data-Set */
static ClRcT _clTxnServiceCkptTxnUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen,
        CL_IN   ClPtrT      cookie); 

/* Dummy serialiser function for Recovery log Data-Set */
static ClRcT _clTxnCkptDummyPackFn(
        CL_IN   ClUint32T   dataSetId,
        CL_OUT  ClAddrT     *ppData,
        CL_OUT  ClUint32T   *pDataLen,
        CL_IN   ClPtrT      cookie);

/* Dummy deserialiser function for Recovery log Data-Set */
static ClRcT _clTxnCkptDummyUnpackFn(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen,
        CL_IN   ClPtrT      cookie); 

static ClRcT _clTxnServiceCkptTxnStatePack(
        CL_IN   ClUint32T   dataSetId,
        CL_OUT  ClAddrT     *ppData,
        CL_OUT  ClUint32T   *pDataLen,
        CL_IN   ClPtrT      cookie);

static ClRcT _clTxnServiceCkptTxnStateUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen,
        CL_IN   ClPtrT      cookie); 
/**
 * Initializes Checkpoint for transaction-service. 
 * This function initializes checkpoint service with check-point name
 * and creates default data-set to hold transaction-service-state.
 */
ClRcT clTxnServiceCkptInitialize()
{
    ClRcT       rc      = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        rc = clCkptLibraryInitialize(&(clTxnServiceCfg->txnCkptHandle) );

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize transaction ckpt library. rc:0x%x", rc));
            CL_FUNC_EXIT();
            return CL_GET_ERROR_CODE(rc);
        }

        /* Initialize check-point name and create checkpoint for txn-service */
        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        clCkptLibraryDoesCkptExist(clTxnServiceCfg->txnCkptHandle, &txnCkptName, &(clTxnServiceCfg->retVal));
        rc = clCkptLibraryCkptCreate(clTxnServiceCfg->txnCkptHandle, &txnCkptName); 

        if (CL_OK == rc)
        {
            rc = clCkptLibraryCkptDataSetCreate(clTxnServiceCfg->txnCkptHandle, 
                                                &txnCkptName, CL_TXN_SERVICE_DATA_SET,
                                                0, 0, 
                                                _clTxnServiceCkptAppStatePack, 
                                                _clTxnServiceCkptAppStateUnpack);
        }

        if (CL_OK == rc)
        {
            /* Create data-set for transaction logs requied for recovery */
            rc = clCkptLibraryCkptDataSetCreate(clTxnServiceCfg->txnCkptHandle, 
                                                &txnCkptName, CL_TXN_RECOVERY_DATA_SET,
                                                0, 0,
                                                _clTxnCkptDummyPackFn,  /* Dummy fn */
                                                _clTxnCkptDummyUnpackFn);   /* Dummy fn */
        }

        if (CL_OK == rc)
        {
            /* Create elements of this data-set */
            rc = clCkptLibraryCkptElementCreate(clTxnServiceCfg->txnCkptHandle, 
                                                &txnCkptName, CL_TXN_RECOVERY_DATA_SET, 
                                                _clTxnServiceCkptTxnStatePack, 
                                                _clTxnServiceCkptTxnStateUnpack);
        }

        if (CL_OK != rc) 
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create checkpoint/data-set. rc:0x%x", rc));
            clCkptLibraryCkptDelete(clTxnServiceCfg->txnCkptHandle, &txnCkptName);
            clCkptLibraryFinalize(clTxnServiceCfg->txnCkptHandle);
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Finalize Transaction service Checkpointing
 */
ClRcT clTxnServiceCkptFinalize()
{
    ClRcT       rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        rc = clCkptLibraryCkptDataSetDelete (clTxnServiceCfg->txnCkptHandle, 
                                             &txnCkptName, CL_TXN_SERVICE_DATA_SET);
        rc = clCkptLibraryCkptDataSetDelete (clTxnServiceCfg->txnCkptHandle, 
                                             &txnCkptName, CL_TXN_RECOVERY_DATA_SET);
        rc = clCkptLibraryCkptDelete(clTxnServiceCfg->txnCkptHandle, &txnCkptName);

        rc = clCkptLibraryFinalize(clTxnServiceCfg->txnCkptHandle);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete cktp. rc:0x%x", rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}


/**
 * API to checkpoint current state of transaction-service
 */
ClRcT clTxnServiceCkptAppStateCheckpoint()
{
    ClRcT rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        rc = clCkptLibraryCkptDataSetWrite(clTxnServiceCfg->txnCkptHandle, &txnCkptName, 
                                          CL_TXN_SERVICE_DATA_SET, 0x0);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to write data-set [0x%x], rc:0x%x", 
                           CL_TXN_SERVICE_DATA_SET, rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to restore status of transaction-service from check-point
 */
ClRcT clTxnServiceCkptAppStateRestore()
{
    ClRcT       rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Restoring transaction-service state"));

        clLogDebug("SER", NULL,
                "Ckpt exists, reading data");
        rc = clCkptLibraryCkptDataSetRead(clTxnServiceCfg->txnCkptHandle, &txnCkptName, 
                                          CL_TXN_SERVICE_DATA_SET, 0x0);

        if (CL_OK != rc)
        {
            clLogError("SER", NULL, 
                    "Failed to restore transaction-service state, rc [0x%x]", rc);
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to checkpoint recovery-log details of a given transaction.
 */
ClRcT clTxnServiceCkptRecoveryLogCheckpoint(
        CL_IN   ClTxnTransactionIdT     txnId)
{
    ClRcT   rc  = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;
        ClCharT     elementName[200];

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        elementName[0] = '\0';
        sprintf(elementName, "TXN_%x_%x", txnId.txnMgrNodeAddress, txnId.txnId);

        rc = clCkptLibraryCkptElementWrite (clTxnServiceCfg->txnCkptHandle, 
                                            &txnCkptName, CL_TXN_RECOVERY_DATA_SET, 
                                            elementName, (strlen(elementName) + 1),
                                            (ClCntHandleT)&txnId);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to create element of txn-recovery data for txn 0x%x:0x%x, rc:0x%x", 
                 txnId.txnMgrNodeAddress, txnId.txnId, rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}
/**
 * API to delete checkpoint recovery-log details of a given transaction.
 */
ClRcT clTxnServiceCkptRecoveryLogCheckpointDelete(
        CL_IN   ClTxnTransactionIdT     txnId)
{
    ClRcT   rc  = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;
        ClCharT     elementName[200];

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        elementName[0] = '\0';
        sprintf(elementName, "TXN_%x_%x", txnId.txnMgrNodeAddress, txnId.txnId);

        rc = clCkptLibraryCkptElementDelete (clTxnServiceCfg->txnCkptHandle, 
                                            &txnCkptName, CL_TXN_RECOVERY_DATA_SET, 
                                            elementName, (strlen(elementName) + 1));
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to delete element of txn-recovery data for txn 0x%x:0x%x, rc:0x%x", 
                 txnId.txnMgrNodeAddress, txnId.txnId, rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to restore recovery logs of previously active transactions
 * from check-point
 */
ClRcT clTxnServiceCkptRecoveryLogRestore()
{
    ClRcT       rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Restoring transaction-recovery logs"));

        /* AD-1: To use the checkpoint exists api to check whether its recovery or
         * normal startup */
        clLogDebug("SER", NULL,
                "Checkpoint exists, reading data for recovery");
        rc = clCkptLibraryCkptDataSetRead(clTxnServiceCfg->txnCkptHandle, &txnCkptName, 
                                          CL_TXN_RECOVERY_DATA_SET, 0x0);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to restore transaction-service state. rc:0x%x", rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to create a data-set for a newly created transaction
 */
ClRcT clTxnServiceCkptNewTxnCheckpoint(
        CL_IN   ClTxnDefnT  *pTxnDefn)
{
    ClRcT rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Create data-set for transaction 0x%x:0x%x",
                       pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId));

        rc = clCkptLibraryCkptDataSetCreate(clTxnServiceCfg->txnCkptHandle, 
                                            &txnCkptName, 
                                            pTxnDefn->serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET, 
                                            0, 0, 
                                            _clTxnServiceCkptTxnPack, /* Packs state of a given txn */
                                            _clTxnServiceCkptTxnUnpack); /* Unpack state of a given txn */

        if (CL_OK == rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Checkpointing transaction txnId: 0x%x:0x%x", 
                           pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId));
            
            rc = clCkptLibraryCkptDataSetWrite(clTxnServiceCfg->txnCkptHandle, 
                                               &txnCkptName, 
                                               pTxnDefn->serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET, 
                                               pTxnDefn);
        }

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to create data-set for transaction[0x%x:0x%x], rc:0x%x", 
                     pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId, rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to restore definition of a transaction from check-point data-set
 */
ClRcT clTxnServiceCkptTxnRestore(
        CL_IN   ClTxnTransactionIdT     serverTxnId)
{
    ClRcT rc = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Restoring transaction [0x%x:0x%x] from checkpoint", 
                       serverTxnId.txnMgrNodeAddress, serverTxnId.txnId));

        /* First initialize the data-set */
        rc = clCkptLibraryCkptDataSetCreate(clTxnServiceCfg->txnCkptHandle, 
                                            &txnCkptName, 
                                            serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET, 
                                            0, 0, 
                                            _clTxnServiceCkptTxnPack, /* Packs state of a given txn */
                                            _clTxnServiceCkptTxnUnpack); /* Unpack state of a given txn */

        if (CL_OK == rc)
        {
            rc = clCkptLibraryCkptDataSetRead(clTxnServiceCfg->txnCkptHandle, 
                                              &txnCkptName, 
                                              serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET, 
                                              &serverTxnId);
        }

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to read/restore transaction[0x%x:0x%x] rc:0x%x", 
                     serverTxnId.txnMgrNodeAddress, serverTxnId.txnId, rc));
            rc = CL_GET_ERROR_CODE(rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Delete the data-set created for a transaction 
 */
ClRcT clTxnServiceCkptTxnDelete(
        CL_IN   ClTxnTransactionIdT     serverTxnId)
{
    ClRcT   rc  = CL_OK;
    if(!clTxnServiceCfg->dsblCkpt)
    {
        ClNameT     txnCkptName;

        CL_FUNC_ENTER();

        txnCkptName.length = strlen(CL_TXN_CKPT_NAME) + 1;
        memset(&(txnCkptName.value[0]), '\0', txnCkptName.length);
        strncpy(&(txnCkptName.value[0]), CL_TXN_CKPT_NAME, txnCkptName.length); 

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting transaction [0x%x:0x%x] from checkpoint", 
                       serverTxnId.txnMgrNodeAddress, serverTxnId.txnId));

        /* First initialize the data-set */
        rc = clCkptLibraryCkptDataSetDelete(clTxnServiceCfg->txnCkptHandle, 
                                            &txnCkptName, 
                                            serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET);
    }

    CL_FUNC_EXIT();
    return (rc);
}


/******************************************************************************
 *                      Internal Functions                                    *
 *****************************************************************************/

/**
 * Internal Function - used to pack/prepare data-set for checkpointing 
 * transaction-service
 */
static ClRcT _clTxnServiceCkptAppStatePack(
        CL_IN   ClUint32T   dataSetId, 
        CL_OUT  ClAddrT     *ppData, 
        CL_OUT  ClUint32T   *pDataLen, 
        CL_IN   ClPtrT      cookie)
{
    ClRcT                       rc = CL_OK;
    ClBufferHandleT      txnSrvcStateBuff;

    CL_FUNC_ENTER();
    /* Walk through the list of transactions available in activeTxnMap.
       For each transaction, call clTxnStreamTxnCfgInfoPack()
       For the received message-buffer, append it to the master message-buffer.
    */

    if ( (ppData == NULL) || (pDataLen == NULL) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguements"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    rc = clBufferCreate (&txnSrvcStateBuff);

    if (CL_OK == rc)
    {
        clXdrMarshallClUint32T( &(clTxnServiceCfg->txnIdCounter), txnSrvcStateBuff, 0);

        /* Copy to buffer */
        *pDataLen = 0;
        *ppData = NULL;

        if (CL_OK == rc)
        {
            ClUint32T   ckptDataLen;

            rc = clBufferLengthGet(txnSrvcStateBuff, &ckptDataLen);

            if ( (CL_OK == rc) && ( ckptDataLen != 0) )
            {
                *ppData = (ClAddrT) clHeapAllocate(ckptDataLen);
                *pDataLen = ckptDataLen;
                if ( *ppData != NULL )
                {
                    rc = clBufferNBytesRead(txnSrvcStateBuff, 
                            (ClUint8T *) *ppData, &ckptDataLen);
                    if (CL_OK != rc)
                    {
                        *pDataLen = 0x0;
                        clHeapFree(*ppData);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create txn-service checkpoint. rc:0x%x", rc));
                        rc = CL_GET_ERROR_CODE(rc);
                    }


                }
                else
                {
                    rc = CL_ERR_NO_MEMORY;
                }
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Error while packing or no data rc:0x%x data-len:0%x", rc, ckptDataLen));
            }
        }

        rc = clBufferDelete(&txnSrvcStateBuff);

    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal function to pack/prepare state oa given transaction
 */
static ClRcT _clTxnServiceCkptTxnPack(
        CL_IN   ClUint32T   dataSetId, 
        CL_OUT  ClAddrT     *ppData, 
        CL_OUT  ClUint32T   *pDataLen, 
        CL_IN   ClPtrT      cookie)
{
    ClRcT                   rc = CL_OK;
    ClTxnDefnT              *pTxnDefn;
    ClBufferHandleT  txnStateBuf;
    ClUint32T               msgLen;

    CL_FUNC_ENTER();

    pTxnDefn = (ClTxnDefnT *) cookie;

    if (pTxnDefn == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument"));
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
            ("Packing for data-set 0x%x for txn[0x%x:0x%x]", dataSetId, 
             pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId));

    /* Validate between dataSetId and txn-Id */
#if 0
    if ( (dataSetId  - CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET) != pTxnId->txnId)
    {
        CL_TXN_RETURN_RC(CL_ERR_INVALID_PARAMETER, 
                ("Invalid data-set(0x%x) for transaction-id(0x%x)\n", 
                 dataSetId, pTxnId->txnId));
    }
#endif

    rc = clBufferCreate (&txnStateBuf);

    if (CL_OK == rc)
    {
        rc = clTxnStreamTxnCfgInfoPack (pTxnDefn, txnStateBuf);

        *ppData = NULL;
        *pDataLen = 0;

        if (CL_OK == rc)
        {
            /* Copy the state-information from message-buffer to ppData */
            rc = clBufferLengthGet(txnStateBuf, &msgLen);
        }

        if (CL_OK == rc)
        {
            *pDataLen = msgLen;
            *ppData = (ClInt8T *) clHeapAllocate(msgLen);
            if ( *ppData == NULL )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                rc = CL_ERR_NO_MEMORY;
            }
        }

        if (CL_OK == rc)
        {
            rc = clBufferNBytesRead(txnStateBuf, (ClUint8T *) *ppData, &msgLen);
            if (CL_OK != rc)
            {
                clHeapFree(*ppData);
                *pDataLen = 0x0;
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Failed to pack/serialize txn-defn[0x%x:0x%x] for ckpt. rc:0x%x",
                         pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId, rc));
                rc = CL_GET_ERROR_CODE(rc);
            }


        }

        rc = clBufferDelete(&txnStateBuf);

    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function - extracts transaction-service state from checkpoint data-set
 */
static ClRcT _clTxnServiceCkptAppStateUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen, 
        CL_IN   ClPtrT      cookie)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT  txnStateMsgBuf;
    ClUint32T               txnIdCounter;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("To restore state of transaction-service\n"));

    rc = clBufferCreate(&txnStateMsgBuf);
    if (CL_OK == rc)
    {
        rc = clBufferNBytesWrite(txnStateMsgBuf, (ClUint8T *) pData, dataLen);

        if (CL_OK == rc)
        {
            rc = clXdrUnmarshallClUint32T(txnStateMsgBuf, &txnIdCounter);
        }

        if (CL_OK == rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Updating txnId Counter to 0x%x", txnIdCounter));
            clTxnServiceCfg->txnIdCounter = txnIdCounter;
        }

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to restore transaction service state. Could result in inconsistent operation. rc:0x%x", rc));
            rc = CL_GET_ERROR_CODE(rc);
        }

        rc = clBufferDelete(&txnStateMsgBuf);

    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal function to unpack saved state of a transaction
 * from checkpoint
 */
static ClRcT _clTxnServiceCkptTxnUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen, 
        CL_IN   ClPtrT      cookie)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT  txnStateBuf;
    ClTxnDefnT              *pTxnDefn;

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("To unpack data-set 0x%x\n", dataSetId));
    rc = clBufferCreate(&txnStateBuf);

    if (CL_OK == rc)
    { 
        rc = clBufferNBytesWrite(txnStateBuf, (ClUint8T *)pData, dataLen);

        if (CL_OK == rc)
        { 
            rc = clTxnStreamTxnCfgInfoUnpack(txnStateBuf, &pTxnDefn);
        }

        if ( (CL_OK == rc) && 
                ((pTxnDefn->serverTxnId.txnId + CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET) == dataSetId) )
        {
            /* Add this definition into txnDefnDb */
            rc = clTxnRecoverySessionUpdate (pTxnDefn);
        }
        else
        {
            if (CL_OK == rc)
                rc = CL_ERR_INVALID_PARAMETER;
        }

        rc = clBufferDelete(&txnStateBuf);

    }
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to extract transaction definition from data-set[0x%x], rc:0x%x", dataSetId, rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/** 
 * Dummy serialiser function for Recovery log Data-Set 
 */
static ClRcT _clTxnCkptDummyPackFn(
        CL_IN   ClUint32T   dataSetId,
        CL_OUT  ClAddrT     *ppData,
        CL_OUT  ClUint32T   *pDataLen,
        CL_IN   ClPtrT      cookie)
{
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Bug in implementation"));

    CL_FUNC_EXIT();
    return (CL_OK);
}

/** 
 * Dummy deserialiser function for Recovery log Data-Set 
 */
static ClRcT _clTxnCkptDummyUnpackFn(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen,
        CL_IN   ClPtrT      cookie)
{
    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Bug in implementation"));

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Serializer function for transaction state
 */
static ClRcT _clTxnServiceCkptTxnStatePack(
        CL_IN   ClUint32T   dataSetId,
        CL_OUT  ClAddrT     *ppData,
        CL_OUT  ClUint32T   *pDataLen,
        CL_IN   ClPtrT      cookie)
{
    ClRcT                   rc  = CL_OK;
    ClTxnTransactionIdT     *pTxnId;
    ClBufferHandleT  msgHandle;
    ClUint32T               msgLen;

    CL_FUNC_ENTER();

    pTxnId = (ClTxnTransactionIdT *) cookie;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("To checkpoint recovery-log of txn 0x%x:0x%x", 
                   pTxnId->txnMgrNodeAddress, pTxnId->txnId));

    rc = clBufferCreate(&msgHandle);

    if (CL_OK == rc)
    {
        rc = clTxnRecoveryLogPack(pTxnId, msgHandle);

        *ppData = NULL;
        *pDataLen = 0;
        if (CL_OK == rc)
        {
            /* Copy the state-information from message-buffer to ppData */
            rc = clBufferLengthGet(msgHandle, &msgLen);
        }

        if (CL_OK == rc)
        {
            *pDataLen = msgLen;
            *ppData = (ClInt8T *) clHeapAllocate(msgLen);
            if ( *ppData == NULL )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                rc = CL_ERR_NO_MEMORY;
            }
        }

        if (CL_OK == rc)
        {
            rc = clBufferNBytesRead(msgHandle, (ClUint8T *) *ppData, &msgLen);
            if (CL_OK != rc)
            {
                clHeapFree(*ppData);
                *pDataLen = 0x0;
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Failed to pack/serialize txn-defn[0x%x:0x%x] for ckpt. rc:0x%x",
                         pTxnId->txnMgrNodeAddress, pTxnId->txnId, rc));
                rc = CL_GET_ERROR_CODE(rc);
            }
        }
        rc = clBufferDelete(&msgHandle);
    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Deserializer function for transaction state
 */
static ClRcT _clTxnServiceCkptTxnStateUnpack(
        CL_IN   ClUint32T   dataSetId, 
        CL_IN   ClAddrT     pData, 
        CL_IN   ClUint32T   dataLen,
        CL_IN   ClPtrT      cookie)
{
    ClRcT                       rc  = CL_OK;
    ClBufferHandleT      msgHandle;

    CL_FUNC_ENTER();

    /*
       - If, from the restored txn-log, the txn is in intermediate state, 
         add it to pending-recovery-list
         else, discard
     */
    rc = clBufferCreate (&msgHandle);

    if (CL_OK == rc)
    {
        rc = clBufferNBytesWrite(msgHandle, (ClUint8T *)pData, dataLen);

        if (CL_OK == rc)
        {
            rc = clTxnRecoveryLogUnpack(msgHandle);
        }

    }
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to extract transaction logs from ckpt. rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}
