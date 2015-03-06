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
 * ModuleName  : txn
 * File        : clTxnApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains external functions available for the application to manage
 * the transaction-jobs.
 *
 *
 *****************************************************************************/


/**********************************************************************************/
/************************* Transaction  functions ***********************************/
/**********************************************************************************/
/*                                                                                */
/* pagetxn101 : clTxnClientInitialize                                             */
/* pagetxn102 : clTxnClientFinalize                                               */
/* pagetxn103 : clTxnTransactionCreate                                            */
/* pagetxn104 : clTxnTransactionCancel                                            */
/* pagetxn105 : clTxnTransactionStart                                             */
/* pagetxn106 : clTxnTransactionStartAsync                                        */
/* pagetxn107 : clTxnJobAdd                                                       */
/* pagetxn108 : clTxnJobRemove                                                    */
/* pagetxn109 : clTxnComponentSet                                                 */
/*                                                                                */
/**********************************************************************************/


/**
 *  \defgroup group34 Transaction Manager (TM)
 *  \ingroup group4
 */

/**
 *  \file
 *  \ingroup group34
 */

/**
 * \name Transaction Manager (TM)
 */

/**
 *  \addtogroup group34
 *  \{
 */

#ifndef _CL_TXN_API_H_
#define _CL_TXN_API_H_

#include <clCommon.h>
#include <clIocApi.h>
#include <clCntApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \page pagetm Transaction Manager (TM)
 *
 *
 *  \par Overview
 *  The OpenClovis Transaction Manager (TM) provides an infrastructure library for resource managers
 *  to use transaction semantics to manage distributed data. Any component which requires to be
 *  part of transactions can link with this library and act as a resource manager. It automatically
 *  tracks participants and provides ACID semantics to ensure that all participants are updated
 *  or will rollback to previous state assuring data integrity despite component failures. \n
<BR>
 *  The TM supports re-startable transactions. For instance, if any transaction participant fails,
 *  data recovery mechanism helps to recover the application state using the transaction logs
 *  for enhanced availability.
 *
 *
 *
 *
 */

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 * Configuration for transaction processing. This is the default configuration
 * where atomicity is defined at transaction-level.
 */
#define CL_TXN_CONFIG_TXN_ATOMICITY                             0x0

/**
 * Configuration for transaction processing. This defines job-level atomicity.
 */
#define CL_TXN_CONFIG_JOB_ATOMICITY                             (0x1 << 0x0)

/**
 * Configuration for transaction processing. Restart if resource is temporarily
 * not available. If this is not set, abort immediately.
 */
#define CL_TXN_CONFIG_RESTART_ON_RESOURCE_UNAVAILABILITY        (0x1 << 0x1)

/**
 * Configuration for transaction processing. If there are agents participating
 * in transaction but have no matching service (or zero services), this flag when set
 * makes transaction processing continue ignoring such cases.
 */
#define CL_TXN_CONFIG_IGNORE_NO_SERVICE_REGD_AGENT              (0x1 << 0x2)

/**
 * Component configuration - default configuration for components capable of
 * two-phase commit processing.
 */
#define CL_TXN_COMPONENT_CFG_2_PC_CAPABLE                       (0x1 << 0x1)

/**
 * Component configuration - for One-phase commit capable components.
 */
#define CL_TXN_COMPONENT_CFG_1_PC_CAPABLE                       (0x1 << 0x0)

/**
 * Component configuration - for components involved only in verify-phase.
 */
#define CL_TXN_COMPONENT_CFG_FOR_VERIFY_PHASE                   (0x1 << 0x2)

/**
 * Wildcard value for job and agent service types.
 */
#define CL_TXN_SERVICE_TYPE_WILDCARD                            (0xFFFFFFFF)


/******************************************************************************
 *  Data Types
 *****************************************************************************/

/**
 *  The type of the handle for a client accessing transaction service.
 *  This handle is returned while the transaction client is initialized.
 *  It must also be used while finalizing the transaction client.
 */
typedef ClPtrT      ClTxnClientHandleT;

/**
 *  The type of the handle for an active transaction. This handle is returned
 *  when a new transaction is created. It must be passed as the
 *  first parameter in all subsequent operations pertaining to this
 *  transaction session.
 */
