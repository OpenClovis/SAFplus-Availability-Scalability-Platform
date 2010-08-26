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
 * ModuleName  : cor                                                           
 * File        : clCorTxnApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  This file contains the transaction related APIs for COR.
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Transaction related APIs for COR
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */

#ifndef _CL_COR_TXN_API_H_
#define _CL_COR_TXN_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clTxnApi.h>
#include <clCorMetaData.h>

/*********************************************************************************/
/******************************** COR APIs ***************************************/
/*********************************************************************************/
/*                                                                               */
/* clCorTxnJobWalk                                                               */ 
/* clCorTxnSessionCommit                                                         */
/* clCorTxnSessionFinalize                                                       */
/* clCorTxnSessionCancel                                                         */
/* clCorTxnJobHandleToCorTxnIdGet                                                */
/* clCorTxnIdTxnFree                                                             */
/* clCorTxnJobAttributeTypeGet                                                   */
/* clCorTxnJobSetParamsGet                                                       */ 
/* clCorTxnJobAttrPathGet                                                        */ 
/* clCorTxnJobOperationGet                                                       */ 
/* clCorTxnJobMoIdGet                                                            */
/* clCorTxnJobObjectHandleGet                                                    */
/* clCorTxnFirstJobGet                                                           */
/* clCorTxnLastJobGet                                                            */
/* clCorTxnNextJobGet                                                            */
/* clCorTxnPreviousJobGet                                                        */
/* clCorTxnFailedJobGet                                                          */
/* clCorTxnJobStatusSet                                                          */
/* clCorTxnJobStatusGet                                                          */
/* clCorTxnJobDefnHandleUpdate                                                   */
/*                                                                               */
/*********************************************************************************/

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

#define COR_TXN_MAX_STATIONS   8
#define  CL_COR_TXN_SERVICE_ID_READ     1 
#define CL_COR_TXN_SERVICE_ID_WRITE     2 


/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/**
 *****************************************************************************
 *  \brief Type of the callback function which will be called 
 *  while doing transaction job walk.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param trans (in) Transaction Id being walked
 *  \param jobId (in) Job Id in the transaction.
 *  \param cookie (in) User passed data.
 *
 *  \retval ClRcT User passed return value.
 *
 *  \par Description:
 *  This type is used to define the callback function which is used in transaction
 *  job walk. This callback function is passed as an argument to the transaction 
 *  job walk function clCorTxnJobWalk(). This callback function will be called for 
 *  every job found in the transaction. The parameter cookie contains the user-data 
 *  passed when the walk function is called. The parameters txnId contains 
 *  the ID of the Transaction and jobId contains the ID of the job being walked.
 *
 *  \par Library Name:
 *  ClCorClient
 */
typedef ClRcT  (*ClCorTxnFuncT)(ClCorTxnIdT  trans,
                                   ClCorTxnJobIdT jobId,
                                   void *cookie);

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************************
 *  \brief Walk through the transaction Jobs.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param pThis Pointer to the Transaction ID.
 *  \param funcPtr Callback function for handling each job.
 *  \param cookie Pointer to the user data.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS When there are no jobs to walk.
 *
 *  \par Description:
 *  This function is used to walk through the transaction jobs. The callback function is used
 *  which operate on each of the jobs in the transaction. 
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobMoIdGet(), clCorTxnJobAttrPathGet(), 
 *      clCorTxnJobObjectHandleGet(), clCorTxnJobSetParamsGet()
 * 
 */
extern ClRcT    clCorTxnJobWalk(CL_IN ClCorTxnIdT  pThis,
                                CL_IN ClCorTxnFuncT funcPtr,
                                CL_IN void  *cookie);

/**
 ************************************************
 *  \brief Commits an active transaction session.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnSessionId Transaction session ID. The session ID is a unique ID which identifies a COMPLEX transaction
 *  obtained while calling object create, attribute set, or object delete APIs.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS If there are no jobs to commit.
 *
 *  \par Description:
 *  The transaction session ID is obtained using clCorObjectCreate()/clCorObjectAttributeSet()/clCorObjectDelete()
 *  APIs. When this API is invoked, the transaction request is sent to the COR server for
 *  processing. The server contacts the OIs for MOs in the transaction job and completes the
 *  transaction in a two phase commit manner. This is a synchronous operation so the thread
 *  calling this API is blocked until a response is received.
 *
 *  \note
 *  If this API fails, then the user should call clCorTxnSessionFinalize after retrieving the 
 *  failed job information to remove the transaction as well as the failed job information.
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnSessionFinalize()
 */
extern ClRcT    clCorTxnSessionCommit(CL_IN    ClCorTxnSessionIdT     txnSessionId);

