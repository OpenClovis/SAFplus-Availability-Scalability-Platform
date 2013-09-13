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
 * ModuleName  : name
 * File        : clNameClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Name Service Client Side functionality.
 *****************************************************************************/

#define __CLIENT__
#include "stdio.h"
#include "string.h"
#include "clCommon.h"
#include "clCommonErrors.h"
#include "clNameApi.h"
#include <clEoApi.h>
#include "clRmdApi.h"
#include "clDebugApi.h"
#include "clNameCommon.h"
#include "clNameErrors.h"
#include "clLogApi.h"
#include "clNameLog.h"
#include <clIdlApi.h>
#include <clXdrApi.h>
#include "xdrClNameSvcAttrEntryWithSizeIDLT.h"
#include "xdrClNameSvcInfoIDLT.h"
#include "xdrClNameVersionT.h"

#undef __SERVER__
#include <clNameServerFuncTable.h>
 
extern ClRcT clIocLocalAddressGet();
/* Name Server Lib Name. Needed for log */
#define CL_NAME_SERVER_LIB     "name_server"                                                                                                                            
extern ClCharT  *clNameLogMsg[];
static ClUint32T sNSLibInitDone = 0;
/*Supporetd version*/
static ClVersionT clVersionSupported[]={
{'B',0x01 , 0x01}
};
/*Version Database*/
static ClVersionDatabaseT versionDatabase={
sizeof(clVersionSupported)/sizeof(ClVersionT),
clVersionSupported
};



/*
 *  NAME: clNameContextCreate
 *
 *  This API is for creating a user defined context (both global scope and
 *  node local scope)
 *
 *  @param   contextType [I/P] - whether user defined global or node local
 *           contextMapCookie [I/P] - This will be used during lookup. There is
 *              one-to-one mapping between contextMapCookie and contextId that
 *              is returned by this API. CL_NS_DEFT_GLOBAL_MAP_COOKIE(0xFFFFFFF) 
 *              and CL_NS_DEFT_LOCAL_MAP_COOKIE(0xFFFFFFE) are RESERVED and should 
 *              not be used.
 *           contextId [O/P]   - Id of the context created
 *
 *  @returns CL_OK
 *           CL_NS_ERR_LIMIT_EXCEEDED
 *           CL_ERR_NO_MEMORY
 *           CL_ERR_INVALID_PARAMETER
 *           CL_ERR_NULL_POINTER
 *           CL_NS_ERR_CONTEXT_ALREADY_CREATED
 *           CL_ERR_NOT_INITIALIZED         
 */
                                                                                                                             