typedef ClHandleT   ClTxnTransactionHandleT;

/**
 *  The mask for specifying transaction configuration.
 *  This type must be passed as an argument during the creation of a new
 *  transaction.
 */
typedef ClUint8T    ClTxnConfigMaskT;

/**
 *   The type of handle for job of a transaction. This type is returned
 *   when a new transaction job is added. This type must be
 *   passed as an argument during subsequent operations pertaining to the job.
 */
typedef ClPtrT      ClTxnJobHandleT;

/**
 *  The type of handle is used for user-defined job definition. This type
 *  is passed only during creation of the job.
 */
typedef ClPtrT      ClTxnJobDefnHandleT;

/**
 * ClTxnAgentResp
 * {This structure is returned back to the application
 */
/* Transient: A failed phase also to be added to this structure that is returned to the
*  Application 
*/
typedef struct
{
    ClUint8T               failedPhase;
    ClTxnJobHandleT        jobHandle;
    ClTxnJobDefnHandleT    agentJobDefn;
    ClUint32T              agentJobDefnSize;
    ClIocPhysicalAddressT  iocPhyAddress;
}ClTxnAgentRespT;
/**
 *  The mask for specifying component configuration. This type must
 *  be passed as an argument when you define information about
 *  the components participating in the job.
 */
typedef ClUint8T    ClTxnCompConfigMaskT;

/******************************************************************************
 *  Callback Functions
 *****************************************************************************/
/**
 * This callback is specified at the time of initialization of the client.
 * This user-specified callback is invoked when an asynchronously initiated
 * transaction completes. The first parameter is the handle returned during the
 * creation of the transaction. The second parameter is the return code indicating
 * the status of the transaction (success or failure).
 */
typedef void(*ClTxnTransactionCompletionCallbackT) (ClTxnTransactionHandleT txnHandle,
                                                    ClRcT retCode);

/******************************************************************************
 *  Data Structures
 *****************************************************************************/

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \page pagetxn101 clTxnClientInitialize
 *
 *  \par Synopsis:
 *  Initializes transaction client library and registers callbacks.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnClientInitialize(
 *                            CL_IN ClVersionT *pVersion,
 *                            CL_IN ClTxnTransactionCompletionCallbackT pTxnCallback,
 *                            CL_OUT ClTxnClientHandleT *pTxnLibHandle);
 *  \endcode
 *
 *  \param pVersion: (in/out) As an input parameter, Version is a pointer
 *  to the required version of Transaction library.  As an output
 *  parameter, the version actually supported by the Transaction library
 *  is delivered.
 *
 *  \param pTxnCallback: This is an optional parameter if no asynchronous
 *  calls have been made. This callback function used to notify completion of
 *  transaction-job.
 *
 *  \param pTxnLibHandle: (out) This handle identifies this particular
 *  initialization of the transaction library. This must be passed as the first
 *  input argument in all further invocations of functions related to the
 *  transaction library.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: If either of the parameters \e pVersion and
 *  \e pTxnCallback passed to this function is a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY:  On memory allocation failure.
 *  \retval CL_ERR_VERSION_MISMATCH: If the version queried for is not the same
 *  as the version supported.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to
 *  retrieve error code.
 *
 *  \par Description:
 *  This function is used to initialize transaction client library.
 *  The application component integrated with transaction client library
 *  calls this function to initialize the library. This function initializes
 *  various callback functions and does version verification for
 *  invoking EO.
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn102 "clTxnClientFinalize"
 *
 */

ClRcT clTxnClientInitialize(
        CL_IN   ClVersionT                              *pVersion,
        CL_IN   ClTxnTransactionCompletionCallbackT     pTxnCallback,
        CL_OUT  ClTxnClientHandleT                      *pTxnLibHandle);

