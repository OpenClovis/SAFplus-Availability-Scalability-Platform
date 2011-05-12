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
 * File        : clAmsSAServerApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the client side implementation of the AMS client 
 * API. The client API is used by a client client to control the
 * AMS entity.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/


#include <string.h>
#include <clHandleApi.h>

#include <clAmsErrors.h>
#include <clAmsEventServerApi.h>
#include <clAms.h>
#include <clAmsServerUtils.h>
#include <clAmsSAServerApi.h>
#include <clAmsPolicyEngine.h>
#include <clAmsModify.h>
#include <clAmsDBPackUnpack.h>
#include <clAmsCkpt.h>
#include <clCpmClient.h>
#include <clCpmAms.h>
#include <clCpmCommon.h>
#include <clCpmInternal.h>
#include <clCmApi.h>
#include <clFaultApi.h>
#include <clAmsNotifications.h>

/* 
 * Global instance of ams 
 */
extern ClAmsT gAms;

/*
 * Pointer to AMS to CPM callbacks
 */
extern ClCpmAmsToCpmCallT *gAmsToCpmCallbackFuncs;
extern ClRcT clFaultRepairNotification(ClAmsNotificationDescriptorT *notification,
                                ClIocAddressT iocAddress,
                                ClAlarmHandleT alarmHandle,
                                ClUint32T recovery);

/*
 * _clAmsSACSIHAStateGet 
 * ----------------
 * return HA state of a component for a given CSI
 *
 * @param
 *  compName                   - Name of the component whose HA state is is
                                 requested 
 *  csiName                    - Name of the CSI
 *  haState                    - HA state of the component for the CSI 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSACSIHAStateGet( 
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *csiName,
        CL_OUT  ClAmsHAStateT  *haState )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  compRef = {{0},0,0};
    ClAmsEntityRefT  csiRef = {{0},0,0};
    ClAmsCompCSIRefT  *foundCSIRef = NULL;
    ClAmsCompT  *comp = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR(!compName || !csiName || !haState);

    /*
     * Find the component in csi's comp list and return the HA state
     */

    *haState = CL_AMS_HA_STATE_NONE;
    memcpy (&compRef.entity.name,compName,sizeof (ClNameT) );
    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    memcpy (&csiRef.entity.name,csiName,sizeof (ClNameT) );
    csiRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;
    compRef.ptr = NULL;
    csiRef.ptr  = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[compRef.entity.type],
                &compRef),
            gAms.mutex );

    comp = ( ClAmsCompT *) compRef.ptr;

    AMS_CHECKPTR_AND_UNLOCK (!comp,gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsEntityListFindEntityRef(
                &comp->status.csiList,
                &csiRef,
                0,
                (ClAmsEntityRefT **)&foundCSIRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !foundCSIRef, gAms.mutex );

    *haState =  foundCSIRef->haState;

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC (rc);
}

/*
 * _clAmsSAPGTrackAdd 
 * ----------------
 * Add a client in the protection group list of a CSI
 *
 * @param
 *   iocAddress                 - ioc address of the client 
 *   cpmHandle                  - cpm initialization handle 
                                  to invoke the client callback
 *   csiName                    - name of the csi which contains the PG client 
 *   trackFlags                 - tracking flags of the client
 *   notificationBuffer         - Buffer in which the response is sent back
                                  if the track flags is CL_AMS_PG_TRACK_CURRENT
                                  and this buffer is non null
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAPGTrackAdd( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  ClNameT  *csiName,
        CL_IN  ClUint8T  trackFlags,
        CL_INOUT  ClAmsPGNotificationBufferT  *notificationBuffer)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityT  entity = {0};
    ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !csiName );

    AMS_CHECK_BAD_CLNAME ((*csiName));

    /*
     * Add the client in the csi's pgTrackList 
     */

    pgTrackClient = clHeapAllocate (sizeof (ClAmsCSIPGTrackClientT));

    AMS_CHECK_NO_MEMORY (pgTrackClient);

    pgTrackClient->address = iocAddress;
    pgTrackClient->trackFlags = trackFlags;
    pgTrackClient->cpmHandle = cpmHandle;

    entity.type = CL_AMS_ENTITY_TYPE_CSI;
    memcpy ( &entity.name, csiName, sizeof (ClNameT) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsCSIAddToPGTrackList(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                &entity,
                pgTrackClient,
                notificationBuffer),
            gAms.mutex );

    if ( trackFlags == CL_AMS_PG_TRACK_CURRENT )
    {
        clAmsFreeMemory (pgTrackClient);
    }
    else
    {
        AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_DB));
    }

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

    return CL_OK;

exitfn:

    clAmsFreeMemory(pgTrackClient);
    return CL_AMS_RC(rc);

}

/*
 * _clAmsSAPGTrackStop 
 * ----------------
 * Remove the protection group client from the PG list 
 *
 * @param
 *   iocAddress                 - ioc address of the client 
 *   cpmHandle                  - cpm initialization handle 
                                  to invoke the client callback
 *   csiName                    - name of the csi which contains the PG client 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAPGTrackStop( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  ClNameT  *csiName)
{

    ClRcT  rc = CL_OK;
    ClAmsCSIPGTrackClientT  pgTrackClient;
    ClAmsEntityT  entity = {0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !csiName );

    pgTrackClient.address = iocAddress;
    pgTrackClient.cpmHandle = cpmHandle;

    entity.type = CL_AMS_ENTITY_TYPE_CSI;
    memcpy ( &entity.name, csiName, sizeof (ClNameT) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsCSIDeleteFromPGTrackList(
                gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                entity,
                &pgTrackClient),
            gAms.mutex );

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_DB));

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC(rc);

}

/*
 * _clAmsSAPGTrackDispatch
 * ----------------
 * AMS to CPM callout to dispatch the response to pg clients interested in the
 * protection group of the CSI

 @param
 * iocAddress               - ioc address of the pg track client
 * cpmHandle                - cpm library handle
 *  csi                     - csi whose PG has changed 
 * buffer                   - protection group notification buffer
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAPGTrackDispatch( 
        CL_IN  ClIocAddressT  iocAddress,
        CL_IN  ClCpmHandleT  cpmHandle,
        CL_IN  ClNameT  *csiName,
        CL_IN  ClAmsPGNotificationBufferT  *buffer )
{
    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!csiName || !buffer);

    rc =  (*gAmsToCpmCallbackFuncs->compPGTrack)(
            iocAddress,
            cpmHandle,
            *csiName,
            buffer,
            buffer->numItems,
            CL_TRUE);

    if ( rc != CL_OK )
    {
        AMS_LOG( CL_DEBUG_ERROR,(" PG Track Dispatch Failed On Node-Address[%d] "
                    "PortID [%d] for CSI [%s] with Error [%x]", 
                    iocAddress.iocPhyAddress.nodeAddress,iocAddress.iocPhyAddress.portId,
                    csiName->value,rc) );
    }

    /*
     * PG Track dispach is part of some CSI operation performed by Policy Engine
     * Failure to dispach does not mean failure of CSI operation. So incase of 
     * failure to dispatch PG Track response we will flag a warning and return 
     * CL_OK to the Calling PE function.
     */

    return CL_OK;

}

