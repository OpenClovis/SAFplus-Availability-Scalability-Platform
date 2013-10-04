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
 * ModuleName  : gms
 * File        : clSafGmsWrapper.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains the implementation of the implementation of SAF Cluster
 * membership apis. 
 *
 *****************************************************************************/


#include <saAis.h>
#include <saClm.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <string.h>
#include <clHandleApi.h>
#include <clGmsApiClient.h>
#include <clGmsSafApi.h>
#include <clDispatchApi.h>
#include <ipi/clHandleIpi.h>
#include <ipi/clEoIpi.h>
#include <clErrorApi.h>

#define GMS_LOG_AREA_CLM		"CLM"
#define GMS_LOG_CTX_CLM_FINALISE	"FIN"
#define GMS_LOG_CTX_CLM_DB		"DB"
#define GMS_LOG_CTX_CLM_CALLBACK	"CALLBACK"

static ClHandleDatabaseHandleT  databaseHandle = CL_HANDLE_INVALID_VALUE;
ClUint32T saGmsInitCount = 0;

static ClRcT saClmLibInitialize()
{
    ClRcT   rc = CL_OK;

    if (isInitDone() == CL_TRUE)
    {
        /* Initialize has already happened. so just return */
        return rc;
    }

    rc = clHandleDatabaseCreate(saClmHandleInstanceDestructor,
                                &databaseHandle);
    if (rc != CL_OK)
        {
        clLogError("DB","CRE",
                   "Handle database create failed with rc 0x%x\n",rc);
        }
    return rc;
        }

/* FIXME: Need to have a databaseHandleDestructor. 
   Where and how to handle it? */

