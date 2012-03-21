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
 * File        : clAmsMgmtClientApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the client side implementation of the AMS management 
 * API. The management API is used by a management client to control the
 * AMS entity.
 *
 *
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
#include <stdarg.h>
#include <clAmsDebug.h>
#include <clAmsErrors.h>
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsMgmtClientRmd.h>
#include <clAmsXdrHeaderFiles.h>
#include <clVersionApi.h>
#include <clHandleApi.h>
#include <clDebugApi.h>
#include <clAmsUtils.h>
#include <clAmsMgmtHooks.h>
#include <clEoApi.h>
#include <clHash.h>
static ClEoExecutionObjT *gpEOObj;

/******************************************************************************
 * LOCAL TYPES AND VARIABLES
 *****************************************************************************/

#define AMSAREA "AMS"
#define AMSCTXT "MGT"

#define AMS_ADMIN_API_CALL(amsHandle, retry, rc, fn) do {   \
    ClInt32T iter = 0;                                      \
    ClTimerTimeOutT delay = {.tsSec=2,.tsMilliSec=0};       \
    do {                                                    \
        rc = (fn);                                          \
    } while((retry) &&                                      \
            CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN &&    \
            ++iter < 10 &&                                  \
            clOsalTaskDelay(delay) == CL_OK);               \
    if(rc != CL_OK)                                         \
    {                                                       \
        if( (amsHandle) )                                   \
            clHandleCheckin(handle_database, (amsHandle));  \
        goto exitfn;                                        \
    }                                                       \
}while(0)                                                   


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

static ClVersionDatabaseT version_database = 
{
    sizeof (versions_supported) / sizeof (ClVersionT),
    versions_supported
};

#if defined (CL_AMS_MGMT_HOOKS)
static ClRcT clAmsMgmtEntityAdminResponse(ClEoDataT data,
                                          ClBufferHandleT inMsg,
                                          ClBufferHandleT outMsg
                                          );
static ClEoPayloadWithReplyCallbackT gClAmsMgmtClientFuncList[]={
    (ClEoPayloadWithReplyCallbackT)clAmsMgmtEntityAdminResponse,
};
#endif
/* 
 * Context for a AMS "instance" - i.e. one user of AMS service 
 */

struct ams_instance 
{
    ClAmsMgmtHandleT                server_handle;

    /* 
     * User's callback functions
     */

    ClAmsMgmtCallbacksT             callbacks; 

    /* 
     * Indicate that Finalize() has been called
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
#if 0
static void  ams_handle_instance_destructor(void*);
#endif

static ClRcT check_lib_init(void);

/******************************************************************************
 * EXPORTED LIB INIT/FINALIZE FUNCTIONS
 *****************************************************************************/
/* 
 * Library initialize function.  Not reentrant! Creates AMS handle database 
 */

static ClRcT clAmsLibInitialize(void)
{

    ClRcT rc = CL_OK;

    if (lib_initialized == CL_FALSE)
    {
        AMS_CHECK_RC_ERROR( clHandleDatabaseCreate( NULL, &handle_database) );
        lib_initialized = CL_TRUE;
    }

exitfn:

    return rc;
}


/* 
 * Library cleanup function.  Not reentrant! Removes AMS handle database
 */

ClRcT 
clAmsLibFinalize(void)
{
    ClRcT rc = CL_OK;

    if (lib_initialized == CL_TRUE)
    {
        AMS_CHECK_RC_ERROR( clHandleDatabaseDestroy(handle_database) );
        lib_initialized = CL_FALSE;
    }

exitfn:

    return rc;
}

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

#if defined (CL_AMS_MGMT_HOOKS)

static ClRcT clAmsMgmtEntityAdminResponse(ClEoDataT data,
                                          ClBufferHandleT inMsg,
                                          ClBufferHandleT outMsg
                                          )
{
    ClAmsMgmtEntityAdminResponseT response;
    ClRcT rc = CL_OK;
    struct ams_instance *ams_instance = NULL;
    memset(&response,0,sizeof(response));
    
    rc = VDECL_VER(clXdrUnmarshallClAmsMgmtEntityAdminResponseT, 4, 0, 0)(inMsg,(void*)&response);
    if(rc != CL_OK)
    {
        clLogError(AMSAREA,AMSCTXT,"Admin response unmarshall returned [0x%x]",rc);
        goto out;
    }
    rc = clHandleCheckout(handle_database,response.clientHandle,(ClPtrT*)&ams_instance);
    if(rc != CL_OK)
    {
        clLogError(AMSAREA,AMSCTXT,"Handle checkout returned [0x%x]",rc);
        goto out;
    }
    if(ams_instance->server_handle != response.mgmtHandle)
    {
        clLogError(AMSAREA,AMSCTXT,"Server handle is incorrect");
        clHandleCheckin(handle_database,response.clientHandle);
        goto out;
    }
    if(ams_instance->callbacks.pEntityAdminResponse != NULL)
    {
        ams_instance->callbacks.pEntityAdminResponse
            (response.type,response.oper,(ClRcT)response.retCode);
    }
    clHandleCheckin(handle_database,response.clientHandle);
    rc = CL_OK;
    out:
    return rc;
}

#endif

static ClRcT check_lib_init(void)
{
    ClRcT rc = CL_OK;

    if (lib_initialized == CL_FALSE)
    {
        rc = clEoMyEoObjectGet(&gpEOObj);
        if(rc != CL_OK)
        {
            return rc;
        }
        CL_ASSERT(gpEOObj != NULL);
#if defined (CL_AMS_MGMT_HOOKS)
        rc = clEoClientInstall(gpEOObj,CL_AMS_MGMT_CLIENT_TABLE_ID,
                               gClAmsMgmtClientFuncList,0,
                               (ClUint32T)(sizeof(gClAmsMgmtClientFuncList)/
                                           sizeof(gClAmsMgmtClientFuncList[0])
                                           ));
        if(rc != CL_OK)
        {
            return rc;
        }
#endif        
        rc = clAmsLibInitialize();
    }

    return rc;
}
#if 0
static void ams_handle_instance_destructor(ClPtrTptr)
{
    clAmsFreeMemory (ptr);
}
#endif

/******************************************************************************
 * EXPORTED API FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * Management API functions summary
 *
 * Functions to manipulate AMS entity
 * ----------------------------------
 * Initialize:          Initialize management api, get a handle 
 * Finalize:            Finish use of management api
 * Enable:              Enable an AMS instance
 * Disable:             Disable an AMS instance
 *
 * Functions to manipulate the AMS database
 * ----------------------------------------
 * Instantiate:         Create a new instance of an entity
 * Terminate:           Delete an instance of an entity
 * FindByName:          Find an entity matching name and type
 * FindByAttribute:     Find an entity matching attributes
 * GetStatus:           Get the status of an entity
 * SetConfig:           Set any of the parameters/attributes of an entity
 *
 * Functions to manipulate all AMF entities
 * ----------------------------------------
 * Unlock:              Set admin state to unlock, ie can provide service
 * Lock:                Set admin state to lock, ie cannot provide service
 * LockAssignment:      Prevent entity from accepting service assignment
 * LockInstantiation:   Prevent entity from being instantiated, same as lock
 * Shutdown:            Set admin state to shutting down
 * Repaired:            Indicate that an entity is repaired and ready
 * Restart:             Restarts the entity
 * StartMonitoring:     Starts external active monitoring of entity
 * StopMonitoring:      Stops external active monitoring of entity
 *
 * SISwap:              Swaps the HA states of CSIs within a SI
 * SGAdjustPreference:  Return entity to default preferences
 *
 * Note on Execution Model
 * -----------------------
 * Note that many of these functions result in cascading policy actions
 * that affect a wide range of entities. Some of these operations may
 * require a significant time to be completed. It is not necessary for
 * a function to block until all operations are completed. A delayed
 * callback to the management client informs it of the conclusion of
 * the operation.
 *****************************************************************************/

/******************************************************************************
 * Functions to manipulate AMS
 *****************************************************************************/

/*
 * clAmsMgmtInitialize
 * -------------------
 * Initialize/Start the use of the management API library.
 *
 * @param
 *   amsHandle                  - Handle returned by AMS to mgmt API user
 *   amsCallbacks               - Callbacks into mgmt API user
 *   version                    - Versions supported by AMS and user
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_VERSION_MISMATCH    - Error: Version not supported by AMS
 *
 */

ClRcT
clAmsMgmtInitialize(
        CL_OUT  ClAmsMgmtHandleT  *amsHandle,
        CL_IN  const ClAmsMgmtCallbacksT  *amsMgmtCallbacks,
        CL_INOUT  ClVersionT  *version)
{

    ClRcT  rc = CL_OK; 
#if defined(CL_AMS_MGMT_HOOKS)
    clAmsMgmtInitializeRequestT  req;
    clAmsMgmtInitializeResponseT  *res = NULL;
#endif
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT ( !amsHandle || !version );
   
    AMS_CHECK_RC_ERROR( check_lib_init() );

#if defined (CL_AMS_MGMT_HOOKS)
    memset(&req,0,sizeof(req));
    req.srcAddress.discriminant = CLAMSMGMTIOCADDRESSIDLTIOCPHYADDRESS;
    req.srcAddress.clAmsMgmtIocAddressIDLT.iocPhyAddress.nodeAddress=clIocLocalAddressGet();
    req.srcAddress.clAmsMgmtIocAddressIDLT.iocPhyAddress.portId = gpEOObj->eoPort;
#endif
    
    AMS_CHECK_RC_ERROR( clVersionVerify(&version_database,version) );

    AMS_CHECK_RC_ERROR( clHandleCreate(handle_database, 
                sizeof(struct ams_instance), amsHandle) );

    AMS_CHECK_RC_ERROR( clHandleCheckout( handle_database,*amsHandle,
                (ClPtrT)&ams_instance) );

    if (amsMgmtCallbacks) 
    {
        memcpy(&ams_instance->callbacks,
                amsMgmtCallbacks,
                sizeof(ClAmsMgmtCallbacksT));
    } 
    else 
    {
        memset(&ams_instance->callbacks,
                0,
                sizeof(ClAmsMgmtCallbacksT));
    }

    AMS_CHECK_RC_ERROR( clOsalMutexCreate(&ams_instance->response_mutex) );

#if defined(CL_AMS_MGMT_HOOKS)
    req.handle = *amsHandle;
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_initialize(&req, &res) );
    ams_instance->server_handle = res->handle;
#else
    ams_instance->server_handle = *amsHandle;
#endif

    AMS_CHECK_RC_ERROR( clHandleCheckin(handle_database,*amsHandle) );

exitfn:

#if defined(CL_AMS_MGMT_HOOKS)
    clAmsFreeMemory(res);
#endif

    return rc;
}

/*
 * clAmsMgmtFinalize
 * -----------------
 * Finalize/terminate the use of the management API library.
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
clAmsMgmtFinalize(
        CL_IN  ClAmsMgmtHandleT  amsHandle)
{

    ClRcT  rc = CL_OK;
#if defined(CL_AMS_MGMT_HOOKS)
    clAmsMgmtFinalizeRequestT  req;
    clAmsMgmtFinalizeResponseT  *res = NULL;
#endif
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECK_RC_ERROR( clHandleCheckout( handle_database, amsHandle, 
                (ClPtrT)&ams_instance));

    AMS_CHECK_RC_ERROR( clOsalMutexLock(ams_instance->response_mutex) );

    if (ams_instance->finalize)
    {
        AMS_CHECK_RC_ERROR( clOsalMutexUnlock(ams_instance->response_mutex));

        clHandleCheckin(handle_database, amsHandle);
        rc = CL_ERR_INVALID_HANDLE;
        goto exitfn;
    }

    ams_instance->finalize = 1;

    AMS_CHECK_RC_ERROR( clOsalMutexUnlock(ams_instance->response_mutex) );

#if defined(CL_AMS_MGMT_HOOKS)
    req.handle = ams_instance->server_handle;
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_finalize( &req, &res));
#endif

    AMS_CHECK_RC_ERROR( clOsalMutexDelete(ams_instance->response_mutex) );

    AMS_CHECK_RC_ERROR( clHandleDestroy(handle_database,amsHandle) );

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

#if defined(CL_AMS_MGMT_HOOKS)
    clAmsFreeMemory(res);
#endif

    return rc;

}

/******************************************************************************
 * Generic functions for all AMS entities
 *****************************************************************************/

/*
 * clAmsMgmtEntityCreate
 * --------------------------
 * Create a new instance of an AMS entity such as Node, Application, SG,
 * SU, SI, Component or CSI in the AMS entity database.
 *
 * The entity is created with a default config and only the entity name 
 * and type is set by default. All other entity attributes must be 
 * configured by calling the clAmsMgmtEntitySetConfig function. All 
 * entities are created with a management state of disabled and must be 
 * explicitly added into the active entity database.
 *
 * This function wraps equivalent AMS database API functions.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

ClRcT
clAmsMgmtEntityCreate(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityInstantiateRequestT  req;
    clAmsMgmtEntityInstantiateResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT ( !entity ) ;

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));

    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    rc = cl_ams_mgmt_entity_instantiate( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );
   
exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 * clAmsMgmtEntityDelete
 * ------------------------
 * Remove an instance of an AMS entity such as Node, Application, SG, SU, 
 * SI, Component or CSI from the AMS entity database. 
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * For all entities with an admin state (node, application, SG, SU, SI), 
 * the entity is first brought into a locked state, before being removed 
 * from the database. The state transitions will trigger the evaluation 
 * of necessary AMS policies. If state transitions are locked for an entity, 
 * the locks are overridden. Entities without an admin state (component 
 * and CSI) are removed from the appropriate data bases after removing 
 * the entities from service. 
 *
 * This function wraps equivalent AMS database and client API functions.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

ClRcT
clAmsMgmtEntityDelete(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityTerminateRequestT  req;
    clAmsMgmtEntityTerminateResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT ( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    rc = cl_ams_mgmt_entity_terminate( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 * clAmsMgmtEntityFindByAttribute
 * ------------------------------
 * Find an instance of an AMS entity in the appropriate AMS entity
 * database using an entity attribute as key. It is possible that 
 * this function will return multiple entities.
 *
 * This function wraps equivalent AMS database functions. It is
 * likely that specific functions will be needed for each entity
 * type.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name // XXX
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

// XXX Need to fix how attributes are indicated in the fn call

ClRcT
clAmsMgmtEntityFindByAttribute(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_INOUT  ClAmsEntityRefT  *entityRef )
{
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*
 * clAmsMgmtEntitySetConfig
 * ------------------------
 * Configure attributes for AMS entities.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *   attributeChangeList        - structure indicating which attributes to
 *                                change and values.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */


ClRcT
clAmsMgmtEntitySetConfig(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClAmsEntityConfigT  *entityConfig,
        CL_IN  ClUint32T  peInstantiateFlag )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetConfigRequestT  req;
    clAmsMgmtEntitySetConfigResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !entityConfig || !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    req.peInstantiateFlag = peInstantiateFlag;
    req.entityConfig = entityConfig;

    rc = cl_ams_mgmt_entity_set_config( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 * clAmsMgmtEntitySetAlphaFactor
 * ------------------------
 * Configure attributes for AMS entities.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *   alphaFactor                - SG alpha facator
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_PARAMETER   - Error: Invalid parameter
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */


ClRcT
clAmsMgmtEntitySetAlphaFactor(
                         CL_IN  ClAmsMgmtHandleT  amsHandle,
                         CL_IN  ClAmsEntityT  *entity,
                         CL_IN  ClUint32T alphaFactor)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetAlphaFactorRequestT  req;
    clAmsMgmtEntitySetAlphaFactorResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    if(entity->type != CL_AMS_ENTITY_TYPE_SG || alphaFactor > 100)
    {
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                                         (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy( &req.entity, entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    req.alphaFactor = alphaFactor;
    
    rc = cl_ams_mgmt_entity_set_alpha_factor( &req, &res);

    if(rc != CL_OK)
    {
        clHandleCheckin( handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

    exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT
clAmsMgmtEntitySetBetaFactor(
                         CL_IN  ClAmsMgmtHandleT  amsHandle,
                         CL_IN  ClAmsEntityT  *entity,
                         CL_IN  ClUint32T betaFactor)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetBetaFactorRequestT  req;
    clAmsMgmtEntitySetBetaFactorResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    if(entity->type != CL_AMS_ENTITY_TYPE_SG || betaFactor > 100)
    {
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                                         (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy( &req.entity, entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    req.betaFactor = betaFactor;
    
    rc = cl_ams_mgmt_entity_set_beta_factor( &req, &res);

    if(rc != CL_OK)
    {
        clHandleCheckin( handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

    exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 * clAmsMgmtEntitySetRef
 * ------------------------
 * Configure refernces for AMS entities.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   soureEntity                - Entity type and name to which reference has to be added
 *   targetEntity               - Entity type and name to whose reference has to be added
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */


ClRcT
clAmsMgmtEntitySetRef(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *sourceEntity,
        CL_IN  const ClAmsEntityT  *targetEntity )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntitySetRefRequestT  req;
    clAmsMgmtEntitySetRefResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !sourceEntity || !targetEntity);

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));
    req.handle = ams_instance->server_handle;
    memcpy ( &req.sourceEntity, sourceEntity, sizeof(ClAmsEntityT));
    memcpy ( &req.targetEntity, targetEntity, sizeof(ClAmsEntityT));

    rc = cl_ams_mgmt_entity_set_ref( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 * clAmsMgmtCSISetNVP
 * ------------------------
 * add name value pair to a CSI's name value pair list
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   soureEntity                - Entity type and name to which reference has to be added
     nvp                        - Name value pair to be added in CSI's nvp list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */


ClRcT
clAmsMgmtCSISetNVP(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *sourceEntity,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSINameValuePairT  nvp )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCSISetNVPRequestT  req;
    clAmsMgmtCSISetNVPResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !sourceEntity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.sourceEntity, sourceEntity, sizeof(ClAmsEntityT));
    memcpy ( &req.nvp, &nvp , sizeof(ClAmsCSINameValuePairT));

    rc = cl_ams_mgmt_csi_set_nvp( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 * clAmsMgmtEntityLockAssignment
 * -----------------------------
 * Prevents an AMS entity from taking work, all SIs are locked. This 
 * function is only valid for entities with an admin state.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityLockAssignmentExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT  retry)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityLockAssignmentRequestT  req;
    clAmsMgmtEntityLockAssignmentResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_entity_lock_assignment( &req, &res ));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT
clAmsMgmtEntityLockAssignment(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityLockAssignmentExtended(amsHandle, entity, CL_TRUE);
}

static ClRcT __amsLockInstantiation(ClAmsMgmtHandleT serverHandle, 
                                    const ClAmsEntityT *entity,
                                    ClBoolT retry,
                                    ClBoolT forceFlag)
{
    ClRcT  rc = CL_OK;
    clAmsMgmtEntityLockInstantiationRequestT  req;
    clAmsMgmtEntityLockInstantiationResponseT  *res = NULL;

    req.handle = serverHandle,
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL( 0, retry, rc, cl_ams_mgmt_entity_lock_instantiation( &req, forceFlag, &res));

     exitfn:
     clAmsFreeMemory(res);
     return rc;
}

static ClRcT amsLockInstantiation(ClAmsMgmtHandleT serverHandle, 
                                  const ClAmsEntityT *entity,
                                  ClBoolT retry)
{
    return __amsLockInstantiation(serverHandle, entity, retry, CL_FALSE);
}

static ClRcT amsForceLockInstantiation(ClAmsMgmtHandleT serverHandle, 
                                  const ClAmsEntityT *entity,
                                  ClBoolT retry)
{
    return __amsLockInstantiation(serverHandle, entity, retry, CL_TRUE);
}

ClRcT
clAmsMgmtEntityForceLockExtended(
                                 CL_IN  ClAmsMgmtHandleT  amsHandle,
                                 CL_IN  const ClAmsEntityT  *entity,
                                 CL_IN  ClUint32T lockFlags)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityForceLockRequestT req = {0};
    clAmsMgmtEntityForceLockResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                                         (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    req.lock  = (lockFlags & 1) ? CL_TRUE : CL_FALSE;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    /*
     * Force force lock and then take it to locked instantiation. 
     * If that fails, unlock the soft lock taken.
     */
    rc = cl_ams_mgmt_entity_force_lock( &req, &res );
    if(rc != CL_OK)
    {
        clLogError("OP", "FORCE-LOCK", "Force lock operation failed on entity [%s] with [%#x]",
                   req.entity.name.value, rc);
        goto exit_checkin;
    }

    /*
     * LockA + LockI
     */
    if(!(lockFlags ^ 3 ))
    {
        rc = amsLockInstantiation(req.handle, entity, CL_TRUE);

        if(rc != CL_OK)
        {
            clLogError("OP", "FORCE-LOCK", "Lock instantiation failed on entity [%s] with [%#x]",
                       req.entity.name.value, rc);
            clLogNotice("OP", "FORCE-LOCK", "Force lock reverting the soft lock");
            req.lock = CL_FALSE;
            clAmsFreeMemory(res);
            rc = cl_ams_mgmt_entity_force_lock(&req, &res);
            if(rc != CL_OK)
            {
                clLogError("OP", "FORCE-LOCK", "Force lock unlock also failed with [%#x] on entity [%s]",
                           rc, req.entity.name.value);
            }
            goto exitfn; /* handle already checked in by this point */
        }
    }

    exit_checkin:
    clHandleCheckin(handle_database, amsHandle);

    exitfn:
    clAmsFreeMemory (res);
    return rc;
}

ClRcT
clAmsMgmtEntityForceLock(
                         CL_IN  ClAmsMgmtHandleT  amsHandle,
                         CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityForceLockExtended(amsHandle, entity, 3);
}

ClRcT
clAmsMgmtEntityForceLockInstantiationExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    rc = amsForceLockInstantiation(ams_instance->server_handle, entity, retry);
    if(rc != CL_OK)
    {
        goto exitfn; /* a failure would have already checked in the handle */
    }

    AMS_CHECK_RC_ERROR(clHandleCheckin( handle_database, amsHandle));

exitfn:
    return rc;
}

ClRcT
clAmsMgmtEntityForceLockInstantiation(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityForceLockInstantiationExtended(amsHandle, entity, CL_TRUE);
}

/*
 * clAmsMgmtEntityLockInstantiation
 * --------------------------------
 * Prevents an AMS entity from being instantiated. This function is only
 * valid for entities with an admin state.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * The entity must be in the locked assignment state for this call to
 * be successful.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityLockInstantiationExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    rc = amsLockInstantiation(ams_instance->server_handle, entity, retry);
    if(rc != CL_OK)
    {
        goto exitfn; /* a failure would have already checked in the handle */
    }

    AMS_CHECK_RC_ERROR(clHandleCheckin( handle_database, amsHandle));

exitfn:
    return rc;
}

ClRcT
clAmsMgmtEntityLockInstantiation(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityLockInstantiationExtended(amsHandle, entity, CL_TRUE);
}

/*
 * clAmsMgmtEntityUnlock
 * ---------------------
 * Enables an AMS entity to be utilized by AMS. This function is only
 * valid for entities with an admin state.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityUnlockExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityUnlockRequestT  req;
    clAmsMgmtEntityUnlockResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT ( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_entity_unlock( &req, &res));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT
clAmsMgmtEntityUnlock(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityUnlockExtended(amsHandle, entity, CL_TRUE);
}

 
/*
 * clAmsMgmtEntityShutdown
 * -----------------------
 * Shutdown an AMS entity. This function is only valid for entities with 
 * an admin state.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityShutdownExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{
    ClRcT  rc = CL_OK;
    clAmsMgmtEntityShutdownRequestT  req;
    clAmsMgmtEntityShutdownResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !entity );
    
    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_entity_shutdown( &req, &res));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT
clAmsMgmtEntityShutdown(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityShutdownExtended(amsHandle, entity, CL_TRUE);
}

/*
 * clAmsMgmtEntityRestart
 * ----------------------
 * Restart an AMS entity. If there are any SIs or CSIs affected by this action,
 * then they are recovered as per the recovery policy for the restarted entity.
 * This function is valid for node, SU, application and component.
 *
 * As per Clovis extensions, if the SU is a container SU, then this fn is
 * executed recursively on all its contained SUs.
 *
 * If the recovery policy for all components in an SU is restart and it is
 * not disabled for any component, then no service assignements to other
 * entities need take place.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityRestartExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityRestartRequestT  req;
    clAmsMgmtEntityRestartResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_entity_restart( &req, &res));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}

ClRcT
clAmsMgmtEntityRestart(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityRestartExtended(amsHandle, entity, CL_TRUE);
}

/*
 * clAmsMgmtEntityRepaired
 * -----------------------
 * Indicate that a faulty AMS entity has been repaired and should be put back
 * into service. This function is only valid for node and SU.
 *
 * As per Clovis extensions, if the SU is a container SU, then this fn is
 * executed recursively on all its contained SUs.
 *
 * The entity is identified either by an handle, or if its null, then by 
 * the entity type and name. 
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   entity                     - Entity type and name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtEntityRepairedExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityRepairedRequestT  req;
    clAmsMgmtEntityRepairedResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy ( &req.entity , entity, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_entity_repaired( &req, &res));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}

ClRcT
clAmsMgmtEntityRepaired(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity)
{
    return clAmsMgmtEntityRepairedExtended(amsHandle, entity, CL_TRUE);
}


/*
 * clAmsMgmtSISwap
 * ----------------------
 * Swaps the HA states of the CSIs within the SI. It swaps the ha states
 * of the components of the service unit to which the swapped SI is assigned.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   si                         - SI name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_BAD_OPERATION       - Operation is not allowed on the SI
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: SI does not exist
 *   CL_ERR_TRY_AGAIN           - SI assigned service units dont have the right presence state
 *   CL_AMS_ERR_INVALID_ENTITY -  Operation not tried on SI
 *
 */

ClRcT
clAmsMgmtSISwapExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClCharT *si,
        CL_IN  ClBoolT retry)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtSISwapRequestT req = {0};
    clAmsMgmtSISwapResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECKPTR_SILENT( !si);

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT*)&ams_instance));

    req.handle = ams_instance->server_handle;
    req.entity.type = CL_AMS_ENTITY_TYPE_SI;
    clNameSet(&req.entity.name, si);
    ++req.entity.name.length;

    AMS_ADMIN_API_CALL(amsHandle, retry, rc, cl_ams_mgmt_si_swap( &req, &res));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;
}

ClRcT
clAmsMgmtSISwap(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClCharT *si)
{
    return clAmsMgmtSISwapExtended(amsHandle, si, CL_TRUE);
}


/*
 * clAmsMgmtSGAdjust
 * ----------------------
 * Adjust the SG back to the most preferred assignments based on the SU rank. After this call,
 * the SIs could undergo multiple relocations to bind with the most preferred SU.
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   sg                         - SG name
 *

 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_NOT_EXIST           - Error: SG does not exist
 *   CL_AMS_ERR_INVALID_ENTITY -  Operation not tried on SG
 *
 */

ClRcT clAmsMgmtSGAdjustExtended(ClHandleT handle, const ClCharT *sg, ClBoolT enable, ClBoolT retry)
{
    ClRcT rc = CL_OK;
    clAmsMgmtSGAdjustPreferenceRequestT request = {0};
    clAmsMgmtSGAdjustPreferenceResponseT *response = NULL;
    struct ams_instance *ams_instance = NULL;

    AMS_CHECKPTR_SILENT(!sg);

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database,
                                         handle,
                                         (ClPtrT)&ams_instance));

    request.handle = ams_instance->server_handle;
    request.enable = (enable == CL_TRUE ? 1 : 0);
    request.entity.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&request.entity.name, sg);
    ++request.entity.name.length;

    AMS_ADMIN_API_CALL(handle, retry, rc, cl_ams_mgmt_sg_adjust(&request, &response));

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, handle) );

exitfn:

    clAmsFreeMemory (response);
    return rc;
}

ClRcT clAmsMgmtSGAdjust(ClHandleT handle, const ClCharT *sg, ClBoolT enable)
{
    return clAmsMgmtSGAdjustExtended(handle, sg, enable, CL_TRUE);
}

/******************************************************************************
 * Entity Specific Functions
 *****************************************************************************/

/*
 * clAmsMgmtEntityListEntityRefAdd 
 * ---------------
 * Add target entity reference to the entity list of the source entity  
 *
 * The entity whose list has to be added is identified by sourceEntityRef, the entity
 * whose reference has to be added is identified by targetEntityRef. 
 * The list to which the target referece has to be added is identified by 
 * entityListName
 *
 * @param
 *   amsHandle                  - Handle assigned to API user
 *   sourceEntity               - Entity whose list has to be modified 
 *   targetEntity               - Entity whose reference has to be added 
 *   entityListName             - List name in the lists for the sourceEntity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_AMS_ERR_INVALID_LIST    - List does not exist for the given entity
 *   CL_ERR_NOT_EXIST           - Entity referece does not exist 
 *
 */