/*
 * _clAmsSAMarshalPGTrackDispatch
 * ----------------
 * This function marshals the pg track response to be sent to pg clients
 *
 * @param
 *  csi                     - csi whose PG has changed 
 *  comp                    - component name whose HA state has changed
 *  pgChange                - change in the protection group
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAMarshalPGTrackDispatch( 
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClAmsCompT  *comp, 
        CL_IN  ClAmsPGChangeT  pgChange)

{
    ClRcT  rc = CL_OK;
    ClAmsPGNotificationBufferT  *fullNotificationBuffer = NULL;
    ClAmsPGNotificationBufferT  *changeNotificationBuffer = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !csi || !comp );

    AMS_CALL( clAmsCSIMarshalPGTrackNotificationBuffer(
                &fullNotificationBuffer,
                csi,
                comp,
                CL_AMS_PG_TRACK_CHANGES,
                pgChange) );

    AMS_CHECK_RC_ERROR ( clAmsCSIMarshalPGTrackNotificationBuffer(
                &changeNotificationBuffer,
                csi,
                comp,
                CL_AMS_PG_TRACK_CHANGES_ONLY,
                pgChange) );

    AMS_CHECK_RC_ERROR ( clAmsCSIDispatchPGTrackResponse(
                csi,
                fullNotificationBuffer,
                changeNotificationBuffer) );

exitfn:

    if ( fullNotificationBuffer)
    {
        clAmsFreeMemory (fullNotificationBuffer->notification);
    }

    if ( changeNotificationBuffer)
    {
        clAmsFreeMemory (changeNotificationBuffer->notification);
    }

    clAmsFreeMemory (fullNotificationBuffer);
    clAmsFreeMemory (changeNotificationBuffer);

    return CL_AMS_RC (rc);

}

/*
 * _clAmsSACSIQuiescingComplete
 * ----------------
 * CPM callback to inform AMS about CSI quiescing completion
 *
 * @param
 *  invocation                 - invocation for csi quiescing request 
 *  retCode                    - return code from the client to inform about
                                 success / failure of the csi operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSACSIQuiescingComplete( 
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClRcT  retCode )
{

    ClRcT  rc = CL_OK;
    ClAmsCompT  *comp = NULL;
    ClAmsInvocationT data = {0};
    ClAmsInvocationT *invocationData = NULL;
    ClAmsEntityRefT  compRef = { {0},0,0};
    ClUint32T  callbackType = 0;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECK_SERVICE_STATE_LOCKED(gAms.serviceState);

    clAmsInvocationLock();
    clOsalMutexLock(gpClCpm->invocationMutex);
    if ( ( rc = cpmInvocationGetWithLock(
                                         invocation,
                                         &callbackType,
                                         (void **)&invocationData))
         != CL_OK )
    {
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clAmsInvocationUnlock();

        invocationData = &data;

#ifdef POST_RC2

        AMS_CALL ( clAmsCntFindInvocationElement(
                    gAms.invocationList,
                    invocationID,
                    invocationData) ); 
        
        if ( invocationData->cmd == CL_AMS_CSI_SET_CALLBACK )
        {
            callbackType = CL_CPM_CSI_SET_CALLBACK;
        }
        else if(invocationData->cmd == CL_AMS_CSI_QUIESCING_CALLBACK )
        {
            callbackType = CL_CPM_CSI_QUIESCING_CALLBACK;
        }
        else if ( invocationData->cmd == CL_AMS_CSI_RMV_CALLBACK )
        {
            callbackType = CL_CPM_CSI_RMV_CALLBACK;
        }
        else
        {
            /*
             * Should never come here
             */
            return CL_AMS_ERR_INVALID_OPERATION;
        }

        AMS_LOG (CL_DEBUG_TRACE,
                ("AMS Restart Mode: Invocation found in the previous "
                 "AMS instance invocation list \n")); 
#else
        return rc;

#endif

    }
    else
    {
        memcpy(&data, invocationData, sizeof(data));
        invocationData = &data;
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clAmsInvocationUnlock();
    }

    AMS_CHECKPTR (!invocationData);

    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    memcpy ( &compRef.entity.name, &invocationData->compName, sizeof (ClNameT));

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    /*
     * Check again as we dropped the lock.
     */

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &compRef),
            gAms.mutex );

    comp = (ClAmsCompT *)compRef.ptr;

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( 
            clAmsPeCompQuiescingCompleteCallback(
                comp,
                invocation,
                retCode,
                CL_AMS_ENTITY_SWITCHOVER_GRACEFUL),
            gAms.mutex );

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC(rc);
}


static ClRcT
_clAmsSACSISetWithCkpt(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsHAStateT  haState,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor,
        CL_IN  ClBoolT  doCkpt)
{
    ClRcT  rc = CL_OK;
    ClIocPhysicalAddressT  srcPhyAddr = {0};
    ClCharT  *nodeName = NULL;
    ClNameT *compRef = compName;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !compName || !proxyCompName );

    if(compName != proxyCompName)
        compRef = proxyCompName;

    AMS_CALL( _clAmsSAMarshalRmdParams(
                compRef,
                &nodeName,
                &srcPhyAddr) );

    AMS_CHECKPTR(!nodeName);

    rc =  (*gAmsToCpmCallbackFuncs->compCSISet)(
                compName->value,
                proxyCompName->value,
                nodeName,
                invocation,
                haState,
                csiDescriptor);

    if(doCkpt == CL_TRUE)
    {
        AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));
    }

    clAmsFreeMemory(nodeName);
    return CL_AMS_RC (rc);

}

ClRcT
_clAmsSACSISetNoCkpt(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsHAStateT  haState,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor )
{
    return _clAmsSACSISetWithCkpt(compName,
                                  proxyCompName,
                                  invocation,
                                  haState,
                                  csiDescriptor,
                                  CL_FALSE);
}