SaAisErrorT
saClmInitialize (
        SaClmHandleT*const clmHandle,
        const SaClmCallbacksT*const clmCallbacks,
        SaVersionT*const version
        )
{
    ClRcT rc = CL_OK;
    ClGmsCallbacksT gmsCallbacks = {0};
    ClGmsHandleT gmsHandle = CL_HANDLE_INVALID_VALUE;
    /* 
     * Use a 32 bit handle type to create a handle. Dont
     * use the 64 bit clmHandle directly as it would create issues
     * in mixed mode operations.
     */
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;
    ClHandleT   dispatchHandle = CL_HANDLE_INVALID_VALUE;

    if (clmHandle == (const void*)NULL)
    {
        return _aspErrToAisErr(CL_ERR_NULL_POINTER);
    }

    rc = clASPInitialize();
    if(CL_OK != rc)
    {
        clLogCritical("CLM", "INI",
                      "ASP initialize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }
    
    /* 
     * Create the handle database for clm first, if it is 
     * not already created
     */
    saClmLibInitialize();

    if (clmCallbacks != (const void*)NULL)
    {
        /*Set GMS callbacks to callback wrappers*/
        if (clmCallbacks->saClmClusterNodeGetCallback != NULL)
        {
            gmsCallbacks.clGmsClusterMemberGetCallback =
                clGmsClusterMemberGetCallbackWrapper;
        }

        if (clmCallbacks->saClmClusterTrackCallback != NULL)
        {
            gmsCallbacks.clGmsClusterTrackCallback =
                clGmsClusterTrackCallbackWrapper;
    }
    }

    /* Initialize GMS */
    rc = clGmsInitialize(&gmsHandle,
            &gmsCallbacks, 
                         (ClVersionT*)version);
    if(rc != CL_OK)
    {
        clLogError("CLM","INI",
                   "clGmsInitialize failed with rc 0x%x\n",rc);
        return _aspErrToAisErr(rc);
    }

    localHandle = gmsHandle;
    /*
     * Create a local handle to be returned as clmHandle
     * Here we will create the handle value same as that of
     * gmsHandle so that we can keep the reference during
     * callback invocation through thread specific data
     */
    rc = clHandleCreateSpecifiedHandle(databaseHandle,
                                       sizeof(SaClmInstanceT),
                                       localHandle);
    if (rc != CL_OK)
    {
        clLogError("CLM","INI",
                   "clHandleCreateSpecifiedHandle failed with rc 0x%x\n",rc);
        return _aspErrToAisErr(rc);
    }

    /*
     * Assign localHandle to clmHandle, but keep using localHandle,
     * as it is compliant with ASP APIs.
     */
    *clmHandle = localHandle;
    SA_GMS_INIT_COUNT_INC();

    /* Checkout the handle */
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    CL_ASSERT((rc == CL_OK) && (clmInstance != NULL));


    if (clmCallbacks != (const void*)NULL)
    {
        /*Save the saf callbacks in the handle*/
        memcpy(&clmInstance->callbacks, clmCallbacks, sizeof(SaClmCallbacksT));
    }

    /* Initialize dispatch */
    rc = clDispatchRegister(&dispatchHandle,
                            localHandle,
                            dispatchWrapperCallback,
                            dispatchQDestroyCallback);
    if (rc != CL_OK)
    {
        clLogError("CLM","INI",
                   "clDispatchRegister failed with rc 0x%x\n",rc);
        goto error_return;
    }
    /* Store dispatchHandle in the clmHandle */
    clmInstance->dispatchHandle = dispatchHandle;

error_return:
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
}

    return _aspErrToAisErr(rc);
}


SaAisErrorT
saClmFinalize (
        const SaClmHandleT clmHandle
        )
{
    ClRcT rc = CL_OK;
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;
    
    SA_GMS_CHECK_INIT_COUNT();

    /* Handle checkout */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
    return _aspErrToAisErr(rc);
}
    CL_ASSERT(clmInstance != NULL);
    
    /* Finalize with GMS */
    rc = clGmsFinalize(localHandle);
    if (rc != CL_OK)
    {
        clLogError("CLM","FIN",
                   "clGmsFinalize failed with rc 0x%x\n",rc);
    }

    /* Deregister with dispatch */
    rc = clDispatchDeregister(clmInstance->dispatchHandle);
    if (rc != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_FINALISE,
                   "clDispatchDeregister failed with rc 0x%x\n",rc);
    }

    /* Checkin the handle */
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_FINALISE,
                    "clHandleCheckin failed");
    }

    /* Destroy the handle */
    if ((clHandleDestroy(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_FINALISE,
                   "clHandleDestroy failed");
    }

    SA_GMS_INIT_COUNT_DEC();
    if (isLastFinalize() == CL_TRUE)
    {
        rc = clHandleDatabaseDestroy(databaseHandle);
        if (rc != CL_OK)
        {
            clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_DB,
                       "clHandleDatabaseDestroy failed with rc 0x%x\n",rc);
        }
    }
    
    if(CL_OK != clASPFinalize())
    {
        clLogInfo("CLM", "FIN",
                  "ASP finalize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }

    return _aspErrToAisErr(rc);
}


SaAisErrorT
saClmClusterTrack (
        const SaClmHandleT clmHandle,
        const SaUint8T trackFlags,
        SaClmClusterNotificationBufferT*const notificationBuffer)
{
    ClRcT rc = CL_OK;
    ClGmsClusterNotificationBufferT gms_cluster_notf_buffer = {0};
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;
    ClUint32T i = 0x0;

    if (((trackFlags & SA_TRACK_CURRENT)==SA_TRACK_CURRENT) && /* If current view is requested */
            (notificationBuffer != NULL) &&        /* Buffer is provided */
            (notificationBuffer->notification != NULL) && /* Caller provides array */
            (notificationBuffer->numberOfItems == 0))
        /* then size must be given */
    {
        return _aspErrToAisErr(CL_ERR_INVALID_PARAMETER);
    }

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
        return _aspErrToAisErr(rc);
    }
    CL_ASSERT(clmInstance != NULL);
    
    if (notificationBuffer != NULL)
    {
        gms_cluster_notf_buffer.viewNumber = notificationBuffer->viewNumber;
        gms_cluster_notf_buffer.numberOfItems = notificationBuffer->numberOfItems;
        if (notificationBuffer->numberOfItems > 0) 
        {
            gms_cluster_notf_buffer.notification =
                clHeapAllocate((notificationBuffer->numberOfItems)*(sizeof(ClGmsClusterNotificationT)));

            if (gms_cluster_notf_buffer.notification == NULL)
            {
                rc = CL_ERR_NO_MEMORY;
                goto error_return;
            }
            memset(gms_cluster_notf_buffer.notification, 0,
                   (notificationBuffer->numberOfItems)*(sizeof(ClGmsClusterNotificationT)));
        }
    }

    /* Call the GmsClusterTrack Api */
    if (notificationBuffer == NULL)
    {
    rc = clGmsClusterTrack(localHandle,
                               trackFlags,
                               NULL);
    }
    else
    {
    rc = clGmsClusterTrack(localHandle,
            trackFlags,
                           &gms_cluster_notf_buffer);
    }

    if(rc != CL_OK )
    {
        if (gms_cluster_notf_buffer.notification != NULL)
        {
            clHeapFree ((void*)gms_cluster_notf_buffer.notification);
        }
        goto error_return;
    }

    if (trackFlags == SA_TRACK_CHANGES ||
            trackFlags == SA_TRACK_CHANGES_ONLY)
    {
        /* We should ignore the notification buffer */
        if (gms_cluster_notf_buffer.notification != NULL)
        {
            clHeapFree ((void*)gms_cluster_notf_buffer.notification);
        }
        rc = CL_OK;
        goto error_return;
    }

    if( notificationBuffer )
    {
    /* FIXME: Verify the requirement of below check */
    if (gms_cluster_notf_buffer.notification == NULL)
    {
        return SA_AIS_ERR_FAILED_OPERATION;
    }

        /* Xlate the GmsClusternotification data type to ClmClusternotification
         *  data type */
        notificationBuffer->viewNumber= gms_cluster_notf_buffer.viewNumber;
        notificationBuffer->numberOfItems= gms_cluster_notf_buffer.numberOfItems;
        if( notificationBuffer->numberOfItems > 0x0 )
        {
            /* if the notification array is NULL then allocate the memory and
             *  fill the buffer, User needs to free this memory */
            if( notificationBuffer->notification == NULL )
            {
                notificationBuffer->notification 
                    =(SaClmClusterNotificationT*)
                    clHeapAllocate(
                            (notificationBuffer->numberOfItems)
                            *(sizeof(SaClmClusterNotificationT)));

                if( notificationBuffer->notification == NULL )
                {
                    return _aspErrToAisErr(CL_ERR_NO_MEMORY);
                }
            }

            /* copy the data from the GmsClusternotification to
             *  ClmClusternotification structure */
            for( i = 0 ; i < gms_cluster_notf_buffer.numberOfItems; i++ )
            {
                (notificationBuffer->notification[i]).clusterChange =
                    gms_cluster_notf_buffer.notification[i].clusterChange;
                /* copy the node information from the gms node to clm node */
                (notificationBuffer->notification[i]).clusterNode.nodeId =
                    gms_cluster_notf_buffer.notification[i].clusterNode.nodeId;
                (notificationBuffer->notification[i]).clusterNode.member=
                    gms_cluster_notf_buffer.notification[i].clusterNode.memberActive;
                notificationBuffer->notification[i].clusterNode.bootTimestamp=
                    gms_cluster_notf_buffer.notification[i].clusterNode.bootTimestamp;
                notificationBuffer->notification[i].clusterNode.initialViewNumber=
                    gms_cluster_notf_buffer.notification[i].clusterNode.initialViewNumber;
                /* copy the node name */
                memcpy( 
                        &notificationBuffer->notification[i].clusterNode.nodeName,
                        &gms_cluster_notf_buffer.notification[i].clusterNode.nodeName,
                        sizeof(SaNameT)
                      );
                /* copy the node ipaddress */
                memcpy(&notificationBuffer->notification[i].clusterNode.nodeAddress,
                        &gms_cluster_notf_buffer.notification[i].clusterNode.nodeIpAddress,
                        sizeof(SaClmNodeAddressT)
                      );
            }
        }
    }

error_return:
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
    }
    return _aspErrToAisErr(rc);
}


