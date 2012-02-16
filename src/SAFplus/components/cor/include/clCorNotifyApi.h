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
 * File        : clCorNotifyApi.h
 *******************************************************************************/

/*****************************************************************************
 * Description :                                                                
 *
 * The files contains APIs for subscribing/unsubscribing to the events
 * on a Managed Objects. and associated datastrucutes.
 *
 *
 *****************************************************************************/

/****************************************************************************************/
/******************************** COR APIs **********************************************/
/****************************************************************************************/
/*                                                                                      */
/* clCorEventSubscribe                                                          */
/* clCorEventUnsubscribe                                                   */
/* clCorNotifyEventToOperationGet < -- OBSOLETE -- >                       */
/* clCorNotifyEventToMoIdGet      < -- OBSOLETE -- >                       */
/* clCorNotifyEventToAttrPathGet  < -- OBSOLTE -- >                         */
/* clCorNotifyEventToTransactionHandleGet <CHGD TO> clCorEventToCorTxnIdGet */
/*                                                                                       */
/*****************************************************************************************/

/**
 *  \file
 *  \brief Header file of COR APIs for subscribing/unsubscribing to the events
 *         on a Managed Objects.
 *  \ingroup cor_apis
 */


/**
 *  \addtogroup cor_apis
 *  \{
 */

#ifndef _CL_COR_NOTIFY_API_H_
#define _CL_COR_NOTIFY_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clEoApi.h>
#include <clEoConfigApi.h>
#include <clEventApi.h>
#include "clCorMetaData.h"
#include "clCorServiceId.h"
#include <clCorTxnApi.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/**
 * Type to store the list of attributes.
 */
typedef struct ClCorAttrList {
  /**
   * Number of attributes.
   */
  ClUint32T attrCnt;

  /**
   * List of attributes.
   */
  ClInt32T attr[1];
} ClCorAttrListT;

/**
 * Pointer type to ClCorAttrListT.
 */
typedef ClCorAttrListT * ClCorAttrListPtrT;

/**
 * Type to store the list of MoIds.
 */
typedef struct ClCorMOIdList
{
    /**
     * Number of MoIds.
     */
    ClUint32T moIdCnt;

    /**
     * List of MoIds.
     */
    ClCorMOIdT moId[1];

}ClCorMOIdListT;

/**
 * Pointer type to ClCorMOIdListT.
 */
typedef ClCorMOIdListT* ClCorMOIdListPtrT;

/**
 *  COR notify handle, return by subscribe, used for unsubscribing.
 */
typedef ClHandleT  ClCorNotifyHandleT;


#define clCorEventName "COR_EVT_CHANNEL"
#define CL_COR_EVT_CHANGE_NOTIFICATION 0x1

/* This is an IPI which is only used by alarm client to susbscribe for multiple moIds. */
extern ClRcT
clCorMOListEventSubscribe(ClEventChannelHandleT     channelHandle,
                                ClCorMOIdListPtrT   pMoIdList,
                                ClCorAttrPathPtrT   pAttrPath,
                                ClCorAttrListPtrT   pAttrList,
                                ClCorOpsT           ops,
                                void*               cookie,
                                ClEventSubscriptionIdT subscriptionID);

/**
 **********************************************
 *  \brief Subscribes for notifications when a change occurs in an attribute.
 *
 *  \par Header File:
 *  clCorNotifyApi.h
 *
 *  \param channelHandle (in) Handle of the COR channel. You are required to allocate the memory
 *                         for this parameter.
 *
 *  \param changedObj (in) This is the complete path to the object. Wildcards can be used to specify
 *                    a class or subtree of objects.
 *
 *  \param pAttrPath (in) To subscribe for the attributes of the contained objects, the attrPath
 *                    identifying the contained object must be specified. It must be set to NULL for the
 *                     current implementation and will be supported in future releases.
 *  \param attrList (in) If the subscriber is interested in receiving notifications when certain
 *  attribute(s) of an object change, the list of these attribute IDs can be specified here. If it
 *  is set to NULL, the subscriber receives notifications when a change occurs in any of the
 *  attributes of the specified MO.
 *  Following are the critical usage restrictions for this parameter:
 *  - \c This parameter is interpreted only if the parameter flags contains one or more
 *  _SET_ operations. Refer to 3.5.5 for the possible operations types.
 *  - Specifying the attrList implies that you are interested in the changes on the
 *  attribute level.
 *  - The service ID must not contain wildcard entries.
 *  - The class value in changedObj (which is a MOID) must not contain wildcard
 *  entries.
 *  - Wildcard entries can be used to specify instance value in changedObj.
 *
 *  \param flags (in) This parameter contains the operations that the user can subscribe for. The
 *               operations are:
 *  - \c CREATE
 *  - \c DELETE
 *  - \c SET
 *
 *  \param cookie (in) This contains the user-data which is passed as the parameter to the
 *                      notification callback function.
 *
 *  \param subscriptionId (in)You are required to provide the subscription ID. This subscription ID
 *                          has to be used while unsubscribing.
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_COR_ERR_NULL_PTR changedObj is a NULL pointer.
 *  \retval CL_COR_NOTIFY_ERR_INVALID_OP The operation specified for the subscription is not valid.
 *  \retval CL_EVENT_ERR_BAD_HANDLE channelhandle is zero.
 *  This error code is generated by Event Service, therefore the 
 *  return value contains the Event Service Component Identifier.
 *
 *  \par Description:
 *   This API is used to subscribe for CREATE, SET, and DELETE operations on a particular
 *   MO, specified by changedObject. When these operations are performed on the MO, the
 *   notification callback of the subscriber is invoked. The event handle is passed as the
 *   parameter to the notification callback. The clCorEventHandleToCorTxnIdGet() API can be
 *   used to retrieve the transaction-ID specific to COR. This transaction-ID can be used to
 *   obtain information about the jobs by using the clCorTxnJobMoIdGet(),
 *   clCorTxnJobSetParamsGet(), clCorTxnJobOperationGet() and clCorTxnJobWalk() APIs.
 *
 *  \par Library File:
 *  ClCorClient
 *  
 *  \sa clCorEventUnsubscribe()
 *
 */