/**
 ************************************************
 *  \brief Cancels a transaction session.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnSessionId: Transaction session ID.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE The txnSessionId is invalid.
 *
 *  \par Description:
 *   This API is used to cancel the active transaction session whose session ID is passed through this API.
 *   This API is deprecated, the user is supposed to use clCorTxnSessionFinalize.
 *
 *  \par Library Name:
 *  ClCorClient
 *   
 *  \sa clCorTxnSessionFinalize
 */
extern ClRcT    clCorTxnSessionCancel(CL_IN    ClCorTxnSessionIdT  txnSessionId) CL_DEPRECATED;

/**
 ************************************************
 *  \brief Finalizes a COR transaction session.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnSessionId: Transaction session ID.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE The txnSessionId is invalid.
 *
 *  \par Description:
 *  This API is used to free all the resources that are allocated for a COMPLEX transaction only
 *  when the transaction fails. It frees the allocated resources which include the client specific
 *  data and the failed jobs list. This function should be called to free the resources used by the
 *  transaction session.
 *
 *  \par Library Name:
 *  ClCorClient
 *   
 *  \sa clCorTxnSessionCommit()
 */
extern ClRcT    clCorTxnSessionFinalize(CL_IN    ClCorTxnSessionIdT  txnSessionId);

/**
 ************************************************
 *  \brief Get thetransactionID from thetransactionJob handle.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 * 
 *  \param jobDefn Transaction job handle.
 *  \param size Size of the transaction.
 *  \param pTxnId (out) Transaction ID .
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to retrieve the COR Transaction ID by using the transaction job handle.
 * 
 *  \par Library Name:
 *  ClCorClient
 *  
 */
extern ClRcT clCorTxnJobHandleToCorTxnIdGet(
           CL_IN    ClTxnJobDefnHandleT   jobDefn,
           CL_IN    ClSizeT               size, 
           CL_OUT   ClCorTxnIdT          *pTxnId);


/**
 ************************************************
 *  \brief Frees the data for the transactionID .
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param corTxnId COR transaction ID.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to free the data corresponding to the transactionID .
 * 
 *  \par Library Name:
 *  ClCorClient
 *  
 */
extern ClRcT clCorTxnIdTxnFree(
           CL_IN    ClCorTxnIdT    corTxnId);

/**
 ************************************************
 *  \brief Get the attribute type information.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID.
 *  \param  jobId ID of the job in the transaction.
 *  \param  pAttrType (out) Attribute type that is whether it is simple or array or association.
 *  \param  pAttrDataType (out) Basic type of the attribute. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID Invalid job ID.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *  This function is used to extract the attribute type and its basic type from the transaction job. In case of a simple attribute 
 *  (which corresponds to the ClCorTypeT) the basic type is indicated by \e pAttrDataType. In case of an array type, 
 *  \e pAttrDataType contains the basic type.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobSetParamsGet()
 *
 */
extern ClRcT clCorTxnJobAttributeTypeGet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId,
          CL_OUT  ClCorAttrTypeT      *pAttrType,
          CL_OUT  ClCorTypeT          *pAttrDataType);


/**
 ************************************************
 *  \brief Get all the information necessary for setting the attribute.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID .
 *  \param  jobId Transaction job ID .
 *  \param  pAttrId (out) Pointer to attribute ID .
 *  \param  pIndex (out) Pointer index of the attribute ( CL_COR_INVALID_ATTR_IDX in case of simple attributes).
 *  \param  pValue (out) Pointer to the pointer to the value.
 *  \param  pSize (out) Size of the attribute value to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID Job ID passed is NULL.
 *
 *  \par Description:
 *  This function is used to extract the set parameters that were passed when the set function is called on the client side.
 *  The set parameters are the following:
 *  \arg \e attrId 
 *  \arg index of the attribute
 *  \arg pointer to the value
 *  \arg size of the attribute value
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \note
 *  This function is used only when the operation type is \e set.
 *
 *  \sa clCorTxnJobOperationGet(), clCorTxnJobAttrPathGet()
 */
extern  ClRcT clCorTxnJobSetParamsGet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_OUT  ClCorAttrIdT        *pAttrId,
          CL_OUT  ClInt32T            *pIndex,
          CL_OUT  void               **pValue,
          CL_OUT  ClUint32T           *pSize);

/**
 ************************************************
 *  \brief Retrieves the \e attrpath from the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID .
 *  \param  jobId Transaction Job ID .
 *  \param  pAttrPath (out) Pointer to pointer of attrPath.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID Returned when the operation type is not set.
 *
 *  \par Description:
 *  This function is used to extract the \e attrpath or the containment path for the \e set operation.
 *  If the operation type is not 'set', an error is returned. 
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobSetParamsGet(), clCorTxnJobOperationGet()
 */