ClRcT clNameContextCreate(ClNameSvcContextT contextType,
                          ClUint32T contextMapCookie,
                          ClUint32T *pContextId)
{
    ClNameSvcInfoIDLT      nsInfo;
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClRcT                  rc      = CL_OK;
    ClIocNodeAddressT      sAddr   = 0;
    ClNameVersionT         version = {0};


    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameContextCreate \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if(pContextId == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if((contextType != CL_NS_USER_GLOBAL) && 
       (contextType != CL_NS_USER_NODELOCAL))
    {
        rc = CL_NS_RC(CL_ERR_INVALID_PARAMETER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Invalid contextType specified \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if((contextMapCookie == CL_NS_DEFT_GLOBAL_MAP_COOKIE) ||
       (contextMapCookie == CL_NS_DEFT_LOCAL_MAP_COOKIE))
    {
        rc = CL_NS_RC(CL_ERR_INVALID_PARAMETER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Invalid cookie specified \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    sAddr = clIocLocalAddressGet();

    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);
    memset(&nsInfo, 0, sizeof(ClNameSvcInfoIDLT));
    nsInfo.version          = CL_NS_VERSION_NO;
    nsInfo.contextType      = contextType;
    nsInfo.contextMapCookie = contextMapCookie;
    nsInfo.source           = CL_NS_CLIENT;

    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHandle,0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)&nsInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_CONTEXT_CREATE, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clNameContextCreate  Failed \n rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }
    else 
    {
          rc = clXdrUnmarshallClUint32T(outMsgHandle, (void *)pContextId);
    }
    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}


/**
 *  NAME: clNameRegister
 *
 *  This API is for registering a name to obj reference mapping with NS.
 *  In clovis HA scenario, whenever a component is assigned ACTIVE HA state
 *  for a given service it registers the same with NS.
 *  For registering to user defined context, first the context has to be
 *  created using clNameContextCreate API.
 *
 *  @param   contextId[I/P] - Context in which the service will be provided.
 *              For registering to default global context,
 *                         contextId=CL_NS_DEFT_GLOBAL_CONTEXT
 *              For registering to default node local context.
 *                         contextId=CL_NS_DEFT_NODELOCAL_CONTEXT
 *              For user defined contexts, contextId=Id returned by
 *                         clNameContextCreate API
 *           pNSRegisInfo[I/P] - registration related information
 *
 *  @returns CL_OK
 *           CL_ERR_NULL_POINTER
 *           CL_NS_ERR_CONTEXT_NOT_CREATED
 *           CL_NS_ERR_LIMIT_EXCEEDED
 *           CL_NS_ERR_DUPLICATE_ENTRY
 *           CL_NS_ERR_ATTRIBUTE_MISMATCH
 *           CL_ERR_NO_MEMORY
 *           CL_ERR_NOT_INITIALIZED         
 */
                                                                                                                             
ClRcT clNameRegister(ClUint32T contextId, ClNameSvcRegisterT* pNSRegisInfo,
                     ClUint64T *pObjReference)
{
    ClRcT                  rc      = CL_OK;
    ClNameSvcInfoIDLT*     pNSInfo = NULL;
    ClBufferHandleT        inMsgHandle;
    ClBufferHandleT        outMsgHandle;
    ClUint32T              size    = sizeof(ClNameSvcInfoIDLT);
    ClIocNodeAddressT      sAddr   = 0;
    ClNameVersionT         version = {0};
    ClEoExecutionObjT      *eoObj = NULL;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameRegister \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if((pNSRegisInfo == NULL) || (pObjReference == NULL))
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }
    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);

    pNSInfo = (ClNameSvcInfoIDLT*) clHeapAllocate(size);

    if(pNSInfo == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, rc);
        clBufferDelete(&inMsgHandle);
        CL_FUNC_EXIT();
        return rc;
    }


    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(pNSInfo, 0, sizeof(ClNameSvcInfoIDLT));
    pNSInfo->version      = CL_NS_VERSION_NO;
    pNSInfo->source       = CL_NS_CLIENT;
    pNSInfo->objReference = *pObjReference;
    /* Copy the name, make it null terminated also */
    saNameCopy(&pNSInfo->name, &pNSRegisInfo->name);
    pNSInfo->compId       = pNSRegisInfo->compId;
    pNSInfo->priority     = pNSRegisInfo->priority;
    pNSInfo->contextId    = contextId;
    pNSInfo->attrCount    = pNSRegisInfo->attrCount;
                                                                                                                           
    sAddr = clIocLocalAddressGet();

    /* Get Eo object to retrieve eoID and eoPort */
    rc = clEoMyEoObjectGet(&eoObj);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clEoMyEoObjectGet  Failed \n rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                "clEoMyEoObjectGet failed with rc 0x%x", rc);
        return CL_NS_RC(rc);
    }

    pNSInfo->eoID          = eoObj->eoID;
    pNSInfo->nodeAddress   = sAddr;
    pNSInfo->clientIocPort = eoObj->eoPort;

    if(pNSRegisInfo->attrCount>0)
    {
        pNSInfo->attrLen = (pNSRegisInfo->attrCount)*
                               sizeof(ClNameSvcAttrEntryIDLT); 
        pNSInfo->attr = (ClNameSvcAttrEntryIDLT *) 
               clHeapAllocate(pNSInfo->attrLen);
        memcpy(pNSInfo->attr, pNSRegisInfo->attr,
               pNSInfo->attrLen);
        size = size + pNSInfo->attrLen;
    }

 
    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle,0);
    clXdrMarshallClUint32T((void *)&size, inMsgHandle, 0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)pNSInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_REGISTER, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clNameRegister  Failed \n rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }
    else 
    {
        clXdrUnmarshallClUint64T(outMsgHandle, (void*)pObjReference);
    }
    clHeapFree(pNSInfo->attr); 
    clHeapFree(pNSInfo);
    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}