/*
 * _clAmsSACSISet
 * ----------------
 * This function requests CPM to assign HA state to a component for a given CSI
 * on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                assigned the CSI 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 * invocation                 - invocation ID for the CSI set operation
 * haState                    - ha state assigned to the component 
 * csiDescriptor              - csi descriptor for the CSI
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSACSISet(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        CL_IN  ClAmsHAStateT  haState,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor )
{
    return _clAmsSACSISetWithCkpt(compName,
                                  proxyCompName,
                                  invocation,
                                  haState,
                                  csiDescriptor,
                                  CL_TRUE);
}

/*
 * _clAmsSACSIRemove
 * ----------------
 * This function requests CPM to remove CSI assignment from the component
 * on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                assigned the CSI 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 * invocation                 - invocation ID for the CSI set operation
 * csiDescriptor              - csi descriptor for the CSI
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSACSIRemove(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName,
        CL_IN  ClInvocationT  invocation,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSIDescriptorT  csiDescriptor )

{

    ClRcT  rc = CL_OK;
    ClCharT  *nodeName = NULL;
    ClIocPhysicalAddressT  srcPhyAddr = {0};
    ClNameT *compRef = compName;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !compName || !proxyCompName );

    if(compName != proxyCompName)
        compRef = proxyCompName;

    AMS_CALL( _clAmsSAMarshalRmdParams(
                compRef,
                &nodeName,
                &srcPhyAddr) );

    AMS_CHECKPTR ( !nodeName );

    rc =  (*gAmsToCpmCallbackFuncs->compCSIRmv)(
                compName->value,
                proxyCompName->value,
                nodeName,
                invocation, 
                &csiDescriptor.csiName,
                csiDescriptor.csiFlags);

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    clAmsFreeMemory(nodeName);
    return CL_AMS_RC (rc);

}


/*
 * _clAmsSACSIOperationResponse
 * ----------------
 * CPM callback to inform AMS about CSI operation completion
 *
 * @param
 *  invocation                 - invocation for csi operation request 
 *  retCode                    - return code from the client to inform about
                                 success / failure of the csi operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
_clAmsSACSIOperationResponse(
        CL_IN  ClInvocationT  invocationID,
        CL_IN  ClRcT  retCode )
{

    ClRcT  rc = CL_OK;
    ClUint32T  callbackType = 0;
    ClAmsEntityRefT  compRef = { {0},0,0};
    ClAmsInvocationT data = {0};
    ClAmsInvocationT  *invocationData = NULL;
    ClAmsCompT  *comp = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    clAmsInvocationLock();
    clOsalMutexLock(gpClCpm->invocationMutex);
    if ( ( rc = cpmInvocationGetWithLock(
                                         invocationID,
                                         &callbackType,
                                         (void **)&invocationData))
         != CL_OK )
    {
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clAmsInvocationUnlock();
        invocationData = &data;

#ifdef POST_RC2

        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCntFindInvocationElement(
                                                                            gAms.invocationList,
                                                                            invocationID,
                                                                            invocationData),
                                              gAms.mutex); 
        
        if ( invocationData->cmd == CL_AMS_CSI_SET_CALLBACK )
        {
            callbackType = CL_CPM_CSI_SET_CALLBACK;
        }
        else if ( invocationData->cmd == CL_AMS_CSI_RMV_CALLBACK )
        {
            callbackType = CL_CPM_CSI_RMV_CALLBACK;
        }
        else if (invocationData->cmd == CL_AMS_CSI_QUIESCING_CALLBACK)
        {
            callbackType = CL_CPM_CSI_QUIESCING_CALLBACK;
        }
        else
        {
            /*
             * Should never come here
             */

    	    AMS_CALL(clOsalMutexUnlock(gAms.mutex));
            return CL_AMS_ERR_INVALID_OPERATION;
        }

        AMS_LOG (CL_DEBUG_TRACE,
                ("AMS Restart Mode: Invocation found in the previous "
                 "AMS instance invocation list \n")); 
#else
	
    	AMS_CALL(clOsalMutexUnlock(gAms.mutex));
        return rc;

#endif


    }
    else
    {
        memcpy(&data, invocationData, sizeof(data));
        invocationData = &data;
        clOsalMutexUnlock(gpClCpm->invocationMutex);
        clAmsInvocationUnlock();
    }
    
    AMS_CHECKPTR ( !invocationData );

    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    memcpy ( &compRef.entity.name, &invocationData->compName, sizeof (ClNameT));

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &compRef),
            gAms.mutex );

    comp = (ClAmsCompT *)compRef.ptr;

    switch (callbackType)
    {

        case CL_CPM_CSI_SET_CALLBACK:
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                        clAmsPeCompAssignCSICallback(
                            comp,
                            invocationID,
                            retCode,
                            CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE),
                        gAms.mutex );
            }

            break;

        case CL_CPM_CSI_RMV_CALLBACK:
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                        clAmsPeCompRemoveCSICallback(
                            comp,
                            invocationID,
                            retCode,
                            CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE),
                        gAms.mutex );

                break;
            }

        case CL_CPM_CSI_QUIESCING_CALLBACK:
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                         clAmsPeCompQuiescingCSICallback(
                             comp,
                             invocationID,
                             retCode,
                             CL_AMS_ENTITY_SWITCHOVER_GRACEFUL),
                         gAms.mutex );
            }
            break;

        default:
            { 
                AMS_LOG ( CL_DEBUG_ERROR, ("invalid callback function \n"));
                AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
            }
    }

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));
	

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAComponentErrorReport
 * ----------------
 * CPM callback to inform AMS about error reported for a component 
 *
 * @param
 *  compName                   - Component name on which error in reported 
 *  errorDetectionTime         - time stamp of error detection
 *  recommendedRecovery        - recovery action recommended by the reportee
 *  alarmHandle                - alram handle returned by alarm raise for this
                                 error. This is used by fault manager / alarm
                                 manager to get the information about the error                                  / fault when AMS informs it about the recovery
                                 action taken.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAComponentErrorReport (
        CL_IN  const ClNameT  *compName,
        CL_IN  ClTimeT  errorDetectionTime,
        CL_IN  ClAmsLocalRecoveryT  recommendedRecovery,
        CL_IN  ClUint32T  alarmHandle,
        CL_IN  ClUint64T instantiateCookie)
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL ( clAmsUpdateAlarmHandle(compName,alarmHandle) );

    AMS_CALL ( clAmsGetFaultReport( compName, recommendedRecovery, instantiateCookie) );

    return CL_OK;
}

/*
 * _clAmsSAFaultReportCallback
 * ----------------
 * This function marshals the response to fault manager 
 *
 * @param
 *  entity                  - entity on which the fault was reported 
 *  recovery                - recovery action undertaken by AMF
 *  repairNecessary         - indication from AMS whether repair is necessary
 *
 * @returns
 *   CL_OK                  - Operation successful
 *
 */