extern ClRcT clCorEventSubscribe(ClEventChannelHandleT channelHandle,
                                ClCorMOIdPtrT      changedObj,
                                ClCorAttrPathPtrT  pAttrPath,
                                ClCorAttrListPtrT attrList,
                                ClCorOpsT    flags,
                                void * cookie,
                                ClEventSubscriptionIdT subscriptionId);
/**
 **********************************************
 *  \brief Unsubscribes for attribute change notification.
 *
 *  \par Header File:
 *  clCorNotifyApi.h
 *
 *  \param channelHandle (in) Channel handle obtained when the COR channel was opened
 *          by the application.
 *  \param subscriptionId (in) Subscription ID obtained from COR while subscribing.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_EVENT_ERR_INIT_NOT_DONE Event library is not initialized.
 *  \retval CL_EVENT_ERR_BAD_HANDLE The handle is invalid.
 *  \retval CL_EVENT_INTERNAL_ERROR An unexpected problem occurred with the Event Manager.
 *  \retval CL_EVENT_ERR_INVALID_PARAM An invalid parameter has been passed to the
 *              function. A parameter is not set correctly.
 *  \retval CL_EVENT_ERR_NO_MEM Memory allocation failure.
 *  \par
 *  Error codes listed above are generated by Event Service, therefore the 
 *  return value contains the Event Service Component Identifier.
 *
 *  \par Description:
 *  This API is used to unsubscribe from the attribute change notification. The subscription ID
 *  obtained while subscribing for the event must be used to unsubscribe the event.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorEventSubscribe()
 *
 */

extern ClRcT clCorEventUnsubscribe(ClEventChannelHandleT channelHandle, ClEventSubscriptionIdT subscriptionId);

/**
 **********************************************
 *  \brief
 *
 *  \par Header File:
 *  clCorNotifyApi.h
 *
 *  \param evtH (in) Handle to the event.
 *  \param size (in) Size of the event data.
 *  \param corTxnId (out) COR transaction Id extracted from the event.
 *
 *  \retval CL_OK The API is successfully executed.
 *  \retval CL_COR_ERR_INVALID_PARAM Invalid parameter passed.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *  \retval CL_COR_TXN_ERR_ZERO_JOBS No jobs found in the transaction.
 *
 *  \par Description:
 *  This function is used to GET the transaction ID from the event handle that is passed as a
 *  parameter to the Event Deliver Callback function. The user can subscribe for the changes
 *  on a MO using the function clCorEventSubscribe(). The Event Deliver Callback
 *  function is called when a change occurs on that MO. The transaction ID extracted from the
 *  event handle can be used to retrieve the information about the jobs in that transaction using
 *  the clCorTxnJobMoIdGet() / clCorTxnJobSetParamsGet() /
 *  clCorTxnJobOperationGet() functions
 * 
 *  \par Library File:
 *  ClCorClient
 *
 *  \par Note
 *  This function allocates memory for corTxnId when it is successfully executed. You must use
 *  the function clCorTxnIdTxnFree() with corTxnId as the parameter to free this memory.
 * 
 *  \sa clCorEventSubscribe(), clCorTxnIdTxnFree(), clCorTxnJobMoIdGet(),
 *  clCorTxnJobSetParamsGet(), clCorTxnJobOperationGet(), clCorTxnJobWalk()
 *
 */

extern ClRcT
clCorEventHandleToCorTxnIdGet(ClEventHandleT evtH, ClSizeT size,  ClCorTxnIdT *corTxnId);

#ifdef __cplusplus
}
#endif

#endif  /*  _CL_COR_NOTIFY_API_H_ */

/** \} */