ClRcT 
clAmsMgmtEntityListEntityRefAdd(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  ClAmsEntityT  *sourceEntity,
        CL_IN  ClAmsEntityT  *targetEntity,
        CL_IN  ClAmsEntityListTypeT  entityListName)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityListEntityRefAddRequestT  req;
    clAmsMgmtEntityListEntityRefAddResponseT  *res = NULL;
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT ( !sourceEntity || !targetEntity );

    if ( entityListName <= CL_AMS_CONFIG_LIST_START || 
         entityListName >= CL_AMS_CONFIG_LIST_END )
    {
        return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_LIST);
    }

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = ams_instance->server_handle;
    memcpy (&req.sourceEntity,sourceEntity, sizeof (ClAmsEntityT));
    memcpy (&req.targetEntity,targetEntity, sizeof (ClAmsEntityT));
    req.entityListName = entityListName;

    rc = cl_ams_mgmt_entity_list_entity_ref_add( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtDebugEnable
 * ---------------
 * Enable debugging for the given entity 
 *
 *
 * @param
 *   amsHandle                 - Handle assigned to API user
 *   entity                    - Entity type and name for which debugging needs  *                               to be enabled
 *  debugFlags               - specific to AMS , defines the section of the code for which the debugging needs to be enabled
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtDebugEnable(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClUint8T  debugFlags )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtDebugEnableRequestT  req;
    clAmsMgmtDebugEnableResponseT  *res = NULL; 
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    memset (&req,0,sizeof (clAmsMgmtDebugEnableRequestT));

    if ( !entity )
    {
        req.handle = amsHandle;
        req.entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
        req.debugFlags = debugFlags;
    }
    else
    {
        req.handle = amsHandle;
        memcpy ( &req.entity, entity, sizeof (ClAmsEntityT));
        req.debugFlags = debugFlags;
    }

    rc = cl_ams_mgmt_debug_enable( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    return rc;

}

/*
 *
 * clAmsMgmtDebugDisable
 * ---------------
 * Disable debugging for the given entity 
 *
 *
 * @param
 *   amsHandle                 - Handle assigned to API user
 *   entity                    - Entity type and name for which debugging needs  *                               to be disabled
 *  debugFlags               - specific to AMS , defines the section of the code for which the debugging needs to be disabled
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtDebugDisable(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClUint8T  debugFlags )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtDebugDisableRequestT  req;
    clAmsMgmtDebugDisableResponseT  *res = NULL; 
    struct ams_instance  *ams_instance = NULL;

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));
    memset (&req,0,sizeof (clAmsMgmtDebugDisableRequestT));

    if ( !entity )
    {
        req.entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
        req.handle = amsHandle;
        req.debugFlags = debugFlags;
    }
    else
    {
        req.handle = amsHandle;
        memcpy ( &req.entity, entity, sizeof (ClAmsEntityT));
        req.debugFlags = debugFlags;
    }

    rc = cl_ams_mgmt_debug_disable( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    return rc;

}


/*
 *
 * clAmsMgmtDebugGet
 * ---------------
 * returns debugging information for the given entity 
 *
 *
 * @param
 *   amsHandle                 - Handle assigned to API user
 *   entity                    - Entity type and name for which debugging info i
 *                               is requested
 *  debugFlags               - specific to AMS , defines the section of the 
 *                               code for which the debugging information has 
 *                               been requested
 @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *   CL_ERR_INVALID_STATE       - Error: The operation is not valid in the
 *                                current state
 *   CL_ERR_NOT_EXIST           - Error: Resource/Entity does not exist
 *
 */

ClRcT
clAmsMgmtDebugGet(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_OUT  ClUint8T  *debugFlags )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtDebugGetRequestT  req;
    clAmsMgmtDebugGetResponseT  *res = NULL; 
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECKPTR_SILENT( !debugFlags );

    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    memset (&req,0,sizeof (clAmsMgmtDebugGetRequestT));

    if ( !entity )
    {
        req.entity.type = CL_AMS_ENTITY_TYPE_ENTITY;
        req.handle = amsHandle;
    }
    else
    {
        req.handle = amsHandle;
        memcpy ( &req.entity, entity, sizeof (ClAmsEntityT));
    }

    rc = cl_ams_mgmt_debug_get( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

    *debugFlags = res->debugFlags;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtDebugEnableLogToConsole
 * --------------------------------
 * Enables AMS debugging messages to be displayed on the console 
 *
 *
 * @param
 *   amsHandle                 - Handle assigned to API user
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

ClRcT
clAmsMgmtDebugEnableLogToConsole(
        CL_IN  ClAmsMgmtHandleT  amsHandle )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtDebugEnableLogToConsoleRequestT  req = {0};
    clAmsMgmtDebugEnableLogToConsoleResponseT  *res = NULL; 
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = amsHandle;

    rc = cl_ams_mgmt_debug_enable_log_to_console( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtDebugDisableLogToConsole
 * --------------------------------
 * Disables AMS debugging messages to be displayed on the console 
 *
 *
 * @param
 *   amsHandle                 - Handle assigned to API user
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: Handle was invalid
 *
 */

ClRcT
clAmsMgmtDebugDisableLogToConsole(
        CL_IN  ClAmsMgmtHandleT  amsHandle )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtDebugDisableLogToConsoleRequestT  req = {0};
    clAmsMgmtDebugDisableLogToConsoleResponseT  *res = NULL; 
    struct ams_instance  *ams_instance = NULL;


    AMS_CHECK_RC_ERROR( clHandleCheckout(handle_database, amsHandle,
                (ClPtrT)&ams_instance));

    req.handle = amsHandle;

    rc = cl_ams_mgmt_debug_disable_log_to_console( &req, &res);
    if(rc != CL_OK)
    {
        clHandleCheckin(handle_database, amsHandle);
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clHandleCheckin( handle_database, amsHandle) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}


/*
 *
 * clAmsMgmtCCBInitialize
 * --------------------------------
 * Creates and Initializes context for AML configuration APIs. 
 *
 *
 * @param
 *   amlHandle                 - Handle assigned to API user
 *   ccbHandle                 - CCB Handle returned by the AML. This should be used for all
 *                             - subsequent AML configurations APIs
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML Mgmt Handle is invalid
 *
 */

ClRcT
clAmsMgmtCCBInitialize(
        CL_IN   ClAmsMgmtHandleT    amlHandle,
        CL_OUT  ClAmsMgmtCCBHandleT *ccbHandle )
{

    ClRcT  rc = CL_OK; 
    clAmsMgmtCCBInitializeRequestT  req;
    clAmsMgmtCCBInitializeResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !ccbHandle);

    /*
     * XXX: Verify the AMS Mgmt Handle here
     */
   
    /* 
     * Get the CCB handle from the server 
     */

    req.handle = amlHandle;
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_initialize(&req, &res) );

    *ccbHandle = res->handle;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBFinalize
 * --------------------------------
 * deletes and finalizes specifc context for AML configuration in AMS server and AML. 
 *
 *
 * @param
 *   ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                             - clAmsMgmtCCBInitialize API.
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT
clAmsMgmtCCBFinalize(
        CL_IN   ClAmsMgmtCCBHandleT ccbHandle )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBFinalizeRequestT  req;
    clAmsMgmtCCBFinalizeResponseT  *res = NULL;

    req.handle = ccbHandle;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_finalize( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}


/*
 *
 * clAmsMgmtCCBEntitySetConfig
 * --------------------------------
 * Sets one or more scalar attributes of an AMS entity
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  entityConfig              - New configuration values for the AMS entity attribute(s) whose configuration 
 *                            - is being set. AMS entity name and type is also specified in this structure.
 *
 *  bitmask                   - mask specifying the attribute(s) whose value is being set to new values
 *                             
    
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBEntitySetConfig(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityConfigT    *entityConfig,
        CL_IN   ClUint64T   bitMask )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBEntitySetConfigRequestT  req;
    clAmsMgmtCCBEntitySetConfigResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !entityConfig);


    req.handle = handle;
    req.bitmask = bitMask;
    req.entityConfig = entityConfig;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_entity_set_config( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBCSISetNVP
 * --------------------------------
 * Sets or creates name value pair for a CSI. AMS creates the name value pair if the
 * name specified does not already exist for the CSI.
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  csiName                   - Name of the CSI for which the name value pair is being set or created  
 *
 *  nvp                       - name and value pair for the CSI
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */



ClRcT clAmsMgmtCCBCSISetNVP(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsCSINVPT    *nvp )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBCSISetNVPRequestT  req;
    clAmsMgmtCCBCSISetNVPResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !csiName || !nvp );

    req.handle = handle;
    memcpy (&req.csiName, csiName, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    memcpy (&req.nvp, nvp, sizeof(ClAmsCSINVPT));

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_csi_set_nvp( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBCSIDeleteNVP
 * --------------------------------
 * deletes name value pair for a CSI. 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  csiName                   - Name of the CSI for which the name value pair is being deleted  
 *
 *  nvp                       - name and value pair for the CSI to be deleted
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBCSIDeleteNVP(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsCSINVPT    *nvp )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBCSIDeleteNVPRequestT  req;
    clAmsMgmtCCBCSIDeleteNVPResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !csiName || !nvp );

    req.handle = handle;
    memcpy (&req.csiName, csiName, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    memcpy (&req.nvp, nvp, sizeof(ClAmsCSINVPT));

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_csi_delete_nvp( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBSetNodeDependency
 * --------------------------------
 *  
 *  Adds a node in the node dependencies list for a AMS node
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  nodeName                  - Name of the node whose node dependencies list is being modified 
 *
 *  dependencyNodeName        - dependency node name to be added in the node's dependencies list 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetNodeDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *dependencyNodeName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetNodeDependencyRequestT  req;
    clAmsMgmtCCBSetNodeDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !nodeName || !dependencyNodeName );

    req.handle = handle;
    memcpy(&req.nodeName, nodeName, sizeof(ClAmsEntityT));
    memcpy(&req.dependencyNodeName, dependencyNodeName, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyNodeName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_node_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT clAmsMgmtCCBDeleteNodeDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *dependencyNodeName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetNodeDependencyRequestT  req;
    clAmsMgmtCCBSetNodeDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !nodeName || !dependencyNodeName );

    req.handle = handle;
    memcpy(&req.nodeName, nodeName, sizeof(ClAmsEntityT));
    memcpy(&req.dependencyNodeName, dependencyNodeName, sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyNodeName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_node_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBSetNodeSUList
 * --------------------------------
 *  
 *  Adds a su in the node-su list of a node
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  nodeName                  - Name of the node whose su list is being modified 
 *
 *  suName                    - Name of the new su being added to node-su list 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetNodeSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *suName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetNodeSUListRequestT  req;
    clAmsMgmtCCBSetNodeSUListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !nodeName || !suName );

    req.handle = handle;
    memcpy (&req.nodeName,nodeName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_node_su_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}

ClRcT clAmsMgmtCCBDeleteNodeSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *suName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetNodeSUListRequestT  req;
    clAmsMgmtCCBSetNodeSUListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !nodeName || !suName );

    req.handle = handle;
    memcpy (&req.nodeName,nodeName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_node_su_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;


}


/*
 *
 * clAmsMgmtCCBSetSGSUList
 * --------------------------------
 *  
 *  Adds a su in the sg-su list of a sg 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  sgName                    - Name of the sg whose su list is being modified 
 *
 *  suName                    - Name of the new su being added to sg-su list 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSGSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *suName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSUListRequestT  req;
    clAmsMgmtCCBSetSGSUListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sgName || !suName );

    req.handle = handle;
    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_sg_su_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT clAmsMgmtCCBDeleteSGSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *suName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSUListRequestT  req;
    clAmsMgmtCCBSetSGSUListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sgName || !suName );

    req.handle = handle;
    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_sg_su_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBSetSGSIList
 * --------------------------------
 *  
 *  Adds a si in the sg-si list of a sg 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  sgName                    - Name of the sg whose si list is being modified 
 *
 *  siName                    - Name of the new si being added to sg-si list 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSGSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *siName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSIListRequestT  req;
    clAmsMgmtCCBSetSGSIListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sgName || !siName )

    req.handle = handle;
    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    AMS_CHECK_RC_ERROR(  cl_ams_mgmt_ccb_set_sg_si_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtCCBDeleteSGSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *siName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSGSIListRequestT  req;
    clAmsMgmtCCBSetSGSIListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sgName || !siName )

    req.handle = handle;
    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    AMS_CHECK_RC_ERROR(  cl_ams_mgmt_ccb_delete_sg_si_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBSetSUCompList
 * --------------------------------
 *  
 *  Adds a component in the su-comp list of a su 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  suName                    - Name of the su whose component list is being modified 
 *
 *  compName                  - Name of the new component being added to su-comp list 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSUCompList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *suName,
        CL_IN   ClAmsEntityT    *compName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSUCompListRequestT  req;
    clAmsMgmtCCBSetSUCompListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !suName || !compName );

    req.handle = handle;
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    memcpy (&req.compName,compName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    CL_AMS_NAME_LENGTH_CHECK(req.compName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_su_comp_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtCCBDeleteSUCompList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *suName,
        CL_IN   ClAmsEntityT    *compName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSUCompListRequestT  req;
    clAmsMgmtCCBSetSUCompListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !suName || !compName );

    req.handle = handle;
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    memcpy (&req.compName,compName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    CL_AMS_NAME_LENGTH_CHECK(req.compName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_su_comp_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBSetSISURankList
 * --------------------------------
 *  
 *  Adds a su in the si-su rank list of a si 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  siName                    - Name of the si whose si-su rank list is being modified 
 *
 *  suName                    - Name of the new su being added to si-su rank list 
 *
 *  suRank                    - rank of the su for the si 
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSISURankList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *suName)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISURankListRequestT  req;
    clAmsMgmtCCBSetSISURankListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !suName );
    
    memset(&req, 0, sizeof(req));
    req.handle = handle;
    req.suRank = 0;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_si_su_rank_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT clAmsMgmtCCBDeleteSISURankList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *suName)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISURankListRequestT  req;
    clAmsMgmtCCBSetSISURankListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !suName );

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    req.suRank = 0;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_si_su_rank_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBSetSIDependency
 * --------------------------------
 *  
 *  Adds a si in the si-si dependency list of a si 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  siName                    - Name of the si whose si-si dependency list is being modified 
 *
 *  dependencySIName          - dependency si to be addded in the si-si dependency list 
 *
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *dependencySIName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISIDependencyRequestT  req;
    clAmsMgmtCCBSetSISIDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !dependencySIName );

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.dependencySIName,dependencySIName,sizeof(ClAmsEntityT));

    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencySIName);
    
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_si_si_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBSetCSIDependency
 * --------------------------------
 *  
 *  Adds a csi in the csi-csi dependency list of a csi 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  csiName                    - Name of the csi whose csi-csi dependency list is being modified 
 *
 *  dependencyCSIName          - dependency csi to be addded in the csi-csi dependency list 
 *
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetCSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsEntityT    *dependencyCSIName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetCSICSIDependencyRequestT  req;
    clAmsMgmtCCBSetCSICSIDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !csiName || !dependencyCSIName );

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    memcpy (&req.csiName, csiName, sizeof(ClAmsEntityT));
    memcpy (&req.dependencyCSIName,dependencyCSIName,sizeof(ClAmsEntityT));

    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyCSIName);
    
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_csi_csi_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtCCBDeleteSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *dependencySIName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSISIDependencyRequestT  req;
    clAmsMgmtCCBSetSISIDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !dependencySIName );

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.dependencySIName,dependencySIName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencySIName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_si_si_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtCCBDeleteCSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsEntityT    *dependencyCSIName )

{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetCSICSIDependencyRequestT  req;
    clAmsMgmtCCBSetCSICSIDependencyResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !csiName || !dependencyCSIName );

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    memcpy (&req.csiName, csiName, sizeof(ClAmsEntityT));
    memcpy (&req.dependencyCSIName,dependencyCSIName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyCSIName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_csi_csi_dependency( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBSetSICSIList
 * --------------------------------
 *  
 *  Adds a csi in the si-csi list of a si 
 *
 *
 * @param
 *  ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                            - clAmsMgmtCCBInitialize API.
 *
 *  siName                    - Name of the si whose si-csi list is being modified 
 *
 *  csiName                   - Name of the new csi to be addded in the si-csi list 
 *
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBSetSICSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *csiName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSICSIListRequestT  req;
    clAmsMgmtCCBSetSICSIListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !csiName );

    req.handle = handle;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.csiName,csiName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_set_si_csi_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClRcT clAmsMgmtCCBDeleteSICSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *csiName )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBSetSICSIListRequestT  req;
    clAmsMgmtCCBSetSICSIListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !siName || !csiName );

    req.handle = handle;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.csiName,csiName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_delete_si_csi_list( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtCCBEntityCreate
 * --------------------------------
 * Creates a new instance of an AMS entity 
 *
 *
 * @param
 *   ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                               clAmsMgmtCCBInitialize API.
 *   entity                    - New AMS entity name and type to be created        
 *                             
    
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBEntityCreate(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   const ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBEntityCreateRequestT  req;
    clAmsMgmtCCBEntityCreateResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    req.handle = handle;
    memcpy (&req.entity,entity,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_entity_create( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBEntityDelete
 * --------------------------------
 * Deletes a instance of an AMS entity from AMS DB.
 *
 *
 * @param
 *   ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                               clAmsMgmtCCBInitialize API.
 *   entity                    - AMS entity name and type to be deleted        
 *                             
    
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtCCBEntityDelete(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   const ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBEntityDeleteRequestT  req;
    clAmsMgmtCCBEntityDeleteResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    req.handle = handle;
    memcpy (&req.entity,entity,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_entity_delete( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtCCBCommit
 * --------------------------------
 * Applies the operation specific to CCB context in AMS DB atomically.
 *
 *
 * @param
 *   ccbHandle                 - CCB Handle returned by the AML on invocation of 
 *                             - clAmsMgmtCCBInitialize API.
        
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT
clAmsMgmtCCBCommit(
        CL_IN  ClAmsMgmtCCBHandleT  ccbHandle )
{

    ClRcT  rc = CL_OK;
    clAmsMgmtCCBCommitRequestT  req;
    clAmsMgmtCCBCommitResponseT  *res = NULL;

    req.handle = ccbHandle;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_ccb_commit( &req, &res) );

exitfn:

    clAmsFreeMemory (res);
    return rc;

}



/*
 *
 * clAmsMgmtEntityGet
 * --------------------------------
 *  
 * returns the configuration and status scalar attributes of an AMS entity
 *
 *
 * @param
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  entity                    - Name of the AMS entity whose configuration and status attributes
 *                            - are queried. The attributes are also returned in this structure.
 *
 * 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtEntityGet(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_INOUT    ClAmsEntityRefT    *entityRef)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityGetRequestT  req;
    clAmsMgmtEntityGetResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !entityRef );

    req.handle = handle;
    memcpy ( &req.entity, &entityRef->entity, sizeof(ClAmsEntityT) );

    AMS_CHECK_RC_ERROR ( cl_ams_mgmt_entity_get( &req, &res) );

    entityRef->ptr = res->entity;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtEntityGetConfig
 * --------------------------------
 *  
 * returns the configuration scalar attributes of an AMS entity
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  entityName                - Name of the AMS entity whose configuration attributes are queried
 *
 *  entityConfig              - Entity configuration structure returned by AML
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtEntityGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN   ClAmsEntityT    *entity,
        CL_OUT  ClAmsEntityConfigT  **entityConfig)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityGetConfigRequestT  req;
    clAmsMgmtEntityGetConfigResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !entity || !entityConfig );

    req.handle = handle;
    memcpy ( &req.entity, entity, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_entity_get_config( &req, &res) );

    *entityConfig = res->entityConfig;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClAmsNodeConfigT* clAmsMgmtNodeGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsNodeConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_NODE;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;    
}

ClAmsSGConfigT* clAmsMgmtServiceGroupGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSGConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsSUConfigT* clAmsMgmtServiceUnitGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSUConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsSIConfigT* clAmsMgmtServiceInstanceGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSIConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SI;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsCSIConfigT* clAmsMgmtCompServiceInstanceGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsCSIConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_CSI;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsCompConfigT* clAmsMgmtCompGetConfig(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsCompConfigT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_COMP;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetConfig(handle, &entity, (ClAmsEntityConfigT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}






/*
 *
 * clAmsMgmtEntityGetStatus
 * --------------------------------
 *  
 * returns the status (transient) scalar attributes of an AMS entity
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  entity                    - Name of the AMS entity whose status attributes are queried
 *
 *  entityStatus              - Entity status structure returned by AML
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtEntityGetStatus(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *entity,
        CL_OUT  ClAmsEntityStatusT  **entityStatus)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtEntityGetStatusRequestT  req;
    clAmsMgmtEntityGetStatusResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!entity || !entityStatus );

    req.handle = handle;
    memcpy ( &req.entity, entity, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_entity_get_status( &req, &res) );

    *entityStatus= res->entityStatus;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


ClAmsNodeStatusT* clAmsMgmtNodeGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsNodeStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_NODE;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;    
}

ClAmsSGStatusT* clAmsMgmtServiceGroupGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSGStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsSUStatusT* clAmsMgmtServiceUnitGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSUStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SU;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsSIStatusT* clAmsMgmtServiceInstanceGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsSIStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_SI;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsCSIStatusT* clAmsMgmtCompServiceInstanceGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsCSIStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_CSI;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}

ClAmsCompStatusT* clAmsMgmtCompGetStatus(CL_IN   ClAmsMgmtHandleT    handle, CL_IN const ClCharT *entName)
{
    ClRcT  rc = CL_OK;
    ClAmsCompStatusT* ret;
    ClAmsEntityT entity = {0};
    entity.type = CL_AMS_ENTITY_TYPE_COMP;
    clNameSet(&entity.name, entName);
    entity.name.length++;  /* Strange AMS behavior requires the length to include the \0 */
    
    rc = clAmsMgmtEntityGetStatus(handle, &entity, (ClAmsEntityStatusT**) &ret);
    if(rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) /* Don't log because the caller may be doing existence checking */       
            clLogWarning(AMSAREA,AMSCTXT,"Possibly benign return code [0x%x] in function %s when accessing entity %s",rc,__func__,entName);   
        return NULL;
    }    
    return ret;
}


