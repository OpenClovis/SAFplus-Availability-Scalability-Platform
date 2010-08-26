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
 * ModuleName  : amf
 * File        : clAmsSAClientApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains AMS client API related definitions.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_SA_CLIENT_API_H_
#define _CL_AMS_SA_CLIENT_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clAmsTypes.h>

/******************************************************************************
 * Client API functions.
 * -- The following are functions invoked by AMS. 
 *****************************************************************************/

/*
 * ClAmsSACSISetCallbackT
 * ----------------------
 * AMS calls this function to assign a CSI to a component.
 *
 * @param
 *   invocation                 - back reference to AMS
 *   compName                   - Name of component
 *   haState                    - HA state being assigned
 *   csiDescriptor              - CSIs to which this fn is targetted
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSACSISetCallbackT)(
        CL_IN       ClInvocationT               invocation,
        CL_IN       const ClNameT               *compName,
        CL_IN       ClAmsHAStateT               haState,
        CL_IN       ClAmsCSIDescriptorT         csiDescriptor);

/*
 * ClAmsSACSIRemoveCallbackT
 * -------------------------
 * AMS calls this function to remove a CSI assignment.
 *
 * @param
 *   invocation                 - back reference to AMS
 *   compName                   - Name of component
 *   csiName                    - Name of CSI
 *   csiFlags                   - operation
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSACSIRemoveCallbackT)(
        CL_IN   ClInvocationT                   invocation,
        CL_IN   const ClNameT                   *compName,
        CL_IN   const ClNameT                   *csiName,
        CL_IN   ClAmsCSIFlagsT                  csiFlags);

/*
 * ClAmsSAPGTrackCallbackT
 * -----------------------
 * AMS calls this function to let a component know changes
 * affecting a protection group. The component must have
 * previously requested tracking.
 *
 * @param
 *   csiName                    - Name of CSI
 *   notificationBuffer         - Where to put results
 *   numMembers                 - Mumber of members in a PG
 *   error                      - status of change
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSAPGTrackCallbackT)(
        CL_IN       const ClNameT               *csiName,
        CL_IN       ClAmsPGNotificationBufferT  notificationBuffer,
        CL_IN       ClUint32T                   numMembers,
        CL_IN       ClRcT                       error);

/*
 * ClAmsSACompHealthcheckCallbackT
 * -------------------------------
 * AMS calls this function to do a healthcheck on a component.
 *
 * @param
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSACompHealthcheckCallbackT)(
        CL_IN       ClInvocationT               invocation,
        CL_IN       const ClNameT               *compName,
        CL_IN       ClAmsCompHealthcheckKeyT    *healthcheckkey);

/*
 * ClAmsSACompTerminateCallbackT
 * -----------------------------
 * AMS calls this function to terminate a component
 *
 * @param
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSACompTerminateCallbackT)(
        CL_IN       ClInvocationT               invocation,
        CL_IN       const ClNameT               *compName);

/*
 * ClAmsSAProxiedCompInstantiateCallbackT
 * --------------------------------------
 * AMS calls this function to instantiate a proxied component.
 * This fn is called for the proxy component.
 *
 * @param
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSAProxiedCompInstantiateCallbackT)(
        CL_IN       ClInvocationT               invocation,
        CL_IN       const ClNameT               *proxiedCompName);

/*
 * ClAmsSAProxiedCompCleanupCallbackT
 * ----------------------------------
 * AMS calls this function to abruptly terminate a proxied component.
 * This fn is called for the proxy component.
 *
 * @param
 *
 * @returns
 *   Nothing
 */

typedef void (*ClAmsSAProxiedCompCleanupCallbackT)(
        CL_IN       ClInvocationT               invocation,
        CL_IN       const ClNameT               *proxiedCompName);
 
typedef struct
{
    ClAmsSACompHealthcheckCallbackT           healthcheckCallback;
    ClAmsSACompTerminateCallbackT             compTerminateCallback;
    ClAmsSACSISetCallbackT                    csiSetCallback;
    ClAmsSACSIRemoveCallbackT                 csiRemoveCallback;
    ClAmsSAPGTrackCallbackT                   pgTrackCallback;
    ClAmsSAProxiedCompInstantiateCallbackT    proxiedCompInstantiateCallback;
    ClAmsSAProxiedCompCleanupCallbackT        proxiedCompCleanupCallback;
} ClAmsSAClientCallbacksT;

/******************************************************************************
 * SA API functions invoked by client
 *****************************************************************************/

