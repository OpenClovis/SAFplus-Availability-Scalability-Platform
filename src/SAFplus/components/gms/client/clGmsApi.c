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
 * File        : clGmsApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is the client side implementation of GMS.
 *
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clIocIpi.h>

#include <clClmTmsCommon.h>
#include <clClmApi.h>
#include <clGmsErrors.h>
#include <clGmsCommon.h>
#include <saClm.h>

#include "clHandleApi.h"
#include "clVersionApi.h"
#include "clGmsRmdClient.h"
#include "clGmsApiClient.h"

#define   CB_QUEUE_FIN             'f'


/******************************************************************************
 * LOCAL TYPES AND VARIABLES
 *****************************************************************************/

/* Contex for a GMS "instance" - i.e. one user of GMS service */

/* Instance handle database */
ClHandleDatabaseHandleT handle_database;

/* 
 * Thread specific data key which will hold handle for a particular
 * invocation of the callback.
 */
ClUint32T       clGmsPrivateDataKey = 0;
static ClBoolT  gClTaskKeyCreated = CL_FALSE;

/* Flag to show if library is initialized */
static ClBoolT lib_initialized = CL_FALSE;

/* Destructor to be called when deleting GMS handles */
static void gms_handle_instance_destructor(void *);

/*
 * Versions supported
 */
static ClVersionT versions_supported[] = {
	{ 'B', 1, 1 }
};

static ClVersionDatabaseT version_database = {
	sizeof (versions_supported) / sizeof (ClVersionT),
	versions_supported
};

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void
gms_handle_instance_destructor(void *notused)
{
    /*Destroy function for HandleDatabaseCreate. Not implemented*/
}

ClRcT clGmsLibInitialize(void);

static ClRcT check_lib_init(void)
{
    ClRcT rc = CL_OK;
    
    if (lib_initialized == CL_FALSE)
    {
        rc = clGmsLibInitialize();
    }
    
    return rc;
}


/******************************************************************************
 * EXPORTED LIB INIT/FINALIZE FUNCTIONS
 *****************************************************************************/

/* Library initialize function.  Not reentrant! Creates GMS handle database */
ClRcT clGmsLibInitialize(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *eo = NULL;
    
    if (lib_initialized == CL_FALSE)
    {
        rc = clHandleDatabaseCreate(gms_handle_instance_destructor,
                                    &handle_database);
        if (rc != CL_OK)
        {
            goto error_exit;
        }
        
        /*
         * FIXME: On the long run, when multiple EOs can exist in same process,
         * the following function installation may be devorced from the lib
         * initialization, so that it can be called multiple times.
         */
        rc = clEoMyEoObjectGet(&eo);
        if (rc != CL_OK)
        {
            if ((clHandleDatabaseDestroy( handle_database )) != CL_OK)
            {
                CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                        ("\nclHandleDatabaseDestroy failed"));
            }
            goto error_exit;
        }
        
        rc = clGmsClientRmdTableInstall(eo);
        if (rc != CL_OK)
        {
            if ((clHandleDatabaseDestroy( handle_database )) != CL_OK)
            {
                CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                        ("\nclHandleDatabaseDestroy failed"));
            }

            if ((clEoClientUninstall( eo , CL_GMS_CLIENT_TABLE_ID )) != CL_OK)
            {
                CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                        ("\nclEoClientUninstall failed"));
            }
            goto error_exit;
        }

        rc = clGmsClientClientTableRegistrer(eo);
        if (rc != CL_OK)
        {
            return rc;
        }

        rc = clGmsClientTableRegister(eo);
        if (rc != CL_OK)
        {
            return rc;
        }


        lib_initialized = CL_TRUE;
    }

error_exit:
    return rc;
}


/* Library cleanup function.  Not reentrant! Remoes GMS handle database*/
ClRcT clGmsLibFinalize(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *eo = NULL;
    
    if (lib_initialized == CL_TRUE)
    {
        rc = clHandleDatabaseDestroy(handle_database);
        if (rc != CL_OK)
        {
            goto error_exit;
        }

        rc = clEoMyEoObjectGet(&eo);
        if (rc != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                   ("clEoMyEoObjectGet Failed with RC - 0x%x\n",rc));
        }

        rc = clGmsClientRmdTableUnInstall(eo);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clEoClientUninstall Failed with RC - 0x%x\n",rc));
        }

        lib_initialized = CL_FALSE;
    }