ClRcT
_clAmsSAFaultReportCallback(
                            CL_IN  ClAmsEntityT  *entity,
                            CL_IN  ClAmsRecoveryT  recovery,
                            CL_IN  ClUint32T  repairNecessary)
{

    ClUint32T  alarmHandle = -1;
    ClIocAddressT  iocAddress; 
    ClAmsCompT  *comp = NULL;
    ClAmsSUT  *su = NULL;
    ClAmsNodeT  *node = NULL;
    ClAmsNotificationDescriptorT notification = {0};
    ClNameT faultyCompName = {0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entity );

    if ( entity->type == CL_AMS_ENTITY_TYPE_COMP )
    {
        AMS_CHECK_COMP ( comp = (ClAmsCompT *) entity );
        AMS_CHECK_SU   ( su   = (ClAmsSUT   *) comp->config.parentSU.ptr );
        AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );
        memcpy(&faultyCompName, &entity->name, sizeof(entity->name));
        alarmHandle = comp->status.alarmHandle;
    }
    else if ( entity->type == CL_AMS_ENTITY_TYPE_NODE )
    {
        ClAmsEntityRefT *entityRef = NULL;
        
        AMS_CHECK_NODE ( node = (ClAmsNodeT *) entity );

        /*
         * For NON-asp aware nodes, get the faulty comp name. of the proxied
         */
        if(!node->config.isASPAware)
        {
            for(entityRef = clAmsEntityListGetFirst(&node->config.suList);
                entityRef != NULL;
                entityRef = clAmsEntityListGetNext(&node->config.suList, entityRef))
            {
                ClAmsEntityRefT *compRef = NULL;
                ClAmsSUT *su = (ClAmsSUT*)entityRef->ptr;
                for(compRef = clAmsEntityListGetFirst(&su->config.compList);
                    compRef != NULL;
                    compRef = clAmsEntityListGetNext(&su->config.compList, compRef))
                {
                    ClAmsCompT *comp = (ClAmsCompT*)compRef->ptr;
                    if(comp->status.recovery)
                    {
                        memcpy(&faultyCompName, &comp->config.entity.name, 
                               sizeof(comp->config.entity.name));
                        goto update_alarm_handle;
                    }
                }
            }
        }
        memcpy(&faultyCompName, &entity->name, sizeof(entity->name));        

        update_alarm_handle:
        alarmHandle = node->status.alarmHandle;
    }
    else
    {
        return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
    }


    /*
     * Send notification about the fault
     */

    notification.type = CL_AMS_NOTIFICATION_FAULT;
    notification.entityType = entity->type;
    memcpy ( &notification.entityName,&entity->name,sizeof(notification.entityName) );
    memcpy (&notification.faultyCompName, &faultyCompName, sizeof(notification.faultyCompName));
    notification.entityName.length = strlen(entity->name.value);
    notification.recoveryActionTaken = recovery;
    notification.repairNecessary = repairNecessary;

    AMS_CALL_PUBLISH_NTF ( clAmsNotificationEventPublish( &notification ) );

    /*
     * XXX: Need to indicate to FM if AMF thinks repair is necessary
     * this is necessary for autorepair actions. currently API does
     * not allow that
     */

    AMS_CALL ( (*gAmsToCpmCallbackFuncs->cpmIocAddressForNodeGet)(
                                                                  &node->config.entity.name,
                                                                  &iocAddress) );

#if 0
    ClRcT rc = CL_OK;
    clOsalMutexUnlock(gAms.mutex);
    rc = clFaultRepairNotification(&notification, iocAddress, alarmHandle, recovery);
    clOsalMutexLock(gAms.mutex);
    return rc;
#else
    return clFaultRepairAction(iocAddress, alarmHandle, recovery);
#endif
}

