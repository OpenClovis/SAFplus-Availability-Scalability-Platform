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

#include <clCommon.h>
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clLogApi.h>
#include <clCorClientDebug.h>
#include <clCorRMDWrap.h> 
#include <clXdrApi.h>
#include <clCorErrors.h>
#include <xdrClCorMOIdT.h>
#include <xdrClCorAttrTypeT.h>
#include <xdrClCorTypeT.h>
#include <clBitApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorTxnClientIpi.h>

static ClRcT 
clCorDbgCliClCorObjectCreateAndSet ( ClUint32T argc, 
                                  ClCharT *argv[], 
                                  ClCharT **retStr );

static ClRcT 
clCorDbgCliClCorObjectAttributeSet ( ClUint32T argc, 
                                  ClCharT *argv[], 
                                  ClCharT **retStr);

static ClRcT 
clCorDbgCliClCorObjectAttrTypeGet ( ClUint32T argc, 
                                 ClCharT *argv[], 
                                 ClCharT **retStr );

static ClRcT 
clCorClntDbgCtNStExecute ( ClNameT txnIdName, ClCorCtAndSetInfoPtrT pAttrList, ClCharT* moToCreate );

static ClRcT 
clCorClntDbgSetExecute ( ClNameT txnName, ClCorCtAndSetInfoPtrT pAttrList );

static ClRcT 
_clCorClientCliCommonFunc ( ClCorClientCliOpT cliOp, 
                            ClUint32T argc, 
                            ClCharT *argv[], 
                            ClCorCtAndSetInfoPtrT pAttrList );

static ClRcT
_clCorTxnIdGet(ClNameT txnIdName, ClCorTxnSessionIdT* pTxnId);

static ClRcT
clCorDbgCliClCorTxnSessionCommit ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr );

static ClRcT
clCorDbgCliClCorTxnSessionCancel ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr );

static ClRcT
clCorDbgCliClCorTxnSessionFinalize ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr );

static ClRcT
clCorDbgCliClCorTxnFirstFailedJobGet( ClUint32T argc, ClCharT* argv[], ClCharT** retStr );

static ClRcT
clCorDbgCliClCorTxnNextFailedJobGet( ClUint32T argc, ClCharT* argv[], ClCharT** retStr );

static void clCorCliTxnIdMapDelFn(ClCntKeyHandleT key, ClCntDataHandleT data);

static ClInt32T clCorCliTxnIdMapCompFn(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

static ClRcT _clCorTxnIdMapCntNodeDelete(ClNameT txnIdName);

static ClRcT 
clCorDbgCliObjectDelete ( ClUint32T argc, ClCharT* argv[], ClCharT **retStr );

static ClInt32T _clCorTxnInfoNodeKeyCompare(
    CL_IN       ClCntKeyHandleT         key1,
    CL_IN       ClCntKeyHandleT         key2);

static ClRcT _clCorPrintFailedJobInfo(ClCorTxnInfoT* corTxnInfo, ClCharT* pCorStr);

/* Container to store the txn Id name to txn Id value mapping */
ClCntHandleT gCorCliTxnIdMap;

/* Linked-list to store the txn-id to txn-info node mapping */
static ClCntHandleT gCorTxnInfoNodeIdx;

/* Global object handle to store the objects created in complex transaction */
ClCorObjectHandleT gObjHandle;

static ClDebugFuncEntryT _clCorClientDbgCliFuncList[] = {
    {clCorDbgCliClCorObjectCreateAndSet, "corObjCreateAndSet", "Creates the object and sets the attribute(s)"},
    {clCorDbgCliClCorObjectAttributeSet, "corObjAttrSet", "Sets the value of the attribute(s)"},
    {clCorDbgCliClCorObjectAttrTypeGet, "corObjAttrTypeGet", "Gets the type of the attribute"},
    {clCorDbgCliObjectDelete, "corObjDelete", "Delete the object(s)"},
    {clCorDbgCliClCorTxnSessionCommit, "corTxnSessionCommit", "Commits the transaction"},
    {clCorDbgCliClCorTxnSessionCancel, "corTxnSessionCancel", "Cancels the transaction"},
    {clCorDbgCliClCorTxnSessionFinalize, "corTxnSessionFinalize", "Finalizes the transaction and removes the failed job information"},
    {clCorDbgCliClCorTxnFirstFailedJobGet, "corTxnFirstFailedJobGet", "Displays the first failed job information"},
    {clCorDbgCliClCorTxnNextFailedJobGet, "corTxnNextFailedJobGet", "Displays the next failed job information"}
};

/**
 * Utility function to put the information in the string set to debug client. 
 */ 
void clCorClientCliDbgPrint ( ClCharT* str, ClCharT** retStr )
{
                                                                                                                             
    *retStr = clHeapAllocate ( strlen ( str ) + 1 );
    if ( NULL == *retStr )
    {
        clLogError("COR", "DBG", "Failed while allocating memory. ");
        return;
    }
    sprintf(*retStr, str);
    return;
}


/**
 * Function to register the CLI commands that can be used for testing.
 */
ClRcT 
clCorClientDebugCliRegister ( CL_OUT ClHandleT *pDbgHandle )
{
    ClRcT rc = CL_OK;
    ClNameT cliName = {0};
    ClCpmHandleT cpmHandle = {0};
    ClCharT debugPrompt[15] = {0};
    

    clDbgIfNullReturn ( pDbgHandle, CL_CID_COR );

    rc = clCpmComponentNameGet ( cpmHandle, &cliName );
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DRG", "Failed to get the component Name. rc[0x%x]", rc );
        return rc;
    }

    clLogTrace ( "COR", "DRG", "Registering the COR CLI commands for the component [%s]", cliName.value );

    /* Assuming that the component name is not taking whole of 256 bytes of cliName.value */
    strncpy ( debugPrompt, cliName.value, 6 );
    strcat ( debugPrompt, "_COR_CLI" );

    clLog ( CL_LOG_SEV_TRACE, "COR", "DRG", "The CLI command prompt is [%s]", debugPrompt );

    /* Registering the debug cli */
    rc  =   clDebugRegister ( _clCorClientDbgCliFuncList, 
                            sizeof ( _clCorClientDbgCliFuncList )/sizeof ( ClDebugFuncEntryT ),
                            pDbgHandle );
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DRG", "Failed to register the function pointers" );
        return rc; 
    }

    /* Setting the debug prompt.  */
    rc = clDebugPromptSet ( debugPrompt );
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DRG", "Failed while setting the prompt. rc[0x%x]", rc );
        return rc;
    }

    clLogInfo ( "COR", "DRG", "Successfully completed the CLI registration [%s]", debugPrompt );

    return rc;
}


/** 
 * Function to deregister the CLI commands registered earlier.
 */ 
ClRcT clCorClientDebugCliDeregister ( CL_IN ClHandleT dbgHandle )
{
    ClRcT   rc = CL_OK;
    
    clLog ( CL_LOG_SEV_TRACE, "COR", "DDR", "Deregistering the CLI commands " );

    rc = clDebugDeregister ( dbgHandle );
    if(CL_OK != rc)
    {
        clLogError ( "COR", "DDR", " Failed while deregistering the CLI. rc[0x%x]", rc );
        return rc;
    }
    
    clLogInfo ("COR", "DDR", "Successfully deregistered the CLI commands" );

    return rc;
}
/**
 * Function to get the next failed job information for a given txn-id.
 */
