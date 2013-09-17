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
 * ModuleName  : amf
 * File        : clAmsSAClientApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the AMS client library implementation.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#include <string.h>

#include <clAmsDebug.h>
#include <clAmsErrors.h>
#include <clAmsSACommon.h>
#include <clAmsSAClientApi.h>
#include <clAmsSAClientRmd.h>
#include <clHandleApi.h>
#include <clVersionApi.h>
#include <clDebugApi.h>
#include <clAmsUtils.h>

/******************************************************************************
 Local and Static Variables and functions 
*****************************************************************************/

/* 
 * Flag to show if library is initialized 
 */

static ClBoolT lib_initialized = CL_FALSE;

/*
 * Versions supported
 */

static ClVersionT versions_supported[] = 
{
    { 'B', 1, 1 }
};

static ClVersionDatabaseT version_database = {
    sizeof (versions_supported) / sizeof (ClVersionT),
    versions_supported
};

/* 
 * Context for a AMS "instance" - i.e. one user of AMS service 
 */

struct ams_instance 
{
    ClAmsClientHandleT              server_handle;

   /*
    * User's callback functions
    */
   
    ClAmsSAClientCallbacksT         callbacks; 

    /*
     * Indicate that finalize has been called
     */
                                               
    ClBoolT                         finalize; 
    ClOsalMutexIdT                  response_mutex;

};

/* 
 * Instance handle database 
 */

static ClHandleDatabaseHandleT handle_database;

/******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 *****************************************************************************/

static void ams_handle_instance_destructor(
        void *notused)
{
    /* 
     * nothing to do 
     */
}

/******************************************************************************
  Static Functions
*******************************************************************************/
static ClRcT
clAmsLibInitialize(void)
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *eo;

    if (lib_initialized == CL_FALSE)
    {
        if ( (rc = clHandleDatabaseCreate(
                        ams_handle_instance_destructor,
                        &handle_database))
                != CL_OK )
            goto error_exit;

        /*
         * XXX: On the long run, when multiple EOs can exist in same process,
         * the following function installation may be devorced from the lib
         * initialization, so that it can be called multiple times.
         */

        if ( ( rc = clEoMyEoObjectGet(
                        &eo)) 
                != CL_OK )
            goto error_exit;

        if ( ( rc = clEoClientInstall(
                        eo,
                        CL_AMS_CLIENT_TABLE_ID,
                        cl_ams_client_callback_list,
                        /*
                         * cdata passsed to each function call 
                         */
                        0,
                        (int)(sizeof(cl_ams_client_callback_list)/
                              sizeof(ClEoPayloadWithReplyCallbackT))))
                != CL_OK )
            goto error_exit;
        lib_initialized = CL_TRUE;
    }

    return CL_OK;

error_exit:

    return CL_AMS_RC (rc);
}

/***************************************************************************/
static ClRcT
check_lib_init(void)
{
    ClRcT rc = CL_OK;

    if (lib_initialized == CL_FALSE)
    {
        rc = clAmsLibInitialize();
    }

     return CL_AMS_RC (rc);
}

/******************************************************************************
 * EXPORTED API FUNCTIONS
 *****************************************************************************/

/*
 * clAmsSAInitialize
 * ---------------------
 * Initialize/Start the use of the AMS client API library.
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

ClRcT
clAmsSAInitialize(
        CL_OUT      ClAmsClientHandleT                      *amsHandle,
        CL_IN       const ClAmsSAClientCallbacksT           *amsClientCallbacks,
        CL_INOUT    ClVersionT                              *version )
{

    ClRcT                                       rc = CL_OK; 
    clAmsClientInitializeRequestT               req;
    clAmsClientInitializeResponseT              *res = NULL;
    struct ams_instance                         *ams_instance = NULL;


    if (amsHandle == NULL || version == NULL)
    {
        return CL_AMS_RC(CL_ERR_NULL_POINTER);
    }
   
    /* 
     * Initialize the library and database 
     */

    if ((rc = check_lib_init())
            != CL_OK)
        goto error;

    /* 
     * Verify the version information 
     */

    if (( rc = clVersionVerify (
                    &version_database,
                    version))
            != CL_OK)
        goto error;

    /* 
     * Create the handle  
     */

    if ((rc = clHandleCreate (
                    handle_database,
                    sizeof(struct ams_instance),
                    amsHandle))
            != CL_OK)
        goto error;

    /* 
     * Check-Out the handle 
     */

    if ((rc = clHandleCheckout(
                    handle_database,
                    *amsHandle,
                    (void *)&ams_instance))
            != CL_OK)
        goto error;


    /* 
     * Initialize instance entry 
     */

    if (amsClientCallbacks) 
    {
        memcpy(&ams_instance->callbacks, amsClientCallbacks, sizeof(ClAmsSAClientCallbacksT));
    } 
    else 
    {
        memset(&ams_instance->callbacks, 0, sizeof(ClAmsSAClientCallbacksT));
    }

     if ( ( rc = clOsalMutexCreate(&ams_instance->response_mutex)) != CL_OK )
         goto error;

    /* 
     * Inform the server 
     */

    req.handle = *amsHandle;
    if ( (rc = cl_ams_client_initialize(
                    &req,
                    &res))
            != CL_OK)
        goto error;

    /* 
     * Decrement handle use count and return 
     */

    if ((rc = clHandleCheckin(
                    handle_database,
                    *amsHandle))
            != CL_OK)
        goto error;
   
    clHeapFree((void*)res);
    res = NULL;
    return CL_OK;

