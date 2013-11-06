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
 * File        : clAmsMgmtServerApi.c
 ******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS management 
 * API. The management API is used by a management client to control the
 * AMS entity.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 ******************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <string.h>
#include <clEoApi.h>
#include <clAmsErrors.h>
#include <clHandleApi.h>
#include <clHandleIpi.h>
#include <clAmsMgmtServerApi.h>
#include <clAmsMgmtCommon.h>
#include <clAmsModify.h>
#include <clAmsServerUtils.h>
#include <clAmsPolicyEngine.h>
#include <clAmsMgmtServerRmd.h>
#include <clAmsXdrHeaderFiles.h>
#include <clAmsMgmtHooks.h>
#include <clAmsSAServerApi.h>
#include <clAmsCkpt.h>
#include <clAmsMgmtMigrate.h>
#include <clAmsEntityUserData.h>
#include <custom.h>
#include <clCpmInternal.h>

/******************************************************************************
 * Data structures
 *****************************************************************************/

/* 
 * global instance of ams 
 */

extern ClAmsT  gAms;
extern ClAmsEntityParamsT gClAmsEntityParams[];
extern ClAmsSGConfigT gClAmsSGDefaultConfig;

#if defined (POST_RC2) || defined (CL_AMS_MGMT_HOOKS)

struct ams_server_instance 
{
    ClAmsMgmtHandleT  client_handle;
    ClBoolT  finalize;
    ClOsalMutexIdT  response_mutex;
    ClIocAddressT clientAddress;
};

/* 
 * instance handle database 
 */

static ClHandleDatabaseHandleT  handle_database;

/* 
 * Flag to show if library is initialized 
 */

static ClBoolT  lib_initialized = CL_FALSE;

/* 
 * Destructor to be called when deleting AMS handles 
 */

static void 
ams_handle_instance_destructor( ClPtrT );

static void 
ams_handle_instance_destructor( ClPtrT notused)
{
}

/******************************************************************************
 * Library Management Functions
 *****************************************************************************/

static ClRcT 
clAmsLibInitialize(void)
{

    if (lib_initialized == CL_FALSE)
    {
        AMS_CALL ( clHandleDatabaseCreate(
                    ams_handle_instance_destructor,
                    &handle_database) );

        lib_initialized = CL_TRUE;
    }

    return CL_OK;

}

static ClRcT 
check_lib_init(void)
{

    if (lib_initialized == CL_FALSE)
    {
       return  clAmsLibInitialize();
    }

     return CL_OK;
}

#endif

#if defined (CL_AMS_MGMT_HOOKS)

#define CL_AMS_MGMT_ENTITY_ADMIN_DEFAULT_TIMEOUT {.tsSec=10,.tsMilliSec=0}

#define CL_AMS_MGMT_CLIENT_FN_ID CL_EO_GET_FULL_FN_NUM(CL_AMS_MGMT_CLIENT_TABLE_ID,0)

#define CL_AMS_MGMT_HOOK_ID(id) do {                                \
        if(gClAmsMgmtEntityAdminList.currentHookId+1>               \
           gClAmsMgmtEntityAdminList.currentHookId)                 \
        {                                                           \
            id=++gClAmsMgmtEntityAdminList.currentHookId;           \
        }                                                           \
        else                                                        \
        {                                                           \
            id=gClAmsMgmtEntityAdminList.currentHookId=1;           \
            clLogCritical(NULL,NULL,                                \
                          "HookID overflow.Resetting hookId to 1"); \
        }                                                           \
}while(0)

#define CL_AMS_MGMT_CHECK_AND_ADD_HOOK(handle,entity,oper,lock) do {    \
        struct ams_server_instance *_ams_server_instance=NULL;          \
        rc=clHandleCheckout(handle_database,handle,(ClPtrT*)&_ams_server_instance); \
        if(rc==CL_OK)                                                   \
        {                                                               \
            rc=clAmsMgmtHookAdd(entity,oper,handle,_ams_server_instance->client_handle,&_ams_server_instance->clientAddress); \
            if(rc != CL_OK)                                             \
            {                                                           \
                clHandleCheckin(handle_database,handle);                \
                clOsalMutexUnlock(lock);                                \
                goto exitfn;                                            \
            }                                                           \
            clHandleCheckin(handle_database,handle);                    \
        }                                                               \
        else                                                            \
        {                                                               \
            clOsalMutexUnlock(lock);                                    \
            goto exitfn;                                                \
        }                                                               \
}while(0)


/* 
 * We have a list head of pending operations based on the entity type
 * and entity admin operation type.
 * There cannot be more than 1 admin op. on the same entity.
 */
typedef struct ClAmsMgmtEntityAdminOper
{
    ClHandleT mgmtHandle;
    ClHandleT clientHandle;
    ClTimerHandleT timerHandle;
    ClAmsEntityTypeT type;
    ClAmsMgmtAdminOperT oper;
    ClIocAddressT clientAddress;
    ClUint64T hookId;
}ClAmsMgmtEntityAdminOperT;

typedef struct ClAmsMgmtEntityAdminList
{
    ClAmsMgmtEntityAdminOperT *pEntityOpList[CL_AMS_ENTITY_TYPE_MAX+1][CL_AMS_MGMT_ADMIN_OPER_MAX];
    ClOsalMutexT mutex;
    ClUint64T currentHookId;
}ClAmsMgmtEntityAdminListT;

static ClAmsMgmtEntityAdminListT gClAmsMgmtEntityAdminList;

static ClRcT clAmsMgmtHookTimeOut(void *pArg);

static ClRcT clAmsMgmtHookAdd(ClAmsEntityT *pEntity,ClAmsMgmtAdminOperT oper,ClHandleT mgmtHandle,ClHandleT clientHandle,ClIocAddressT *pClientAddress);

#else

#define CL_AMS_MGMT_CHECK_AND_ADD_HOOK(handle,entity,oper,lock)

#endif


/*
 * clAmsMgmtInitialize
 * -------------------
 * Initialize/Start the use of the management API library.
 *
 */

ClRcT
VDECL(_clAmsMgmtInitialize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtInitializeRequestT  req = {0};
    clAmsMgmtInitializeResponseT  res = {0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtInitializeRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

#if defined (POST_RC2) || defined (CL_AMS_MGMT_HOOKS)

    ClAmsMgmtHandleT  handle = 0;
    struct ams_server_instance  *ams_server_instance = NULL;


    /* 
     * Check if the handle database has been created and initialized 
     */

    AMS_CALL( check_lib_init() );


    /*
     * Create a corresponding handle to be sent back to the client 
     */

    AMS_CALL( clHandleCreate(
                handle_database,
                sizeof(struct ams_server_instance),
                &handle) );

    /* 
     * Checkout the handle to update the value of the client handle 
     */

    AMS_CALL( clHandleCheckout(
                handle_database,
                handle,
                (ClPtrT)&ams_server_instance) );
           
    /*
     * Update the value of client handle obtained from the client 
     */

    ams_server_instance->client_handle = req.handle;

#if defined (CL_AMS_MGMT_HOOKS)
    switch(req.srcAddress.discriminant)
    {
    case CLAMSMGMTIOCADDRESSIDLTIOCPHYADDRESS:
        memcpy(&ams_server_instance->clientAddress.iocPhyAddress,&req.srcAddress.clAmsMgmtIocAddressIDLT.iocPhyAddress,sizeof(ams_server_instance->clientAddress.iocPhyAddress));
        break;
    case CLAMSMGMTIOCADDRESSIDLTIOCLOGICALADDRESS:
        memcpy(&ams_server_instance->clientAddress.iocLogicalAddress,&req.srcAddress.clAmsMgmtIocAddressIDLT.iocLogicalAddress,sizeof(ams_server_instance->clientAddress.iocLogicalAddress));
        break;
    case CLAMSMGMTIOCADDRESSIDLTIOCMULTICASTADDRESS:
        memcpy(&ams_server_instance->clientAddress.iocMulticastAddress,
               &req.srcAddress.clAmsMgmtIocAddressIDLT.iocMulticastAddress,
               sizeof(ams_server_instance->clientAddress.iocMulticastAddress));
        break;
    default:
        AMS_CALL(clHandleCheckin(handle_database,handle));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }
#endif

    AMS_CALL( clOsalMutexCreate(&ams_server_instance->response_mutex) ) ;

    AMS_CALL( clHandleCheckin( handle_database, handle) );

    res.handle = handle;

#endif

    AMS_CALL( VDECL_VER(clXdrMarshallclAmsMgmtInitializeResponseT, 4, 0, 0)(&res,out,0) );

    return CL_OK;

}

/*
 * clAmsMgmtFinalize
 * -----------------
 * Finalize the use of the management API library.
 *
 */

ClRcT
VDECL(_clAmsMgmtFinalize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtFinalizeRequestT  req = {0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtFinalizeRequestT, 4, 0, 0)( in, (ClPtrT)&req) );

#if defined (POST_RC2) || defined (CL_AMS_MGMT_HOOKS)

    ClAmsMgmtHandleT  handle = 0;
    struct ams_server_instance  *ams_server_instance = NULL;

    handle = req.handle;

#ifdef HANDLE_VALIDATE 
    
    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    AMS_CALL( clHandleCheckout(
                handle_database,
                handle,
                (ClPtrT)&ams_server_instance) ); 
    
    AMS_CALL( clOsalMutexLock(ams_server_instance->response_mutex) );

    /*
     * Another thread has already started finalizing
     */

    if (ams_server_instance->finalize) 
    {

        AMS_CALL( clOsalMutexUnlock(ams_server_instance->response_mutex) );
        clHandleCheckin(handle_database, handle);
        return CL_AMS_RC ( CL_ERR_INVALID_HANDLE );
    }

    /* 
     * Update the value of client handle obtained from the client 
     */ 
    
    ams_server_instance->finalize = 1; 
    
    AMS_CALL( clOsalMutexUnlock(ams_server_instance->response_mutex) );

    /*
     * Delete the Mutex
     */

    AMS_CALL( clOsalMutexDelete (ams_server_instance->response_mutex) ); 

    /*
     * Destroy and Check-In the handle 
     */

    AMS_CALL( clHandleDestroy( handle_database, handle) );

    /*
     * Check-In the Handle
     */

    AMS_CALL( clHandleCheckin( handle_database, handle) );

#endif

    return CL_OK;

}

/*
 * clAmsMgmtEntityCreate
 * -------------------
 * Instantiate the  the AMS Entity 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityCreate)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityInstantiateRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};


    AMS_CALL( _clAmsMgmtReadInputBuffer(
                in,
                (ClUint8T*)&req,
                sizeof (clAmsMgmtEntityInstantiateRequestT)) );


#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    /*
     * Instantiate the entity 
     */

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CHECK_ENTITY_TYPE (req.entity.type);

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbAddEntity(
                &gAms.db.entityDb[req.entity.type],
                &entityRef ),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * clAmsMgmtEntityDelete
 * -------------------
 * Terminate the  the AMS Entity 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityDelete)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityTerminateRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_CALL( _clAmsMgmtReadInputBuffer(
                in,
                (ClUint8T*)&req,
                sizeof (clAmsMgmtEntityTerminateRequestT)) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

#ifdef DYNAMIC_CONFIG 

    AMS_CALL( clAmsPeEntityDelete( &entityRef.entity) );

#endif

    AMS_CHECK_ENTITY_TYPE (req.entity.type);

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
            clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[req.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * _clAmsMgmtEntitySetConfig
 * -------------------
 * Set the config portion of the AMS Entity using Entity Name 
 *
 */

static ClRcT
__clAmsMgmtEntitySetConfig(
                           CL_IN  ClEoDataT  data,
                           CL_IN  ClBufferHandleT  in,
                           CL_OUT ClBufferHandleT  out,
                           ClUint32T versionCode)
{

    AMS_FUNC_ENTER (("\n"));

    ClAmsEntityTypeT entityType = CL_AMS_ENTITY_TYPE_ENTITY;
    ClAmsEntityConfigT *entityConfig = NULL;
    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetConfigRequestT  req = { 0 };
    ClHandleT handle = CL_HANDLE_INVALID_VALUE;

    req.entityConfig = clHeapCalloc(1, sizeof(ClAmsEntityConfigT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req.entityConfig);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtEntitySetConfigRequestT, 4, 0, 0)(
                in, (ClPtrT)&req) );

    entityType = req.entityConfig->type;

    if(entityType > CL_AMS_ENTITY_TYPE_MAX)
    {
        AMS_LOG( CL_DEBUG_ERROR, ("EntitySetConfig: invalid entity type "\
                                  "[%d]\n", entityType));
        goto exitfn;
    }

    handle = req.handle;

#ifdef HANDLE_VALIDATE

    AMS_CHECK_RC_ERROR( clHandleValidateHandle(handle_database, handle) );

#endif

    entityConfig = clHeapAllocate( gClAmsEntityParams[entityType].configSize );

    AMS_CHECK_NO_MEMORY_AND_EXIT (entityConfig);

    clAmsFreeMemory (req.entityConfig);

    switch (entityType)
    {
        case CL_AMS_ENTITY_TYPE_NODE:
            { 
                AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallClAmsNodeConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            { 
                switch(versionCode)
                {
                case CL_VERSION_CODE(4, 0, 0):
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 0, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    /*
                     * Disable the feature/
                     */
                    ((ClAmsSGConfigT*)entityConfig)->maxFailovers = 0;
                    ((ClAmsSGConfigT*)entityConfig)->failoverDuration = 
                        gClAmsSGDefaultConfig.failoverDuration;
                    ((ClAmsSGConfigT*)entityConfig)->beta = 0;
                break;

                case CL_VERSION_CODE(4, 1, 0):
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 1, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    /*
                     * Disable the feature/
                     */
                    ((ClAmsSGConfigT*)entityConfig)->beta = 0;
                    break;
 
               default:
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 5, 0, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    break;
                }
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSUConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSIConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCompConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSIConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        default:
            { 
                rc = CL_AMS_ERR_INVALID_ENTITY;
                goto exitfn;
            }
    
    }
 
    req.entityConfig = entityConfig;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
            clAmsEntitySetConfig(
                &gAms.db.entityDb[entityType],
                (ClAmsEntityT*)req.entityConfig,
                req.entityConfig),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

    /*  
     * Call the PE based on the PE Instantiate flag in the request 
     */

#ifdef DYNAMIC_CONFIG 

    if ( req.peInstantiateFlag )
    {
        AMS_CALL( clAmsPeEntityAdd( (ClAmsEntityT*)req.entityConfig ) );
    }

#endif

exitfn:

    clAmsFreeMemory (req.entityConfig);
    return rc;
}

ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 4, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out)
{

    return __clAmsMgmtEntitySetConfig(data, in, out, CL_VERSION_CODE(4, 0, 0));
}

ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 4, 1, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out)
{

    return __clAmsMgmtEntitySetConfig(data, in, out, CL_VERSION_CODE(4, 1, 0));
}

ClRcT
VDECL_VER(_clAmsMgmtEntitySetConfig, 5, 0, 0)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out)
{

    return __clAmsMgmtEntitySetConfig(data, in, out, CL_VERSION_CODE(5, 0, 0));
}