ClRcT
clCorDbgCliClCorTxnNextFailedJobGet( ClUint32T argc, ClCharT* argv[], ClCharT** retStr )
{
    ClCorTxnSessionIdT txnId = 0;
    ClNameT txnIdName;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];
    ClCorTxnInfoT corTxnInfo = {0};
    ClCorTxnInfoT* pCorTxnInfo = NULL;

    if (argc != 2)
    {
        clCorClientCliDbgPrint ( "Usage : corTxnNextFailedJobGet <txnIdentifier> \n \
                txnIdentifier : Identifier of the transaction.\n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(argv[1]);

    rc = _clCorTxnIdGet(txnIdName, &txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to get the Txn-Id value. rc [0x%x]", rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    rc = clCntDataForKeyGet(gCorTxnInfoNodeIdx, (ClCntKeyHandleT) txnId, (ClCntDataHandleT *)&pCorTxnInfo);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", "Failed to find the node in Txn Info Node Idx linked list. rc [0x%x]", rc);
        sprintf(corStr, " Failed to find the node in Txn Info Node Idx linked list. rc [0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    rc = clCorTxnFailedJobGet(txnId, pCorTxnInfo, &corTxnInfo);
    if (rc != CL_OK)
    {
        if (rc != CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_NOT_EXIST))
        {
            clLogError("COR", "DBG", "Unable to get the failed job information from the container. rc [0x%x]", rc);
            sprintf(corStr, "Unable to get the failed job information from the container. rc [0x%x]", rc);
            clCorClientCliDbgPrint(corStr, retStr);
            return (rc);
        }
        else
        {
            sprintf(corStr, "No more failed job information found.");
            clCorClientCliDbgPrint(corStr, retStr);
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_GET));
        }
    }

    /* Display the result */
    rc = _clCorPrintFailedJobInfo(&corTxnInfo, (ClCharT *) corStr);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", "Unable to get the failed job information from the container. rc [0x%x]", rc);
        sprintf(corStr, "Unable to get the failed job information from the container. rc [0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    clCorClientCliDbgPrint(corStr, retStr);

    /* Store the node information back */
    memcpy(pCorTxnInfo, &corTxnInfo, sizeof(ClCorTxnInfoT));

    return (CL_OK);
}

/**
 * Function to get the first failed job information for a given txn-id.
 */
ClRcT
clCorDbgCliClCorTxnFirstFailedJobGet( ClUint32T argc, ClCharT* argv[], ClCharT** retStr )
{
    ClCorTxnSessionIdT txnId = 0;
    ClNameT txnIdName;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];
    ClCorTxnInfoT corTxnInfo = {0};
    ClCorTxnInfoT* pCorTxnInfo = NULL;

    if (argc != 2)
    {
        clCorClientCliDbgPrint ( "Usage : corTxnFirstFailedJobGet <txnIdentifier> \n \
                txnIdentifier : Identifier of the transaction.\n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(argv[1]);

    rc = _clCorTxnIdGet(txnIdName, &txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to get the Txn-Id value. rc [0x%x]", rc);
        sprintf(corStr, "Failed to get the Txn-Id value. rc [0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    /* Get the failed job information from failed job info store container and store the pointer 
     * to the node into a linked list along with the txn-id.
     */ 
    rc = clCorTxnFailedJobGet(txnId, NULL, &corTxnInfo);
    if (rc != CL_OK)
    {
        if (rc == CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_NOT_EXIST))
        {
            clLogError("DBG", "TXN", 
                    "No failed job information found. rc [0x%x]", rc);
            sprintf(corStr, 
                    "No failed job information found. rc [0x%x]", rc);
        }
        else
        {
            clLogError("DBG", "TXN", 
                "Unable to get the failed job information. rc [0x%x]", rc);
            sprintf(corStr,
                "Unable to get the failed job information. rc [0x%x]", rc);
        }

        clCorClientCliDbgPrint(corStr, retStr);
        return rc;
    }

    rc = _clCorPrintFailedJobInfo(&corTxnInfo, (ClCharT *) corStr);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG",
                "Unable to print the failed job information. rc [0x%x]", rc);
        sprintf(corStr,
                "Unable to print the failed job information. rc [0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return rc;
    }

    /* Get the node for this txn-id, add the node if it doesn't exist already */
    rc = clCntDataForKeyGet(gCorTxnInfoNodeIdx, (ClCntKeyHandleT) txnId, (ClCntDataHandleT *) &pCorTxnInfo);
    if (pCorTxnInfo == NULL)
    {
        /* Store the pointer into the linked list */
        pCorTxnInfo = (ClCorTxnInfoT *) clHeapAllocate(sizeof(ClCorTxnInfoT));
        if (pCorTxnInfo == NULL)
        {
            clLogError("COR", "DBG", "Failed to allocate memory");
            sprintf(corStr, "Failed to allocate memory. rc [0x%x]", CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            clCorClientCliDbgPrint(corStr, retStr);
            return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        }

        memset(pCorTxnInfo, 0, sizeof(ClCorTxnInfoT));

        rc = clCntNodeAdd(gCorTxnInfoNodeIdx, (ClCntKeyHandleT) txnId, (ClCntDataHandleT) pCorTxnInfo, NULL);
        if (rc != CL_OK)
        {
            clLogError("COR", "DBG", "Failed to add a node into Txn Info Node linked-list. rc [0x%x]", rc);
            sprintf(corStr, "Failed to add a node into Txn Info Node linked-list. rc [0x%x]", rc);
            clHeapFree(pCorTxnInfo);
            clCorClientCliDbgPrint(corStr, retStr);
            return (rc);
        }
    }

    clCorClientCliDbgPrint(corStr, retStr);

    memcpy(pCorTxnInfo, &corTxnInfo, sizeof(ClCorTxnInfoT));

    return (CL_OK);
}

ClRcT _clCorPrintFailedJobInfo(ClCorTxnInfoT* corTxnInfo, ClCharT* pCorStr)
{
    ClRcT rc = CL_OK;
    ClNameT moIdName;

    /* Display the failed job information */
    rc = clCorMoIdToMoIdNameGet(&(corTxnInfo->moId), &moIdName);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", 
                "Failed to get the MoId Name from the MoId. rc [0x%x]", rc);
        sprintf(pCorStr,
                "Failed to get the MoId Name from the MoId. rc [0x%x]", rc);
        return rc;
    }

    sprintf(pCorStr,
            "\nMoId       : %s\nSvc ID     : %d\nOp         : %d\nAttr ID    : 0x%x\nJob Status : 0x%x\n", 
             moIdName.value, corTxnInfo->moId.svcId, corTxnInfo->opType, corTxnInfo->attrId, corTxnInfo->jobStatus);

    return (CL_OK);   
}

void _clCorTxnInfoNodeDelete(
        CL_IN ClCntKeyHandleT key,
        CL_IN ClCntDataHandleT data)
{
    ClCorTxnInfoT* pCorTxnInfo = NULL;

    /* Need not free the key */
    pCorTxnInfo = (ClCorTxnInfoT *) data;

    clHeapFree(pCorTxnInfo);
}

ClInt32T _clCorTxnInfoNodeKeyCompare(
    CL_IN       ClCntKeyHandleT     key1,
    CL_IN       ClCntKeyHandleT     key2)
{
    ClCorTxnSessionIdT txnId1;
    ClCorTxnSessionIdT txnId2;

    txnId1 = (ClCorTxnSessionIdT) key1;
    txnId2 = (ClCorTxnSessionIdT) key2;

    if (txnId1 == txnId2)
    {
        return 0;
    }

    return 1;
}