/**
 *  NAME: clNameComponentDeregister
 *
 *  This API is for deregistering entries of all the services being provided
 *  by a given component. In clovis HA scenario this should be called by CPM
 *  whenever it detects that a component has died. This can also be called
 *  by a component when it does down gracefully.
 *
 *  @param   compId[I/P] - component id of the component going down
 *
 *  @returns CL_OK
 *           CL_ERR_NOT_INITIALIZED         
 */
                                                                                                                             
ClRcT clNameComponentDeregister(ClUint32T compId)
{
    ClRcT                  rc      = CL_OK;
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClNameSvcInfoIDLT      nsInfo;
    ClIocNodeAddressT      sAddr   = 0;
    ClNameVersionT         version = {0};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameComponentDeregister \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);

    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(&nsInfo, 0, sizeof(ClNameSvcInfoIDLT));
    nsInfo.version      = CL_NS_VERSION_NO;
    nsInfo.compId       = compId;
    nsInfo.source       = CL_NS_CLIENT;
    sAddr = clIocLocalAddressGet();

    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle,0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)&nsInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_COMPONENT_DEREGISTER, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clNameComponentDeregister Failed rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }

    clBufferDelete(&outMsgHandle);
    clBufferDelete(&inMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}


/**
 *  NAME: clNameServiceDeregister
 *
 *  This API is for deregistering a service  being provided  by a given
 *  component.
 *
 *  @param   compId[I/P] - component id of the component hosting the service
 *           serviceName[I/P] - name of the service
 *           contextId[I/P] - context in which the service is being provided
 *              For default global context,
 *                        contextId=CL_NS_DEFT_GLOBAL_CONTEXT
 *              For default node local context.
 *                        contextId=CL_NS_DEFT_NODELOCAL_CONTEXT
 *              For user defined contexts, contextId=Id used in
 *                         clNameRegister API
 *
 *  @returns CL_OK
 *           CL_ERR_NULL_POINTER
 *           CL_NS_ERR_CONTEXT_NOT_CREATED
 *           CL_NS_ERR_SERVICE_NOT_REGISTERED
 *           CL_ERR_NOT_INITIALIZED         
 */
                                                                                                                             
ClRcT clNameServiceDeregister(ClUint32T contextId, ClUint32T compId,
                              SaNameT* serviceName)
{
    ClRcT                  rc      = CL_OK;
    ClNameSvcInfoIDLT      nsInfo;
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClIocNodeAddressT      sAddr   = 0;
    ClNameVersionT         version = {0};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameServiceDeregister \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if(serviceName == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);

    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(&nsInfo, 0, sizeof(ClNameSvcInfoIDLT));
    nsInfo.version      = CL_NS_VERSION_NO;
    nsInfo.compId       = compId;
    nsInfo.contextId    = contextId;
    /* copy the name & put the null terminating character */
    saNameCopy(&nsInfo.name, serviceName);
    nsInfo.source       = CL_NS_CLIENT;
    sAddr = clIocLocalAddressGet();

    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle, 0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)&nsInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_SERVICE_DEREGISTER, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nclNameServiceDeregister Failed rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }
    clBufferDelete(&outMsgHandle);
    clBufferDelete(&inMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}