/*
 * clAmsMgmtEntitySetAlphaFactor
 * -------------------
 * Set the alpha factor for the SG to tune the active
 * service units incase we have less than the preferred active 
 * service units
 *
 */

ClRcT
VDECL(_clAmsMgmtEntitySetAlphaFactor)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetAlphaFactorRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntitySetAlphaFactorRequestT, 4, 0, 0)(in, &req));

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    if(req.entity.type != CL_AMS_ENTITY_TYPE_SG)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Alpha factor request on entity other than SG\n"));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
                                        clAmsEntitySetAlphaFactor( entityRef.ptr, req.alphaFactor),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * clAmsMgmtEntitySetBetaFactor
 * -------------------
 * Set the beta factor for the SG to tune the standby
 * service units incase we have less than the preferred standby service units.
 *
 */

ClRcT
VDECL(_clAmsMgmtEntitySetBetaFactor)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetBetaFactorRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntitySetBetaFactorRequestT, 4, 0, 0)(in, &req));

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    if(req.entity.type != CL_AMS_ENTITY_TYPE_SG)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Beta factor request on entity other than SG\n"));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
                                        clAmsEntitySetBetaFactor( entityRef.ptr, req.betaFactor),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}


/*
 * clAmsMgmtEntityLockAssignment
 * -------------------
 * Set the admin state of the AMS Entity to Lock Assignment 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityLockAssignment)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityLockAssignmentRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};


    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityLockAssignmentRequestT, 4, 0, 0)(in, &req));

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityLockAssignment( entityRef.ptr),
            gAms.mutex );

    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

ClRcT
VDECL(_clAmsMgmtEntityForceLock)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityForceLockRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityForceLockRequestT, 4, 0, 0)(in, &req));

    if( (req.entity.type != CL_AMS_ENTITY_TYPE_SU) && (req.entity.type != CL_AMS_ENTITY_TYPE_NODE) )
    {
        AMS_LOG(CL_DEBUG_ERROR,                 
                ("AMF force lock instantiation operation allowed only on SUs and Node. Operation failed on entity [%s]\n",
                 CL_AMS_STRING_ENTITY_TYPE(req.entity.type)));
        return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
    }

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
                                        clAmsPeSUForceLockOperation( (ClAmsSUT*)entityRef.ptr, req.lock),
                                        gAms.mutex );


    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

ClRcT
VDECL_VER(_clAmsMgmtEntityForceLockInstantiation, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityLockInstantiationRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityLockInstantiationRequestT, 4, 0, 0)(in, &req));

//    if(req.entity.type != CL_AMS_ENTITY_TYPE_SU)
//    {
//        AMS_LOG(CL_DEBUG_ERROR, 
//                ("AMF force lock instantiation operation allowed only on SUs. Operation failed on entity [%s]\n",
//                 CL_AMS_STRING_ENTITY_TYPE(req.entity.type)));
//        return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
//    }
    if( (req.entity.type != CL_AMS_ENTITY_TYPE_SU) && (req.entity.type != CL_AMS_ENTITY_TYPE_NODE) )
    {
        AMS_LOG(CL_DEBUG_ERROR,                 
                ("AMF force lock instantiation operation allowed only on SUs and Node. Operation failed on entity [%s]\n",
                 CL_AMS_STRING_ENTITY_TYPE(req.entity.type)));
        return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
    }

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

//    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
//                                        clAmsPeSUForceLockInstantiationOperation( (ClAmsSUT*)entityRef.ptr),
//                                        gAms.mutex );

    if(CL_AMS_ENTITY_TYPE_SU == entityRef.entity.type){
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsPeSUForceLockInstantiationOperation( (ClAmsSUT*)entityRef.ptr), gAms.mutex );
    }
    else if(CL_AMS_ENTITY_TYPE_NODE == entityRef.entity.type){
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsPeNodeForceLockInstantiationOperation( (ClAmsNodeT*)entityRef.ptr), gAms.mutex );
    }
    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * clAmsMgmtEntityLockInstantiation
 * -------------------
 * Set the admin state of the AMS Entity to Lock Instantiation
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityLockInstantiation)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityLockInstantiationRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityLockInstantiationRequestT, 4, 0, 0)(in,&req));

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK( !entityRef.ptr, gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityLockInstantiate(entityRef.ptr),
            gAms.mutex );

    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * clAmsMgmtEntityUnlock
 * -------------------
 * Set the admin state of the AMS Entity to Unlock 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityUnlock)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityUnlockRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityUnlockRequestT, 4, 0, 0)( in,(ClPtrT)&req ));

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    CL_AMS_MGMT_CHECK_AND_ADD_HOOK(req.handle, 
                                   entityRef.ptr, 
                                   CL_AMS_MGMT_ADMIN_OPER_UNLOCK,
                                   gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityUnlock(entityRef.ptr),
            gAms.mutex );

    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

/*
 * clAmsMgmtEntityShutdown
 * -------------------
 * Shutdown the entity 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityShutdown)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityShutdownRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityShutdownRequestT, 4, 0, 0)( in, &req ) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    CL_AMS_MGMT_CHECK_AND_ADD_HOOK(req.handle, 
                                   entityRef.ptr, 
                                   CL_AMS_MGMT_ADMIN_OPER_SHUTDOWN,
                                   gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityShutdown(entityRef.ptr),
            gAms.mutex );

    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

/*
 * clAmsMgmtEntityRestart
 * -------------------
 * Restart the entity : only valid for SU and COMPONENTS
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityRestart)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityRestartRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityRestartRequestT, 4, 0, 0)(in, &req) );
       
    /* 
     * Checkout the handle to see if its a valid handle
     */
       
#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    CL_AMS_MGMT_CHECK_AND_ADD_HOOK(req.handle, 
                                   entityRef.ptr, 
                                   CL_AMS_MGMT_ADMIN_OPER_RESTART,
                                   gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityRestart(
                entityRef.ptr,
                CL_AMS_ENTITY_SWITCHOVER_GRACEFUL),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

/*
 * clAmsMgmtEntityRepaired
 * -------------------
 * Inform the Policy engine that the entity has been repaired 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityRepaired)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityRepairedRequestT  req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};


    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityRepairedRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    CL_AMS_MGMT_CHECK_AND_ADD_HOOK(req.handle, 
                                   entityRef.ptr, 
                                   CL_AMS_MGMT_ADMIN_OPER_RESTART,
                                   gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsPeEntityRepaired(entityRef.ptr),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

/*
 *_clAmsMgmtSGAdjustPreference 
 * -------------------
 * _clAmsMgmtSGAdjustPreference 
 *
 */

ClRcT
VDECL(_clAmsMgmtSGAdjustPreference)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in, 
        CL_OUT  ClBufferHandleT  out)
{
    ClRcT rc = CL_OK;
    clAmsMgmtSGAdjustPreferenceRequestT request = {0};
    ClAmsEntityRefT entityRef = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS server is not running. Dropping sg adjust request"));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL(VDECL_VER(clXdrUnmarshallclAmsMgmtSGAdjustPreferenceRequestT, 4, 0, 0)(in, &request));

#ifdef HANDLE_VALIDATE
    AMS_CALL(clHandleValidateHandle(handle_database, request.handle));
#endif    
    
    memcpy(&entityRef.entity, &request.entity, sizeof(entityRef.entity));

    AMS_CALL (clOsalMutexLock(gAms.mutex) );
    
    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                                                                &entityRef), gAms.mutex );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(clAmsPeSGAdjust((ClAmsSGT*)entityRef.ptr, request.enable),
                                        gAms.mutex);


    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    AMS_CALL(clOsalMutexUnlock(gAms.mutex));

    exitfn:
    return rc;
}

/*
 *_clAmsMgmtSISwap 
 * -------------------
 * _clAmsMgmtSISwap 
 *
 */

