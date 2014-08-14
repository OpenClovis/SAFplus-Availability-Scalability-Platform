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
 * File        : clAmsMgmtClientCCB.c
 ******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the client side implementation of the AMS dynamic ha
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
#include <clDebugApi.h>
#include <clAmsErrors.h>
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsMgmtClientRmd.h>
#include <clAmsXdrHeaderFiles.h>
#include <clAmsUtils.h>
#include <clNodeCache.h>

/******************************************************************************
 * Data structures
 *****************************************************************************/

/*
 * _clAmsMgmtCCBInitialize
 * -------------------
 *
 * Creates and Initializes context for AML configuration APIs.
 *
 */

typedef struct ClAmsMgmtCCBBatch
{
    ClUint32T items;
    ClUint32T itemsOffset;
    ClBufferHandleT buffer;
    ClUint32T version;
} ClAmsMgmtCCBBatchT;

static ClRcT amsMgmtCCBBatchBufferInitialize(ClAmsMgmtCCBBatchT *batch)
{
    ClRcT rc = CL_OK;

    if(batch->buffer)
    {
        rc |= clBufferDelete(&batch->buffer);
        if(rc != CL_OK)
            goto out;
        batch->buffer = 0;
    }
    rc |= clBufferCreate(&batch->buffer);
    if(rc != CL_OK)
        goto out;
    batch->items = 0;
    batch->version = CL_VERSION_CURRENT;
    clNodeCacheMinVersionGet(NULL, &batch->version);
    rc = clXdrMarshallClUint32T(&batch->version, batch->buffer, 0);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    rc = clBufferWriteOffsetGet(batch->buffer, &batch->itemsOffset);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    rc = clXdrMarshallClUint32T(&batch->items, batch->buffer, 0);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    goto out;

    out_free:
    clBufferDelete(&batch->buffer);
    batch->buffer = 0;
 
    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchInitialize(ClAmsMgmtHandleT mgmtHandle, ClAmsMgmtCCBBatchHandleT *batchHandle)
{
    ClAmsMgmtCCBBatchT *batch = NULL;
    ClRcT rc = CL_OK;

    batch = clHeapCalloc(1, sizeof(*batch));
    CL_ASSERT(batch != NULL);

    rc = amsMgmtCCBBatchBufferInitialize(batch);
    if(rc != CL_OK)
        goto out_free;

    *batchHandle = (ClAmsMgmtCCBBatchHandleT)batch;
    goto out;

    out_free:
    if(batch->buffer)
        clBufferDelete(&batch->buffer);
    clHeapFree(batch);

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchFinalize(ClAmsMgmtCCBBatchHandleT *batchHandle)
{
    ClAmsMgmtCCBBatchT *batch = NULL;
    if(!batchHandle || !(batch = (ClAmsMgmtCCBBatchT*)*batchHandle))
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(batch->buffer)
        clBufferDelete(&batch->buffer);
    clHeapFree(batch);
    *batchHandle = (ClAmsMgmtCCBBatchHandleT)0;
    return CL_OK;
}

ClRcT clAmsMgmtCCBBatchEntityCreate(ClAmsMgmtCCBBatchHandleT batchHandle,
                                    const ClAmsEntityT *entity)
{
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBEntityCreateRequestT createRequest = {0};
    ClInt32T op = (ClInt32T)CL_AMS_MGMT_CCB_OPERATION_CREATE;
    ClRcT rc = CL_OK;
    if(!batch || !entity)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy(&createRequest.entity, entity, sizeof(createRequest.entity));
    CL_AMS_NAME_LENGTH_CHECK(createRequest.entity);

    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBEntityCreateRequestT, 4, 0, 0)(&createRequest, 
                                                                            batch->buffer, 0);
    if(rc != CL_OK)
    {
        goto out;
    }
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchEntityDelete(ClAmsMgmtCCBBatchHandleT batchHandle,
                                    const ClAmsEntityT *entity)
{
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBEntityDeleteRequestT deleteRequest = {0};
    ClInt32T op = (ClInt32T)CL_AMS_MGMT_CCB_OPERATION_DELETE;
    ClRcT rc = CL_OK;

    if(!batch || !entity)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy(&deleteRequest.entity, entity, sizeof(deleteRequest.entity));
    CL_AMS_NAME_LENGTH_CHECK(deleteRequest.entity);

    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBEntityDeleteRequestT, 4, 0, 0)(&deleteRequest,
                                                                            batch->buffer, 0);
    if(rc != CL_OK)
    {
        goto out;
    }
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchEntitySetConfig(ClAmsMgmtCCBBatchHandleT batchHandle,
                                       ClAmsEntityConfigT *entityConfig,
                                       ClUint64T bitmask)
{
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBEntitySetConfigRequestT req = {0};
    ClInt32T op = (ClInt32T)CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG;
    ClRcT rc = CL_OK;

    if(!batch || !entityConfig)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    req.bitmask = bitmask;
    req.entityConfig = entityConfig;
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= __marshalClAmsMgmtCCBEntitySetConfig(&req, batch->buffer, batch->version);
    if(rc != CL_OK)
    {
        goto out;
    }
    ++batch->items;

    out:
    return rc;
}

static ClRcT amsMgmtCCBBatchCSISetNVP(ClAmsMgmtCCBBatchHandleT batchHandle,
                                      CL_IN   ClAmsEntityT    *csiName,
                                      CL_IN   ClAmsCSINVPT    *nvp,
                                      CL_IN ClAmsMgmtCCBOperationsT opId)

{
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBCSISetNVPRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !csiName || !nvp)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.csiName, csiName, sizeof(req.csiName));
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    memcpy (&req.nvp, nvp, sizeof(req.nvp));
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBCSISetNVPRequestT, 4, 0, 0)(&req,
                                                                         batch->buffer, 0);
    if(rc != CL_OK)
    {
        goto out;
    }
    ++batch->items;
    
    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchCSISetNVP(ClAmsMgmtCCBBatchHandleT batchHandle,
                                 CL_IN   ClAmsEntityT    *csiName,
                                 CL_IN   ClAmsCSINVPT    *nvp )