/**
 *  NAME: clNameContextDelete
 *
 *  This API is for deleting a user defined context (both global scope and
 *  node local scope)
 *
 *  @param   contextId[I/P] - context to be deleted
 *
 *  @returns CL_OK
 *           CL_NS_ERR_CONTEXT_NOT_CREATED
 *           CL_NS_ERR_OPERATION_NOT_PERMITTED 
 *           CL_ERR_NOT_INITIALIZED         
 */
                                                                                                                             
ClRcT clNameContextDelete(ClUint32T contextId)
{
    ClRcT                  rc      = CL_OK;
    ClNameSvcInfoIDLT      nsInfo;
    ClIocNodeAddressT      sAddr   = 0;
    ClNameVersionT         version = {0};

    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameContextDelete \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);

    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(&nsInfo, 0, sizeof(ClNameSvcInfoIDLT));
    nsInfo.version      = CL_NS_VERSION_NO;
    nsInfo.contextId    = contextId;
    nsInfo.source       = CL_NS_CLIENT;
    sAddr = clIocLocalAddressGet();

    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle, 0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)&nsInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_CONTEXT_DELETE, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clNameContextDelete  Failed \n rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }

    clBufferDelete(&outMsgHandle);
    clBufferDelete(&inMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}
                                                                                                                             

/**
 *  NAME: saNameToObjectReferenceGet
 *
 *  This API is for getting/querying the object reference given the
 *  service name
 *
 *  @param  name[I/P]       - name of the service
 *          attrCount[I/P]  - No. of attributes being passed in the query.
 *            attrCount = CL_NS_DEFT_ATTR_LIST can be used if no. of attributes
 *            is unknown. In that case, object reference of component with 
 *            highest priority will be returned.
 *          pAttr[I/P]     -  List of attributes. 
 *            If attrCount = CL_NS_DEFT_ATTR_LIST, pAttr should be NULL.
 *          contextMapCookie[I/P]   - Used to find the context to look into.
 *            There is one-to-one mapping between contextMapCookie & context
 *            For default global context cookie = CL_NS_DEFT_GLOBAL_MAP_COOKIE
 *            For default local  context cookie = CL_NS_DEFT_LOCAL_MAP_COOKIE 
 *          objReference[O/P] - object reference associated with the service
 *
 *  @returns CL_OK
 *           CL_NS_ERR_CONTEXT_NOT_CREATED
 *           CL_NS_ERR_ENTRY_NOT_FOUND
 *           CL_ERR_NULL_POINTER
 *           CL_ERR_NOT_INITIALIZED         
 */