ClRcT
VDECL(_clAmsMgmtSISwap)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtSISwapRequestT req = {0};
    ClAmsEntityRefT  entityRef = {{0},0,0};


    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) &&
            (gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("AMS server is not functioning, dropping the request\n"));
        return CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
    }

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtSISwapRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy(&entityRef.entity, &req.entity ,sizeof (ClAmsEntityT));
    entityRef.ptr = NULL;

    if(entityRef.entity.type != CL_AMS_ENTITY_TYPE_SI)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Swap not performed on SI. Entity [%.*s] invalid\n",
                                 req.entity.name.length-1, req.entity.name.value));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityRef.entity.type],
                &entityRef),
            gAms.mutex );

    AMS_CHECKPTR_AND_UNLOCK ( !entityRef.ptr, gAms.mutex );

    CL_AMS_MGMT_CHECK_AND_ADD_HOOK(req.handle, 
                                   entityRef.ptr, 
                                   CL_AMS_MGMT_ADMIN_SI_SWAP,
                                   gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
                                        clAmsPeSISwap((ClAmsSIT*)entityRef.ptr),
                                        gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

/*
 *_clAmsMgmtEntityListEntityRefAdd 
 * -------------------
 * _clAmsMgmtEntityListEntityRefAdd 
 *
 */

ClRcT
VDECL(_clAmsMgmtEntityListEntityRefAdd)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityListEntityRefAddRequestT  req = {0};

    AMS_CALL( _clAmsMgmtReadInputBuffer(
                in,
                (ClUint8T*)&req,
                sizeof (clAmsMgmtEntityListEntityRefAddRequestT)) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    /*
     * Validate that the list is one of the config list and not status list
     */
   
    if ( ( req.entityListName <= CL_AMS_CONFIG_LIST_START ) || 
            ( req.entityListName >= CL_AMS_CONFIG_LIST_END ) )
    {
        rc =  CL_AMS_ERR_INVALID_ENTITY_LIST;
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clAmsIsValidList(&req.sourceEntity,req.entityListName) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsAddToEntityList( 
                &req.sourceEntity,
                &req.targetEntity, 
                req.entityListName),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

ClRcT
VDECL(_clAmsMgmtEntitySetRef)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetRefRequestT  req = {0};
    ClAmsEntityRefT  sourceEntityRef = {{0},0,0};
    ClAmsEntityRefT  targetEntityRef = {{0},0,0};


    AMS_CALL( _clAmsMgmtReadInputBuffer(
                in,
                (ClUint8T*)&req,
                sizeof (clAmsMgmtEntitySetRefRequestT)) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif


    memcpy ( &sourceEntityRef.entity, &req.sourceEntity, sizeof (ClAmsEntityT));
    memcpy ( &targetEntityRef.entity, &req.targetEntity, sizeof (ClAmsEntityT));

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsEntitySetRefPtr(
                sourceEntityRef,
                targetEntityRef),
            gAms.mutex)

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

ClRcT 
VDECL(_clAmsMgmtCSISetNVP)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCSISetNVPRequestT  req = {0};
    ClAmsEntityT  sourceEntity = {0};


    AMS_CALL( _clAmsMgmtReadInputBuffer(
                in,
                (ClUint8T*)&req,
                sizeof (clAmsMgmtCSISetNVPRequestT)) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    memcpy ( &sourceEntity, &req.sourceEntity, sizeof (ClAmsEntityT));

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(
            clAmsCSISetNVP (
                gAms.db.entityDb[req.sourceEntity.type],
                req.sourceEntity,
                req.nvp),
            gAms.mutex );

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;

}

ClRcT
VDECL(_clAmsMgmtDebugEnable)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    clAmsMgmtDebugEnableRequestT  req = {0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtDebugEnableRequestT, 4, 0, 0)( in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    if ( req.entity.type == CL_AMS_ENTITY_TYPE_ENTITY )
    {
        /*
         * Enable logging for all the entities
         */
        gAms.debugFlags |= req.debugFlags;
    }
    else
    {
        AMS_CALL ( clAmsDebugEnable(&req) );
    }

    return CL_OK;

}

ClRcT
VDECL(_clAmsMgmtDebugDisable)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    clAmsMgmtDebugDisableRequestT  req = {0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtDebugDisableRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif
    if ( req.entity.type == CL_AMS_ENTITY_TYPE_ENTITY )
    {
        /*
         * Disable logging for all the entities
         */
        gAms.debugFlags &= ~req.debugFlags;
    }
    else
    {
        AMS_CALL( clAmsDebugDisable (&req) );
    }

    return CL_OK;

}

ClRcT
VDECL(_clAmsMgmtDebugGet)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    clAmsMgmtDebugGetRequestT  req = {0};
    clAmsMgmtDebugGetResponseT  res = {0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtDebugGetRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    if ( req.entity.type == CL_AMS_ENTITY_TYPE_ENTITY )
    {
        /*
         * Get logging for all the entities
         */
        res.debugFlags = gAms.debugFlags;
    }
    else
    {
        AMS_CALL( clAmsDebugGet(&req.entity,&res) );
    }

    AMS_CALL( VDECL_VER(clXdrMarshallclAmsMgmtDebugGetResponseT, 4, 0, 0)(&res,out,0) );

    return CL_OK;

}

ClRcT
VDECL(_clAmsMgmtDebugEnableLogToConsole)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtDebugEnableLogToConsoleRequestT  req = {0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtDebugEnableLogToConsoleRequestT, 4, 0, 0)(
                in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    gAms.debugLogToConsole = CL_TRUE;

    return CL_OK;

}

ClRcT
VDECL(_clAmsMgmtDebugDisableLogToConsole)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtDebugDisableLogToConsoleRequestT  req = {0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtDebugDisableLogToConsoleRequestT, 4, 0, 0)(
                in,(ClPtrT)&req) );

#ifdef HANDLE_VALIDATE

    AMS_CALL( clHandleValidateHandle(handle_database,req.handle) );

#endif

    gAms.debugLogToConsole = CL_FALSE;

    return CL_OK;

}

#if defined (CL_AMS_MGMT_HOOKS)

ClRcT clAmsMgmtHookInitialize(void)
{
    memset(&gClAmsMgmtEntityAdminList,0,sizeof(gClAmsMgmtEntityAdminList));
    clOsalMutexInit(&gClAmsMgmtEntityAdminList.mutex);
    return CL_OK;
}

ClRcT clAmsMgmtHookFinalize(void)
{
    clOsalMutexDestroy(&gClAmsMgmtEntityAdminList.mutex);
    return CL_OK;
}

static ClRcT clAmsMgmtEntityAdminResponse(ClAmsMgmtEntityAdminOperT *pEntityOp,
                                          ClRcT retCode)
{
    ClIocAddressT destAddress;
    ClBufferHandleT inMsg=0;
    ClRcT rc=CL_OK;
    ClUint32T flags=CL_RMD_CALL_NON_PERSISTENT|CL_RMD_CALL_ASYNC;
    ClAmsMgmtEntityAdminResponseT response;

    if((rc=clBufferCreate(&inMsg))!=CL_OK)
    {
        clLogError(NULL,NULL,"Buffer create returned [0x%x]",rc);
        goto out;
    }
    memset(&response,0,sizeof(response));
    response.mgmtHandle = pEntityOp->mgmtHandle;
    response.clientHandle=pEntityOp->clientHandle;
    response.type = pEntityOp->type;
    response.oper = pEntityOp->oper;
    response.retCode = (ClUint32T)retCode;
    if((rc=clXdrMarshallClAmsMgmtEntityAdminResponseT((void*)&response,inMsg,0)) != CL_OK)
    {
        clLogError(NULL,NULL,"Admin response marshall returned [0x%x]",rc);
        goto out_delete;
    }
    memcpy(&destAddress,&pEntityOp->clientAddress,sizeof(destAddress));

    rc=clRmdWithMsg(destAddress,CL_AMS_MGMT_CLIENT_FN_ID,inMsg,0,flags,NULL,NULL);
    if(rc != CL_OK)
    {
        clLogError(NULL,NULL,"Rmd returned [0x%x]",rc);
        goto out;
    }

    goto out;

    out_delete:
    clBufferDelete(&inMsg);

    out:
    return rc;
}


static ClRcT clAmsMgmtHookTimeOut(void *pArg)
{
    ClAmsMgmtEntityAdminOperT *pEntityOp = pArg;
    CL_ASSERT(pArg != NULL);
    clOsalMutexLock(&gClAmsMgmtEntityAdminList.mutex);
    if(pEntityOp != gClAmsMgmtEntityAdminList.pEntityOpList[pEntityOp->type][pEntityOp->oper])
    {
        clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
        goto out;
    }
    gClAmsMgmtEntityAdminList.pEntityOpList[pEntityOp->type][pEntityOp->oper]=NULL;
    clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
    clTimerDelete(&pEntityOp->timerHandle);
    
    /*
     * Check for the validity of the handle
     */
    if(clHandleValidate(handle_database,pEntityOp->mgmtHandle) != CL_OK)
    {
        goto out_free;
    }

    clAmsMgmtEntityAdminResponse(pEntityOp,CL_ERR_TIMEOUT);

    out_free:
    clHeapFree(pEntityOp);

    out:
    return CL_OK;
}

static ClRcT clAmsMgmtHookAdd(ClAmsEntityT *pEntity,ClAmsMgmtAdminOperT oper,ClHandleT mgmtHandle,ClHandleT clientHandle,ClIocAddressT *pClientAddress)
{
    ClAmsMgmtEntityAdminOperT  *pEntityOp = NULL;
    ClTimerHandleT timerHandle = 0;
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    ClTimerTimeOutT timeout = CL_AMS_MGMT_ENTITY_ADMIN_DEFAULT_TIMEOUT;

    if(pEntity == NULL)
    {
        clLogError(NULL,NULL,"Entity is NULL");
        goto out;
    }

    if(oper >= CL_AMS_MGMT_ADMIN_OPER_MAX)
    {
        clLogError(NULL,NULL,"Oper [%d] out of range",oper);
        goto out;
    }

    pEntityOp = clHeapCalloc(1,sizeof(*pEntityOp));
    if(pEntityOp == NULL)
    {
        clLogCritical(NULL,NULL,"Could not allocate [%d] bytes of memory",sizeof(*pEntityOp));
        goto out;
    }
    pEntityOp->mgmtHandle = mgmtHandle;
    pEntityOp->clientHandle = clientHandle;
    pEntityOp->type = pEntity->type;
    pEntityOp->oper = oper;
    memcpy(&pEntityOp->clientAddress,pClientAddress,sizeof(pEntityOp->clientAddress));
    clOsalMutexLock(&gClAmsMgmtEntityAdminList.mutex);
    if(gClAmsMgmtEntityAdminList.pEntityOpList[pEntity->type][oper] != NULL)
    {
        clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
        rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
        goto out_free;
    }
    CL_AMS_MGMT_HOOK_ID(pEntityOp->hookId);
    pEntity->hookId = pEntityOp->hookId;
    gClAmsMgmtEntityAdminList.pEntityOpList[pEntity->type][oper]=pEntityOp;
    clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
    rc = clTimerCreateAndStart(timeout,CL_TIMER_ONE_SHOT,CL_TIMER_SEPARATE_CONTEXT,clAmsMgmtHookTimeOut,(void*)pEntityOp,&timerHandle);
    if(rc != CL_OK)
    {
        goto out_delete;
    }
    pEntityOp->timerHandle = timerHandle;
    goto out;

    out_delete:

    clOsalMutexLock(&gClAmsMgmtEntityAdminList.mutex);
    --gClAmsMgmtEntityAdminList.currentHookId;
    pEntity->hookId = 0;
    gClAmsMgmtEntityAdminList.pEntityOpList[pEntity->type][oper]=NULL;
    clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);

    out_free:
    clHeapFree(pEntityOp);

    out:
    return rc;
}

/*
 * Delete the hook
*/
ClRcT clAmsMgmtHookDel(ClAmsEntityT *pEntity,ClAmsMgmtAdminOperT oper,ClRcT retCode)
{
    ClAmsMgmtEntityAdminOperT *pEntityOp = NULL;
    ClUint64T hookId = pEntity->hookId;
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(pEntity == NULL)
    {
        goto out;
    }

    if(oper >= CL_AMS_MGMT_ADMIN_OPER_MAX)
    {
        goto out;
    }
    
    rc = CL_OK;

    /*First sanity check the hook id*/
    if(!hookId)
    {
        goto out;
    }

    pEntity->hookId = 0;

    clOsalMutexLock(&gClAmsMgmtEntityAdminList.mutex);

    /*Now check if the given hookId has a pending entityOp*/
    if((pEntityOp = gClAmsMgmtEntityAdminList.pEntityOpList[pEntity->type][oper]) == NULL)
    {
        clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
        goto out;
    }

    if(pEntityOp->hookId != hookId)
    {
        clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);
        goto out_free;
    }

    /*
     * HookId matches.So its a tracked admin op that has finished.
     * Provide the feedback if handle is still valid.
     */
    gClAmsMgmtEntityAdminList.pEntityOpList[pEntity->type][oper]=NULL;

    clOsalMutexUnlock(&gClAmsMgmtEntityAdminList.mutex);

    clTimerDelete(&pEntityOp->timerHandle);

    if(clHandleValidate(handle_database,pEntityOp->mgmtHandle) != CL_OK)
    {
        goto out_free;
    }

    clAmsMgmtEntityAdminResponse(pEntityOp,retCode);

    out_free:

    clHeapFree(pEntityOp);

    out:
    return rc;
}

#endif


/*
 * _clAmsMgmtCCBInitialize
 * -------------------
 *
 * Creates and Initializes context for AML configuration APIs.
 *
 */

ClRcT
VDECL(_clAmsMgmtCCBInitialize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBInitializeRequestT  req = {0};
    clAmsMgmtCCBInitializeResponseT  res = {0};

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBInitializeRequestT, 4, 0, 0)(in,(ClPtrT)&req) );

    ClAmsMgmtCCBHandleT  handle = 0;
    clAmsMgmtCCBT   *ccbInstance = NULL;

    /*
     * Create a corresponding handle to be sent back to the client 
     */

    AMS_CALL(clHandleCreate( gAms.ccbHandleDB, sizeof(clAmsMgmtCCBT), &handle));

    AMS_CALL(clHandleCheckout(gAms.ccbHandleDB,handle,(ClPtrT*)&ccbInstance));

    AMS_CALL(clAmsCCBOpListInstantiate(&ccbInstance->ccbOpListHandle)); 

    AMS_CALL( clHandleCheckin( gAms.ccbHandleDB, handle) );

    res.handle = handle;

    AMS_CALL( VDECL_VER(clXdrMarshallclAmsMgmtCCBInitializeResponseT, 4, 0, 0)((ClPtrT)&res,out,0));

    return CL_OK;
}

/*
 * _clAmsMgmtCCBFinalize
 * -----------------
 *
 * deletes and finalizes  CCB context  
 *
 */

ClRcT
VDECL(_clAmsMgmtCCBFinalize)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBFinalizeRequestT  req = {0};
    ClAmsMgmtCCBHandleT  handle = CL_HANDLE_INVALID_VALUE;
    clAmsMgmtCCBT   *ccbInstance = NULL;
    ClRcT  rc = CL_OK;

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBFinalizeRequestT, 4, 0, 0)( in, (ClPtrT)&req) );

    handle = req.handle; 

    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR(clHandleCheckout(gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    /*
     * XXX: Cleanup all the operations in the CCBOpList and delete the list
     * Need to consider multiple finalize calls
     */

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    AMS_CHECK_RC_ERROR(clAmsCCBOpListTerminate(&ccbInstance->ccbOpListHandle)); 

    AMS_CHECK_RC_ERROR( clHandleDestroy( gAms.ccbHandleDB, handle) );

    /*
     * Now checkin the handle to rip it off otherwise there would be a leak.
     */
    
    AMS_CHECK_RC_ERROR( clHandleCheckin(gAms.ccbHandleDB, handle) );


exitfn:

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );
    return rc;
}


