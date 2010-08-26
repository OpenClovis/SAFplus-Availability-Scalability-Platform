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
 * File        : clTxnAgentApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains all definitions relavant to transaction agent
 *
 *
 ********************************************************************************/


/********************************************************************************/
/************************* Transaction Agent functions *******************************/
/********************************************************************************/
/*                                                                              */
/* pagetxn201 : clTxnAgentInitialize                                            */
/* pagetxn202 : clTxnAgentFinalize                                              */
/* pagetxn203 : clTxnAgentServiceRegister                                       */
/* pagetxn204 : clTxnAgentServiceUnRegister                                     */
/*                                                                              */
/********************************************************************************/

/**
 *  \file
 *  \ingroup group34
 */

/**
 *  \addtogroup group34
 *  \{
 */




#ifndef _CL_TXN_AGENT_API_H_
#define _CL_TXN_AGENT_API_H_

#include <clCommon.h>
#include <clEoApi.h>
#include <clTxnErrors.h>
#include <clTxnApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/**
 * Handle to the service registering with the transaction-agent.
 */
typedef ClPtrT         ClTxnAgentServiceHandleT;

/**
 * Handle to the application-specific data retained by the transaction agent.
 */
typedef ClPtrT         ClTxnAgentCookieT;

/******************************************************************************
 *  Callback Functions
 *****************************************************************************/
/**
 * This callback function is invoked before the validation of a transaction
 */
typedef ClRcT (*ClTxnAgentTxnStartCallbackT) (
/**
 * Transaction handle for which this callback is called.
 */
                                                CL_IN   ClTxnTransactionHandleT txnHandle,
/**
 *  Handle to the cookie used by application implementation.
 */
                                                CL_INOUT    ClTxnAgentCookieT   *pCookie);
/**
 * This callback function is invoked after the completion of a transaction
 */
typedef ClRcT (*ClTxnAgentTxnStopCallbackT) (
/**
 * Transaction handle for which this callback is called.
 */
                                                CL_IN   ClTxnTransactionHandleT txnHandle,
/**
 *  Handle to the cookie used by application implementation.
 */
                                                CL_INOUT    ClTxnAgentCookieT   *pCookie);
/** 
 *  This callback function will be invoked when transaction agent validates the
 *  current job for its completion. Implementation must check for possibility of
 *  completion of the intended operation and must prepare for the same.
 */
typedef ClRcT (*ClTxnAgentTxnJobPrepareCallbackT) (
/**
 * Transaction handle for which this callback is called.
 */
                                                CL_IN   ClTxnTransactionHandleT txnHandle,

/**
 *  Handle to application-specific job definition.
 */
                                                CL_IN   ClTxnJobDefnHandleT     jobDefn, 
/**
 *  Size of the application-specific job definition.
 */
                                                CL_IN   ClUint32T               jobDefnSize, 
/**
 *  Handle to the cookie used by application implementation.
 */
                                                CL_INOUT    ClTxnAgentCookieT   *pCookie);


/**
 *  This callback function will be invoked when transaction agent commits the
 *  current job. Implementation must make sure that commit happens in ISOLATION
 *  with respect to concurrent transactions in the system.
 */
typedef ClRcT (*ClTxnAgentTxnJobCommitCallbackT) (
/**
 * Transaction handle for which this callback is made.
 */
                                                CL_IN   ClTxnTransactionHandleT txnHandle,
/**
 *  Handle to application-specific job definition.
 */
                                                CL_IN   ClTxnJobDefnHandleT     jobDefn, 
/**
 *  Size of the application-specific job definition.
 */
                                                CL_IN   ClUint32T               jobDefnSize, 
/**
 *  Handle to cookie used by application implementation.
 */
                                                CL_INOUT    ClTxnAgentCookieT   *pCookie);

/**
 *  This callback function will be invoked when Transaction Manager finds failure
 *  conditions for which transaction need to be aborted. 
 *
 *  \note
 *  It is the responsibility of application component developers to implement rollback
 *  functionality appropriately. Transaction service provides the necessary framework
 *  for failure-atomicity by invoking callback functions.
 */
typedef ClRcT (*ClTxnAgentTxnJobRollbackCallbackT) (
/**
 * Transaction handle for which this callback is made.
 */
                                                CL_IN   ClTxnTransactionHandleT txnHandle,
/**
 *  Handle to application specific job-definition.
 */
                                                CL_IN   ClTxnJobDefnHandleT     jobDefn, 
/**
 *  Size of the application specific job-definition.
 */
                                                CL_IN   ClUint32T               jobDefnSize, 
/**
 *  Handle to the cookie used by application implementation.
 */
                                                CL_INOUT    ClTxnAgentCookieT   *pCookie);


/******************************************************************************
 *  Data Structures
 *****************************************************************************/
/**
 * This structure contains a list of callback functions provided by
 * the application for transaction agent for their role in active transactions.
 *
 * Application developer needs to initialize this structure and use it while
 * calling clTxnAgentServiceRegister()
 */
typedef struct {
    ClTxnAgentTxnStartCallbackT         fpTxnAgentStart;
    ClTxnAgentTxnJobPrepareCallbackT    fpTxnAgentJobPrepare;
    ClTxnAgentTxnJobCommitCallbackT     fpTxnAgentJobCommit;
    ClTxnAgentTxnJobRollbackCallbackT   fpTxnAgentJobRollback;
    ClTxnAgentTxnStopCallbackT          fpTxnAgentStop;
} ClTxnAgentCallbacksT;