/*
 *
 * clAmsMgmtGetCSINVPList
 * --------------------------------
 *  
 * returns the name value pair list for a csi 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  csi                       - Name of the csi whose nvp list is queried 
 *
 *  nvpBuffer                 - Buffer containing the nvp list for the csi 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetCSINVPList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *csi,
        CL_OUT  ClAmsCSINVPBufferT  *nvpBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetCSINVPListRequestT  req;
    clAmsMgmtGetCSINVPListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !csi || !nvpBuffer );

    req.handle = handle;
    memcpy ( &req.csi, csi, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.csi);
    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_csi_nvp_list( &req, &res) );

    memcpy ( nvpBuffer,res,sizeof (ClAmsCSINVPBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetCSIDependenciesList
 * --------------------------------------
 *  
 * returns the csi-csi dependencies list for a csi 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  csi                       - Name of the csi whose csi-csi dependencies list is queried 
 *
 *  dependenciesCSIBuffer     - Buffer containing the csi-csi dependencies list for the csi 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */

ClRcT clAmsMgmtGetCSIDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *csi,
        CL_OUT  ClAmsEntityBufferT  *dependenciesCSIBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!csi || !dependenciesCSIBuffer );

    req.handle = handle;
    memcpy ( &req.entity, csi, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( dependenciesCSIBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtGetList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityListTypeT listName,
        CL_OUT  ClAmsEntityBufferT  *buffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !buffer );

    req.handle = handle;
    memset(&req.entity, 0, sizeof(ClAmsEntityT));
    req.entityListName = listName;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( buffer, res, sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtGetSGList(ClAmsMgmtHandleT handle,
                         ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_SG_LIST, buffer);
}

ClRcT clAmsMgmtGetSIList(ClAmsMgmtHandleT handle,
                         ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_SI_LIST, buffer);
}

ClRcT clAmsMgmtGetCSIList(ClAmsMgmtHandleT handle,
                         ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_CSI_LIST, buffer);
}

ClRcT clAmsMgmtGetNodeList(ClAmsMgmtHandleT handle,
                           ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_NODE_LIST, buffer);
}

ClRcT clAmsMgmtGetSUList(ClAmsMgmtHandleT handle,
                         ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_SU_LIST, buffer);
}

ClRcT clAmsMgmtGetCompList(ClAmsMgmtHandleT handle,
                           ClAmsEntityBufferT *buffer)
{
    return clAmsMgmtGetList(handle, CL_AMS_COMP_LIST, buffer);
}

/*
 *
 * clAmsMgmtGetNodeDependenciesList
 * --------------------------------------
 *  
 * returns the node dependencies list for a node 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  node                      - Name of the node whose node dependencies list is queried 
 *
 *  dependencyBuffer          - Buffer containing the node dependencies list for the node 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetNodeDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *dependencyBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !node || !dependencyBuffer );

    req.handle = handle;
    memcpy ( &req.entity, node, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( dependencyBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetNodeSUList
 * --------------------------------------
 *  
 * returns the node su list for a node 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  node                      - Name of the node whose su list is queried 
 *
 *  suBuffer                  - Buffer containing the node su list for the node 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetNodeSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *suBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !node || !suBuffer );

    req.handle = handle;
    memcpy ( &req.entity, node, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_NODE_CONFIG_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( suBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetSGSUList
 * --------------------------------------
 *  
 * returns the sg su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose su list is queried 
 *
 *  suBuffer                  - Buffer containing the sg su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *suBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sg || !suBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    req.entityListName = CL_AMS_SG_CONFIG_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );
    memcpy ( suBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSGSIList
 * --------------------------------------
 *  
 * returns the sg si list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose si list is queried 
 *
 *  siBuffer                  - Buffer containing the sg si list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *siBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sg || !siBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SG_CONFIG_SI_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( siBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSUCompList
 * --------------------------------------
 *  
 * returns the component list for a su 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  su                        - Name of the su whose component list is queried 
 *
 *  compBuffer                - Buffer containing the component list for the su 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSUCompList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsEntityBufferT  *compBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!su || !compBuffer );

    req.handle = handle;
    memcpy ( &req.entity, su, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SU_CONFIG_COMP_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( compBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetSISURankList
 * --------------------------------------
 *  
 * returns the si-su rank list for a si 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  si                        - Name of the si whose su rank list is queried 
 *
 *  suBuffer                  - Buffer containing the su rank list for the si 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSISURankList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *suBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !si || !suBuffer );

    req.handle = handle;
    memcpy ( &req.entity, si, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SI_CONFIG_SU_RANK_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( suBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSIDependenciesList
 * --------------------------------------
 *  
 * returns the si-si dependencies list for a si 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  si                        - Name of the si whose si-si dependencies list is queried 
 *
 *  dependenciesSIBuffer      - Buffer containing the si-si dependencies list for the si 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSIDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *dependenciesSIBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!si || !dependenciesSIBuffer );

    req.handle = handle;
    memcpy ( &req.entity, si, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( dependenciesSIBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSICSIList
 * --------------------------------------
 *  
 * returns the si-csi list for a si 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  si                        - Name of the si whose si-csi list is queried 
 *
 *  csiBuffer                 - Buffer containing the csi list for the si 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSICSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *csiBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !si || !csiBuffer );

    req.handle = handle;
    memcpy ( &req.entity, si, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SI_CONFIG_CSI_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res));

    memcpy ( csiBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetSGInstantiableSUList
 * --------------------------------------
 *  
 * returns the instantiable su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose instantiable su list is queried 
 *
 *  instantiableSUBuffer      - Buffer containing the instantiable su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGInstantiableSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *instantiableSUBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sg || !instantiableSUBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( instantiableSUBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSGInstantiatedSUList
 * --------------------------------------
 *  
 * returns the instantiated su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose instantiated su list is queried 
 *
 *  instantiatedSUBuffer      - Buffer containing the instantiated su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGInstantiatedSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *instantiatedSUBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sg || !instantiatedSUBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( instantiatedSUBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSGInServiceSpareSUList
 * --------------------------------------
 *  
 * returns the in service spare su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose inservice spare su list is queried 
 *
 *  inserviceSpareSUBuffer    - Buffer containing the inservice spare su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGInServiceSpareSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *inserviceSpareSUBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!sg || !inserviceSpareSUBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);

    req.entityListName = CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( inserviceSpareSUBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSGAssignedSUList
 * --------------------------------------
 *  
 * returns the assigned su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose assigned su list is queried 
 *
 *  assignedSUBuffer          - Buffer containing the assigned su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGAssignedSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *assignedSUBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !sg || !assignedSUBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SG_STATUS_ASSIGNED_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( assignedSUBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSGFaultySUList
 * --------------------------------------
 *  
 * returns the faulty su list for a sg 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  sg                        - Name of the sg whose faulty su list is queried 
 *
 *  faultySUBuffer            - Buffer containing the faulty su list for the sg 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSGFaultySUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *faultySUBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!sg || !faultySUBuffer );

    req.handle = handle;
    memcpy ( &req.entity, sg, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SG_STATUS_FAULTY_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_entity_list( &req, &res) );

    memcpy ( faultySUBuffer,res,sizeof (ClAmsEntityBufferT));

exitfn:

    clAmsFreeMemory (res);
    return rc;

}


/*
 *
 * clAmsMgmtGetSUAssignedSIsList
 * --------------------------------------
 *  
 * returns the assigned si's list for su 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  su                        - Name of the su whose assigned si's list is queried 
 *
 *  siBuffer                  - Buffer containing the si's list for the su 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSUAssignedSIsList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsSUSIRefBufferT  *siBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetOLEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !su || !siBuffer );

    req.handle = handle;
    memcpy ( &req.entity, su, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SU_STATUS_SI_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_ol_entity_list( &req, &res) );

    siBuffer->count = res->count;
    siBuffer->entityRef = (ClAmsSUSIRefT *)res->entityRef;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtGetSUAssignedSIsExtendedList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsSUSIExtendedRefBufferT  *siBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetOLEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !su || !siBuffer );

    req.handle = handle;
    memcpy ( &req.entity, su, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SU_STATUS_SI_EXTENDED_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_ol_entity_list( &req, &res) );

    siBuffer->count = res->count;
    siBuffer->entityRef = (ClAmsSUSIExtendedRefT *)res->entityRef;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetSISUList
 * --------------------------------------
 *  
 * returns the su list for si 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  si                        - Name of the si whose su list is queried 
 *
 *  suBuffer                  - Buffer containing the su list for the si 
 *
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetSISUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsSISURefBufferT  *suBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetOLEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!si || !suBuffer );

    req.handle = handle;
    memcpy ( &req.entity, si, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SI_STATUS_SU_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_ol_entity_list( &req, &res) );

    suBuffer->count = res->count;
    suBuffer->entityRef = (ClAmsSISURefT *)res->entityRef;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtGetSISUExtendedList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsSISUExtendedRefBufferT  *suBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetOLEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT(!si || !suBuffer );

    req.handle = handle;
    memcpy ( &req.entity, si, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_SI_STATUS_SU_EXTENDED_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_ol_entity_list( &req, &res) );

    suBuffer->count = res->count;
    suBuffer->entityRef = (ClAmsSISUExtendedRefT *)res->entityRef;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

/*
 *
 * clAmsMgmtGetCompCSIList
 * --------------------------------------
 *  
 * returns the list of csi's assigned to a component 
 *
 *
 * @param
 *
 *  handle                    - Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 *
 *  comp                      - Name of the component whose csi list is queried 
 *
 *  csiBuffer                 - Buffer containing the list of csi's assigned to the component 
 *
 *
 * @returns
 *   CL_OK                      - Operation successful
 *   CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 *
 */


ClRcT clAmsMgmtGetCompCSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *comp,
        CL_OUT  ClAmsCompCSIRefBufferT  *csiBuffer)
{

    ClRcT  rc = CL_OK;
    clAmsMgmtGetEntityListRequestT  req;
    clAmsMgmtGetOLEntityListResponseT  *res = NULL;

    AMS_CHECKPTR_SILENT( !comp || !csiBuffer );

    req.handle = handle;
    memcpy ( &req.entity, comp, sizeof(ClAmsEntityT) );
    CL_AMS_NAME_LENGTH_CHECK(req.entity);
    req.entityListName = CL_AMS_COMP_STATUS_CSI_LIST;

    AMS_CHECK_RC_ERROR( cl_ams_mgmt_get_ol_entity_list( &req, &res) );

    csiBuffer->count = res->count;
    csiBuffer->entityRef = (ClAmsCompCSIRefT *)res->entityRef;

exitfn:

    clAmsFreeMemory (res);
    return rc;

}

ClRcT clAmsMgmtFreeCompCSIRefBuffer(ClAmsCompCSIRefBufferT *buffer)
{
    ClUint32T i;
    for(i = 0; i < buffer->count; ++i)
    {
        if(buffer->entityRef[i].activeComp)
        {
            clHeapFree(buffer->entityRef[i].activeComp);
            buffer->entityRef[i].activeComp = NULL;
        }
    }
    clHeapFree(buffer->entityRef);
    buffer->entityRef = NULL;
    return CL_OK;
}

ClRcT clAmsMgmtMigrateSG(ClAmsMgmtHandleT handle,
                         const ClCharT *sg,
                         const ClCharT *prefix,
                         ClUint32T activeSUs,
                         ClUint32T standbySUs,
                         ClAmsMgmtMigrateListT *migrateList)
{
    ClRcT rc = CL_OK;
    ClAmsMgmtMigrateRequestT request;
    ClAmsMgmtMigrateResponseT response;

    AMS_CHECKPTR_SILENT(!sg || !prefix);
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    clNameSet(&request.sg, sg);
    clNameSet(&request.prefix, prefix);
    ++request.sg.length;
    ++request.prefix.length;
    request.activeSUs = activeSUs;
    request.standbySUs = standbySUs;

    rc = cl_ams_mgmt_migrate_sg(&request, &response);
    if(rc == CL_OK)
    {
        if(migrateList)
            memcpy(migrateList, &response.migrateList, sizeof(*migrateList));
        else
        {
            if(response.migrateList.si.entity) clHeapFree(response.migrateList.si.entity);
            if(response.migrateList.csi.entity) clHeapFree(response.migrateList.csi.entity);
            if(response.migrateList.node.entity) clHeapFree(response.migrateList.node.entity);
            if(response.migrateList.su.entity) clHeapFree(response.migrateList.su.entity);
            if(response.migrateList.comp.entity) clHeapFree(response.migrateList.comp.entity);
        }
    }

    return rc;
}

ClRcT clAmsMgmtEntityUserDataSet(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT *entity,
                                 ClCharT *data,
                                 ClUint32T len)
{
    ClAmsMgmtUserDataSetRequestT request = {0};
    AMS_CHECKPTR_SILENT( !entity );
    if(!data && len) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    request.entity = entity;
    request.key = NULL;
    request.data = data;
    request.len = len;
    return cl_ams_mgmt_user_data_set(&request);
}

ClRcT clAmsMgmtEntityUserDataSetKey(ClAmsMgmtHandleT handle,
                                    ClAmsEntityT *entity,
                                    ClNameT *key,
                                    ClCharT *data,
                                    ClUint32T len)
                                    
{
    ClAmsMgmtUserDataSetRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity || !key);
    if(!data && len) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    request.entity = entity;
    request.key = key;
    request.data = data;
    request.len = len;
    return cl_ams_mgmt_user_data_setkey(&request);
}

ClRcT clAmsMgmtEntityUserDataGet(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT *entity,
                                 ClCharT **data,
                                 ClUint32T *len)
{
    ClAmsMgmtUserDataGetRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity || !len);
    request.entity = entity;
    request.key = NULL;
    request.data = data;
    request.len = len;
    return cl_ams_mgmt_user_data_get(&request);
}