error_exit:

    return rc;
}

static ClRcT gmsTaskKeyCreate(void)
{
    ClRcT rc = CL_OK;
    if(gClTaskKeyCreated) return CL_OK;
    rc = clOsalTaskKeyCreate(&clGmsPrivateDataKey, NULL);
    if (rc != CL_OK)
    {
        return rc;
    }
    gClTaskKeyCreated = CL_TRUE;
    return rc;
}

static ClRcT gmsTaskKeyDelete(void)
{
    ClRcT rc = CL_OK;
    if(!gClTaskKeyCreated) return CL_OK;
    gClTaskKeyCreated = CL_FALSE;
    /* Delete the key for thread specific data set */
    rc = clOsalTaskKeyDelete(clGmsPrivateDataKey);
    return rc;
}


/******************************************************************************
 * EXPORTED API FUNCTIONS
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * Initialize API
 *---------------------------------------------------------------------------*/
ClRcT clGmsInitialize(
    CL_OUT   ClGmsHandleT* const    gmsHandle,
    CL_IN    const ClGmsCallbacksT* const gmsCallbacks,
    CL_INOUT ClVersionT*   const      version)
{
    struct gms_instance *gms_instance_ptr = NULL;
    ClRcT rc = CL_OK;
    ClGmsClientInitRequestT req = {{0}};
    ClGmsClientInitResponseT *res = NULL;

    /* Step 0: Check readiness of library */

    rc = check_lib_init();
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_NOT_INITIALIZED);
    }
    
    /* Step 1: Checking inputs */
    
    if ((gmsHandle == NULL) || (version == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    /* Step 2: Verifying version match */
    
    rc = clVersionVerify (&version_database, version);
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_VERSION_MISMATCH);
        
    }

    /* Step 3: Obtain unique handle */

    rc = clHandleCreate(handle_database,
                        sizeof(struct gms_instance),
                        gmsHandle);
    if (rc != CL_OK)
    {
        rc = CL_GMS_RC(CL_ERR_NO_RESOURCE);
        goto error_no_destroy;
    }
    
    rc = clHandleCheckout(handle_database, *gmsHandle, (void *)&gms_instance_ptr);

    CL_ASSERT(rc == CL_OK); /* Should never happen */

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    /* Step 4: Negotiate version with the server */
    req.clientVersion.releaseCode = version->releaseCode;
    req.clientVersion.majorVersion= version->majorVersion;
    req.clientVersion.minorVersion= version->minorVersion;

    rc = cl_gms_clientlib_initialize_rmd((void*)&req, 0x0 ,&res );
    if(rc != CL_OK )
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\n cl_gms_clientlib_initialize_rmd failed with rc:0x%x ",rc));
        return CL_GMS_RC(rc);
    }
    

    /* Step 5: Initialize instance entry */
    
    if (gmsCallbacks) {
        memcpy(&gms_instance_ptr->callbacks, gmsCallbacks, sizeof(ClGmsCallbacksT));
    } else {
        memset(&gms_instance_ptr->callbacks, 0, sizeof(ClGmsCallbacksT));
    }

   
    clGmsMutexCreate(&gms_instance_ptr->response_mutex);

    memset(&gms_instance_ptr->cluster_notification_buffer, 0,
           sizeof(ClGmsClusterNotificationBufferT));
    memset(&gms_instance_ptr->group_notification_buffer, 0,
           sizeof(ClGmsGroupNotificationBufferT));

    /* Create the key for thread specific data set */
    rc = gmsTaskKeyCreate();
    if(rc != CL_OK)
    {
        clLogError("TASK", "KEY", "TaskKeyCreate returned [%#x]", rc);
    }

    /* Step 6: Decrement handle use count and return */
    if ((clHandleCheckin(handle_database, *gmsHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }
    clHeapFree(res);
    return CL_OK;

error_no_destroy:
    return rc;
}

/*-----------------------------------------------------------------------------
 * Finalize API
 *---------------------------------------------------------------------------*/
ClRcT clGmsFinalize(
    CL_IN const ClGmsHandleT gmsHandle)
{
	struct gms_instance *gms_instance_ptr = NULL;
	ClRcT rc= CL_OK;

	rc = clHandleCheckout(handle_database, gmsHandle, (void *)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_INVALID_HANDLE);
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    clGmsMutexLock(gms_instance_ptr->response_mutex);

	/*
	 * Another thread has already started finalizing
	 */
	if (gms_instance_ptr->finalize) {
		clGmsMutexUnlock(gms_instance_ptr->response_mutex);
		if ((clHandleCheckin(handle_database, gmsHandle)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nclHandleCheckin Error"));
        }
		return CL_GMS_RC(CL_ERR_INVALID_HANDLE);
	}

	gms_instance_ptr->finalize = 1;

	clGmsMutexUnlock(gms_instance_ptr->response_mutex);
	clGmsMutexDelete(gms_instance_ptr->response_mutex);

    /* Delete the key for thread specific data set */
    rc = gmsTaskKeyDelete();
    if (rc != CL_OK)
    {
        clLogError("TASK", "KEY", "TaskKeyDelete returned [%#x]", rc);
    }

	if ((clHandleDestroy(handle_database, gmsHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nclHandleDestroy Error"));
    }
    
	if ((clHandleCheckin(handle_database, gmsHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nclHandleCheckin Error"));
    }

	return CL_GMS_RC(rc);
}


/******************************************************************************
 * CLUSTER MEMBERSHIP SERVICE APIs
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * Cluster Join API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterJoin(
    CL_IN const ClGmsHandleT                        gmsHandle,
    CL_IN const ClGmsClusterManageCallbacksT* const clusterManageCallbacks,
    CL_IN const ClGmsLeadershipCredentialsT         credentials,
    CL_IN const ClTimeT                             timeout,
    CL_IN const ClGmsNodeIdT                        nodeId,
    CL_IN const ClNameT*                      const nodeName)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterJoinRequestT             req = {0};
    ClGmsClusterJoinResponseT           *res = NULL;
    
    clLog(INFO,CLM,NA, "clGmsClusterJoin API is being invoked");
    CL_GMS_SET_CLIENT_VERSION( req );

    if ((nodeName == (const void*)NULL) ||
        (clusterManageCallbacks == (const void*)NULL) ||
        (clusterManageCallbacks->clGmsMemberEjectCallback == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
    
    memcpy(&(gms_instance_ptr->cluster_manage_callbacks),
           clusterManageCallbacks,
           sizeof(ClGmsClusterManageCallbacksT));
    
    req.gmsHandle   = gmsHandle;
    req.credentials = credentials;
    req.nodeId      = nodeId;
    memcpy(&req.nodeName,nodeName, sizeof(ClNameT));
    req.sync        = CL_TRUE;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    if (clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclEoMyEoIocPortGet failed"));
    }
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    clLog(TRACE,CLM,NA, "Sending RMD to GMS server for cluster join");
    rc = cl_gms_cluster_join_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    clLog(TRACE,CLM,NA, "clGmsClusterJoin RMD returned");

    
    if( res ) 
    clHeapFree((void*)res);
    
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Join Async API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterJoinAsync(
    CL_IN const ClGmsHandleT                        gmsHandle,
    CL_IN const ClGmsClusterManageCallbacksT* const clusterManageCallbacks,
    CL_IN const ClGmsLeadershipCredentialsT         credentials,
    CL_IN const ClGmsNodeIdT                        nodeId,
    CL_IN const ClNameT*                      const nodeName)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterJoinRequestT             req = {0};
    ClGmsClusterJoinResponseT           *res = NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    if ((nodeName == (const void*)NULL) ||
        (clusterManageCallbacks == (const void*)NULL) ||
        (clusterManageCallbacks->clGmsMemberEjectCallback == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    
    memcpy(&(gms_instance_ptr->cluster_manage_callbacks),
           clusterManageCallbacks,
           sizeof(ClGmsClusterManageCallbacksT));
    
    req.gmsHandle   = gmsHandle;
    req.credentials = credentials;
    req.nodeId      = nodeId;
    memcpy(&req.nodeName,nodeName, sizeof(ClNameT));
    req.sync        = CL_FALSE;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_cluster_join_rmd(&req, 0 /* use def. timeout */, &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Leave API
 *---------------------------------------------------------------------------*/
static ClRcT gmsClusterLeave(
                             CL_IN const ClGmsHandleT                      gmsHandle,
                             CL_IN const ClTimeT                           timeout,
                             CL_IN const ClGmsNodeIdT                      nodeId,
                             CL_IN ClBoolT native)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterLeaveRequestT            req = {0};
    ClGmsClusterLeaveResponseT          *res = NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    
    memset(&(gms_instance_ptr->cluster_manage_callbacks), 0,
           sizeof(ClGmsClusterManageCallbacksT));
    
    req.gmsHandle = gmsHandle;
    req.nodeId    = nodeId;
    req.sync      = native ? CL_FALSE : CL_TRUE;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);

    if(native)
    {
        rc = cl_gms_cluster_leave_rmd_native(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    }
    else
    {
        rc = cl_gms_cluster_leave_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    }

    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return rc;
}

ClRcT clGmsClusterLeave(
                        CL_IN const ClGmsHandleT                      gmsHandle,
                        CL_IN const ClTimeT                           timeout,
                        CL_IN const ClGmsNodeIdT                      nodeId)
{
    return gmsClusterLeave(gmsHandle, timeout, nodeId, CL_FALSE);
}

ClRcT clGmsClusterLeaveNative(
                        CL_IN const ClGmsHandleT                      gmsHandle,
                        CL_IN const ClTimeT                           timeout,
                        CL_IN const ClGmsNodeIdT                      nodeId)
{
    return gmsClusterLeave(gmsHandle, timeout, nodeId, CL_TRUE);
}


/*-----------------------------------------------------------------------------
 * Cluster Leave Async API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterLeaveAsync(
    CL_IN const ClGmsHandleT                      gmsHandle,
    CL_IN const ClGmsNodeIdT                      nodeId)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterLeaveRequestT            req = {0};
    ClGmsClusterLeaveResponseT          *res = NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    memset(&(gms_instance_ptr->cluster_manage_callbacks), 0,
           sizeof(ClGmsClusterManageCallbacksT));
    
    req.gmsHandle = gmsHandle;
    req.nodeId    = nodeId;
    req.sync      = CL_FALSE;

    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_cluster_leave_rmd(&req, 0 /* use def. timeout */, &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Track API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterTrack(
    CL_IN    const ClGmsHandleT               gmsHandle,
    CL_IN    const ClUint8T                   trackFlags,
    CL_INOUT ClGmsClusterNotificationBufferT* const notificationBuffer)
{    
    ClRcT                       rc = CL_OK;
    struct gms_instance        *gms_instance_ptr = NULL;
    ClGmsClusterTrackRequestT   req = {0};
    ClGmsClusterTrackResponseT *res = NULL;
    const ClUint8T validFlag = CL_GMS_TRACK_CURRENT | CL_GMS_TRACK_CHANGES |
                            CL_GMS_TRACK_CHANGES_ONLY;
    ClBoolT shouldFreeNotification = CL_TRUE;

    clLog(TRACE,CLM,NA,"clGmsClusterTrack API is invoked");
    if (((trackFlags | validFlag) ^ validFlag) != 0) {
        return CL_GMS_RC(CL_ERR_BAD_FLAG);
    }

    CL_GMS_SET_CLIENT_VERSION( req );
    
    if (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) && /* If current view is requested */
        (notificationBuffer != NULL) &&        /* Buffer is provided */
        (notificationBuffer->notification != NULL) && /* Caller provides array */
        (notificationBuffer->numberOfItems == 0)) /* then size must be given */
    {
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    if (trackFlags == 0) /* at least one flag should be specified */
    {
        return CL_GMS_RC(CL_ERR_BAD_FLAG);
    }
    
    if (((trackFlags & CL_GMS_TRACK_CHANGES) == CL_GMS_TRACK_CHANGES) &&
        ((trackFlags & CL_GMS_TRACK_CHANGES_ONLY) == CL_GMS_TRACK_CHANGES_ONLY)) /* mutually exclusive flags */
    {
        return CL_GMS_RC(CL_ERR_BAD_FLAG);
    }
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_INVALID_HANDLE);
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
    
    /* If not a sync call, then clGmsClusterTrackCallbackHandler must be given */
    if (((trackFlags & (CL_GMS_TRACK_CHANGES|CL_GMS_TRACK_CHANGES_ONLY)) != 0) ||
        (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) &&
         (notificationBuffer == NULL)))
    {
        if (gms_instance_ptr->callbacks.clGmsClusterTrackCallback == NULL)
        {
            rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
            goto error_checkin;
        }
    }
    
    req.gmsHandle  = gmsHandle;
    req.trackFlags = trackFlags;
    req.sync       = CL_FALSE;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId));
    
    CL_ASSERT(rc == CL_OK); /* Should really never happen */
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    if (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) &&
        (notificationBuffer != NULL)) /* Sync response requested */
    {
        /*
         * We need to call the extended track() request which returns with
         * a notification buffer allocated by the XDR layer.
         */
        clLogMultiline(TRACE,CLM,NA,
                "Sending RMD to GMS server for Cluster track with"
                " track flags CL_GMS_TRACK_CURRENT");
        req.sync = CL_TRUE;
        rc = cl_gms_cluster_track_rmd(&req, 0 /* use def. timeout */, &res);
        clLog(TRACE,CLM,NA,"Returned from cluster track RMD");
        if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
        {
            switch (CL_GET_ERROR_CODE(rc))
            {
                case CL_ERR_TIMEOUT:    rc = CL_GMS_RC(CL_ERR_TIMEOUT); break;
                case CL_ERR_TRY_AGAIN:  rc = CL_GMS_RC(CL_ERR_TRY_AGAIN); break;
                default:                rc = CL_GMS_RC(CL_ERR_UNSPECIFIED);
            }
            /* FIXME: Need to get back to this! Based on consensus among
             *  engineers.
             */
            goto error_unlock_checkin;
        }

        if (res->rc != CL_OK) /* If other side indicated error, we need
                               * to free the buffer.
                               */
        {
            if (res->buffer.notification != NULL)
            {
                clHeapFree((void*)res->buffer.notification);
            }
            rc = res->rc;
            goto error_exit;
        }

        /* All fine, need to copy buffer */
        if (notificationBuffer->notification == NULL) /* we provide array */
        {
            memcpy(notificationBuffer, &res->buffer,
                   sizeof(*notificationBuffer)); /* This takes care of array */
            shouldFreeNotification = CL_FALSE;
        }
        else
        { /* caller provided array with fixed given size; we need to copy if
           * there is enough space.
           */
            if (notificationBuffer->numberOfItems >=
                res->buffer.numberOfItems)
            {
                /* Copy array, as much as we can */
                memcpy((void*)notificationBuffer->notification,
                       (void*)res->buffer.notification,
                       res->buffer.numberOfItems *
                          sizeof(ClGmsClusterNotificationT));
            }
            /*
             * Instead of copying the rest of the fields in buffer one-by-one,
             * we do a trick: relink the above array and than copy the entire
             * struck over.  This will keep working even if the buffer struct
             * grows in the future; without change here.
             */
            clHeapFree((void*)res->buffer.notification);
            res->buffer.notification = notificationBuffer->notification;
            memcpy((void*)notificationBuffer, (void*)&res->buffer,
                   sizeof(*notificationBuffer));
            shouldFreeNotification = CL_FALSE;
        }
    }
    else
    {
        clLog(TRACE,CLM,NA, "Sending Async RMD to GMS server for cluster track"); 
        /* No sync response requested, so we call the simple rmd call */
        rc = cl_gms_cluster_track_rmd(&req, 0 /* use def. timeout */, &res);
        clLog(TRACE,CLM,NA, "Cluster track RMD returned");
        /* No sync response requested, so we call the simple rmd call */
        if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
        {
            goto error_unlock_checkin;
        }
        
        rc = res->rc;
    }     

error_exit:
    if(shouldFreeNotification == CL_TRUE )
    {
        clHeapFree((void*)res->buffer.notification);
    }
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
error_checkin:
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        clLog(ERROR,CLM,NA,"clHandleCheckin Failed");
    }
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Track Stop API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterTrackStop(
    CL_IN const ClGmsHandleT gmsHandle)
{
    ClRcT                           rc = CL_OK;
    struct gms_instance            *gms_instance_ptr = NULL;
    ClGmsClusterTrackStopRequestT   req = {0};
    ClGmsClusterTrackStopResponseT *res = NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_INVALID_HANDLE);
    }
    
    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    req.gmsHandle = gmsHandle;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId));
    
    CL_ASSERT(rc == CL_OK); /* Should really never happen */
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_cluster_track_stop_rmd(&req, 0 /* use def. timeout */, &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_exit;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);