/*
 * ----- SA Library LifeCycle Management Functions Invoked by Client -----
 */

/*
 * clAmsSAInitialize
 * -----------------
 * Initialize/Start the use of the AMS SA API library.
 *
 * @param
 *   amsHandle                  - Handle returned by AMS to API user
 *   amsCallbacks               - Callbacks into API user
 *   version                    - Versions supported by AMS and user
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_VERSION_MISMATCH    - Error: Version not supported by AMS
 *
 */

extern ClRcT clAmsSAInitialize(
        CL_OUT      ClAmsClientHandleT            *amsHandle,
        CL_IN       const ClAmsSAClientCallbacksT *amsClientCallbacks,
        CL_INOUT    ClVersionT                    *version);

/*
 * clAmsSAFinalize
 * ---------------
 * Finalize/terminate the use of the AMS client API library.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSAFinalize(
        CL_IN       ClAmsClientHandleT          amsHandle);

/*
 * clAmsSASelectionObjectGet
 * -------------------------
 * Get the OS handle associated with amsHandle. This fn may not be
 * supported by ASP.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSASelectionObjectGet(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_OUT      ClSelectionObjectT          *selectionObject);

/*
 * clAmsSADispatch
 * ---------------
 * A client can use this fn to invoke pending callbacks. This fn
 * may not be supported by ASP.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

//extern ClRcT clAmsSADispatch(
 //       CL_IN       ClAmsClientHandleT          amsHandle,
  //      CL_IN       ClDispatchFlagsT            dispatchFlags);

/*
 * ----- SA Functions Invoked by Client -----
 */

/*
 * clAmsSAComponentRegister
 * ------------------------
 * Register a component.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   compName                   - Name of component being registered
 *   proxyCompName              - Name of proxy registering the component
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSACompRegister(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       const ClNameT               *compName,
        CL_IN       const ClNameT               *proxyCompName);

/*
 * clAmsSAComponentUnregister
 * --------------------------
 * Unregister a component.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   compName                   - Name of component being unregistered
 *   proxyCompName              - Name of proxy unregistering the component
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSACompUnregister(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       const ClNameT               *compName,
        CL_IN       const ClNameT               *proxyCompName);

/*
 * clAmsSAComponentNameGet
 * -----------------------
 * Get the name of a component
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   compName                   - Name of component invoking this fn
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSACompNameGet(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_OUT      ClNameT               *compName);

/*
 * clAmsSACSIHAStateGet
 * --------------------
 * Get HA state associated with a CSI
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   compName                   - Name of component
 *   csiName                    - Name of CSI
 *   haState                    - ha state
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSACSIHAStateGet(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       const ClNameT               *compName,
        CL_IN       const ClNameT               *csiName,
        CL_OUT      ClAmsHAStateT               *haState);

/*
 * clAmsSACSIQuiescingComplete
 * ---------------------------
 * Inform AMS that client has stopped processing a workload.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   invocation                 - back reference to AMS for
 *                                prior operation
 *   error                      - status of operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSACSIQuiescingComplete(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       ClInvocationT               invocation,
        CL_IN       ClRcT                       error);

/*
 * clAmsSAPGTrack
 * --------------
 * Request tracking from AMS for status of a PG.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   csiName                    - Name of CSI / PG
 *   trackFlags                 - Type of tracking desired
 *   notificationBuffer         - Where to put results
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSAPGTrack(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       ClNameT                     *csiName,
        CL_IN       ClUint8T                    trackFlags,
        CL_IN       ClAmsPGNotificationBufferT  *notificationBuffer);

/*
 * clAmsSAPGTrackStop
 * ------------------
 * Indicate tracking is no longer required for PG.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   csiName                    - Name of CSI / PG
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSAPGTrackStop(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       const ClNameT               *csiName);

/*
 * clAmsSAResponse
 * ---------------
 * Send a delayed reponse from client to AMS about a prior
 * operation.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   invocation                 - back reference to AMS for
 *                                prior operation
 *   error                      - status of operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

extern ClRcT clAmsSAResponse(
        CL_IN       ClAmsClientHandleT          amsHandle,
        CL_IN       ClInvocationT               invocation,
        CL_IN       ClRcT                       error);

/*
 * TODO
 *
 * Functions currently not present in this file
 *
 * clAmsSAPmStart
 * clAmsSAPmStop
 * clAmsSAHealthcheckStart
 * clAmsSAHealthcheckStop
 * clAmsSAHealthcheckConfirm
 */

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_SA_CLIENT_API_H_ */