ClRcT clCorCliTxnFailedJobLlistCreate()
{
    ClRcT rc = CL_OK;
    
    CL_FUNC_ENTER();

    rc = clCntLlistCreate (_clCorTxnInfoNodeKeyCompare,
               _clCorTxnInfoNodeDelete,
               _clCorTxnInfoNodeDelete, CL_CNT_UNIQUE_KEY, 
               (ClCntHandleT *) &(gCorTxnInfoNodeIdx));
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", 
                "Failed to create the linked list to store the Node idx of the failed job container. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}    

ClRcT clCorCliTxnFailedJobLlistFinalize()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    rc = clCntAllNodesDelete(gCorTxnInfoNodeIdx);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", "Failed to delete all the nodes in the Txn Info Node list. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntDelete(gCorTxnInfoNodeIdx);
    if (rc != CL_OK)
    {
        clLogError("COR", "DBG", "Failed to finalize Txn Info Node list. rc [0x%x]", rc);
        return rc;
    }

    return (CL_OK);
}

/**
 * Function to finalize a complex transaction
 */
ClRcT
clCorDbgCliClCorTxnSessionFinalize ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr )
{
    ClCorTxnSessionIdT txnId = 0;
    ClNameT txnIdName;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];

    if (argc < 2)
    {
        clCorClientCliDbgPrint ( "Usage : corTxnSessionFinalize <txnIdentifier> \n \
                txnIdentifier : Identifier of the transaction.\n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(argv[1]);

    rc = _clCorTxnIdGet(txnIdName, &txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to get the Txn-Id value. rc [0x%x]", rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    rc = clCorTxnSessionFinalize(txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to cancel the transaction. rc [0x%x]", rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    rc = _clCorTxnIdMapCntNodeDelete(txnIdName);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to delete the container node for key [%s]. rc [0x%x]", txnIdName.value, rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    clLogInfo ("DBG", "TXN", "Successfully finalized the transaction [%s]", txnIdName.value);

    return (CL_OK);  
}

/**
 * Function to cancel a complex transaction
 */
ClRcT
clCorDbgCliClCorTxnSessionCancel ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr )
{
    ClRcT rc = CL_OK;

    rc = clCorDbgCliClCorTxnSessionFinalize(argc, argv, retStr);

    return (rc);  
}

                                  // coverity[pass_by_value]
ClRcT _clCorTxnIdMapCntNodeDelete(ClNameT txnIdName)
{
    ClCntNodeHandleT nodeHandle = 0;
    ClRcT rc = CL_OK;
    ClCorTxnSessionIdT txnId = 0;

    rc = _clCorTxnIdGet(txnIdName, &txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to get the txnId value from txnId name [%s]. rc [0x%x]", txnIdName.value, rc);
        return rc;
    }

    /* Delete the node frm the Transaction Id Map container. */
    rc = clCntNodeFind(gCorCliTxnIdMap, (ClCntKeyHandleT) &txnIdName, (ClCntNodeHandleT *) &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to find the container node for the key [%s]", txnIdName.value);
        return rc;
    }

    rc = clCntNodeDelete(gCorCliTxnIdMap, nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to delete the container node for the key [%s]", txnIdName.value);
        return rc;
    }
    clLogInfo("DBG", "TXN", "Txn-Id map for the identifier [%s] is deleted successfully", txnIdName.value);

    /* Delete the node from the Txn Id to node mapping container. */
    rc = clCntNodeFind(gCorTxnInfoNodeIdx, (ClCntKeyHandleT) txnId, (ClCntNodeHandleT *) &nodeHandle);
    if (rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            clLogError("DBG", 
                "TXN", "Failed to find the node from the failed job node info container for key [%s]. rc [0x%x]", 
                txnIdName.value, rc);
            return rc;
        }

        rc = CL_OK;
    }
    else
    {
        rc = clCntNodeDelete(gCorTxnInfoNodeIdx, nodeHandle);
        if (rc != CL_OK)
        {
            clLogError("DBG", "TXN", 
                    "Failed to delete the container node for the key [%s] from node info container. rc [0x%x]", 
                    txnIdName.value, rc);
            return rc;
        }
    }

    return CL_OK;
}

/*
 * Function to commit the complex transaction
 */
ClRcT
clCorDbgCliClCorTxnSessionCommit ( ClUint32T argc, ClCharT* argv[], ClCharT** retStr )
{
    ClCorTxnSessionIdT txnId = 0;
    ClNameT txnIdName;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];

    if (argc < 2)
    {
        clCorClientCliDbgPrint ( "Usage : corTxnSessionCommit <txnIdentifier> \n \
                                          txnIdentifier : Identifier of the transaction.\n", retStr);
        return (CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE));
    }

    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(argv[1]);

    clLogInfo("DBG", "TXN", "Committing the transaction [%s]", txnIdName.value);

    rc = _clCorTxnIdGet(txnIdName, &txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to get the Txn-Id value. rc [0x%x]", rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    rc = clCorTxnSessionCommit(txnId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "TXN", "Failed to commit the transaction [%s]. rc [0x%x]", txnIdName.value, rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    clLogInfo ("DBG", "TXN", "Committed transaction [%s] successfully", txnIdName.value);

    rc = _clCorTxnIdMapCntNodeDelete(txnIdName);
    if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
    {
        clLogError("DBG", "TXN", "Failed to delete the container node for key [%s]. rc [0x%x]", txnIdName.value, rc);
        sprintf(corStr, "Execution Result : Failed [0x%x]\n", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        return (rc);
    }

    corStr[0] = '\0';
    sprintf(corStr, "Execution Result : passed for txnIdName [%s]\n", txnIdName.value);
    clCorClientCliDbgPrint(corStr, retStr);

    return (CL_OK);
}

ClRcT
               // coverity[pass_by_value]
_clCorTxnIdGet(ClNameT txnIdName, 
               ClCorTxnSessionIdT* pTxnId)
{
    ClRcT rc = CL_OK;
    ClCorTxnSessionIdT* pTmpTxnId = NULL;

    rc = clCntDataForKeyGet(gCorCliTxnIdMap, (ClCntKeyHandleT) &txnIdName, (ClCntDataHandleT *) &pTmpTxnId);
    if (rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            clLogError("DBG", "TXN", 
                "Txn-Id Name to Txn-Id value mapping doesn't exist. Invalid Txn-Identifier specified. rc [0x%x]", rc);
            return rc;
        }

        clLogError("DBG", "TXN",
            "Failed to get the Txn-Id value. rc [0x%x]", rc);
        return rc;
    }

    *pTxnId = *pTmpTxnId;

    return CL_OK;
}

/**
 *  Function to create and set the attributes of the COR object in a single call.
 */