ClRcT saNameToObjectReferenceGet(SaNameT*            pName,
                                 ClUint32T           attrCount,
                                 ClNameSvcAttrEntryT *pAttr,
                                 ClUint32T           contextMapCookie,
                                 ClUint64T*          pObjReference)
{
    ClRcT                  rc          = CL_OK;
    ClUint32T              size        = sizeof(ClNameSvcInfoIDLT);
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClNameSvcInfoIDLT      *pNSInfo    = NULL;
    ClUint32T              tempCnt     = 0;
    ClIocNodeAddressT      sAddr       = 0;
    ClNameVersionT         version     = {0};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside saNameToObjectReferenceGet \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if((pName == NULL) || (pObjReference == NULL) || 
       ((attrCount>0) && (attrCount != CL_NS_DEFT_ATTR_LIST) && 
       (pAttr == NULL)))
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if(attrCount == CL_NS_DEFT_ATTR_LIST)
    {
        tempCnt   = 1;
        attrCount = 0;
    }
    
    pNSInfo = (ClNameSvcInfoIDLT*) clHeapAllocate(size);
    if(pNSInfo == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }


    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(pNSInfo, 0, sizeof(ClNameSvcInfoIDLT));
    pNSInfo->version          = CL_NS_VERSION_NO;
    pNSInfo->source           = CL_NS_CLIENT;
    /* copy the name & put NULL terminating character */
    saNameCopy(&pNSInfo->name, pName);
    pNSInfo->contextMapCookie = contextMapCookie;
    pNSInfo->op               = CL_NS_QUERY_OBJREF;
                                                                                                                           
    if(attrCount>0)
    {
        pNSInfo->attrLen = attrCount*sizeof(ClNameSvcAttrEntryIDLT); 
        pNSInfo->attr = (ClNameSvcAttrEntryIDLT *) 
               clHeapAllocate(pNSInfo->attrLen);
        memcpy(pNSInfo->attr, pAttr, pNSInfo->attrLen);
        size = size + pNSInfo->attrLen;
    }

    if(tempCnt == 1)
    {         
        pNSInfo->attrCount = CL_NS_DEFT_ATTR_LIST;
        attrCount          = CL_NS_DEFT_ATTR_LIST;
    }    
    else
    {     
        pNSInfo->attrCount = attrCount;
    }    


    sAddr = clIocLocalAddressGet();
    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);

    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle, 0);
    clXdrMarshallClUint32T((void *)&size, inMsgHandle, 0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)pNSInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_QUERY, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n saNameToObjectReferenceGet Failed," \
                             " rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }
    else 
    {
        clXdrUnmarshallClUint64T(outMsgHandle, (void*)pObjReference);
    }
    if( (attrCount > 0) && (CL_NS_DEFT_ATTR_LIST != attrCount))
    {    
        clHeapFree(pNSInfo->attr);
    }   
    clHeapFree(pNSInfo);
    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}

/**
 *  NAME: saNameToObjectMappingGet
 *
 *  This API is for getting the entire entry given the service name
 *
 *  @param  name[I/P]       - name of the service
 *          attrCount[I/P]  - No. of attributes being passed in the query.
 *            attrCount = CL_NS_DEFT_ATTR_LIST can be used if no. of attributes
 *            is unknown. In that case, object reference of component with 
 *            highest priority will be returned.
 *          pAttr[I/P]     -  List of attributes. 
 *            If attrCount = CL_NS_DEFT_ATTR_LIST, pAttr should be NULL.
 *          contextMapCookie[I/P]   - Used to find the context to look into.
 *            There is one-to-one mapping between contextMapCookie & context
 *            For default global context cookie = CL_NS_DEFT_GLOBAL_MAP_COOKIE
 *            For default local  context cookie = CL_NS_DEFT_LOCAL_MAP_COOKIE 
 *          pOutBuff[O/P] - will contain the entry
 *
 *  @returns CL_OK
 *           CL_NS_ERR_CONTEXT_NOT_CREATED
 *           CL_NS_ERR_ENTRY_NOT_FOUND
 *           CL_ERR_NULL_POINTER
 *           CL_ERR_NOT_INITIALIZED         
 */