error_exit:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);

    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return CL_GMS_RC(rc);
}

/*-----------------------------------------------------------------------------
 * Cluster Member Get API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterMemberGet(
    CL_IN  const  ClGmsHandleT         gmsHandle,
    CL_IN  const  ClGmsNodeIdT         nodeId, 
    CL_IN  const  ClTimeT              timeout,
    CL_OUT ClGmsClusterMemberT* const clusterMember)
{
    ClRcT                           rc = CL_OK;
    struct gms_instance            *gms_instance_ptr= NULL;
    ClGmsClusterMemberGetRequestT   req = {0};
    ClGmsClusterMemberGetResponseT *res= NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    if (clusterMember == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
     
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    
    req.gmsHandle = gmsHandle;
    req.nodeId    = nodeId;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_cluster_member_get_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
    memcpy((void*)clusterMember, (void*)&res->member,
           sizeof(ClGmsClusterMemberT));

error_exit:
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }
    
    return CL_GMS_RC(rc);
}


/*-----------------------------------------------------------------------------
 * Cluster Member Get Async API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterMemberGetAsync(
    CL_IN const ClGmsHandleT   gmsHandle,
    CL_IN const ClInvocationT  invocation,
    CL_IN const ClGmsNodeIdT   nodeId)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterMemberGetAsyncRequestT   req = {0};
    ClGmsClusterMemberGetAsyncResponseT *res = NULL;
        
    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    if (gms_instance_ptr->callbacks.clGmsClusterMemberGetCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin;
    }
    
    req.gmsHandle  = gmsHandle;
    req.nodeId     = nodeId;
    req.invocation = invocation;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId));
    if (rc != CL_OK)
    {
        goto error_checkin;
    }
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_cluster_member_get_async_rmd(&req, 0 /* use def. timeout */,
                                             &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }

error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
error_checkin:
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Leader Elect API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterLeaderElect(
    CL_IN const ClGmsHandleT                      gmsHandle,
    CL_IN const ClGmsNodeIdT                      preferredLeader,
    CL_INOUT    ClGmsNodeIdT                     *leader,
    CL_INOUT    ClGmsNodeIdT                     *deputy,
    CL_INOUT    ClBoolT                          *leadershipChanged)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterLeaderElectRequestT      req = {0};
    ClGmsClusterLeaderElectResponseT    *res = NULL;
    
    if ((leader == NULL) || (deputy == NULL) || (leadershipChanged == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    
    clGmsMutexLock( gms_instance_ptr->response_mutex);
    req.gmsHandle = gmsHandle;
    req.preferredLeaderNode = preferredLeader;
    
    rc = cl_gms_cluster_leader_elect_rmd(&req, 0 /* use def. timeout */, &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    *leader = res->leader;
    *deputy = res->deputy;
    *leadershipChanged = res->leadershipChanged;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Member Eject API
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterMemberEject(
    CL_IN const ClGmsHandleT                      gmsHandle,
    CL_IN const ClGmsNodeIdT                      nodeId,
    CL_IN const ClGmsMemberEjectReasonT           reason)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsClusterMemberEjectRequestT      req = {0};
    ClGmsClusterMemberEjectResponseT    *res = NULL;
    
    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (gms_instance_ptr == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    clGmsMutexLock( gms_instance_ptr->response_mutex);
    
    req.gmsHandle = gmsHandle;
    req.nodeId    = nodeId;
    req.reason    = reason;
    
    rc = cl_gms_cluster_member_eject_rmd(&req, 0 /* use def. timeout */, &res);
    if ((rc != CL_OK) || (res == NULL)) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

    
    return rc;
}

/*-----------------------------------------------------------------------------
 * API used from CPM to notify GMS about the event server comp up.
 *---------------------------------------------------------------------------*/

ClRcT clGmsCompUpNotify (
        CL_IN  ClUint32T         compId)
{
    ClRcT                          rc = CL_OK;
    ClGmsCompUpNotifyRequestT      req = {0};
    ClGmsCompUpNotifyResponseT    *res = NULL;

    CL_GMS_SET_CLIENT_VERSION( req );

    req.compId = compId;

    rc = cl_gms_comp_up_notify_rmd(&req, 0, &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        return CL_GMS_RC(rc);
    }
    CL_ASSERT(res != NULL);

    rc = res->rc;

    clHeapFree((void*)res);
    return CL_GMS_RC(rc);
}


/******************************************************************************
 * CALLBACK HANDLER FUNCTIONS
 *****************************************************************************/
/*----------------------------------------------------------------------------
 *  Cluster Track Callback Handler
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterTrackCallbackHandler(
    CL_IN   ClGmsClusterTrackCallbackDataT* const res)
{
    ClRcT rc = CL_OK;
    struct gms_instance *gms_instance_ptr = NULL;
    ClGmsHandleT gmsHandle = CL_GMS_INVALID_HANDLE;

    CL_ASSERT(res != NULL);
    clLog(INFO,NA,NA,"received cluster track callback");

    gmsHandle = res->gmsHandle;
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        goto error_free_res;
    }

    if (gms_instance_ptr == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NULL_POINTER);
        goto error_free_res;
    }

    if (gms_instance_ptr->callbacks.clGmsClusterTrackCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin_free_res;
    }

    /*
     * Calling the user's callback function with the data.  The user cannot
     * free the data we provide.  If it needs to reatin it, it has to copy
     * it out from what we provide here.
     */
            (*gms_instance_ptr->callbacks.clGmsClusterTrackCallback)
            (&res->buffer, res->numberOfMembers, res->rc);
  

error_checkin_free_res:
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

error_free_res:
    /* Need to free data (res) if are not able to call the actual callback */
    if (res->buffer.notification != NULL)
    {
        clHeapFree((void*)res->buffer.notification);
    }
    clHeapFree((void*)res);
    return rc;
}


/*----------------------------------------------------------------------------
 *  Cluster Member Get Callback Handler
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterMemberGetCallbackHandler(
    CL_IN   ClGmsClusterMemberGetCallbackDataT* const res)
{
    ClRcT rc = CL_OK;
    struct gms_instance *gms_instance_ptr = NULL;
    ClGmsHandleT gmsHandle = CL_GMS_INVALID_HANDLE;
    
    CL_ASSERT(res != NULL);
    
    gmsHandle = res->gmsHandle;
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        goto error_free_res;
    }

    if (gms_instance_ptr == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NULL_POINTER);
        goto error_free_res;
    }
    
    if (gms_instance_ptr->callbacks.clGmsClusterMemberGetCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin_free_res;
    }

    /*
     * Calling the user's callback function with the data.  The user cannot
     * free the data we provide.  If it needs to reatin it, it has to copy
     * it out from what we provide here.
     */
    (*gms_instance_ptr->callbacks.clGmsClusterMemberGetCallback)
                              (res->invocation, &res->member, res->rc);

error_checkin_free_res:
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }

error_free_res:
    /* Need to free data (res) if are not able to call the actual callback */
    clHeapFree((void*)res);
    
    return rc;
}