ClRcT 
clCorDbgCliClCorObjectCreateAndSet ( ClUint32T argc, ClCharT *argv[], ClCharT **retStr )
{
    ClRcT               rc = CL_OK;
    ClCharT             corStr[CL_COR_MAX_NAME_SZ] = {0};
    ClCorClientCliOpT   cliOp = CL_COR_CLIENT_CLI_CREATE_AND_SET_OP;
    ClCorCtAndSetInfoT  attrList = {{{{0}}}};
    ClNameT             txnIdName;

    if ( argc < 4 )
    {
        clCorClientCliDbgPrint ( "Usage : corObjCreateAndSet <txnIdentifier> <moId> <serviceId> <attrId> <value> <attrId> <value> ... [MO-CREATE (true/TRUE)]\n \
                txnIdentifier : Identifier for the transaction. \n \
                                0          - Simple transaction, \n \
                                Any string - Complex transaction \n \
                moId : \\0x10001:0\\0x10010:3 or \\Chassis:0\\GigeBlade:3 \n \
                service Id: Service Id of the MSO. It should be -1 for MO \n \
                attrId : Attribute Identifier for the attribute. \n \
                value : Its value. \n \
                MO-CREATE : MO has to be created for the Mso. [true/TRUE]\n", retStr );

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }  

    rc = _clCorClientCliCommonFunc ( cliOp, argc, argv, &attrList);
    if ( CL_OK != rc )
    {
        corStr[0] = '\0';
        sprintf ( corStr, "Execution Result : Failed [0x%x]", rc );
        clCorClientCliDbgPrint ( corStr, retStr );    
        return rc;
    }

    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(txnIdName.value);

    rc = clCorClntDbgCtNStExecute (txnIdName, &attrList, argv[argc-1]);
    if ( CL_OK != rc )
    {
        corStr[0]='\0';
        sprintf ( corStr, "Execution Result : Failed [0x%x]", rc );
        clCorClientCliDbgPrint ( corStr, retStr );
        clCorClntDbgAttrListFinalize ( &attrList );
        return rc;
    }
    
    corStr[0] = '\0';
    sprintf ( corStr, "Execution Result : Passed" );
    clCorClientCliDbgPrint ( corStr, retStr );

    clCorClntDbgAttrListFinalize ( &attrList );

    return rc;
}


/**
 * Function to do the attribute set on the COR object attributes.
 */
ClRcT 
clCorDbgCliClCorObjectAttributeSet ( ClUint32T argc, ClCharT *argv[], ClCharT **retStr )
{
    ClRcT               rc = CL_OK;
    ClCharT             corStr[CL_COR_MAX_NAME_SZ] = {0};
    ClCorClientCliOpT   cliOp = CL_COR_CLIENT_CLI_ATTR_SET_OP;
    ClCorCtAndSetInfoT  attrList = {{{{0}}}};
    ClNameT             txnIdName = {0};

    if( ( argc < 6 ) || ( argc % 2 != 0 ) )
    {
        clCorClientCliDbgPrint ( "Usage : corObjAttrSet <txnIdentifier> <moId> <serviceId> <attrId> <value> <attrId> <value> ...\n \
                txnIdentifier : Identifier for the transaction. \n \
                                0          - Simple transaction, \n \
                                Any string - Complex transaction \n \
                moId : \\0x10001:0\\0x10010:3 or \\Chassis:0\\GigeBlade:3 \n \
                service Id: Service Id of the MSO. It should be -1 for MO\n \
                attrId : Attribute Identifier for the attribute. \n \
                value : Its value. \n", retStr );
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }  

    rc = _clCorClientCliCommonFunc ( cliOp, argc, argv, &attrList );
    if ( CL_OK != rc )
    {
        corStr[0] = '\0';
        sprintf ( corStr, "[corObjAttrSet] : Failed [0x%x]", rc );
        clCorClientCliDbgPrint ( corStr, retStr );    
        return rc;
    }
    
    strcpy(txnIdName.value, argv[1]);
    txnIdName.length = strlen(txnIdName.value);

    rc = clCorClntDbgSetExecute ( txnIdName, &attrList );
    if ( CL_OK != rc )
    {
        corStr[0] = '\0';
        sprintf ( corStr, "[corObjAttrSet] : Failed [0x%x]", rc );
        clCorClientCliDbgPrint ( corStr, retStr );
        clCorClntDbgAttrListFinalize ( &attrList );
        return rc;
    }

    corStr[0] = '\0';
    sprintf ( corStr, "[corObjAttrSet] : Passed ");
    clCorClientCliDbgPrint ( corStr, retStr );

    clCorClntDbgAttrListFinalize ( &attrList );

    return rc;
}


/**
 * Function to get the type of the attribute given the moId, service Id and attribute Id.
 */
ClRcT
clCorDbgCliClCorObjectAttrTypeGet ( ClUint32T argc, ClCharT *argv[], ClCharT **retStr )
{
    ClRcT   rc = CL_OK;

    return rc;    
}



/**
 * Function to marshall the cor client debug cli information.
 */ 
ClRcT _clCorClientDebugDataMarshall ( ClCorClientCliOpT cliOp, 
                                      ClUint32T argc, 
                                      ClCharT *argv[], 
                                      ClBufferHandleT inMsgH )
{
    ClRcT   rc = CL_OK;
    ClUint32T   multiplicity = 0;
    ClInt32T    index = 0;

    rc = clXdrMarshallClInt32T ( &cliOp, inMsgH, 0 );
    if ( CL_OK != rc )
    {
        clLogDebug ( "COR", "DBG", "Failed while marshalling the op type. rc[0x%x]", rc );
        return rc;
    }

    rc = clXdrMarshallClUint32T ( &argc, inMsgH, 0 );
    if( CL_OK != rc )
    {
        clLogDebug ( "COR", "DBG", "Failed while marshalling the argument count [%d]. rc[0x%x]", argc, rc );
        return rc;
    }

    for ( index = 0 ; index < argc ; index ++ ) 
    {
        if ( ( argv + index ) != NULL )
        {
            multiplicity = strlen ( argv[index] ) + 1;
            rc = clXdrMarshallClUint32T ( &multiplicity, inMsgH, 0 );
            if ( CL_OK != rc )
            {
                clLogDebug ( "COR", "DBG",  "Failed while adding the \
                        multiplicity[%d] of index [%d] ", multiplicity, index );
                return rc; 
            }

            rc = clXdrMarshallPtrClCharT ( argv[index], multiplicity , inMsgH, 0);
            if ( CL_OK != rc ) 
            {
                clLogDebug ( "COR", "DBG", "Failed while marshalling the cli \
                        argument data [%s] for index [%d], ", argv[index], index ); 
                return rc;
            }
        }
    }

    return CL_OK;
}




/**
 * Function to unmarshall the cor client debug cli infromation.
 */ 