{
    return amsMgmtCCBBatchCSISetNVP(batchHandle, csiName, nvp,
                                    CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP);
}

ClRcT clAmsMgmtCCBBatchCSIDeleteNVP(
                                    CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                    CL_IN   ClAmsEntityT    *csiName,
                                    CL_IN   ClAmsCSINVPT    *nvp )
{
    return amsMgmtCCBBatchCSISetNVP(batchHandle, csiName, nvp,
                                    CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP);
}

static ClRcT amsMgmtCCBBatchSetNodeDependency(
                                              CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                              CL_IN   ClAmsEntityT    *nodeName,
                                              CL_IN   ClAmsEntityT    *dependencyNodeName,
                                              CL_IN ClAmsMgmtCCBOperationsT opId)

{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetNodeDependencyRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !nodeName || !dependencyNodeName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy(&req.nodeName, nodeName, sizeof(req.nodeName));
    memcpy(&req.dependencyNodeName, dependencyNodeName, 
           sizeof(req.dependencyNodeName));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyNodeName);
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetNodeDependencyRequestT, 4, 0, 0)(&req,
                                                                                 batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetNodeDependency(
                                         CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                         CL_IN   ClAmsEntityT    *nodeName,
                                         CL_IN   ClAmsEntityT    *dependencyNodeName )

{
    return amsMgmtCCBBatchSetNodeDependency(batchHandle, nodeName, dependencyNodeName,
                                            CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY);
}

ClRcT clAmsMgmtCCBBatchDeleteNodeDependency(
                                            CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                            CL_IN   ClAmsEntityT    *nodeName,
                                            CL_IN   ClAmsEntityT    *dependencyNodeName )

{
    return amsMgmtCCBBatchSetNodeDependency(batchHandle, nodeName, dependencyNodeName,
                                            CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY);
}

static ClRcT amsMgmtCCBBatchSetNodeSUList(
                                          CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                          CL_IN   ClAmsEntityT    *nodeName,
                                          CL_IN   ClAmsEntityT    *suName,
                                          CL_IN ClAmsMgmtCCBOperationsT opId)
{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetNodeSUListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !nodeName || !suName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.nodeName,nodeName,sizeof(req.nodeName));
    memcpy (&req.suName,suName,sizeof(req.suName));
    CL_AMS_NAME_LENGTH_CHECK(req.nodeName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);

    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetNodeSUListRequestT, 4, 0, 0)(&req,
                                                                             batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;
    
    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetNodeSUList(
                                     CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                     CL_IN   ClAmsEntityT    *nodeName,
                                     CL_IN   ClAmsEntityT    *suName)
{
    return amsMgmtCCBBatchSetNodeSUList(batchHandle, nodeName, suName,
                                        CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteNodeSUList(
                                        CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                        CL_IN   ClAmsEntityT    *nodeName,
                                        CL_IN   ClAmsEntityT    *suName )
{
    return amsMgmtCCBBatchSetNodeSUList(batchHandle, nodeName, suName,
                                        CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST);
}

static ClRcT amsMgmtCCBBatchSetSGSUList(
                                        CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                        CL_IN   ClAmsEntityT    *sgName,
                                        CL_IN   ClAmsEntityT    *suName,
                                        CL_IN   ClAmsMgmtCCBOperationsT opId)
{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSGSUListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !sgName || !suName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSGSUListRequestT, 4, 0, 0)(&req, 
                                                                           batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSGSUList(
                                   CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                   CL_IN   ClAmsEntityT    *sgName,
                                   CL_IN   ClAmsEntityT    *suName)
{
    return amsMgmtCCBBatchSetSGSUList(batchHandle, sgName, suName, 
                                      CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSGSUList(
                                      CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                      CL_IN   ClAmsEntityT    *sgName,
                                      CL_IN   ClAmsEntityT    *suName )
{
    return amsMgmtCCBBatchSetSGSUList(batchHandle, sgName, suName,
                                      CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST);
}

static ClRcT amsMgmtCCBBatchSetSGSIList(
                                        CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                        CL_IN   ClAmsEntityT    *sgName,
                                        CL_IN   ClAmsEntityT    *siName,
                                        CL_IN ClAmsMgmtCCBOperationsT opId)
{
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSGSIListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !sgName || !siName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.sgName,sgName,sizeof(ClAmsEntityT));
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.sgName);
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSGSIListRequestT, 4, 0, 0)(&req,
                                                                           batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSGSIList(
                                   CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                   CL_IN   ClAmsEntityT    *sgName,
                                   CL_IN   ClAmsEntityT    *siName)
{
    return amsMgmtCCBBatchSetSGSIList(batchHandle, sgName, siName,
                                      CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSGSIList(
                                      CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                      CL_IN   ClAmsEntityT    *sgName,
                                      CL_IN   ClAmsEntityT    *siName)
{
    return amsMgmtCCBBatchSetSGSIList(batchHandle, sgName, siName,
                                      CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST);
}

static ClRcT amsMgmtCCBBatchSetSUCompList(
                                          CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                          CL_IN   ClAmsEntityT    *suName,
                                          CL_IN   ClAmsEntityT    *compName,
                                          CL_IN ClAmsMgmtCCBOperationsT opId)

{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSUCompListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !suName || !compName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    memcpy (&req.compName,compName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    CL_AMS_NAME_LENGTH_CHECK(req.compName);

    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSUCompListRequestT, 4, 0, 0)(&req,
                                                                             batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSUCompList(
                                CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                CL_IN   ClAmsEntityT    *suName,
                                CL_IN   ClAmsEntityT    *compName)
{
    return amsMgmtCCBBatchSetSUCompList(batchHandle, suName, compName,
                                        CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSUCompList(
                                        CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                        CL_IN   ClAmsEntityT    *suName,
                                        CL_IN   ClAmsEntityT    *compName)
{
    return amsMgmtCCBBatchSetSUCompList(batchHandle, suName, compName,
                                        CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST);
}

static ClRcT amsMgmtCCBBatchSetSISURankList(
                                            CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                            CL_IN   ClAmsEntityT    *siName,
                                            CL_IN   ClAmsEntityT    *suName,
                                            CL_IN ClAmsMgmtCCBOperationsT opId)
{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSISURankListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;
    
    if(!batch || !siName || !suName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    
    req.suRank = 0;
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.suName,suName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.suName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSISURankListRequestT, 4, 0, 0)(&req,
                                                                               batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;
    
    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSISURankList(
                                       CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                       CL_IN   ClAmsEntityT    *siName,
                                       CL_IN   ClAmsEntityT    *suName)
{
    return amsMgmtCCBBatchSetSISURankList(batchHandle, siName, suName,
                                          CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSISURankList(
                                          CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                          CL_IN   ClAmsEntityT    *siName,
                                          CL_IN   ClAmsEntityT    *suName)
{
    return amsMgmtCCBBatchSetSISURankList(batchHandle, siName, suName,
                                          CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST);
}

static ClRcT amsMgmtCCBBatchSetSIDependency(
                                            CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                            CL_IN   ClAmsEntityT    *siName,
                                            CL_IN   ClAmsEntityT    *dependencySIName,
                                            CL_IN ClAmsMgmtCCBOperationsT opId)
{
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSISIDependencyRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !siName || !dependencySIName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.dependencySIName,dependencySIName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencySIName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSISIDependencyRequestT, 4, 0, 0)(&req,
                                                                                 batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSIDependency(
                                     CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                     CL_IN   ClAmsEntityT    *siName,
                                     CL_IN   ClAmsEntityT    *dependencySIName)
{
    return amsMgmtCCBBatchSetSIDependency(batchHandle, siName, dependencySIName,
                                          CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSIDependency(
                                          CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                          CL_IN   ClAmsEntityT    *siName,
                                          CL_IN   ClAmsEntityT    *dependencySIName)
{
    return amsMgmtCCBBatchSetSIDependency(batchHandle, siName, dependencySIName,
                                          CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST);
}

static ClRcT amsMgmtCCBBatchSetCSIDependency(
                                            CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                            CL_IN   ClAmsEntityT    *csiName,
                                            CL_IN   ClAmsEntityT    *dependencyCSIName,
                                            CL_IN ClAmsMgmtCCBOperationsT opId)
{
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetCSICSIDependencyRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;

    if(!batch || !csiName || !dependencyCSIName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    memcpy (&req.csiName, csiName,sizeof(ClAmsEntityT));
    memcpy (&req.dependencyCSIName,dependencyCSIName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    CL_AMS_NAME_LENGTH_CHECK(req.dependencyCSIName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetCSICSIDependencyRequestT, 4, 0, 0)(&req,
                                                                                   batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;

    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetCSIDependency(
                                        CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                        CL_IN   ClAmsEntityT    *csiName,
                                        CL_IN   ClAmsEntityT    *dependencyCSIName)
{
    return amsMgmtCCBBatchSetCSIDependency(batchHandle, csiName, dependencyCSIName,
                                           CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteCSIDependency(
                                           CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                           CL_IN   ClAmsEntityT    *csiName,
                                           CL_IN   ClAmsEntityT    *dependencyCSIName)
{
    return amsMgmtCCBBatchSetCSIDependency(batchHandle, csiName, dependencyCSIName,
                                           CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST);
}


static ClRcT amsMgmtCCBBatchSetSICSIList(
                                         CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                         CL_IN   ClAmsEntityT    *siName,
                                         CL_IN   ClAmsEntityT    *csiName,
                                         CL_IN ClAmsMgmtCCBOperationsT opId)
{

    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    clAmsMgmtCCBSetSICSIListRequestT  req = {0};
    ClInt32T op = (ClInt32T)opId;
    
    if(!batch || !siName || !csiName)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    
    memcpy (&req.siName,siName,sizeof(ClAmsEntityT));
    memcpy (&req.csiName,csiName,sizeof(ClAmsEntityT));
    CL_AMS_NAME_LENGTH_CHECK(req.siName);
    CL_AMS_NAME_LENGTH_CHECK(req.csiName);
    
    rc |= clXdrMarshallClInt32T(&op, batch->buffer, 0);
    rc |= VDECL_VER(clXdrMarshallclAmsMgmtCCBSetSICSIListRequestT, 4, 0, 0)(&req,
                                                                            batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    ++batch->items;
    
    out:
    return rc;
}

ClRcT clAmsMgmtCCBBatchSetSICSIList(
                                    CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                    CL_IN   ClAmsEntityT    *siName,
                                    CL_IN   ClAmsEntityT    *csiName)
{
    return amsMgmtCCBBatchSetSICSIList(batchHandle, siName, csiName,
                                       CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST);
}

ClRcT clAmsMgmtCCBBatchDeleteSICSIList(
                                       CL_IN   ClAmsMgmtCCBBatchHandleT batchHandle,
                                       CL_IN   ClAmsEntityT    *siName,
                                       CL_IN   ClAmsEntityT    *csiName)
{
    return amsMgmtCCBBatchSetSICSIList(batchHandle, siName, csiName,
                                       CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST);
}

ClRcT
clAmsMgmtCCBBatchCommit(CL_IN ClAmsMgmtCCBBatchHandleT batchHandle)
{
    ClRcT  rc = CL_OK;
    ClAmsMgmtCCBBatchT *batch = (ClAmsMgmtCCBBatchT*)batchHandle;
    ClUint32T saveOffset = 0;

    if(!batch)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!batch->buffer)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);

    if(!batch->items)
    {
        clLogWarning("CCB", "COMMIT", "Batch commit tried with no items");
        return CL_OK;
    }
    rc = clBufferWriteOffsetGet(batch->buffer, &saveOffset);
    if(rc != CL_OK)
        goto out;

    rc = clBufferWriteOffsetSet(batch->buffer, batch->itemsOffset, CL_BUFFER_SEEK_SET);
    if(rc != CL_OK)
        goto out;

    /*
     * Update the item count
     */
    rc = clXdrMarshallClUint32T(&batch->items, batch->buffer, 0);
    if(rc != CL_OK)
        goto out;
    
    rc = clBufferWriteOffsetSet(batch->buffer, saveOffset, CL_BUFFER_SEEK_SET);
    if(rc != CL_OK)
        goto out;

    rc = cl_ams_ccb_batch_rmd(CL_AMS_MGMT_CCB_BATCH_COMMIT, batch->buffer, CL_VERSION_CODE(5, 1, 0));
    if(rc != CL_OK)
        goto out;

    /*
     * Clear the buffer for reuse
     */
    rc = amsMgmtCCBBatchBufferInitialize(batch);

    out:
    return rc;
}