/*----------------------------------------------------------------------------
 *  Cluster Member Eject Callback Handler
 *---------------------------------------------------------------------------*/
ClRcT clGmsClusterMemberEjectCallbackHandler(
    CL_IN   ClGmsClusterMemberEjectCallbackDataT* const res)
{
    ClRcT rc = CL_OK;
    struct gms_instance *gms_instance_ptr = NULL;
    ClGmsHandleT gmsHandle = CL_GMS_INVALID_HANDLE;
    
    CL_ASSERT(res != NULL);
    
    gmsHandle = res->gmsHandle;
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        goto error_free_res;
    }

    if (gms_instance_ptr == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NULL_POINTER);
        goto error_free_res;
    }

    
    if (gms_instance_ptr->
                cluster_manage_callbacks.clGmsMemberEjectCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin_free_res;
    }
    
    /*
     * Calling the user's callback function with the data.  The user cannot
     * free the data we provide.  If it needs to reatin it, it has to copy
     * it out from what we provide here.
     */
    (*gms_instance_ptr->cluster_manage_callbacks.clGmsMemberEjectCallback)
                               (res->reason);
     
error_checkin_free_res:
    if (clHandleCheckin(handle_database, gmsHandle))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,
                ("\nclHandleCheckin failed"));
    }