/*
 * _clAmsSANodeFailFast
 * ----------------
 * This is AMS callout to inform CPM that fail fast / reboot operation should be
 * performed on the faulty node
 * @param
 *  nodeName                  - Name of the node which needs to be rebooted 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT _clAmsSANodeFailFast(
                           CL_IN  ClNameT  *nodeName,
                           CL_IN  ClBoolT  isASPAware)
{

    AMS_FUNC_ENTER (("\n"));

    clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL);

    AMS_CALL ( (*gAmsToCpmCallbackFuncs->cpmNodeFailFast) (nodeName, isASPAware) );

    return CL_OK;
}

/*
 * _clAmsSANodeFailOver
 * ----------------
 * This is AMS callout to inform CPM that node failover has occured 

 * @param
 *  nodeName                  - Name of the node which needs to be rebooted 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT _clAmsSANodeFailOver(
                           CL_IN   ClNameT *nodeName,
                           CL_IN   ClBoolT isASPAware)
{
    ClRcT   rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL);

    if ( (rc = (*gAmsToCpmCallbackFuncs->cpmNodeFailOver)(
                                                          nodeName, isASPAware) )
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("function %s returned with error %x \n",__FUNCTION__,rc));
        return CL_AMS_RC (rc);
    }
    return CL_OK;
}


/*
 * _clAmsSANodeFailOverRestart
 * ----------------
 * This is AMS callout to inform CPM that node failover has occured 
 * and also checks if the node has to be restarted after failover.
 *
 * @param
 *  nodeName                  - Name of the node which needs to be rebooted 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT _clAmsSANodeFailOverRestart(
                                  CL_IN   ClNameT *nodeName,
                                  CL_IN   ClBoolT isASPAware)
{
    ClRcT   rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL);

    if ( (rc = (*gAmsToCpmCallbackFuncs->cpmNodeFailOverRestart)(
                                                                 nodeName, isASPAware) )
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("function %s returned with error %x \n",__FUNCTION__,rc));
        return CL_AMS_RC (rc);
    }
    return CL_OK;
}


/*
 * _clAmsSAComponentInstantiate
 * ----------------
 * This function requests CPM to instantiate a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                instantiated
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAComponentInstantiate(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName,
        CL_IN  ClUint64T instantiateCookie )
{

    ClRcT  rc = CL_OK;
    ClIocPhysicalAddressT  srcPhyAddr = {0};
    ClCharT  *nodeName = NULL;
    ClNameT  *compRef = compName;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !compName || !proxyCompName );

    if(compName != proxyCompName)
        compRef = proxyCompName;

    AMS_CALL( _clAmsSAMarshalRmdParams(
                compRef,
                &nodeName,
                &srcPhyAddr) );

    AMS_CHECKPTR (!nodeName);

    rc = (*gAmsToCpmCallbackFuncs->compInstantiate)(
                compName->value,
                proxyCompName->value,
                instantiateCookie,
                nodeName,
                &srcPhyAddr,
                CPM_CPML_CONFIRM);

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    clAmsFreeMemory(nodeName);
    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAComponentTerminate
 * ----------------
 * This function requests CPM to terminate a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                terminated 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAComponentTerminate(
        CL_IN  ClNameT  *compName,
        CL_IN  ClNameT  *proxyCompName )
{

    ClRcT  rc = CL_OK;
    ClIocPhysicalAddressT  srcPhyAddr = {0};
    ClCharT  *nodeName = NULL;
    ClNameT *compRef = compName;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !compName || !proxyCompName );

    if(compName != proxyCompName)
        compRef = proxyCompName;

    AMS_CALL(  _clAmsSAMarshalRmdParams(
                compRef,
                &nodeName,
                &srcPhyAddr) );

    AMS_CHECKPTR (!nodeName);

    rc =  (*gAmsToCpmCallbackFuncs->compTerminate)(
                compName->value,
                proxyCompName->value,
                nodeName,
                &srcPhyAddr,
                CPM_CPML_CONFIRM);

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    clAmsFreeMemory(nodeName);
    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAComponentCleanup
 * ----------------
 * This function requests CPM to cleanup a component on behalf of AMS 
 *
 * @param
 *  compName                  - Name of the component which needs to be 
                                cleaned 
 *  proxyCompName             - Name of the proxy component when compName is a 
                                proxied component this name will be same as the 
                                component name for the non proxied components
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
_clAmsSAComponentCleanup(
        CL_IN  ClNameT  *compName, 
        CL_IN  ClNameT  *proxyCompName )
{

    ClRcT  rc = CL_OK;
    ClIocPhysicalAddressT  srcPhyAddr = {0};
    ClCharT  *nodeName = NULL;
    ClNameT *compRef = compName;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!compName || !proxyCompName);

    if(compName != proxyCompName)
        compRef = proxyCompName;

    AMS_CALL( _clAmsSAMarshalRmdParams(
                compRef,
                &nodeName,
                &srcPhyAddr) );

    AMS_CHECKPTR (!nodeName);

    rc = (*gAmsToCpmCallbackFuncs->compCleanup)(
                compName->value,
                proxyCompName->value,
                nodeName,
                &srcPhyAddr,
                CPM_CPML_CONFIRM,
                CL_CPM_CLEANUP);

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    clAmsFreeMemory(nodeName);
    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAComponentOperationResponse
 * ----------------
 * CPM callback to inform AMS about Component operation completion
 *
 * @param
 *  compName                  - Name of the component on which the operation
                                was performed
 *  requestType               - type of operation performed on the component
 *  retCode                    - return code from the client to inform about
                                 success / failure of the component operation
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT 
_clAmsSAComponentOperationResponse(
        // coverity[pass_by_value]
        CL_IN  ClNameT  compName,
        CL_IN  ClCpmCompRequestTypeT  requestType,
        CL_IN  ClRcT  retCode)
{

    ClRcT  rc = CL_OK;
    ClAmsCompT  *comp = NULL;
    ClAmsEntityRefT  compRef = { {0},0,0};

    AMS_FUNC_ENTER (("\n"));

    if ( requestType == CL_CPM_CLEANUP ) 
    {
        return CL_OK;
    }

    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    memcpy ( &compRef.entity.name, &compName, sizeof (ClNameT));
    compRef.entity.name.length +=1;
   
    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &compRef),
            gAms.mutex );

    comp = (ClAmsCompT *)compRef.ptr;

    AMS_CHECKPTR_AND_UNLOCK ( !comp, gAms.mutex );

    switch ( requestType )
    {
        case CL_CPM_INSTANTIATE :
        case CL_CPM_PROXIED_INSTANTIATE:
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                        clAmsPeCompInstantiateCallback (
                            comp,
                            retCode),
                        gAms.mutex );

                break;

            }
        case CL_CPM_TERMINATE :
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                        clAmsPeCompTerminateCallback (
                            comp,
                            retCode),
                        gAms.mutex );

                break;

            }
            
        case CL_CPM_PROXIED_CLEANUP:
            {
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                        clAmsPeCompProxiedCompCleanupCallback(
                            comp,
                            retCode),
                        gAms.mutex );

                break;

            }

        default:
            {
                AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
            }
    }

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL));

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAEntityAdd
 * ----------------
 * This function requests CPM to add a component/node to its table.
 *
 * @param
 *  entity                      - Component/node which needs to be added
 *                                to CPM table.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT _clAmsSAEntityAdd(CL_IN ClAmsEntityRefT *entityRef)
{

    ClRcT rc = CL_OK;
    
    AMS_CHECKPTR (!entityRef);

    switch (entityRef->entity.type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            AMS_CHECK_RC_ERROR ( (*gAmsToCpmCallbackFuncs->
                                  cpmEntityAdd)(entityRef) );
            break;
        }
        default:
        {
            break;
        }
    }

    return CL_OK;
    
exitfn:
    return CL_AMS_RC (rc);
}

/*
 * _clAmsSAEntityRmv
 * ----------------
 * This function requests CPM to remove a component/node from its table.
 *
 * @param
 *  entity                      - Component/node which needs to be removed
 *                                from CPM table.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT _clAmsSAEntityRmv(CL_IN ClAmsEntityRefT *entityRef)
{

    ClRcT rc = CL_OK;
    
    AMS_CHECKPTR (!entityRef);

    switch (entityRef->entity.type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            AMS_CHECK_RC_ERROR ( (*gAmsToCpmCallbackFuncs->
                                  cpmEntityRmv)(entityRef) );
            break;
        }
        default:
        {
            break;
        }
    }

    return CL_OK;
    
exitfn:
    return CL_AMS_RC (rc);

}

/*
 * _clAmsSAEntitySetConfig
 * ----------------
 * This function requests CPM to set a component/node configuration
 * in its table.
 *
 * @param
 *  entityRef                     - Component/node for which configuration
 *                                needs to be set.
 *  bitMask                       - Bit mask specifying which attrs to set.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT _clAmsSAEntitySetConfig(CL_IN ClAmsEntityConfigT *entityConfig,
                              CL_IN ClUint64T bitMask)
{

    ClRcT rc = CL_OK;

    AMS_CHECKPTR (!entityConfig);

    switch (entityConfig->type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            AMS_CHECK_RC_ERROR ( (*gAmsToCpmCallbackFuncs->
                                  cpmEntitySetConfig)(entityConfig, bitMask) );
            break;
        }
        default:
        {
            break;
        }
    }

    return CL_OK;

exitfn:
    return CL_AMS_RC (rc);
}

/*
 * _clAmsSANodeJoin
 * ----------------
 * This is CPM callout to inform AMS about the node arrival event 
 *
 * @param
 *  nodeName                  - Name of the node which has joined the cluster 

 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT 
_clAmsSANodeJoin(
        CL_IN  ClNameT  *nodeName )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  nodeRef = {{0},0,0};
    ClAmsNodeT  *node = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!nodeName);

    nodeRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    memcpy ( &nodeRef.entity.name, nodeName, sizeof (ClNameT));
   
    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &nodeRef),
            gAms.mutex );

    node = (ClAmsNodeT *)nodeRef.ptr;

    AMS_CHECKPTR_AND_UNLOCK ( !node, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
            clAmsPeNodeJoinCluster(node), 
            gAms.mutex );

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_DB));

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC (rc);

}

/*
 * _clAmsSANodeLeave
 * ----------------
 * This is CPM callout to inform AMS about the node departure event 
 *
 * @param
 *  nodeName                  - Name of the node which has left/leaving the 
                                cluster 
 *  request                   - indication whether the node is leaving or
                                already left the cluster

 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT 
_clAmsSANodeLeave(
        CL_IN  ClNameT  *nodeName,
        CL_IN  ClCpmNodeLeaveT  request,
        CL_IN  ClBoolT scFailover)
{

    ClRcT  rc = CL_OK;
    ClAmsNodeT  *node = NULL;
    ClAmsEntityRefT  nodeRef = {{0},0,0};
    ClBoolT auditEpilogue = CL_FALSE;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!nodeName);

    nodeRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    memcpy ( &nodeRef.entity.name, nodeName, sizeof (ClNameT));

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    AMS_CHECK_SERVICE_STATE_AND_UNLOCK_MUTEX(gAms.serviceState, gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &nodeRef),
            gAms.mutex );

    node = (ClAmsNodeT *)nodeRef.ptr;

    AMS_CHECKPTR_AND_UNLOCK ( !node,gAms.mutex );

    switch (request)
    {
        case CL_CPM_NODE_LEFT: 
            {

                auditEpilogue = CL_TRUE;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
                                                    clAmsPeNodeHasLeftCluster( node, scFailover),
                                                    gAms.mutex );
                
                break;

            }

        case CL_CPM_NODE_LEAVING:
            {

                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                                                     clAmsPeNodeIsLeavingCluster( node, scFailover),
                                                     gAms.mutex );

                break;

            }
        default:
            {

                AMS_LOG ( CL_DEBUG_ERROR,("invalid node operation request \n"));
                AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);

            }
    } 

    AMS_CALL_CKPT_WRITE (clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_DB));

    clOsalMutexUnlock(gAms.mutex);

exitfn:
    
    if(auditEpilogue)
        clAmsAuditDbEpilogue(nodeName);

    return CL_AMS_RC (rc);

}

/*
 * _clAmsSANodeLeaveCompleted
 * ----------------
 * This is AMS callout to inform CPM that switchover for the node is complete
 * and the node can be shutdown 
 *
 * @param
 *  nodeName                  - Name of the node which had requested shutdown
                                operation 
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT 
_clAmsSANodeLeaveCompleted(
                           CL_IN  ClNameT  *nodeName,
                           ClCpmNodeLeaveT nodeLeft)
{
    AMS_FUNC_ENTER (("\n"));

    clAmsCkptWrite(&gAms,CL_AMS_CKPT_WRITE_ALL);

    AMS_CALL ( (*gAmsToCpmCallbackFuncs->nodeDepartureAllowed) (nodeName, nodeLeft) );

    return CL_OK;
}

/*
 * _clAmsSACkptServerReady
 * ----------------
 * CPM to AMS callout to inform AMS that ckpt server is up and running. It
 * also informs the AMS about the checkpoint library initialization handle 
 * for AMS use.

 @param
 * ckptInitHandle           - ckpt library initialization handle 
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSACkptServerReady(
        CL_IN  ClCkptHdlT  ckptInitHandle,
        CL_IN  ClUint32T  mode )
{
    AMS_FUNC_ENTER (("\n"));

    if(gAms.ckptServerReady)
    {
        clLogWarning("AMS", "CKP", "AMS checkpointing already ready. Skipping initialization");
        return CL_OK;
    }

    AMS_CALL (clAmsCkptInitialize(&gAms,ckptInitHandle,mode));

    gAms.ckptServerReady = CL_TRUE;

    return CL_OK;

}

/*
 * _clAmsSAEventServerReady
 * ----------------
 * CPM to AMS callout to inform AMS about the availability of event server.
 * This callback will be called when-ever event server changes state from
 * available to non-available and vice vera.
 *
 * @param
 * eventServerReady           - event server is available / not available
 *
 * @returns
 *  CL_OK                    - Operation successful
 * 
 */

