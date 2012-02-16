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
 * File        : clCorTxnInterface.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements interface between COR and Transaction Management module
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_TXN_INTERFACE_H_
#define _CL_COR_TXN_INTERFACE_H_

#include <clCommon.h>
#include <clCorTxnApi.h>
#include <clCorTxnClientIpi.h>

#include <clTxnApi.h>
#include <clTxnAgentApi.h>
#include <clCorPvt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define     CL_COR_TXN_MAX_COMPONENTS       20

#define     CL_COR_TXN_MAX_SESSIONS         20

typedef struct {
    ClTxnClientHandleT          corTxnClientHandle;
    ClTxnAgentServiceHandleT    corTxnServiceHandle;
} ClCorTxnConnectionT;


typedef struct {
    ClUint32T                  structId;
    ClUint32T                  count;
    ClCorTxnJobHeaderNodeT   *pJobHdrNode[1];
}ClCorTxnJobListT;

#define ClCorTxnJobListStructId 0x7a530b93
    
/**
 * This is a initiailization function for using transaction service from COR
 */
extern ClRcT clCorTxnInterfaceInit();

/**
 * This is a finalization function for COR-TXN interface.
 */
extern ClRcT  clCorTxnInterfaceFini();


/**
 * Function to open session of modifying COR Objects.
 * This is will be available as RMD Callback function.
 */
extern ClRcT clCorTxnTxnSessionOpen(
            CL_IN   ClTxnTransactionHandleT     *txnHandle);

/**
 * Function to close this session of object modification.
 * This triggers transaction to complete all operations.
 * This is will be available as RMD Callback function.
 */
extern ClRcT clCorTxnTxnSessionClose(
            CL_IN   ClTxnTransactionHandleT     txnHandle);

/**
 * Function to cancel this session of object modification
 * This is will be available as RMD Callback function.
 */
extern ClRcT clCorTxnTxnSessionCancel(
            CL_IN   ClTxnTransactionHandleT     txnHandle);

/**
 * Function to perform the intended operation identified by
 * pTxnStream
 */
extern ClRcT clCorTxnProcessTxn (
            CL_IN   ClCorTxnJobHeaderNodeT         *pTxnJobHdrNode);

/**
 * Function to publish event per object (including contained object), for 
 * a given transaction stream.
 */
extern ClRcT  clCorTxnJobEventPublish(
           CL_IN   ClCorTxnJobHeaderNodeT *pJobHdrNode);

/**
 * Function to validate all operations of a given cor-txn-job.
 * -> fills unfilled objectHandle/moId and attrType
 * -> formats the user given data to Network Order.
 */
ClRcT   clCorTxnRegularize(CL_IN ClCorTxnJobListT * pTxnList,
                           CL_IN ClUint32T         idx, 
                           CL_OUT ClCntHandleT *pCntHandle,
                           CL_IN  const ClCorAddrPtrT pCompAddr);


/**
 *  Function for creating the container which will be used
 *  for storing the session related information.
 */

ClRcT   clCorBundleDataContCreate();

/**
 *  Function for deleting the enteries as well as the container
 *  which was used to store the session related information.
 */

ClRcT   clCorBundleDataContFinalize();

/**
 *  Function which will be used to seggregate the configuration and
 *  the runtime attribute jobs.
 */

ClRcT clCorBundleDataStore( CL_IN  ClCorTxnJobListT        *pBundleList,
                             CL_IN  ClTxnTransactionHandleT  txnHandle,
                             CL_IN  ClCorBundleDataPtrT    pBundleData,
                             CL_OUT ClCntHandleT            *pRtAttrCnt);

/**
 *  Function for adding the required jobs which will be processed using 
 *  the transaction.
 */

ClRcT clCorBundleTxnJobAdd( CL_IN ClTxnTransactionHandleT txnHandle,
                             CL_IN ClCntHandleT rtJobCont,
                             CL_IN ClCorBundleDataT sessionData);

/**
 *  Function for sending the jobs after getting all the jobs.
 */

ClRcT   clCorBundleResponseSend(ClCorBundleDataT sessionData);

/**
 *  Function for creating the container for storing the transaction jobs.
 */

ClRcT   clCorBundleJobsContainerCreate(ClCntHandleT *pBundleInfoCont);


/**
 * Function for finalizing the container used for storing the transaction jobs.
 */

ClRcT   clCorBundleJobsContainerFinalize(ClCntHandleT sessionInfoCont);


/**
 *  Function for getting the session information for the given transaction handle
 *  once the transaction completes.
 */

ClRcT clCorBundleDataGet( CL_IN ClTxnTransactionHandleT    txnHandle,
                           CL_OUT ClCorBundleDataPtrT      *pBundleStoreData);
/**
 *  Function for deleting the session from the session data container.
 */

ClRcT clCorBundleDataNodeDelete( CL_IN ClTxnTransactionHandleT    txnHandle);

/** 
 *  Function for adding the jobs in the container.
 */
ClRcT
clCorBundleAttrJobAdd(CL_IN ClCntHandleT            jobCont, 
                       CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                       CL_IN ClCorTxnObjJobNodeT     *pObjJobNode,
                       CL_IN ClCorTxnAttrSetJobNodeT *pAttrGetJobNode);

/** 
 * Function for aggregating the job information 
 */
ClRcT 
clCorBundleJobAggregate( CL_IN ClCorBundleDataT sessionData,
                          CL_IN ClCorTxnJobHeaderNodeT *pJobHdrNode);


/** 
 *  Function to add the jobs which are not processed at all.
 */


ClRcT
clCorBundleInvalidJobAdd(
                              CL_IN ClCorBundleDataT       sessionData,
                              CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                              CL_IN ClRcT                   status);
/**
 * Function to validate the transaction job list against all the 
 * in-progress job.
 */ 

ClRcT _clCorTxnJobInprogressValidation(ClCorTxnJobListT *pTxnList);

/**
 * Function to unset the in-progress transaction status to complete.
 */ 
ClRcT _clCorTxnJobInprogressUnset (ClCorTxnJobListT *pTxnList);

#ifdef __cplusplus
}
#endif


#endif  /* _CL_COR_TXN_INTERFACE_H_ */