/**
 ************************************
 *  \page pagetxn102 clTxnClientFinalize
 *
 *  \par Synopsis:
 *  Finalizes transaction client library.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnClientFinalize(
 *                               CL_IN ClTxnClientHandleT txnLibHandle);
 *  \endcode
 *
 *  \param txnLibHandle: Handle of the transaction client library to
 *   be finalized. This is created by clTxnClientInitialize().
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h
 *  to retrieve error code.
 *
 *  \par Description:
 *  This function is used to finalize transaction client library. It frees all
 *  information pertaining to the transaction. It must be called by the
 *  application during termination of a process of an application,
 *  a component or an EO. To avoid memory leaks, every initialize
 *  call must be eventually followed by a finalize call.
 *
 *  \par Related Function(s):
 *  \ref pagetxn101 "clTxnClientInitialize"
 *
 */
ClRcT clTxnClientFinalize(
        CL_IN ClTxnClientHandleT txnLibHandle);

/**
 ************************************
 *  \page pagetxn103 clTxnTransactionCreate
 *
 *  \par Synopsis:
 *  Creates a new transaction.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnTransactionCreate(
 *                               CL_IN ClTxnConfigMaskT txnConfig,
 *                               CL_OUT ClTxnTransactionHandleT *pTxnHandle);
 *  \endcode
 *
 *  \param  txnConfig: Configuration for executing transaction-jobs.
 *  \param pTxnHandle: (out) Handle to the newly created transaction. In all
 *  further operations pertaining to this transaction, this handle must
 *  be passed as the input argument.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing txnConfig as an invalid parameter.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to
 *  retrieve error code.
 *
 *  \par Description:
 *  This client function is used to create a new transaction in the
 *  transaction server. The new transaction created gets a unique identification
 *  which is used for reference. The application creates a new transaction and
 *  registers with the server before sending the request to execute the operation. \n
<BR>
 *  The Transaction management provides different run-time configuration which are
 *  specified while defining it. Currently, the following configurations are supported:
 *  \arg \c ABORT: Aborts a transaction on failure of the transaction agent.
 *  \arg \c RESTART: Restarts a transaction on failure of transaction agent.
 *  \arg \c ALL_JOB: Strict Atomicity is defined by execution of all jobs.
 *  \arg \c ANY_JOB: Lenient Atomicity is defined by execution of any particular job.
 *
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn104 "clTxnTransactionCancel" ,
 *  \ref pagetxn105 "clTxnTransactionStart" ,
 *  \ref pagetxn106 "clTxnTransactionStartAsync"
 *
 *
 */

ClRcT clTxnTransactionCreate(
        CL_IN   ClTxnConfigMaskT            txnConfig,
        CL_OUT  ClTxnTransactionHandleT     *pTxnHandle);

/**
 ************************************
 *  \page pagetxn104 clTxnTransactionCancel
 *
 *  \par Synopsis:
 *  Cancels the given transaction.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnTransactionCancel(
 *                               CL_IN ClTxnTransactionHandleT txnHandle);
 *  \endcode
 *
 *  \param txnHandle: Handle of transaction to be deleted.
 *  This is created by clTxnTransactionCreate().
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameters.
 *  \retval CL_ERR_TIMEOUT: If communication fails with server.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h
 *  to retrieve error code.
 *
 *  \par Description:
 *  This client function is used to cancel the transaction in the
 *  transaction server. A transaction can be canceled if it is in
 *  \c PRE-INIT state. The \c PRE-INIT state is when the jobs are being defined
 *  and \c COMMIT command has not been issued by the server to
 *  any one of the components participating in the transaction.
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn103 "clTxnTransactionCreate" ,
 *  \ref pagetxn105 "clTxnTransactionStart" ,
 *  \ref pagetxn106 "clTxnTransactionStartAsync"
 *
 *
 */
ClRcT clTxnTransactionCancel(
        CL_IN   ClTxnTransactionHandleT     txnHandle);