/*
 * _clAmsMgmtCCBCommit
 * -----------------
 *
 * Commits the operations in the CCB into AMS database 
 *
 */


ClRcT
VDECL(_clAmsMgmtCCBCommit)( 
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )
{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBCommitRequestT  req = {0};
    clAmsMgmtCCBT  *ccbInstance = NULL;
    ClAmsMgmtCCBHandleT  handle;
    ClRcT  rc = CL_OK;

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBCommitRequestT, 4, 0, 0)( in, (ClPtrT)&req) );

    handle = req.handle;

    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR(clHandleCheckout(gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    /*
     * Go through the list of operations for the CCB and commit them in
     * the AMS database
     */

    AMS_CHECK_RC_ERROR(clAmsMgmtCommitCCBOperations(&ccbInstance->ccbOpListHandle)); 

    /*
     * Write the configuration changes to the DB persistently.
     */
    clAmsCkptDBWrite();

    /*
     * Checkpoint the committed changes.
     */
    
    rc = clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Ams ckpt write returned [%#x]\n", rc));
        goto exitfn;
    }

exitfn:

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );
    return rc;
}


/*
 *
 * _clAmsMgmtCCBEntityCreate
 * --------------------------------
 * Creates a new instance of an AMS entity in the AMS database
 *
 */

ClRcT 
VDECL(_clAmsMgmtCCBEntityCreate)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBEntityCreateRequestT  *req = NULL;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;
    ClRcT  rc = CL_OK;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBEntityCreateRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBEntityCreateRequestT, 4, 0, 0)( 
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                CL_AMS_MGMT_CCB_OPERATION_CREATE ));

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,req->handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = CL_AMS_MGMT_CCB_OPERATION_CREATE;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}


/*
 *
 * _clAmsMgmtCCBEntityDelete
 * --------------------------------
 * Deletes an instance of an AMS entity in the AMS database
 *
 */

ClRcT 
VDECL(_clAmsMgmtCCBEntityDelete)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBEntityDeleteRequestT  *req = NULL;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;
    ClRcT  rc = CL_OK;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBEntityDeleteRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBEntityDeleteRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );
    
    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                CL_AMS_MGMT_CCB_OPERATION_DELETE ));

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,req->handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = CL_AMS_MGMT_CCB_OPERATION_DELETE;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

exitfn:

    return rc;
}

/*
 *
 * _clAmsMgmtCCBEntitySetConfig
 * --------------------------------
 *
 * Sets one or more scalar attributes of an AMS entity
 *
 */

static ClRcT 
__clAmsMgmtCCBEntitySetConfig(
                              CL_IN  ClEoDataT  data,
                              CL_IN  ClBufferHandleT  in,
                              CL_OUT  ClBufferHandleT  out,
                              ClUint32T versionCode)

{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBEntitySetConfigRequestT  *req = NULL;
    ClAmsEntityConfigT  *entityConfig = NULL;
    ClAmsEntityTypeT  entityType = {0};
    clAmsMgmtCCBOperationDataT  *opData = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    ClRcT  rc = CL_OK;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBEntitySetConfigRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    req->entityConfig = NULL;
    req->entityConfig = clHeapAllocate(sizeof(ClAmsEntityConfigT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req->entityConfig);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBEntitySetConfigRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );


    entityType = req->entityConfig->type;

    if(entityType > CL_AMS_ENTITY_TYPE_MAX)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("CCBEntitySetConfig - invalid entity type "\
                                 " [ %d ]\n", entityType));
        goto exitfn;
    }

    entityConfig = clHeapAllocate( gClAmsEntityParams[entityType].configSize );

    AMS_CHECK_NO_MEMORY_AND_EXIT (entityConfig);

    clAmsFreeMemory (req->entityConfig);

    switch (entityType)
    {
        case CL_AMS_ENTITY_TYPE_NODE:
            { 
                AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallClAmsNodeConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            { 
                switch(versionCode)
                {
                case CL_VERSION_CODE(4, 0, 0):
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 0, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    ((ClAmsSGConfigT*)entityConfig)->maxFailovers = 0;
                    ((ClAmsSGConfigT*)entityConfig)->failoverDuration = 
                        gClAmsSGDefaultConfig.failoverDuration;
                    ((ClAmsSGConfigT*)entityConfig)->beta = 0;
                    break;

                case CL_VERSION_CODE(4, 1, 0):
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 1, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    ((ClAmsSGConfigT*)entityConfig)->beta = 0;
                    break;

                default:
                    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 5, 0, 0)(
                                                                                         in, 
                                                                                         (ClPtrT)entityConfig) );
                    break;
                }
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSUConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSIConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCompConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            { 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSIConfigT, 4, 0, 0)(
                            in, (ClPtrT)entityConfig) );
                break;
            }

        default:
            { 
                rc = CL_AMS_ERR_INVALID_ENTITY;
                goto exitfn;
            }
    
    }
 
    req->entityConfig = entityConfig;
    handle = req->handle;
    ccbInstance = NULL;
    if(entityConfig->name.length == strlen(entityConfig->name.value))
        ++entityConfig->name.length;

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG ));

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    if ( req && req->entityConfig )
    {
        clAmsFreeMemory (req->entityConfig);
    }

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);

    return rc;
}

ClRcT 
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtCCBEntitySetConfig(data, in, out, CL_VERSION_CODE(4, 0, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtCCBEntitySetConfig(data, in, out, CL_VERSION_CODE(4, 1, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtCCBEntitySetConfig, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtCCBEntitySetConfig(data, in, out, CL_VERSION_CODE(5, 0, 0));
}

/*
 *
 * _clAmsMgmtCCBCSISetNVP
 * --------------------------------
 *
 * Sets or creates name value pair for a CSI
 *
 *
 */

ClRcT
VDECL(_clAmsMgmtCCBCSISetNVP)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBCSISetNVPRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBCSISetNVPRequestT));
    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBCSISetNVPRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP ));

    handle = req->handle;
    ccbInstance = NULL;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT ( !ccbInstance);

    opData= clHeapAllocate ( sizeof(clAmsMgmtCCBOperationDataT) );  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}


/*
 *
 * _clAmsMgmtCCBCSIDeleteNVP
 * --------------------------------
 *
 * Sets or creates name value pair for a CSI
 *
 *
 */

ClRcT
VDECL(_clAmsMgmtCCBCSIDeleteNVP)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBCSISetNVPRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBCSISetNVPRequestT));
    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtCCBCSISetNVPRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP ));

    handle = req->handle;
    ccbInstance = NULL;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT ( !ccbInstance);

    opData= clHeapAllocate ( sizeof(clAmsMgmtCCBOperationDataT) );  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}



/*
 *
 * _clAmsMgmtCCBSetNodeDependency
 * --------------------------------
 *
 *  Adds/Deletes a node in the node dependencies list for a AMS node
 *
 *
 */

static ClRcT 
_clAmsMgmtCCBSetNodeDependencyOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetNodeDependencyRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetNodeDependencyRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetNodeDependencyRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;
    ccbInstance = NULL;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate ( sizeof(clAmsMgmtCCBOperationDataT) );  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetNodeDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetNodeDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY);
}

ClRcT
VDECL(_clAmsMgmtCCBDeleteNodeDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetNodeDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY);
}


/*
 *
 * _clAmsMgmtCCBSetNodeSUList
 * --------------------------------
 *
 *  Adds/Deletes a su in the node list for an AMS node
 *
 *
 */

static ClRcT 
_clAmsMgmtCCBSetNodeSUListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtCCBSetNodeSUListRequestT  *req = NULL;
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBHandleT  handle = 0;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetNodeSUListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetNodeSUListRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;
    ccbInstance = NULL;

    AMS_CHECK_RC_ERROR(clHandleCheckout(gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;

    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );
   
    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetNodeSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetNodeSUListOp(data, in, out, 
                                        CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST);
}


ClRcT
VDECL(_clAmsMgmtCCBDeleteNodeSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetNodeSUListOp(data, in, out, 
                                        CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSGSUList
 * --------------------------------
 *
 *  Adds/Deletes a sg in the sg su list for a AMS sg 
 *
 *
 */

static ClRcT
_clAmsMgmtCCBSetSGSUListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSUListRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSGSUListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSGSUListRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));
    handle = req->handle;
    ccbInstance = NULL;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT ( !ccbInstance);

    opData= clHeapAllocate ( sizeof(clAmsMgmtCCBOperationDataT) );  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}


ClRcT
VDECL(_clAmsMgmtCCBSetSGSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSGSUListOp(data, in, out,
                                      CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST);
}