ClRcT saNameToObjectMappingGet(SaNameT* pName,
                               ClUint32T attrCount,
                               ClNameSvcAttrEntryT *pAttr,
                               ClUint32T contextMapCookie,
                               ClNameSvcEntryPtrT* pOutBuff)
{
    ClRcT                  rc          = CL_OK;
    ClIocNodeAddressT      sAddr       = 0;
    ClUint32T              size        = sizeof(ClNameSvcInfoT);
    ClUint32T              listSize    = sizeof(ClNameSvcCompListT);
    ClUint32T              totalSize   = 0;
    ClUint32T              refCount    = 0;
    ClUint32T              index       = 0;
    ClNameSvcCompListT*    pList       = NULL;
    ClNameSvcInfoIDLT*     pNSInfo     = NULL;
    ClUint32T              tempCnt     = 0;
    ClNameVersionT         version     = {0};
    ClBufferHandleT inMsgHandle;
    ClBufferHandleT outMsgHandle;
    ClNameSvcAttrEntryWithSizeIDLT attrList;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside saNameToObjectMappingGet \n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if((pName == NULL) || (pOutBuff == NULL) ||
       ((attrCount>0) && (attrCount != CL_NS_DEFT_ATTR_LIST) && 
       (pAttr == NULL)))
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if(attrCount == CL_NS_DEFT_ATTR_LIST)
    {
        tempCnt   = 1;
        attrCount = 0;
    }
    
    pNSInfo = (ClNameSvcInfoIDLT*) clHeapAllocate(size);

    if(pNSInfo == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }


    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    version.reserved     = 0x00;

    memset(pNSInfo, 0, sizeof(ClNameSvcInfoT));
    pNSInfo->version          = CL_NS_VERSION_NO;
    pNSInfo->source           = CL_NS_CLIENT;
    /* copy the name & put NULL terminating character */
    saNameCopy(&pNSInfo->name, pName);
    pNSInfo->contextMapCookie = contextMapCookie;
    pNSInfo->op               = CL_NS_QUERY_MAPPING;
                                                                                                                           
    if(attrCount>0)
    {
        pNSInfo->attrLen = attrCount*sizeof(ClNameSvcAttrEntryIDLT); 
        pNSInfo->attr = (ClNameSvcAttrEntryIDLT *) 
               clHeapAllocate(pNSInfo->attrLen);
        memcpy(pNSInfo->attr, pAttr, pNSInfo->attrLen);
        size = size + pNSInfo->attrLen;
    }

    if(tempCnt == 1)
    {    
        pNSInfo->attrCount = CL_NS_DEFT_ATTR_LIST;
        attrCount          = CL_NS_DEFT_ATTR_LIST;
    }    
    else
    {    
        pNSInfo->attrCount = attrCount;
    }    

    sAddr = clIocLocalAddressGet();
    rc = clBufferCreate (&inMsgHandle);
    rc = clBufferCreate (&outMsgHandle);
    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHandle, 0);
    clXdrMarshallClUint32T((void *)&size, inMsgHandle, 0);
    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)pNSInfo, inMsgHandle, 0);
    CL_NS_CALL_RMD(sAddr, CL_NS_QUERY, inMsgHandle, outMsgHandle, rc);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n saNameToObjectMappingGet  Failed,"\
                             " rc =%x",rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            /* FUTURE */
        }
    }
    else 
    {
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&size);
        *pOutBuff = (ClNameSvcEntryPtrT) clHeapAllocate(size);
        clXdrUnmarshallSaNameT(outMsgHandle, (void*)&((*pOutBuff)->name));
        clXdrUnmarshallClUint64T(outMsgHandle, (void*)&((*pOutBuff)->objReference));
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&((*pOutBuff)->refCount));
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&((*pOutBuff)->compId.compId));
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&((*pOutBuff)->compId.priority));
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&((*pOutBuff)->attrCount));
        VDECL_VER(clXdrUnmarshallClNameSvcAttrEntryWithSizeIDLT, 4, 0, 0)(outMsgHandle, (void*)&attrList);
        if(attrList.attrLen > 0)
        {    
            memcpy(((*pOutBuff)->attr), attrList.attr, attrList.attrLen); 
            clHeapFree(attrList.attr);
        }    
        refCount  = (*pOutBuff)->refCount;
            
        totalSize = size; 

        index = index + totalSize;
        (*pOutBuff)->compId.pNext = NULL;
        while(refCount > 0)
        {
            pList = (ClNameSvcCompListT*) clHeapAllocate(listSize);
            pList->pNext = (*pOutBuff)->compId.pNext;                
            (*pOutBuff)->compId.pNext = pList;
            clXdrUnmarshallClUint32T(outMsgHandle, (void*)&(pList->compId));
            refCount --;
            index = index + sizeof(ClUint32T);
            clXdrUnmarshallClUint32T(outMsgHandle, (void*)&(pList->priority));
            index = index + sizeof(ClUint32T);
        }            
    }
    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);
    if((attrCount > 0) && (CL_NS_DEFT_ATTR_LIST != attrCount))
    {    
        clHeapFree(pNSInfo->attr);
    } 
    clHeapFree(pNSInfo);
    CL_FUNC_EXIT();
    return CL_NS_RC(rc);
}