ClRcT clAmsMgmtEntityUserDataGetKey(ClAmsMgmtHandleT handle,
                                    ClAmsEntityT *entity,
                                    ClNameT *key,
                                    ClCharT **data,
                                    ClUint32T *len)
{
    ClAmsMgmtUserDataGetRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity || !key || !len);
    request.entity = entity;
    request.key = key;
    request.data = data;
    request.len = len;
    return cl_ams_mgmt_user_data_getkey(&request);
}

ClRcT clAmsMgmtEntityUserDataDelete(ClAmsMgmtHandleT handle,
                                    ClAmsEntityT *entity)
{
    ClAmsMgmtUserDataDeleteRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity);
    request.entity = entity;
    request.key = NULL;
    request.clear = CL_FALSE;
    return cl_ams_mgmt_user_data_delete(&request);
}

ClRcT clAmsMgmtEntityUserDataDeleteKey(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *entity,
                                       ClNameT *key)
{
    ClAmsMgmtUserDataDeleteRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity || !key);
    request.entity = entity;
    request.key = key;
    request.clear = CL_FALSE;
    return cl_ams_mgmt_user_data_deletekey(&request);
}

ClRcT clAmsMgmtEntityUserDataDeleteAll(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *entity)
{
    ClAmsMgmtUserDataDeleteRequestT request = {0};
    AMS_CHECKPTR_SILENT(!entity);
    request.entity = entity;
    request.key = NULL;
    request.clear = CL_TRUE;
    return cl_ams_mgmt_user_data_deleteall(&request);
}

static ClRcT clAmsMgmtSwitchoverSU(ClAmsMgmtHandleT handle, ClAmsEntityT *su)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!su) goto out;
    rc = clAmsMgmtEntityLockAssignment(handle, su);
    if(rc != CL_OK)
    {
        clLogError("SU", "SWITCHOVER", "SU [%s] lock assignment returned [%#x]",
                   su->name.value, rc);
        goto out;
    }
    rc = clAmsMgmtEntityUnlock(handle, su);
    if(rc != CL_OK)
    {
        clLogError("SU", "SWITCHOVER", "SU [%s] unlock returned [%#x]", 
                   su->name.value, rc);
        goto out;
    }
    out:
    return rc;
}

static ClRcT clAmsMgmtSwitchoverActiveSU(ClAmsMgmtHandleT handle, ClAmsEntityT *su, ClAmsEntityT *activeSU)
{
    ClAmsSUConfigT *suConfig = NULL;
    ClAmsEntityBufferT suList = {0};
    ClAmsSGStatusT *sgStatus = NULL;
    ClAmsSUStatusT **suStatusList = NULL;
    ClRcT rc = CL_OK;
    ClInt32T i;

    suConfig = clAmsMgmtServiceUnitGetConfig(handle, (const ClCharT*)su->name.value);
    if(suConfig == NULL)
    {
        clLogError("SU", "ACT-SWITCHOVER", "SU [%s] config not found", 
                   su->name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    sgStatus = clAmsMgmtServiceGroupGetStatus(handle, (const ClCharT*)suConfig->parentSG.entity.name.value);
    if(sgStatus == NULL)
    {
        clLogError("SU", "ACT-SWITCHOVER", "SG [%s] status not found", suConfig->parentSG.entity.name.value);
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out_free;
    }

    rc = clAmsMgmtGetSGAssignedSUList(handle, &suConfig->parentSG.entity, &suList);
    if(rc != CL_OK)
    {
        clLogError("SU", "ACT-SWITCHOVER", "SG [%s] assigned su list returned [%#x]", 
                   suConfig->parentSG.entity.name.value, rc);
        goto out_free;
    }
    suStatusList = clHeapCalloc(suList.count, sizeof(*suStatusList));
    CL_ASSERT(suStatusList != NULL);
    /*
     * Lock out all the other standby's first except the target SU.
     */
    for(i = 0; i < suList.count; ++i)
    {
        if(!strcmp((const ClCharT*)suList.entity[i].name.value, (const ClCharT*)su->name.value))
        {
            suStatusList[i] = NULL;
            continue;
        }
        suStatusList[i] = clAmsMgmtServiceUnitGetStatus(handle, 
                                                        (const ClCharT*)
                                                        suList.entity[i].name.value);
        if(!suStatusList[i])
        {
            rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
            clLogError("SU", "ACT-SWITCHOVER", "SU [%s] status get failed", 
                       suList.entity[i].name.value);
            goto unlock_standby;
        }
    
        if(suStatusList[i]->numStandbySIs > 0)
        {
            rc = clAmsMgmtEntityLockAssignment(handle, &suList.entity[i]);
            if(rc != CL_OK)
            {
                clLogError("SU", "ACT-SWITCHOVER", "Lock assignment on SU [%s] returned [%#x]",
                           suList.entity[i].name.value, rc);
                clHeapFree(suStatusList[i]);
                suStatusList[i] = NULL;
            }
        }
    }

    /*
     * Now iterate through the active SUs and lock the required.
     */
    for(i = 0; i < suList.count; ++i)
    {
        if(suStatusList[i] && suStatusList[i]->numActiveSIs > 0)
        {
            ClBoolT canLock = CL_FALSE;
            if(!activeSU) canLock = CL_TRUE;
            else
                canLock = !strncmp(activeSU->name.value, suList.entity[i].name.value,
                                   strlen(activeSU->name.value));
            if(canLock)
                goto found;
        }
    }
    
    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
    clLogError("SU", "ACT-SWITCHOVER", "No active SU found for the SG [%s]", 
               suConfig->parentSG.entity.name.value);
    goto unlock_standby;

    found:
    clLogInfo("SU", "ACT-SWITCHOVER", "Switching over active SU [%s]",
              suList.entity[i].name.value);
    rc = clAmsMgmtSwitchoverSU(handle, &suList.entity[i]);

    unlock_standby:
    for(i = 0; i < suList.count; ++i)
    {
        if(suStatusList[i] && suStatusList[i]->numStandbySIs > 0)
        {
            ClRcT rc2 = clAmsMgmtEntityUnlock(handle, &suList.entity[i]);
            if(rc2 != CL_OK)
            {
                clLogError("SU", "ACT-SWITCHOVER", "SU [%s] unlock returned with [%#x]",
                           suList.entity[i].name.value, rc2);
            }
        }
    }

    out_free:
    if(suConfig) clHeapFree(suConfig);
    if(sgStatus) clHeapFree(sgStatus);
    for(i = 0; i < suList.count; ++i)
    {
        if(suStatusList[i])
            clHeapFree(suStatusList[i]);
    }
    if(suStatusList) clHeapFree(suStatusList);
    if(suList.entity) clHeapFree(suList.entity);
    
    return rc;
}

static ClRcT clAmsMgmtGetSISUHAState(ClAmsMgmtHandleT handle,
                                     ClAmsEntityT *siEntity,
                                     ClAmsEntityT *suEntity,
                                     ClAmsHAStateT *haState,
                                     ClBoolT *fullyAssigned)
{
    ClAmsSUSIExtendedRefBufferT suSIRefBuffer = {0};
    ClInt32T i;
    ClRcT rc = CL_OK;
    rc = clAmsMgmtGetSUAssignedSIsExtendedList(handle, suEntity, &suSIRefBuffer);
    if(rc != CL_OK)
    {
        clLogError("STATE", "GET", "Assigned SI list for SU [%s] returned with [%#x]",
                   suEntity->name.value, rc);
        goto out_free;
    }

    for(i = 0; i < suSIRefBuffer.count; ++i)
    {
        ClAmsSUSIExtendedRefT *suSIRef = suSIRefBuffer.entityRef + i;
        if(!strcmp((const ClCharT*)suSIRef->entityRef.entity.name.value,
                   (const ClCharT*)siEntity->name.value))
        {
            /*
             * Don't overwrite standby state. so a standby fully assigned can be used as a marker for
             * SI to be fully assigned irrespective of the SI SU status list.
             */
            if(*haState != CL_AMS_HA_STATE_STANDBY)
                *haState = suSIRef->haState;
            if(fullyAssigned)
            {
                if(suSIRef->haState == CL_AMS_HA_STATE_ACTIVE
                   &&
                   (
                    suSIRef->pendingInvocations > 0
                    ||
                    suSIRef->numActiveCSIs < suSIRef->numCSIs
                    )
                   )
                {
                    *fullyAssigned = CL_FALSE;
                }
                else if(suSIRef->haState == CL_AMS_HA_STATE_STANDBY
                        &&
                        (
                         suSIRef->pendingInvocations > 0
                         ||
                         suSIRef->numStandbyCSIs < suSIRef->numCSIs
                         )
                        )
                {
                    *fullyAssigned = CL_FALSE;
                }
            }
            goto out_free;
        }
    }
    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);

    out_free:
    if(suSIRefBuffer.entityRef)
        clHeapFree(suSIRefBuffer.entityRef);

    return rc;
}

static ClRcT clAmsMgmtGetSIHAStateHard(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *siEntity,
                                       ClAmsEntityT *suEntity,
                                       ClAmsHAStateT *haState,
                                       ClBoolT *fullyAssigned)
{
    ClInt32T i;
    ClAmsHAStateT currentHAState = CL_AMS_HA_STATE_NONE;
    ClAmsSISURefBufferT siSURefBuffer = {0};
    ClRcT rc = CL_OK;

    if(!siEntity || !haState || siEntity->type != CL_AMS_ENTITY_TYPE_SI) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(suEntity && suEntity->type != CL_AMS_ENTITY_TYPE_SU)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *haState = CL_AMS_HA_STATE_NONE;
    if(fullyAssigned)
        *fullyAssigned = CL_TRUE;

    if(!suEntity)
    {
        /*
         * Find the SU to which this SI is assigned.
         */
        rc = clAmsMgmtGetSISUList(handle, siEntity, &siSURefBuffer);
        if(rc != CL_OK)
        {
            clLogError("STATE", "GET", "SI SU list get for [%s] returned with [%#x]",
                       siEntity->name.value, rc);
            goto out_free;
        }
        if(!siSURefBuffer.count)
            goto out_free;
        for(i = 0; i < siSURefBuffer.count; ++i)
        {
            rc = clAmsMgmtGetSISUHAState(handle, siEntity, &siSURefBuffer.entityRef[i].entityRef.entity, 
                                         &currentHAState, fullyAssigned);
            if(rc != CL_OK)
            {
                clLogError("STATE", "GET", "SI SU ha state get for SI [%s], SU [%s] returned with [%#x]",
                           siEntity->name.value, siSURefBuffer.entityRef[i].entityRef.entity.name.value, rc);
                goto out_free;
            }
        }
    }
    else
    {
        rc = clAmsMgmtGetSISUHAState(handle, siEntity, suEntity, &currentHAState, fullyAssigned);
        if(rc != CL_OK)
        {
            clLogError("STATE", "GET", "SI SU ha state get for SI [%s], SU [%s] returned with [%#x]",
                       siEntity->name.value, suEntity->name.value, rc);
            goto out_free;
        }
    }

    *haState = currentHAState;

    out_free:
    if(siSURefBuffer.entityRef)
        clHeapFree(siSURefBuffer.entityRef);

    return rc;
}

static ClRcT clAmsMgmtGetSUHAStateHard(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *entity,
                                       ClBoolT checkAllSIs,
                                       ClAmsHAStateT *haState,
                                       ClBoolT *fullyAssigned)
{
    ClInt32T i;
    ClAmsHAStateT currentHAState = CL_AMS_HA_STATE_NONE;
    ClAmsSUSIExtendedRefBufferT suSIRefBuffer = {0};
    ClAmsSUConfigT *suConfig = NULL;
    ClAmsSGConfigT *sgConfig = NULL;
    ClUint32T activeSIsPerSU = 0;
    ClUint32T standbySIsPerSU = 0;
    ClRcT rc = CL_OK;

    if(!entity || !haState || entity->type != CL_AMS_ENTITY_TYPE_SU) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *haState = CL_AMS_HA_STATE_NONE;
    if(fullyAssigned)
        *fullyAssigned = CL_TRUE;

    /*
     * Not really required since most of the times its gonna be 1 SI per SU. Only for the rare
     * case, we check the status of all the SIs
     */
    if(checkAllSIs)
    {
        rc = clAmsMgmtEntityGetConfig(handle, entity, (ClAmsEntityConfigT**)&suConfig);
        if(rc != CL_OK)
        {
            clLogError("STATE", "GET", "SU config for [%s] failed with [%#x]",
                       entity->name.value, rc);
            goto out_free;
        }
        rc = clAmsMgmtEntityGetConfig(handle, &suConfig->parentSG.entity, (ClAmsEntityConfigT**)&sgConfig);
        if(rc != CL_OK)
        {
            clLogError("STATE", "GET", "SG config [%s] failed with [%#x]",
                       suConfig->parentSG.entity.name.value, rc);
            goto out_free;
        }
        activeSIsPerSU = sgConfig->maxActiveSIsPerSU;
        standbySIsPerSU = sgConfig->maxStandbySIsPerSU;
    }

    rc = clAmsMgmtGetSUAssignedSIsExtendedList(handle, entity, &suSIRefBuffer);
    if(rc != CL_OK) goto out_free;

    if(!suSIRefBuffer.count) goto out_free;

    for(i = 0; i < suSIRefBuffer.count; ++i)
    {
        ClAmsSUSIExtendedRefT *suSIRef = suSIRefBuffer.entityRef + i;
        currentHAState = suSIRef->haState;
        if(fullyAssigned)
        {
            if(suSIRef->haState == CL_AMS_HA_STATE_ACTIVE
               &&
               (
                suSIRef->pendingInvocations > 0
                ||
                suSIRef->numActiveCSIs < suSIRef->numCSIs
                )
               )
            {
                *fullyAssigned = CL_FALSE;
            }
            else if(suSIRef->haState == CL_AMS_HA_STATE_STANDBY
                    &&
                    (
                     suSIRef->pendingInvocations > 0
                     ||
                     suSIRef->numStandbyCSIs < suSIRef->numCSIs
                     )
                    )
            {
                *fullyAssigned = CL_FALSE;
            }
        }
    }

    if(checkAllSIs && fullyAssigned)
    {
        if(currentHAState == CL_AMS_HA_STATE_ACTIVE
           &&
           suSIRefBuffer.count < activeSIsPerSU)
        {
            *fullyAssigned = CL_FALSE;
        }
        if(currentHAState == CL_AMS_HA_STATE_STANDBY
           &&
           suSIRefBuffer.count < standbySIsPerSU)
        {
            *fullyAssigned = CL_FALSE;
        }
    }

    *haState = currentHAState;

    out_free:
    if(suConfig)
        clHeapFree(suConfig);
    if(sgConfig)
        clHeapFree(sgConfig);
    if(suSIRefBuffer.entityRef)
        clHeapFree(suSIRefBuffer.entityRef);

    return rc;
}

ClRcT clAmsMgmtGetSIHAState(ClAmsMgmtHandleT handle,
                            const ClCharT *si,
                            const ClCharT *su,
                            ClAmsHAStateT *haState,
                            ClBoolT *fullyAssigned)
{
    ClAmsEntityT siEntity = {0};
    ClAmsEntityT suEntity = {0};
    ClAmsEntityT *pSUEntity = &suEntity;
    if(!handle || !si || !haState)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    siEntity.type = CL_AMS_ENTITY_TYPE_SI;
    clNameSet(&siEntity.name, si);
    CL_AMS_NAME_LENGTH_CHECK(siEntity);
    if(su)
    {
        suEntity.type = CL_AMS_ENTITY_TYPE_SU;
        clNameSet(&suEntity.name, su);
        CL_AMS_NAME_LENGTH_CHECK(suEntity);
    }
    else
    {
        pSUEntity = NULL;
    }
    return clAmsMgmtGetSIHAStateHard(handle, &siEntity, pSUEntity, haState, fullyAssigned);
}

ClRcT clAmsMgmtGetSUHAState(ClAmsMgmtHandleT handle,
                            const ClCharT *su,
                            ClBoolT checkAllSIs,
                            ClAmsHAStateT *haState,
                            ClBoolT *fullyAssigned)
{
    ClAmsEntityT suEntity = {0};
    if(!handle || !su || !haState)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    suEntity.type = CL_AMS_ENTITY_TYPE_SU;
    clNameSet(&suEntity.name, su);
    CL_AMS_NAME_LENGTH_CHECK(suEntity);
    return clAmsMgmtGetSUHAStateHard(handle, &suEntity, checkAllSIs, haState, fullyAssigned);
}

#ifdef CL_AMS_TEST_STATE
ClRcT clAmsMgmtTestHAState(const ClCharT *filename)
{
    FILE *fptr;
    ClAmsMgmtHandleT handle;
    ClVersionT version = {'B', 1, 1};
    char buf[0xff+1];
    const char *delim = " \t";
    ClRcT rc = clAmsMgmtInitialize(&handle, NULL, &version);
    CL_ASSERT(rc == CL_OK);
    if(!filename)
        filename = "/tmp/test_ha_state";
    fptr = fopen(filename, "r");
    if(!fptr)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out_finalize;
    }
    while(fgets(buf, sizeof(buf), fptr))
    {
        ClBoolT fullyAssigned = CL_FALSE;
        ClAmsHAStateT haState = 0;
        int l = strlen(buf);
        ClAmsEntityTypeT type;
        char siname[0xff+1];
        char suname[0xff+1];
        char *s = buf;
        if(buf[l-1] == '\n') buf[l-1] = 0;
        l = strspn(buf, delim);
        s += l;
        if(!*s || *s == '#') continue;
        l = strcspn(s, delim);
        if(!l) continue;
        if(!strncasecmp(s, "si", 2)) type = CL_AMS_ENTITY_TYPE_SI;
        else if(!strncasecmp(s, "su", 2)) type = CL_AMS_ENTITY_TYPE_SU;
        else continue; /*skip the entry*/
        s += l;
        l = strspn(s, delim);
        if(!l) continue;
        s += l;
        if(!*s) continue;
        l = strcspn(s, delim);
        if(type == CL_AMS_ENTITY_TYPE_SI)
        {
            char *su = suname;
            suname[0] = 0;
            strncpy(siname, s, CL_MIN(l, sizeof(siname)-1));
            siname[CL_MIN(l, sizeof(siname)-1)] = 0;
            s += l;
            l = strspn(s, delim);
            s += l;
            if(!*s) su  = NULL;
            else
            {
                l = strcspn(s, delim);
                strncpy(suname, s, CL_MIN(l, sizeof(suname)-1));
                suname[CL_MIN(l, sizeof(suname)-1)] = 0;
                s += l;
            }
            clLogNotice("TEST", "STATE", "Getting HA state for SI [%s], SU [%s]",
                        siname, suname[0] ? suname : "All");
            rc = clAmsMgmtGetSIHAState(handle, siname, su, &haState, &fullyAssigned);
            if(rc != CL_OK)
            {
                clLogError("TEST", "STATE", "SI ha state get returned with [%#x]", rc);
            }
            else
            {
                clLogNotice("TEST", "STATE", "SI [%s] ha state is [%s] for SU [%s], fully assigned [%s]",
                            siname, haState == CL_AMS_HA_STATE_ACTIVE ? "active" :
                            (haState == CL_AMS_HA_STATE_STANDBY) ? "standby" : 
                            (haState == CL_AMS_HA_STATE_NONE) ? "none" : "unknown",
                            su ? su : "All", fullyAssigned ? "yes" : "no");
            }
        }
        else 
        {
            ClBoolT checkAllSIs = CL_FALSE;
            strncpy(suname, s, CL_MIN(l, sizeof(suname)-1));
            suname[CL_MIN(l, sizeof(suname)-1)] = 0;
            s += l;
            l = strspn(s, delim);
            s += l;
            if(*s) checkAllSIs = CL_TRUE;
            clLogNotice("TEST", "STATE", "Getting HA state for SU [%s], check all [%s]",
                        suname, checkAllSIs ? "Yes" : "No");
            rc = clAmsMgmtGetSUHAState(handle, suname, checkAllSIs, &haState, &fullyAssigned);
            if(rc != CL_OK)
            {
                clLogError("TEST", "STATE", "HA state get for SU [%s] returned with [%#x]",
                           suname, rc);
            }
            else
            {
                clLogNotice("TEST", "STATE", "HA state for SU [%s] is [%s], fully assigned [%s]",
                            suname, haState == CL_AMS_HA_STATE_ACTIVE ? "active" : 
                            (haState == CL_AMS_HA_STATE_STANDBY) ? "standby" : 
                            (haState == CL_AMS_HA_STATE_NONE) ? "none" : "unknown",
                            fullyAssigned ? "yes" : "no");
            }
        }
    }
    fclose(fptr);
    out_finalize:
    clAmsMgmtFinalize(handle);
    return CL_OK;
}
#endif