SaAisErrorT
saClmClusterTrackStop (
        const SaClmHandleT clmHandle
        )
{
    ClRcT rc;
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
    return _aspErrToAisErr(rc);
}
    CL_ASSERT(clmInstance != NULL);

    rc = clGmsClusterTrackStop(localHandle);

    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
    }

    return _aspErrToAisErr(rc);
}


SaAisErrorT
saClmClusterNodeGet (
        const SaClmHandleT clmHandle,
        const SaClmNodeIdT nodeId,
        const SaTimeT timeout,
        SaClmClusterNodeT* const clusterNode
        )
{
    ClRcT rc = CL_OK;
    ClGmsClusterMemberT gms_node = {0};
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;

    /*
     * ASP considers 0 timeout to be infinite wait. However, SAF 
     * specify it as invalid parameter. Hence handling the sanity check 
     * for timeout variable here itself. 
     */
    if (clusterNode == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if (timeout == 0)
    {
        return SA_AIS_ERR_TIMEOUT;
    }

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
        return _aspErrToAisErr(rc);
    }
    CL_ASSERT(clmInstance != NULL);

    rc = clGmsClusterMemberGet(localHandle,
            (ClGmsMemberIdT)nodeId,
            timeout,
                               &gms_node);
    if( rc != CL_OK )
    {
        goto error_return;
    }

    /* Do the xlation between the clovis node type and the Ais node type */
    clusterNode->nodeId = gms_node.nodeId;
    clusterNode->member = gms_node.memberActive;
    clusterNode->bootTimestamp = gms_node.bootTimestamp;
    clusterNode->initialViewNumber= gms_node.initialViewNumber;
    memcpy(&clusterNode->nodeName, &gms_node.nodeName ,sizeof(SaNameT));
    memcpy(&clusterNode->nodeAddress,&gms_node.nodeIpAddress,
            sizeof(SaClmNodeAddressT));
error_return:
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
}

    return _aspErrToAisErr(rc);
}