ClRcT
VDECL(_clAmsMgmtCCBDeleteSGSUList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSGSUListOp(data, in, out,
                                      CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSGSIList
 * --------------------------------
 *
 *  Adds/deletes a sg in the sg si list for a AMS sg 
 *
 *
 */

static ClRcT
_clAmsMgmtCCBSetSGSIListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSIListRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSGSIListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSGSIListRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle, 
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetSGSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSGSIListOp(data, in, out,
                                      CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST);
}


ClRcT
VDECL(_clAmsMgmtCCBDeleteSGSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSGSIListOp(data, in, out,
                                      CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSUCompList
 * --------------------------------
 *
 *  Adds/deletes a component in the su comp list for a AMS su 
 *
 *
 */

static ClRcT
_clAmsMgmtCCBSetSUCompListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSUCompListRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSUCompListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSUCompListRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle, 
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetSUCompList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSUCompListOp(data, in, out, 
                                        CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST);
}


ClRcT
VDECL(_clAmsMgmtCCBDeleteSUCompList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSUCompListOp(data, in, out, 
                                        CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSISURankList
 * --------------------------------
 *
 *  Adds/deletes a su in the si-su rank list for a AMS si 
 *
 *
 */

static ClRcT
_clAmsMgmtCCBSetSISURankListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISURankListRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSISURankListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSISURankListRequestT, 4, 0, 0)( in, 
                (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req, 
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout( gAms.ccbHandleDB,handle,
                (ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetSISURankList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSISURankListOp(data, in, out,
                                          CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST);
}

ClRcT
VDECL(_clAmsMgmtCCBDeleteSISURankList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSISURankListOp(data, in, out,
                                          CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSISIDependency
 * --------------------------------
 *
 *  Adds/deletes a si in the si-si dependencies list for a AMS si 
 *
 *
 */

static 
ClRcT 
_clAmsMgmtCCBSetSISIDependencyOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISIDependencyRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSISIDependencyRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSISIDependencyRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT 
VDECL(_clAmsMgmtCCBSetSISIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSISIDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST);
}


ClRcT 
VDECL(_clAmsMgmtCCBDeleteSISIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSISIDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST);
}

/*
 *
 * _clAmsMgmtCCBSetSICSIList
 * --------------------------------
 *
 *  Adds/deletes a csi in the si-csi list for a AMS si 
 *
 *
 */

static ClRcT
_clAmsMgmtCCBSetSICSIListOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSICSIListRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetSICSIListRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetSICSIListRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(
                clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}

ClRcT
VDECL(_clAmsMgmtCCBSetSICSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSICSIListOp(data, in, out,
                                       CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST);
}

ClRcT
VDECL(_clAmsMgmtCCBDeleteSICSIList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetSICSIListOp(data, in, out,
                                       CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST);
}


/*
 *
 * _clAmsMgmtCCBSetCSICSIDependency
 * --------------------------------
 *
 *  Adds/deletes a csi in the csi-csi dependencies list for a AMS csi 
 *
 *
 */

static 
ClRcT 
_clAmsMgmtCCBSetCSICSIDependencyOp(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out,
        ClAmsMgmtCCBOperationsT opId)

{
    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetCSICSIDependencyRequestT  *req = NULL;
    ClAmsMgmtCCBHandleT  handle;
    clAmsMgmtCCBT  *ccbInstance = NULL;
    clAmsMgmtCCBOperationDataT  *opData = NULL;

    req = clHeapAllocate (sizeof(clAmsMgmtCCBSetCSICSIDependencyRequestT));

    AMS_CHECK_NO_MEMORY_AND_EXIT (req);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallclAmsMgmtCCBSetCSICSIDependencyRequestT, 4, 0, 0)(
                in, (ClPtrT)req) );

    AMS_CHECK_RC_ERROR( clAmsCCBValidateOperation( (ClPtrT)req,
                                                   opId));

    handle = req->handle;

    AMS_CHECK_RC_ERROR(clHandleCheckout(
                gAms.ccbHandleDB,handle,(ClPtrT)&ccbInstance)); 

    AMS_CHECKPTR_AND_EXIT (!ccbInstance);

    opData= clHeapAllocate (sizeof(clAmsMgmtCCBOperationDataT));  

    AMS_CHECK_NO_MEMORY_AND_EXIT (opData);

    opData->opId = opId;
    opData->payload = (ClPtrT)req;
    
    AMS_CALL( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsCCBAddOperation(
                ccbInstance->ccbOpListHandle, opData, 0), gAms.mutex );

    AMS_CALL( clOsalMutexUnlock(gAms.mutex) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (req);
    clAmsFreeMemory (opData);
    return rc;
}


ClRcT 
VDECL(_clAmsMgmtCCBSetCSICSIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetCSICSIDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST);
}


ClRcT 
VDECL(_clAmsMgmtCCBDeleteCSICSIDependency)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out)
{
    return _clAmsMgmtCCBSetCSICSIDependencyOp(data, in, out,
                                            CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST);
}

/*
 *
 * _clAmsMgmtEntityGet
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

static ClRcT 
__clAmsMgmtEntityGet(
                     CL_IN  ClEoDataT  data,
                     CL_IN  ClBufferHandleT  in,
                     CL_OUT  ClBufferHandleT  out,
                     ClUint32T versionCode)

{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtEntityGetRequestT  req;
    ClAmsEntityTypeT  entityType;
    ClAmsEntityRefT  entityRef;
    ClRcT  rc = CL_OK;


    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityGetRequestT, 4, 0, 0)( in, (ClPtrT)&req) );

    entityType = req.entity.type;

    AMS_CHECK_ENTITY_TYPE (entityType);

    memcpy ( &entityRef.entity, &req.entity, sizeof (ClAmsEntityT) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityType],
                &entityRef),
            gAms.mutex);

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

    AMS_CHECKPTR (!entityRef.ptr); 
    
    AMS_CALL( VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)( &entityRef.entity, out, 0));

    switch (entityType)
    {

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                AMS_CALL( VDECL_VER(clXdrMarshallClAmsNodeT, 4, 0, 0)( entityRef.ptr, out, 0));
                break;

            }

        case CL_AMS_ENTITY_TYPE_SG:
            { 
                switch(versionCode)
                {
                case CL_VERSION_CODE(4, 0, 0):
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsSGT, 4, 0, 0)( entityRef.ptr, out, 0));
                    break;
                case CL_VERSION_CODE(4, 1, 0):
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsSGT, 4, 1, 0)( entityRef.ptr, out, 0));
                    break;
                default:
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsSGT, 5, 0, 0)( entityRef.ptr, out, 0));
                    break;
                }

                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            { 

                AMS_CALL( VDECL_VER(clXdrMarshallClAmsSUT, 4, 0, 0)( entityRef.ptr, out, 0));
                break;

            }

        case CL_AMS_ENTITY_TYPE_SI:
            { 

                AMS_CALL( VDECL_VER(clXdrMarshallClAmsSIT, 4, 0, 0)( entityRef.ptr, out, 0));
                break;

            }

        case CL_AMS_ENTITY_TYPE_COMP:
            { 
                switch(versionCode)
                {
                case CL_VERSION_CODE(5, 1, 0):
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsCompT, 5, 1, 0)( entityRef.ptr, out, 0));
                    break;
                default:
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsCompT, 4, 0, 0)( entityRef.ptr, out, 0));
                    break;
                }

                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            { 

                AMS_CALL( VDECL_VER(clXdrMarshallClAmsCSIT, 4, 0, 0)( entityRef.ptr, out, 0));
                break;

            }

        default:
            {
                break; 
            }
    
    }

exitfn:

    return rc;

}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGet(data, in, out, CL_VERSION_CODE(4, 0, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGet(data, in, out, CL_VERSION_CODE(4, 1, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGet(data, in, out, CL_VERSION_CODE(5, 0, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGet, 5, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGet(data, in, out, CL_VERSION_CODE(5, 1, 0));
}


static ClRcT
aspGetEntityConfig(ClAmsCompConfigT *compConfig)
{
    register ClInt32T i;
    struct aspCompMap
    {
        const ClCharT *name;
        ClBoolT isRestartable;
        ClUint32T instantiateLevel;
        ClAmsLocalRecoveryT recoveryOnTimeout;
        ClUint32T numMaxStandbyCSIs;
        const ClCharT *parentSU;
        const ClCharT *instantiateCommand;
    } aspCompMap[] = {
        { "amfServer", CL_FALSE, 1, CL_AMS_RECOVERY_NODE_FAILOVER, 1, NULL, clAmsGetInstantiateCommand()},
        { "cpmServer", CL_FALSE, 1, CL_AMS_RECOVERY_NODE_FAILOVER, 1, NULL, clAmsGetInstantiateCommand()},
        { "logServer", CL_TRUE, 2, CL_AMS_RECOVERY_COMP_RESTART, 1, "amfServerSU", "asp_logd" },
        { "gmsServer", CL_FALSE, 3, CL_AMS_RECOVERY_NODE_FAILOVER, 1, "amfServerSU", "asp_gms" },
        { "eventServer", CL_TRUE, 4, CL_AMS_RECOVERY_COMP_RESTART, 0, "amfServerSU", "asp_event" },
        { "msgServer", CL_FALSE, 4, CL_AMS_RECOVERY_NODE_FAILOVER, 0, "amfServerSU", "asp_msg" },
        { "txnServer", CL_TRUE, 4, CL_AMS_RECOVERY_COMP_RESTART, 0, "amfServerSU", "asp_txn" },
        { "nameServer", CL_TRUE, 5, CL_AMS_RECOVERY_COMP_RESTART, 0, "amfServerSU", "asp_name" },
        { "corServer", CL_FALSE, 5, CL_AMS_RECOVERY_NODE_FAILOVER, 1, "amfServerSU", "asp_cor" },
        { "ckptServer", CL_TRUE, 5, CL_AMS_RECOVERY_COMP_RESTART, 1, "amfServerSU", "asp_ckpt" },
        { "faultServer", CL_TRUE, 6, CL_AMS_RECOVERY_COMP_RESTART, 0, "amfServerSU", "asp_fault" },
        { "alarmServer", CL_TRUE, 6, CL_AMS_RECOVERY_COMP_RESTART, 0, "amfServerSU", "asp_alarm" },
        { "cmServer", CL_TRUE, 7, CL_AMS_RECOVERY_COMP_RESTART, 1, "amfServerSU", "asp_cm" },
        { NULL, },
    };

    for(i = 0; aspCompMap[i].name != NULL 
            &&
            strncmp(aspCompMap[i].name, compConfig->entity.name.value, 
                    CL_MIN(compConfig->entity.name.length, strlen(aspCompMap[i].name)));
        ++i);

    if(!aspCompMap[i].name) return CL_AMS_RC(CL_ERR_NOT_EXIST);
    compConfig->numSupportedCSITypes = 0;
    compConfig->pSupportedCSITypes = NULL;
    memset(&compConfig->proxyCSIType, 0, sizeof(compConfig->proxyCSIType));
    compConfig->capabilityModel = CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY;
    compConfig->property = CL_AMS_COMP_PROPERTY_SA_AWARE;
    compConfig->isRestartable = aspCompMap[i].isRestartable;
    compConfig->instantiateLevel = aspCompMap[i].instantiateLevel;
    compConfig->numMaxInstantiate = CL_CPM_ASP_COMP_RESTART_ATTEMPT;
    compConfig->numMaxInstantiateWithDelay = CL_CPM_ASP_COMP_RESTART_WINDOW * 1000; 
    compConfig->numMaxActiveCSIs = 1;
    compConfig->numMaxStandbyCSIs = aspCompMap[i].numMaxStandbyCSIs;
    compConfig->recoveryOnTimeout = aspCompMap[i].recoveryOnTimeout;
    if(aspCompMap[i].parentSU)
        clNameSet(&compConfig->parentSU.entity.name, aspCompMap[i].parentSU);
    if(aspCompMap[i].instantiateCommand)
        strncpy(compConfig->instantiateCommand, aspCompMap[i].instantiateCommand, 
                sizeof(compConfig->instantiateCommand)-1);
    return CL_OK;
}


/*
 *
 * _clAmsMgmtEntityGetConfig
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

static ClRcT 
__clAmsMgmtEntityGetConfig(
                           CL_IN  ClEoDataT  data,
                           CL_IN  ClBufferHandleT  in,
                           CL_OUT  ClBufferHandleT  out,
                           ClUint32T versionCode)

{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtEntityGetConfigRequestT  req ;
    ClAmsEntityTypeT  entityType;
    ClAmsEntityRefT  entityRef;
    ClAmsCompConfigT aspEntityConfig = {{0}};
    ClRcT  rc = CL_OK;

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityGetConfigRequestT, 4, 0, 0)( in, 
                (ClPtrT)&req) );

    entityType = req.entity.type;
    
    AMS_CHECK_ENTITY_TYPE (entityType);

    memcpy ( &entityRef.entity, &req.entity, sizeof (ClAmsEntityT) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityType],
                                 &entityRef);
    if(rc != CL_OK)
    {
        if(entityType != CL_AMS_ENTITY_TYPE_COMP)
            goto out_unlock;

        /*
         * Check if its an ASP component/entity
         */
        entityRef.ptr = (ClAmsEntityT*)&aspEntityConfig;
        memcpy(&aspEntityConfig.entity, &req.entity, sizeof(ClAmsEntityConfigT));
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX(aspGetEntityConfig(&aspEntityConfig),
                                            gAms.mutex);
    }

    if(!entityRef.ptr)
    {
        rc = CL_AMS_RC(CL_ERR_NULL_POINTER);
        goto out_unlock;
    }

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                        VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&entityRef.entity, out, 0),
                                        gAms.mutex );
    switch (entityType)
    {

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeT  *node = (ClAmsNodeT *)entityRef.ptr;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                    VDECL_VER(clXdrMarshallClAmsNodeConfigT, 4, 0, 0)(&node->config, out,
                                                                                                      0),
                                                    gAms.mutex);
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            { 
                ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
                switch(versionCode)
                {
                case CL_VERSION_CODE(4, 0, 0):
                    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                        VDECL_VER(clXdrMarshallClAmsSGConfigT, 4, 0, 0)
                                                        ( &sg->config, out, 0),
                                                        gAms.mutex);
                    break;
                case CL_VERSION_CODE(4, 1, 0):
                    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                        VDECL_VER(clXdrMarshallClAmsSGConfigT, 4, 1, 0)
                                                        ( &sg->config, out, 0),
                                                        gAms.mutex);
                    break;
                default:
                    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                        VDECL_VER(clXdrMarshallClAmsSGConfigT, 5, 0, 0)
                                                        ( &sg->config, out, 0),
                                                        gAms.mutex);
                    break;
                }
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            { 
                ClAmsSUT  *su = (ClAmsSUT *)entityRef.ptr;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                    VDECL_VER(clXdrMarshallClAmsSUConfigT, 4, 0, 0)( &su->config, out, 0),
                                                     gAms.mutex);
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            { 
                ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                    VDECL_VER(clXdrMarshallClAmsSIConfigT, 4, 0, 0)( &si->config, out, 0),
                                                     gAms.mutex);
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            { 
                ClAmsCompT  *comp = (ClAmsCompT *)entityRef.ptr;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                    VDECL_VER(clXdrMarshallClAmsCompConfigT, 4, 0, 0)( &comp->config, out, 
                                                                                                       0),
                                                     gAms.mutex);
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            { 
                ClAmsCSIT  *csi = (ClAmsCSIT *)entityRef.ptr;
                AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
                                                    VDECL_VER(clXdrMarshallClAmsCSIConfigT, 4, 0, 0)( &csi->config, out, 0),
                                                    gAms.mutex);
                break;
            }

        default:
            {
                break; 
            } 
    }

    out_unlock:
    clOsalMutexUnlock(gAms.mutex);

    exitfn:
    return rc;
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetConfig(data, in, out, CL_VERSION_CODE(4, 0, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetConfig(data, in, out, CL_VERSION_CODE(4, 1, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetConfig, 5, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetConfig(data, in, out, CL_VERSION_CODE(5, 0, 0));
}

/*
 *
 * _clAmsMgmtEntityGetStatus
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

static ClRcT 
__clAmsMgmtEntityGetStatus(
                           CL_IN  ClEoDataT  data,
                           CL_IN  ClBufferHandleT  in,
                           CL_OUT  ClBufferHandleT  out,
                           ClUint32T versionCode)

{
    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtEntityGetStatusRequestT  req ;
    ClAmsEntityTypeT  entityType;
    ClAmsEntityRefT  entityRef;
    ClRcT  rc = CL_OK;

    AMS_CALL( VDECL_VER(clXdrUnmarshallclAmsMgmtEntityGetStatusRequestT, 4, 0, 0)( in, 
                (ClPtrT)&req) );

    entityType = req.entity.type;

    AMS_CHECK_ENTITY_TYPE (entityType);

    memcpy ( &entityRef.entity, &req.entity, sizeof (ClAmsEntityT) );

    AMS_CALL ( clOsalMutexLock(gAms.mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( 
            clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityType],
                &entityRef),
            gAms.mutex);

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex) );

    AMS_CHECKPTR (!entityRef.ptr); 
    
    AMS_CALL( VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)( &entityRef.entity, out, 0));

    switch (entityType)
    {

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeT  *node = (ClAmsNodeT *)entityRef.ptr;
                AMS_CALL( VDECL_VER(clXdrMarshallClAmsNodeStatusT, 4, 0, 0)( &node->status, out, 
                            0));
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            { 
                ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
                switch(versionCode)
                {
                case CL_VERSION_CODE(4, 0, 0):
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsSGStatusT, 4, 0, 0)( &sg->status, out, 0));
                    break;
                default:
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsSGStatusT, 4, 1, 0)( &sg->status, out, 0));
                    break;
                }
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            { 
                ClAmsSUT  *su = (ClAmsSUT *)entityRef.ptr;
                AMS_CALL( VDECL_VER(clXdrMarshallClAmsSUStatusT, 4, 0, 0)( &su->status, out, 0));
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            { 
                ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
                AMS_CALL( VDECL_VER(clXdrMarshallClAmsSIStatusT, 4, 0, 0)( &si->status, out, 0));
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            { 
                ClAmsCompT  *comp = (ClAmsCompT *)entityRef.ptr;
                switch(versionCode)
                {
                case CL_VERSION_CODE(5, 1, 0):
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsCompStatusT, 5, 1, 0)( &comp->status, out, 
                                                                                 0));
                    break;
                default:
                    AMS_CALL( VDECL_VER(clXdrMarshallClAmsCompStatusT, 4, 0, 0)( &comp->status, out, 
                                                                                 0));
                    break;
                }
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            { 
                ClAmsCSIT  *csi = (ClAmsCSIT *)entityRef.ptr;
                AMS_CALL( VDECL_VER(clXdrMarshallClAmsCSIStatusT, 4, 0, 0)( &csi->status, out, 0));
                break;
            }

        default:
            {
                break; 
            }
    
    }

    return CL_OK;

exitfn:

    return rc;
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 4, 0, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetStatus(data, in, out, CL_VERSION_CODE(4, 0, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 4, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetStatus(data, in, out, CL_VERSION_CODE(4, 1, 0));
}

ClRcT 
VDECL_VER(_clAmsMgmtEntityGetStatus, 5, 1, 0)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{
    return __clAmsMgmtEntityGetStatus(data, in, out, CL_VERSION_CODE(5, 1, 0));
}

/*
 *
 * _clAmsMgmtGetCSINVPList
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

ClRcT 
VDECL(_clAmsMgmtGetCSINVPList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtGetCSINVPListRequestT  req ;
    clAmsMgmtGetCSINVPListResponseT res = {0};
    ClRcT  rc = CL_OK;
    ClUint32T  i = 0;


    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtGetCSINVPListRequestT, 4, 0, 0)( in, 
                (ClPtrT)&req) );

    clOsalMutexLock(gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsCSIGetNVP(&req.csi,&res), 
                                         gAms.mutex);

    clOsalMutexUnlock(gAms.mutex);
    
    AMS_CHECK_RC_ERROR( clXdrMarshallClUint32T(&res.count,out,0) );

    for ( i=0; i< res.count; i++ )
    {
        AMS_CHECK_RC_ERROR (VDECL_VER(clXdrMarshallClAmsCSINVPT, 4, 0, 0)(&res.nvp[i],out,0));
    }

exitfn:

    clAmsFreeMemory (res.nvp);

    return rc;
}

/*
 *
 * _clAmsMgmtGetEntityList
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

ClRcT 
VDECL(_clAmsMgmtGetEntityList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtGetEntityListRequestT  req ;
    clAmsMgmtGetEntityListResponseT res = {0};
    ClRcT  rc = CL_OK;
    ClUint32T  i = 0;


    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtGetEntityListRequestT, 4, 0, 0)( in, 
                (ClPtrT)&req) );

    clOsalMutexLock(gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsGetEntityList(&req.entity,req.entityListName,
                                                            &res), gAms.mutex);

    clOsalMutexUnlock(gAms.mutex);

    AMS_CHECK_RC_ERROR( clXdrMarshallClUint32T(&res.count,out,0) );

    for ( i=0; i< res.count; i++ )
    {
        AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&res.entity[i], out,
                    0));
    }

exitfn:

    clAmsFreeMemory (res.entity);
    return rc;
}



/*
 *
 * _clAmsMgmtGetOLEntityList
 * --------------------------------
 *
 * return the scalar attributes of an AMS entity
 *
 *
 */

ClRcT 
VDECL(_clAmsMgmtGetOLEntityList)(
        CL_IN  ClEoDataT  data,
        CL_IN  ClBufferHandleT  in,
        CL_OUT  ClBufferHandleT  out )

{

    AMS_FUNC_ENTER (("\n"));

    clAmsMgmtGetOLEntityListRequestT  req ;
    clAmsMgmtGetOLEntityListResponseT  res = {0};
    ClUint32T  size = 0;
    ClUint32T  i = 0;
    ClRcT  rc = CL_OK;

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrUnmarshallclAmsMgmtGetOLEntityListRequestT, 4, 0, 0)(in, 
                (ClPtrT)&req) );

    clOsalMutexLock(gAms.mutex);

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsGetOLEntityList(&req.entity, req.entityListName, 
                                                              &res),
                                         gAms.mutex);

    clOsalMutexUnlock(gAms.mutex);

    AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&req.entityListName, 
                out,0));

    AMS_CHECK_RC_ERROR( clXdrMarshallClUint32T(&res.count,out,0) ); 
    

    if ( req.entityListName == CL_AMS_SU_STATUS_SI_LIST )
    {
        size = sizeof (ClAmsSUSIRefT);
    }

    else if ( req.entityListName == CL_AMS_SU_STATUS_SI_EXTENDED_LIST )
    {
        size = sizeof (ClAmsSUSIExtendedRefT);
    }

    else if (req.entityListName == CL_AMS_SI_STATUS_SU_LIST)
    {
        size = sizeof (ClAmsSISURefT);
    }

    else if (req.entityListName == CL_AMS_SI_STATUS_SU_EXTENDED_LIST)
    {
        size = sizeof (ClAmsSISUExtendedRefT);
    }

    else if (req.entityListName == CL_AMS_COMP_STATUS_CSI_LIST)
    {
        size = sizeof (ClAmsCompCSIRefT);
    }

    else
    {
        rc = CL_AMS_ERR_INVALID_ENTITY_LIST;
        goto exitfn;
    }


    for ( i=0; i< res.count; i++ )
    {

        if ( req.entityListName == CL_AMS_SU_STATUS_SI_LIST )
        {
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsSUSIRefT, 4, 0, 0)( 
                        (ClInt8T *)res.entityRef + i*size,out,0));
        }
        else if ( req.entityListName == CL_AMS_SU_STATUS_SI_EXTENDED_LIST )
        {
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsSUSIExtendedRefT, 4, 0, 0)( 
                        (ClInt8T *)res.entityRef + i*size,out,0));
        }
        else if ( req.entityListName == CL_AMS_SI_STATUS_SU_LIST )
        {
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsSISURefT, 4, 0, 0)( 
                        (ClInt8T *)res.entityRef + i*size,out,0));
        }
        else if ( req.entityListName == CL_AMS_SI_STATUS_SU_EXTENDED_LIST )
        {
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsSISUExtendedRefT, 4, 0, 0)( 
                        (ClInt8T *)res.entityRef + i*size,out,0));
        }
        else if ( req.entityListName == CL_AMS_COMP_STATUS_CSI_LIST )
        {
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsCompCSIRefT, 4, 0, 0)( 
                        (ClInt8T *)res.entityRef + i*size,out,0));
        }
    }