/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \page pagetxn201 clTxnAgentInitialize
 *
 *  \par Synopsis:
 *  Initializes the Transaction Agent.
 * 
 *  \par Header File:
 *  clTxnAgentApi.h 
 * 
 *  \par Syntax:
 *  \code   ClRcT clTxnAgentInitialize(
 *                           CL_IN ClEoExecutionObjT *pEoObj 
 *                           CL_IN ClVersionT *pVersion);
 *  \endcode
 *
 *  \param pEoObj: Pointer to current EO Object.
 *  \param pVersion: Version information passed from application being integrated.
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  
 *  \note
 *  Returned error is a combination of component-Id and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve error code.
 *
 *  \par Description:
 *  This function is used to initialize the Transaction Agent during the EO initialization 
 *  process. The Agent acts as intermediate layer between transaction service and application 
 *  during an active transaction. It receives job definition and commands from 
 *  Transaction Manager and takes specific action.
 * 
 *  \par Library File:
 *  ClTxnAgent 
 * 
 *  \par Related Function(s):
 *  \ref pagetxn201 "clTxnAgentFinalize"
 * 
 */
ClRcT clTxnAgentInitialize(
        CL_IN   ClEoExecutionObjT   *pEoObj, 
        CL_IN   ClVersionT          *pVersion);

/**
 ************************************
 *  \page pagetxn202 clTxnAgentFinalize
 *
 *  \par Synopsis:
 *  Finalize routine of transaction agent library. 
 * 
 *  \par Header File:
 *  clTxnAgentApi.h 
 * 
 *  \par Syntax:
 *  \code   ClRcT clTxnAgentFinalize();
 *  \endcode
 *
 *  \par Parameters
 *  None.
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \par Description:
 *  This function is used to clean up the Transaction Agent. It must be called
 *  during the finalization process of the EO. The application must have un-registered
 *  using clTxnAgentServiceUnRegister(), otherwise transaction could be in an inconsistent
 *  state.
  * 
 *  \par Library File:
 *  ClTxnAgent 
 *
 *  \par Related Function(s):
 *  \ref pagetxn201 "clTxnAgentInitialize" ,
 *  \ref pagetxn204 "clTxnAgentServiceUnRegister" 
 *
 */
ClRcT clTxnAgentFinalize();

/**
 ************************************
 *  \page pagetxn203 clTxnAgentServiceRegister
 *
 *  \par Synopsis:
 *  Registers a service hosted in this component. 
 * 
 *  \par Header File:
 *  clTxnAgentApi.h  
 * 
 *  \par Syntax:
 *  \code   ClRcT clTxnAgentServiceRegister(
 *                            CL_IN ClInt32T serviceId, 
 *                            CL_IN ClTxnAgentCallbacksT tCallbacks, 
 *                            CL_OUT ClTxnAgentServiceHandleT *pServiceHandle);
 *  \endcode
 *
 *  \param serviceId: ID of service hosted by this application component.
 *  \param tCallbacks: Instance of callback functions implementation application for this service.
 *  \param pServiceHandle: (out) Handle to registered service in Transaction Agent.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY: On memory allocating failure.
 *  \retval CL_ERR_DUPLICATE: If an entry for this service already exists.
 *  
 *  \note 
 *  Returned error is a combination of component-ID and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve error code.
 *
 *  \par Description:
 *  This function is used to register services hosted in the component. A component may 
 *  consist of multiple services. A transaction requires multiple services of 
 *  components for its completion. The Transaction Agent provides this function for services 
 *  to register for their interest in transactions.
 * 
 *  \par Library File:
 *  ClTxnAgent
 * 
 *  \par Related Function(s):
 *  \ref pagetxn204 "clTxnAgentServiceUnRegister" 
 * 
 */
ClRcT clTxnAgentServiceRegister(
        CL_IN   ClInt32T                    serviceId, 
        CL_IN   ClTxnAgentCallbacksT        tCallbacks, 
        CL_OUT  ClTxnAgentServiceHandleT    *pServiceHandle);
/**
 ************************************
 *  \page pagetxn204 clTxnAgentServiceUnRegister
 *
 *  \par Synopsis:
 *  Unregisters services provided by the component for its role in transaction. 
 * 
 *  \par Header File:
 *  clTxnAgentApi.h  
 * 
 *  \par Syntax:
 *  \code   ClRcT clTxnAgentServiceUnRegister(
 *                          CL_IN ClTxnAgentServiceHandleT serviceHandle); 
 *  \endcode
 *
 *  \param serviceHandle: Handle to the service has to be unregistered. 
 *  This is created by clTxnAgentServiceRegister().                   
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NOT_EXIST: If the corresponding entry does not exist.
 *  
 *  \note
 *  Returned error is a combination of component-Id and error-code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve error code.
 *
 *  \par Description:
 *  This function is used to un-register the services of component from the Transaction
 *  Management. Applications register their role in transactions using clTxnAgentServiceRegister()
 *  and provide application-specific callback functions.
  * 
 *  \par Library File:
 *  ClTxnAgent
 * 
 *  \par Related Function(s):
 *  \ref pagetxn203 "clTxnAgentServiceRegister"
 *
 * 
 */
ClRcT clTxnAgentServiceUnRegister(
        CL_IN   ClTxnAgentServiceHandleT    serviceHandle);


#ifdef __cplusplus
}
#endif

#endif  /* _CL_TXN_AGENT_API_H_ */



/** \} */