#ifdef CL_AMS_TEST_CAS

ClRcT clAmsMgmtTestCAS(const ClCharT *e, const ClCharT *type)
{
    ClAmsEntityT entity = {0};
    ClAmsAdminStateT cas = CL_AMS_ADMIN_STATE_NONE;
    ClRcT rc;
    if(!e || !type) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!strncasecmp(type, "sg", 2)) entity.type = CL_AMS_ENTITY_TYPE_SG;
    else if(!strncasecmp(type, "si", 2)) entity.type = CL_AMS_ENTITY_TYPE_SI;
    else if(!strncasecmp(type, "csi", 3)) entity.type = CL_AMS_ENTITY_TYPE_CSI;
    else if(!strncasecmp(type, "node", 4)) entity.type = CL_AMS_ENTITY_TYPE_NODE;
    else if(!strncasecmp(type, "su", 2)) entity.type = CL_AMS_ENTITY_TYPE_SU;
    else if(!strncasecmp(type, "comp", 4)) entity.type = CL_AMS_ENTITY_TYPE_COMP;
    else return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    clNameSet(&entity.name, e);
    rc = clAmsMgmtComputedAdminStateGet(0, &entity, &cas);
    if(rc != CL_OK) return rc;
    clLogNotice("CAS", "GET", "Computed admin state for entity [%s: %s] is [%s]",
                type, e, CL_AMS_STRING_A_STATE(cas));
    return rc;
}

#endif

static ClRcT clAmsMgmtGetSUHAStateSoft(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *entity,
                                       ClAmsHAStateT *haState)
{
    ClInt32T i;
    ClAmsHAStateT currentHAState = 0;
    ClAmsSUSIRefBufferT suSIRefBuffer = {0};
    ClRcT rc = CL_OK;

    if(!entity || !haState) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *haState = CL_AMS_HA_STATE_NONE;
    rc = clAmsMgmtGetSUAssignedSIsList(handle, entity, &suSIRefBuffer);
    if(rc != CL_OK) return rc;

    if(!suSIRefBuffer.count) return CL_OK;

    for(i = 0; i < suSIRefBuffer.count; ++i)
    {
        ClAmsSUSIRefT *entityRef = suSIRefBuffer.entityRef + i;
        if(entityRef->haState != CL_AMS_HA_STATE_ACTIVE
           && 
           entityRef->haState != CL_AMS_HA_STATE_STANDBY)
        {
            currentHAState = CL_AMS_HA_STATE_NONE;
            break;
        }
        currentHAState |= entityRef->haState;
    }

    *haState = currentHAState;
    clHeapFree(suSIRefBuffer.entityRef);
    return CL_OK;
}

static ClRcT clAmsMgmtSetActiveSU(ClAmsMgmtHandleT handle, ClAmsEntityT *su, ClAmsEntityT *activeSU)
{
    ClRcT rc = CL_OK;
    ClAmsHAStateT haState = CL_AMS_HA_STATE_NONE;

    if(!su->name.length) return rc;
    rc = clAmsMgmtGetSUHAStateSoft(handle, su, &haState);
    if(rc != CL_OK)
        goto out;

    if(haState == CL_AMS_HA_STATE_STANDBY)
    {
        rc = clAmsMgmtSwitchoverActiveSU(handle, su, activeSU);
    }

    out:
    return rc;
}

ClRcT clAmsMgmtSetActive(ClAmsMgmtHandleT handle, ClAmsEntityT *entity, ClAmsEntityT *activeSU)
{
    ClAmsEntityBufferT entityBuffer = {0};
    ClRcT rc = CL_OK;
    ClInt32T i;

    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(entity->type != CL_AMS_ENTITY_TYPE_NODE &&
       entity->type != CL_AMS_ENTITY_TYPE_SU)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(entity->type == CL_AMS_ENTITY_TYPE_NODE)
    {
        rc = clAmsMgmtGetNodeSUList(handle, entity, &entityBuffer);
        if(rc != CL_OK)
        {
            clLogError("SET", "ACTIVE", "Getting SU list for node [%s] returned [%#x]",
                       entity->name.value, rc);
            return rc;
        }
        for(i = 0; i < entityBuffer.count; ++i)
        {
            ClAmsHAStateT haState = 0;
            ClAmsEntityT *su = entityBuffer.entity + i;
            clAmsMgmtGetSUHAStateSoft(handle, su, &haState);
            if(haState != CL_AMS_HA_STATE_STANDBY)
            {
                su->name.length = 0;
            }
        }
    }
    else
    {
        entityBuffer.count = 1;
        entityBuffer.entity = entity;
    }
    
    for(i = 0; i < entityBuffer.count; ++i)
    {
        ClAmsEntityT *su = entityBuffer.entity+i;
        if(!su->name.length) continue;
        rc = clAmsMgmtSetActiveSU(handle, su, activeSU);
        goto out_free;
    }

    out_free:
    if(entityBuffer.entity != entity)
        clHeapFree(entityBuffer.entity);

    return rc;
}

ClRcT
clAmsMgmtSIAssignSU(const ClCharT *si, const ClCharT *activeSU, const ClCharT *standbySU)
{
    ClAmsMgmtSIAssignSUCustomRequestT req = {{0}};

    AMS_CHECKPTR_SILENT(!si);
    
    req.si.type = CL_AMS_ENTITY_TYPE_SI;
    clNameSet(&req.si.name, si);
    ++req.si.name.length;

    if(activeSU)
    {
        req.activeSU.type = CL_AMS_ENTITY_TYPE_SU;
        clNameSet(&req.activeSU.name, activeSU);
        ++req.activeSU.name.length;
    }

    if(standbySU)
    {
        req.standbySU.type = CL_AMS_ENTITY_TYPE_SU;
        clNameSet(&req.standbySU.name, standbySU);
        ++req.standbySU.name.length;
    }

    return cl_ams_mgmt_si_assign_su_custom(&req);
}

ClRcT
clAmsMgmtGetAspInstallInfo(ClAmsMgmtHandleT mgmtHandle, const ClCharT *nodeName, 
                           ClCharT *aspInstallInfo, ClUint32T len)
{
    ClRcT rc = CL_OK;
    ClCharT *installInfo = NULL;
    ClUint32T installInfoLen = 0;
    ClNameT installInfoKey = {0};
    ClAmsEntityT entity = {0};

    if(!aspInstallInfo || !len)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!nodeName) /* use local node */
    {
        rc = clCpmLocalNodeNameGet(&entity.name);
        if(rc != CL_OK)
            goto out;
    }
    else
        clNameSet(&entity.name, nodeName);

    ++entity.name.length;
    entity.type = CL_AMS_ENTITY_TYPE_NODE;
    clNameSet(&installInfoKey, ASP_INSTALL_KEY);

    rc = clAmsMgmtEntityUserDataGetKey(mgmtHandle, &entity, &installInfoKey, 
                                       &installInfo, &installInfoLen);
    if(rc != CL_OK)
        goto out;

    *aspInstallInfo = 0;
    strncat(aspInstallInfo, installInfo, CL_MIN(installInfoLen, len));
    clHeapFree(installInfo);
    
    out:
    return rc;
}

typedef struct ClAmsNodeCache
{
    struct hashStruct hash; /* hash index */
    ClListHeadT list; /* list index*/
    ClAmsNodeConfigT config;
    ClAmsNodeStatusT status;
    ClListHeadT suList;
}ClAmsNodeCacheT;

typedef struct ClAmsSUCache
{
    struct hashStruct hash;
    ClListHeadT list; 
    ClAmsSUConfigT config;
    ClAmsSUStatusT status;
    ClListHeadT compList;
    ClListHeadT nodeList; /*to the parent node*/
    ClListHeadT sgList; /* to the parent sg*/
    ClAmsSUSIExtendedRefBufferT siBuffer;
}ClAmsSUCacheT;

typedef struct ClAmsSGCache
{
    struct hashStruct hash;
    ClListHeadT list;
    ClAmsSGConfigT config;
    ClAmsSGStatusT status;
    ClListHeadT siList;
    ClListHeadT suList;
}ClAmsSGCacheT;

typedef struct ClAmsSICache
{
    struct hashStruct hash;
    ClListHeadT list;
    ClAmsSIConfigT config;
    ClAmsSIStatusT status;
    ClListHeadT csiList;
    ClListHeadT sgList; /* index into the parent sg */
    ClAmsSISUExtendedRefBufferT suBuffer; /* si su buffer list */
}ClAmsSICacheT;

typedef struct ClAmsCSICache
{
    struct hashStruct hash;
    ClListHeadT list;
    ClAmsCSIConfigT config;
    ClAmsCSIStatusT status;
    ClListHeadT siList; /* index into the parent si*/
}ClAmsCSICacheT;

typedef struct ClAmsCompCache
{
    struct hashStruct hash;
    ClListHeadT list;
    ClAmsCompConfigT config;
    ClAmsCompStatusT status;
    ClAmsCompCSIRefBufferT csiBuffer;
    ClListHeadT suList;
}ClAmsCompCacheT;

typedef struct ClAmsMgmtEntityCache
{
#define CL_AMS_MGMT_DB_CACHE_BITS (0x8)
#define CL_AMS_MGMT_DB_CACHE_SIZE (1 << CL_AMS_MGMT_DB_CACHE_BITS)
#define CL_AMS_MGMT_DB_CACHE_MASK (CL_AMS_MGMT_DB_CACHE_SIZE - 1)
    struct hashStruct *entityMap[CL_AMS_MGMT_DB_CACHE_SIZE];
}ClAmsMgmtEntityCacheT;

typedef struct ClAmsMgmtDBCache
{
    ClAmsMgmtEntityCacheT entityCache[CL_AMS_ENTITY_TYPE_MAX+1]; /* hash map */
    ClListHeadT entityList[CL_AMS_ENTITY_TYPE_MAX+1]; /*direct list*/
}ClAmsMgmtDBCacheT;

static void amsMgmtDBCacheInitialize(ClAmsMgmtDBCacheT *cache)
{
    ClUint32T i;
    for(i = 0; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        CL_LIST_HEAD_INIT(cache->entityList+i);
    }
}

static __inline__ ClUint32T amsMgmtDBEntityHash(ClAmsEntityT *entity)
{
    ClUint32T key = 0;
    clCksm32bitCompute( (ClUint8T*)entity->name.value, entity->name.length, &key);
    return key & CL_AMS_MGMT_DB_CACHE_MASK;
}

static ClRcT amsMgmtDBCompCacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsCompCacheT *comp = clHeapCalloc(1, sizeof(*comp));
        ClUint32T hash;
        ClAmsEntityListTypeT type = 0;
        CL_ASSERT(comp != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsCompConfigT, 4, 0, 0)(msg, &comp->config);
        if(rc != CL_OK)
        {
            clHeapFree(comp);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsCompStatusT, 5, 1, 0)(msg, &comp->status);
        if(rc != CL_OK)
        {
            clHeapFree(comp);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsEntityListTypeT, 4, 0, 0)(msg, &type);
        if(rc != CL_OK)
        {
            clHeapFree(comp);
            return rc;
        }
        if(type == CL_AMS_COMP_STATUS_CSI_LIST)
        {
            ClUint32T j;
            rc = clXdrUnmarshallClUint32T(msg, &comp->csiBuffer.count);
            if(rc != CL_OK)
            {
                clHeapFree(comp);
                return rc;
            }
            comp->csiBuffer.entityRef = clHeapCalloc(comp->csiBuffer.count,
                                                     sizeof(*comp->csiBuffer.entityRef));
            CL_ASSERT(comp->csiBuffer.entityRef != NULL);
            for(j = 0; j < comp->csiBuffer.count; ++j)
            {
                rc |= VDECL_VER(clXdrUnmarshallClAmsCompCSIRefT, 4, 0, 0)(msg, &comp->csiBuffer.entityRef[j]);
            }
            if(rc != CL_OK)
            {
                clHeapFree(comp->csiBuffer.entityRef);
                clHeapFree(comp);
                return rc;
            }
        }
        clListAddTail(&comp->list, &cache->entityList[CL_AMS_ENTITY_TYPE_COMP]);
        hash = amsMgmtDBEntityHash(&comp->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_COMP].entityMap, hash, &comp->hash);
    }
    return rc;
}