SaAisErrorT
saClmClusterNodeGetAsync (
        const SaClmHandleT clmHandle,
        const SaInvocationT invocation,
        const SaClmNodeIdT nodeId)
{
    ClRcT rc;
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
        return _aspErrToAisErr(rc);
    }
    CL_ASSERT(clmInstance != NULL);

    rc = clGmsClusterMemberGetAsync(localHandle,
                invocation,
                (ClGmsMemberIdT)nodeId); 

    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
    }

    return _aspErrToAisErr(rc);
}



SaAisErrorT
saClmSelectionObjectGet (
        const SaClmHandleT clmHandle,
        SaSelectionObjectT*const selectionObject
        )
{
    ClRcT rc;
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
        return _aspErrToAisErr(rc);
    }
    CL_ASSERT(clmInstance != NULL);

    
    rc = clDispatchSelectionObjectGet(
                        clmInstance->dispatchHandle,
                        selectionObject);

    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
    }

    return _aspErrToAisErr(rc);
}





SaAisErrorT
saClmDispatch (
        const SaClmHandleT clmHandle,
        const SaDispatchFlagsT dispatchFlags
        )
{
    ClRcT rc;
    ClHandleT   localHandle = CL_HANDLE_INVALID_VALUE;
    SaClmInstanceT      *clmInstance = NULL;

    SA_GMS_CHECK_INIT_COUNT();

    /* Check the validity of the handle */
    localHandle = (ClHandleT)clmHandle;
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);

    if (rc != CL_OK)
    {
        return _aspErrToAisErr(rc);
    }
    CL_ASSERT(clmInstance != NULL);
    
    /* 
     * Since dispatch can be blocking and it would return only after
     * finalize, we will do the checkin before getting into dispatch
     */
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clHandleCheckin failed");
    }
    
    rc = clDispatchCbDispatch(
            clmInstance->dispatchHandle,
            (ClDispatchFlagsT)dispatchFlags);

    return _aspErrToAisErr(rc);
}




/* Utility function to map ASP errors to Ais errors */
static SaAisErrorT  
_aspErrToAisErr(
        const ClRcT  clError
        )
{
    if (clError == CL_OK)
    {
        return SA_AIS_OK;
    }

    switch (CL_GET_ERROR_CODE(clError))
    {
        case CL_GMS_ERR_CLUSTER_VERSION_MISMATCH:
            return SA_AIS_ERR_VERSION;
        default:
            return clErrorToSaf(clError);
    }
}