exitfn:

    clAmsFreeMemory (res.entityRef);
    return rc;
}

ClRcT 
VDECL(_clAmsMgmtMigrateSG)(ClEoDataT data,
                          ClBufferHandleT inMsg,
                          ClBufferHandleT outMsg)
{
    ClAmsMgmtMigrateRequestT request ;
    ClAmsMgmtMigrateResponseT response;
    ClAmsSGRedundancyModelT model  = CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY;
    ClAmsEntityRefT entityRef = {{0}};
    ClRcT rc = CL_OK;

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    rc = VDECL_VER(clXdrUnmarshallClAmsMgmtMigrateRequestT, 4, 0, 0)(inMsg, (ClPtrT)&request);
    if(rc != CL_OK)
        return rc;

    memcpy(&entityRef.entity.name, &request.sg, sizeof(entityRef.entity.name));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_SG;

    clOsalMutexLock(gAms.mutex);

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                                 &entityRef);

    clOsalMutexUnlock(gAms.mutex);

    if(rc != CL_OK)
    {
        clLogError("SG", "MIGRATE", "Migration SG [%.*s] not found in the AMS db",
                   request.sg.length-1, request.sg.value);
        return rc;
    }

    if(!request.activeSUs)
    {
        clLogError("SG", "MIGRATE", "Active SU cannot be zero for SG [%.*s] migration",
                   request.sg.length-1, request.sg.value);
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    if(!request.standbySUs)
    {
        if(request.activeSUs > 1)
            model = CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N;
    }
    else
    {
        if(request.activeSUs > 1 )
            model = CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N;
        else
            model = CL_AMS_SG_REDUNDANCY_MODEL_TWO_N;
            
    }

    clLogInfo("SG", "MIGRATE", "Migrating SG [%.*s] to [%s] with Prefix [%.*s]",
              request.sg.length-1, request.sg.value, CL_AMS_STRING_SG_REDUNDANCY_MODEL(model),
              request.prefix.length-1, request.prefix.value);

    rc = clAmsMgmtMigrateSGRedundancy(model, (const ClCharT*)request.sg.value,
                                      (const ClCharT*)request.prefix.value, 
                                      request.activeSUs,
                                      request.standbySUs, &response.migrateList);

    if(rc != CL_OK)
    {
        clLogError("SG", "MIGRATE", "SG [%.*s] migrate returned [%#x]", 
                   request.sg.length-1, request.sg.value, rc);
        return rc;
    }

    clLogInfo("SG", "MIGRATE", "SG [%.*s] migrate to [%s] success",
              request.sg.length-1, request.sg.value, CL_AMS_STRING_SG_REDUNDANCY_MODEL(model));

    rc = VDECL_VER(clXdrMarshallClAmsMgmtMigrateResponseT, 4, 0, 0)((ClPtrT)&response, outMsg, 1);
    if(rc != CL_OK)
    {
        clLogError("SG", "MIGRATE", "SG migrate response marshall returned [%#x]", rc);
        return rc;
    }
    
    return CL_OK;
}

ClRcT
VDECL(_clAmsMgmtEntityUserDataSet)(ClEoDataT userData,
                                   ClBufferHandleT inMsgHdl,
                                   ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
    ClAmsEntityRefT entityRef = {{0}};
    ClUint32T len = 0;
    ClCharT *data = NULL;

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return rc;

    AMS_CALL( VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef.entity) );

    AMS_CALL (clXdrUnmarshallClUint32T(inMsgHdl, &len));

    if(len)
    {
        data = clHeapCalloc(1, len);
        CL_ASSERT(data != NULL);
        rc = clXdrUnmarshallArrayClCharT(inMsgHdl, data, len);
        if(rc != CL_OK)
        {
            clHeapFree(data);
            return rc;
        }
    }
    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        if(data) clHeapFree(data);
        clLogError("USERDATA", "SET", "Entity locate [%.*s] returned [%#x]",
                   entityRef.entity.name.length-1, entityRef.entity.name.value, rc);
        return rc;
    }
    clLogInfo("USERDATA", "SET", "Setting user data for entity [%.*s]",
              entityRef.entity.name.length-1, entityRef.entity.name.value);
    rc = _clAmsEntityUserDataSet(&entityRef.entity.name, data, len);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        if(data) clHeapFree(data);
        clLogError("USERDATA", "SET", "User data set returned [%#x]", rc);
        return rc;
    }

    /*
     * TODO -> Good idea to move it to a separate ckpt section.
     * and dataset as anyway its kept outside. Unnecessary to do it
     * for 3.1 at this phase as it isnt really required.
     */
    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);

    clAmsCkptDBWrite();

    clOsalMutexUnlock(gAms.mutex);

    return rc;
}

ClRcT
VDECL(_clAmsMgmtEntityUserDataSetKey)(ClEoDataT userData,
                                      ClBufferHandleT inMsg,
                                      ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClCharT *data = NULL;
    ClUint32T len = 0;
    ClNameT key = {0};
    ClAmsEntityRefT entityRef = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       && 
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);

    AMS_CALL (VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, &entityRef.entity));
    AMS_CALL (clXdrUnmarshallClNameT(inMsg, &key) );
    AMS_CALL (clXdrUnmarshallClUint32T(inMsg, &len));

    if(len)
    {
        data = clHeapCalloc(1, len);
        CL_ASSERT(data != NULL);
        rc = clXdrUnmarshallArrayClCharT(inMsg, data, len);
        if(rc != CL_OK)
        {
            clHeapFree(data);
            return rc;
        }
    }
    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        if(data) clHeapFree(data);
        clLogError("USERDATA", "SETKEY", "Entity [%.*s] not found in the AMS db",
                   entityRef.entity.name.length-1, entityRef.entity.name.value);
        return rc;
    }

    clLogInfo("USERDATA", "SETKEY", "Setting user data with key for entity [%.*s]",
              entityRef.entity.name.length-1, entityRef.entity.name.value);
    rc = _clAmsEntityUserDataSetKey(&entityRef.entity.name, &key, data, len);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        if(data) clHeapFree(data);
        clLogError("USERDATA", "SETKEY", "Setting user data with key returned [%#x]",
                   rc);
        return rc;
    }

    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);
    clAmsCkptDBWrite();
    clOsalMutexUnlock(gAms.mutex);

    return rc;
}