static ClRcT amsMgmtDBCSICacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsCSICacheT *csi = clHeapCalloc(1, sizeof(*csi));
        ClUint32T hash;
        CL_ASSERT(csi != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsCSIConfigT, 4, 0, 0)(msg, &csi->config);
        if(rc != CL_OK)
        {
            clHeapFree(csi);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsCSIStatusT, 4, 0, 0)(msg, &csi->status);
        if(rc != CL_OK)
        {
            clHeapFree(csi);
            return rc;
        }
        clListAddTail(&csi->list, &cache->entityList[CL_AMS_ENTITY_TYPE_CSI]);
        hash = amsMgmtDBEntityHash(&csi->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_CSI].entityMap, hash, &csi->hash);
    }
    return rc;
}

static ClRcT amsMgmtDBSICacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsSICacheT *si = clHeapCalloc(1, sizeof(*si));
        ClUint32T hash, j = 0;
        ClAmsEntityListTypeT listType = 0;
        CL_ASSERT(si != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsSIConfigT, 4, 0, 0)(msg, &si->config);
        if(rc != CL_OK)
        {
            clHeapFree(si);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsSIStatusT, 4, 0, 0)(msg, &si->status);
        if(rc != CL_OK)
        {
            clHeapFree(si);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsEntityListTypeT, 4, 0, 0)(msg, &listType);
        if(rc != CL_OK)
        {
            clHeapFree(si);
            return rc;
        }
        if(listType == CL_AMS_SI_STATUS_SU_EXTENDED_LIST)
        {
            rc = clXdrUnmarshallClUint32T(msg, &si->suBuffer.count);
            if(rc != CL_OK)
            {
                clHeapFree(si);
                return rc;
            }
            si->suBuffer.entityRef = clHeapCalloc(si->suBuffer.count, sizeof(*si->suBuffer.entityRef));
            CL_ASSERT(si->suBuffer.entityRef != NULL);
            for(j = 0; j < si->suBuffer.count; ++j)
            {
                rc |= VDECL_VER(clXdrUnmarshallClAmsSISUExtendedRefT, 4, 0, 0)(msg, 
                                                                               si->suBuffer.entityRef + j);
            }
            if(rc != CL_OK)
            {
                clHeapFree(si->suBuffer.entityRef);
                clHeapFree(si);
                return rc;
            }
        }
        CL_LIST_HEAD_INIT(&si->csiList);
        clListAddTail(&si->list, &cache->entityList[CL_AMS_ENTITY_TYPE_SI]);
        hash = amsMgmtDBEntityHash(&si->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_SI].entityMap, hash, &si->hash);
    }
    return rc;
}

static ClRcT amsMgmtDBSGCacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsSGCacheT *sg = clHeapCalloc(1, sizeof(*sg));
        ClUint32T hash;
        CL_ASSERT(sg != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 5, 0, 0)(msg, &sg->config);
        if(rc != CL_OK)
        {
            clHeapFree(sg);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsSGStatusT, 4, 1, 0)(msg, &sg->status);
        if(rc != CL_OK)
        {
            clHeapFree(sg);
            return rc;
        }
        CL_LIST_HEAD_INIT(&sg->siList);
        CL_LIST_HEAD_INIT(&sg->suList);
        clListAddTail(&sg->list, &cache->entityList[CL_AMS_ENTITY_TYPE_SG]);
        hash = amsMgmtDBEntityHash(&sg->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_SG].entityMap, hash, &sg->hash);
    }
    return rc;
}

static ClRcT amsMgmtDBSUCacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsSUCacheT *su = clHeapCalloc(1, sizeof(*su));
        ClUint32T hash, j;
        ClAmsEntityListTypeT listType = 0;
        CL_ASSERT(su != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsSUConfigT, 4, 0, 0)(msg, &su->config);
        if(rc != CL_OK)
        {
            clHeapFree(su);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsSUStatusT, 4, 0, 0)(msg, &su->status);
        if(rc != CL_OK)
        {
            clHeapFree(su);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsEntityListTypeT, 4, 0, 0)(msg, &listType);
        if(rc != CL_OK)
        {
            clHeapFree(su);
            return rc;
        }
        if(listType == CL_AMS_SU_STATUS_SI_EXTENDED_LIST)
        {
            rc = clXdrUnmarshallClUint32T(msg, &su->siBuffer.count);
            if(rc != CL_OK)
            {
                clHeapFree(su);
                return rc;
            }
            su->siBuffer.entityRef = clHeapCalloc(su->siBuffer.count, sizeof(*su->siBuffer.entityRef));
            CL_ASSERT(su->siBuffer.entityRef != NULL);
            for(j = 0; j < su->siBuffer.count; ++j)
            {
                rc |= VDECL_VER(clXdrUnmarshallClAmsSUSIExtendedRefT, 4, 0, 0)(msg, 
                                                                               su->siBuffer.entityRef + j);
            }
            if(rc != CL_OK)
            {
                clHeapFree(su->siBuffer.entityRef);
                clHeapFree(su);
                return rc;
            }
        }
        CL_LIST_HEAD_INIT(&su->compList);
        clListAddTail(&su->list, &cache->entityList[CL_AMS_ENTITY_TYPE_SU]);
        hash = amsMgmtDBEntityHash(&su->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_SU].entityMap, hash, &su->hash);
    }
    return rc;
}

static ClRcT amsMgmtDBNodeCacheLoad(ClAmsEntityBufferT *buffer, ClAmsMgmtDBCacheT *cache, ClBufferHandleT msg)
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    for(i = 0; i < buffer->count; ++i)
    {
        ClAmsNodeCacheT *node = clHeapCalloc(1, sizeof(*node));
        ClUint32T hash;
        CL_ASSERT(node != NULL);
        rc = VDECL_VER(clXdrUnmarshallClAmsNodeConfigT, 4, 0, 0)(msg, &node->config);
        if(rc != CL_OK)
        {
            clHeapFree(node);
            return rc;
        }
        rc = VDECL_VER(clXdrUnmarshallClAmsNodeStatusT, 4, 0, 0)(msg, &node->status);
        if(rc != CL_OK)
        {
            clHeapFree(node);
            return rc;
        }
        CL_LIST_HEAD_INIT(&node->suList);
        clListAddTail(&node->list, &cache->entityList[CL_AMS_ENTITY_TYPE_NODE]);
        hash = amsMgmtDBEntityHash(&node->config.entity);
        hashAdd(cache->entityCache[CL_AMS_ENTITY_TYPE_NODE].entityMap, hash, &node->hash);
    }
    return rc;
}

#define amsMgmtDBCacheFind(search, cast, ent, cache) do {               \
    struct hashStruct *__iter;                                          \
    struct hashStruct **__table = (cache)->entityCache[(ent)->type].entityMap; \
    cast *__index = NULL;                                               \
    ClUint32T __hash = amsMgmtDBEntityHash((ent));                      \
    search = NULL;                                                      \
    for(__iter = __table[__hash]; __iter; __iter = __iter->pNext)       \
    {                                                                   \
        __index = hashEntry(__iter, cast, hash);                        \
        if(!strncmp(__index->config.entity.name.value, (ent)->name.value, (ent)->name.length)) \
        {                                                               \
            search = __index;                                           \
            break;                                                      \
        }                                                               \
    }                                                                   \
}while(0)

/*
 * Set the relation for the various entities.
 */
static ClRcT amsMgmtDBCacheRelationSet(ClAmsMgmtDBCacheT *cache)
{
    ClListHeadT *iter;
    /*
     * Set NODE su list and SG su list as well
     */
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_SU])
    {
        ClAmsSUCacheT *su = CL_LIST_ENTRY(iter, ClAmsSUCacheT, list);
        ClAmsNodeCacheT *node = NULL;
        ClAmsSGCacheT *sg = NULL;
        amsMgmtDBCacheFind(node, ClAmsNodeCacheT, &su->config.parentNode.entity, cache);
        if(!node)
        {
            clLogError("DB", "SET", "Node [%s] not found in the amf db cache. Cache looks stale",
                       su->config.parentNode.entity.name.value);
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }
        clListAddTail(&su->nodeList, &node->suList);
        amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &su->config.parentSG.entity, cache);
        if(!sg)
        {
            clLogError("DB", "SET", "SG [%s] not found in the amf db cache. Cache looks stale",
                       su->config.parentSG.entity.name.value);
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }
        clListAddTail(&su->sgList, &sg->suList);
    }
    /*
     * Set SG si list.
     */
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_SI])
    {
        ClAmsSICacheT *si = CL_LIST_ENTRY(iter, ClAmsSICacheT, list);
        ClAmsSGCacheT *sg = NULL;
        amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &si->config.parentSG.entity, cache);
        if(!sg)
        {
            clLogError("DB", "SET", "SG [%s] not found in the amf db cache. Cache looks stale",
                       si->config.parentSG.entity.name.value);
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }
        clListAddTail(&si->sgList, &sg->siList);
    }
    /*
     * Set SI csi list.
     */
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_CSI])
    {
        ClAmsCSICacheT *csi = CL_LIST_ENTRY(iter, ClAmsCSICacheT, list);
        ClAmsSICacheT *si = NULL;
        amsMgmtDBCacheFind(si, ClAmsSICacheT, &csi->config.parentSI.entity, cache);
        if(!si)
        {
            clLogError("DB", "SET", "SI [%s] not found in the amf db cache. Cache looks stale",
                       csi->config.parentSI.entity.name.value);
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }
        clListAddTail(&csi->siList, &si->csiList);
    }
    /*
     * Set SU comp list.
     */
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_COMP])
    {
        ClAmsCompCacheT *comp = CL_LIST_ENTRY(iter, ClAmsCompCacheT, list);
        ClAmsSUCacheT *su = NULL;
        amsMgmtDBCacheFind(su, ClAmsSUCacheT, &comp->config.parentSU.entity, cache);
        if(!su)
        {
            clLogError("DB", "SET", "SU [%s] not found in the amf db cache. Cache looks stale",
                       comp->config.parentSU.entity.name.value);
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }
        clListAddTail(&comp->suList, &su->compList);
    }
    return CL_OK;
}

static ClRcT amsMgmtDBCacheLoad(ClUint8T *db, ClUint32T len, ClAmsMgmtDBCacheT *cache)
{
    ClRcT rc;
    ClBufferHandleT msg = 0;
    ClUint32T lastOffset = 0;
    ClUint32T dbSize = len;
    ClAmsEntityBufferT buffer = {0};

    amsMgmtDBCacheInitialize(cache);
    rc = clBufferCreate(&msg);
    if(rc != CL_OK)
        goto out;
    /*
     * FIXME: Could stitch this directly to the heap
     */
    rc = clBufferNBytesWrite(msg, db, len);
    if(rc != CL_OK)
        goto out_free;
    while(len > 0)
    {
        ClAmsEntityListTypeT type = 0;
        ClUint32T curOffset = 0;
        if(buffer.entity)
        {
            clHeapFree(buffer.entity);
            buffer.entity = NULL;
        }
        buffer.count = 0;
        rc = VDECL_VER(clXdrUnmarshallClAmsEntityListTypeT, 4, 0, 0)(msg, &type);
        if(rc != CL_OK)
            goto out_free;
        rc = VDECL_VER(clXdrUnmarshallClAmsEntityBufferT, 4, 0, 0)(msg, &buffer);
        if(rc != CL_OK)
            goto out_free;
        switch(type)
        {
        case CL_AMS_NODE_LIST:
            {
                rc = amsMgmtDBNodeCacheLoad(&buffer, cache, msg);
            }
            break;
        case CL_AMS_SU_LIST:
            {
                rc = amsMgmtDBSUCacheLoad(&buffer, cache, msg);
            }
            break;
        case CL_AMS_SG_LIST:
            {
                rc = amsMgmtDBSGCacheLoad(&buffer, cache, msg);
            }
            break;
        case CL_AMS_SI_LIST:
            {
                rc = amsMgmtDBSICacheLoad(&buffer, cache, msg);
            }
            break;
        case CL_AMS_CSI_LIST:
            {
                rc = amsMgmtDBCSICacheLoad(&buffer, cache, msg);
            }
            break;
        case CL_AMS_COMP_LIST:
            {
                rc = amsMgmtDBCompCacheLoad(&buffer, cache, msg);
            }
            break;
        default:
            break;
        }
        rc = clBufferReadOffsetGet(msg, &curOffset);
        if(rc != CL_OK)
            goto out_free;
        if(curOffset == lastOffset)
        {
            rc = CL_AMS_RC(CL_ERR_LIBRARY);
            clLogError("DB", "GET", "AMF db cache load not reading/progressing successfully");
            goto out_free;
        }
        lastOffset = curOffset;
        len = dbSize - curOffset;
    }

    amsMgmtDBCacheRelationSet(cache);

    out_free:
    clBufferDelete(&msg);
    if(buffer.entity)
        clHeapFree(buffer.entity);
    out:
    return rc;
}

#define amsMgmtDBListFree(cast, listHead, destroy_cb) do {  \
    while(!CL_LIST_HEAD_EMPTY((listHead)))                  \
    {                                                       \
        ClListHeadT *top = (listHead)->pNext;               \
        cast *__entry = CL_LIST_ENTRY(top, cast, list);     \
        clListDel(&__entry->list);                          \
        destroy_cb(&__entry->hash);                         \
    }                                                       \
}while(0)

static void amsMgmtEntityCacheFree(struct hashStruct *hash)
{
    hashDel(hash);
    clHeapFree((ClPtrT)hash);
}

static void amsMgmtCompCacheFree(struct hashStruct *hash)
{
    ClAmsCompCacheT *comp = (ClAmsCompCacheT*)hash;
    clAmsMgmtFreeCompCSIRefBuffer(&comp->csiBuffer);
    if(comp->config.pSupportedCSITypes)
    {
        clHeapFree(comp->config.pSupportedCSITypes);
        comp->config.pSupportedCSITypes = NULL;
    }
    amsMgmtEntityCacheFree(hash);
}

static void amsMgmtSICacheFree(struct hashStruct *hash)
{
    ClAmsSICacheT *si = (ClAmsSICacheT*)hash;
    if(si->suBuffer.entityRef)
    {
        clHeapFree(si->suBuffer.entityRef);
        si->suBuffer.entityRef = NULL;
    }
    amsMgmtEntityCacheFree(hash);
}

static void amsMgmtSUCacheFree(struct hashStruct *hash)
{
    ClAmsSUCacheT *su = (ClAmsSUCacheT*)hash;
    if(su->siBuffer.entityRef)
    {
        clHeapFree(su->siBuffer.entityRef);
        su->siBuffer.entityRef = NULL;
    }
    amsMgmtEntityCacheFree(hash);
}

static ClRcT amsMgmtDBCacheFree(ClAmsMgmtDBCacheT *cache)
{
    amsMgmtDBListFree(ClAmsCompCacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_COMP], amsMgmtCompCacheFree);
    amsMgmtDBListFree(ClAmsCSICacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_CSI], amsMgmtEntityCacheFree);
    amsMgmtDBListFree(ClAmsSICacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_SI], amsMgmtSICacheFree);
    amsMgmtDBListFree(ClAmsSUCacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_SU], amsMgmtSUCacheFree);
    amsMgmtDBListFree(ClAmsSGCacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_SG], amsMgmtEntityCacheFree);
    amsMgmtDBListFree(ClAmsNodeCacheT, &cache->entityList[CL_AMS_ENTITY_TYPE_NODE], amsMgmtEntityCacheFree);
    return CL_OK;
}