ClRcT
_clAmsSAEventServerReady(
        CL_IN   ClBoolT  eventServerReady )
{

    AMS_FUNC_ENTER (("\n"));

    if ( gAms.eventServerInitialized == CL_FALSE )
    {
        AMS_CALL ( clAmsNotificationEventInitialize() );
        gAms.eventServerInitialized = CL_TRUE;
    }

    gAms.eventServerReady = eventServerReady ;

    return CL_OK;

}

/*
 * _clAmsSAAmsStateChange
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * its state from Active to Standby or Standby to Active 

 @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAAmsStateChange(
        ClUint32T  mode )
{

    AMS_FUNC_ENTER (("\n"));

    if ( mode&CL_AMS_STATE_CHANGE_ACTIVE_TO_STANDBY )
    { 
        AMS_CALL (_clAmsSAStateChangeActive2Standby(mode));
    }
    else if ( mode&CL_AMS_STATE_CHANGE_STANDBY_TO_ACTIVE )
    {
        AMS_CALL (_clAmsSAStateChangeStandby2Active(mode));
    }
    else if ( mode&CL_AMS_STATE_CHANGE_RESET )
    {
        AMS_CALL (_clAmsSAStateChangeReset(mode));
    }

    return CL_OK;

}


/*
 * _clAmsSAStateChangeActive2Standby
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * state from Active to Standby. AMS running on this controller card should 
 * no longer service the requests and stop all the current operations. 

 @param
 *  mode                      - mode to inform type of transition
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAStateChangeActive2Standby(
        ClUint32T  mode)
{

    AMS_FUNC_ENTER (("\n"));

    AMS_LOG(CL_DEBUG_TRACE,("AMS Server Changing State From Active->Standby\n"));

    /*
     * Update the checkpoint
     */

    clOsalMutexLock(gAms.mutex);

    clAmsCkptWriteSync(&gAms,CL_AMS_CKPT_WRITE_ALL);

    clAmsInvocationListDeleteAllInvocations(gAms.invocationList);

    clAmsDbTerminate(&gAms.db);

    gAms.serviceState = CL_AMS_SERVICE_STATE_UNAVAILABLE;
    
    clOsalMutexUnlock(gAms.mutex);

    return CL_OK;

}