static void clGmsClusterMemberGetCallbackWrapper (
        CL_IN const ClInvocationT         invocation,
        CL_IN const ClGmsClusterMemberT* const clusterMember,
        CL_IN const ClRcT                 rc)
{
    SaClmClusterNodeT *safNode = NULL;
    ClRcT              error = CL_OK;
    ClGmsHandleT localHandle = CL_HANDLE_INVALID_VALUE;
    ClGmsHandleT *pGmsHandle = NULL;
    SaClmClusterNodeGetDataT   *callbackArg = NULL;
    SaClmInstanceT      *clmInstance = NULL;
    
    /* 
     * Get the handle context in which this callback is invoked,
     * by using thread specific data.
     */
    error = clOsalTaskDataGet(clGmsPrivateDataKey, (ClOsalTaskDataT*)&pGmsHandle);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clOsalTaskDataGet failed with rc 0x%x\n",error);
    }
    localHandle = *pGmsHandle;

    /* Handle checkout */
    error = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "localHandle checkout failed with rc 0x%x\n",error);
        return;
    }

    if ((rc == CL_OK) && (clusterMember != (const void*)NULL))
    {
        /*Allocate memory for safNode*/
        safNode = clHeapAllocate(sizeof(SaClmClusterNodeT));
        if (safNode == NULL)
        {
            clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                       "MemberGet Callback invocation faile due to no memory");
            goto error_exit;
        }
        else
        {
            memset (safNode, 0, sizeof(SaClmClusterNodeT));

            /*Get the values of clusterMember into safNode*/
            safNode->nodeId = clusterMember->nodeId;

            safNode->nodeAddress.family = (SaClmNodeAddressFamilyT)clusterMember->nodeIpAddress.family;
            safNode->nodeAddress.length = clusterMember->nodeIpAddress.length;
            memcpy(&safNode->nodeAddress.value,
                    &clusterMember->nodeIpAddress.value,sizeof(SaUint8T)*SA_CLM_MAX_ADDRESS_LENGTH);

            safNode->nodeName.length = clusterMember->nodeName.length;
            memcpy(&safNode->nodeName.value,&clusterMember->nodeName.value,sizeof(SaUint8T)*SA_MAX_NAME_LENGTH);

            safNode->member = clusterMember->memberActive;
            safNode->bootTimestamp = clusterMember->bootTimestamp;
            safNode->initialViewNumber = clusterMember->initialViewNumber;
        }
    }

    callbackArg = (SaClmClusterNodeGetDataT*)clHeapAllocate(sizeof(SaClmClusterNodeGetDataT));
    if (callbackArg == NULL)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "MemberGet Callback invocation faile due to no memory");
        if (safNode != NULL)
        {
            clHeapFree(safNode);
        }
        goto error_exit;
    }
    callbackArg->invocation = invocation;
    callbackArg->clusterNode = safNode;
    callbackArg->rc = _aspErrToAisErr(rc);
    
    error = clDispatchCbEnqueue(clmInstance->dispatchHandle,
                                CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK,
                                (void*)callbackArg);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clDispatchCbEnqueue failed with rc 0x%x",error);
    }

error_exit:
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clHandleCheckin failed");
    }

    return;
}

static void clGmsClusterTrackCallbackWrapper (
        CL_IN const ClGmsClusterNotificationBufferT* const notificationBuffer,
        CL_IN const ClUint32T             numberOfMembers,
        CL_IN const ClRcT                 rc)
{
    ClRcT                          error = CL_OK;
    ClGmsHandleT localHandle = CL_HANDLE_INVALID_VALUE;
    ClGmsHandleT *pGmsHandle = NULL;
    SaClmClusterNotificationBufferT *safbuf = NULL;
    SaClmInstanceT      *clmInstance = NULL;
    ClUint32T                       index = 0;
    SaClmClusterTrackDataT          *callbackArg;

    /*
     * Get the handle context in which this callback is invoked,
     * by using thread specific data.
     */
    error = clOsalTaskDataGet(clGmsPrivateDataKey, (ClOsalTaskDataT*)&pGmsHandle);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clOsalTaskDataGet failed with error 0x%x\n",error);
    }
    localHandle = *pGmsHandle;

    /* Handle checkout */
    error = clHandleCheckout(databaseHandle,
                             localHandle,
                             (void*)&clmInstance);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "localHandle checkout failed with error 0x%x\n",error);
        return;
    }

    /* Convert the notification gotten from GMS into SAF notification */
    safbuf = clHeapAllocate(sizeof(SaClmClusterNotificationBufferT));
    if (safbuf == NULL)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "Cluster Track Callback failed due to no memory");
        goto error_return;
    }

    safbuf->viewNumber = notificationBuffer->viewNumber;
    safbuf->numberOfItems = notificationBuffer->numberOfItems;
    if( safbuf->numberOfItems != 0x0 )
    {
        safbuf->notification = (SaClmClusterNotificationT*)
                               clHeapAllocate(
                                       (safbuf->numberOfItems)
                                       *(sizeof(SaClmClusterNotificationT)));
        if (safbuf->notification == NULL)
        {
            clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                       "Cluster Track Callback failed due to no memory");
            clHeapFree(safbuf);
            goto error_return;
        }

        for (index = 0; index < safbuf->numberOfItems; index++)
        {
            (safbuf->notification[index]).clusterChange =
                notificationBuffer->notification[index].clusterChange;

            (safbuf->notification[index]).clusterNode.nodeId =
                notificationBuffer->notification[index].clusterNode.nodeId;

            (safbuf->notification[index]).clusterNode.member =
                notificationBuffer->notification[index].clusterNode.memberActive;

            safbuf->notification[index].clusterNode.bootTimestamp =
                notificationBuffer->notification[index].clusterNode.bootTimestamp;

            safbuf->notification[index].clusterNode.initialViewNumber =
                notificationBuffer->notification[index].clusterNode.initialViewNumber;

            /* Copy the node name */
            memcpy(&safbuf->notification[index].clusterNode.nodeName,
                   &notificationBuffer->notification[index].clusterNode.nodeName,
                   sizeof(SaNameT));

            /* copy the node ipaddress */
            memcpy(&safbuf->notification[index].clusterNode.nodeAddress,
                   &notificationBuffer->notification[index].clusterNode.nodeIpAddress,
                   sizeof(SaClmNodeAddressT));
        }
    }

    callbackArg = (SaClmClusterTrackDataT*)clHeapAllocate(sizeof(SaClmClusterTrackDataT));
    if (callbackArg == NULL)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "Cluster track callback failed due to no memory");
        clHeapFree(safbuf->notification);
        clHeapFree(safbuf);
        goto error_return;
    }

    callbackArg->notificationBuffer = safbuf;
    callbackArg->numberOfMembers = numberOfMembers;
    callbackArg->rc = _aspErrToAisErr(rc);

    error = clDispatchCbEnqueue(clmInstance->dispatchHandle,
                                CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK,
                                (void*)callbackArg);
    if (error != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clDispatchCbEnqueue failed with rc 0x%x",error);
    }