extern  ClRcT  clCorTxnJobAttrPathGet(
          CL_IN   ClCorTxnIdT     txnId, 
          CL_IN   ClCorTxnJobIdT  jobId,
          CL_OUT  ClCorAttrPathT      **pAttrPath);


/**
 ************************************************
 *  \brief Get the operation type.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID .
 *  \param  jobId Transaction Job ID .
 *  \param     op (out) Operation Type. \c (CL_COR_CREATE/SET/DELETE)
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function is used to extract the operation type from the transaction job. The transaction job type can 
 *   can be one of the following:
 *   \arg \c CL_COR_CREATE
 *   \arg \c CL_COR_DELETE
 *   \arg \c CL_COR_SET
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobMoIdGet(), clCorTxnJobAttrPathGet(),
 *      clCorTxnJobSetParamsGet()
 */
extern  ClRcT  clCorTxnJobOperationGet(
         CL_IN     ClCorTxnIdT    txnId,
         CL_IN     ClCorTxnJobIdT jobId,
         CL_OUT    ClCorOpsT          *op);


/**
 ************************************************
 *  \brief Get the MoId From the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID .
 *  \param  pMOId Pointer to the MoId for the transaction.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a Null Pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *   This function is used to extract the unique \e MoID associated with each transaction ID .
 *   The transaction having multiple jobs are broken into jobs 
 *   having moId, attrPath (if used)  and operation. So while we walk we get these jobs and
 *   we can operate on these data one by one.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobAttrPathGet(), clCorTxnJobSetParamsGet()
 */
extern  ClRcT  clCorTxnJobMoIdGet(
         CL_IN     ClCorTxnIdT  txnId,
         CL_OUT    ClCorMOIdT  *pMOId);


/**
 ************************************************
 *  \brief Get the Object handle from the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID .
 *  \param pObjHandle (out) Pointer to the Object handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *
 *  \par Description:
 *  This function is used to extract the Object handle from the transaction. The transaction will contain the 
 *  Object handle that it passed when the transaction was prepared. 
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobMoIdGet(),
 *      clCorTxnJobAttrPathGet(),
 *      clCorTxnJobSetParamsGet()
 */
extern  ClRcT  clCorTxnJobObjectHandleGet(
         CL_IN     ClCorTxnIdT  txnId,
         CL_OUT    ClCorObjectHandleT  *pObjHandle);


/**
 ************************************************
 *  \brief Get the first job.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID .
 *  \param  pJobId Pointer to the transaction job ID .
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *
 *  \par Description:
 *  This function is used to get the first job in the transaction jobs when a walk is being performed. 
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnNextJobGet(),
 *      clCorTxnPreviousJobGet(),
 *      clCorTxnLastJobGet()
 */
extern  ClRcT  clCorTxnFirstJobGet(
         CL_IN     ClCorTxnIdT          txnId,
         CL_OUT    ClCorTxnJobIdT  *pJobId);

/**
 ************************************************
 *  \brief Get the last job in the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param  txnId Transaction ID .
 *  \param  pJobId (out) Pointer to the transaction Job ID .
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *
 *  \par Description:
 *  This function is used to retrieve the last job in the transaction. The operation type for 
 *  transaction job is \e set.
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnPreviousJobGet()  
 */

extern  ClRcT  clCorTxnLastJobGet(
         CL_IN     ClCorTxnIdT            txnId,
         CL_OUT    ClCorTxnJobIdT   *pJobId);

/**
 ************************************************
 *  \brief Get the next job in the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID . 
 *  \param currentJobHdl Current transaction job handle.
 *  \param pNextJobHdl (out) Pointer to the next transaction job handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *  \retval CL_COR_TXN_ERR_LAST_JOB If the current job is the last job.
 *
 *  \par Description:
 *   This function is used to retrieve the next job queued in the transaction list.
 *   This function can be used after getting the first job in the transaction list.
 *  
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnFirstJobGet()
 */

extern  ClRcT  clCorTxnNextJobGet(
         CL_IN     ClCorTxnIdT           txnId,
         CL_IN     ClCorTxnJobIdT    currentJobHdl,
         CL_OUT    ClCorTxnJobIdT   *pNextJobHdl);


/**
 ************************************************
 *  \brief Get the previous job in the transaction.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID . 
 *  \param currentJobHdl Current transaction job handle.
 *  \param pNextJobHdl (out) Pointer to the next transaction job handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR On passing a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM On passing an invalid parameter.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *  \retval CL_COR_TXN_ERR_FIRST_JOB If the current job is the first job.
 *
 *  \par Description:
 *   This function is used to get the previous job from the current job handle. This function must not  
 *   be used while the current handle is pointing towards the first job in the transaction job list.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnNextJobGet()
 */