/**
 ************************************
 *  \page pagetxn105 clTxnTransactionStart
 *
 *  \par Synopsis:
 *  Starts a transaction identified by transaction-ID (asynchronously).
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnTransactionStart(
 *                               CL_IN ClTxnTransactionHandleT txnHandle);
 *  \endcode
 *
 *  \param  txnHandle:  Handle of transaction to be started.
 *   This is created by clTxnTransactionCreate().
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_TXN_ERR_NOT_INITIALIZED: If transaction is not initialized.
 *  \retval CL_TXN_ERR_VALIDATE_FAILED: If application-defined preparation for
 *  the transaction fails.
 *  \retval CL_TXN_ERR_COMMIT_FAILED: If transaction results in aborting
 *  of operation.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameters.
 *  \retval CL_ERR_TIMEOUT: If communication fails with server.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to
 *  retrieve error code.
 *
 *  \par Description:
 *  This function is used to start a transaction identified by transaction-ID.
 *  The client creates a new transaction to execute the intended operation.
 *  Once the task is defined for this transaction, this function triggers
 *  the co-ordination to complete the transaction. This is a blocking call.
  *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn103 "clTxnTransactionCreate" ,
 *  \ref pagetxn104 "clTxnTransactionCancel" ,
 *  \ref pagetxn106 "clTxnTransactionStartAsync"
 *
 *
 */
ClRcT clTxnTransactionStart(
        CL_IN ClTxnTransactionHandleT txnHandle);

/**
 ************************************
 *  \page pagetxn106 clTxnTransactionStartAsync
 *
 *  \par Synopsis:
 *   Synchronously starts a transaction identified by the transaction-ID.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnTransactionStartAsync(
 *                               CL_IN ClTxnTransactionHandleT txnHandle);
 *  \endcode
 *
 *  \param  txnHandle: Handle of transaction to be started asynchronously.
 *   This is created by clTxnTransactionCreate().
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_TXN_ERR_NOT_INITIALIZED: If transaction is not initialized.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameters.
 *  \retval CL_ERR_TIMEOUT: If communication fails with server.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h
 *  to retrieve error code.
 *
 *  \par Description:
 *  This function is used to start a transaction asynchronously identified
 *  by a transaction-ID.
 *  The client creates a new transaction to execute the intended operation.
 *  Once the task is defined for this transaction, this function triggers
 *  the background coordination to complete the transaction. This is non-blocking.
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn103 "clTxnTransactionCreate" ,
 *  \ref pagetxn104 "clTxnTransactionCancel" ,
 *  \ref pagetxn105 "clTxnTransactionStart"
 *
 *
 */

ClRcT clTxnTransactionStartAsync(
        CL_IN ClTxnTransactionHandleT txnHandle);

/**
 ************************************
 *  \page pagetxn107 clTxnJobAdd
 *
 *  \par Synopsis:
 *  Registers a new job as part of transaction. A transaction consists
 *  of atleast one job description.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnJobAdd(
 *                              CL_IN ClTxnTransactionHandleT      txnHandle,
 *                              CL_IN ClTxnJobDefnHandleT  jobDefn,
 *                              CL_IN ClUint32T             jobDefnSize,
 *                              CL_IN ClInt32T              serviceType,
 *                              CL_OUT ClTxnJobHandleT      *pJobHandle);
 *  \endcode
 *
 *  \param txnHandle:  Transaction handle to which the job is to be added.
 *   This is created by clTxnTransactionCreate().
 *  \param jobDefn:  Application-defined job description.
 *  \param jobDefnSize: Size of application defined job-definition.
 *  \param serviceType: Service within component involved in transaction
 *  (-1 for all services of component).
 *  \param pJobHandle: (out) Identification of job for later references.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameters.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve error code.
 *
 *  \par Description:
 *  This function is used to register a new job as a part of a transaction.
 *  As part of modifying a data entity in distributed environment, application provides
 *  necessary details termed as job-description. Flexibility is given to application developer
 *  to define job and pass-on definition as a byte-stream. Hence details about jobs are
 *  transparent to the transaction management. Each job is identified using a unique job-ID.
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn108 "clTxnJobRemove"
 *
 *
 */
ClRcT clTxnJobAdd(
        CL_IN   ClTxnTransactionHandleT     txnHandle,
        CL_IN   ClTxnJobDefnHandleT         jobDefn,
        CL_IN   ClUint32T                   jobDefnSize,
        CL_IN   ClInt32T                    serviceType,
        CL_OUT  ClTxnJobHandleT             *pJobHandle);