ClRcT
VDECL(_clAmsMgmtEntityUserDataGet)(ClEoDataT userData,
                                   ClBufferHandleT inMsgHdl,
                                   ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCharT *data = NULL;
    ClUint32T len = 0;
    ClAmsEntityRefT entityRef = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);

    AMS_CALL(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef.entity));

    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "GET", "Entity [%.*s] not found in the AMS",
                   entityRef.entity.name.length-1, entityRef.entity.name.value);
        return rc;
    }
    rc = _clAmsEntityUserDataGet(&entityRef.entity.name, &data, &len);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        /* The only way to tell if it has any is to poll it
           clLogInfo("USERDATA", "GET", "User data get for entity [%.*s] returned [%#x]",
                   entityRef.entity.name.length-1, entityRef.entity.name.value, rc);
        */
        return rc;
    }
    clOsalMutexUnlock(gAms.mutex);

    rc = clXdrMarshallClUint32T(&len, outMsgHdl, 0);
    if(rc != CL_OK)
    {
        if(data) clHeapFree(data);
        return rc;
    }

    if(len)
    {
        rc = clXdrMarshallArrayClCharT(data, len, outMsgHdl, 0);
        clHeapFree(data);
    }

    return rc;
}

ClRcT
VDECL(_clAmsMgmtEntityUserDataGetKey)(ClEoDataT userData,
                                      ClBufferHandleT inMsgHdl,
                                      ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClNameT key = {0};
    ClCharT *data = NULL;
    ClUint32T len = 0;
    ClAmsEntityRefT entityRef = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);

    AMS_CALL(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef.entity));
    AMS_CALL (clXdrUnmarshallClNameT(inMsgHdl, &key));

    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "GETKEY", "Entity [%.*s] not found in the AMS",
                   entityRef.entity.name.length-1, entityRef.entity.name.value);
        return rc;
    }
    rc = _clAmsEntityUserDataGetKey(&entityRef.entity.name, &key, &data, &len);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "GETKEY", "User data get for entity [%.*s] returned [%#x]",
                   entityRef.entity.name.length-1, entityRef.entity.name.value, rc);
        return rc;
    }
    clOsalMutexUnlock(gAms.mutex);
    
    rc = clXdrMarshallClUint32T(&len, outMsgHdl, 0);
    if(rc != CL_OK)
    {
        if(data) clHeapFree(data);
        return rc;
    }
    if(len)
    {
        rc = clXdrMarshallArrayClCharT(data, len, outMsgHdl, 0);
        clHeapFree(data);
    }
    return rc;
}

ClRcT 
VDECL(_clAmsMgmtEntityUserDataDelete)(ClEoDataT userData,
                                      ClBufferHandleT inMsgHdl,
                                      ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0}};
    ClBoolT clear = CL_FALSE;

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    
    AMS_CALL (VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef));
    AMS_CALL (clXdrUnmarshallClUint16T(inMsgHdl, &clear) );

    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "DELETE", "Entity [%.*s] not found in AMS db",
                   entityRef.entity.name.length-1, entityRef.entity.name.value);
        return rc;
    }
    rc = _clAmsEntityUserDataDelete(&entityRef.entity.name, clear);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "DELETE", "User data delete for entity [%.*s] returned [%#x]",
                   entityRef.entity.name.length-1, entityRef.entity.name.value, rc);
        return rc;
    }
    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);
    clAmsCkptDBWrite();
    clOsalMutexUnlock(gAms.mutex);

    return rc;
}


ClRcT 
VDECL(_clAmsMgmtEntityUserDataDeleteKey)(ClEoDataT userData,
                                         ClBufferHandleT inMsgHdl,
                                         ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0}};
    ClNameT key = {0};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    
    AMS_CALL (VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef));
    AMS_CALL (clXdrUnmarshallClNameT(inMsgHdl, &key));

    clOsalMutexLock(gAms.mutex);
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "DELETEKEY", "Entity [%.*s] not found in AMS db",
                   entityRef.entity.name.length-1, entityRef.entity.name.value);
        return rc;
    }
    rc = _clAmsEntityUserDataDeleteKey(&entityRef.entity.name, &key);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gAms.mutex);
        clLogError("USERDATA", "DELETEKEY", "User data delete for entity [%.*s] returned [%#x]",
                   entityRef.entity.name.length-1, entityRef.entity.name.value, rc);
        return rc;
    }
    clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);
    clAmsCkptDBWrite();
    clOsalMutexUnlock(gAms.mutex);
    return rc;
}

ClRcT 
VDECL(_clAmsMgmtSIAssignSUCustom)(ClEoDataT userData,
                                  ClBufferHandleT inMsgHdl,
                                  ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClAmsSIT *si = NULL;
    ClAmsSUT *activeSU =  NULL;
    ClAmsSUT *standbySU = NULL;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsMgmtSIAssignSUCustomRequestT req = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    
    AMS_CALL (VDECL_VER(clXdrUnmarshallClAmsMgmtSIAssignSUCustomRequestT, 4, 0, 0)(inMsgHdl, &req));
    
    clOsalMutexLock(gAms.mutex);

    memcpy(&entityRef.entity, &req.si, sizeof(entityRef.entity));
    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                                                 &entityRef), gAms.mutex);
    si = (ClAmsSIT*)entityRef.ptr;

    if(!req.activeSU.name.length)
        activeSU = NULL;
    else
    {
        memcpy(&entityRef.entity, &req.activeSU, sizeof(entityRef.entity));
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsEntityDbFindEntity(
                                                                     &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                                                     &entityRef), gAms.mutex);

        activeSU = (ClAmsSUT*)entityRef.ptr;
    }

    if(!req.standbySU.name.length)
        standbySU = NULL;
    else
    {
        memcpy(&entityRef.entity, &req.standbySU, sizeof(entityRef.entity));
        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX( clAmsEntityDbFindEntity(
                                                                     &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                                                     &entityRef),
                                             gAms.mutex);
        standbySU = (ClAmsSUT*)entityRef.ptr;
    }

    rc = clAmsPeSIAssignSUCustom(si, activeSU, standbySU);

    exitfn:

    clOsalMutexUnlock(gAms.mutex);

    return rc;
}

ClRcT 
VDECL_VER(_clAmsMgmtDBGet, 5, 1, 0)(ClEoDataT userData,
                                    ClBufferHandleT inMsgHdl,
                                    ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msg = 0;
    ClUint32T len = 0;
    ClUint8T *dbBuffer = NULL;

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);

    rc = clBufferCreate(&msg);
    if(rc != CL_OK)
    {
        goto out;
    }

    clOsalMutexLock(gAms.mutex);
    rc = clAmsDBGet(msg);
    clOsalMutexUnlock(gAms.mutex);

    if(rc != CL_OK)
    {
        clLogError("DB", "GET", "AMF mgmt db get returned with [%#x]", rc);
        goto out_free;
    }
    
    rc = clBufferLengthGet(msg, &len);
    if(rc != CL_OK)
        goto out_free;

    rc = clBufferFlatten(msg, &dbBuffer);
    if(rc != CL_OK)
        goto out_free;

    rc = clXdrMarshallClUint32T(&len, outMsgHdl, 0);
    if(rc != CL_OK)
        goto out_free;

    rc = clXdrMarshallArrayClUint8T(dbBuffer, len, outMsgHdl, 0);
    
    out_free:
    clBufferDelete(&msg);

    if(dbBuffer) clHeapFree(dbBuffer);

    out:
    return rc;
}

ClRcT 
VDECL_VER(_clAmsMgmtComputedAdminStateGet, 5, 0, 0)(ClEoDataT userData,
                                                    ClBufferHandleT inMsgHdl,
                                                    ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT computedAdminState = CL_AMS_ADMIN_STATE_NONE;
    ClAmsEntityRefT entityRef = {{0}};

    if(gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING
       &&
       gAms.serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN)
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);

    AMS_CALL( VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entityRef.entity) );
        
    clOsalMutexLock(gAms.mutex);

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type], &entityRef);
    if(rc != CL_OK)
    {
        clLogError("CAS", "GET", "Entity [%.*s] not found in the amf db", entityRef.entity.name.length,
                   entityRef.entity.name.value);
        goto out_unlock;
    }

    switch(entityRef.entity.type)
    {
    case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT *sg = (ClAmsSGT*)entityRef.ptr;
            AMS_CHECKPTR(!sg);
            rc = clAmsPeSGComputeAdminState(sg, &computedAdminState);
        }
        break;

    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT *si = (ClAmsSIT*)entityRef.ptr;
            AMS_CHECKPTR(!si);
            rc = clAmsPeSIComputeAdminState(si, &computedAdminState);
        }
        break;

    case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT *csi = (ClAmsCSIT*)entityRef.ptr;
            AMS_CHECKPTR(!csi);
            rc = clAmsPeCSIComputeAdminState(csi, &computedAdminState);
        }
        break;

    case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *node = (ClAmsNodeT*)entityRef.ptr;
            AMS_CHECKPTR(!node);
            rc = clAmsPeNodeComputeAdminState(node, &computedAdminState);
        }
        break;

    case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT *su = (ClAmsSUT*)entityRef.ptr;
            AMS_CHECKPTR(!su);
            rc = clAmsPeSUComputeAdminState(su, &computedAdminState);
        }
        break;

    case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
            AMS_CHECKPTR(!comp);
            rc = clAmsPeCompComputeAdminState(comp, &computedAdminState);
        }
        break;

    default:
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto out_unlock;
    }

    out_unlock:
    clOsalMutexUnlock(gAms.mutex);
    if(rc != CL_OK)
        return rc;

    rc = VDECL_VER(clXdrMarshallClAmsAdminStateT, 4, 0, 0)(&computedAdminState, outMsgHdl, 0);
    return rc;
}

ClRcT
clAmsMgmtCommitCCBOperations(
                             CL_IN  ClCntHandleT  *opListHandle )