error:

    clHeapFree((void*)res);
    res = NULL;
    return CL_AMS_RC(rc);
}

/*
 * clAmsSAFinalize
 * -----------------
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

ClRcT
clAmsSAFinalize(
        CL_IN       ClAmsClientHandleT          amsHandle)
{

    ClRcT                                       rc;
    clAmsClientFinalizeRequestT                 req;
    clAmsClientFinalizeResponseT                *res = NULL;
    struct ams_instance                         *ams_instance = NULL;

    /* 
     * Checkout the client handle 
     */

    if ( (rc = clHandleCheckout(
                    handle_database,
                    amsHandle,
                    (void *)&ams_instance))
            != CL_OK )
        goto error;

    if ( ( rc = clOsalMutexLock(ams_instance->response_mutex)) != CL_OK )
        goto error;

    /*
     * Another thread has already started finalizing
     */

    if (ams_instance->finalize)
    {
        if (( rc = clOsalMutexUnlock(ams_instance->response_mutex)) != CL_OK )
            goto error;
        clHandleCheckin(handle_database, amsHandle);
        rc = CL_ERR_INVALID_HANDLE;
        goto error;
    }

    ams_instance->finalize = 1;

    if ( (rc = clOsalMutexUnlock(ams_instance->response_mutex)) != CL_OK )
        goto error;

    /* 
     * Send the information to the server to finalize the 
     * corresponding handle 
     */

    req.handle = ams_instance->server_handle;
    if ( (rc = cl_ams_client_finalize(
                    &req,
                    &res))
            != CL_OK )
        goto error;

    /*
     * Delete the Mutex
     */

    if ( (rc = clOsalMutexDelete (ams_instance->response_mutex) ) != CL_OK )
        goto error;

    /* 
     * Destroy the handle 
     */

    if ( (rc = clHandleDestroy(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    /* 
     * Check-In the handle 
     */

    if ( (rc = clHandleCheckin(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    clHeapFree((void*)res);
    res = NULL;
    return CL_OK;

error:

    clHeapFree((void*)res);
    res = NULL;
    return CL_AMS_RC(rc);

}

/*
 * clAmsSACSIHAStateGet
 * ------------------------
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

ClRcT
clAmsSACSIHAStateGet(
        CL_IN   ClAmsClientHandleT              amsHandle,
        CL_IN   const SaNameT                   *compName,
        CL_IN   const SaNameT                   *csiName,
        CL_OUT  ClAmsHAStateT                   *haState)
{

    ClRcT                                           rc;
    struct ams_instance                             *ams_instance = NULL;


    if ( !compName || !csiName || !haState)
        return CL_ERR_NULL_POINTER;
    /* 
     * Checkout the client handle 
     */

    if ( (rc = clHandleCheckout(
                    handle_database,
                    amsHandle,
                    (void *)&ams_instance))
            != CL_OK )
        goto error;

    /* 
     * Send the information to the server to 
     */

    if ( ( rc = _clAmsSACSIHAStateGet(
                    (SaNameT *)compName,
                    (SaNameT *)csiName,
                    haState ))
            != CL_OK )
        goto error;

    if ( (rc = clHandleCheckin(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    return CL_OK;

error:

    return CL_AMS_RC(rc);

}

/*
 * clAmsSACSIQuiescingComplete
 * -------------------------------
 * Get HA state associated with a CSI
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

ClRcT
clAmsSACSIQuiescingComplete(
        CL_IN   ClAmsClientHandleT              amsHandle,
        CL_IN   ClInvocationT                   invocation,
        CL_IN   ClRcT                           error)
{

    ClRcT                                                   rc;
    struct ams_instance                                     *ams_instance = NULL;

    /* 
     * Checkout the client handle 
     */

    if ( (rc = clHandleCheckout(
                    handle_database,
                    amsHandle,
                    (void *)&ams_instance))
            != CL_OK )
        goto error;

    /* 
     * Check-In the handle 
     */

    if ( (rc = clHandleCheckin(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    return CL_OK;

error:

    return CL_AMS_RC(rc);
}

/*
 * clAmsSAPGTrack
 * ------------------
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

ClRcT
clAmsSAPGTrack(
        CL_IN   ClAmsClientHandleT              amsHandle,
        CL_IN   SaNameT                         *csiName,
        CL_IN   ClUint8T                        trackFlags,
        CL_IN   ClAmsPGNotificationBufferT      *notificationBuffer)
{
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*
 * clAmsSAPGTrackStop
 * ----------------------
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

ClRcT
clAmsSAPGTrackStop(
        CL_IN   ClAmsClientHandleT              amsHandle,
        CL_IN   const SaNameT                   *csiName)
{

    ClRcT                                       rc;
    clAmsClientPGTrackStopRequestT              req;
    clAmsClientPGTrackStopResponseT             *res = NULL;
    struct ams_instance                         *ams_instance = NULL;


    if ( !csiName )
        return CL_ERR_NULL_POINTER;

    /* 
     * Checkout the client handle 
     */

    if ( (rc = clHandleCheckout(
                    handle_database,
                    amsHandle,
                    (void *)&ams_instance))
            != CL_OK )
        goto error;

    /* 
     * Copy the client information in the req structure 
     */

    req.handle = ams_instance->server_handle;

    memcpy (&req.csiName, csiName, sizeof(SaNameT) );

    /* 
     * Send the information to the server to 
     */

    if ( (rc = cl_ams_client_pg_track_stop(
                    &req,
                    &res))
            != CL_OK )
        goto error;

    /* 
     * Check-In the handle 
     */

    if ( (rc = clHandleCheckin(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    clHeapFree((void*)res);
    res = NULL;
    return CL_OK;

error:

    clHeapFree((void*)res);
    res = NULL;
    return CL_AMS_RC(rc);
}

/*
 * clAmsSAResponse
 * -------------------
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

ClRcT
clAmsSAResponse(
        CL_IN   ClAmsClientHandleT              amsHandle,
        CL_IN   ClInvocationT                   invocation,
        CL_IN   ClRcT                           error)
{

    ClRcT                                       rc;
    clAmsClientResponseRequestT                 req;
    clAmsClientResponseResponseT                *res = NULL;
    struct ams_instance                         *ams_instance = NULL;


    /* 
     * Checkout the client handle 
     */

    if ( (rc = clHandleCheckout(
                    handle_database,
                    amsHandle,
                    (void *)&ams_instance))
            != CL_OK )
        goto error;

    /* 
     * Copy the client information in the req structure 
     */

    req.handle = ams_instance->server_handle;
    req.invocation = invocation;
    req.error =error; 


    /* 
     * Send the information to the server to 
     */

    if ( (rc = cl_ams_client_response(
                    &req,
                    &res))
            != CL_OK )
        goto error;

    /* 
     * Check-In the handle 
     */

    if ( (rc = clHandleCheckin(
                    handle_database,
                    amsHandle))
            != CL_OK )
        goto error;

    clHeapFree((void*)res);
    res = NULL;
    return CL_OK;

error:

    clHeapFree((void*)res);
    res = NULL;
    return CL_AMS_RC(rc);

}
ClRcT
clAmsSAComponentInstantiateCallback (
        SaNameT             *compName )
{
    return CL_OK;
}
ClRcT
clAmsSAProxyComponentInstantiateCallback (
        SaNameT             *compName )
{
    return CL_OK;
}
ClRcT
clAmsSAComponentTerminateCallback (
        SaNameT             *compName )
{
    return CL_OK;
}

ClRcT
clAmsSAComponentCleanupCallback (
        SaNameT             *compName )
{
    return CL_OK;
}

ClRcT
clAmsSAComponentRestartCallback (
        SaNameT             *compName )
{

    return CL_OK;
}

ClRcT
clAmsSACSISetCallback(
        SaNameT             *compName,
        ClInvocationT       invocation,
        ClAmsHAStateT       haState,
        ClAmsCSIDescriptorT csiDescriptor )
{
    /*
     * Print the CSI Descriptor
     */


    if ( csiDescriptor.csiFlags != CL_AMS_CSI_FLAG_TARGET_ALL )


    for ( int i =0; i<csiDescriptor.csiAttributeList.numAttributes; i++ )
    {
    }


    return CL_OK;
}

ClRcT
clAmsSACSIRemoveCallback(
        SaNameT             *compName,
        ClInvocationT       invocation,
        ClAmsCSIDescriptorT csiDescriptor )
{


    if ( csiDescriptor.csiFlags != CL_AMS_CSI_FLAG_TARGET_ALL )

    for ( int i =0; i<csiDescriptor.csiAttributeList.numAttributes; i++ )
    {
    }

    return CL_OK;
}