/**
 ************************************
 *  \page pagetxn108 clTxnJobRemove
 *
 *  \par Synopsis:
 *  Removes a registered job from the transaction.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnJobRemove(
 *                              CL_IN ClTxnTransactionHandleT txnHandle,
 *                              CL_IN ClTxnJobHandleT jobHandle);
 *  \endcode
 *
 *  \param txnHandle:  Transaction handle from which the job is to be removed.
 *   This handle is created by clTxnTransactionCreate().
 *  \param jobHandle: Handle of the job to be deleted. This handle
 *   is created by clTxnJobAdd().
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NOT_EXIST: If there is no transaction or job corresponding
 *  to the handles passed.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to
 *  retrieve error code.
 *
 *  \par Description:
 *  This function is used to remove an already registered job from transaction.
 *  As part of the defining a transaction, application defines a transaction
 *  and manages jobs by registering and removing them. Each job registered in
 *  transaction is uniquely identified by a job-ID.
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  \ref pagetxn107 "clTxnJobAdd"
 *
 *
 */
ClRcT clTxnJobRemove(
        CL_IN   ClTxnTransactionHandleT     txnHandle,
        CL_IN   ClTxnJobHandleT             jobHandle);

/**
 ************************************
 *  \page pagetxn109 clTxnComponentSet
 *
 *  \par Synopsis:
 *  Sets a component to be involved in transaction-job.
 *
 *  \par Header File:
 *  clTxnApi.h
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clTxnComponentSet(
 *                       CL_IN ClTxnTransactionHandleT  txnHandle,
 *                       CL_IN ClTxnJobHandleT  jobHandle,
 *                       CL_IN ClIocAddressT txnCompAddress,
 *                       CL_IN ClTxnCompConfigMaskT  configMask);
 *  \endcode
 *
 *  \param txnHandle : Transaction handle of the job for which
 *   components is being added. This is created by clTxnTransactionCreate().
 *  \param jobHandle: Transaction-job handle for which components is to be added.
 *  This is created by clTxnJobAdd().
 *  \param txnCompAddress: IOC address of the component.
 *  \param configMask: Configuration information defining the role of
 *  this component in transaction.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NOT_EXIST: If there is no job corresponding to the handles passed.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *  \retval CL_ERR_DUPLICATE: If a duplicate entry exists.
 *
 *  \note
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h
 *  to retrieve error code.
 *
 *  \par Description:
 *  This function is used to set a component to be involved in a transaction-job.
 *  The application interfacing transaction-management using these client functions
 *  needs to provide a list of components involved in transaction. Optionally, it
 *  could also control the order in which these components are to be visited during
 *  execution of the transaction.
 *
 *  \note
 *  Currently, the list of components are for the job. The application must
 *  explicitly specify for each job, whether the transaction contains multiple jobs.
 *
 *  \note
 *  Order in which a component participates in a given transaction is dependent
 *  on capabilities of all components (for 1-PC Capable, 2-PC Capable) and other
 *  factors related to resource availability (locks, for example).
 *
 *  \par Library File:
 *  ClTxnClient
 *
 *  \par Related Function(s):
 *  None.
 *
 *
 */
ClRcT clTxnComponentSet(
        CL_IN   ClTxnTransactionHandleT txnHandle,
        CL_IN   ClTxnJobHandleT         jobHandle,
        CL_IN   ClIocAddressT           txnCompAddress,
        CL_IN   ClTxnCompConfigMaskT    configMask);

/**
 * This api is use to get the list of failed job list. This needs to called each time, until NULL is returned with
 * pNextNodeHandle.
 */
ClRcT   clTxnJobInfoGet(
        CL_IN   ClTxnTransactionHandleT txnHandle,
        CL_IN   ClCntNodeHandleT        currentNodeHandle,
        CL_OUT  ClCntNodeHandleT        *pNextNodeHandle,
        CL_OUT  ClTxnAgentRespT         *pAgentResp);
/**
 * This deletes the entire agent response list
 */
ClRcT   clTxnJobInfoListDel(
        ClTxnTransactionHandleT txnHandle);

/**
 * This is specific to COR,to read 
 * the jobs from agents
 */

ClRcT clTxnReadAgentJobsAsync(
        CL_IN ClTxnTransactionHandleT tHandle,
        CL_IN ClTxnTransactionCompletionCallbackT pTxnReadJobCallback);
#ifdef __cplusplus
}
#endif

#endif  /* _CL_TXN_API_H_ */