{

    AMS_FUNC_ENTER(("\n"));

    clAmsMgmtCCBOperationDataT  *opData = NULL;
    ClCntNodeHandleT  nodeHandle = 0;
    ClCntNodeHandleT  nextNodeHandle = 0;
    ClAmsEntityRefT  entityRef = {{0},0};
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  nodeRef = {{0}} ,suRef = {{0}};

    AMS_CHECKPTR (!opListHandle);

    /*
     * Traverse the operations list in the CCB and apply the changes in AMS DB
     */ 
    
    for ( opData = clAmsCCBGetFirstElement(opListHandle, &nextNodeHandle);
          opData != NULL; opData = 
              clAmsCCBGetNextElement(opListHandle,&nodeHandle,&nextNodeHandle))
    {

        AMS_CHECKPTR (!opData);
        nodeHandle = nextNodeHandle;

        switch (opData->opId)
        {

        case CL_AMS_MGMT_CCB_OPERATION_CREATE :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_CREATE\n")); 
                    
                clAmsMgmtCCBEntityCreateRequestT  *req = 
                    (clAmsMgmtCCBEntityCreateRequestT *)opData->payload;
                ClAmsNotificationDescriptorT descriptor = {0};

                memcpy(&entityRef.entity, &req->entity ,
                       sizeof (ClAmsEntityT));
                clLogNotice("ENTITY", "CREATE", "Create entity [%s]", req->entity.name.value);
                AMS_CHECK_RC_ERROR( clAmsEntityDbAddEntity(
                                                           &gAms.db.entityDb[req->entity.type],&entityRef));
                
                _clAmsSAEntityAdd(&entityRef);

                if(clAmsGenericNotificationEventPayloadSet(CL_AMS_NOTIFICATION_ENTITY_CREATE, 
                                                           &req->entity, 
                                                           &descriptor) == CL_OK)
                    clAmsNotificationEventPublish(&descriptor);

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE\n"));

                clAmsMgmtCCBEntityDeleteRequestT  *req = 
                    (clAmsMgmtCCBEntityDeleteRequestT *)opData->payload;
                ClAmsNotificationDescriptorT descriptor = {0};

                memcpy(&entityRef.entity, &req->entity,
                       sizeof (ClAmsEntityT));

                clLogNotice("ENTITY", "DELETE", "Delete entity [%s]", req->entity.name.value);
                
                AMS_CHECK_RC_ERROR( clAmsEntityDbFindEntity(&gAms.db.entityDb[req->entity.type],
                                                            &entityRef));

                /*
                 * Dont do error check here since the entities might not have been added
                 * to CPM as nodes are added dynamically incase they are not present in CPM
                 * when they are started. Same with components.
                 */
                
                _clAmsSAEntityRmv(&entityRef);

                /*
                 * Delete references from the parent and children if any.
                 */
                clAmsEntityDeleteRefs(&entityRef);

                AMS_CHECK_RC_ERROR( clAmsEntityDbDeleteEntity(
                                                              &gAms.db.entityDb[req->entity.type],&entityRef));

                if(entityRef.entity.type == CL_AMS_ENTITY_TYPE_CSI)
                {
                    /*
                     * Delete stale invocation CSI entries.
                     */
                    clAmsInvocationListUpdateCSIAll(CL_FALSE);
                }
                if(clAmsGenericNotificationEventPayloadSet(CL_AMS_NOTIFICATION_ENTITY_DELETE, 
                                                           &req->entity,
                                                           &descriptor) == CL_OK)
                    clAmsNotificationEventPublish(&descriptor);

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG:
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG\n"));

                clAmsMgmtCCBEntitySetConfigRequestT  *req = 
                    (clAmsMgmtCCBEntitySetConfigRequestT *)opData->payload;

                AMS_CHECK_RC_ERROR(clAmsEntitySetConfigNew(
                                                           req->entityConfig,
                                                           req->bitmask));
                _clAmsSAEntitySetConfig(
                                        req->entityConfig,
                                        req->bitmask);
                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP :
            {

                clAmsMgmtCCBCSISetNVPRequestT  *req = 
                    (clAmsMgmtCCBCSISetNVPRequestT *)opData->payload;

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP\n"));
                    
                AMS_CHECK_RC_ERROR(
                                   clAmsCSISetNVP (
                                                   gAms.db.entityDb[req->csiName.type],
                                                   req->csiName,
                                                   req->nvp));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP :
            {

                clAmsMgmtCCBCSISetNVPRequestT  *req = 
                    (clAmsMgmtCCBCSISetNVPRequestT *)opData->payload;

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP\n"));
                    
                AMS_CHECK_RC_ERROR(
                                   clAmsCSIDeleteNVP (
                                                      gAms.db.entityDb[req->csiName.type],
                                                      req->csiName,
                                                      req->nvp));

                break;

            }


        case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY:
            {

                clAmsMgmtCCBSetNodeDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetNodeDependencyRequestT *)opData->payload;

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY\n"));
                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->nodeName,
                                                        &req->dependencyNodeName,
                                                        CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST));

                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->dependencyNodeName,
                                                        &req->nodeName,
                                                        CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST));
                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY:
            {
                clAmsMgmtCCBSetNodeDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetNodeDependencyRequestT *)opData->payload;

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY\n"));
                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->nodeName,
                                                             &req->dependencyNodeName,
                                                             CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST));


                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->dependencyNodeName,
                                                             &req->nodeName,
                                                             CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST));
                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST :
            {
                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST\n"));

                clAmsMgmtCCBSetNodeSUListRequestT  *req = 
                    (clAmsMgmtCCBSetNodeSUListRequestT *)opData->payload;

                ClAmsEntityT *sourceEntity = NULL;
                ClAmsEntityT *targetEntity = NULL;
                ClAmsSUT *targetSU = NULL;
                rc =                  clAmsAddGetEntityList(
                                                        &req->nodeName,
                                                        &req->suName,
                                                        CL_AMS_NODE_CONFIG_SU_LIST,
                                                        &sourceEntity, &targetEntity);
                if (rc != CL_OK)
                {
                    AMS_LOG(CL_DEBUG_ERROR,("Unable to add SU [%.*s] to node [%.*s], return code [0x%x]",req->suName.name.length-1,req->suName.name.value,req->nodeName.name.length-1,req->nodeName.name.value,rc));
                    AMS_CHECK_RC_ERROR(rc);
                }
                

                /*
                 * Add the parentNode reference for the SU
                 */ 


                memcpy (&nodeRef.entity,&req->nodeName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                rc = clAmsEntitySetRefPtr(suRef,nodeRef);
                if (rc != CL_OK)
                {
                    AMS_LOG(CL_DEBUG_CRITICAL,("Unable to set SU's [%.*s] parent node to [%.*s] return code [0x%x]. AMF database is inconsistent.",req->suName.name.length-1,req->suName.name.value,req->nodeName.name.length-1,req->nodeName.name.value, rc));
                    AMS_CHECK_RC_ERROR(rc);
                }
                targetSU = (ClAmsSUT*)targetEntity;
                if(targetSU && 
                   targetSU->config.parentSG.ptr && 
                   targetSU->config.parentNode.ptr)
                {
                    /*
                     * Mark the SU as instantiable. Would be skipped if its already uninstantiable
                     */
                    clAmsPeSUMarkInstantiable(targetSU);
                }

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST\n"));

                clAmsMgmtCCBSetNodeSUListRequestT  *req = 
                    (clAmsMgmtCCBSetNodeSUListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->nodeName,
                                                             &req->suName,
                                                             CL_AMS_NODE_CONFIG_SU_LIST));

                /*
                 * Delete the parentNode reference for the SU
                 */ 

                ClAmsEntityRefT  nodeRef,suRef;

                memcpy (&nodeRef.entity,&req->nodeName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntityUnsetRefPtr(
                                                           suRef,nodeRef));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST\n"));

                clAmsMgmtCCBSetSGSUListRequestT  *req = 
                    (clAmsMgmtCCBSetSGSUListRequestT *)opData->payload;
                ClAmsEntityT *sourceEntity = NULL;
                ClAmsEntityT *targetEntity = NULL;
                ClAmsSUT *targetSU = NULL;
                AMS_CHECK_RC_ERROR(
                                   clAmsAddGetEntityList(
                                                        &req->sgName,
                                                        &req->suName,
                                                        CL_AMS_SG_CONFIG_SU_LIST,
                                                        &sourceEntity, &targetEntity));

                /*
                 * Add the parentSG reference for the SU
                 */ 

                ClAmsEntityRefT  sgRef,suRef;

                memcpy (&sgRef.entity,&req->sgName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntitySetRefPtr(
                                                         suRef, sgRef));
             
                /*
                 * Move the newly added SU to instantiable list. Mark instantiable
                 * already checks if SU and node is instantiable before moving it.
                 * So default case would be a no-op in case the SU is locked or 
                 * parent SG or node is locked.
                 * Do it only if the SU is associated with both the node and the sg.
                 */
                targetSU = (ClAmsSUT*)targetEntity;
                if(targetSU && 
                   targetSU->config.parentSG.ptr && 
                   targetSU->config.parentNode.ptr)
                {
                    clAmsPeSUMarkInstantiable(targetSU);
                }
                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST:
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST\n"));

                clAmsMgmtCCBSetSGSUListRequestT  *req = 
                    (clAmsMgmtCCBSetSGSUListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->sgName,
                                                             &req->suName,
                                                             CL_AMS_SG_CONFIG_SU_LIST));

                /*
                 * Delete the parentSG reference for the SU
                 */ 

                ClAmsEntityRefT  sgRef,suRef;

                memcpy (&sgRef.entity,&req->sgName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntityUnsetRefPtr(
                                                           suRef, sgRef));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST\n"));

                clAmsMgmtCCBSetSGSIListRequestT  *req = 
                    (clAmsMgmtCCBSetSGSIListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->sgName,
                                                        &req->siName,
                                                        CL_AMS_SG_CONFIG_SI_LIST));

                /*
                 * Add the parentSG reference for the SI
                 */ 

                ClAmsEntityRefT  sgRef,siRef;

                memcpy (&sgRef.entity,&req->sgName,sizeof(ClAmsEntityT));
                memcpy (&siRef.entity,&req->siName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntitySetRefPtr(
                                                         siRef, sgRef));

                break;

            }


        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST\n"));

                clAmsMgmtCCBSetSGSIListRequestT  *req = 
                    (clAmsMgmtCCBSetSGSIListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->sgName,
                                                             &req->siName,
                                                             CL_AMS_SG_CONFIG_SI_LIST));

                /*
                 * Delete the parentSG reference for the SI
                 */ 

                ClAmsEntityRefT  sgRef,siRef;

                memcpy (&sgRef.entity,&req->sgName,sizeof(ClAmsEntityT));
                memcpy (&siRef.entity,&req->siName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntityUnsetRefPtr(
                                                           siRef, sgRef));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST\n"));

                clAmsMgmtCCBSetSUCompListRequestT  *req = 
                    (clAmsMgmtCCBSetSUCompListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->suName,
                                                        &req->compName,
                                                        CL_AMS_SU_CONFIG_COMP_LIST));

                /*
                 * Add the parentNode reference for the SU
                 */ 

                ClAmsEntityRefT  compRef,suRef;

                memcpy (&compRef.entity,&req->compName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntitySetRefPtr(
                                                         compRef, suRef));

                break;


            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST\n"));

                clAmsMgmtCCBSetSUCompListRequestT  *req = 
                    (clAmsMgmtCCBSetSUCompListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->suName,
                                                             &req->compName,
                                                             CL_AMS_SU_CONFIG_COMP_LIST));

                /*
                 * Delete the parentNode reference for the SU
                 */ 

                ClAmsEntityRefT  compRef,suRef;

                memcpy (&compRef.entity,&req->compName,sizeof(ClAmsEntityT));
                memcpy (&suRef.entity,&req->suName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntityUnsetRefPtr(
                                                           compRef, suRef));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST\n"));

                clAmsMgmtCCBSetSISURankListRequestT  *req = 
                    (clAmsMgmtCCBSetSISURankListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->siName,
                                                        &req->suName,
                                                        CL_AMS_SI_CONFIG_SU_RANK_LIST));

                break;

            }


        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST\n"));

                clAmsMgmtCCBSetSISURankListRequestT  *req = 
                    (clAmsMgmtCCBSetSISURankListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->siName,
                                                             &req->suName,
                                                             CL_AMS_SI_CONFIG_SU_RANK_LIST));
                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST\n"));

                clAmsMgmtCCBSetSISIDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetSISIDependencyRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->siName,
                                                        &req->dependencySIName,
                                                        CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST));

                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->dependencySIName,
                                                        &req->siName,
                                                        CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST));

                break;

            }

          
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST\n"));

                clAmsMgmtCCBSetSISIDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetSISIDependencyRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->siName,
                                                             &req->dependencySIName,
                                                             CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST));

                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->dependencySIName,
                                                             &req->siName,
                                                             CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST\n"));

                clAmsMgmtCCBSetSICSIListRequestT  *req = 
                    (clAmsMgmtCCBSetSICSIListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->siName,
                                                        &req->csiName,
                                                        CL_AMS_SI_CONFIG_CSI_LIST));

                /*
                 * Add the parentSI reference for the CSI 
                 */ 

                ClAmsEntityRefT  siRef,csiRef;

                memcpy (&siRef.entity,&req->siName,sizeof(ClAmsEntityT));
                memcpy (&csiRef.entity,&req->csiName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntitySetRefPtr(
                                                         csiRef, siRef));

                break;


            }


        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST\n"));

                clAmsMgmtCCBSetSICSIListRequestT  *req = 
                    (clAmsMgmtCCBSetSICSIListRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->siName,
                                                             &req->csiName,
                                                             CL_AMS_SI_CONFIG_CSI_LIST));

                /*
                 * Delete the parentSI reference for the CSI 
                 */ 

                ClAmsEntityRefT  siRef,csiRef;

                memcpy (&siRef.entity,&req->siName,sizeof(ClAmsEntityT));
                memcpy (&csiRef.entity,&req->csiName,sizeof(ClAmsEntityT));
                AMS_CHECK_RC_ERROR( clAmsEntityUnsetRefPtr(
                                                           csiRef, siRef));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST\n"));

                clAmsMgmtCCBSetCSICSIDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetCSICSIDependencyRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->csiName,
                                                        &req->dependencyCSIName,
                                                        CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST));

                AMS_CHECK_RC_ERROR(
                                   clAmsAddToEntityList(
                                                        &req->dependencyCSIName,
                                                        &req->csiName,
                                                        CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST));

                break;

            }

        case CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST :
            {

                AMS_LOG(CL_DEBUG_TRACE, 
                        ("CCB: CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST\n"));

                clAmsMgmtCCBSetCSICSIDependencyRequestT  *req = 
                    (clAmsMgmtCCBSetCSICSIDependencyRequestT *)opData->payload;

                    
                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->csiName,
                                                             &req->dependencyCSIName,
                                                             CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST));

                AMS_CHECK_RC_ERROR(
                                   clAmsDeleteFromEntityList(
                                                             &req->dependencyCSIName,
                                                             &req->csiName,
                                                             CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST));

                break;

            }

        default: 
            {
                AMS_LOG(CL_DEBUG_ERROR, ("invalid ccb operation\n"));
                rc = CL_AMS_ERR_INVALID_OPERATION;
                goto exitfn;
            }

        }

    } 

    
    exitfn:
    if (rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR,("Failure [%x] when committing CCB operations.  Application may not be notified as this operation may occur asynchronously.",rc));
    }

    AMS_CALL (clCntAllNodesDelete(*opListHandle)) ;
    return rc;
}