error_free_res:
    clHeapFree((void*)res);
    
    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT clGmsGroupMemberGetCallbackHandler(
    CL_IN   ClGmsGroupMemberGetCallbackDataT *res)
{
    return CL_ERR_NOT_IMPLEMENTED;
}



/*-----------------------------------------------------------------------------
 * Group Member Get API
 *---------------------------------------------------------------------------*/
ClRcT clGmsGroupMemberGet(
    CL_IN  ClGmsHandleT         gmsHandle,
    CL_IN  ClGmsGroupIdT        groupId,
    CL_IN  ClGmsMemberIdT       memberId,
    CL_IN  ClTimeT              timeout,
    CL_OUT ClGmsGroupMemberT   *groupMember)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Member Get Async API
 *---------------------------------------------------------------------------*/
ClRcT clGmsGroupMemberGetAsync(
    CL_IN ClGmsHandleT         gmsHandle,
    CL_IN ClInvocationT        invocation,
    CL_IN ClGmsGroupIdT        groupId,
    CL_IN ClGmsMemberIdT       memberId)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Leader Elect API
 *---------------------------------------------------------------------------*/
ClRcT clGmsGroupLeaderElect(
    CL_IN ClGmsHandleT                      gmsHandle,
    CL_IN ClGmsGroupIdT                     groupId)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Member Eject API
 *---------------------------------------------------------------------------*/