ClRcT _clCorClientDebugDataUnMarshall ( ClBufferHandleT inMsgH, 
                                        ClCorClientCliOpT *pCliOp, 
                                        ClUint32T *pArgc, 
                                        ClCharT ***pArgv )
{
    ClRcT   rc  = CL_OK;
    ClUint32T   multiplicity = 0;
    ClUint32T  index = 0;
//    ClCharT *tempPtr = NULL;

    /* NULL check */
    clDbgIfNullReturn ( pCliOp, CL_CID_COR );
    clDbgIfNullReturn ( pArgc, CL_CID_COR );
    clDbgIfNullReturn ( pArgv, CL_CID_COR );

    rc = clXdrUnmarshallClInt32T ( inMsgH, pCliOp );
    if(CL_OK != rc)
    {
        clLogDebug ( "COR", "DBG", "Failed while unmarshalling the op type. rc[0x%x]", rc );
        return rc;
    }

    rc = clXdrUnmarshallClUint32T ( inMsgH, pArgc );
    if(CL_OK != rc)
    {
        clLogDebug ( "COR", "DBG", "Failed while marshalling the argument count. rc[0x%x]", rc);
        return rc;
    }

    if ( *pArgc <= 0 )
    {
        clLogError ( "COR", "DBG", "Invalid value of argument cournt [%d] recieved", *pArgc );
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
    
    *pArgv = ( ClCharT ** ) clHeapAllocate ( *pArgc * sizeof(ClCharT *));
    if( NULL == *pArgv )
    {
        clLogError ( "COR", "DBG", "Failed while allocating the argument array" );
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    for ( index = 0 ; index < *pArgc ; index ++ )
    {
        //if ( ( *pArgv + index ) != NULL )
        {
            multiplicity = 0;
            rc = clXdrUnmarshallClUint32T ( inMsgH, &multiplicity );
            if( CL_OK != rc )
            {
                clLogDebug ( "COR", "DBG",  "Failed while adding the \
                        data length[%d] of index [%d] ", multiplicity, index );
                return rc; 
            }

#if 0
            ( *pArgv + index ) = ( ClCharT * ) clHeapAllocate ( multiplicity );
            if ( NULL == (*pArgv + index ) )
            {
                clLogError ( "COR", "DBG", "Failed to allocate memory for argument \
                        data for index [%d] and mulitiplicity [%d]", index, multiplicity );
                return rc;
            }
#endif
            //tempPtr = (ClCharT *)(pArgv[index]);

            rc = clXdrUnmarshallPtrClCharT ( inMsgH, (void **)((*pArgv + index)), multiplicity );
            if(CL_OK != rc)
            {
                clLogDebug ( "COR", "DBG", "Failed while unmarshalling the cli \
                        argument data for index [%d], ", index ); 
            }
            //*pArgv + index = tempPtr;
        }
        /*x/else
        {
            clLogError ( "COR", "DBG", "The argument list is NULL ");
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }*/
    }

    return CL_OK;
}



/**
 * Function to marshall the create and set attrlist information.
 */ 
ClRcT
clCorClntDbgCtNStMarshall ( ClCorCtAndSetInfoPtrT pAttrList, ClBufferHandleT msgH )
{
    ClRcT   rc  =   CL_OK;
    ClUint32T   index = 0;
    
    rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)( &pAttrList->moId, msgH, 0 );
    if ( CL_OK != rc )
    {
        clLogError ( "DBG","CST", "Failed to marshall the object handle. rc[0x%x]", rc );
        return rc;
    }
 
    rc = clXdrMarshallClInt32T ( &pAttrList->num, msgH, 0 ) ;
    if ( CL_OK != rc)
    {
        clLogError("DBG", "CST", "Failed to unmarsahll the num of attribute list. rc[0x%x]", rc);
        return rc;
    }

    for ( index = 0 ; index < pAttrList->num ; index++)
    {
       /*
        rc = VDECL_VER(clXdrMarshallClCorAttrPathT, 4, 0, 0)(pAttrList->pAttrInfo[index].attrPath, msgH, 0);
        if(CL_OK != rc)
        {
            clLogError("DBG", "CST", "Failed to marshall the attribute path information. rc[0x%x]", rc);
            return rc;
        } 
        */ 
        rc = VDECL_VER(clXdrMarshallClCorAttrTypeT, 4, 0, 0)( &pAttrList->pAttrInfo[index].attrId, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the attrId. rc[0x%x]", rc );
            return rc;
        }

        rc = clXdrMarshallClInt32T ( &pAttrList->pAttrInfo[index].index, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the attribute index. rc[0x%x]", rc );
            return rc;
        }

        rc = VDECL_VER(clXdrMarshallClCorAttrTypeT, 4, 0, 0)( &pAttrList->pAttrInfo[index].attrType, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the attribute type. rc[0x%x]", rc );
            return rc;
        }

        rc = VDECL_VER(clXdrMarshallClCorTypeT, 4, 0, 0)( &pAttrList->pAttrInfo[index].corType, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the corType. rc[0x%x]", rc );
            return rc;  
        }

        rc = clXdrMarshallClUint32T ( &pAttrList->pAttrInfo[index].size, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the size. rc[0x%x]", rc );
            return rc;
        }

        rc = clXdrMarshallPtrClCharT  ( pAttrList->pAttrInfo[index].pValue, pAttrList->pAttrInfo[index].size, msgH, 0 );
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while marshalling the pointer. rc[0x%x]", rc );
            return rc;
        }
    }
    return rc;
}

/**
 * Function to unmarshall the create and set attrlist information.
 */ 
ClRcT
clCorClntDbgCtNStUnmarshall ( ClBufferHandleT msgH, ClCorCtAndSetInfoPtrT pAttrList )
{
    ClRcT   rc  =   CL_OK;
    ClUint32T   index = 0;

    clDbgIfNullReturn ( pAttrList, CL_CID_COR );

    rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)( msgH, &pAttrList->moId );
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DBG","Failed to unmarshall the object handle. rc[0x%x]", rc );
        return rc;
    }

    rc = clXdrUnmarshallClInt32T ( msgH, &pAttrList->num ) ;
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DBG", "Failed to unmarsahll the num of attribute list. rc[0x%x]", rc );
        return rc;
    }

    pAttrList->pAttrInfo = (_ClCorAttrInfoT *) clHeapAllocate ( sizeof ( _ClCorAttrInfoT ) * pAttrList->num );
    if ( NULL == pAttrList->pAttrInfo )
    {
        clLogError ( "COR", "DBG", "Failed to allocate the memory. " );
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);        
    }    

    for ( index = 0 ; index < pAttrList->num ; index++ )
    {
       /*
        rc = VDECL_VER(clXdrMarshallClCorAttrPathT, 4, 0, 0)(pAttrList->pAttrInfo[index].attrPath, msgH, 0);
        if(CL_OK != rc)
        {
            clLogError("DBG", "CST", "Failed to marshall the attribute path information. rc[0x%x]", rc);
            return rc;
        } 
        */ 
        rc = VDECL_VER(clXdrUnmarshallClCorAttrTypeT, 4, 0, 0)( msgH, &pAttrList->pAttrInfo[index].attrId );
        if ( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the attrId. rc[0x%x]", rc );
            return rc;
        }

        rc = clXdrUnmarshallClInt32T ( msgH, &pAttrList->pAttrInfo[index].index );
        if ( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the attribute index. rc[0x%x]", rc );
            return rc;
        }

        rc = VDECL_VER(clXdrUnmarshallClCorAttrTypeT, 4, 0, 0)( msgH, &pAttrList->pAttrInfo[index].attrType );
        if ( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the attribute type. rc[0x%x]", rc );
            return rc;
        }

        rc = VDECL_VER(clXdrUnmarshallClCorTypeT, 4, 0, 0)( msgH, &pAttrList->pAttrInfo[index].corType );
        if ( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the corType. rc[0x%x]", rc );
            return rc;  
        }

        rc = clXdrUnmarshallClUint32T ( msgH, &pAttrList->pAttrInfo[index].size );
        if ( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the size. rc[0x%x]", rc );
            return rc;
        }

        rc = clXdrUnmarshallPtrClCharT ( msgH, &pAttrList->pAttrInfo[index].pValue, pAttrList->pAttrInfo[index].size );
        if( CL_OK != rc )
        {
            clLogError ( "COR", "DBG", "Failed while unmarshalling the attribute value. rc[0x%x]", rc );
            return rc;
        }


        if ( CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet() )
        {
            rc = clCorObjAttrValSwap ( (ClUint8T *)pAttrList->pAttrInfo[index].pValue, 
                                      pAttrList->pAttrInfo[index].size, 
                                      pAttrList->pAttrInfo[index].attrType, 
                                      pAttrList->pAttrInfo[index].corType ); 
            if ( CL_OK != rc )
            {
                clLogError ( "COR", "DBG", "Failed while converting the value to little endian format. rc[0x%x]", rc );
                return rc;
            }
        }

    }
    return rc;
}


/**
 * Function to make the actual call for create and set and to create a complex transaction involving multiple jobs.
 */ 
ClRcT
                          // coverity[pass_by_value]