extern ClRcT  clCorTxnPreviousJobGet(
         CL_IN     ClCorTxnIdT      txnId,
         CL_IN     ClCorTxnJobIdT   currentJobId,
         CL_OUT    ClCorTxnJobIdT  *pPrevJobId);

/**
 ************************************************
 *  \brief Retrieves the information about the failed transaction job for a pariticular transaction Id.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnSessionId (in) The transaction session Id for which the 
 *         transaction had failed.
 *  \param pPrevTxnInfo (in) If this is set to NULL, the first failed job information is sent in pNextTxnInfo.
 *  Otherwise, the next entry with respect to pPrevTxnInfo is sent.
 *  \param pNextTxnInfo (out) The next failed job information is populated in this parameter.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_COR_TXN_ERR_FAILED_JOB_GET For the transaction Id there is no failed job info avaliable.
 *
 *  \par Description:
 *  Application that initiates complex transaction to perform MO create/delete and attribute set operation, need
 *  to use the API to obtain  information regarding failures. Any component that partcipates in a transaction can
 *  report failures via their agent callback functions. The above function retrieves all such failures reported.
 *  \par 
 *  In case of multiple failures (on differnt components), there would be more than one  error
 *  entry added in COR for a particular transaction. All the error enteries are obtained through the
 *  following mechanism
 *  - Obtain First Entry:  Call the API with the parameter pPrevTxnInfo specified as NULL. This function
 *    would return the first error record in pNextTxnInfo parameter.
 *  - Obtain SubSequent Enteries: Copy the content of pNextTxnInfo, obtained in previous invocation of this API, 
 *    and assign it to pPrevTxnInfo and call clCorTxnFailedJobGet.
 * 
 *  \note
 *  After receiving failed jobs, the clCorTxnSessionFinalize() API must be called to free the memory.
 *
 *  \par Library Name:
 *  ClCorClient
 *
 *  \sa clCorTxnSessionFinalize()
 */

extern ClRcT clCorTxnFailedJobGet(
                    ClCorTxnSessionIdT txnSessionId, 
                    ClCorTxnInfoT *pPrevTxnInfo, 
                    ClCorTxnInfoT *pNextTxnInfo);

/**
 ************************************************
 *  \brief Set the status of a particular job.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID . 
 *  \param jobId job id in the transaction.
 *  \param jobStatus status to be set. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID The jobId passed is invalid. 
 *
 *  \par Description:
 *   This function is used to set the status of a particular job, given its txnId and jobId. The status will 
 *   be a 32-bit value.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobStatusGet()
 */

extern ClRcT clCorTxnJobStatusSet(
         CL_IN   ClCorTxnIdT        txnId,
         CL_IN   ClCorTxnJobIdT     jobId,
         CL_IN   ClUint32T          jobStatus);

/**
 ************************************************
 *  \brief Get the status of a particular job.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param txnId Transaction ID . 
 *  \param jobId job id in the transaction.
 *  \param jobStatus to get the status. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs in the transaction. 
 *  \retval CL_COR_TXN_ERR_INVALID_JOB_ID The jobId passed is invalid. 
 *
 *  \par Description:
 *   This function is used to get the status of a particular job, given its txnId and jobId. The status will 
 *   be a 32-bit value.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 *  \sa clCorTxnJobStatusSet()
 */

extern ClRcT clCorTxnJobStatusGet(
         CL_IN      ClCorTxnIdT     txnId,
         CL_IN      ClCorTxnJobIdT  jobId,
         CL_OUT     ClUint32T*      jobStatus);

/**
 ************************************************
 *  \brief This function is used to pack the transaction information and 
 *  update it in the given job definition handle.
 *
 *  \par Header File:
 *  clCorTxnApi.h
 *
 *  \param jobDefnHandle Handle for the job definition. 
 *  \param corTxnId Transaction Id.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *   This function is used to copy the updated transaction information into the job definition handle
 *   given in the agent callback functions. It will be called by the agents when the given job
 *   got failed.
 * 
 *  \par Library Name:
 *  ClCorClient
 * 
 */

extern ClRcT clCorTxnJobDefnHandleUpdate(
                CL_OUT     ClTxnJobDefnHandleT  jobDefnHandle, 
                CL_IN      ClCorTxnIdT          corTxnId);


extern ClRcT clCorTxnJobOmClassIdGet(CL_IN ClCorTxnIdT corTxnId,
                                   CL_OUT ClCorClassTypeT * pOmClassId);

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_COR_TXN_API_H_ */



/** \} */