ClRcT clAmsMgmtDBCacheDump(ClAmsMgmtDBHandleT db)
{
    ClListHeadT *iter;
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!db) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_NODE])
    {
        ClAmsNodeCacheT *node = CL_LIST_ENTRY(iter, ClAmsNodeCacheT, list);
        ClListHeadT *child;
        clLogNotice("DB", "DUMP", "Node [%s]", node->config.entity.name.value);
        CL_LIST_FOR_EACH(child, &node->suList)
        {
            ClAmsSUCacheT *su = CL_LIST_ENTRY(child, ClAmsSUCacheT, nodeList);
            clLogNotice("DB", "DUMP", "Node has SU [%s]", su->config.entity.name.value);
        }
    }
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_SG])
    {
        ClAmsSGCacheT *sg = CL_LIST_ENTRY(iter, ClAmsSGCacheT, list);
        ClListHeadT *child;
        clLogNotice("DB", "DUMP", "SG [%s]", sg->config.entity.name.value);
        CL_LIST_FOR_EACH(child, &sg->siList)
        {
            ClAmsSICacheT *si = CL_LIST_ENTRY(child, ClAmsSICacheT, sgList);
            clLogNotice("DB", "DUMP", "SG has SI [%s]", si->config.entity.name.value);
        }
        CL_LIST_FOR_EACH(child, &sg->suList)
        {
            ClAmsSUCacheT *su = CL_LIST_ENTRY(child, ClAmsSUCacheT, sgList);
            clLogNotice("DB", "DUMP", "SG has SU [%s]", su->config.entity.name.value);
        }
    }
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_SI])
    {
        ClAmsSICacheT *si = CL_LIST_ENTRY(iter, ClAmsSICacheT, list);
        ClListHeadT *child;
        ClUint32T i;
        clLogNotice("DB", "DUMP", "SI [%s]", si->config.entity.name.value);
        CL_LIST_FOR_EACH(child, &si->csiList)
        {
            ClAmsCSICacheT *csi = CL_LIST_ENTRY(child, ClAmsCSICacheT, siList);
            clLogNotice("DB", "DUMP", "SI has CSI [%s]", csi->config.entity.name.value);
        }
        for(i = 0; i < si->suBuffer.count; ++i)
        {
            ClAmsSISUExtendedRefT *siSURef = si->suBuffer.entityRef + i;
            clLogNotice("DB", "DUMP", "SI has SU [%s] assigned with ha state [%s], invocations [%d]",
                        siSURef->entityRef.entity.name.value, 
                        CL_AMS_STRING_H_STATE(siSURef->haState), siSURef->pendingInvocations);
        }
    }
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_CSI])
    {
        ClAmsCSICacheT *csi = CL_LIST_ENTRY(iter, ClAmsCSICacheT, list);
        clLogNotice("DB", "DUMP", "CSI [%s]", csi->config.entity.name.value);
    }
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_SU])
    {
        ClAmsSUCacheT *su = CL_LIST_ENTRY(iter, ClAmsSUCacheT, list);
        ClListHeadT *child;
        ClUint32T i;
        clLogNotice("DB", "DUMP", "SU [%s]", su->config.entity.name.value);
        CL_LIST_FOR_EACH(child, &su->compList)
        {
            ClAmsCompCacheT *comp = CL_LIST_ENTRY(child, ClAmsCompCacheT, suList);
            clLogNotice("DB", "DUMP", "SU has COMP [%s]", comp->config.entity.name.value);
        }
        for(i = 0; i < su->siBuffer.count; ++i)
        {
            ClAmsSUSIExtendedRefT *suSIRef = su->siBuffer.entityRef + i;
            clLogNotice("DB", "DUMP", "SU has SI [%s] assigned with ha state [%s], active csis [%d], "
                        "standby csis [%d], quiesced csis [%d], quiescing csis [%d], numcsis [%d], "
                        "pending invocations [%d]",
                        suSIRef->entityRef.entity.name.value, 
                        CL_AMS_STRING_H_STATE(suSIRef->haState), suSIRef->numActiveCSIs,
                        suSIRef->numStandbyCSIs, suSIRef->numQuiescedCSIs, suSIRef->numQuiescingCSIs,
                        suSIRef->numCSIs, suSIRef->pendingInvocations);
        }
    }
    CL_LIST_FOR_EACH(iter, &cache->entityList[CL_AMS_ENTITY_TYPE_COMP])
    {
        ClAmsCompCacheT *comp = CL_LIST_ENTRY(iter, ClAmsCompCacheT, list);
        ClUint32T i;
        clLogNotice("DB", "DUMP", "COMP [%s]", comp->config.entity.name.value);
        for(i = 0; i < comp->csiBuffer.count; ++i)
        {
            ClAmsCompCSIRefT *csiRef = comp->csiBuffer.entityRef + i;
            clLogNotice("DB", "DUMP", "COMP has CSI reference [%s] with ha state [%s]",
                        csiRef->entityRef.entity.name.value, CL_AMS_STRING_H_STATE(csiRef->haState));
        }
    }
    return CL_OK;
}


#define amsMgmtDBCacheGetList(list, cast, field, buffer) do {           \
    ClListHeadT *__iter;                                                \
    (buffer)->count = 0;                                                \
    (buffer)->entity = NULL;                                            \
    CL_LIST_FOR_EACH(__iter, (list))                                    \
    {                                                                   \
        cast *__entry = CL_LIST_ENTRY(__iter, cast, field);             \
        if(!( (buffer)->count & 15) )                                   \
        {                                                               \
            (buffer)->entity = clHeapRealloc((buffer)->entity, sizeof(*(buffer)->entity)*((buffer)->count+16)); \
            CL_ASSERT((buffer)->entity != NULL);                        \
            memset((buffer)->entity + (buffer)->count, 0, sizeof(*(buffer)->entity) * 16); \
        }                                                               \
        memcpy((buffer)->entity+(buffer)->count, &__entry->config.entity, sizeof(*(buffer)->entity)); \
        ++(buffer)->count;                                              \
    }                                                                   \
}while(0)

ClRcT clAmsMgmtDBGetNodeList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_NODE], ClAmsNodeCacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSUList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_SU], ClAmsSUCacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSGList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_SG], ClAmsSGCacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSIList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_SI], ClAmsSICacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetCSIList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_CSI], ClAmsCSICacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetCompList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    if(!cache || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheGetList(&cache->entityList[CL_AMS_ENTITY_TYPE_COMP], ClAmsCompCacheT, list, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetEntityConfig(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityConfigT **entityConfig)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsEntityConfigT *config = NULL;
    ClAmsEntityT searchEntity;
    ClUint32T sz = 0;
    ClRcT rc = CL_AMS_RC(CL_ERR_NOT_EXIST);

    if(!cache || !entity || !entityConfig) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity); /* patch the name length if required */

    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeCacheT *node = NULL;
            sz = sizeof(ClAmsNodeConfigT);
            amsMgmtDBCacheFind(node, ClAmsNodeCacheT, &searchEntity, cache);
            if(!node)
            {
                goto out;
            }
            config = &node->config.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUCacheT *su = NULL;
            sz = sizeof(ClAmsSUConfigT);
            amsMgmtDBCacheFind(su, ClAmsSUCacheT, &searchEntity, cache);
            if(!su)
            {
                goto out;
            }
            config = &su->config.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGCacheT *sg = NULL;
            sz = sizeof(ClAmsSGConfigT);
            amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &searchEntity, cache);
            if(!sg)
            {
                goto out;
            }
            config = &sg->config.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSICacheT *si = NULL;
            sz = sizeof(ClAmsSIConfigT);
            amsMgmtDBCacheFind(si, ClAmsSICacheT, &searchEntity, cache);
            if(!si)
            {
                goto out;
            }
            config = &si->config.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSICacheT *csi = NULL;
            sz = sizeof(ClAmsCSIConfigT);
            amsMgmtDBCacheFind(csi, ClAmsCSICacheT, &searchEntity, cache);
            if(!csi)
            {
                goto out;
            }
            config = &csi->config.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompCacheT *comp = NULL;
            sz = sizeof(ClAmsCompConfigT);
            amsMgmtDBCacheFind(comp, ClAmsCompCacheT, &searchEntity, cache);
            if(!comp)
            {
                goto out;
            }
            config = &comp->config.entity;
        }
        break;
    default:
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    *entityConfig = clHeapCalloc(1, sz);
    CL_ASSERT(*entityConfig != NULL);
    memcpy(*entityConfig, config, sz);
    
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clAmsMgmtDBGetEntityStatus(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityStatusT **entityStatus)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsEntityStatusT *status = NULL;
    ClAmsEntityT searchEntity;
    ClUint32T sz = 0;
    ClRcT rc = CL_AMS_RC(CL_ERR_NOT_EXIST);

    if(!cache || !entity || !entityStatus) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity); /* patch the name length if required */

    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeCacheT *node = NULL;
            sz = sizeof(ClAmsNodeStatusT);
            amsMgmtDBCacheFind(node, ClAmsNodeCacheT, &searchEntity, cache);
            if(!node)
            {
                goto out;
            }
            status = &node->status.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUCacheT *su = NULL;
            sz = sizeof(ClAmsSUStatusT);
            amsMgmtDBCacheFind(su, ClAmsSUCacheT, &searchEntity, cache);
            if(!su)
            {
                goto out;
            }
            status = &su->status.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGCacheT *sg = NULL;
            sz = sizeof(ClAmsSGStatusT);
            amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &searchEntity, cache);
            if(!sg)
            {
                goto out;
            }
            status = &sg->status.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSICacheT *si = NULL;
            sz = sizeof(ClAmsSIStatusT);
            amsMgmtDBCacheFind(si, ClAmsSICacheT, &searchEntity, cache);
            if(!si)
            {
                goto out;
            }
            status = &si->status.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSICacheT *csi = NULL;
            sz = sizeof(ClAmsCSIStatusT);
            amsMgmtDBCacheFind(csi, ClAmsCSICacheT, &searchEntity, cache);
            if(!csi)
            {
                goto out;
            }
            status = &csi->status.entity;
        }
        break;
    case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompCacheT *comp = NULL;
            sz = sizeof(ClAmsCompStatusT);
            amsMgmtDBCacheFind(comp, ClAmsCompCacheT, &searchEntity, cache);
            if(!comp)
            {
                goto out;
            }
            status = &comp->status.entity;
        }
        break;
    default:
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    *entityStatus = clHeapCalloc(1, sz);
    CL_ASSERT(*entityStatus != NULL);
    memcpy(*entityStatus, status, sz);
    
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clAmsMgmtDBGetNodeSUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsNodeCacheT *node = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(node, ClAmsNodeCacheT, &searchEntity, cache);
    if(!node)
    {
        clLogError("DB", "GET", "Node [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    amsMgmtDBCacheGetList(&node->suList, ClAmsSUCacheT, nodeList, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSGSUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSGCacheT *sg = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &searchEntity, cache);
    if(!sg)
    {
        clLogError("DB", "GET", "SG [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    amsMgmtDBCacheGetList(&sg->suList, ClAmsSUCacheT, sgList, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSGSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSGCacheT *sg = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(sg, ClAmsSGCacheT, &searchEntity, cache);
    if(!sg)
    {
        clLogError("DB", "GET", "SG [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    amsMgmtDBCacheGetList(&sg->siList, ClAmsSICacheT, sgList, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSICSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSICacheT *si = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(si, ClAmsSICacheT, &searchEntity, cache);
    if(!si)
    {
        clLogError("DB", "GET", "SI [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    amsMgmtDBCacheGetList(&si->csiList, ClAmsCSICacheT, siList, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSUCompList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsEntityBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSUCacheT *su = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(su, ClAmsSUCacheT, &searchEntity, cache);
    if(!su)
    {
        clLogError("DB", "GET", "SU [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    amsMgmtDBCacheGetList(&su->compList, ClAmsCompCacheT, suList, buffer);
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSUAssignedSIsList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsSUSIExtendedRefBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSUCacheT *su = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(su, ClAmsSUCacheT, &searchEntity, cache);
    if(!su)
    {
        clLogError("DB", "GET", "SU [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    buffer->count = su->siBuffer.count;
    buffer->entityRef = NULL;
    if(buffer->count > 0)
    {
        buffer->entityRef = clHeapCalloc(su->siBuffer.count, sizeof(*su->siBuffer.entityRef));
        CL_ASSERT(buffer->entityRef != NULL);
        memcpy(buffer->entityRef, su->siBuffer.entityRef, sizeof(*buffer->entityRef) * buffer->count);
    }
    return CL_OK;
}

ClRcT clAmsMgmtDBGetSISUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsSISUExtendedRefBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsSICacheT *si = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(si, ClAmsSICacheT, &searchEntity, cache);
    if(!si)
    {
        clLogError("DB", "GET", "SI [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    buffer->count = si->suBuffer.count;
    buffer->entityRef = NULL;
    if(buffer->count > 0)
    {
        buffer->entityRef = clHeapCalloc(si->suBuffer.count, sizeof(*si->suBuffer.entityRef));
        CL_ASSERT(buffer->entityRef != NULL);
        memcpy(buffer->entityRef, si->suBuffer.entityRef, sizeof(*buffer->entityRef) * buffer->count);
    }
    return CL_OK;
}

ClRcT clAmsMgmtDBGetCompCSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity, ClAmsCompCSIRefBufferT *buffer)
{
    ClAmsMgmtDBCacheT *cache = (ClAmsMgmtDBCacheT*)db;
    ClAmsCompCacheT *comp = NULL;
    ClAmsEntityT searchEntity;
    if(!cache || !entity || !buffer)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&searchEntity, entity, sizeof(searchEntity));
    CL_AMS_NAME_LENGTH_CHECK(searchEntity);
    amsMgmtDBCacheFind(comp, ClAmsCompCacheT, &searchEntity, cache);
    if(!comp)
    {
        clLogError("DB", "GET", "COMP [%s] not found in the amf db cache", searchEntity.name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    buffer->count = comp->csiBuffer.count;
    buffer->entityRef = clHeapCalloc(comp->csiBuffer.count, sizeof(*comp->csiBuffer.entityRef));
    CL_ASSERT(buffer->entityRef != NULL);
    memcpy(buffer->entityRef, comp->csiBuffer.entityRef, sizeof(*buffer->entityRef) * buffer->count);
    for(ClUint32T i = 0; i < buffer->count; ++i)
    {
        if(comp->csiBuffer.entityRef[i].activeComp)
        {
            buffer->entityRef[i].activeComp = clHeapCalloc(1, sizeof(*buffer->entityRef[i].activeComp));
            CL_ASSERT(buffer->entityRef[i].activeComp != NULL);
            memcpy(buffer->entityRef[i].activeComp, comp->csiBuffer.entityRef[i].activeComp,
                   sizeof(*buffer->entityRef[i].activeComp));
        }
    }
    return CL_OK;
}

ClRcT clAmsMgmtDBGet(ClAmsMgmtDBHandleT *db)
{
    ClRcT rc;
    ClAmsMgmtDBGetResponseT dbResponse = {0};
    ClAmsMgmtDBCacheT *cache;
    rc = cl_ams_mgmt_db_get(&dbResponse);
    if(rc != CL_OK)
    {
        clLogError("DB", "GET", "AMF db get returned with [%#x]", rc);
        goto out;
    }
    cache = clHeapCalloc(1, sizeof(*cache));
    CL_ASSERT(cache != NULL);
    rc = amsMgmtDBCacheLoad(dbResponse.buffer, dbResponse.len, cache);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    if(!db)
    {
        clAmsMgmtDBCacheDump((ClAmsMgmtDBHandleT)cache);
        goto out_free;
    }
    *db = (ClAmsMgmtDBHandleT)cache;
    cache = NULL;

    out_free:
    if(cache)
    {
        amsMgmtDBCacheFree(cache);
        clHeapFree(cache);
    }
    if(dbResponse.buffer)
        clHeapFree(dbResponse.buffer);
    out:
    return rc;
}

ClRcT clAmsMgmtDBFinalize(ClAmsMgmtDBHandleT *db)
{
    ClAmsMgmtDBCacheT *cache = NULL;
    if(!db || !(cache = (ClAmsMgmtDBCacheT*)*db))
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    amsMgmtDBCacheFree(cache);
    clHeapFree(cache);
    *db = 0;
    return CL_OK;
}

ClRcT clAmsMgmtComputedAdminStateGet(ClAmsMgmtHandleT handle,
                                     ClAmsEntityT *entity,
                                     ClAmsAdminStateT *adminState)
{
    ClRcT rc = CL_OK;
    ClAmsMgmtCASGetRequestT cas;
    if(!entity || !adminState) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER); 

    memset(&cas, 0, sizeof(cas));
    memcpy(&cas.entity, entity, sizeof(cas.entity));
    cas.computedAdminState = CL_AMS_ADMIN_STATE_NONE;
    CL_AMS_NAME_LENGTH_CHECK(cas.entity);

    rc = cl_ams_mgmt_cas_get(&cas);
    if(rc != CL_OK)
        return rc;

    *adminState = cas.computedAdminState;
    return rc;
}