clCorClntDbgCtNStExecute (ClNameT txnIdName, 
                          ClCorCtAndSetInfoPtrT pAttrList, 
                          ClCharT* moToCreate)
{
    ClRcT   rc = CL_OK;
    ClUint32T index = 0;
    ClCorAttributeValuePtrT pAttrDesc =  NULL;
    ClCorAttributeValueListT attrList = {0};
    ClCorMOIdT tempMoId;

    clDbgIfNullReturn ( pAttrList, CL_CID_COR );
    clDbgIfNullReturn ( moToCreate, CL_CID_COR );

    pAttrDesc = ( ClCorAttributeValuePtrT ) clHeapAllocate ( sizeof ( ClCorAttributeValueT ) * pAttrList->num );    
    if ( pAttrDesc == NULL )
    {
        clLogError ( "DBG", "CST", "Failed while allocating the memory." );
        return rc;
    }

    for ( index = 0 ; index < pAttrList->num ; index ++ )
    {
        pAttrDesc[index].pAttrPath = NULL;
        pAttrDesc[index].attrId = pAttrList->pAttrInfo[index].attrId;
        pAttrDesc[index].index = pAttrList->pAttrInfo[index].index;
        pAttrDesc[index].bufferPtr = pAttrList->pAttrInfo[index].pValue;
        pAttrDesc[index].bufferSize = pAttrList->pAttrInfo[index].size;
    }
   
    attrList.numOfValues = pAttrList->num;
    attrList.pAttributeValue = pAttrDesc;

    /* Check whether MO has to be created */
    if((pAttrList->moId.svcId != CL_COR_INVALID_SVC_ID) && (strcasecmp(moToCreate, "true") == 0))
    {
        clCorMoIdInitialize(&tempMoId);
        tempMoId = pAttrList->moId;
        tempMoId.svcId = CL_COR_INVALID_SVC_ID;
        rc = clCorObjectHandleGet(&tempMoId, &gObjHandle);
        if(CL_OK != rc)
        {
            /* Free the object handle. */
            clCorObjectHandleFree(&gObjHandle);
             
            rc = clCorObjectCreate(NULL, &tempMoId, NULL);
            if(CL_OK != rc)
            {
                clLogError("DBG", "CST", "Failed while doing the object create and set. rc[0x%x] ", rc);
                clHeapFree(pAttrDesc);
                return rc;
            }
        }
    }

    if (strcmp(txnIdName.value, "0") == 0)
    {    
        /* Create the object in SIMPLE transaction */
        clLogInfo ( "DBG", "CST", "Creating MO in SIMPLE transaction.");
        rc = clCorObjectCreateAndSet ( NULL, &pAttrList->moId, &attrList, NULL);
        if ( CL_OK != rc )
        {
            clLogError ( "DBG", "CST", "Failed while doing the object create and set. rc[0x%x] ", rc );
            clHeapFree(pAttrDesc);
            return rc;
        }
    }
    else
    {
        ClCorTxnSessionIdT* pTxnId = NULL;
        ClNameT* pTxnIdName = NULL;
        ClCorTxnSessionIdT tid = 0;

        clLogInfo ( "DBG", "CST", "Creating MO in the complex transaction.");

        rc = clCntDataForKeyGet(gCorCliTxnIdMap, (ClCntKeyHandleT) &txnIdName, (ClCntDataHandleT *) &pTxnId);
        if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
        {
            clLogError ( "DBG", "CST", "Failed to get the txn name mapping for txn id");
            clHeapFree(pAttrDesc);
            return rc;
        }

        if (pTxnId == NULL)
        {
            tid = 0;

            rc = clCorObjectCreateAndSet( &tid, &(pAttrList->moId), &attrList, NULL);
            if (CL_OK != rc)
            {
                clLogError ( "DBG", "CST", "Failed while doing object create and set. rc [0x%x]", rc );
                clHeapFree(pAttrDesc);
                return rc;
            }
        
            /* Add the mapping into the table */
            pTxnIdName = clHeapAllocate(sizeof(ClNameT));
            if (pTxnIdName == NULL)
            {
                clLogError("DBG", "CST", "Failed to allocate memory");
                clHeapFree(pAttrDesc);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }

            memcpy(pTxnIdName, &txnIdName, sizeof(ClNameT));

            pTxnId = clHeapAllocate(sizeof(ClCorTxnSessionIdT));
            if (pTxnId == NULL)
            {
                clLogError("DBG", "CST", "Failed to allocate memory");
                clHeapFree(pAttrDesc);
                clHeapFree(pTxnIdName);
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }

            memcpy(pTxnId, &tid, sizeof(ClCorTxnSessionIdT));

            rc = clCntNodeAdd(gCorCliTxnIdMap, (ClCntKeyHandleT) pTxnIdName, (ClCntDataHandleT) pTxnId, NULL);
            if (rc != CL_OK)
            {
                clLogError ( "DBG", "CST", "Failed to add the mapping between TxnId name to value. rc [0x%x]", rc);
                clHeapFree(pAttrDesc);
                clHeapFree(pTxnIdName);
                clHeapFree(pTxnId);
                return rc;
            }

            clLogInfo ("DBG", "CST", "Created a new transaction [%s]", txnIdName.value);
        }
        else
        {
            /* Use the transaction-id already defined */
            tid = *pTxnId;

            clLogInfo ("DBG", "CST", "Adding job into the transaction [%s]", txnIdName.value);
            rc = clCorObjectCreateAndSet( &tid, &(pAttrList->moId), &attrList, NULL);
            if (CL_OK != rc)
            {
                clLogError ( "DBG", "CST", "Failed to add create-and-set job to the transaction. rc [0x%x]", rc );
                clHeapFree(pAttrDesc);
                return rc;
            }
        }
    }

    clHeapFree(pAttrDesc);
    return rc;
}

/* Container to store TxnId name to TxnId value mapping */

ClRcT clCorCliTxnIdMapCntCreate()
{
    ClRcT rc = CL_OK;

    rc = clCntLlistCreate(clCorCliTxnIdMapCompFn,
                            clCorCliTxnIdMapDelFn,
                            NULL,
                            CL_CNT_UNIQUE_KEY,
                            &gCorCliTxnIdMap);
    if (rc != CL_OK)
    {
        clLogError("DBG", "CST", "Failed to create the container to store TxnId name to TxnId mapping. rc [0x%x]", rc);
        return rc;
    }

    clLogInfo("DBG", "INI", "Container to store Txn-Id mapping is created successfully [%p]", (ClPtrT) gCorCliTxnIdMap);
    return CL_OK;
}

ClInt32T clCorCliTxnIdMapCompFn(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClNameT txnIdName1;
    ClNameT txnIdName2;

    txnIdName1 = * (ClNameT *) key1;
    txnIdName2 = * (ClNameT *) key2;

    if (strcmp(txnIdName1.value, txnIdName2.value) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }

    return 1;
}

void clCorCliTxnIdMapDelFn(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    ClNameT* pTxnIdName = NULL;
    ClCorTxnSessionIdT* pTxnId = NULL;

    pTxnIdName = (ClNameT *) key;
    pTxnId = (ClCorTxnSessionIdT *) data;

    clHeapFree(pTxnIdName);
    clHeapFree(pTxnId);
}