/**
 *  NAME: clNameObjectMappingCleanup
 *
 *  This API is for cleaning up object mapping that was returned during
 *  lookup.
 *
 *  @param   pObjMapping - Pointer to the obj maping to be deleted
 *
 *  @returns CL_OK
 *           CL_ERR_NULL_POINTER
 */
                                                                                                                             
ClRcT clNameObjectMappingCleanup(ClNameSvcEntryPtrT pObjMapping)
{
    ClUint32T           refCount = 0;
    ClRcT               rc       = CL_OK;
    ClNameSvcCompListT *pTemp    = NULL;
    ClNameSvcCompListT *pTemp1   = NULL;

   
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside clNameObjectMappingCleanup\n"));

    if(sNSLibInitDone == 0)
    {
        rc = CL_NS_RC(CL_ERR_NOT_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib not initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_QUERY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    if(pObjMapping == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_ENTRY_CLEANUP_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    refCount  = pObjMapping->refCount;
    pTemp1    = &pObjMapping->compId;

    while(pObjMapping->refCount >= 1)
    {
        pObjMapping->refCount--;
        pTemp1->compId = pTemp1->pNext->compId;
        pTemp = pTemp1->pNext;
        pTemp1->pNext = pTemp->pNext;
        clHeapFree(pTemp);
    }

   clHeapFree(pObjMapping);

   CL_FUNC_EXIT();
   return CL_OK;
}
    

/**
 *  NAME: clNameLibVersionVerify 
 *
 *  This API is for verifying the version with the name service client
 *  
 *  @param   pVersion: Version of NS Library with the user. If version mismatch
 *              happens then this will carry out the version supported by the
 *              NS Library.
 *
 *  @returns CL_OK
 *           CL_ERR_NULL_POINTER         
 *           CL_ERR_INITIALIZED         
 */

ClRcT clNameLibVersionVerify(ClVersionT *pVersion)
{
    ClRcT rc = CL_OK;
    if (NULL == pVersion)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NS: NULL passed for Version\n"));
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
        return rc;
    }

    rc = clVersionVerify(&versionDatabase, pVersion);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NS: Version Mismatch\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
    }

    return CL_NS_RC(rc); 
}
       

/**
 *  NAME: clNameLibInitialize 
 *
 *  This API is for initializing the name service client
 *  
 *  @param   none
 *
 *  @returns CL_OK
 *           CL_ERR_NULL_POINTER         
 *           CL_ERR_INITIALIZED         
 */

ClRcT clNameLibInitialize()
{
    ClRcT rc = CL_OK;

    if(sNSLibInitDone == 0)
    {
        rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, NAM), CL_IOC_NAME_PORT);
        if(CL_OK != rc)
        {
            clLogError("NAM", "INT", "Name EO client table register failed with [%#x]", rc);
            return rc;
        }

        sNSLibInitDone++;
    }
    else
    {
        rc = CL_NS_RC(CL_ERR_INITIALIZED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Lib already initialized \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_NAME_SERVER_LIB,
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
    }
    return rc; 
}
 

/**
 *  NAME: clNameLibFinalize 
 *
 *  This API is for finalizing the name service client
 *  
 *  @param   none
 *
 *  @returns CL_OK
 */

ClRcT clNameLibFinalize(void)
{
    if(sNSLibInitDone != 0)
    {
        sNSLibInitDone = 0;
        clEoClientTableDeregister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, NAM), CL_IOC_NAME_PORT);
    }

    return CL_OK;
}

