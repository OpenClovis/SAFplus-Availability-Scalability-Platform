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
 * ModuleName  : cor
 * File        : clCorTxnLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains routines maintain TXN  log.
 *****************************************************************************/

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>

/* Internal Headers */
#include <clCorTxnApi.h>
#include <clCorTxnInterface.h>

#include <clClistApi.h>
#include <clCorDefs.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif
#include "clCorLog.h"

/* globals */
static ClClistT gCorTxnLogList = 0;

/* TEMP */

/* Forward declaration */
/* TODO - IDL */
#if 0
static ClRcT _clCorTxnLogDelete(ClCorTxnJobStreamT *txnStreamH);
#endif


/**
 *  Intialize the transaction manager log library.
 *
 *  This API Intializes the transaction manager log library. This needs to 
 *    be called before any TXN functions can be used.
 *                                                                        
 *  @param maxLogs: Depth of the circular log in terms of number of 
 *    TXNs. A value of zero implies no-limit.
 *
 *  @returns CL_OK  - Success<br>
 */
/* TODO - IDL */
#if 0
ClRcT
clCorTxnLogInit(CL_IN ClUint32T maxLogs)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if ((rc = clClistCreate(maxLogs,
	                               CL_DROP_FIRST,
                                   (ClClistDeleteCallbackT) _clCorTxnLogDelete,
                                   (ClClistDeleteCallbackT) _clCorTxnLogDelete,
                                   &gCorTxnLogList)) != CL_OK)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
				CL_LOG_MESSAGE_2_INIT, "Transaction-log", rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clClistCreate failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    } 
    
    CL_FUNC_EXIT();
    return (rc);
}
#endif

/**
 *  Finalize the transaction manager log library.
 *
 *  This API finazlize the transaction manager log library.
 *                                                                        
 *  @param  N/A
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorTxnLogFini(void)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if ((rc = clClistDelete(&gCorTxnLogList)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clClistDelete failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    } 
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Add a TXN to the TXN log.
 *
 *  This API adds a TXN to the TXN log.
 *                                                                        
 *  @param txnStream: TXN stream, starting with the station list which
 *   needs to be added to the TXN log.
 *
 *   NOTE: The TXN stream is added to the log as is... No copy is made.
 *    The caller looses the txnStream and should not try to free it.
 *    It is assumed that the txnStream is allocated from the heap using
 *    COR_MALLOC. TXN Log manager frees it when the log limit is reached.
 *
 *  @returns CL_OK  - Success<br>
 */

ClRcT
clCorTxnLogAdd(ClCorTxnJobStreamT  *txnStreamH)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    

    if (txnStreamH == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corTxnLogAdd NULL TXN stream"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if ((rc = clClistLastNodeAdd(gCorTxnLogList,
	                                    (ClClistDataT)txnStreamH)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clClistLastNodeAdd failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    } 
    
    CL_FUNC_EXIT();
    return (rc);
}


/**
 *  Show all the TXN present in the TXN log.
 *
 *  This API prints the contents of all the TXN present in the
 *   TXN log. 
 *   
 *  @param txnStream: TXN stream, starting with the station list which
 *   needs to be printed.
 *         verbose: Specifies how much detail to print, 0 means minimal
 *   1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
#ifdef COR_TEST
ClRcT
clCorTxnLogEntryShow(ClCorTxnJobStreamT  *txnStream, void* verbose)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    clOsalPrintf ("---\n");
    
    clCorTxnStreamTxnUnpack (txnStream);
    clCorTxnShow (&txnStream->txn, *(int*)verbose); 
    
    CL_FUNC_EXIT();
    return (rc);
}
#endif

/**
 *  Show a TXN entry.
 *
 *  This API prints the contents of a txn entry.
 *   
 *  @param verbose: Specifies how much detail to print, 0 means minimal
 *   1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
#ifdef COR_TEST
ClRcT
clCorTxnLogShow(ClUint32T verbose)
{
    ClRcT rc = CL_OK;
    ClUint32T size;

    CL_FUNC_ENTER();

    if ((rc = clClistSizeGet(gCorTxnLogList, 
                                    &size)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clClistSizeGet failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    clOsalPrintf ("\n\n-------------------------------------------------------------\r\n");
    clOsalPrintf (    "-                      COR TXN LOG                          -\r\n");
    clOsalPrintf (    "-                    Number of TXNs ........ %d             -\r\n",
	          size);
    clOsalPrintf (    "-------------------------------------------------------------\r\n");

    if ((rc = clClistWalk(gCorTxnLogList,
                                 (ClClistWalkCallbackT)clCorTxnLogEntryShow,
                                 (void *) &verbose)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clClistWalk failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

	CL_FUNC_EXIT();
    return (rc);
}
#endif



/**
 *  Show a TXN entry.
 *
 *  This API prints the contents of a txn entry.
 *   
 *  @param verbose: Specifies how much detail to print, 0 means minimal
 *   1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
_clCorTxnLogDelete(ClCorTxnJobStreamT  *txnStreamH)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    

    if (txnStreamH == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL TXN stream"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    clHeapFree (txnStreamH);
    CL_FUNC_EXIT();
    return (rc);
}