/*
 * _clAmsSAStateChangeStandby2Active
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card has changed
 * state from standby to active. AMS running on this controller card should 
 * start servicing the requests. This callout is valid in a situation when 
 * Active system controller was made standby and is now again made active 

 @param
 *
 *  mode                      - mode to inform type of transition
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAStateChangeStandby2Active(
                                  ClUint32T  mode )
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT thisNodeRef = {{0}};
    ClAmsNodeT *thisNode = NULL;
    ClAmsNodeClusterMemberT nodeStatus = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;

    AMS_FUNC_ENTER (("\n"));

    AMS_LOG(CL_DEBUG_TRACE,("AMS Server Changing State From Standby->Active\n"));

    clOsalMutexLock(&gAms.ckptMutex);

    if(gAms.serviceState == CL_AMS_SERVICE_STATE_RUNNING)
    {
        clOsalMutexUnlock(&gAms.ckptMutex);
        AMS_LOG(CL_DEBUG_TRACE,("AMS Already Active.Ignoring state change\n"));
        return CL_OK;
    }

    /*
     * If its not hot standby, re-create the AMS db by reading the checkpoint.
     */
    if(gAms.serviceState != CL_AMS_SERVICE_STATE_HOT_STANDBY)
    {
        clAmsDbTerminate(&gAms.db);
        if( (rc = clAmsDbInstantiate(&gAms.db) ) != CL_OK)
        {
            clOsalMutexUnlock(&gAms.ckptMutex);
            clLogError("STATE", "STDBY2ACT", "AMS db instantiate returned [%#x]", rc);
            return rc;
        }

        clLogNotice("CKPT", "READ", "AMS checkpoint read during failover");
        rc = clAmsCkptRead(&gAms);
        if(rc != CL_OK)
        {
            gAms.serviceState = CL_AMS_SERVICE_STATE_SHUTTINGDOWN;
            clOsalMutexUnlock(&gAms.ckptMutex);
            clLogError("CKPT", "READ", "AMS checkpoint read returned [%#x]", rc);
            return rc;
        }
    }

    gAms.serviceState = CL_AMS_SERVICE_STATE_RUNNING;

    /*
     * Deregister ourselves through a NULL op. install
     */
    clCkptImmediateConsumptionRegister(gAms.ckptOpenHandle, NULL , NULL);

    clOsalMutexUnlock(&gAms.ckptMutex);

    /*
     * Now try auditing the DB to rectify inconsistencies.
     */
    clOsalMutexLock(gAms.mutex);

    /*
     * Now get the status of this node.
     * If this node is not a member of the cluster, that implies
     * that we are trying to become active while we are either not completely
     * UP or we were ourselves leaving the cluster. before the premature failover occured
     * We then immediately go down detecting the premature failover.
     */
    if(!gAmsToCpmCallbackFuncs
       ||
       (rc = (*gAmsToCpmCallbackFuncs->cpmNodeNameForNodeAddressGet)(
                                                                     clIocLocalAddressGet(),
                                                                     &thisNodeRef.entity.name)) != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        goto shutdown;
    }
    thisNodeRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    thisNodeRef.entity.name.length = strlen(thisNodeRef.entity.name.value)+1;
    
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                                 &thisNodeRef);
    if(rc != CL_OK 
       || 
       !(thisNode = (ClAmsNodeT*)thisNodeRef.ptr)
       ||
       ((nodeStatus = thisNode->status.isClusterMember) != CL_AMS_NODE_IS_CLUSTER_MEMBER))
    {
        clOsalMutexUnlock(gAms.mutex);
        goto shutdown;
    }
    
    clAmsDbStartTimers(&gAms.db);

    clAmsAuditDb(thisNode);

    clOsalMutexUnlock(gAms.mutex);

    return CL_OK;

    shutdown:
    clLogCritical("STATE", "CHANGE", 
                  "This node [ %d:%s] cannot become active as its not in the cluster."
                  "Node Cluster Member state [%s]."
                  "Immediately terminating the node and its components.",
                  clIocLocalAddressGet(), thisNodeRef.entity.name.length > 0 ? 
                  thisNodeRef.entity.name.value:"",
                  CL_AMS_STRING_NODE_ISCLUSTERMEMBER(nodeStatus));
    cpmResetNodeElseCommitSuicide(CL_CPM_RESTART_NODE);
    return CL_CPM_RC(CL_ERR_INVALID_STATE);

}

/*
 * _clAmsSAStateChangeReset
 * ----------------
 * CPM to AMS callout to inform AMS that system controller card configuration
 * should be reset 

 @param
 *
 *  mode                      - mode to inform type of transition
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAStateChangeReset(
        ClUint32T  mode )
{
    AMS_FUNC_ENTER (("\n"));

    return CL_OK;

}


/* 
 * Utility functions
 */

/*
 * _clAmsSAPrintDB
 * ----------------
 * This is a debug utility function to print the ams database 
 *
 * @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAPrintDB (void)

{
    AMS_FUNC_ENTER (("\n"));
    return clAmsDbPrint (&gAms.db);
}

/*
 * _clAmsSAPrintDBXML
 * ------------------
 * This is a debug utility function to print the ams database
 * in XML format
 *
 * @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClRcT
_clAmsSAPrintDBXML (void)

{
    AMS_FUNC_ENTER (("\n"));
    return clAmsDbXMLPrint (&gAms.db);
}

/*
 * _clAmsSAEntityPrint
 * ----------------
 * This is a debug utility function to print contents of an AMS entity 
 * @param

 * entity                 -  AMS entity name whose contents needs to be printed
 *
 * @returns
 *   CL_OK                - Operation successful

 */

ClRcT
_clAmsSAEntityPrint (
        CL_IN  ClAmsEntityT *entity )

{
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!entity);

    memcpy ( &entityRef.entity, entity, sizeof (ClAmsEntityT) );

    AMS_CALL ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entity->type],
                &entityRef));

    AMS_CHECKPTR (!entityRef.ptr);

    return clAmsEntityPrint (entityRef.ptr);

}

/*
 * _clAmsSAXMLizeDB
 * ----------------
 * Utility function to dump the AMS db into a XML file which is checkpointed

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

ClRcT
_clAmsSAXMLizeDB(void)
{
    ClUint32T dbInvocationPair=0;
    ClRcT rc=CL_OK;
    ClCharT *pData = NULL;
    FILE *fp = NULL;

    AMS_FUNC_ENTER (("\n"));
    if( (rc = clAmsCkptReadCurrentDBInvocationPair(&gAms,
                                                   &dbInvocationPair))!=CL_OK)
    {
        return rc;
    }
    rc = clAmsXMLizeDB (&gAms,
                        gAms.ckptDBSections[dbInvocationPair].value,&pData);
    
    if(rc != CL_OK)
    {
        return rc;
    }

    if(!pData)
    {
        return CL_AMS_RC(CL_ERR_UNSPECIFIED);
    }
    /*
     * Now write into the db file.
     */
    fp = fopen(gAms.ckptDBSections[dbInvocationPair].value,"w");
    if(!fp)
    {
        free(pData);
        return CL_AMS_RC(CL_ERR_LIBRARY);
    }

    fprintf(fp,pData);
    fflush(fp);
    fclose(fp);
    free(pData);
    return CL_OK;
}

/*
 * _clAmsSADeXMLizeDB
 * ----------------
 * Utility function to read the dumped ams db file and contruct ams 
 * db from it.

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */


ClRcT
_clAmsSADeXMLizeDB(void)
{
    ClUint32T dbInvocationPair=0;
    ClRcT rc=CL_OK;
    AMS_FUNC_ENTER (("\n"));
    if((rc = clAmsCkptReadCurrentDBInvocationPair(&gAms,
                                                  &dbInvocationPair))!=CL_OK)
    {
        return rc;
    }
    return clAmsDeXMLizeDB (&gAms,
                            gAms.ckptDBSections[dbInvocationPair].value);
}

/*
 * _clAmsSAXMLizeInvocation
 * ----------------
 * Utility function to dump the AMS invocations into a XML file which is 
 * checkpointed

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */

ClRcT
_clAmsSAXMLizeInvocation(void)
{
    ClUint32T dbInvocationPair=0;
    ClRcT rc= CL_OK;
    ClCharT *pData = NULL;
    FILE *fp = NULL;

    AMS_FUNC_ENTER (("\n"));

    if((rc = clAmsCkptReadCurrentDBInvocationPair(&gAms,
                                                  &dbInvocationPair)) != CL_OK)
    {
        return rc;
    }
    rc = clAmsXMLizeInvocation(&gAms,
                               gAms.ckptInvocationSections[dbInvocationPair].value, &pData);
    if(rc != CL_OK)
    {
        return rc;
    }
    if(!pData)
    {
        return CL_AMS_RC(CL_ERR_UNSPECIFIED);
    }

    fp = fopen(gAms.ckptInvocationSections[dbInvocationPair].value, "w");
    if(!fp)
    {
        free(pData);
        return CL_AMS_RC(CL_ERR_LIBRARY);
    }
    fprintf(fp,pData);
    fflush(fp);
    fclose(fp);
    free(pData);
    return CL_OK;
}
 
/*
 * _clAmsSADeXMLizeInvocation
 * ----------------
 * Utility function to read the dumped ams invocations XML file and contruct 
 * invocation list from it.

 @param
 * @returns
 *   CL_OK                    - Operation successful
 */
   
ClRcT
_clAmsSADeXMLizeInvocation(void)
{
    ClUint32T dbInvocationPair;
    ClRcT rc = CL_OK;
    AMS_FUNC_ENTER (("\n"));
    if((rc = clAmsCkptReadCurrentDBInvocationPair(&gAms,
                                                  &dbInvocationPair))!=CL_OK)
    {
        return rc;
    }
    AMS_CALL (clAmsDeXMLizeInvocation(&gAms,
                                      gAms.ckptInvocationSections[dbInvocationPair].value));
    return CL_OK;
}

/*
 * _clAmsSAMarshalRmdParams
 * ----------------
 * This function marshals the paramter required for AMS to make call to CPM 
 * like node name and ioc address
 *
 * @param
 *  compName                  - Name of the component on which CPM will perform
                                operation
 *  nodeName                  - node Name for the component
 *  physAddr                  - physical address for the component
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */


ClRcT
_clAmsSAMarshalRmdParams(
        CL_IN  ClNameT  *compName,
        CL_OUT  ClCharT  **nodeName,
        CL_OUT  ClIocPhysicalAddressT  *physAddr )
{

    ClAmsCompT  *comp = NULL;
    ClAmsSUT  *parentSU = NULL;
    ClAmsNodeT  *parentNode = NULL;
    ClAmsEntityRefT  compRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !compName );

    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    memcpy ( &compRef.entity.name, compName, sizeof (ClNameT));

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &compRef) );

    comp = (ClAmsCompT *)compRef.ptr;

    AMS_CHECKPTR ( !comp )

    parentSU = (ClAmsSUT *)comp->config.parentSU.ptr;

    AMS_CHECKPTR ( !parentSU );

    parentNode = (ClAmsNodeT *)parentSU->config.parentNode.ptr;

    AMS_CHECKPTR ( !parentNode );

    *nodeName = clHeapAllocate (parentNode->config.entity.name.length);

    AMS_CHECK_NO_MEMORY (nodeName);

    memcpy (*nodeName, parentNode->config.entity.name.value, 
            parentNode->config.entity.name.length );
    physAddr->nodeAddress = clIocLocalAddressGet(); 
    physAddr->portId = CL_IOC_CPM_PORT; 

    return CL_OK;

}

/*
 * _clAmsSAEventServerTest
 * ----------------
 *
 * Utility function to test event communication

 @param
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */


ClRcT
_clAmsSAEventServerTest()
{

    ClAmsNotificationDescriptorT notification = {0};

    notification.type = CL_AMS_NOTIFICATION_FAULT;
    notification.entityType = CL_AMS_ENTITY_TYPE_COMP;
    strcpy ( (ClCharT*)notification.entityName.value, "Test_Comp");
    notification.entityName.length = strlen ("Test_Comp");
    notification.recoveryActionTaken = CL_AMS_RECOVERY_SU_RESTART;
    notification.repairNecessary = CL_TRUE;

    AMS_CALL ( clAmsNotificationEventPublish( &notification));

    return CL_OK;

}

ClRcT
_clAmsSANodeAdd(const ClCharT *nodeName)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClRcT rc = CL_OK;

    if(!nodeName) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    clNameSet(&entityRef.entity.name, nodeName);
    ++entityRef.entity.name.length;
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                                 &entityRef);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS node [%s] found returned [%#x]\n",
                                 nodeName, rc));
        return rc;
    }

    rc = _clAmsSAEntityAdd(&entityRef);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS SA entity node [%s] add returned [%#x]\n",
                                 nodeName, rc));
        return rc;
    }

    return rc;
}

ClRcT
_clAmsSANodeRestart(const ClNameT *nodeName, ClBoolT graceful)
{
    ClRcT rc = CL_OK;
    ClAmsLocalRecoveryT recovery = CL_AMS_RECOVERY_NONE;
    ClUint32T escalation = CL_FALSE;
    ClAmsEntityRefT  entityRef;
    ClAmsNodeT *node = NULL;

    memset(&entityRef, 0, sizeof(ClAmsEntityRefT));

    recovery = graceful ? CL_AMS_RECOVERY_NODE_SWITCHOVER :
                          CL_AMS_RECOVERY_NODE_FAILFAST;

    memcpy(&entityRef.entity.name, nodeName, sizeof(ClNameT));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;

    AMS_CALL (clOsalMutexLock(gAms.mutex));

    AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(
                       &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                       &entityRef));

    node = (ClAmsNodeT*)entityRef.ptr;

    if(!node)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR(clAmsPeNodeComputeRecoveryAction(node, &recovery, &escalation));

    if(recovery == CL_AMS_RECOVERY_NONE)
    {
        rc = CL_AMS_RC(CL_ERR_NO_OP);
        clLogNotice(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_RESTART_NODE,
                    "Ignoring restart for node [%s] as recovery is already in progress",
                    node->config.entity.name.value);

        goto exitfn;
    }

    AMS_CHECK_RC_ERROR(clAmsPeNodeFaultReportProcess(node,
                                                     NULL,
                                                     &recovery,
                                                     &escalation));

exitfn:
    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
    return CL_AMS_RC (rc);
    
}

ClRcT
_clAmsSANodeHalt(ClNameT *nodeName, ClBoolT aspAware, ClBoolT ckpt)
{
    if(ckpt)
    {
        clAmsCkptWriteSync(&gAms, CL_AMS_CKPT_WRITE_ALL);
    }
    if(!gAmsToCpmCallbackFuncs->cpmNodeHalt)
        return CL_AMS_RC(CL_ERR_NO_OP);
    return (*gAmsToCpmCallbackFuncs->cpmNodeHalt)(nodeName, aspAware);
}

ClRcT
_clAmsSAClusterReset(ClAmsT *ams)
{
    if(!ams)
        return CL_OK;
    ams->serviceState = CL_AMS_SERVICE_STATE_STOPPED;
    if(!gAmsToCpmCallbackFuncs || !gAmsToCpmCallbackFuncs->cpmClusterReset)
        return CL_OK;
    return (*gAmsToCpmCallbackFuncs->cpmClusterReset)();
}