error_return:
    if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,GMS_LOG_CTX_CLM_CALLBACK,
                   "clHandleCheckin failed");
    }

    return;
}

static void saClmHandleInstanceDestructor(void* cbArgs)
{
    /* 
     * Needs to destroy any parameters of handle.
     * Currently no operation required here 
     */
    return;
}

static void dispatchWrapperCallback (ClHandleT  localHandle,
                                     ClUint32T  callbackType,
                                     void*      callbackData)
{
    SaClmInstanceT      *clmInstance = NULL;
    ClRcT                rc = CL_OK;

    /* Handle checkout */
    rc = clHandleCheckout(databaseHandle,
                          localHandle,
                          (void*)&clmInstance);
    if (rc != CL_OK)
    {
        clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                   "localHandle checkout failed with rc 0x%x\n",rc);
        return;
    }
    
    switch (callbackType)
    {
        case CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK:
            /* Invoke the callback */
            clmInstance->callbacks.saClmClusterTrackCallback (
                    ((SaClmClusterTrackDataT*)callbackData)->notificationBuffer,
                    ((SaClmClusterTrackDataT*)callbackData)->numberOfMembers,
                    ((SaClmClusterTrackDataT*)callbackData)->rc);
            break;
        case CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK:
            /* Invoke the callback */
            clmInstance->callbacks.saClmClusterNodeGetCallback (
                    ((SaClmClusterNodeGetDataT*)callbackData)->invocation,
                    ((SaClmClusterNodeGetDataT*)callbackData)->clusterNode,
                    ((SaClmClusterNodeGetDataT*)callbackData)->rc);
            break;
    }
    
     if ((clHandleCheckin(databaseHandle, localHandle)) != CL_OK)
     {
         clLogError(GMS_LOG_AREA_CLM,CL_LOG_CONTEXT_UNSPECIFIED,
                    "clHandleCheckin failed");
     }
     return;
}

static void  dispatchQDestroyCallback (ClUint32T callbackType,
                                       void*     callbackData)
{
	if (callbackData == NULL)
	{
		return;
	}

    switch (callbackType)
    {
        case CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK:
            if (((SaClmClusterTrackDataT*)callbackData)->notificationBuffer != NULL)
            {
                if (((SaClmClusterTrackDataT*)callbackData)->notificationBuffer->notification != 
                        NULL)
                {
                    clHeapFree(((SaClmClusterTrackDataT*)callbackData)->notificationBuffer->notification);
                }
                clHeapFree(((SaClmClusterTrackDataT*)callbackData)->notificationBuffer);
            }
            break;
        case CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK:
            if (((SaClmClusterNodeGetDataT*)callbackData)->clusterNode != NULL)
            {
                clHeapFree(((SaClmClusterNodeGetDataT*)callbackData)->clusterNode);
            }
            break;
    }
    clHeapFree(callbackData);
}