ClRcT clGmsGroupMemberEject(
    CL_IN ClGmsHandleT                      gmsHandle,
    CL_IN ClGmsGroupIdT                     groupId,
    CL_IN ClGmsMemberIdT                    memberId,
    CL_IN ClGmsMemberEjectReasonT           reason)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*---------------------------------------------------------------------------*/
ClRcT clGmsGroupMemberEjectCallbackHandler(
    CL_IN   ClGmsGroupMemberEjectCallbackDataT *res)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


ClRcT clGmsSelectionObjectGet (
        const ClGmsHandleT clmHandle,
        ClSelectionObjectT* const pSelectionObject
        )
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT 
clGmsDispatch (
        const ClGmsHandleT clmHandle,
        const ClDispatchFlagsT dispatchFlags
        )
{
    return CL_ERR_NOT_SUPPORTED;
}


/*---------------------------------------------------------------------------*/
ClRcT clGmsGroupJoinAsync(
    CL_IN ClGmsHandleT                      gmsHandle,
    CL_IN ClGmsGroupIdT                     groupId,
    CL_IN ClGmsMemberIdT                    memberId,
    CL_IN ClGmsMemberNameT                 *memberName,
    CL_IN ClGmsLeadershipCredentialsT       credentials)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*---------------------------------------------------------------------------*/
ClRcT clGmsGroupLeaveAsync(
    CL_IN ClGmsHandleT                      gmsHandle,
    CL_IN ClGmsGroupIdT                     groupId,
    CL_IN ClGmsMemberIdT                    memberId)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