ClRcT clCorCliTxnIdMapCntFinalize()
{
    ClRcT rc = CL_OK;

    if (gCorCliTxnIdMap != 0)
    {
        rc = clCntDelete(gCorCliTxnIdMap);
        if (rc != CL_OK)
        {
            clLogError("DBG", "CST", "Failed to delete the container used for Txn Id mapping. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return rc;
        }

        gCorCliTxnIdMap = 0;
    }
    else
    {
        clLogError("DBG", "CST", "Txn Id mapping container handle is NULL");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Function to execute the set operation.
 */ 
ClRcT
                       // coverity[pass_by_value]
clCorClntDbgSetExecute(ClNameT txnName, 
                       ClCorCtAndSetInfoPtrT pAttrList)
{
    ClRcT               rc = CL_OK;
    ClCorTxnSessionIdT  tid = 0;
    ClUint32T           index = 0;
    ClCorTxnSessionIdT  *pTxnId = NULL;
    ClNameT             *pTxnName = NULL;

    if (strcmp(txnName.value, "0") != 0)
    {
        rc = clCntDataForKeyGet(gCorCliTxnIdMap, (ClCntKeyHandleT) &txnName, (ClCntDataHandleT *) &pTxnId);
        if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
        {
            clLogError ( "COR", "DEL", 
                    "Failed to get the txn id from a txn name[%s] supplied . rc[0x%x]",
                    txnName.value, rc);
            return rc;
        }

        if (NULL != pTxnId)
        {
            tid = *pTxnId;
        }
    }

    for ( index = 0 ; index < pAttrList->num ; index ++ )
    {
        ClCorObjectHandleT objH;

        rc = clCorMoIdToObjectHandleGet(&(pAttrList->moId), &objH);
        if (rc != CL_OK)
        {
            clLogError("COR", "DBG", "Failed while getting the object handle from MoId. rc [0x%x]", rc);
            return rc;
        }

        rc = clCorObjectAttributeSet(&tid, objH, NULL, pAttrList->pAttrInfo[index].attrId,
                pAttrList->pAttrInfo[index].index, pAttrList->pAttrInfo[index].pValue, pAttrList->pAttrInfo[index].size);
        if (CL_OK != rc)
        {
            clLogError("COR", "DBG", "Failed while adding the jobs in cor-txn list. rc[0x%x]", rc);
            if (strcmp(txnName.value, "0") == 0)
                clCorTxnSessionFinalize(tid);
            return rc;
        }

        clCorObjectHandleFree(&objH);
    }

    if (strcmp(txnName.value, "0") == 0)
    {
        rc = clCorTxnSessionCommit(tid);
        if (CL_OK != rc)
        {
            clLogError("COR", "DBG", "Failed while commiting the transaction. rc[0x%x]", rc);
            clCorTxnSessionFinalize(tid);
            return rc;
        }
    }
    else
    {
        if (NULL == pTxnId)
        {
            pTxnId = clHeapAllocate(sizeof(ClCorTxnSessionIdT));
            if (NULL == pTxnId)
            {
                clLogError("COR", "DBG", "Failed while allocating memory for txn sessoin Id. ");
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }

            pTxnName = clHeapAllocate(sizeof(ClNameT));
            if (NULL == pTxnName)
            {
                clLogError("COR", "DBG", "Failed while allocating the memory for the txn session name. ");
                clHeapFree(pTxnId);
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }
            
            memcpy (pTxnId, &tid, sizeof(ClCorTxnSessionIdT));

            memcpy (pTxnName, &txnName, sizeof(ClNameT));

            rc = clCntNodeAdd(gCorCliTxnIdMap, (ClCntKeyHandleT) pTxnName, (ClCntDataHandleT) pTxnId, NULL);
            if ((rc != CL_OK) && (rc != CL_ERR_DUPLICATE))
            {
                clHeapFree(pTxnName);
                clHeapFree(pTxnId);
                clLogError ( "COR", "DEL", "Failed to add the mapping between TxnId name to value. rc [0x%x]", rc);
                return rc;
            }
        }
    }

    return CL_OK;
}



/**
 * Function to finalize the attribute list . 
 */

void 
clCorClntDbgAttrListFinalize ( ClCorCtAndSetInfoPtrT pAttrList ) 
{
    ClInt32T index = 0;

    for ( index = 0 ; index < pAttrList->num; index ++ )
    {
        clHeapFree ( pAttrList->pAttrInfo[index].pValue );
    }
    clHeapFree ( pAttrList->pAttrInfo );
}

/**
 * Common functionality used by create n set and set operation.
 */ 
static ClRcT 
_clCorClientCliCommonFunc ( ClCorClientCliOpT cliOp, 
                            ClUint32T argc, 
                            ClCharT *argv[], 
                            ClCorCtAndSetInfoPtrT pAttrList )
{
    ClRcT               rc = CL_OK;
    ClBufferHandleT     inMsgHandle = 0, outMsgHandle = 0;

    clDbgIfNullReturn(pAttrList, CL_CID_COR);
       
    rc = clBufferCreate ( &inMsgHandle );
    if ( CL_OK != rc )
    {
        clLogError("COR", "DBG", "Failed while creating the IN buffer. rc[0x%x]", rc);
        return rc;
    }

    rc = clBufferCreate ( &outMsgHandle );
    if ( CL_OK != rc )
    {
        clLogError("COR", "DBG", "Failed while creating the OUT buffer. rc[0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    rc = _clCorClientDebugDataMarshall ( cliOp, (argc - 2) , &argv[2], inMsgHandle );
    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DBG", "Failed while marsahlling the debug Data. rc[0x%x]", rc );
        clBufferDelete(&inMsgHandle);
        clBufferDelete(&outMsgHandle);
        return rc;
    }

    COR_CALL_RMD_SYNC_WITH_MSG(COR_CLIENT_DBG_CLI_OP, inMsgHandle, outMsgHandle, rc);

    if ( CL_OK != rc )
    {
        clLogError ( "COR", "DBG", "Failed while sending the RMD call to the COR server. rc[0x%x]", rc );
        clBufferDelete(&inMsgHandle);
        clBufferDelete(&outMsgHandle);
        return rc;
    }

    rc = clCorClntDbgCtNStUnmarshall ( outMsgHandle, pAttrList);
    if ( CL_OK != rc )
    {
        clLogError("COR", "DBG", "Failed while unmarsahlling the information. rc[0x%x]", rc);
        clBufferDelete ( &inMsgHandle );
        clBufferDelete ( &outMsgHandle );
        return rc;
    }

    clBufferDelete ( &inMsgHandle );
    clBufferDelete ( &outMsgHandle );

    clLogTrace("COR", "DBG", "Obtained the value from COR successfully.  ");

    return CL_OK;
}


/**
 * CLI function to delete the objects.
 */ 

ClRcT 
clCorDbgCliObjectDelete ( ClUint32T argc, ClCharT *argv[], ClCharT **retStr )
{
    ClRcT   rc  =   CL_OK;
    ClCorMOIdT  moId;
    ClCorServiceIdT    serviceId = 0;
    ClNameT     *pTxnIdName = NULL;
    ClNameT     moIdName = {0}, txnIdName = {0};
    ClCorTxnSessionIdT txnId = 0;
    ClCorTxnSessionIdT* pTxnId = NULL;
    ClCharT             corStr[1024] = {0};
    ClCorObjectHandleT  objH = NULL;

    if ( ( argc < 4 ) || ( argc > 4 ) )
    {
        clCorClientCliDbgPrint ( "Usage: corObjDelete <txnIdentifier> <moId> <serviceId \n \
                        txnIdentifier : Identifier for the transaction. \n \
                                        0          - Simple transaction, \n \
                                        Any string - Complex transaction \n \
                        moId : \\0x10001:0\\0x10010:3 or \\Chassis:0\\GigeBlade:3 \n \
                        service Id: Service Id of the MSO. It should be -1 for MO ", retStr); 
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if(strlen(argv[2]) >= sizeof(moIdName.value))
    {
        corStr[0]='\0';
        sprintf(corStr, "The moId string [%s] passed has length [%zd] which is larger than [256]. rc[0x%x]",
                argv[2], strlen(argv[2]), CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
        clCorClientCliDbgPrint(corStr, retStr);
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    if(strlen(argv[1]) >= sizeof(txnIdName.value))
    {
        corStr[0]='\0';
        sprintf(corStr, "The txnIdentifier string [%s] passed has length [%zd] which is larger than [256]. rc[0x%x]",
                argv[1], strlen(argv[1]), CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
        clCorClientCliDbgPrint(corStr, retStr);
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    strncpy ( moIdName.value, argv [2] , strlen(argv[2]));
    moIdName.length = strlen(argv[2]);

    rc = clCorMoIdNameToMoIdGet(&moIdName, &moId);
    if(CL_OK != rc)
    {
        corStr[0]='\0';
        sprintf(corStr, "Failed while getting the MOId to MoId name. rc[0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    clLogTrace("COR", "DBG", "corObjDelete: Got the MOId from MOId name [%s] from COR", moIdName.value);
    
    serviceId = atoi(argv[3]);
    if(serviceId == -1)
    {
        moId.svcId = CL_COR_INVALID_SVC_ID;
    }
    else
        moId.svcId = serviceId;

    clLogTrace("COR", "DBG", "corObjDelete: Got the service Id [%d] ", serviceId);

    rc = clCorObjectHandleGet(&moId, &objH);
    if(CL_OK != rc)
    {
        corStr[0]='\0';
        sprintf(corStr, "Failed while getting the object handle. rc[0x%x]", rc);
        clCorClientCliDbgPrint(corStr, retStr);
        CL_FUNC_EXIT();
        return (rc);
    }    

    clLogTrace("COR", "DBG", "corObjDelete: Got the object Handle for the MO [%s], service Id[%d], txnIdName [%s]", 
            moIdName.value, serviceId, txnIdName.value);

    strncpy ( txnIdName.value, argv [1] , strlen(argv[1]));
    txnIdName.length = strlen(argv[1]);
    
    if (strcmp(txnIdName.value, "0") == 0)
    {    
        /* DeleSIMPLE transaction */
        clLogTrace( "COR", "DEL", "Deleting the Object [%s] in SIMPLE transaction.", argv[2]);

        rc = clCorObjectDelete ( NULL, objH );
        if ( CL_OK != rc )
        {
            corStr[0]='\0';
            sprintf(corStr, "Failed while deleting the object [%s] and serviceId [%d]. rc[0x%x] ", 
                    argv[2], serviceId, rc);
            clCorClientCliDbgPrint(corStr, retStr);
            CL_FUNC_EXIT();
            return rc;
        }
        
        corStr[0] = '\0';
        sprintf(corStr, "corObjDelete: Completed the object delete of [%s]:svcId [%d] successfully ", argv[2], serviceId);
        clCorClientCliDbgPrint(corStr, retStr);
    }
    else
    {
        clLogTrace ( "COR", "DEL", "Deleting the Object in a complex transaction...");

        rc = clCntDataForKeyGet(gCorCliTxnIdMap, (ClCntKeyHandleT) &txnIdName, (ClCntDataHandleT *) &pTxnId);
        if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
        {
            corStr[0]='\0';
            clLogError ( "COR", "DEL", "Failed to get the txn name mapping for txn id");
            sprintf(corStr, "Failed while getting the txn handle info. rc[0x%x]", rc);
            clCorClientCliDbgPrint(corStr, retStr);
            CL_FUNC_EXIT();
            return rc;
        }

        if (pTxnId == NULL)
        {
            corStr[0]='\0';
            rc = clCorObjectDelete( &txnId, objH );
            if (CL_OK != rc)
            {
                corStr[0]='\0';
                clLogError ( "COR", "DEL", "Failed while adding the job for deleting \
                        the object [%s] service Id [%d]. rc [0x%x]", argv[2], serviceId, rc );
                sprintf(corStr, "Failed while adding the job for deleting the object [%s] service id[%d]. rc[0x%x]", 
                        argv[2], serviceId, rc);
                clCorClientCliDbgPrint(corStr, retStr);
                CL_FUNC_EXIT();
                return (rc);
            }
        
            /* Add the mapping into the table */
            pTxnIdName = clHeapAllocate(sizeof(ClNameT));
            if (pTxnIdName == NULL)
            {
                corStr[0]='\0';
                clLogError("COR", "DEL", "Failed to allocate memory for TxnId Name [%s] for MO[%s]",
                        argv[1], argv[2]);
                sprintf(corStr, "Failed to allocate Memory for TxnId Name [%s]", argv[1]);
                clCorClientCliDbgPrint(corStr, retStr);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }

            memcpy(pTxnIdName, &txnIdName, sizeof(ClNameT));

            pTxnId = clHeapAllocate(sizeof(ClCorTxnSessionIdT));
            if (pTxnId == NULL)
            {
                corStr[0]='\0';
                clHeapFree(pTxnIdName);
                clLogError("COR", "DEL", "Failed to allocate memory for txnId for MO[%s]", argv[2]);
                sprintf(corStr, "Failed to allocate the memory for txnId for MO [%s]", argv[2]);
                clCorClientCliDbgPrint(corStr, retStr);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }

            /* Assigning the transaction Id*/
            *pTxnId = txnId;

            clLogTrace("COR", "DBG", "The txnId obtained is [%p] for txnIdName [%s]", (ClPtrT)txnId, argv[1]);

            rc = clCntNodeAdd(gCorCliTxnIdMap, (ClCntKeyHandleT) pTxnIdName, (ClCntDataHandleT) pTxnId, NULL);
            if ((rc != CL_OK) && (rc != CL_ERR_DUPLICATE))
            {
                corStr[0]='\0';
                clHeapFree(pTxnIdName);
                clHeapFree(pTxnId);
                clLogError ( "COR", "DEL", "Failed to add the mapping between TxnId name to value. rc [0x%x]", rc);
                sprintf(corStr, "Failed to add the mapping for the txnId name [%s] for MO[%s]. rc[0x%x]",
                        argv[1], argv[2], rc);
                clCorClientCliDbgPrint(corStr, retStr);
                CL_FUNC_EXIT();
                return rc;
            }

            clLogTrace ("COR", "DEL", "Created a new transaction [%s]", argv[1]);
        }
        else
        {
            /* Use the transaction-id already defined */
            txnId = *pTxnId;

            clLogTrace("COR", "DBG", "The txnId obtained is [%p] for txnIdName [%s]", (ClPtrT)txnId, argv[1]);

            rc = clCorObjectDelete( &txnId, objH );
            if (CL_OK != rc)
            {
                clLogError ( "COR", "DEL", "Failed to add Delete job for MO [%s] to the transaction. rc [0x%x]", 
                        argv[2], rc );
                corStr[0]='\0';
                sprintf(corStr, "Failed to add the delete job for MO[%s] to the transaction. rc[0x%x]", argv[2], rc);
                clCorClientCliDbgPrint(corStr, retStr);
                CL_FUNC_EXIT();
                return rc;
            }
        }
        
        corStr[0] = '\0';
        sprintf(corStr, "corObjDelete: Txn Job of txnId name [%s] : Object delete of [%s]:svcId [%d] ", argv[1], argv[2], serviceId);
        clCorClientCliDbgPrint(corStr, retStr);
    }

    clLogTrace("COR", "DEL", "Completed the corObjDelete Cli call");

    CL_FUNC_EXIT();
    return rc; 
}
