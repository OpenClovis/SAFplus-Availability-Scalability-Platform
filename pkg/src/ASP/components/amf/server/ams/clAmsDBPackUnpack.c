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
 * File        : clAmsDBPackUnpack.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the AMS server side file used for dumping the AMS DB into a 
 * XML file and reconstructing the AMS DB from the XML file. The dumped 
 * XML file is checkpointed so that it can be used for AMS resiliency.
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

#include <clAmsDBPackUnpack.h>
#include <clCommonErrors.h>
#include <clAmsServerUtils.h>
#include <clAmsMgmtCommon.h>
#include <clAmsModify.h>
#include <clAmsCkpt.h>
#include <clAmsEntityUserData.h>
#include <clHandleIpi.h>

/******************************************************************************
 * Global structures and macros  
 *****************************************************************************/
/*
 * All entities in the AMS database are dumped into a XML file. This strcture holds the pointers to
 * the tags for specific entities. This is the root/key to generated XML tree.
 */
ClAmsParserPtrT  amfPtr = {0};
/*
 * All invocation entries in the invocation list are dumped into a XML file. This pointer is the
 * key/root for the XML generated for AMS invocations.
 */
ClParserPtrT  invocationPtr = {0};

/*
 * This is the global cluster reference. In future this should be replaced by specifc cluster pointer.
 */
extern ClAmsT gAms;

typedef struct ClAmsCCBHandleDBWalkArg
{
    ClInt32T nhandles;
    ClHandleT *pHandles;
}ClAmsCCBHandleDBWalkArgT;

#define AMS_XML_TAG_HEADER_NAME  "amf"
#define AMS_XML_TAG_INVOCATION_HEADER_NAME  "amfInvocation"
#define AMS_XML_TAG_GAMS  "gAms"

#define AMS_XML_TAG_NODE  "node"
#define AMS_XML_TAG_NODE_NAMES  "nodeNames"
#define AMS_XML_TAG_NODE_NAME  "nodeName"

#define AMS_XML_TAG_SG_NAME  "sgName"
#define AMS_XML_TAG_SG_NAMES  "sgNames"

#define AMS_XML_TAG_SU  "su"
#define AMS_XML_TAG_SU_NAME  "suName"
#define AMS_XML_TAG_SU_NAMES  "suNames"
#define AMS_XML_TAG_SU_LIST  "suList"
#define AMS_XML_TAG_INSTANTIABLE_SU_LIST  "instantiableSUList"
#define AMS_XML_TAG_INSTANTIATED_SU_LIST  "instantiatedSUList"
#define AMS_XML_TAG_IN_SERVICE_SPARE_SU_LIST  "spareSUList"
#define AMS_XML_TAG_ASSIGNED_SU_LIST  "assignedSUList"
#define AMS_XML_TAG_FAULTY_SU_LIST  "faultySUList"
#define AMS_XML_TAG_SU_FAILOVER_TIMER  "suFailoverTimer"

#define AMS_XML_TAG_SI  "si"
#define AMS_XML_TAG_SI_NAME  "siName"
#define AMS_XML_TAG_SI_NAMES  "siNames"
#define AMS_XML_TAG_SI_LIST  "siList"

#define AMS_XML_TAG_COMP  "comp"
#define AMS_XML_TAG_COMP_NAME  "compName"
#define AMS_XML_TAG_COMP_NAMES  "compNames"
#define AMS_XML_TAG_COMP_LIST  "compList"

#define AMS_XML_TAG_CSI  "csi"
#define AMS_XML_TAG_CSI_NAME  "csiName"
#define AMS_XML_TAG_CSI_NAMES  "csiNames"
#define AMS_XML_TAG_CSI_LIST  "csiList"
#define AMS_XML_TAG_CSI_TYPES "csiTypes"
#define AMS_XML_TAG_CSI_TYPE "csiType"

#define AMS_XML_TAG_NAME  "name"
#define AMS_XML_TAG_CONFIG  "config"
#define AMS_XML_TAG_STATUS  "status"
#define AMS_XML_TAG_ADMIN_STATE  "adminState"
#define AMS_XML_TAG_ID  "id"

#define AMS_XML_TAG_DEPENDENTS_LIST  "dependentsList"
#define AMS_XML_TAG_DEPENDENCIES_LIST  "dependenciesList"
#define AMS_XML_TAG_PG_LIST  "pgList"
#define AMS_XML_TAG_PG  "pg"

#define MAX_NUM_LENGTH_INT_8  4  
#define MAX_NUM_LENGTH_INT_32  16 
#define MAX_NUM_LENGTH_INT_64  256 


ClRcT   
clAmsDBPackUnpackMain ( 
        CL_INOUT  ClAmsT  *ams )
{

    AMS_CHECKPTR (!ams);

    amfPtr.headPtr = CL_PARSER_NEW(AMS_XML_TAG_HEADER_NAME);

    amfPtr.gAmsPtr= CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_GAMS, 1);

    amfPtr.nodeNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_NODE_NAMES, 1);

    amfPtr.sgNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_SG_NAMES, 1);

    amfPtr.suNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_SU_NAMES, 1);

    amfPtr.siNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_SI_NAMES, 1);

    amfPtr.compNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_COMP_NAMES, 1);

    amfPtr.csiNamesPtr = CL_PARSER_ADD_CHILD(
            amfPtr.headPtr, AMS_XML_TAG_CSI_NAMES, 1);

    return CL_OK;

}

ClRcT
clAmsInvocationMainMarshall(ClAmsT *ams, ClBufferHandleT msg)
{
    static ClVersionT currentVersion = { CL_RELEASE_VERSION, 1, CL_MINOR_VERSION };
    return clXdrMarshallClVersionT(&currentVersion, msg, 0);
}

ClRcT   
clAmsInvocationPackUnpackMain ( 
        CL_INOUT  ClAmsT  *ams )
{
    static ClCharT versionBuf[CL_MAX_NAME_LENGTH];
    AMS_CHECKPTR (!ams);
    invocationPtr = CL_PARSER_NEW(AMS_XML_TAG_INVOCATION_HEADER_NAME);
    snprintf(versionBuf, sizeof(versionBuf), "%d.%d.%d", 
             CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE);
    CL_PARSER_SET_ATTR(invocationPtr, "version", versionBuf);
    return CL_OK;
}

ClRcT
clAmsInvocationInstanceXMLize (
        CL_IN  ClAmsInvocationT  *invocationData )
{
    ClRcT  rc = CL_OK;
    ClCharT  *command = NULL;
    ClCharT  *csiTargetOne = NULL;
    ClCharT  *invocationID = NULL;
    
    AMS_CHECKPTR (!invocationData);

    ClParserPtrT ptr = 
        CL_PARSER_ADD_CHILD(invocationPtr ,"invocation", 0);

    AMS_CHECKPTR_AND_EXIT ( !ptr );

    AMS_CHECK_RC_ERROR (clAmsDBClUint64ToStr(
                invocationData->invocation,
                &invocationID));

    AMS_CHECK_RC_ERROR(clAmsDBClUint32ToStr(
                invocationData->cmd,
                &command));

    AMS_CHECK_RC_ERROR(clAmsDBClUint32ToStr(
                invocationData->csiTargetOne,
                &csiTargetOne));

    CL_PARSER_SET_ATTR (
            ptr,
            "compName",
            invocationData->compName.value);

    CL_PARSER_SET_ATTR (
            ptr,
            "id",
            invocationID);

    CL_PARSER_SET_ATTR (
            ptr,
            "command",
            command);

    CL_PARSER_SET_ATTR (
            ptr,
            "isCSITargetOne",
            csiTargetOne);

    if ( invocationData->csi )
    {
        CL_PARSER_SET_ATTR (
            ptr,
            "csi",
            invocationData->csi->config.entity.name.value);
    }

exitfn:
    clAmsFreeMemory (command) ;
    clAmsFreeMemory (csiTargetOne) ;
    clAmsFreeMemory (invocationID) ;

    return CL_AMS_RC (rc);

}

ClRcT
clAmsInvocationListUnmarshall (ClBufferHandleT msg)
{
    ClRcT  rc = CL_OK;
    ClUint32T numInvocations = 0;
    VDECL_VER(ClAmsInvocationIDLT, 4, 1, 0) *invocationList = NULL;
    ClUint32T i;

    AMS_CALL(clXdrUnmarshallClUint32T(msg, &numInvocations));

    if(!numInvocations) return rc;

    invocationList = clHeapCalloc(numInvocations, sizeof(*invocationList));
    CL_ASSERT(invocationList != NULL);
    
    for(i = 0; i < numInvocations; ++i)
    {
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsInvocationIDLT, 4, 1, 0)(msg, invocationList+i));
    }

    /*
     * We add to the invocation list once all entries are successfully marshalled.
     */

    for(i = 0; i < numInvocations; ++i)
    {
        clAmsDBConstructInvocation(
                                   &invocationList[i].compName,
                                   invocationList[i].invocation,
                                   invocationList[i].cmd,
                                   invocationList[i].csiTargetOne,
                                   &invocationList[i].csiName );
    }
    
    exitfn:
    clHeapFree(invocationList);
    return CL_OK;
}

ClRcT
clAmsInvocationInstanceDeXMLize (
       CL_IN  ClParserPtrT  invocationPtr )
{
    ClRcT  rc = CL_OK;
    const ClCharT  *command = NULL;
    const ClCharT  *invocationID = NULL;
    const ClCharT  *csiName= NULL;
    const ClCharT  *compName= NULL;
    const ClCharT  *csiTargetOne = NULL;

    AMS_CHECKPTR ( !invocationPtr);

    ClParserPtrT ptr = 
        clParserChild(invocationPtr ,"invocation");

    if ( !ptr )
    {
        return CL_OK;
    }

    while ( ptr )
    {
        compName = clParserAttr(
                ptr,
                "compName");

        command = clParserAttr(
                ptr,
                "command");

        invocationID = clParserAttr(
                ptr,
                "id");

        csiTargetOne = clParserAttr(
                ptr,
                "isCSITargetOne");

         csiName = clParserAttr(
                 ptr,
                 "csi" );

         clAmsDBConstructInvocationList(
                     compName,
                     invocationID,
                     command,
                     csiTargetOne,
                     csiName );

         ptr = ptr->next;
    }

    return CL_AMS_RC (rc);

}

ClRcT 
clAmsDBConstructInvocation(
        CL_IN  const ClNameT  *compName,
        CL_IN  ClInvocationT id,
        CL_IN  ClAmsInvocationCmdT cmd,
        CL_IN  ClBoolT csiTargetOne,
        CL_IN  const ClNameT  *csiName ) 
{
    ClRcT  rc = CL_OK;
    ClAmsInvocationT  *invocationData = NULL;

    AMS_CHECKPTR ( !compName );

    invocationData = clHeapCalloc (1, sizeof (ClAmsInvocationT));

    AMS_CHECK_NO_MEMORY ( invocationData );

    invocationData->invocation = id;
    invocationData->cmd = cmd;
    invocationData->csiTargetOne = csiTargetOne;
    clNameCopy(&invocationData->compName, compName);
    invocationData->csi = NULL;
    invocationData->reassignCSI = CL_FALSE;

    if ( csiName && csiName->length > 0)
    {

        ClAmsEntityRefT  entityRef = {{0},NULL,0};

        clNameCopy(&invocationData->csiName, csiName);

        memset (&entityRef,0,sizeof (ClAmsEntityRefT));
        clNameCopy(&entityRef.entity.name, csiName);
        entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                    &entityRef) );

        invocationData->csi = (ClAmsCSIT *)entityRef.ptr;

    }
    /*
     * Add to AMS + CPM invocation list.
     */
    AMS_CHECK_RC_ERROR ( clAmsInvocationListAddAll(invocationData) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (invocationData);
    return CL_AMS_RC (rc);

}

ClRcT 
clAmsDBConstructInvocationList(
        CL_IN  const ClCharT  *compName,
        CL_IN  const ClCharT  *id,
        CL_IN  const ClCharT  *command,
        CL_IN  const ClCharT  *csiTargetOne,
        CL_IN  const ClCharT  *csiName ) 
{
    ClRcT  rc = CL_OK;
    ClAmsInvocationT  *invocationData = NULL;

    AMS_CHECKPTR ( !compName || !command || !csiTargetOne || !id);

    invocationData = clHeapCalloc (1, sizeof (ClAmsInvocationT));

    AMS_CHECK_NO_MEMORY ( invocationData );

    invocationData->invocation = atoll (id);
    invocationData->cmd = atoi (command);
    invocationData->csiTargetOne = atoi (csiTargetOne);
    clNameSet(&invocationData->compName, compName);
    ++invocationData->compName.length;
    invocationData->csi = NULL;
    invocationData->reassignCSI = CL_FALSE;

    if ( csiName )
    {

        ClAmsEntityRefT  entityRef = {{0},NULL,0};

        clNameSet(&invocationData->csiName, csiName);
        ++invocationData->csiName.length;

        memset (&entityRef,0,sizeof (ClAmsEntityRefT));
        strcpy (entityRef.entity.name.value,csiName);
        entityRef.entity.name.length = strlen (csiName) + 1;
        entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                    &entityRef) );

        invocationData->csi = (ClAmsCSIT *)entityRef.ptr;

    }
    /*
     * Add to AMS + CPM invocation list.
     */
    AMS_CHECK_RC_ERROR ( clAmsInvocationListAddAll(invocationData) );

    return CL_OK;

exitfn:

    clAmsFreeMemory (invocationData);
    return CL_AMS_RC (rc);

}

static ClRcT 
clAmsReassignOpSUMarshall(void *data, ClUint32T dataSize, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsSUReassignOpT *reassignEntry = data;
    ClInt32T i;

    if(!data) return rc;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&reassignEntry->numSIs, inMsgHdl, 0));
    for(i = 0; i < reassignEntry->numSIs; ++i)
    {
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(reassignEntry->sis + i,
                                                           inMsgHdl, 0));
    }

    exitfn:
    return rc;
}

static ClRcT
clAmsReassignOpSUUnmarshall(void **data, ClUint32T *dataSize, ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;
    ClAmsSUReassignOpT reassignOp = {0};
    ClInt32T i;

    if(!data || !dataSize) return rc;
    if(!*dataSize) return rc;

    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &reassignOp.numSIs));
    reassignOp.sis = clHeapCalloc(reassignOp.numSIs, sizeof(*reassignOp.sis));
    CL_ASSERT(reassignOp.sis != NULL);

    for(i = 0; i < reassignOp.numSIs; ++i)
    {
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, reassignOp.sis + i));
    }
    
    *data = clHeapCalloc(1, sizeof(reassignOp));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &reassignOp, sizeof(reassignOp));
    *dataSize = (ClUint32T) sizeof(reassignOp);

    return CL_OK;
    
    exitfn:
    if(reassignOp.sis) clHeapFree(reassignOp.sis);
    return rc;
}

static ClRcT
clAmsReassignOpSIMarshall(void *data, ClUint32T dataSize, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsSIReassignOpT *reassignEntry = data;
    ClRcT rc = CL_OK;
    if(!data) return rc;
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&reassignEntry->su, inMsgHdl, 0));
    exitfn:
    return rc;
}

static ClRcT
clAmsReassignOpSIUnmarshall(void **data, ClUint32T *dataSize, ClBufferHandleT inMsgHdl)
{
    ClAmsSIReassignOpT reassignOp = {{0}};
    ClRcT rc = CL_OK;
    if(!data || !dataSize) return rc;
    *data = NULL;
    if(!*dataSize) return rc;
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &reassignOp.su));
    *data = clHeapCalloc(1, sizeof(reassignOp));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &reassignOp, sizeof(reassignOp));
    *dataSize = (ClUint32T)sizeof(reassignOp);
    
    exitfn:
    return rc;
}

static ClRcT
clAmsEntitySIReassignOpMarshall(ClAmsEntityT *entity, void *data, ClUint32T dataSize, 
                                ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    if(!entity) return CL_OK;
    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_SI:
        return clAmsReassignOpSIMarshall(data, dataSize, inMsgHdl, versionCode);

    case CL_AMS_ENTITY_TYPE_SU:
        return clAmsReassignOpSUMarshall(data, dataSize, inMsgHdl, versionCode);

    default: break;
    }
    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
}

static ClRcT
clAmsEntitySIReassignOpUnmarshall(ClAmsEntityT *entity, void **data, ClUint32T *dataSize, 
                                  ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    if(!entity) return CL_OK;
    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_SI:
        return clAmsReassignOpSIUnmarshall(data, dataSize, inMsgHdl);
        
    case CL_AMS_ENTITY_TYPE_SU:
        return clAmsReassignOpSUUnmarshall(data, dataSize, inMsgHdl);

    default: break;
    }

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
}

static ClRcT
clAmsEntityReduceRemoveOpMarshall(ClAmsEntityT *entity, void *data, ClUint32T dataSize, 
                                  ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsEntityReduceRemoveOpT *reduceOp = data;
    ClRcT rc = CL_OK;
    if(!reduceOp) return rc;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&reduceOp->sisRemoved, inMsgHdl, 0));
    exitfn:
    return rc;
}

static ClRcT
clAmsEntityReduceRemoveOpUnmarshall(ClAmsEntityT *entity, void **data, ClUint32T *dataSize, 
                                    ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsEntityReduceRemoveOpT reduceOp = {0};
    ClUint32T size = 0;
    
    if(!data || !dataSize) return CL_OK;
    *data = NULL;
    size = *dataSize;
    if(!size)  return CL_OK;

    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &reduceOp.sisRemoved));
    
    *data = clHeapCalloc(1, sizeof(ClAmsEntityReduceRemoveOpT));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &reduceOp, sizeof(ClAmsEntityReduceRemoveOpT));
    *dataSize = (ClUint32T)sizeof(ClAmsEntityReduceRemoveOpT);
    
    exitfn:
    return rc;
}

static ClRcT
clAmsEntitySwapActiveOpMarshall(ClAmsEntityT *entity, void *data, ClUint32T dataSize, 
                                ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsEntitySwapActiveOpT *swapActive = data;
    ClRcT rc = CL_OK;
    if(!swapActive) return rc;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&swapActive->sisReassigned, inMsgHdl, 0));
    exitfn:
    return rc;
}

static ClRcT
clAmsEntitySwapActiveOpUnmarshall(ClAmsEntityT *entity, void **data, ClUint32T *dataSize, 
                                  ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsEntitySwapActiveOpT swapOp = {0};
    ClUint32T size = 0;
    
    if(!data || !dataSize) return CL_OK;
    *data = NULL;
    size = *dataSize;
    if(!size)  return CL_OK;

    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &swapOp.sisReassigned));
    
    *data = clHeapCalloc(1, sizeof(swapOp));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &swapOp, sizeof(ClAmsEntitySwapActiveOpT));
    *dataSize = (ClUint32T)sizeof(ClAmsEntitySwapActiveOpT);
    
    exitfn:
    return rc;
}

static ClRcT
clAmsEntitySwapRemoveOpMarshall(ClAmsEntityT *entity, void *data, ClUint32T dataSize, 
                                ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsEntitySwapRemoveOpT *swapOp = data;
    ClRcT rc = CL_OK;
    ClInt32T i;
    if(!swapOp) return rc;
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&swapOp->entity, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&swapOp->sisRemoved, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&swapOp->numOtherSIs, inMsgHdl, 0));
    
    if(swapOp->numOtherSIs)
    {
        for(i = 0; i < swapOp->numOtherSIs; ++i)
        {
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&swapOp->otherSIs[i], inMsgHdl, 0));
        }
    }
    exitfn:
    return rc;
}

static ClRcT
clAmsEntitySwapRemoveOpUnmarshall(ClAmsEntityT *entity, void **data, ClUint32T *dataSize, 
                                  ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsEntitySwapRemoveOpT swapOp = {{0}};
    ClUint32T size = 0;
    ClInt32T i;

    if(!data || !dataSize) return CL_OK;
    *data = NULL;
    size = *dataSize;
    if(!size)  return CL_OK;

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &swapOp.entity));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &swapOp.sisRemoved));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &swapOp.numOtherSIs));

    swapOp.otherSIs = clHeapCalloc(swapOp.numOtherSIs, sizeof(ClAmsEntityT));
    CL_ASSERT(swapOp.otherSIs != NULL);
    for(i = 0; i < swapOp.numOtherSIs; ++i)
    {
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, (void*) &swapOp.otherSIs[i]));
    }
    
    *data = clHeapCalloc(1, sizeof(ClAmsEntitySwapRemoveOpT));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &swapOp, sizeof(ClAmsEntitySwapRemoveOpT));
    *dataSize = (ClUint32T)sizeof(ClAmsEntitySwapRemoveOpT);
    return CL_OK;

    exitfn:
    if(swapOp.otherSIs) clHeapFree(swapOp.otherSIs);
    return rc;
}

static ClRcT
clAmsEntityRemoveOpMarshall(ClAmsEntityT *entity, void *data, ClUint32T dataSize, 
                            ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsEntityRemoveOpT *removeOp = data;
    ClRcT rc = CL_OK;
    if(!removeOp) return CL_OK;
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&removeOp->entity, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&removeOp->sisRemoved, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&removeOp->switchoverMode, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&removeOp->error, inMsgHdl, 0));
    exitfn:
    return rc;
}

static ClRcT
clAmsEntityRemoveOpUnmarshall(ClAmsEntityT *entity, void **data, ClUint32T *dataSize, 
                              ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRemoveOpT removeOp ;
    ClUint32T size = 0;
    
    if(!data || !dataSize) return CL_OK;
    *data = NULL;
    size = *dataSize;
    if(!size)  return CL_OK;
    memset(&removeOp, 0, sizeof(removeOp));

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &removeOp.entity));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &removeOp.sisRemoved));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &removeOp.switchoverMode));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &removeOp.error));
    
    *data = clHeapCalloc(1, sizeof(ClAmsEntityRemoveOpT));
    CL_ASSERT(*data != NULL);
    memcpy(*data, &removeOp, sizeof(ClAmsEntityRemoveOpT));
    *dataSize = (ClUint32T)sizeof(ClAmsEntityRemoveOpT);
    
    exitfn:
    return rc;
}

static ClRcT 
clAmsEntityOpMarshall(ClAmsEntityT *entity, ClUint32T op, void *data, ClUint32T dataSize, 
                      ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    switch(op)
    {
    case CL_AMS_ENTITY_OP_REMOVE_MPLUSN:
    case CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN:
        {
            return clAmsEntityRemoveOpMarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
        break;
    case CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN:
        {
            return clAmsEntitySwapRemoveOpMarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN:
        {
            return clAmsEntitySwapActiveOpMarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN:
        {
            return clAmsEntityReduceRemoveOpMarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN:
        {
            return clAmsEntitySIReassignOpMarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    default: break;
    }
    return CL_OK;
}

static ClRcT
clAmsEntityOpUnmarshall(ClAmsEntityT *entity, ClUint32T op, void **data, ClUint32T *dataSize, 
                        ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    switch(op)
    {
    case CL_AMS_ENTITY_OP_REMOVE_MPLUSN:
    case CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN:
        {
            return clAmsEntityRemoveOpUnmarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN:
        {
            return clAmsEntitySwapRemoveOpUnmarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN:
        {
            return clAmsEntitySwapActiveOpUnmarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN:
        {
            return clAmsEntityReduceRemoveOpUnmarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    case CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN:
        {
            return clAmsEntitySIReassignOpUnmarshall(entity, data, dataSize, inMsgHdl, versionCode);
        }
    default: break;
    }

    return CL_OK;
}

static ClRcT
clAmsDBEntityOpMarshall(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClListHeadT *opStackList = NULL;
    ClListHeadT *iter = NULL;
    ClRcT rc = CL_OK;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&status->opStack.numOps, inMsgHdl, 0));
    if(!status->opStack.opList.pNext)
        CL_LIST_HEAD_INIT(&status->opStack.opList);
    opStackList = &status->opStack.opList;
    CL_LIST_FOR_EACH(iter, opStackList)
    {
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(iter, ClAmsEntityOpT, list);
        AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&opBlock->op, inMsgHdl, 0));
        if(!opBlock->data) opBlock->dataSize = 0;
        AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&opBlock->dataSize, inMsgHdl, 0));
        if(opBlock->data)
            AMS_CHECK_RC_ERROR(clAmsEntityOpMarshall(entity, opBlock->op, opBlock->data, opBlock->dataSize, 
                                                     inMsgHdl, versionCode));
    }
    exitfn:
    return CL_OK;
}

static ClRcT
clAmsDBEntityOpUnmarshall(ClAmsEntityT *entity, ClAmsEntityStatusT *status, 
                          ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClInt32T numOps = 0;
    ClInt32T i;
    ClRcT rc = CL_OK;

    CL_LIST_HEAD_DECLARE(tempList);
    CL_LIST_HEAD_INIT(&status->opStack.opList);
    status->opStack.numOps = 0;
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &numOps));
    for(i = 0; i < numOps; ++i)
    {
        ClAmsEntityOpT *opBlock = clHeapCalloc(1, sizeof(*opBlock));
        CL_ASSERT(opBlock != NULL);
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &opBlock->op));
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &opBlock->dataSize));
        AMS_CHECK_RC_ERROR(clAmsEntityOpUnmarshall(entity, opBlock->op, &opBlock->data, &opBlock->dataSize, 
                                                   inMsgHdl, versionCode));
        clListAddTail(&opBlock->list, &tempList);
    }
    status->opStack.numOps = numOps;
    if(numOps)
        clListMoveInit(&tempList, &status->opStack.opList);

    exitfn:
    return rc;
}

/*
 * Marshall the node config information with the AMS request .
 */

ClRcT   
clAmsDBNodeMarshall(ClAmsNodeT  *node, ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;

    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;

    AMS_CHECK_RC_ERROR( clXdrMarshallClInt32T (&op, inMsgHdl, 0));

    AMS_CHECK_RC_ERROR (VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&node->config.entity, inMsgHdl, 0));

    if( (marshallMask & CL_AMS_ENTITY_DB_CONFIG) )
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;

        AMS_CHECK_RC_ERROR ( clXdrMarshallClInt32T(&op, inMsgHdl, 0) );

        /*write the node config first*/
        AMS_CHECK_RC_ERROR ( VDECL_VER(clXdrMarshallClAmsNodeConfigT, 4, 0, 0)(&node->config, inMsgHdl, 0) );
    }

    if( (marshallMask & CL_AMS_ENTITY_DB_CONFIG_LIST))
    {
        /*
         * Write the nodeDependendents, nodeDependencies and suLists
         */
        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(&node->config.nodeDependentsList,
                                                 CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR( clAmsDBListMarshall(&node->config.nodeDependenciesList,
                                                CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST,
                                                inMsgHdl, versionCode ) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(&node->config.suList, 
                                                 CL_AMS_NODE_CONFIG_SU_LIST,
                                                 inMsgHdl, versionCode ) );
    }

    if( (marshallMask & CL_AMS_ENTITY_DB_STATUS) )
    {
        /*
         * Write the status portion
         */
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;

        AMS_CHECK_RC_ERROR ( clXdrMarshallClInt32T(&op, inMsgHdl, 0) );

        AMS_CHECK_RC_ERROR ( VDECL_VER(clXdrMarshallClAmsNodeStatusT, 4, 0, 0)(&node->status, inMsgHdl, 0) );

        /*
         * Marshall the entity op.
         */
        AMS_CHECK_RC_ERROR (clAmsDBEntityOpMarshall(&node->config.entity, &node->status.entity, inMsgHdl, versionCode));

        /* 
         * Write the timer portion.
         */
        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(&node->status.suFailoverTimer,
                                                  CL_AMS_NODE_TIMER_SUFAILOVER,
                                                  inMsgHdl, versionCode) );
    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;

    AMS_CHECK_RC_ERROR ( clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    exitfn:

    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBNodeXMLize(
       CL_IN  ClAmsNodeT  *node)
{
    ClRcT  rc = CL_OK;
    ClCharT  *adminState = NULL;
    ClCharT  *id = NULL;
    ClCharT  *class = NULL;
    ClCharT  *isSwappable = NULL;
    ClCharT  *isRestartable = NULL;
    ClCharT  *autoRepair = NULL;
    ClCharT  *isASPAware = NULL;
    ClCharT  *suFailoverDuration = NULL;
    ClCharT  *suFailoverCountMax = NULL;

    ClParserPtrT nodePtr = 
        CL_PARSER_ADD_CHILD(amfPtr.nodeNamesPtr,AMS_XML_TAG_NODE_NAME, 0);

    AMS_CHECKPTR (!node);

    CL_PARSER_SET_ATTR (
            nodePtr,
            AMS_XML_TAG_NAME,
            node->config.entity.name.value);

    ClParserPtrT configPtr = CL_PARSER_ADD_CHILD(nodePtr,AMS_XML_TAG_CONFIG , 0);

    AMS_CHECKPTR_AND_EXIT(!configPtr);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.adminState,
                &adminState) );

    CL_PARSER_SET_ATTR (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE,
            adminState);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.id,
                &id) );

    CL_PARSER_SET_ATTR (
            configPtr,
            AMS_XML_TAG_ID,
            id);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.classType,
                &class) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "class",
            class);

    CL_PARSER_SET_ATTR (
            configPtr,
            "subClass",
            node->config.subClassType.value);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.isSwappable,
                &isSwappable) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "isSwappable",
            isSwappable);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.isRestartable,
                &isRestartable) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "isRestartable",
            isRestartable);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.autoRepair,
                &autoRepair) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "autoRepair",
            autoRepair);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.isASPAware,
                &isASPAware) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "isASPAware",
            isASPAware );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                node->config.suFailoverDuration,
                &suFailoverDuration) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "suFailoverDuration",
            suFailoverDuration);

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->config.suFailoverCountMax,
                &suFailoverCountMax) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "suFailoverCountMax",
            suFailoverCountMax);

    /*
     * Write the nodeDependendents, nodeDependencies and suLists
     */

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &node->config.nodeDependentsList,
                CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST ) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &node->config.nodeDependenciesList,
                CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &node->config.suList,
                CL_AMS_NODE_CONFIG_SU_LIST) );

    /*
     * Write the status portion
     */

    ClParserPtrT  statusPtr = CL_PARSER_ADD_CHILD(nodePtr,AMS_XML_TAG_STATUS,0);

    AMS_CHECKPTR_AND_EXIT (!statusPtr);

    ClCharT  *presenceState = NULL;
    ClCharT  *operState = NULL;
    ClCharT  *isClusterMember = NULL;
    ClCharT  *wasMemberBefore = NULL;
    ClCharT  *recovery = NULL;
    ClCharT  *alarmHandle = NULL;
    ClCharT  *suFailoverCount= NULL;
    ClCharT  *numInstantiatedSUs = NULL;
    ClCharT  *numAssignedSUs = NULL; 
    
    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &node->status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.presenceState,
                &presenceState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.operState,
                &operState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.isClusterMember,
                &isClusterMember) );



    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.wasMemberBefore,
                &wasMemberBefore) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.recovery,
                &recovery) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.alarmHandle,
                &alarmHandle) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.suFailoverCount,
                &suFailoverCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.numInstantiatedSUs,
                &numInstantiatedSUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                node->status.numAssignedSUs,
                &numAssignedSUs) );

    CL_PARSER_SET_ATTR (
            statusPtr,
            "presenceState",
            presenceState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "operState",
            operState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "isClusterMember",
            isClusterMember);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "wasMemberBefore",
            wasMemberBefore);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "recovery",
            recovery);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "alarmHandle",
            alarmHandle);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "suFailoverCount",
            suFailoverCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numInstantiatedSUs",
            numInstantiatedSUs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numAssignedSUs",
            numAssignedSUs);

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusPtr,
                &node->status.suFailoverTimer,
                AMS_XML_TAG_SU_FAILOVER_TIMER) );

exitfn:

    clAmsFreeMemory (adminState);
    clAmsFreeMemory (id);
    clAmsFreeMemory (class);
    clAmsFreeMemory (isSwappable);
    clAmsFreeMemory (isRestartable);
    clAmsFreeMemory (autoRepair);
    clAmsFreeMemory (isASPAware);
    clAmsFreeMemory (suFailoverDuration);
    clAmsFreeMemory (suFailoverCountMax);
    clAmsFreeMemory (presenceState);
    clAmsFreeMemory (operState);
    clAmsFreeMemory (isClusterMember);
    clAmsFreeMemory (wasMemberBefore);
    clAmsFreeMemory (recovery);
    clAmsFreeMemory (alarmHandle);
    clAmsFreeMemory (suFailoverCount);
    clAmsFreeMemory (numInstantiatedSUs);
    clAmsFreeMemory (numAssignedSUs);

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBNodeListDeXMLize(
        CL_IN  ClParserPtrT  nodePtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;
    ClAmsEntityTimerT  entityTimer = {0};

    name = clParserAttr (
            nodePtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("nodeName tag does not have node name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
   
    ClParserPtrT configPtr = clParserChild(nodePtr,AMS_XML_TAG_CONFIG);
    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("NODE[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
    
    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST ));

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST ));

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_NODE_CONFIG_SU_LIST ));

    ClParserPtrT statusPtr = clParserChild(nodePtr,AMS_XML_TAG_STATUS);
    if ( !statusPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("NODE[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    entityTimer.count = 0;
    entityTimer.type = CL_AMS_NODE_TIMER_SUFAILOVER;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusPtr,
                &entityTimer,
                AMS_XML_TAG_SU_FAILOVER_TIMER) );

    ClAmsEntityT  entity = {0};

    strcpy ( entity.name.value, name);
    entity.name.length = strlen (name) + 1;
    entity.type = CL_AMS_ENTITY_TYPE_NODE;

    AMS_CHECK_RC_ERROR ( clAmsSetEntityTimer(
                &entity,
                &entityTimer) );
exitfn:

    return CL_AMS_RC (rc);

}

static ClRcT
clAmsDBNodeUnmarshall(ClAmsEntityT *entity,
                      ClBufferHandleT inMsgHdl,
                      ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsNodeT *node = NULL;

    AMS_CHECK_ENTITY_TYPE(entity->type);
    
    if(entity->type != CL_AMS_ENTITY_TYPE_NODE)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Entity type [%d] invalid for node unmarshall\n", entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type], &entityRef);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }

        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("[%s] entitydb operation returned [%#x]\n",
                                     CL_AMS_STRING_ENTITY_TYPE(entity->type), rc));
            goto exitfn;
        }
    }

    node = (ClAmsNodeT*)entityRef.ptr;
    
    CL_ASSERT(node != NULL);

    for(;;)
    {
        ClAmsCkptOperationT op  = 0;

        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));

        switch(op)
        {

        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsNodeConfigT nodeConfig = { {0} };
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsNodeConfigT, 4, 0, 0)(inMsgHdl, &nodeConfig) );
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &nodeConfig.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsNodeStatusT nodeStatus = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsNodeStatusT, 4, 0, 0)(inMsgHdl, &nodeStatus));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &nodeStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &nodeStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP:
            return CL_OK; /*break out of processing*/

        default: break;

        }
    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsDBNodeDeXMLize(
       CL_IN  ClParserPtrT  nodePtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;
    const ClCharT  *adminState = NULL; 
    const ClCharT  *id = NULL; 
    const ClCharT  *class = NULL; 
    const ClCharT  *subClass = NULL; 
    const ClCharT  *isSwappable = NULL; 
    const ClCharT  *isRestartable = NULL; 
    const ClCharT  *autoRepair = NULL; 
    const ClCharT  *isASPAware = NULL; 
    const ClCharT  *suFailoverDuration = NULL; 
    const ClCharT  *suFailoverCountMax = NULL; 

    name = clParserAttr (
            nodePtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("nodeName tag does not have node name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
   
    ClParserPtrT configPtr = clParserChild(nodePtr,AMS_XML_TAG_CONFIG);
    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("nodeName %s tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    adminState = clParserAttr (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE);

    id = clParserAttr (
            configPtr,
            AMS_XML_TAG_ID );

    class = clParserAttr (
            configPtr,
            "class");

    subClass = clParserAttr(
            configPtr,
            "subClass");

    isSwappable = clParserAttr(
            configPtr,
            "isSwappable");

    isRestartable = clParserAttr (
            configPtr,
            "isRestartable");

    autoRepair = clParserAttr (
            configPtr,
            "autoRepair");

    isASPAware = clParserAttr (
            configPtr,
            "isASPAware");

    suFailoverDuration = clParserAttr (
            configPtr,
            "suFailoverDuration");

    suFailoverCountMax= clParserAttr (
            configPtr,
            "suFailoverCountMax");

    if ( !adminState || !id || !class || !subClass || !isSwappable || 
            !isRestartable  || !autoRepair || !isASPAware || 
            !suFailoverDuration || !suFailoverCountMax )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("nodeName %s has a missing config attribute\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    /*
     * Get the status portion
     */

    ClParserPtrT statusPtr = clParserChild(nodePtr,AMS_XML_TAG_STATUS);

    AMS_CHECKPTR_AND_EXIT (!statusPtr);

    const ClCharT  *presenceState = NULL;
    const ClCharT  *operState = NULL;
    const ClCharT  *isClusterMember = NULL;
    const ClCharT  *wasMemberBefore = NULL;
    const ClCharT  *recovery = NULL;
    const ClCharT  *alarmHandle = NULL;
    const ClCharT  *suFailoverCount = NULL;
    const ClCharT  *numInstantiatedSUs = NULL;
    const ClCharT  *numAssignedSUs = NULL;

    presenceState = clParserAttr(
            statusPtr,
            "presenceState");

    operState = clParserAttr(
            statusPtr,
            "operState");

    isClusterMember = clParserAttr(
            statusPtr,
            "isClusterMember");

    wasMemberBefore = clParserAttr(
            statusPtr,
            "wasMemberBefore");

    recovery = clParserAttr(
            statusPtr,
            "recovery");

    alarmHandle = clParserAttr(
            statusPtr,
            "alarmHandle");

    suFailoverCount = clParserAttr(
            statusPtr,
            "suFailoverCount");

    numInstantiatedSUs = clParserAttr(
            statusPtr,
            "numInstantiatedSUs");

    numAssignedSUs = clParserAttr(
            statusPtr,
            "numAssignedSUs");

    if ( !presenceState ||!operState || !isClusterMember || !wasMemberBefore || 
            !recovery || !suFailoverCount || !numInstantiatedSUs || 
            !numAssignedSUs || !alarmHandle )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("nodeName %s has a missing status attribute\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT entityRef = {{0},0,0};
    ClAmsNodeT      node ;

    entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &entityRef) );

    /*
     * Write the config part
     */

    memcpy (&node.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    node.config.adminState = atoi (adminState);
    node.config.id= atoi (id);
    node.config.classType = atoi (class);
    strcpy (node.config.subClassType.value, subClass);
    node.config.subClassType.length = strlen (subClass) + 1;
    node.config.isSwappable= atoi (isSwappable);
    node.config.isRestartable= atoi (isRestartable);
    node.config.autoRepair= atoi (autoRepair);
    node.config.isASPAware= atoi (isASPAware);
    node.config.suFailoverDuration= atol (suFailoverDuration);
    node.config.suFailoverCountMax= atoi (suFailoverCountMax);

    /*
     * Write the status part
     */
    node.status.presenceState = atoi (presenceState);
    node.status.operState= atoi (operState);
    node.status.isClusterMember= atoi (isClusterMember);
    node.status.wasMemberBefore= atoi (wasMemberBefore);
    node.status.recovery= atoi (recovery);
    node.status.alarmHandle= atoi (alarmHandle);
    node.status.suFailoverCount= atoi (suFailoverCount);
    node.status.numInstantiatedSUs= atoi (numInstantiatedSUs);
    node.status.numAssignedSUs= atoi (numAssignedSUs);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &node.status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&node.config) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&node.status) );

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsDBSUMarshall(ClAmsSUT *su, ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;

    AMS_CHECKPTR ( !su );

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&su->config.entity, inMsgHdl, 0));

    if( (marshallMask & CL_AMS_ENTITY_DB_CONFIG))
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSUConfigT, 4, 0, 0)(&su->config, inMsgHdl, 0));
    }
    
    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG_LIST))
    {
        /*
         * Write the compList
         */

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &su->config.compList,
                                                 CL_AMS_SU_CONFIG_COMP_LIST,
                                                 inMsgHdl, versionCode) );
    }

    if( (marshallMask & CL_AMS_ENTITY_DB_STATUS))
    {
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSUStatusT, 4, 0, 0)(&su->status, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(clAmsDBEntityOpMarshall(&su->config.entity, &su->status.entity, inMsgHdl, versionCode));
        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &su->status.compRestartTimer,
                                                  CL_AMS_SU_TIMER_COMPRESTART,
                                                  inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &su->status.suRestartTimer,
                                                  CL_AMS_SU_TIMER_SURESTART,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &su->status.suProbationTimer,
                                                  CL_AMS_SU_TIMER_PROBATION,
                                                  inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &su->status.suAssignmentTimer,
                                                  CL_AMS_SU_TIMER_ASSIGNMENT,
                                                  inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &su->status.siList,
                                                 CL_AMS_SU_STATUS_SI_LIST,
                                                 inMsgHdl, versionCode) );
    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsDBSUXMLize(
       CL_IN  ClAmsSUT  *su)
{
    ClRcT  rc = CL_OK;

    ClParserPtrT suPtr = 
        CL_PARSER_ADD_CHILD(amfPtr.suNamesPtr,AMS_XML_TAG_SU_NAME, 0);

    AMS_CHECKPTR ( !su );

    CL_PARSER_SET_ATTR(
            suPtr,
            AMS_XML_TAG_NAME,
            su->config.entity.name.value);

    ClParserPtrT configPtr = CL_PARSER_ADD_CHILD( suPtr,AMS_XML_TAG_CONFIG , 0);

    AMS_CHECKPTR_AND_EXIT (!configPtr);

    ClCharT  *adminState = NULL;
    ClCharT  *rank = NULL;
    ClCharT  *numComponents = NULL;
    ClCharT  *isPreinstantiable = NULL;
    ClCharT  *isRestartable = NULL;
    ClCharT  *isContainerSU = NULL;

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.adminState,
                &adminState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.rank,
                &rank) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.numComponents,
                &numComponents) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.isPreinstantiable,
                &isPreinstantiable) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.isRestartable,
                &isRestartable) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->config.isContainerSU,
                &isContainerSU) );

    CL_PARSER_SET_ATTR (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE,
            adminState);

    CL_PARSER_SET_ATTR (
            configPtr,
            "rank",
            rank);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numComponents",
            numComponents);

    CL_PARSER_SET_ATTR (
            configPtr,
            "isPreinstantiable",
            isPreinstantiable);

    CL_PARSER_SET_ATTR (
            configPtr,
            "isRestartable",
            isRestartable);

    CL_PARSER_SET_ATTR (
            configPtr,
            "isContainerSU",
            isContainerSU);

    CL_PARSER_SET_ATTR (
            configPtr,
            "parentSG",
            su->config.parentSG.entity.name.value);

    CL_PARSER_SET_ATTR (
            configPtr,
            "parentNode",
            su->config.parentNode.entity.name.value);

    /*
     * Write the compList
     */

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &su->config.compList,
                CL_AMS_SU_CONFIG_COMP_LIST) );

    ClCharT  *presenceState = NULL;
    ClCharT  *operState = NULL;
    ClCharT  *readinessState = NULL;
    ClCharT  *numActiveSIs = NULL;
    ClCharT  *numStandbySIs = NULL;
    ClCharT  *compRestartCount = NULL;
    ClCharT  *suRestartCount = NULL;
    ClCharT  *numInstantiatedComp = NULL;
    ClCharT  *numQuiescedSIs = NULL;
    ClCharT  *recovery = NULL;
    ClCharT  *numPIComp = NULL;

    ClParserPtrT statusPtr = CL_PARSER_ADD_CHILD( suPtr,AMS_XML_TAG_STATUS , 0);

    AMS_CHECKPTR_AND_EXIT (!statusPtr);

    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &su->status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.presenceState,
                &presenceState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.operState,
                &operState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.readinessState,
                &readinessState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.numActiveSIs,
                &numActiveSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.numStandbySIs,
                &numStandbySIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.compRestartCount,
                &compRestartCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.suRestartCount,
                &suRestartCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.numInstantiatedComp,
                &numInstantiatedComp) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.numQuiescedSIs,
                &numQuiescedSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.recovery,
                &recovery) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                su->status.numPIComp,
                &numPIComp) );

    CL_PARSER_SET_ATTR (
            statusPtr,
            "presenceState",
            presenceState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "operState",
            operState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "readinessState",
            readinessState);
    
    CL_PARSER_SET_ATTR (
            statusPtr,
            "numActiveSIs",
            numActiveSIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numStandbySIs",
            numStandbySIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "compRestartCount",
            compRestartCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "suRestartCount",
            suRestartCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numInstantiatedComp",
            numInstantiatedComp);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numQuiescedSIs",
            numQuiescedSIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "recovery",
            recovery);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numPIComp",
            numPIComp);

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusPtr,
                &su->status.compRestartTimer,
                "compRestartTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusPtr,
                &su->status.suRestartTimer,
                "suRestartTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &su->status.siList,
                CL_AMS_SU_STATUS_SI_LIST) );

exitfn:

    clAmsFreeMemory (adminState);
    clAmsFreeMemory (rank);
    clAmsFreeMemory (numComponents);
    clAmsFreeMemory (isPreinstantiable);
    clAmsFreeMemory (isRestartable);
    clAmsFreeMemory (isContainerSU);
    clAmsFreeMemory (presenceState);
    clAmsFreeMemory (operState);
    clAmsFreeMemory (readinessState);
    clAmsFreeMemory (numActiveSIs);
    clAmsFreeMemory (numStandbySIs);
    clAmsFreeMemory (compRestartCount);
    clAmsFreeMemory (suRestartCount);
    clAmsFreeMemory (numInstantiatedComp);
    clAmsFreeMemory (numQuiescedSIs);
    clAmsFreeMemory (recovery);
    clAmsFreeMemory (numPIComp);

    return CL_AMS_RC (rc);

}

static ClRcT   
clAmsDBSUUnmarshall(ClAmsEntityT *entity,
                    ClBufferHandleT inMsgHdl,
                    ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = 0;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsSUT *su = NULL;
    
    AMS_CHECK_ENTITY_TYPE(entity->type);
    
    if(entity->type != CL_AMS_ENTITY_TYPE_SU)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SU unmarshall called with invalid entity [%d]\n", 
                                 entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                 &entityRef);

    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("SU entitydb operation returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    su = (ClAmsSUT*)entityRef.ptr;

    CL_ASSERT(su != NULL);

    for(;;)
    {
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));

        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsSUConfigT suConfig = {{0}};
                ClAmsEntityRefT targetEntityRef = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSUConfigT, 4, 0, 0)(inMsgHdl, &suConfig));
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &suConfig.entity));

                memcpy(&targetEntityRef.entity, &suConfig.parentSG.entity, 
                       sizeof(targetEntityRef.entity));
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);

                memcpy(&targetEntityRef.entity, &suConfig.parentNode.entity, 
                       sizeof(targetEntityRef.entity));
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);
                break;
            }
            
        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsSUStatusT suStatus = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSUStatusT, 4, 0, 0)(inMsgHdl, &suStatus));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &suStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &suStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }
            
        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP:
            return CL_OK;

        default: break;
        }

    }

    exitfn:
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBSUDeXMLize(
       CL_IN  ClParserPtrT  suPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;

    AMS_CHECKPTR (!suPtr);

    name = clParserAttr(
            suPtr,
            AMS_XML_TAG_NAME);

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,("su Instance does not have name attribute \n"));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }
            
    ClParserPtrT configPtr = clParserChild( suPtr,AMS_XML_TAG_CONFIG );
    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SU[%s] does not have config tag \n", name));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    const ClCharT  *adminState = NULL;
    const ClCharT  *rank = NULL;
    const ClCharT  *numComponents = NULL;
    const ClCharT  *isPreinstantiable = NULL;
    const ClCharT  *isRestartable = NULL;
    const ClCharT  *isContainerSU = NULL;

    adminState = clParserAttr (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE);

    rank = clParserAttr (
            configPtr,
            "rank");

    numComponents = clParserAttr (
            configPtr,
            "numComponents");

    isPreinstantiable = clParserAttr (
            configPtr,
            "isPreinstantiable");

    isRestartable = clParserAttr (
            configPtr,
            "isRestartable");

    isContainerSU = clParserAttr (
            configPtr,
            "isContainerSU");

    if ( !adminState || !rank || !numComponents || !isPreinstantiable 
            || !isRestartable || !isContainerSU )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SU[%s] has a missing config attribute \n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    const ClCharT  *presenceState = NULL;
    const ClCharT  *operState = NULL;
    const ClCharT  *readinessState = NULL;
    const ClCharT  *numActiveSIs = NULL;
    const ClCharT  *numStandbySIs = NULL;
    const ClCharT  *compRestartCount = NULL;
    const ClCharT  *suRestartCount = NULL;
    const ClCharT  *numInstantiatedComp = NULL;
    const ClCharT  *numQuiescedSIs = NULL;
    const ClCharT  *recovery = NULL;
    const ClCharT  *numPIComp = NULL;

    ClParserPtrT statusPtr = clParserChild( suPtr,AMS_XML_TAG_STATUS );
    if ( !statusPtr)
    {
        AMS_LOG (CL_DEBUG_ERROR,("SU[%s] does not have status tag \n", name));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    presenceState = clParserAttr (
            statusPtr,
            "presenceState");

    operState = clParserAttr (
            statusPtr,
            "operState");

    readinessState = clParserAttr (
            statusPtr,
            "readinessState");

    numActiveSIs = clParserAttr (
            statusPtr,
            "numActiveSIs");

    numStandbySIs = clParserAttr (
            statusPtr,
            "numStandbySIs");

    compRestartCount = clParserAttr (
            statusPtr,
            "compRestartCount");

    suRestartCount = clParserAttr (
            statusPtr,
            "suRestartCount");

    numInstantiatedComp = clParserAttr (
            statusPtr,
            "numInstantiatedComp");

    numQuiescedSIs = clParserAttr (
            statusPtr,
            "numQuiescedSIs");

    recovery = clParserAttr (
            statusPtr,
            "recovery");

    numPIComp = clParserAttr (
            statusPtr,
            "numPIComp");

    if ( !presenceState || !operState || !readinessState || !numActiveSIs 
            || !numStandbySIs || !compRestartCount || !suRestartCount 
            || !numInstantiatedComp || !numQuiescedSIs || !recovery
            || !numPIComp )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SU[%s] status has a missing attribute \n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT entityRef = { {0},0,0};
    ClAmsSUT      su;
    
    memset (&entityRef,0,sizeof (ClAmsEntityRefT));
    memset (&su,0,sizeof (ClAmsSUT));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_SU;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                &entityRef) );

    /*
     * Write the config part
     */

    memcpy (&su.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    su.config.adminState = atoi (adminState);
    su.config.rank= atoi (rank);
    su.config.numComponents= atoi (numComponents);
    su.config.isPreinstantiable= atoi (isPreinstantiable);
    su.config.isRestartable= atoi (isRestartable);
    su.config.isContainerSU= atoi (isContainerSU);

    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&su.config) );

    su.status.presenceState= atoi (presenceState);
    su.status.operState= atoi (operState);
    su.status.readinessState= atoi (readinessState);
    su.status.numActiveSIs= atoi (numActiveSIs);
    su.status.numStandbySIs= atoi (numStandbySIs);
    su.status.compRestartCount= atoi (compRestartCount);
    su.status.suRestartCount= atoi (suRestartCount);
    su.status.numInstantiatedComp= atoi (numInstantiatedComp);
    su.status.numQuiescedSIs= atoi (numQuiescedSIs);
    su.status.recovery= atoi (recovery);
    su.status.numPIComp= atoi (numPIComp);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &su.status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&su.status) );

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBSUListDeXMLize(
       CL_IN  ClParserPtrT  suPtr)
{
    ClRcT rc = CL_OK;
    const ClCharT  *name = NULL;
    ClAmsEntityTimerT entityTimer = {0};

    name = clParserAttr (
            suPtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("suName tag does not have node name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
   
    ClParserPtrT configPtr = clParserChild(suPtr,AMS_XML_TAG_CONFIG);
    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("SUName[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
    
    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SU_CONFIG_COMP_LIST) );

    const ClCharT  *parentSG = NULL;
    const ClCharT  *parentNode = NULL;

    parentSG = clParserAttr (
            configPtr,
            "parentSG");

    parentNode = clParserAttr (
            configPtr,
            "parentNode");

    if ( !parentSG|| !parentNode )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("SU[%s] has a missing config attribute \n",
                    name));
        goto exitfn;
    }

    ClAmsEntityRefT  suRef = {{0},0,0};
    ClAmsEntityRefT  sgRef = {{0},0,0};
    ClAmsEntityRefT  nodeRef = {{0},0,0};

    memset (&suRef,0,sizeof (ClAmsEntityRefT));
    memset (&sgRef,0,sizeof (ClAmsEntityRefT));
    memset (&nodeRef,0,sizeof (ClAmsEntityRefT));

    suRef.entity.type = CL_AMS_ENTITY_TYPE_SU;
    sgRef.entity.type = CL_AMS_ENTITY_TYPE_SG;
    nodeRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;

    strcpy (suRef.entity.name.value,name);
    strcpy (sgRef.entity.name.value,parentSG);
    strcpy (nodeRef.entity.name.value,parentNode);
    suRef.entity.name.length = strlen(name) + 1 ;
    sgRef.entity.name.length = strlen(parentSG) + 1 ;
    nodeRef.entity.name.length = strlen(parentNode) + 1 ;

    AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                suRef,
                sgRef) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                suRef,
                nodeRef) );

    ClParserPtrT  statusPtr = clParserChild(suPtr,AMS_XML_TAG_STATUS);

    if ( !statusPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("SU[%s] does not have status tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    /*
     * Read the compRestartTimer and suRestartTimer
     */

    entityTimer.count = 0;
    entityTimer.type = CL_AMS_SU_TIMER_COMPRESTART;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusPtr,
                &entityTimer,
                "compRestartTimer") );

    ClAmsEntityT  entity = {0};
    memset (&entity,0,sizeof (ClAmsEntityT));
    strcpy ( entity.name.value, name);
    entity.name.length = strlen (name) + 1;
    entity.type = CL_AMS_ENTITY_TYPE_SU;

    AMS_CHECK_RC_ERROR ( clAmsSetEntityTimer(
                &entity,
                &entityTimer) );

    entityTimer.count = 0;
    entityTimer.type = CL_AMS_SU_TIMER_SURESTART;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusPtr,
                &entityTimer,
                "suRestartTimer") );

    memset (&entity,0,sizeof (ClAmsEntityT));
    strcpy ( entity.name.value, name);
    entity.name.length = strlen (name) + 1;
    entity.type = CL_AMS_ENTITY_TYPE_SU;

    AMS_CHECK_RC_ERROR ( clAmsSetEntityTimer(
                &entity,
                &entityTimer) );

    ClParserPtrT  listPtr = clParserChild( statusPtr,AMS_XML_TAG_SI_LIST);

    if ( !listPtr)
    {
        return CL_OK;
    }


    ClParserPtrT  list2Ptr = clParserChild( listPtr,AMS_XML_TAG_SI);

    if ( !list2Ptr)
    {
        return CL_OK;
    }

    ClAmsEntityRefT  sourceEntityRef = { {0},0,0};
    const ClCharT  *numActiveCSIs = NULL;
    const ClCharT  *numStandbyCSIs = NULL;
    const ClCharT  *haState = NULL;
    const ClCharT  *numQuiescedCSIs = NULL;
    const ClCharT  *numQuiescingCSIs = NULL;
    const ClCharT  *siName = NULL;

    memset (&sourceEntityRef,0,sizeof (ClAmsEntityRefT));
    strcpy (sourceEntityRef.entity.name.value, name);
    sourceEntityRef.entity.name.length = strlen (name) + 1;
    sourceEntityRef.entity.type = CL_AMS_ENTITY_TYPE_SU;

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                &sourceEntityRef) );

    ClAmsSUT  *su = (ClAmsSUT *)sourceEntityRef.ptr;

    AMS_CHECKPTR_AND_EXIT ( !su );

    while ( list2Ptr )
    {

        ClAmsSUSIRefT *siRef = NULL;

        siName = clParserAttr (
                list2Ptr,
                AMS_XML_TAG_NAME );

        haState = clParserAttr (
                list2Ptr,
                "haState");

        numActiveCSIs  = clParserAttr (
                list2Ptr,
                "numActiveCSIs");

        numStandbyCSIs  = clParserAttr (
                list2Ptr,
                "numStandbyCSIs");

        numQuiescedCSIs  = clParserAttr (
                list2Ptr,
                "numQuiescedCSIs");

        numQuiescingCSIs  = clParserAttr (
                list2Ptr,
                "numQuiescingCSIs");

        AMS_CHECKPTR_AND_EXIT ( !siName|| !haState || !numActiveCSIs || !numActiveCSIs 
                || !numQuiescedCSIs || !numQuiescingCSIs );

        siRef = clHeapAllocate (sizeof (ClAmsSUSIRefT));

        AMS_CHECK_NO_MEMORY (siRef);

        strcpy (siRef->entityRef.entity.name.value,siName);
        siRef->entityRef.entity.name.length = strlen (siName) + 1;
        siRef->entityRef.entity.type = CL_AMS_ENTITY_TYPE_SI;

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                    &siRef->entityRef) );

        siRef->haState = atoi (haState);
        siRef->numActiveCSIs = atoi (numActiveCSIs);
        siRef->numStandbyCSIs = atoi (numStandbyCSIs);
        siRef->numQuiescedCSIs = atoi (numQuiescedCSIs);
        siRef->numQuiescingCSIs = atoi (numQuiescingCSIs);

        AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                    &su->status.siList,
                    (ClAmsEntityRefT *)siRef,
                    0) );

        numActiveCSIs = NULL;
        haState = NULL;
        numStandbyCSIs= NULL;
        numQuiescedCSIs= NULL;
        numQuiescingCSIs= NULL;

        list2Ptr = list2Ptr->next;

    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBCompMarshall(ClAmsCompT *comp, ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;

    AMS_CHECKPTR (!comp);
    
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&comp->config.entity, inMsgHdl, 0));

    if( (marshallMask & CL_AMS_ENTITY_DB_CONFIG))
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;

        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCompConfigT, 4, 0, 0)(&comp->config, inMsgHdl, 0));
    }

    if((marshallMask & CL_AMS_ENTITY_DB_STATUS))
    {
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;
    
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCompStatusT, 4, 0, 0)(&comp->status, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(clAmsDBEntityOpMarshall(&comp->config.entity, &comp->status.entity, inMsgHdl, versionCode));

        if ( comp->status.proxyComp )
        {
            op = CL_AMS_CKPT_OPERATION_SET_PROXY_COMP;

            AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
        
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(comp->status.proxyComp, inMsgHdl, 0));

        }

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.instantiate,
                                                  CL_AMS_COMP_TIMER_INSTANTIATE,
                                                  inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.instantiateDelay,
                                                  CL_AMS_COMP_TIMER_INSTANTIATEDELAY,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.terminate,
                                                  CL_AMS_COMP_TIMER_TERMINATE,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.cleanup,
                                                  CL_AMS_COMP_TIMER_CLEANUP,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.amStart,
                                                  CL_AMS_COMP_TIMER_AMSTART,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.amStop,
                                                  CL_AMS_COMP_TIMER_AMSTOP,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.quiescingComplete,
                                                  CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.csiSet,
                                                  CL_AMS_COMP_TIMER_CSISET,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.csiRemove,
                                                  CL_AMS_COMP_TIMER_CSIREMOVE,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.proxiedCompInstantiate,
                                                  CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &comp->status.timers.proxiedCompCleanup,
                                                  CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &comp->status.csiList,
                                                 CL_AMS_COMP_STATUS_CSI_LIST,
                                                 inMsgHdl, versionCode) );

    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;
    
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    
    exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBCompXMLize(
       CL_IN  ClAmsCompT  *comp)
{
    ClRcT rc = CL_OK;

    ClParserPtrT compPtr = 
        CL_PARSER_ADD_CHILD(amfPtr.compNamesPtr,AMS_XML_TAG_COMP_NAME, 0);

    AMS_CHECKPTR (!comp);

    CL_PARSER_SET_ATTR(
            compPtr,
            AMS_XML_TAG_NAME,
            comp->config.entity.name.value);

    ClParserPtrT configPtr = CL_PARSER_ADD_CHILD( compPtr,AMS_XML_TAG_CONFIG , 0);

    AMS_CHECKPTR_AND_EXIT (!configPtr);

    ClCharT  *capabilityModel = NULL;
    ClCharT  *property = NULL;
    ClCharT  *isRestartable = NULL;
    ClCharT  *nodeRebootCleanupFail = NULL;
    ClCharT  *instantiateLevel = NULL;
    ClCharT  *numMaxInstantiate = NULL;
    ClCharT  *numMaxInstantiateWithDelay = NULL;
    ClCharT  *numMaxTerminate = NULL;
    ClCharT  *numMaxAmStart = NULL;
    ClCharT  *numMaxAmStop = NULL;
    ClCharT  *numMaxActiveCSIs = NULL;
    ClCharT  *numMaxStandbyCSIs = NULL;
    ClCharT  *recoveryOnTimeout = NULL;
    ClCharT  *numSupportedCSITypes = NULL;
    ClUint32T i;

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.capabilityModel,
                &capabilityModel) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.property,
                &property) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.isRestartable,
                &isRestartable) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.nodeRebootCleanupFail,
                &nodeRebootCleanupFail) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.instantiateLevel,
                &instantiateLevel) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxInstantiate,
                &numMaxInstantiate) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxInstantiateWithDelay,
                &numMaxInstantiateWithDelay) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxTerminate,
                &numMaxTerminate) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxAmStart,
                &numMaxAmStart) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxAmStop,
                &numMaxAmStop) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxActiveCSIs,
                &numMaxActiveCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numMaxStandbyCSIs,
                &numMaxStandbyCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.recoveryOnTimeout,
                &recoveryOnTimeout) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->config.numSupportedCSITypes,
                &numSupportedCSITypes) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "numSupportedCSITypes",
            numSupportedCSITypes);

    ClParserPtrT csiTypeInstances = CL_PARSER_ADD_CHILD(configPtr,
                                                        AMS_XML_TAG_CSI_TYPES,
                                                        0);
    AMS_CHECKPTR_AND_EXIT(!csiTypeInstances);

    for(i = 0; i < comp->config.numSupportedCSITypes; ++i)
    {
        ClParserPtrT csiTypeInstance = CL_PARSER_ADD_CHILD(csiTypeInstances,
                                                           AMS_XML_TAG_CSI_TYPE,
                                                           0);
        
        AMS_CHECKPTR_AND_EXIT(!csiTypeInstance);

        CL_PARSER_SET_ATTR(csiTypeInstance, 
                           AMS_XML_TAG_NAME,
                           comp->config.pSupportedCSITypes[i].value);
    }

    CL_PARSER_SET_ATTR (
            configPtr,
            "proxyCSIType",
            comp->config.proxyCSIType.value);

    CL_PARSER_SET_ATTR (
            configPtr,
            "capabilityModel",
            capabilityModel);

    CL_PARSER_SET_ATTR (
            configPtr,
            "property",
            property);

    CL_PARSER_SET_ATTR (
            configPtr,
            "isRestartable",
            isRestartable);

    CL_PARSER_SET_ATTR (
            configPtr,
            "nodeRebootCleanupFail",
            nodeRebootCleanupFail);

    CL_PARSER_SET_ATTR (
            configPtr,
            "instantiateLevel",
            instantiateLevel);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxInstantiate",
            numMaxInstantiate);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxInstantiateWithDelay",
            numMaxInstantiateWithDelay);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxTerminate",
            numMaxTerminate);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxAmStart",
            numMaxAmStart);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxAmStop",
            numMaxAmStop);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxActiveCSIs",
            numMaxActiveCSIs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numMaxStandbyCSIs",
            numMaxStandbyCSIs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "recoveryOnTimeout",
            recoveryOnTimeout);

    CL_PARSER_SET_ATTR (
            configPtr,
            "parentSU",
            comp->config.parentSU.entity.name.value);

    ClParserPtrT timeoutsPtr = CL_PARSER_ADD_CHILD(configPtr,"timeouts", 0);

    AMS_CHECKPTR_AND_EXIT ( !timeoutsPtr );

    ClCharT  *instantiateTmOut = NULL;
    ClCharT  *instantiateDelayTmOut = NULL;
    ClCharT  *terminateTmOut= NULL;
    ClCharT  *cleanupTmOut= NULL;
    ClCharT  *amStartTmOut= NULL;
    ClCharT  *amStopTmOut= NULL;
    ClCharT  *quiescingCompleteTmOut= NULL;
    ClCharT  *csiSetTmout= NULL;
    ClCharT  *csiRemoveTmOut= NULL;
    ClCharT  *proxiedCompInstantiateTmOut= NULL;
    ClCharT  *proxiedCompCleanupTmOut= NULL;

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.instantiate,
                &instantiateTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.instantiateDelay,
                &instantiateDelayTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.terminate,
                &terminateTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.cleanup,
                &cleanupTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.amStart,
                &amStartTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.amStop,
                &amStopTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.quiescingComplete,
                &quiescingCompleteTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.csiSet,
                &csiSetTmout) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.csiRemove,
                &csiRemoveTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.proxiedCompCleanup,
                &proxiedCompCleanupTmOut) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                comp->config.timeouts.proxiedCompInstantiate,
                &proxiedCompInstantiateTmOut) );

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "instantiate",
            instantiateTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "instantiateDelay",
            instantiateDelayTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "terminate",
            terminateTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "cleanup",
            cleanupTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "amStart",
            amStartTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "amStop",
            amStopTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "csiSet",
            csiSetTmout);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "csiRemove",
            csiRemoveTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "proxiedCompInstantiate",
            proxiedCompInstantiateTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "proxiedCompCleanup",
            proxiedCompCleanupTmOut);

    CL_PARSER_SET_ATTR (
            timeoutsPtr,
            "quiescingComplete",
            quiescingCompleteTmOut);

    ClParserPtrT statusPtr = CL_PARSER_ADD_CHILD( compPtr,AMS_XML_TAG_STATUS , 0);

    AMS_CHECKPTR_AND_EXIT ( !statusPtr );

    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &comp->status.entity,
                statusPtr) );

    ClCharT  *presenceState = NULL;
    ClCharT  *operState = NULL;
    ClCharT  *readinessState = NULL;
    ClCharT  *recovery = NULL;
    ClCharT  *numActiveCSIs = NULL;
    ClCharT  *numStandbyCSIs = NULL;
    ClCharT  *restartCount = NULL;
    ClCharT  *instantiateCount = NULL;
    ClCharT  *instantiateDelayCount = NULL;
    ClCharT  *amStartCount = NULL;
    ClCharT  *amStopCount = NULL;
    ClCharT  *alarmHandle = NULL;
    ClCharT  *numQuiescingCSIs = NULL;
    ClCharT  *numQuiescedCSIs = NULL;


    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.presenceState,
                &presenceState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.operState,
                &operState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.readinessState,
                &readinessState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.recovery,
                &recovery) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.numActiveCSIs, 
                &numActiveCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.numStandbyCSIs,
                &numStandbyCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.restartCount,
                &restartCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.instantiateCount,
                &instantiateCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.instantiateDelayCount,
                &instantiateDelayCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr( 
                comp->status.amStartCount,
                &amStartCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.amStopCount,
                &amStopCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.alarmHandle,
                &alarmHandle) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.numQuiescingCSIs,
                &numQuiescingCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                comp->status.numQuiescedCSIs,
                &numQuiescedCSIs) );

    CL_PARSER_SET_ATTR (
            statusPtr,
            "presenceState",
            presenceState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "readinessState",
            readinessState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "operState",
            operState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "recovery",
            recovery);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numActiveCSIs",
            numActiveCSIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numStandbyCSIs",
            numStandbyCSIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "restartCount",
            restartCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "instantiateCount",
            instantiateCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "instantiateDelayCount",
            instantiateDelayCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "amStartCount",
            amStartCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "amStopCount",
            amStopCount);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "alarmHandle",
            alarmHandle);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numQuiescingCSIs",
            numQuiescingCSIs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numQuiescedCSIs",
            numQuiescedCSIs);

    if ( comp->status.proxyComp )
    {
        CL_PARSER_SET_ATTR (
                statusPtr,
                "proxyComp",
                comp->status.proxyComp->name.value);
    }
    else
    {
        CL_PARSER_SET_ATTR (
                statusPtr,
                "proxyComp",
                "");
    }

    ClParserPtrT statusTimeoutsPtr = CL_PARSER_ADD_CHILD(statusPtr,"timeouts", 0);

    AMS_CHECKPTR_AND_EXIT ( !statusTimeoutsPtr);

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.instantiate,
                "instantiateTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.instantiateDelay,
                "instantiateDelayTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.terminate,
                "terminateTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.cleanup,
                "cleanupTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.amStart,
                "amStartTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.amStop,
                "amStopTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.quiescingComplete,
                "quiescingCompleteTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.csiSet,
                "csiSetTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.csiRemove,
                "csiRemoveTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.proxiedCompInstantiate,
                "proxiedCompInstantiateTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusTimeoutsPtr,
                &comp->status.timers.proxiedCompCleanup,
                "proxiedCompCleanupTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &comp->status.csiList,
                CL_AMS_COMP_STATUS_CSI_LIST) );

exitfn:

    clAmsFreeMemory(capabilityModel);
    clAmsFreeMemory(property);
    clAmsFreeMemory(isRestartable);
    clAmsFreeMemory(nodeRebootCleanupFail);
    clAmsFreeMemory(instantiateLevel);
    clAmsFreeMemory(numMaxInstantiate);
    clAmsFreeMemory(numMaxInstantiateWithDelay);
    clAmsFreeMemory(numMaxTerminate);
    clAmsFreeMemory(numMaxAmStart);
    clAmsFreeMemory(numMaxAmStop);
    clAmsFreeMemory(numMaxActiveCSIs);
    clAmsFreeMemory(numMaxStandbyCSIs);
    clAmsFreeMemory(recoveryOnTimeout);
    clAmsFreeMemory(instantiateTmOut) ;
    clAmsFreeMemory(instantiateDelayTmOut);
    clAmsFreeMemory(terminateTmOut);
    clAmsFreeMemory(cleanupTmOut);
    clAmsFreeMemory(amStartTmOut);
    clAmsFreeMemory(amStopTmOut);
    clAmsFreeMemory(quiescingCompleteTmOut);
    clAmsFreeMemory(csiSetTmout);
    clAmsFreeMemory(csiRemoveTmOut);
    clAmsFreeMemory(proxiedCompInstantiateTmOut);
    clAmsFreeMemory(proxiedCompCleanupTmOut);
    clAmsFreeMemory(presenceState);
    clAmsFreeMemory(operState);
    clAmsFreeMemory(readinessState);
    clAmsFreeMemory(recovery);
    clAmsFreeMemory(numActiveCSIs);
    clAmsFreeMemory(numStandbyCSIs);
    clAmsFreeMemory(restartCount);
    clAmsFreeMemory(instantiateCount);
    clAmsFreeMemory(instantiateDelayCount);
    clAmsFreeMemory(amStartCount);
    clAmsFreeMemory(amStopCount);
    clAmsFreeMemory(alarmHandle);
    clAmsFreeMemory(numQuiescingCSIs);
    clAmsFreeMemory(numQuiescedCSIs);
    clAmsFreeMemory(numSupportedCSITypes);

    return CL_AMS_RC (rc);

}

static ClRcT   
clAmsDBCompUnmarshall(ClAmsEntityT *entity,
                      ClBufferHandleT inMsgHdl,
                      ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsCkptOperationT op = 0;
    ClAmsCompT *comp = NULL;

    if(entity->type != CL_AMS_ENTITY_TYPE_COMP)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Comp unmarshall invoked with invalid entity type [%d]\n",
                                 entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Comp entitydb operation returned [%#x]\n",
                                     rc));
            goto exitfn;
        }
    }

    comp = (ClAmsCompT*)entityRef.ptr;

    for(;;)
    {
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));
        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsCompConfigT compConfig = {{0}};
                ClAmsEntityRefT targetEntityRef = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCompConfigT, 4, 0, 0)(inMsgHdl, &compConfig));
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &compConfig.entity));
                memcpy(&targetEntityRef.entity, &compConfig.parentSU.entity, 
                       sizeof(targetEntityRef.entity));
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsCompStatusT compStatus = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCompStatusT, 4, 0, 0)(inMsgHdl, &compStatus));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &compStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &compStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_PROXY_COMP:
            {
                ClAmsEntityRefT targetEntityRef = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, 
                                                                     &targetEntityRef.entity));
                /*
                 * Set the proxy reference to the comp.
                 */
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP:
            return CL_OK;

        default: break;
        }
    }

    exitfn:
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBCompDeXMLize(
       CL_IN  ClParserPtrT  compPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;

    AMS_CHECKPTR (!compPtr);

    name = clParserAttr(
            compPtr,
            AMS_XML_TAG_NAME);

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,("comp Instance does not have name attribute \n"));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    ClParserPtrT configPtr = clParserChild( compPtr,AMS_XML_TAG_CONFIG );

    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,("comp Instance %s does not have config tag \n", name));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    const ClCharT  *proxyCSIType = NULL;
    const ClCharT  *capabilityModel = NULL;
    const ClCharT  *property = NULL;
    const ClCharT  *isRestartable = NULL;
    const ClCharT  *nodeRebootCleanupFail = NULL;
    const ClCharT  *instantiateLevel = NULL;
    const ClCharT  *numMaxInstantiate = NULL;
    const ClCharT  *numMaxInstantiateWithDelay = NULL;
    const ClCharT  *numMaxTerminate = NULL;
    const ClCharT  *numMaxAmStart = NULL;
    const ClCharT  *numMaxAmStop = NULL;
    const ClCharT  *numMaxActiveCSIs = NULL;
    const ClCharT  *numMaxStandbyCSIs = NULL;
    const ClCharT  *recoveryOnTimeout = NULL;
    const ClCharT  *pNumSupportedCSITypes = NULL;
    ClNameT *pSupportedCSITypes = NULL;
    ClUint32T numSupportedCSITypes = 0;
    ClUint32T i;
    ClParserPtrT  csiTypeInstances = NULL;
    ClParserPtrT  csiTypeInstance = NULL;

    pNumSupportedCSITypes = clParserAttr(configPtr, "numSupportedCSITypes");
    AMS_CHECKPTR_AND_EXIT(!pNumSupportedCSITypes);

    numSupportedCSITypes = atoi(pNumSupportedCSITypes);
    csiTypeInstances = clParserChild(configPtr, AMS_XML_TAG_CSI_TYPES);

    AMS_CHECKPTR_AND_EXIT(!csiTypeInstances);

    csiTypeInstance = clParserChild(csiTypeInstances, AMS_XML_TAG_CSI_TYPE);

    AMS_CHECKPTR_AND_EXIT(!csiTypeInstance);
    
    pSupportedCSITypes = clHeapCalloc(numSupportedCSITypes,
                                      sizeof(ClNameT));

    AMS_CHECKPTR_AND_EXIT(!pSupportedCSITypes);

    for(i = 0; i < numSupportedCSITypes && csiTypeInstance; ++i) 
    {
        const ClCharT *pData = clParserAttr(csiTypeInstance, AMS_XML_TAG_NAME);
        AMS_CHECKPTR_AND_EXIT(!pData);

        pSupportedCSITypes[i].length = 
            CL_MIN(strlen(pData)+1, CL_MAX_NAME_LENGTH-1);
        strncpy(pSupportedCSITypes[i].value, pData, CL_MAX_NAME_LENGTH-1);
        csiTypeInstance = csiTypeInstance->next;
    }

    if(csiTypeInstance != NULL ||
       i != numSupportedCSITypes)
    {
        AMS_LOG(CL_DEBUG_WARN,
                ("Comp config. numSupportedCSIType mismatch. "\
                 "Expected [%d], Got [%d]\n", numSupportedCSITypes, i));
    }

    proxyCSIType = clParserAttr(
            configPtr,
            "proxyCSIType");

    capabilityModel = clParserAttr (
            configPtr,
            "capabilityModel");
    property = clParserAttr (
            configPtr,
            "property");

    isRestartable = clParserAttr (
            configPtr,
            "isRestartable");

    nodeRebootCleanupFail = clParserAttr (
            configPtr,
            "nodeRebootCleanupFail");

    instantiateLevel = clParserAttr (
            configPtr,
            "instantiateLevel");

    numMaxInstantiate = clParserAttr (
            configPtr,
            "numMaxInstantiate");

    numMaxInstantiateWithDelay = clParserAttr (
            configPtr,
            "numMaxInstantiateWithDelay");

    numMaxTerminate = clParserAttr (
            configPtr,
            "numMaxTerminate");

    numMaxAmStart = clParserAttr (
            configPtr,
            "numMaxAmStart");

    numMaxAmStop = clParserAttr (
            configPtr,
            "numMaxAmStop");

    numMaxActiveCSIs = clParserAttr (
            configPtr,
            "numMaxActiveCSIs");

    numMaxStandbyCSIs = clParserAttr (
            configPtr,
            "numMaxStandbyCSIs");

    recoveryOnTimeout = clParserAttr (
            configPtr,
            "recoveryOnTimeout");

    if (   !capabilityModel || !property || !isRestartable
           || !nodeRebootCleanupFail || !instantiateLevel 
           || !numMaxInstantiate || !numMaxInstantiateWithDelay 
           || !numMaxTerminate || !numMaxAmStart || !numMaxAmStop 
           || !numMaxActiveCSIs || !numMaxStandbyCSIs || !recoveryOnTimeout
           || !proxyCSIType )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("COMP[%s] has a missing config attribute \n",
                                 name));
        goto exitfn;
    }

    ClParserPtrT timeoutsPtr = clParserChild(configPtr,"timeouts");

    AMS_CHECKPTR_AND_EXIT ( !timeoutsPtr );

    const ClCharT  *instantiateTmOut = NULL;
    const ClCharT  *instantiateDelayTmOut = NULL;
    const ClCharT  *terminateTmOut = NULL;
    const ClCharT  *cleanupTmOut = NULL;
    const ClCharT  *amStartTmOut = NULL;
    const ClCharT  *amStopTmOut = NULL;
    const ClCharT  *quiescingCompleteTmOut = NULL;
    const ClCharT  *csiSetTmout = NULL;
    const ClCharT  *csiRemoveTmOut = NULL;
    const ClCharT  *proxiedCompInstantiateTmOut = NULL;
    const ClCharT  *proxiedCompCleanupTmOut = NULL;

    instantiateTmOut = clParserAttr (
            timeoutsPtr,
            "instantiate");

    instantiateDelayTmOut = clParserAttr (
            timeoutsPtr,
            "instantiateDelay");

    terminateTmOut = clParserAttr (
            timeoutsPtr,
            "terminate");

    cleanupTmOut = clParserAttr (
            timeoutsPtr,
            "cleanup");

    amStartTmOut = clParserAttr (
            timeoutsPtr,
            "amStart");

    amStopTmOut = clParserAttr (
            timeoutsPtr,
            "amStop");

    csiSetTmout = clParserAttr (
            timeoutsPtr,
            "csiSet");

    csiRemoveTmOut = clParserAttr (
            timeoutsPtr,
            "csiRemove");

    proxiedCompInstantiateTmOut = clParserAttr (
            timeoutsPtr,
            "proxiedCompInstantiate");

    proxiedCompCleanupTmOut = clParserAttr (
            timeoutsPtr,
            "proxiedCompCleanup");

    quiescingCompleteTmOut = clParserAttr (
            timeoutsPtr,
            "quiescingComplete");


    if ( !instantiateTmOut || !instantiateDelayTmOut || 
         !terminateTmOut || !cleanupTmOut || 
         !amStartTmOut || !amStopTmOut || 
         !quiescingCompleteTmOut || !csiSetTmout || !csiRemoveTmOut || 
         !proxiedCompInstantiateTmOut || !proxiedCompCleanupTmOut )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("COMP[%s] has a missing status timer attribute \n",
                    name));
        goto exitfn;

    }

    ClParserPtrT statusPtr = clParserChild( compPtr,AMS_XML_TAG_STATUS );

    AMS_CHECKPTR_AND_EXIT ( !statusPtr );

    const ClCharT  *presenceState = NULL;
    const ClCharT  *operState = NULL;
    const ClCharT  *readinessState = NULL;
    const ClCharT  *recovery = NULL;
    const ClCharT  *numActiveCSIs = NULL;
    const ClCharT  *numStandbyCSIs = NULL;
    const ClCharT  *restartCount = NULL;
    const ClCharT  *instantiateCount = NULL;
    const ClCharT  *instantiateDelayCount = NULL;
    const ClCharT  *amStartCount = NULL;
    const ClCharT  *amStopCount = NULL;
    const ClCharT  *alarmHandle = NULL;
    const ClCharT  *numQuiescingCSIs = NULL;
    const ClCharT  *numQuiescedCSIs = NULL;

    presenceState = clParserAttr (
            statusPtr,
            "presenceState");

    readinessState = clParserAttr (
            statusPtr,
            "readinessState");

    operState = clParserAttr (
            statusPtr,
            "operState");

    recovery = clParserAttr (
            statusPtr,
            "recovery");

    numActiveCSIs = clParserAttr (
            statusPtr,
            "numActiveCSIs");

    numStandbyCSIs = clParserAttr (
            statusPtr,
            "numStandbyCSIs");

    restartCount = clParserAttr (
            statusPtr,
            "restartCount");

    instantiateCount = clParserAttr (
            statusPtr,
            "instantiateCount");

    instantiateDelayCount = clParserAttr (
            statusPtr,
            "instantiateDelayCount");

    amStartCount = clParserAttr (
            statusPtr,
            "amStartCount");

    amStopCount = clParserAttr (
            statusPtr,
            "amStopCount");

    alarmHandle = clParserAttr (
            statusPtr,
            "alarmHandle");

    numQuiescingCSIs = clParserAttr (
            statusPtr,
            "numQuiescingCSIs");

    numQuiescedCSIs = clParserAttr (
            statusPtr,
            "numQuiescedCSIs");

    if ( !presenceState || !operState || !readinessState || !recovery
            || !numActiveCSIs || !numStandbyCSIs || !restartCount 
            || !instantiateCount || !instantiateDelayCount || !amStartCount
            || !amStopCount || !alarmHandle || !numQuiescingCSIs 
            || !numQuiescedCSIs )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("COMP[%s] has a missing status attribute \n",
                    name));
        goto exitfn;
    }

    ClAmsEntityRefT  entityRef = { {0},0,0};
    ClAmsCompT      comp;

    memset (&entityRef,0,sizeof (ClAmsEntityRefT));
    memset (&comp,0,sizeof (ClAmsCompT));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &entityRef) );

    /*
     * Write the config part
     */

    memcpy (&comp.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    comp.config.numSupportedCSITypes = numSupportedCSITypes;
    comp.config.pSupportedCSITypes = pSupportedCSITypes;
    strcpy (comp.config.proxyCSIType.value,proxyCSIType);
    comp.config.proxyCSIType.length = strlen (proxyCSIType) + 1;
    comp.config.capabilityModel = atoi (capabilityModel);
    comp.config.property= atoi (property);
    comp.config.isRestartable= atoi (isRestartable);
    comp.config.nodeRebootCleanupFail= atoi (nodeRebootCleanupFail);
    comp.config.instantiateLevel= atoi (instantiateLevel);
    comp.config.numMaxInstantiate= atoi (numMaxInstantiate);
    comp.config.numMaxInstantiateWithDelay= atoi (numMaxInstantiateWithDelay);
    comp.config.numMaxTerminate= atoi (numMaxTerminate);
    comp.config.numMaxAmStart= atoi (numMaxAmStart);
    comp.config.numMaxAmStop= atoi (numMaxAmStop);
    comp.config.numMaxActiveCSIs= atoi (numMaxActiveCSIs);
    comp.config.numMaxStandbyCSIs= atoi (numMaxStandbyCSIs);
    comp.config.recoveryOnTimeout= atoi (recoveryOnTimeout);

    comp.config.timeouts.instantiate = atof(instantiateTmOut);
    comp.config.timeouts.instantiateDelay = atof(instantiateDelayTmOut);
    comp.config.timeouts.terminate = atof(terminateTmOut);
    comp.config.timeouts.cleanup = atof(cleanupTmOut);
    comp.config.timeouts.amStart = atof(amStartTmOut);
    comp.config.timeouts.amStop = atof(amStopTmOut);
    comp.config.timeouts.quiescingComplete = atof(quiescingCompleteTmOut);
    comp.config.timeouts.csiSet = atof(csiSetTmout);
    comp.config.timeouts.csiRemove = atof(csiRemoveTmOut);
    comp.config.timeouts.proxiedCompInstantiate = atof(proxiedCompInstantiateTmOut);
    comp.config.timeouts.proxiedCompCleanup = atof(proxiedCompCleanupTmOut);


    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&comp.config) );

    pSupportedCSITypes = NULL;

    comp.status.presenceState = atoi (presenceState);
    comp.status.operState = atoi (operState);
    comp.status.readinessState = atoi (readinessState);
    comp.status.recovery = atoi (recovery);
    comp.status.numActiveCSIs = atoi(numActiveCSIs);
    comp.status.numStandbyCSIs = atoi(numStandbyCSIs);
    comp.status.restartCount = atoi (restartCount);
    comp.status.instantiateCount = atoi(instantiateCount);
    comp.status.instantiateDelayCount = atoi(instantiateDelayCount);
    comp.status.amStartCount = atoi(amStartCount);
    comp.status.amStopCount = atoi(amStopCount);
    comp.status.alarmHandle = atoi(alarmHandle);
    comp.status.numQuiescingCSIs = atoi(numQuiescingCSIs);
    comp.status.numQuiescedCSIs = atoi(numQuiescedCSIs);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &comp.status.entity,
                statusPtr) );

    ClParserPtrT statusTimeoutsPtr = clParserChild(statusPtr,"timeouts");

    AMS_CHECKPTR_AND_EXIT ( !statusTimeoutsPtr);

    comp.status.timers.instantiate.type = CL_AMS_COMP_TIMER_INSTANTIATE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.instantiate,
                "instantiateTimer") );

    comp.status.timers.instantiateDelay.type = CL_AMS_COMP_TIMER_INSTANTIATEDELAY;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.instantiateDelay,
                "instantiateDelayTimer") );

    comp.status.timers.terminate.type= CL_AMS_COMP_TIMER_TERMINATE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.terminate,
                "terminateTimer") );

    comp.status.timers.cleanup.type= CL_AMS_COMP_TIMER_CLEANUP;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.cleanup,
                "cleanupTimer") );

    comp.status.timers.amStart.type= CL_AMS_COMP_TIMER_AMSTART;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.amStart,
                "amStartTimer") );

    comp.status.timers.amStop.type= CL_AMS_COMP_TIMER_AMSTOP;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.amStop,
                "amStopTimer") );

    comp.status.timers.quiescingComplete.type= CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.quiescingComplete,
                "quiescingCompleteTimer") );

    comp.status.timers.csiSet.type= CL_AMS_COMP_TIMER_CSISET;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.csiSet,
                "csiSetTimer") );

    comp.status.timers.csiRemove.type= CL_AMS_COMP_TIMER_CSIREMOVE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.csiRemove,
                "csiRemoveTimer") );

    comp.status.timers.proxiedCompInstantiate.type = CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.proxiedCompInstantiate,
                "proxiedCompInstantiateTimer") );

    comp.status.timers.proxiedCompCleanup.type= CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusTimeoutsPtr,
                &comp.status.timers.proxiedCompCleanup,
                "proxiedCompCleanupTimer") );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&comp.status) );

    return CL_OK;

exitfn:
    if(pSupportedCSITypes)
        clHeapFree(pSupportedCSITypes);
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBCompListDeXMLize(
       CL_IN  ClParserPtrT  compPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;

    name = clParserAttr (
            compPtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName tag does not have  name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClParserPtrT configPtr = clParserChild(compPtr,AMS_XML_TAG_CONFIG);

    if (!configPtr)
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    const ClCharT  *parentSU = clParserAttr (
            configPtr,
            "parentSU");

    if ( !parentSU )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName[%s] tag does not have parentSU attribute\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT  compRef = {{0},0,0};
    ClAmsEntityRefT  suRef = {{0},0,0};

    memset (&compRef,0,sizeof (ClAmsEntityRefT));
    memset (&suRef,0,sizeof (ClAmsEntityRefT));

    compRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    suRef.entity.type = CL_AMS_ENTITY_TYPE_SU;

    strcpy (compRef.entity.name.value,name);
    strcpy (suRef.entity.name.value,parentSU);
    compRef.entity.name.length = strlen(name) + 1 ;
    suRef.entity.name.length = strlen(parentSU) + 1 ;

    AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                compRef,
                suRef) );

    ClParserPtrT statusPtr = clParserChild(compPtr,AMS_XML_TAG_STATUS);

    if ( !statusPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName[%s] tag does not have status tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    const ClCharT  *proxyComp = clParserAttr (
            statusPtr,
            "proxyComp");

    if ( !proxyComp)
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName[%s] tag does not have proxyComp attribute\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT  proxyCompRef = { {0},0,0};

    memset (&proxyCompRef,0,sizeof (ClAmsEntityRefT));
    proxyCompRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    strcpy (proxyCompRef.entity.name.value,proxyComp);
    proxyCompRef.entity.name.length = strlen(proxyComp) + 1 ;

    if ( !strcmp(proxyComp,"") )
    {
        /*
         * No proxy comp so no need to set the ref ptr
         */
    }
    else
    {
        AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                    compRef,
                    proxyCompRef) );
    }

    ClParserPtrT listPtr = clParserChild( statusPtr,AMS_XML_TAG_CSI_LIST);

    if ( !listPtr)
    {
        return CL_OK;
    }

    ClParserPtrT list2Ptr = clParserChild( listPtr,AMS_XML_TAG_CSI);

    if ( !list2Ptr)
    {
        return CL_OK;
    }

    ClAmsEntityRefT  sourceEntityRef = {{0},0,0};
    ClAmsEntityRefT  activeCompRef = {{0},0,0};
    const ClCharT  *haState = NULL;
    const ClCharT  *td = NULL;
    const ClCharT  *rank = NULL;
    const ClCharT  *csiName = NULL;
    const ClCharT  *activeComp = NULL;
    const ClCharT  *pendingOp = NULL;

    memset (&sourceEntityRef, 0, sizeof (ClAmsEntityRefT));
    strcpy (sourceEntityRef.entity.name.value, name);
    sourceEntityRef.entity.name.length = strlen (name) + 1;
    sourceEntityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                &sourceEntityRef) );

    ClAmsCompT  *comp = (ClAmsCompT *)sourceEntityRef.ptr;

    AMS_CHECKPTR_AND_EXIT ( !comp );

    while ( list2Ptr )
    {

        ClAmsCompCSIRefT *csiRef = NULL;

        csiName = clParserAttr (
                list2Ptr,
                AMS_XML_TAG_NAME );

        haState = clParserAttr (
                list2Ptr,
                "haState");

        td = clParserAttr (
                list2Ptr,
                "td");
        
        rank = clParserAttr (
                list2Ptr,
                "rank"); 
        
        activeComp = clParserAttr (
                list2Ptr,
                "activeComp"); 
        
        pendingOp = clParserAttr (
                list2Ptr,
                "pendingOp");

        AMS_CHECKPTR_AND_EXIT ( !csiName|| !haState || !td || !rank ||
                !activeComp || !pendingOp );

        csiRef = clHeapAllocate (sizeof(ClAmsCompCSIRefT));

        AMS_CHECK_NO_MEMORY ( csiRef );

        strcpy (csiRef->entityRef.entity.name.value,csiName);
        csiRef->entityRef.entity.name.length = strlen (csiName) + 1;
        csiRef->entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;

        /*
         * Add the ptr for the entityRef 
         */

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                    &csiRef->entityRef) );

        AMS_CHECKPTR_AND_EXIT ( !csiRef->entityRef.ptr );

        csiRef->haState = atoi (haState);
        csiRef->tdescriptor = atoi (td);
        csiRef->rank = atoi (rank);
        csiRef->pendingOp = atoi (pendingOp); 
        
        memset (&activeCompRef,0,sizeof (ClAmsEntityRefT));
        strcpy (activeCompRef.entity.name.value, activeComp);
        activeCompRef.entity.name.length = strlen (activeComp) + 1;
        activeCompRef.entity.type = CL_AMS_ENTITY_TYPE_COMP; 
        
        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                    &activeCompRef) );
        
        csiRef->activeComp = activeCompRef.ptr;

        AMS_CHECK_RC_ERROR (  clAmsEntityListAddEntityRef(
                    &comp->status.csiList,
                    (ClAmsEntityRefT *)csiRef,
                    0) );

        csiName = NULL;
        haState = NULL;
        td = NULL;
        rank= NULL;
        activeComp = NULL;
        pendingOp = NULL;

        list2Ptr = list2Ptr->next;

    }

exitfn:

    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBSIMarshall(ClAmsSIT *si, ClBufferHandleT inMsgHdl,
                  ClUint32T marshallMask, ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&si->config.entity, inMsgHdl, 0));

    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG))
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSIConfigT, 4, 0, 0)(&si->config, inMsgHdl, 0));
    }

    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG_LIST))
    {
        /*
         * Write the config lists 
         */

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &si->config.suList,
                                                 CL_AMS_SI_CONFIG_SU_RANK_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &si->config.siDependentsList,
                                                 CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &si->config.siDependenciesList,
                                                 CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &si->config.csiList,
                                                 CL_AMS_SI_CONFIG_CSI_LIST,
                                                 inMsgHdl, versionCode) );
    }

    if((marshallMask & CL_AMS_ENTITY_DB_STATUS))
    {
        /*
         * Write the status portion
         */
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSIStatusT, 4, 0, 0)(&si->status, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(clAmsDBEntityOpMarshall(&si->config.entity, &si->status.entity, inMsgHdl, versionCode));
        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &si->status.suList,
                                                 CL_AMS_SI_STATUS_SU_LIST,
                                                 inMsgHdl, versionCode) );
    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBSIXMLize(
       CL_IN  ClAmsSIT  *si)
{
    ClRcT  rc = CL_OK;

    ClParserPtrT siPtr = 
        CL_PARSER_ADD_CHILD(amfPtr.siNamesPtr,AMS_XML_TAG_SI_NAME, 0);

    if (!si || !siPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, 
                ("Entity null pointer or Entity tag null pointer\n"));
        goto exitfn;
    }

    CL_PARSER_SET_ATTR(
            siPtr,
            AMS_XML_TAG_NAME,
            si->config.entity.name.value);

    ClParserPtrT configPtr = CL_PARSER_ADD_CHILD( siPtr,AMS_XML_TAG_CONFIG , 0);

    if ( !configPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null config pointer \n", si->config.entity.name.value));
        goto exitfn;
    }

    ClCharT  *adminState = NULL;
    ClCharT  *rank = NULL;
    ClCharT  *numCSIs = NULL;
    ClCharT  *numStandbyAssignments = NULL;

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->config.adminState,
                &adminState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->config.rank,
                &rank) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->config.numCSIs,
                &numCSIs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->config.numStandbyAssignments,
                &numStandbyAssignments) );

    CL_PARSER_SET_ATTR (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE,
            adminState);

    CL_PARSER_SET_ATTR (
            configPtr,
            "rank",
            rank);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numCSIs",
            numCSIs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numStandbyAssignments",
            numStandbyAssignments);

    CL_PARSER_SET_ATTR (
            configPtr,
            "parentSG",
            si->config.parentSG.entity.name.value);

    /*
     * Write the config lists 
     */

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &si->config.suList,
                CL_AMS_SI_CONFIG_SU_RANK_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &si->config.siDependentsList,
                CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &si->config.siDependenciesList,
                CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &si->config.csiList,
                CL_AMS_SI_CONFIG_CSI_LIST) );

    /*
     * Write the status portion
     */

    ClParserPtrT  statusPtr = CL_PARSER_ADD_CHILD(siPtr,AMS_XML_TAG_STATUS,0);

    ClCharT  *operState = NULL;
    ClCharT  *numActiveAssignments = NULL;
    ClCharT  *statusNumStandbyAssignments = NULL;

    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &si->status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->status.operState,
                &operState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->status.numActiveAssignments,
                &numActiveAssignments) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                si->status.numStandbyAssignments,
                &statusNumStandbyAssignments) );

    CL_PARSER_SET_ATTR (
            statusPtr,
            "operState",
            operState);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numActiveAssignments",
            numActiveAssignments);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numStandbyAssignments",
            statusNumStandbyAssignments);

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &si->status.suList,
                CL_AMS_SI_STATUS_SU_LIST) );

exitfn:

    clAmsFreeMemory (adminState);
    clAmsFreeMemory (rank);
    clAmsFreeMemory (numCSIs);
    clAmsFreeMemory (numStandbyAssignments);
    clAmsFreeMemory (operState);
    clAmsFreeMemory (numActiveAssignments);
    clAmsFreeMemory (statusNumStandbyAssignments);

    return CL_AMS_RC (rc);

}

static ClRcT   
clAmsDBSIUnmarshall(ClAmsEntityT *entity,
                    ClBufferHandleT inMsgHdl,
                    ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = 0;
    ClAmsSIT *si = NULL;
    ClAmsEntityRefT entityRef = {{0}};

    AMS_CHECK_ENTITY_TYPE(entity->type);

    if(entity->type != CL_AMS_ENTITY_TYPE_SI)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SI unmarshall invoked with invalid entity type [%d]\n",
                                 entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                 &entityRef);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("SI entitydb operation returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    si = (ClAmsSIT*)entityRef.ptr;
    
    CL_ASSERT(si != NULL);

    for(;;)
    {
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));
        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsSIConfigT siConfig = {{0}};
                ClAmsEntityRefT targetEntityRef = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSIConfigT, 4, 0, 0)(inMsgHdl, &siConfig));
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &siConfig.entity));
                memcpy(&targetEntityRef.entity, &siConfig.parentSG.entity, 
                       sizeof(targetEntityRef.entity));
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsSIStatusT siStatus = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSIStatusT, 4, 0, 0)(inMsgHdl, &siStatus));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &siStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &siStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP:
            return CL_OK;

        default: break;

        }
    }

    exitfn:
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBSIDeXMLize(
       CL_IN  ClParserPtrT  siPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;

    AMS_CHECKPTR (!siPtr);

    name = clParserAttr(
            siPtr,
            AMS_XML_TAG_NAME);

    if ( !name )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("SI instance does not have name tag\n"));
        goto exitfn;
    }

    ClParserPtrT  configPtr = clParserChild( siPtr,AMS_XML_TAG_CONFIG );

    if ( !configPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null config pointer \n", name));
        goto exitfn;
    }

    const ClCharT  *adminState = NULL;
    const ClCharT  *rank = NULL;
    const ClCharT  *numCSIs = NULL;
    const ClCharT  *numStandbyAssignments = NULL;

    adminState = clParserAttr (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE);

    rank = clParserAttr (
            configPtr,
            "rank");

    numCSIs = clParserAttr (
            configPtr,
            "numCSIs");

    numStandbyAssignments = clParserAttr (
            configPtr,
            "numStandbyAssignments");

    if ( !adminState || !rank || !numCSIs || !numStandbyAssignments )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("SI[%s] has a missing config attribute \n",
                    name));
        goto exitfn;
    }

    /*
     * Read the status portion
     */

    ClParserPtrT  statusPtr = clParserChild(siPtr,AMS_XML_TAG_STATUS);

    if ( !statusPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("SI[%s] does not have a status tag\n",
                    name));
        goto exitfn;
    }

    const ClCharT  *operState = NULL;
    const ClCharT  *numActiveAssignments = NULL;
    const ClCharT  *statusNumStandbyAssignments = NULL;

    operState = clParserAttr (
            statusPtr,
            "operState");

    numActiveAssignments = clParserAttr (
            statusPtr,
            "numActiveAssignments");

    statusNumStandbyAssignments = clParserAttr (
            statusPtr,
            "numStandbyAssignments");

    if ( !operState || !numActiveAssignments || !numStandbyAssignments )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("SI[%s] has a missing status attribute\n",
                    name));
        goto exitfn;
    }

    ClAmsEntityRefT  entityRef = {{0},0,0};
    ClAmsSIT  si;

    memset (&entityRef,0,sizeof (ClAmsEntityRefT));
    memset (&si,0,sizeof (ClAmsSIT));

    entityRef.entity.type = CL_AMS_ENTITY_TYPE_SI;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                &entityRef) );

    /*
     * Write the config part
     */
    memcpy (&si.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    si.config.adminState= atoi (adminState);
    si.config.rank= atoi (rank);
    si.config.numCSIs= atoi (numCSIs);
    si.config.numStandbyAssignments= atoi (numStandbyAssignments);

    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&si.config) );

    si.status.operState= atoi (operState);
    si.status.numActiveAssignments= atoi (numActiveAssignments);
    si.status.numStandbyAssignments= atoi (statusNumStandbyAssignments);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &si.status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&si.status) );

exitfn:

    return CL_AMS_RC (rc);

}


ClRcT   
clAmsDBSIListDeXMLize(
       CL_IN  ClParserPtrT  siPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;

    name = clParserAttr (
            siPtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("siName tag does not have  name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
   
    ClParserPtrT  configPtr = clParserChild( siPtr,AMS_XML_TAG_CONFIG );

    if ( !configPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null config pointer \n", name));
        goto exitfn;
    }

    const ClCharT  *parentSG = NULL;
    
    parentSG = clParserAttr (
            configPtr,
            "parentSG");

    if ( !parentSG )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SI[%s] does not have parentSG attribute \n",
                    name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT  siRef = { {0},0,0};
    ClAmsEntityRefT  sgRef = { {0},0,0};

    memset (&siRef,0,sizeof (ClAmsEntityRefT));
    memset (&sgRef,0,sizeof (ClAmsEntityRefT));

    siRef.entity.type = CL_AMS_ENTITY_TYPE_SI;
    sgRef.entity.type = CL_AMS_ENTITY_TYPE_SG;

    strcpy (siRef.entity.name.value,name);
    strcpy (sgRef.entity.name.value,parentSG);
    siRef.entity.name.length = strlen(name) + 1 ;
    sgRef.entity.name.length = strlen(parentSG) + 1 ;

    AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                siRef,
                sgRef) );

    /*
     * Write the config lists 
     */

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name, 
                CL_AMS_SI_CONFIG_SU_RANK_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SI_CONFIG_CSI_LIST) );

    ClParserPtrT  statusPtr = clParserChild(siPtr,AMS_XML_TAG_STATUS);

    if ( !statusPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("compName[%s] tag does not have status tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    ClParserPtrT  listPtr = clParserChild( statusPtr,AMS_XML_TAG_SU_LIST);
    if ( !listPtr)
    {
        return CL_OK;
    }

    ClParserPtrT  list2Ptr = clParserChild( listPtr,AMS_XML_TAG_SU);

    if ( !list2Ptr)
    {
        return CL_OK;
    }

    ClAmsEntityRefT  sourceEntityRef = { {0},0,0};
    const ClCharT  *haState = NULL;
    const ClCharT  *suRank = NULL;
    const ClCharT  *suName = NULL;

    memset (&sourceEntityRef,0,sizeof (ClAmsEntityRefT));
    strcpy (sourceEntityRef.entity.name.value, name);
    sourceEntityRef.entity.name.length = strlen (name) + 1;
    sourceEntityRef.entity.type = CL_AMS_ENTITY_TYPE_SI;

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                &sourceEntityRef) );

    ClAmsSIT  *si = (ClAmsSIT *)sourceEntityRef.ptr;

    AMS_CHECKPTR_AND_EXIT (!si);

    while ( list2Ptr )
    {

        ClAmsSISURefT  *suRef = NULL;

        suName = clParserAttr (
                list2Ptr,
                AMS_XML_TAG_NAME );

        haState = clParserAttr (
                list2Ptr,
                "haState");

        suRank  = clParserAttr (
                list2Ptr,
                "suRank");


        AMS_CHECKPTR_AND_EXIT ( !suName|| !haState || !suRank );

        suRef = clHeapAllocate (sizeof(ClAmsSISURefT));

        AMS_CHECK_NO_MEMORY (suRef);

        strcpy (suRef->entityRef.entity.name.value,suName);
        suRef->entityRef.entity.name.length = strlen (suName) + 1;
        suRef->entityRef.entity.type = CL_AMS_ENTITY_TYPE_SU;

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                    &suRef->entityRef) );

        suRef->haState = atoi (haState);
        suRef->rank = atoi (suRank);

        AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                    &si->status.suList,
                    (ClAmsEntityRefT *)suRef,
                    0) );

        suName = NULL;
        haState = NULL;
        suRank = NULL;

        list2Ptr = list2Ptr->next;

    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBCSIMarshall(ClAmsCSIT *csi,
                   ClBufferHandleT inMsgHdl,
                   ClUint32T marshallMask,
                   ClUint32T versionCode)
                   
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;
    ClAmsEntityListTypeT listType = 0;
    ClUint32T listEntries = 0;
    ClAmsCSINameValuePairT  *pNVP = NULL;
    ClCntNodeHandleT  nodeHandle = 0;

    AMS_CHECKPTR_AND_EXIT(!csi);

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&csi->config.entity, inMsgHdl, 0));

    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG))
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;

        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCSIConfigT, 4, 0, 0)(&csi->config, inMsgHdl, 0));
    }
    
    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG_LIST))
    {
        /*
         * Write the nameValuePairList
         */
        op = CL_AMS_CKPT_OPERATION_SET_LIST;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

        listType = CL_AMS_CSI_CONFIG_NVP_LIST;
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(clCntSizeGet(csi->config.nameValuePairList, &listEntries) );
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&listEntries, inMsgHdl, 0));

        for ( pNVP = (ClAmsCSINameValuePairT *)clAmsCntGetFirst(
                                                                &csi->config.nameValuePairList,&nodeHandle);
              pNVP != NULL;
              pNVP= (ClAmsCSINameValuePairT *)clAmsCntGetNext(
                                                              &csi->config.nameValuePairList,&nodeHandle))
        {
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCSINVPT, 4, 0, 0)(pNVP, inMsgHdl, 0));
        }

        /*
         * Write the dependency lists.
         */
        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &csi->config.csiDependentsList,
                                                 CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &csi->config.csiDependenciesList,
                                                 CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST,
                                                 inMsgHdl, versionCode) );
    }

    if((marshallMask & CL_AMS_ENTITY_DB_STATUS))
    {
        /*
         * Write the status portion
         */
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCSIStatusT, 4, 0, 0)(&csi->status, inMsgHdl, 0));

        AMS_CHECK_RC_ERROR(clAmsDBEntityOpMarshall(&csi->config.entity, &csi->status.entity, inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &csi->status.pgList,
                                                 CL_AMS_CSI_STATUS_PG_LIST,
                                                 inMsgHdl,
                                                 versionCode) );

        op = CL_AMS_CKPT_OPERATION_SET_LIST;
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

        listType = CL_AMS_CSI_PGTRACK_CLIENT_LIST;
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, inMsgHdl, 0));

        listEntries = 0;
        AMS_CHECK_RC_ERROR(clCntSizeGet(csi->status.pgTrackList, &listEntries));
        AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&listEntries, inMsgHdl, 0));

        ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;

        nodeHandle = 0;

        for ( pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetFirst(
                                                                         &csi->status.pgTrackList,&nodeHandle); pgTrackClient!= NULL;
              pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetNext(
                                                                        &csi->status.pgTrackList,&nodeHandle))
        {
            /* 
             * Marshall IOC address and trackflags. explicitly
             */
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClIocPhysicalAddressT, 4, 0, 0)(&pgTrackClient->address.iocPhyAddress,
                                                                  inMsgHdl, 0) );
            AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&pgTrackClient->trackFlags, inMsgHdl, 0));
            /*AMS_CHECK_RC_ERROR(clXdrMarshallClAmsCSIPGTrackClientT(pgTrackClient, inMsgHdl, 0));*/
        }
    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    
    exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBCSIXMLize(
       CL_IN  ClAmsCSIT  *csi)
{

    ClRcT  rc = CL_OK;

    ClParserPtrT  csiPtr = 
        CL_PARSER_ADD_CHILD(amfPtr.csiNamesPtr,AMS_XML_TAG_CSI_NAME, 0);

    if (!csi || !csiPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, 
                ("Entity null pointer or Entity tag null pointer\n"));
        goto exitfn;
    }

    CL_PARSER_SET_ATTR(
            csiPtr,
            AMS_XML_TAG_NAME,
            csi->config.entity.name.value);

    ClParserPtrT configPtr = CL_PARSER_ADD_CHILD( csiPtr,AMS_XML_TAG_CONFIG , 0);

    if ( !configPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null config pointer \n",
                    csi->config.entity.name.value));
        goto exitfn;
    }

    ClCharT  *rank = NULL;
    ClCharT  *isProxyCSI = NULL;

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                csi->config.rank,
                &rank) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                csi->config.isProxyCSI,
                &isProxyCSI) );

    CL_PARSER_SET_ATTR (
            configPtr,
            "rank",
            rank );

    CL_PARSER_SET_ATTR (
            configPtr,
            "isProxyCSI",
            isProxyCSI );

    CL_PARSER_SET_ATTR (
            configPtr,
            "type",
             csi->config.type.value);

    CL_PARSER_SET_ATTR (
            configPtr,
            "parentSI",
             csi->config.parentSI.entity.name.value);

    /*
     * Write the nameValuePairList
     */

    ClParserPtrT  nvpListPtr = CL_PARSER_ADD_CHILD(configPtr,"nvpList", 0);
    ClAmsCSINameValuePairT  *pNVP = NULL;
    ClCntNodeHandleT  nodeHandle = 0;

    if ( !nvpListPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null nvpList pointer \n",
                    csi->config.entity.name.value));
        goto exitfn;
    }

    for ( pNVP = (ClAmsCSINameValuePairT *)clAmsCntGetFirst(
                &csi->config.nameValuePairList,&nodeHandle);
            pNVP != NULL;
            pNVP= (ClAmsCSINameValuePairT *)clAmsCntGetNext(
                &csi->config.nameValuePairList,&nodeHandle))
    {

        ClParserPtrT  nvpPtr = CL_PARSER_ADD_CHILD(nvpListPtr,"nvp", 0);

        if ( !nvpPtr || !pNVP )
        {
            rc = CL_ERR_NULL_POINTER;
            AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null nvp pointer \n",
                        csi->config.entity.name.value));
            goto exitfn;
        }

        CL_PARSER_SET_ATTR (
                nvpPtr,
                "name",
                 pNVP->paramName.value);

        CL_PARSER_SET_ATTR (
                nvpPtr,
                "value",
                 pNVP->paramValue.value);

    }

    /*
     * Write the dependency lists.
     */
    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &csi->config.csiDependentsList,
                CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &csi->config.csiDependenciesList,
                CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST) );

    /*
     * Write the status portion
     */

    ClParserPtrT  statusPtr = CL_PARSER_ADD_CHILD(csiPtr,AMS_XML_TAG_STATUS,0);

    if ( !statusPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("Entity[%s] has null status pointer \n",
                    csi->config.entity.name.value));
        goto exitfn;
    }

    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &csi->status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &csi->status.pgList,
                CL_AMS_CSI_STATUS_PG_LIST) );

    ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;
    nodeHandle = 0;

    ClParserPtrT  pgTrackListPtr = CL_PARSER_ADD_CHILD(
            statusPtr,"pgTrackClientList",0);

    if ( !pgTrackListPtr )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("Entity[%s] has null pgTrackListPtr pointer \n", csi->config.entity.name.value));
        goto exitfn;
    }

    for ( pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetFirst(
                &csi->status.pgTrackList,&nodeHandle); pgTrackClient!= NULL;
            pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetNext(
                &csi->status.pgTrackList,&nodeHandle))
    {

        ClCharT  *trackFlags = NULL;
        ClCharT  *nodeAddress = NULL;
        ClCharT  *portID = NULL;
        ClParserPtrT  pgTrackClientPtr = CL_PARSER_ADD_CHILD(
                pgTrackListPtr,"pgTrackClient", 0);

        if (!pgTrackClient || !pgTrackClientPtr)
        {
            rc = CL_ERR_NULL_POINTER;
            AMS_LOG (CL_DEBUG_ERROR,("Entity[%s] has null pgTrackClient or pgTrackClientPtr pointer \n", csi->config.entity.name.value));
            goto exitfn;
        }

        /*
         *XXX: Currently not writing the logical address as its not used
         */
        AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                    pgTrackClient->address.iocPhyAddress.nodeAddress,
                    &nodeAddress) );

        AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                    pgTrackClient->address.iocPhyAddress.portId,
                    &portID) );

        AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                    pgTrackClient->trackFlags,
                    &trackFlags) );

        AMS_CHECKPTR_AND_EXIT ( !trackFlags || !nodeAddress || !portID );

        CL_PARSER_SET_ATTR (
                pgTrackClientPtr,
                "nodeAddress",
                nodeAddress);

        CL_PARSER_SET_ATTR (
                pgTrackClientPtr,
                "portID",
                portID);

        CL_PARSER_SET_ATTR (
                pgTrackClientPtr,
                "trackFlags",
                trackFlags);

        clAmsFreeMemory (nodeAddress);
        clAmsFreeMemory (portID);
        clAmsFreeMemory (trackFlags);

    }

exitfn:

    clAmsFreeMemory (rank);
    clAmsFreeMemory (isProxyCSI);

    return CL_AMS_RC (rc);

}

static ClRcT   
clAmsDBCSIUnmarshall(ClAmsEntityT *entity,
                     ClBufferHandleT inMsgHdl,
                     ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = 0;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsCSIT *csi = NULL;

    AMS_CHECK_ENTITY_TYPE(entity->type);
    if(entity->type != CL_AMS_ENTITY_TYPE_CSI)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("CSI unmarshall invoked with invalid entity type [%d]\n",
                                 entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }
    
    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                 &entityRef);

    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("CSI entitydb operation returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    csi = (ClAmsCSIT*)entityRef.ptr;

    CL_ASSERT(csi != NULL);

    for(;;)
    {
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));
        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsCSIConfigT csiConfig = {{0}};
                ClAmsEntityRefT targetEntityRef = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSIConfigT, 4, 0, 0)(inMsgHdl, 
                                                                  &csiConfig));
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &csiConfig.entity));
                memcpy(&targetEntityRef.entity, &csiConfig.parentSI.entity, 
                       sizeof(targetEntityRef.entity));
                clAmsEntitySetRefPtr(entityRef, targetEntityRef);
                break;
            }
            
        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsCSIStatusT csiStatus = {{0}};
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSIStatusT, 4, 0, 0)(inMsgHdl,
                                                                  &csiStatus));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &csiStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &csiStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP:
            return CL_OK;

        default: break;

        }
    }

    exitfn:
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBCSIDeXMLize(
       CL_IN  ClParserPtrT  csiPtr)
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR (!csiPtr);

    const ClCharT  *name = NULL;
    
    name = clParserAttr(
            csiPtr,
            AMS_XML_TAG_NAME  );

    if ( !name )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("CSI instance does not have a name tag\n"));
        goto exitfn;
    }

    ClParserPtrT  configPtr = clParserChild ( csiPtr,AMS_XML_TAG_CONFIG );

    AMS_CHECKPTR_AND_EXIT (!configPtr);

    const ClCharT  *rank = NULL;
    const ClCharT  *type = NULL;
    const ClCharT  *isProxyCSI = NULL;

    rank = clParserAttr (
            configPtr,
            "rank");

    type = clParserAttr (
            configPtr,
            "type");

    isProxyCSI = clParserAttr (
            configPtr,
            "isProxyCSI");

    if ( !rank || !type || !isProxyCSI)
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("CSI[%s] has a missing attribute",
                    name));
        goto exitfn;
    }


    ClAmsEntityRefT  entityRef = { {0},0,0};
    ClAmsCSIT  csi;

    memset (&entityRef,0,sizeof (ClAmsEntityRefT));
    memset (&csi,0,sizeof (ClAmsCSIT));

    entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                &entityRef) );

    /*
     * Write the config part
     */

    memcpy (&csi.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    csi.config.rank= atoi (rank);
    csi.config.isProxyCSI= atoi (isProxyCSI);
    strcpy (csi.config.type.value,type);
    csi.config.type.length = strlen (type) + 1;

    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&csi.config) );

    ClParserPtrT  statusPtr = clParserChild ( csiPtr,AMS_XML_TAG_STATUS );

    AMS_CHECKPTR_AND_EXIT (!statusPtr);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &csi.status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&csi.status) );


exitfn:

    return CL_AMS_RC (rc);

}


ClRcT   
clAmsDBCSIListDeXMLize(
       CL_IN  ClParserPtrT  csiPtr)
{
    ClRcT  rc = CL_OK;

    AMS_CHECKPTR (!csiPtr);

    const ClCharT  *name = NULL;
    
    name = clParserAttr(
            csiPtr,
            AMS_XML_TAG_NAME  );

    if ( !name )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("CSI instance does not have a name tag\n"));
        goto exitfn;
    }

    ClParserPtrT  configPtr = clParserChild ( csiPtr,AMS_XML_TAG_CONFIG );

    AMS_CHECKPTR_AND_EXIT (!configPtr);

    const ClCharT  *parentSI = NULL;

    parentSI = clParserAttr (
            configPtr,
            "parentSI");

    if ( !parentSI )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR, ("CSI[%s] does not have a parentSI tag\n",
                    name));
        goto exitfn;
    }

    ClAmsEntityRefT  csiRef = {{0},0,0};
    ClAmsEntityRefT  siRef = {{0},0,0};

    memset (&csiRef,0,sizeof (ClAmsEntityRefT));
    memset (&siRef,0,sizeof (ClAmsEntityRefT));

    csiRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;
    siRef.entity.type = CL_AMS_ENTITY_TYPE_SI;

    strcpy (csiRef.entity.name.value,name);
    csiRef.entity.name.length = strlen(name) + 1 ;
    strcpy (siRef.entity.name.value,parentSI);
    siRef.entity.name.length = strlen(parentSI) + 1 ;

    AMS_CHECK_RC_ERROR ( clAmsEntitySetRefPtr(
                csiRef,
                siRef) );

    /*
     * Read the nameValuePairList
     */

    ClParserPtrT  nvpListPtr = 
        clParserChild(configPtr,"nvpList");

    if ( nvpListPtr )
    {
        ClParserPtrT  nvpPtr = clParserChild(nvpListPtr,"nvp");

        while (nvpPtr)
        {

            const ClCharT  *nvpName = NULL;
            const ClCharT  *value = NULL;

            nvpName = clParserAttr (
                    nvpPtr,
                    "name");

            value = clParserAttr (
                    nvpPtr,
                    "value");

            AMS_CHECKPTR_AND_EXIT ( !nvpName || !value);

            ClAmsCSINameValuePairT  nvp = {{0},{0},{0}};

            strcpy (nvp.csiName.value, name);
            strcpy (nvp.paramName.value, nvpName);
            strcpy (nvp.paramValue.value, value);
            nvp.csiName.length = strlen (name) + 1;
            nvp.paramName.length = strlen (nvpName) + 1;
            nvp.paramValue.length = strlen (value) + 1;

            ClAmsEntityT  csiEntity = {0};

            memset (&csiEntity,0, sizeof (ClAmsEntityT));
            csiEntity.type = CL_AMS_ENTITY_TYPE_CSI;
            strcpy (csiEntity.name.value,name);
            csiEntity.name.length = strlen (name) + 1;

            AMS_CHECK_RC_ERROR ( clAmsCSISetNVP(
                        gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                        csiEntity,
                        nvp) );

            nvpPtr = nvpPtr->next;

        }
    }

    /*
     * Read the dependency lists.
     */

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST) );

    /*
     * Read the status portion
     */

    ClParserPtrT  statusPtr = clParserChild(csiPtr,AMS_XML_TAG_STATUS);
    ClParserPtrT  listPtr = clParserChild( statusPtr,AMS_XML_TAG_PG_LIST);

    if ( listPtr)
    {

        ClParserPtrT  list2Ptr = clParserChild( listPtr,AMS_XML_TAG_PG);
        ClAmsEntityRefT  sourceEntityRef = {{0},0,0};

        memset (&sourceEntityRef,0,sizeof (ClAmsEntityRefT));
        strcpy (sourceEntityRef.entity.name.value, name);
        sourceEntityRef.entity.name.length = strlen (name) + 1;
        sourceEntityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;

        AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                    &sourceEntityRef) );

        ClAmsCSIT  *csi = (ClAmsCSIT *)sourceEntityRef.ptr;

        AMS_CHECKPTR_AND_EXIT ( !csi )

        while ( list2Ptr)
        {

            ClAmsCSICompRefT  *compRef = NULL;
            const ClCharT  *haState = NULL;
            const ClCharT  *rank = NULL;
            const ClCharT  *compName = NULL; 

            compName= clParserAttr (
                    list2Ptr,
                    "name");

            haState = clParserAttr (
                    list2Ptr,
                    "haState");

            rank = clParserAttr (
                    list2Ptr,
                    "rank");

            AMS_CHECKPTR_AND_EXIT ( !haState || !compName || !rank );

            compRef = clHeapAllocate (sizeof(ClAmsCSICompRefT));

            AMS_CHECK_NO_MEMORY (compRef);

            strcpy (compRef->entityRef.entity.name.value,compName);
            compRef->entityRef.entity.name.length = strlen (compName) + 1;
            compRef->entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
            compRef->haState = atoi (haState);
            compRef->rank = atoi (rank);

            /*
             * Add the ptr for the entityRef 
             */

            AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                        &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                        &compRef->entityRef) );

            AMS_CHECKPTR_AND_EXIT ( !compRef->entityRef.ptr );

            AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                        &csi->status.pgList,
                        (ClAmsEntityRefT *)compRef,
                        0) );

            haState = NULL;
            rank = NULL;
            compName = NULL;
            list2Ptr = list2Ptr->next;

        }
    }

    ClParserPtrT  pgTrackListPtr = clParserChild( statusPtr,"pgTrackClientList");

    if (pgTrackListPtr)
    {

        ClParserPtrT  pgTrackClientPtr = clParserChild(
                pgTrackListPtr,"pgTrackClient");
        const ClCharT  *nodeAddress = NULL;
        const ClCharT  *portID = NULL;
        const ClCharT  *trackFlags = NULL;

        while (pgTrackClientPtr)
        {
            
            nodeAddress = clParserAttr (
                    pgTrackClientPtr,
                    "nodeAddress");

            portID = clParserAttr (
                    pgTrackClientPtr,
                    "portID");

            trackFlags = clParserAttr (
                    pgTrackClientPtr,
                    "trackFlags");

            AMS_CHECKPTR_AND_EXIT ( !nodeAddress || !portID || !trackFlags );

            ClAmsEntityT  csiEntity = {0};

            memset (&csiEntity,0,sizeof (ClAmsEntityT));
            csiEntity.type = CL_AMS_ENTITY_TYPE_CSI;
            strcpy (csiEntity.name.value,name);
            csiEntity.name.length = strlen(name) + 1;

            ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;

            pgTrackClient = clHeapAllocate (sizeof(ClAmsCSIPGTrackClientT));

            AMS_CHECK_NO_MEMORY ( pgTrackClient );

            pgTrackClient->trackFlags = atoi (trackFlags);
            pgTrackClient->address.iocPhyAddress.nodeAddress
                = atoi (nodeAddress);
            pgTrackClient->address.iocPhyAddress.portId
                = atoi (portID);

            AMS_CHECK_RC_ERROR ( clAmsCSIAddToPGTrackList(
                        &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                        &csiEntity,
                        pgTrackClient,
                        NULL) );

            pgTrackClientPtr = pgTrackClientPtr->next;

        }
    }

exitfn:

    return CL_AMS_RC (rc);

}

/*
 * Marshall the failover history for the SG. if non-empty
 */
static ClRcT clAmsDBSGFailoverHistoryMarshall(ClAmsSGT *sg, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_SET_FAILOVER_HISTORY;
    ClListHeadT *iter = NULL;

    if(versionCode == CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE))
        return CL_OK;

    if(!sg->config.maxFailovers ||
       !sg->status.failoverHistoryCount)
        return CL_OK;

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    CL_LIST_FOR_EACH(iter, &sg->status.failoverHistory)
    {
        ClAmsSGFailoverHistoryT *history = CL_LIST_ENTRY(iter, ClAmsSGFailoverHistoryT, list);
        CL_ASSERT(history->timer != CL_HANDLE_INVALID_VALUE);
        AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&history->index, inMsgHdl, 0));
        AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&history->numFailovers, inMsgHdl, 0));
        /*
         * Now pack the cluster timer
         */
        AMS_CHECK_RC_ERROR(clTimerClusterPack(history->timer, inMsgHdl));
    }
    
    exitfn:
    return rc;
}

static ClRcT clAmsDBSGFailoverHistoryUnmarshall(ClAmsSGT *sg, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClInt32T i;

    for(i = 0; i < sg->status.failoverHistoryCount; ++i)
    {
        ClAmsSGFailoverHistoryT *failoverHistory = clHeapCalloc(1, sizeof(*failoverHistory));
        CL_ASSERT(failoverHistory != NULL);
        clListAddTail(&failoverHistory->list, &sg->status.failoverHistory);
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &failoverHistory->index));
        memcpy(&failoverHistory->entity, &sg->config.entity, sizeof(failoverHistory->entity));
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &failoverHistory->numFailovers));
        AMS_CHECK_RC_ERROR(clTimerClusterUnpack(inMsgHdl, &failoverHistory->timer));
    }
    
    return rc;

    exitfn:
    {
        while(!CL_LIST_HEAD_EMPTY(&sg->status.failoverHistory))
        {
            ClListHeadT *head = sg->status.failoverHistory.pNext;
            ClAmsSGFailoverHistoryT *history = CL_LIST_ENTRY(head, ClAmsSGFailoverHistoryT, list);
            clListDel(&history->list);
            if(history->timer)
            {
                clTimerClusterFree(history->timer);
            }
            clHeapFree(history);
        }
    }
    return rc;
}

static ClRcT
clAmsDBSGMarshallConfigVersion(ClAmsSGConfigT *pSGConfig, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    
    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSGConfigT, 4, 0, 0)(pSGConfig, inMsgHdl, 0));
            break;
    case CL_VERSION_CODE(4, 1, 0):
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSGConfigT, 4, 1, 0)(pSGConfig, inMsgHdl, 0));
            break;
    default:
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSGConfigT, 5, 0, 0)(pSGConfig, inMsgHdl, 0));
        break;
    }

    exitfn:
    return rc;
}

static ClRcT
clAmsDBSGMarshallStatusVersion(ClAmsSGStatusT *pSGStatus, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    
    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSGStatusT, 4, 0, 0)(pSGStatus, inMsgHdl, 0));
        break;
    default:
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsSGStatusT, 4, 1, 0)(pSGStatus, inMsgHdl, 0));
        break;
    }

    exitfn:
    return rc;
}

ClRcT   
clAmsDBSGMarshall(ClAmsSGT *sg, ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_START_GROUP;

    AMS_CHECKPTR (!sg);

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR (VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&sg->config.entity, inMsgHdl, 0));

    if( (marshallMask & CL_AMS_ENTITY_DB_CONFIG))
    {
        op = CL_AMS_CKPT_OPERATION_SET_CONFIG;
        AMS_CHECK_RC_ERROR (clXdrMarshallClInt32T(&op, inMsgHdl, 0) );
        AMS_CHECK_RC_ERROR(clAmsDBSGMarshallConfigVersion(&sg->config, inMsgHdl, versionCode));
    }

    if((marshallMask & CL_AMS_ENTITY_DB_CONFIG_LIST))
    {
        /*
         * Write the suList and siList
         */

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->config.suList,
                                                 CL_AMS_SG_CONFIG_SU_LIST,
                                                 inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->config.siList,
                                                 CL_AMS_SG_CONFIG_SI_LIST,
                                                 inMsgHdl, versionCode));
    }

    if((marshallMask & CL_AMS_ENTITY_DB_STATUS))
    {
        /*
         * Write the status portion
         */
        op = CL_AMS_CKPT_OPERATION_SET_STATUS;
        AMS_CHECK_RC_ERROR (clXdrMarshallClInt32T(&op, inMsgHdl, 0) );
        AMS_CHECK_RC_ERROR(clAmsDBSGMarshallStatusVersion(&sg->status, inMsgHdl, versionCode));
        AMS_CHECK_RC_ERROR(clAmsDBEntityOpMarshall(&sg->config.entity,
                                                   &sg->status.entity,
                                                   inMsgHdl,
                                                   versionCode));
        AMS_CHECK_RC_ERROR(clAmsDBSGFailoverHistoryMarshall(sg, inMsgHdl, versionCode));
        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &sg->status.instantiateTimer,
                                                  CL_AMS_SG_TIMER_INSTANTIATE,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &sg->status.adjustTimer,
                                                  CL_AMS_SG_TIMER_ADJUST,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBTimerMarshall(
                                                  &sg->status.adjustProbationTimer,
                                                  CL_AMS_SG_TIMER_ADJUST_PROBATION,
                                                  inMsgHdl, versionCode));

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->status.instantiableSUList,
                                                 CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->status.instantiatedSUList,
                                                 CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->status.inserviceSpareSUList,
                                                 CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->status.assignedSUList,
                                                 CL_AMS_SG_STATUS_ASSIGNED_SU_LIST,
                                                 inMsgHdl, versionCode) );

        AMS_CHECK_RC_ERROR ( clAmsDBListMarshall(
                                                 &sg->status.faultySUList,
                                                 CL_AMS_SG_STATUS_FAULTY_SU_LIST,
                                                 inMsgHdl, versionCode) );
    }

    op = CL_AMS_CKPT_OPERATION_END_GROUP;

    AMS_CHECK_RC_ERROR( clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBSGXMLize(
       CL_IN  ClAmsSGT  *sg)
{
    ClRcT  rc = CL_OK;

    ClParserPtrT  sgPtr = 
        CL_PARSER_ADD_CHILD(amfPtr.sgNamesPtr,AMS_XML_TAG_SG_NAME, 0);

    AMS_CHECKPTR (!sg);

    CL_PARSER_SET_ATTR(
            sgPtr,
            AMS_XML_TAG_NAME,
            sg->config.entity.name.value);

    ClParserPtrT  configPtr = CL_PARSER_ADD_CHILD( sgPtr,AMS_XML_TAG_CONFIG , 0);

    AMS_CHECKPTR_AND_EXIT (!configPtr);

    ClCharT  *adminState = NULL;
    ClCharT  *redundancyModel = NULL;
    ClCharT  *loadingStrategy = NULL;
    ClCharT  *failbackOption = NULL;
    ClCharT  *instantiateDuration = NULL;
    ClCharT  *numPrefActiveSUs = NULL;
    ClCharT  *numPrefStandbySUs = NULL;
    ClCharT  *numPrefInserviceSUs = NULL;
    ClCharT  *numPrefAssignedSUs = NULL;
    ClCharT  *numPrefActiveSUsPerSI = NULL;
    ClCharT  *maxActiveSIsPerSU = NULL;
    ClCharT  *maxStandbySIsPerSU = NULL;
    ClCharT  *compRestartDuration = NULL;
    ClCharT  *compRestartCountMax = NULL;
    ClCharT  *suRestartDuration = NULL;
    ClCharT  *suRestartCountMax = NULL;
    ClCharT  *autoRepair = NULL;
    ClCharT  *isCollocationAllowed = NULL;
    ClCharT  *alphaFactor = NULL;
    ClCharT  *betaFactor = NULL;

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.adminState,
                &adminState) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.redundancyModel,
                &redundancyModel) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.loadingStrategy,
                &loadingStrategy) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.failbackOption,
                &failbackOption) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
               sg->config.autoRepair,
               &autoRepair) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                sg->config.instantiateDuration,
                &instantiateDuration) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.numPrefActiveSUs,
                &numPrefActiveSUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.numPrefStandbySUs,
                &numPrefStandbySUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.numPrefInserviceSUs,
                &numPrefInserviceSUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.numPrefAssignedSUs,
                &numPrefAssignedSUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.numPrefActiveSUsPerSI,
                &numPrefActiveSUsPerSI) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.maxActiveSIsPerSU,
                &maxActiveSIsPerSU) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.maxStandbySIsPerSU,
                &maxStandbySIsPerSU) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                sg->config.compRestartDuration,
                &compRestartDuration) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.compRestartCountMax,
                &compRestartCountMax) );

    AMS_CHECK_RC_ERROR ( clAmsDBClInt64ToStr(
                sg->config.suRestartDuration,
                &suRestartDuration) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.suRestartCountMax,
                &suRestartCountMax) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.isCollocationAllowed,
                &isCollocationAllowed) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.alpha,
                &alphaFactor) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->config.beta,
                &betaFactor) );

    CL_PARSER_SET_ATTR (
            configPtr,
            AMS_XML_TAG_ADMIN_STATE,
            adminState);

    CL_PARSER_SET_ATTR (
            configPtr,
            "redundancyModel",
            redundancyModel);

    CL_PARSER_SET_ATTR (
            configPtr,
            "loadingStrategy",
            loadingStrategy);

    CL_PARSER_SET_ATTR (
            configPtr,
            "failbackOption",
            failbackOption);

    CL_PARSER_SET_ATTR (
            configPtr,
            "autoRepair",
            autoRepair);

    CL_PARSER_SET_ATTR (
            configPtr,
            "instantiateDuration",
            instantiateDuration);
    
    CL_PARSER_SET_ATTR (
            configPtr,
            "numPrefActiveSUs",
            numPrefActiveSUs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numPrefStandbySUs",
            numPrefStandbySUs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numPrefInserviceSUs",
            numPrefInserviceSUs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numPrefAssignedSUs",
            numPrefAssignedSUs);

    CL_PARSER_SET_ATTR (
            configPtr,
            "numPrefActiveSUsPerSI",
            numPrefActiveSUsPerSI);

    CL_PARSER_SET_ATTR (
            configPtr,
            "maxActiveSIsPerSU",
            maxActiveSIsPerSU);

    CL_PARSER_SET_ATTR (
            configPtr,
            "maxStandbySIsPerSU",
            maxStandbySIsPerSU);

    CL_PARSER_SET_ATTR (
            configPtr,
            "compRestartDuration",
            compRestartDuration);

    CL_PARSER_SET_ATTR (
            configPtr,
            "compRestartCountMax",
            compRestartCountMax);

    CL_PARSER_SET_ATTR (
            configPtr,
            "suRestartDuration",
            suRestartDuration);

    CL_PARSER_SET_ATTR (
            configPtr,
            "suRestartCountMax",
            suRestartCountMax);

    CL_PARSER_SET_ATTR (
            configPtr,
            "isCollocationAllowed",
            isCollocationAllowed);

    CL_PARSER_SET_ATTR (
            configPtr,
            "alphaFactor",
            alphaFactor);

    CL_PARSER_SET_ATTR (
            configPtr,
            "betaFactor",
            betaFactor);

    /*
     * Write the suList and siList
     */

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &sg->config.suList,
                CL_AMS_SG_CONFIG_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                configPtr,
                &sg->config.siList,
                CL_AMS_SG_CONFIG_SI_LIST) );

    /*
     * Write the status portion
     */

    ClParserPtrT  statusPtr = CL_PARSER_ADD_CHILD(sgPtr,AMS_XML_TAG_STATUS,0);

    ClCharT  *isStarted = NULL;
    ClCharT  *numCurrActiveSUs = NULL;
    ClCharT  *numCurrStandbySUs = NULL;

    AMS_CHECK_RC_ERROR( clAmsDBWriteEntityStatus(
                &sg->status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->status.isStarted,
                &isStarted) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->status.numCurrActiveSUs,
                &numCurrActiveSUs) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                sg->status.numCurrStandbySUs,
                &numCurrStandbySUs) );

    CL_PARSER_SET_ATTR (
            statusPtr,
            "isStarted",
            isStarted);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numCurrActiveSUs",
            numCurrActiveSUs);

    CL_PARSER_SET_ATTR (
            statusPtr,
            "numCurrStandbySUs",
            numCurrStandbySUs);

    AMS_CHECK_RC_ERROR ( clAmsDBWriteEntityTimer(
                statusPtr,
                &sg->status.instantiateTimer,
                "instantiateTimer") );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &sg->status.instantiableSUList,
                CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &sg->status.instantiatedSUList,
                CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &sg->status.inserviceSpareSUList,
                CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &sg->status.assignedSUList,
                CL_AMS_SG_STATUS_ASSIGNED_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBListToXML(
                statusPtr,
                &sg->status.faultySUList,
                CL_AMS_SG_STATUS_FAULTY_SU_LIST) );

exitfn:

    clAmsFreeMemory (adminState);
    clAmsFreeMemory (redundancyModel);
    clAmsFreeMemory (loadingStrategy);
    clAmsFreeMemory (failbackOption);
    clAmsFreeMemory (instantiateDuration);
    clAmsFreeMemory (numPrefActiveSUs);
    clAmsFreeMemory (numPrefStandbySUs);
    clAmsFreeMemory (numPrefInserviceSUs);
    clAmsFreeMemory (numPrefAssignedSUs);
    clAmsFreeMemory (numPrefActiveSUsPerSI);
    clAmsFreeMemory (maxActiveSIsPerSU);
    clAmsFreeMemory (maxStandbySIsPerSU);
    clAmsFreeMemory (compRestartDuration);
    clAmsFreeMemory (compRestartCountMax);
    clAmsFreeMemory (suRestartDuration);
    clAmsFreeMemory (suRestartCountMax);
    clAmsFreeMemory (isStarted);
    clAmsFreeMemory (numCurrActiveSUs);
    clAmsFreeMemory (numCurrStandbySUs);
    clAmsFreeMemory (autoRepair);
    clAmsFreeMemory (isCollocationAllowed);
    clAmsFreeMemory (alphaFactor);

    return CL_AMS_RC (rc);

}

static ClRcT
clAmsDBSGConfigUnmarshallVersion(ClBufferHandleT inMsgHdl, ClAmsSGConfigT *pSGConfig, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    extern ClAmsSGConfigT gClAmsSGDefaultConfig;

    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 0, 0)(inMsgHdl, pSGConfig));
        pSGConfig->maxFailovers =  0; /*disables the feature*/
        pSGConfig->failoverDuration = gClAmsSGDefaultConfig.failoverDuration;
        pSGConfig->beta = 0;
        break;

    case CL_VERSION_CODE(4, 1, 0):
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 4, 1, 0)(inMsgHdl, pSGConfig));
        pSGConfig->beta = 0; /*disables the feature*/
        break;
    default:
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGConfigT, 5, 0, 0)(inMsgHdl, pSGConfig));
        break;
    }

    exitfn:
    return rc;
}

static ClRcT
clAmsDBSGStatusUnmarshallVersion(ClBufferHandleT inMsgHdl, ClAmsSGStatusT *pSGStatus, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;

    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGStatusT, 4, 0, 0)(inMsgHdl, pSGStatus));
        break;

    default:
        AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSGStatusT, 4, 1, 0)(inMsgHdl, pSGStatus));
        break;
    }

    exitfn:
    return rc;
}

static ClRcT   
clAmsDBSGUnmarshall(ClAmsEntityT *entity,
                    ClBufferHandleT inMsgHdl,
                    ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsSGT *sg = NULL;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsCkptOperationT op = 0;

    if(entity->type != CL_AMS_ENTITY_TYPE_SG)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SG unmarshall invoked with invalid entity type [%d]\n",
                                 entity->type));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                 &entityRef);

    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = clAmsEntityDbAddEntity(&gAms.db.entityDb[entity->type],
                                        &entityRef);
        }

        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("SG entitydb operation returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    sg = (ClAmsSGT*)entityRef.ptr;
    CL_ASSERT(sg != NULL);

    for(;;)
    {
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));
        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_SET_CONFIG:
            {
                ClAmsSGConfigT sgConfig = {{0}};
                AMS_CHECK_RC_ERROR(clAmsDBSGConfigUnmarshallVersion(inMsgHdl, &sgConfig, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetConfig(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &sgConfig.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_STATUS:
            {
                ClAmsSGStatusT sgStatus = {{0}};
                AMS_CHECK_RC_ERROR(clAmsDBSGStatusUnmarshallVersion(inMsgHdl, &sgStatus, versionCode));
                AMS_CHECK_RC_ERROR(clAmsDBEntityOpUnmarshall(entity, &sgStatus.entity, inMsgHdl, versionCode));
                AMS_CHECK_RC_ERROR(clAmsEntitySetStatus(&gAms.db.entityDb[entity->type],
                                                        entity,
                                                        &sgStatus.entity));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_LIST:
            {
                AMS_CHECK_RC_ERROR(clAmsDBListUnmarshall(entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_FAILOVER_HISTORY:
            {
                AMS_CHECK_RC_ERROR(clAmsDBSGFailoverHistoryUnmarshall(sg, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SET_TIMER:
            {
                AMS_CHECK_RC_ERROR(clAmsDBTimerUnmarshall(entityRef.ptr, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END_GROUP: return CL_OK;
            
        default:
            break;

        }
    }

    exitfn:
    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBSGDeXMLize(
       CL_IN  ClParserPtrT  sgPtr)
{
    ClRcT  rc = CL_OK;

    AMS_CHECKPTR (!sgPtr);

    const ClCharT  *name = NULL;

    name = clParserAttr(
            sgPtr,
            AMS_XML_TAG_NAME);

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,("sg Instance does not have name attribute \n"));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    ClParserPtrT  configPtr = clParserChild( sgPtr,AMS_XML_TAG_CONFIG );

    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SG[%s] does not have config tag \n", name));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    const ClCharT  *adminState = NULL;
    const ClCharT  *redundancyModel = NULL;
    const ClCharT  *loadingStrategy = NULL;
    const ClCharT  *failbackOption = NULL;
    const ClCharT  *instantiateDuration = NULL;
    const ClCharT  *numPrefActiveSUs = NULL;
    const ClCharT  *numPrefStandbySUs = NULL;
    const ClCharT  *numPrefInserviceSUs = NULL;
    const ClCharT  *numPrefAssignedSUs = NULL;
    const ClCharT  *numPrefActiveSUsPerSI = NULL;
    const ClCharT  *maxActiveSIsPerSU = NULL;
    const ClCharT  *maxStandbySIsPerSU = NULL;
    const ClCharT  *compRestartDuration = NULL;
    const ClCharT  *compRestartCountMax = NULL;
    const ClCharT  *suRestartDuration = NULL;
    const ClCharT  *suRestartCountMax = NULL;
    const ClCharT  *autoRepair = NULL;
    const ClCharT  *isCollocationAllowed = NULL;
    const ClCharT  *alphaFactor = NULL;
    const ClCharT  *betaFactor = NULL;

    adminState = clParserAttr(
            configPtr,
            AMS_XML_TAG_ADMIN_STATE);
            
    redundancyModel = clParserAttr(
            configPtr,
            "redundancyModel");
           
    loadingStrategy = clParserAttr(
            configPtr,
            "loadingStrategy");
          
    failbackOption = clParserAttr(
            configPtr,
            "failbackOption");

    autoRepair = clParserAttr(
            configPtr,
            "autoRepair");
         
    instantiateDuration = clParserAttr(
            configPtr,
            "instantiateDuration");
        
    numPrefActiveSUs = clParserAttr(
            configPtr,
            "numPrefActiveSUs");
       
    numPrefStandbySUs = clParserAttr(
            configPtr,
            "numPrefStandbySUs");
      
    numPrefInserviceSUs = clParserAttr(
            configPtr,
            "numPrefInserviceSUs");
     
    numPrefAssignedSUs = clParserAttr(
            configPtr,
            "numPrefAssignedSUs");
    
    numPrefActiveSUsPerSI = clParserAttr(
            configPtr,
            "numPrefActiveSUsPerSI");
   
    maxActiveSIsPerSU = clParserAttr(
            configPtr,
            "maxActiveSIsPerSU");
  
    maxStandbySIsPerSU = clParserAttr(
            configPtr,
            "maxStandbySIsPerSU");
 
    compRestartDuration = clParserAttr(
            configPtr,
            "compRestartDuration");

    compRestartCountMax = clParserAttr(
            configPtr,
            "compRestartCountMax");

    suRestartDuration = clParserAttr(
            configPtr,
            "suRestartDuration");

    suRestartCountMax = clParserAttr(
            configPtr,
            "suRestartCountMax");

    isCollocationAllowed = clParserAttr(
            configPtr,
            "isCollocationAllowed");

    alphaFactor = clParserAttr(
            configPtr,
            "alphaFactor");

    betaFactor = clParserAttr(
            configPtr,
            "betaFactor");

    if ( !adminState || !redundancyModel || !loadingStrategy
            || !failbackOption || !instantiateDuration || !numPrefActiveSUs
            || !numPrefStandbySUs || !numPrefInserviceSUs ||!numPrefAssignedSUs
            || !numPrefActiveSUsPerSI || !maxActiveSIsPerSU || !maxStandbySIsPerSU
            || !compRestartDuration || !compRestartCountMax || !suRestartDuration
            || !suRestartCountMax || !autoRepair || !isCollocationAllowed 
            || !alphaFactor 
            || !betaFactor)
    {
        AMS_LOG (CL_DEBUG_ERROR,("SG[%s] has a missing config attribute \n",name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }


    /*
     * Read the status portion
     */
    ClParserPtrT  statusPtr = clParserChild(sgPtr,AMS_XML_TAG_STATUS);

    const ClCharT  *isStarted = NULL;
    const ClCharT  *numCurrActiveSUs = NULL;
    const ClCharT  *numCurrStandbySUs = NULL;


    isStarted = clParserAttr (
            statusPtr,
            "isStarted");

    numCurrActiveSUs = clParserAttr (
            statusPtr,
            "numCurrActiveSUs");

    numCurrStandbySUs= clParserAttr (
            statusPtr,
            "numCurrStandbySUs");

    if ( !isStarted || !numCurrActiveSUs || !numCurrStandbySUs )
    {
        AMS_LOG (CL_DEBUG_ERROR,("SG[%s] status has a missing attribute \n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }

    ClAmsEntityRefT  entityRef = { {0},0,0};
    ClAmsSGT  sg;

    entityRef.entity.type = CL_AMS_ENTITY_TYPE_SG;
    strcpy (entityRef.entity.name.value,name);
    entityRef.entity.name.length = strlen(name) + 1 ;

#ifdef AMS_TEST_CKPT

    AMS_CHECK_RC_ERROR ( clAmsEntityDbDeleteEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                &entityRef) );

#endif

    AMS_CHECK_RC_ERROR ( clAmsEntityDbAddEntity(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                &entityRef) );

    /*
     * Write the config part
     */

    memcpy (&sg.config.entity, &entityRef.entity, sizeof (ClAmsEntityT));
    sg.config.adminState = atoi (adminState);
    sg.config.redundancyModel= atoi (redundancyModel);
    sg.config.loadingStrategy= atoi (loadingStrategy);
    sg.config.failbackOption= atoi (failbackOption);
    sg.config.autoRepair= atoi (autoRepair);
    sg.config.instantiateDuration= atol (instantiateDuration);
    sg.config.numPrefActiveSUs= atoi (numPrefActiveSUs);
    sg.config.numPrefStandbySUs= atoi (numPrefStandbySUs);
    sg.config.numPrefInserviceSUs= atoi (numPrefInserviceSUs);
    sg.config.numPrefAssignedSUs= atoi (numPrefAssignedSUs);
    sg.config.numPrefActiveSUsPerSI= atoi (numPrefActiveSUsPerSI);
    sg.config.maxActiveSIsPerSU= atoi (maxActiveSIsPerSU);
    sg.config.maxStandbySIsPerSU= atoi (maxStandbySIsPerSU);
    sg.config.compRestartDuration= atol (compRestartDuration);
    sg.config.compRestartCountMax= atoi (compRestartCountMax);
    sg.config.suRestartDuration= atol (suRestartDuration);
    sg.config.suRestartCountMax= atoi (suRestartCountMax);
    sg.config.isCollocationAllowed = atoi (isCollocationAllowed);
    sg.config.alpha = atoi (alphaFactor);
    sg.config.beta = atoi(betaFactor);

    AMS_CHECK_RC_ERROR ( clAmsEntitySetConfig (
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                &entityRef.entity,
                (ClAmsEntityConfigT *)&sg.config) );

    sg.status.isStarted = atoi(isStarted);
    sg.status.numCurrActiveSUs = atoi(numCurrActiveSUs);
    sg.status.numCurrStandbySUs = atoi(numCurrStandbySUs);

    AMS_CHECK_RC_ERROR( clAmsDBReadEntityStatus(
                &sg.status.entity,
                statusPtr) );

    AMS_CHECK_RC_ERROR ( clAmsEntitySetStatus(
                &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                &entityRef.entity,
                (ClAmsEntityStatusT *)&sg.status) );

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBSGListDeXMLize(
       CL_IN  ClParserPtrT  sgPtr)
{
    ClRcT  rc = CL_OK;
    const ClCharT  *name = NULL;
    ClAmsEntityTimerT  entityTimer = {0};

    name = clParserAttr (
            sgPtr,
            AMS_XML_TAG_NAME );

    if ( !name )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("sgName tag does not have node name attribute\n"));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
   
    ClParserPtrT  configPtr = clParserChild(sgPtr,AMS_XML_TAG_CONFIG);

    if ( !configPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("SGName[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    }
    
    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SG_CONFIG_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                configPtr,
                name,
                CL_AMS_SG_CONFIG_SI_LIST) );

    ClParserPtrT  statusPtr = clParserChild(sgPtr,AMS_XML_TAG_STATUS);

    if ( !statusPtr )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("sgName[%s] tag does not have config tag\n", name));
        rc = CL_ERR_NULL_POINTER;
        goto exitfn;
    } 

    entityTimer.count = 0;
    entityTimer.type = CL_AMS_SG_TIMER_INSTANTIATE;

    AMS_CHECK_RC_ERROR ( clAmsDBReadEntityTimer(
                statusPtr,
                &entityTimer,
                "instantiateTimer") );

    ClAmsEntityT  entity = {0};

    strcpy ( entity.name.value, name);
    entity.name.length = strlen (name) + 1;
    entity.type = CL_AMS_ENTITY_TYPE_SG;

    AMS_CHECK_RC_ERROR ( clAmsSetEntityTimer(
                &entity,
                &entityTimer) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                statusPtr,
                name,
                CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                statusPtr,
                name,
                CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                statusPtr,
                name,
                CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                statusPtr,
                name,
                CL_AMS_SG_STATUS_ASSIGNED_SU_LIST) );

    AMS_CHECK_RC_ERROR ( clAmsDBXMLToList(
                statusPtr,
                name,
                CL_AMS_SG_STATUS_FAULTY_SU_LIST) );

exitfn:

    return CL_AMS_RC (rc);

}


/*
 * Utility functions
 */

ClRcT   
clAmsDBClUint32ToStr(
        CL_IN  ClUint32T  number,
        CL_OUT  ClCharT  **str )
{
    AMS_CHECKPTR (!str);

    *str = clHeapAllocate (MAX_NUM_LENGTH_INT_32*sizeof(ClCharT));

    AMS_CHECK_NO_MEMORY (*str);

    memset (*str, 0, MAX_NUM_LENGTH_INT_32);
    sprintf (*str,"%d",number);

    return CL_OK;

}

ClRcT   
clAmsDBClUint8ToStr(
        CL_IN  ClUint8T  number,
        CL_OUT  ClCharT  **str )
{
    AMS_CHECKPTR (!str);

    *str = clHeapAllocate (MAX_NUM_LENGTH_INT_8*sizeof(ClCharT));

    AMS_CHECK_NO_MEMORY (*str);

    memset (*str, 0, MAX_NUM_LENGTH_INT_8);
    sprintf (*str,"%d",number);

    return CL_OK;

}


ClRcT   
clAmsDBClUint64ToStr(
       CL_IN  ClInt64T  number,
       CL_OUT  ClCharT  **str )
{
    AMS_CHECKPTR (!str);

    *str = clHeapAllocate (MAX_NUM_LENGTH_INT_64*sizeof(ClCharT));

    AMS_CHECK_NO_MEMORY (*str);

    memset (*str, 0, MAX_NUM_LENGTH_INT_64);
    sprintf (*str,"%lld",number);

    return CL_OK;

}

ClRcT   
clAmsDBClInt64ToStr(
        CL_IN  ClInt64T  number,
        CL_OUT  ClCharT  **str )
{
    AMS_CHECKPTR (!str);

    *str = clHeapAllocate (MAX_NUM_LENGTH_INT_64*sizeof(ClCharT));

    AMS_CHECK_NO_MEMORY (*str);

    memset (*str, 0, MAX_NUM_LENGTH_INT_64);
    sprintf (*str,"%lld",number);

    return CL_OK;

}

ClRcT   
clAmsDBListUnmarshall(ClAmsEntityT *entity, 
                      ClBufferHandleT inMsgHdl,
                      ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityListTypeT listType;
    ClUint32T listEntries = 0;
    ClUint32T i;
    ClAmsEntityRefT *newEntityRef = NULL;
    ClAmsEntityRefT entityRef = {{0}};

    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],
                                               &entityRef));
    CL_ASSERT(entityRef.ptr != NULL);

    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityListTypeT, 4, 0, 0)(inMsgHdl, &listType));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &listEntries));

    for(i = 0; i < listEntries; ++i)
    {
        ClAmsEntityRefT targetEntityRef = {{0}};
        ClBoolT setRef = CL_FALSE;

        switch (listType)
        {

        case CL_AMS_NODE_CONFIG_SU_LIST:
        case CL_AMS_SG_CONFIG_SU_LIST:
        case CL_AMS_SG_CONFIG_SI_LIST:
        case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
        case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
        case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
        case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
        case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
        case CL_AMS_SU_CONFIG_COMP_LIST:
        case CL_AMS_SI_CONFIG_CSI_LIST:
            setRef = CL_TRUE;
            /*fall through*/

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
        case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
        case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
        case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
        case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
        case CL_AMS_SI_CONFIG_SU_RANK_LIST:
            {
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, 
                                                                     &targetEntityRef.entity));

                if((rc = clAmsAddToEntityList(entity, 
                                              &targetEntityRef.entity, 
                                              listType)) != CL_OK)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Entity list add returned [%#x] while adding "\
                                             "entity [%.*s] to entity [%.*s] list [%d]\n",
                                             rc, targetEntityRef.entity.name.length-1, 
                                             targetEntityRef.entity.name.value,
                                             entity->name.length-1, entity->name.value, listType));
                    goto exitfn;
                }
                if(setRef)
                {
                    clAmsEntitySetRefPtr(targetEntityRef, entityRef);
                }
                break;
            }

        case CL_AMS_SU_STATUS_SI_LIST:
            {
                ClAmsSUSIRefT *suSIRef = NULL;
                ClAmsSUT *su =  NULL;

                suSIRef = clHeapCalloc(1, sizeof(*suSIRef));
                CL_ASSERT(suSIRef != NULL);

                newEntityRef = &suSIRef->entityRef;

                su = (ClAmsSUT*)entityRef.ptr;

                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSUSIRefT, 4, 0, 0)(inMsgHdl, suSIRef));
 
                /*
                 * Fill up the reference pointer.
                 */
                AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[suSIRef->entityRef.entity.type],
                                                           &suSIRef->entityRef));

                AMS_CHECK_RC_ERROR(clAmsEntityListAddEntityRef(&su->status.siList,
                                                               &suSIRef->entityRef,
                                                               0));
                break;
            }

        case CL_AMS_COMP_STATUS_CSI_LIST:
            {
                ClAmsCompCSIRefT *compCSIRef = NULL;
                ClAmsCompT *comp = NULL;
                ClAmsEntityRefT activeRef = {{0}};
                ClAmsEntityRefT *pActiveRef = NULL;

                compCSIRef = clHeapCalloc(1, sizeof(*compCSIRef));
                CL_ASSERT(compCSIRef != NULL);

                newEntityRef = &compCSIRef->entityRef;

                comp = (ClAmsCompT*)entityRef.ptr;
  
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCompCSIRefT, 4, 0, 0)(inMsgHdl, 
                                                                   compCSIRef));
                
                if(compCSIRef->activeComp)
                {
                    memcpy(&activeRef.entity, compCSIRef->activeComp, sizeof(activeRef.entity));
                    clHeapFree(compCSIRef->activeComp);
                    compCSIRef->activeComp = NULL;
                    pActiveRef = &activeRef;
                }

                AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[compCSIRef->entityRef.entity.type],
                                                           &compCSIRef->entityRef));

                /* 
                 * fill up active comp reference
                 */
                if(pActiveRef)
                {
                    AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                                               &activeRef));
                    
                    compCSIRef->activeComp = activeRef.ptr;
                }

                AMS_CHECK_RC_ERROR(clAmsEntityListAddEntityRef(&comp->status.csiList,
                                                               &compCSIRef->entityRef,
                                                               0));

                break;
            }

        case CL_AMS_SI_STATUS_SU_LIST:
            {
                ClAmsSISURefT *siSURef = NULL;
                ClAmsSIT *si = NULL;

                siSURef = clHeapCalloc(1, sizeof(*siSURef));
                CL_ASSERT(siSURef != NULL);

                newEntityRef = &siSURef->entityRef;

                si = (ClAmsSIT*)entityRef.ptr;

                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsSISURefT, 4, 0, 0)(inMsgHdl, siSURef));
                AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[siSURef->entityRef.entity.type],
                                                           &siSURef->entityRef));

                AMS_CHECK_RC_ERROR(clAmsEntityListAddEntityRef(&si->status.suList,
                                                               &siSURef->entityRef,
                                                               0));
                break;
            }

        case CL_AMS_CSI_STATUS_PG_LIST:
            {
                ClAmsCSICompRefT *csiCompRef = NULL;
                ClAmsCSIT *csi = NULL;
                
                csiCompRef = clHeapCalloc(1, sizeof(*csiCompRef));
                CL_ASSERT(csiCompRef != NULL);

                newEntityRef = &csiCompRef->entityRef;

                csi = (ClAmsCSIT*)entityRef.ptr;
                 
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSICompRefT, 4, 0, 0)(inMsgHdl, csiCompRef));
                AMS_CHECK_RC_ERROR(clAmsEntityDbFindEntity(&gAms.db.entityDb[csiCompRef->entityRef.entity.type],
                                                           &csiCompRef->entityRef));

                AMS_CHECK_RC_ERROR(clAmsEntityListAddEntityRef(&csi->status.pgList,
                                                               &csiCompRef->entityRef,
                                                               0));
                break;
                
            }

        case CL_AMS_CSI_CONFIG_NVP_LIST:
            {
                ClAmsCSINVPT nvp;
                memset(&nvp, 0, sizeof(nvp));
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsCSINVPT, 4, 0, 0)(inMsgHdl, &nvp));
                AMS_CHECK_RC_ERROR(clAmsCSISetNVP(gAms.db.entityDb[entity->type],
                                                  entityRef.entity,
                                                  nvp));
                break;
            }

        case CL_AMS_CSI_PGTRACK_CLIENT_LIST:
            {
                ClAmsCSIPGTrackClientT *pgTrackClient = NULL;
                ClIocPhysicalAddressT address = {0};
                ClAmsPGTrackFlagT trackFlags = 0;

                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClIocPhysicalAddressT, 4, 0, 0)(inMsgHdl, &address));
                AMS_CHECK_RC_ERROR(clXdrUnmarshallClUint32T(inMsgHdl, &trackFlags));

                pgTrackClient = clHeapCalloc(1, sizeof(*pgTrackClient));
                CL_ASSERT(pgTrackClient != NULL);

                memcpy(&pgTrackClient->address.iocPhyAddress, &address, sizeof(pgTrackClient->address.iocPhyAddress));
                pgTrackClient->trackFlags = trackFlags;

                rc = clAmsCSIAddToPGTrackList(&gAms.db.entityDb[entity->type],
                                              entity,
                                              pgTrackClient,
                                              NULL);
                if(rc != CL_OK)
                {
                    clHeapFree(pgTrackClient);
                    goto exitfn;
                }

                break;
            }
            
        default:
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Invalid list type [%d] in Function [%s] \n",
                                           listType, __FUNCTION__));
                break;
            }
        }

        newEntityRef = NULL;
    }

    exitfn:
    if(newEntityRef)
        clHeapFree(newEntityRef);

    return CL_AMS_RC (rc);
}

ClRcT   
clAmsDBXMLToList(
       CL_IN  ClParserPtrT  tagPtr,
       CL_IN  const ClCharT  *sourceEntityName,
       CL_IN  ClAmsEntityListTypeT  listType )
{
    ClRcT  rc = CL_OK;
    ClAmsEntityTypeT  sourceEntityType = 0;
    ClAmsEntityTypeT  targetEntityType = 0;
    ClParserPtrT  listPtr = NULL;
    ClCharT  *listTag = NULL;
    ClCharT  *list2Tag = NULL;
    const ClCharT  *targetEntityName = NULL;

    AMS_CHECKPTR (!sourceEntityName || !tagPtr);

    switch (listType)
    {

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_NODE;
                sourceEntityType = CL_AMS_ENTITY_TYPE_NODE;
                targetEntityType = CL_AMS_ENTITY_TYPE_NODE;
                break;
            }

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_NODE;
                sourceEntityType = CL_AMS_ENTITY_TYPE_NODE;
                targetEntityType = CL_AMS_ENTITY_TYPE_NODE;
                break;
            } 

        case CL_AMS_NODE_CONFIG_SU_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_NODE;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_CONFIG_SU_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_CONFIG_SI_LIST:
            {
                listTag = AMS_XML_TAG_SI_LIST;
                list2Tag = AMS_XML_TAG_SI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SI;
                break;
            }

        case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
            {
                listTag = AMS_XML_TAG_INSTANTIABLE_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
            {
                listTag = AMS_XML_TAG_INSTANTIATED_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
            {
                listTag = AMS_XML_TAG_IN_SERVICE_SPARE_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
            {
                listTag = AMS_XML_TAG_ASSIGNED_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
            {
                listTag = AMS_XML_TAG_FAULTY_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SG;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }

        case CL_AMS_SU_CONFIG_COMP_LIST:
            {
                listTag = AMS_XML_TAG_COMP_LIST;
                list2Tag = AMS_XML_TAG_COMP;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SU;
                targetEntityType = CL_AMS_ENTITY_TYPE_COMP;
                break;
            }

        case CL_AMS_SU_STATUS_SI_LIST:
            {
                listTag = AMS_XML_TAG_SI_LIST;
                list2Tag = AMS_XML_TAG_SI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SU;
                targetEntityType = CL_AMS_ENTITY_TYPE_SI;
                break;
            }

        case CL_AMS_COMP_STATUS_CSI_LIST:
            {
                listTag = AMS_XML_TAG_CSI_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_COMP;
                targetEntityType = CL_AMS_ENTITY_TYPE_CSI;
                break;
            }

        case CL_AMS_SI_CONFIG_SU_RANK_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SI;
                targetEntityType = CL_AMS_ENTITY_TYPE_SU;
                break;
            }
        case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_SI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SI;
                targetEntityType = CL_AMS_ENTITY_TYPE_SI;
                break;
            }
        case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_SI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SI;
                targetEntityType = CL_AMS_ENTITY_TYPE_SI;
                break;
            }
        case CL_AMS_SI_CONFIG_CSI_LIST:
            {
                listTag = AMS_XML_TAG_CSI_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_SI;
                targetEntityType = CL_AMS_ENTITY_TYPE_CSI;
                break;
            }
        case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_CSI;
                targetEntityType = CL_AMS_ENTITY_TYPE_CSI;
                break;
            }
        case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                sourceEntityType = CL_AMS_ENTITY_TYPE_CSI;
                targetEntityType = CL_AMS_ENTITY_TYPE_CSI;
                break;
            }

        default:
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Invalid list type [%d] in Function [%s] \n",
                            listType, __FUNCTION__));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_LIST);
            }
    }

    listPtr = clParserChild( tagPtr,listTag);

    if ( !listPtr)
    {
        return CL_OK;
    }

    ClParserPtrT  list2Ptr = NULL;
    ClAmsEntityT  sourceEntity = {0};
    ClAmsEntityT  targetEntity = {0};

    strcpy (sourceEntity.name.value, sourceEntityName);
    sourceEntity.name.length = strlen (sourceEntityName) + 1;
    sourceEntity.type = sourceEntityType;
    targetEntity.type = targetEntityType;

    list2Ptr = clParserChild( listPtr,list2Tag);

    if ( !list2Ptr)
    {
        return CL_OK;
    }

    while ( list2Ptr )
    {

        targetEntityName = clParserAttr (
                list2Ptr,
                AMS_XML_TAG_NAME );

        AMS_CHECKPTR_AND_EXIT (!targetEntityName);

        strcpy (targetEntity.name.value, targetEntityName);
        targetEntity.name.length = strlen (targetEntityName) + 1;

        AMS_CHECK_RC_ERROR ( clAmsAddToEntityList(
                    &sourceEntity,
                    &targetEntity,
                    listType) );

        list2Ptr = list2Ptr->next;

    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBListMarshall(
       ClAmsEntityListT  *list,
       ClAmsEntityListTypeT  listType,
       ClBufferHandleT inMsgHdl,
       ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  *entityRef = NULL;
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_SET_LIST;

    AMS_CHECKPTR (!list);

    /* 
     * Write the op. code and the list type. and number of entries.
     */
    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0) );
    AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR(clXdrMarshallClUint32T(&list->numEntities, inMsgHdl, 0));

    if ( listType == CL_AMS_COMP_STATUS_CSI_LIST )
    {
        
        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsCompCSIRefT  *csiRef = (ClAmsCompCSIRefT *)entityRef;

            AMS_CHECKPTR_AND_EXIT(!csiRef);

            if ( (csiRef->activeComp == NULL) )
            {
                AMS_LOG (CL_DEBUG_ERROR, ("Error: Component CSI reference with " 
                            "missing active component for CSI [%s] \n", 
                            csiRef->entityRef.entity.name.value));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_COMP);
            }
            
            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsCompCSIRefT, 4, 0, 0)(csiRef, inMsgHdl, 0) );

        }
    }

    else if ( listType == CL_AMS_SU_STATUS_SI_LIST )
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsSUSIRefT  *siRef = (ClAmsSUSIRefT *)entityRef;

            AMS_CHECKPTR_AND_EXIT ( !siRef );

            AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsSUSIRefT, 4, 0, 0)(siRef, inMsgHdl, 0) );

        }
    }

    else if ( listType == CL_AMS_SI_STATUS_SU_LIST )
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsSISURefT  *suRef = (ClAmsSISURefT *)entityRef;

            AMS_CHECKPTR_AND_EXIT ( !suRef);
            
            AMS_CHECK_RC_ERROR (VDECL_VER(clXdrMarshallClAmsSISURefT, 4, 0, 0)(suRef, inMsgHdl, 0) );

        }
    }
    else if ( listType == CL_AMS_CSI_STATUS_PG_LIST )
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 
            
            ClAmsCSICompRefT  *compRef = (ClAmsCSICompRefT*)entityRef;

            AMS_CHECKPTR_AND_EXIT ( !compRef);

            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsCSICompRefT, 4, 0, 0)(compRef, inMsgHdl, 0));
        }
    }
    else
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 
            AMS_CHECK_RC_ERROR(VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(&entityRef->entity, inMsgHdl, 0));
        }
    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBListToXML(
       CL_IN  ClParserPtrT  tagPtr,
       CL_IN  ClAmsEntityListT  *list,
       CL_IN  ClAmsEntityListTypeT  listType )
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  *entityRef = NULL;
    ClParserPtrT  listPtr = NULL;
    ClCharT  *listTag = NULL;
    ClCharT  *list2Tag = NULL;

    AMS_CHECKPTR (!list || !tagPtr);

    switch (listType)
    { 

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_NODE;
                break;
            }

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_NODE;
                break;
            }

        case CL_AMS_NODE_CONFIG_SU_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_CONFIG_SU_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_CONFIG_SI_LIST:
            {
                listTag = AMS_XML_TAG_SI_LIST;
                list2Tag = AMS_XML_TAG_SI;
                break;
            }

        case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
            {
                listTag = AMS_XML_TAG_INSTANTIABLE_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
            {
                listTag = AMS_XML_TAG_INSTANTIATED_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
            {
                listTag = AMS_XML_TAG_IN_SERVICE_SPARE_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
            {
                listTag = AMS_XML_TAG_ASSIGNED_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
            {
                listTag = AMS_XML_TAG_FAULTY_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SU_CONFIG_COMP_LIST:
            {
                listTag = AMS_XML_TAG_COMP_LIST;
                list2Tag = AMS_XML_TAG_COMP;
                break;
            }

        case CL_AMS_SU_STATUS_SI_LIST:
            {
                listTag = AMS_XML_TAG_SI_LIST;
                list2Tag = AMS_XML_TAG_SI;
                break;
            }

        case CL_AMS_COMP_STATUS_CSI_LIST:
            {
                listTag = AMS_XML_TAG_CSI_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                break;
            }

        case CL_AMS_SI_CONFIG_SU_RANK_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_SI;
                break;
            }

        case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_SI;
                break;
            }

        case CL_AMS_SI_CONFIG_CSI_LIST:
            {
                listTag = AMS_XML_TAG_CSI_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                break;
            }

        case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENTS_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                break;
            }

        case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
            {
                listTag = AMS_XML_TAG_DEPENDENCIES_LIST;
                list2Tag = AMS_XML_TAG_CSI;
                break;
            }

        case CL_AMS_SI_STATUS_SU_LIST:
            {
                listTag = AMS_XML_TAG_SU_LIST;
                list2Tag = AMS_XML_TAG_SU;
                break;
            }

        case CL_AMS_CSI_STATUS_PG_LIST:
            {
                listTag = AMS_XML_TAG_PG_LIST;
                list2Tag = AMS_XML_TAG_PG;
                break;
            }

        default:
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Invalid list type [%d] in Function [%s] \n",
                            listType, __FUNCTION__));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_LIST);
            } 

    }

    listPtr = CL_PARSER_ADD_CHILD( tagPtr,listTag, 0);

    AMS_CHECKPTR_AND_EXIT (!listPtr);

    if ( listType == CL_AMS_COMP_STATUS_CSI_LIST )
    {
        
        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsCompCSIRefT  *csiRef = (ClAmsCompCSIRefT *)entityRef;
            ClCharT  *haState = NULL;
            ClCharT  *td  = NULL;
            ClCharT  *rank = NULL;
            ClCharT  *pendingOp = NULL;

            if ( (csiRef != NULL) && (csiRef->activeComp == NULL) )
            {
                AMS_LOG (CL_DEBUG_ERROR, ("Error: Component CSI reference with " 
                            "missing active component for CSI [%s] \n", 
                            csiRef->entityRef.entity.name.value));
                return CL_AMS_RC (CL_AMS_ERR_INVALID_COMP);
            }

            ClParserPtrT  list2Ptr = CL_PARSER_ADD_CHILD(
                    listPtr,list2Tag, 0);

            AMS_CHECKPTR_AND_EXIT ( !list2Ptr || !csiRef)

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        csiRef->haState,
                        &haState) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        csiRef->tdescriptor,
                        &td) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        csiRef->rank,
                        &rank) ); 
            
            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        csiRef->pendingOp,
                        &pendingOp) );

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    AMS_XML_TAG_NAME,
                    csiRef->entityRef.entity.name.value);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "haState",
                    haState);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "td",
                    td);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "rank",
                    rank); 
            
            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "activeComp",
                    csiRef->activeComp->name.value);
           
            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "pendingOp",
                    pendingOp);

            clAmsFreeMemory (haState);
            clAmsFreeMemory (td);
            clAmsFreeMemory (rank);
            clAmsFreeMemory (pendingOp);

        }
    }

    else if ( listType == CL_AMS_SU_STATUS_SI_LIST )
    {

        ClCharT  *haState = NULL;
        ClCharT  *numActiveCSIs = NULL;
        ClCharT  *numStandbyCSIs = NULL;
        ClCharT  *numQuiescedCSIs = NULL;
        ClCharT  *numQuiescingCSIs = NULL;

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsSUSIRefT  *siRef = (ClAmsSUSIRefT *)entityRef;

            AMS_CHECKPTR_AND_EXIT ( !siRef );

            ClParserPtrT  list2Ptr = CL_PARSER_ADD_CHILD(
                    listPtr,list2Tag, 0);

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        siRef->haState,
                        &haState) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        siRef->numActiveCSIs,
                        &numActiveCSIs) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        siRef->numStandbyCSIs,
                        &numStandbyCSIs) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        siRef->numQuiescedCSIs,
                        &numQuiescedCSIs) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        siRef->numQuiescingCSIs,
                        &numQuiescingCSIs) );

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    AMS_XML_TAG_NAME,
                    siRef->entityRef.entity.name.value);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "haState",
                    haState);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "numActiveCSIs",
                    numActiveCSIs);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "numStandbyCSIs",
                    numStandbyCSIs);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "numQuiescedCSIs",
                    numQuiescedCSIs);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "numQuiescingCSIs",
                    numQuiescingCSIs);


            clAmsFreeMemory (haState);
            clAmsFreeMemory (numActiveCSIs);
            clAmsFreeMemory (numStandbyCSIs);
            clAmsFreeMemory (numQuiescedCSIs);
            clAmsFreeMemory (numQuiescingCSIs);

        }
    }

    else if ( listType == CL_AMS_SI_STATUS_SU_LIST )
    {

        ClCharT  *haState = NULL;
        ClCharT  *suRank  = NULL;

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClAmsSISURefT  *suRef = (ClAmsSISURefT *)entityRef;


            AMS_CHECKPTR_AND_EXIT ( !suRef);

            ClParserPtrT  list2Ptr = CL_PARSER_ADD_CHILD(
                    listPtr,list2Tag, 0);

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        suRef->haState,
                        &haState) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        suRef->rank,
                        &suRank) );

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    AMS_XML_TAG_NAME,
                    suRef->entityRef.entity.name.value);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "haState",
                    haState);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "suRank",
                    suRank);

            clAmsFreeMemory (haState);
            clAmsFreeMemory (suRank);

        }
    }
    else if ( listType == CL_AMS_CSI_STATUS_PG_LIST )
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 
            
            ClAmsCSICompRefT  *compRef = (ClAmsCSICompRefT*)entityRef;
            ClCharT  *haState = NULL;
            ClCharT  *rank = NULL;

            AMS_CHECKPTR_AND_EXIT ( !compRef);

            ClParserPtrT  list2Ptr = CL_PARSER_ADD_CHILD(
                    listPtr,list2Tag, 0);

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        compRef->haState,
                        &haState) );

            AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                        compRef->rank,
                        &rank) );

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    AMS_XML_TAG_NAME,
                    compRef->entityRef.entity.name.value);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "haState",
                    haState);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    "rank",
                    rank);

            clAmsFreeMemory (haState);
            clAmsFreeMemory (rank);

        }
    }
    else
    {

        for ( entityRef=clAmsEntityListGetFirst(list);
                entityRef != (ClAmsEntityRefT *) NULL;
                entityRef = clAmsEntityListGetNext(list, entityRef) )
        { 

            ClParserPtrT  list2Ptr = CL_PARSER_ADD_CHILD(
                    listPtr,list2Tag, 0);

            CL_PARSER_SET_ATTR (
                    list2Ptr,
                    AMS_XML_TAG_NAME,
                    entityRef->entity.name.value);

        }
    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsDBTimerUnmarshall(ClAmsEntityT *entity, 
                       ClBufferHandleT inMsgHdl,
                       ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    ClAmsEntityTimerT entityTimer = {0};

    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &entityTimer.type));
    AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &entityTimer.count));

    entityTimer.entity = entity;

    AMS_CHECK_RC_ERROR(clAmsSetEntityTimer(entity, &entityTimer));

    exitfn:
    return CL_AMS_RC(rc);
}

ClRcT
clAmsDBTimerMarshall(ClAmsEntityTimerT *entityTimer,
                     ClAmsEntityTimerTypeT timer,
                     ClBufferHandleT inMsgHdl,
                     ClUint32T versionCode)
{
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_SET_TIMER;
    ClRcT rc = CL_OK;

    AMS_CHECK_RC_ERROR (clXdrMarshallClInt32T(&op, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR (clXdrMarshallClInt32T(&timer, inMsgHdl, 0));
    AMS_CHECK_RC_ERROR (clXdrMarshallClInt32T(&entityTimer->count, inMsgHdl, 0));

    exitfn:
    return CL_OK;

}

ClRcT
clAmsDBReadEntityTimer(
        CL_IN  ClParserPtrT  tagPtr,
        CL_OUT  ClAmsEntityTimerT  *entityTimer,
        CL_IN  ClCharT  *timerTag )
{
    const ClCharT  *timerRunning = NULL;

    AMS_CHECKPTR ( !entityTimer || !timerTag || !tagPtr); 
    
    timerRunning = clParserAttr(tagPtr,timerTag);

    AMS_CHECKPTR ( !timerRunning );

    if (!strcmp (timerRunning,"1"))
    {
        entityTimer->count = 1;
    }
    else
    {
        entityTimer->count = 0;
    }

    return CL_OK;

}

ClRcT
clAmsDBWriteEntityTimer(
        CL_IN  ClParserPtrT  tagPtr,
        CL_IN  ClAmsEntityTimerT  *entityTimer,
        CL_IN  ClCharT  *timerTag )
{
    AMS_CHECKPTR ( !entityTimer || !timerTag ||!tagPtr ); 

    if ( entityTimer->count )
    {
        CL_PARSER_SET_ATTR(
            tagPtr,timerTag,"1");
    }
    else
    {
        CL_PARSER_SET_ATTR(
            tagPtr,timerTag,"0");
    }

    return CL_OK;

}

static ClRcT
clAmsEntityDBUnmarshall(ClAmsEntityT *entity,
                        ClBufferHandleT inMsgHdl,
                        ClUint32T versionCode)
{
    ClRcT rc = CL_OK;

    AMS_CHECK_ENTITY_TYPE(entity->type);

    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_NODE:
        {
            AMS_CHECK_RC_ERROR(clAmsDBNodeUnmarshall(entity, inMsgHdl, versionCode));
            break;
        }

    case CL_AMS_ENTITY_TYPE_SG:
        {
            AMS_CHECK_RC_ERROR(clAmsDBSGUnmarshall(entity, inMsgHdl, versionCode));
            break;
        }

    case CL_AMS_ENTITY_TYPE_SU:
        {
            AMS_CHECK_RC_ERROR(clAmsDBSUUnmarshall(entity, inMsgHdl, versionCode));
            break;
        }

    case CL_AMS_ENTITY_TYPE_SI:
        {
            AMS_CHECK_RC_ERROR(clAmsDBSIUnmarshall(entity, inMsgHdl, versionCode));
            break;
        }
        
    case CL_AMS_ENTITY_TYPE_COMP:
        {
            AMS_CHECK_RC_ERROR(clAmsDBCompUnmarshall(entity, inMsgHdl, versionCode));
            break;
        }

    case CL_AMS_ENTITY_TYPE_CSI:
        {
            AMS_CHECK_RC_ERROR(clAmsDBCSIUnmarshall(entity, inMsgHdl,versionCode));
            break;
        }
    default:
        rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        goto exitfn;
    }

    AMS_LOG(CL_DEBUG_TRACE, ("[%s] entity unmarshalled\n",
                             CL_AMS_STRING_ENTITY_TYPE(entity->type)));
    exitfn:
    return rc;
}

static ClRcT clAmsDBUnmarshallVersion(ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;
    for(;;)
    {
        ClAmsCkptOperationT op = 0;
        ClAmsEntityT entity = {0};
        AMS_CHECK_RC_ERROR(clXdrUnmarshallClInt32T(inMsgHdl, &op));
        switch(op)
        {
        case CL_AMS_CKPT_OPERATION_START_GROUP:
            {
                AMS_CHECK_RC_ERROR(VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsgHdl, &entity));
                AMS_CHECK_RC_ERROR(clAmsEntityDBUnmarshall(&entity, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_SERVER_DATA:
            {
                AMS_CHECK_RC_ERROR(clAmsServerDataUnmarshall(&gAms, inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_USER_DATA:
            {
                AMS_CHECK_RC_ERROR(clAmsEntityUserDataUnmarshall(inMsgHdl, versionCode));
                break;
            }

        case CL_AMS_CKPT_OPERATION_END:
            AMS_LOG(CL_DEBUG_INFO, ("DB unmarshalling done\n"));
            return CL_OK;

        default:
            AMS_LOG(CL_DEBUG_ERROR, ("AMS db unmarshall returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    exitfn:
    return CL_AMS_RC(rc);
}

ClRcT 
clAmsDBUnmarshall(ClBufferHandleT inMsgHdl)
{
    ClRcT rc = CL_OK;
    ClVersionT version = {0};
    ClUint32T versionCode = 0;

    AMS_CHECK_RC_ERROR(clXdrUnmarshallClVersionT(inMsgHdl, &version));

    switch((versionCode = CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion)))
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
    case CL_VERSION_CODE(CL_RELEASE_VERSION, 1, CL_MINOR_VERSION):
    case CL_VERSION_CODE(5, 0, 0):
        {
            rc = clAmsDBUnmarshallVersion(inMsgHdl, versionCode);
        }
        break;

    default:
        {
            clLogError("DB", "UNMARSHALL", "AMS db unsupported version [%d.%d.%d]",
                       version.releaseCode, version.majorVersion, version.minorVersion);
            rc = CL_AMS_RC(CL_ERR_VERSION_MISMATCH);
            break;
        }
    }

    exitfn:
    return rc;
}

ClRcT
clAmsDBConfigDeserialize(ClBufferHandleT inMsgHdl)
{
    /* 
     * db unmarshall is generic enough. So use it.
     */
    return clAmsDBUnmarshall(inMsgHdl);
}

ClRcT
clAmsDBUnpackMain(
        CL_IN  ClCharT  *fileName)
{
    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !fileName );

    amfPtr.headPtr = clParserParseFile(fileName);

    if (!amfPtr.headPtr)
    {
        AMS_LOG (CL_DEBUG_ERROR, 
                (" Error in reading the XML file \n"));
    }

    amfPtr.gAmsPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_GAMS);

    amfPtr.nodeNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_NODE_NAMES);

    amfPtr.sgNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_SG_NAMES);

    amfPtr.suNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_SU_NAMES);

    amfPtr.siNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_SI_NAMES);

    amfPtr.compNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_COMP_NAMES);

    amfPtr.csiNamesPtr = clParserChild(
            amfPtr.headPtr, AMS_XML_TAG_CSI_NAMES);

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.nodeNamesPtr,
                clAmsDBNodeDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.sgNamesPtr,
                clAmsDBSGDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.suNamesPtr,
                clAmsDBSUDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.siNamesPtr,
                clAmsDBSIDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.compNamesPtr,
                clAmsDBCompDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.csiNamesPtr,
                clAmsDBCSIDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.nodeNamesPtr,
                clAmsDBNodeListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.sgNamesPtr,
                clAmsDBSGListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.suNamesPtr,
                clAmsDBSUListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.siNamesPtr,
                clAmsDBSIListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.compNamesPtr,
                clAmsDBCompListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBParseEntityNames(
                amfPtr.csiNamesPtr,
                clAmsDBCSIListDeXMLize) );

    AMS_CHECK_RC_ERROR ( clAmsDBGAmsDeXMLize(
                amfPtr.gAmsPtr,
                &gAms ) );

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsDBParseEntityNames (
        CL_IN  ClParserPtrT  namesPtr,
        CL_IN  ClRcT  (*fn)  (ClParserPtrT  namePtr))
{
    ClRcT  rc = CL_OK;
    ClCharT name[16] = {0} ;

    AMS_CHECKPTR (!namesPtr || !fn);
    
    if ( !strcmp (namesPtr->name,AMS_XML_TAG_NODE_NAMES))
    {
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_NODE_NAME);
    }

    else if ( !strcmp (namesPtr->name,AMS_XML_TAG_SG_NAMES))
    {
    
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_SG_NAME);
    }

    else if ( !strcmp (namesPtr->name,AMS_XML_TAG_SU_NAMES))
    {
    
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_SU_NAME);
    }

    else if ( !strcmp (namesPtr->name,AMS_XML_TAG_SI_NAMES))
    {
    
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_SI_NAME);
    }

    else if ( !strcmp (namesPtr->name,AMS_XML_TAG_COMP_NAMES))
    {
    
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_COMP_NAME);
    }

    else if ( !strcmp (namesPtr->name,AMS_XML_TAG_CSI_NAMES))
    {
        memset (name,0,16);
        strcpy (name,AMS_XML_TAG_CSI_NAME);
    }

    ClParserPtrT  namePtr = clParserChild (namesPtr,name);

    if ( !namePtr )
    {
        return CL_OK;
    }

    while ( namePtr )
    {

        AMS_CHECK_RC_ERROR( (*fn)(namePtr) );
        namePtr=namePtr->next;

    }

exitfn:

    return CL_AMS_RC (rc);

}

static ClRcT clAmsCCBHandleDBCallback(ClHandleDatabaseHandleT db, ClHandleT handle, ClPtrT cookie)
{
    ClAmsCCBHandleDBWalkArgT *arg = cookie;
    ClInt32T nhandles = arg->nhandles;
    arg->pHandles = clHeapRealloc(arg->pHandles, (nhandles+1)*sizeof(ClHandleT));
    CL_ASSERT(arg->pHandles != NULL);
    arg->pHandles[nhandles] = handle;
    ++arg->nhandles;
    return CL_OK;
}
                                    
static ClRcT clAmsCCBHandleDBMarshall(ClBufferHandleT inMsgHdl)
{
    ClAmsCCBHandleDBWalkArgT arg ;
    ClRcT rc = CL_OK;
    ClInt32T i = 0;

    memset(&arg, 0, sizeof(arg));
    rc = clHandleWalk(gAms.ccbHandleDB, clAmsCCBHandleDBCallback, (ClPtrT)&arg);
    if(rc != CL_OK)
    {
        arg.nhandles = 0;
    }
    AMS_CHECK_RC_ERROR ( clXdrMarshallClInt32T(&arg.nhandles, inMsgHdl, 0) );
    for(i = 0; i < arg.nhandles; ++i)
    {
        AMS_CHECK_RC_ERROR ( clXdrMarshallClHandleT(arg.pHandles+i, inMsgHdl, 0) );
    }
    exitfn:
    if(arg.pHandles)
        clHeapFree(arg.pHandles);
    return rc;
}

static ClRcT clAmsCCBHandleDeleteCallback(ClHandleDatabaseHandleT db, ClHandleT handle, ClPtrT cookie)
{
    clAmsMgmtCCBT *ccbInstance = NULL;
    ClRcT rc = CL_OK;
    rc = clHandleCheckout(db, handle, (ClPtrT*)&ccbInstance);
    if(rc != CL_OK) return rc;
    clAmsCCBOpListTerminate(&ccbInstance->ccbOpListHandle);
    clHandleDestroy(db, handle);
    clHandleCheckin(db, handle);
    return CL_OK;
}

static ClRcT clAmsCCBHandlesDelete(void)
{
    return clHandleWalk(gAms.ccbHandleDB, clAmsCCBHandleDeleteCallback, NULL);
}

static ClRcT clAmsCCBHandleDBUnmarshall(ClBufferHandleT inMsgHdl)
{
    ClInt32T i = 0;
    ClInt32T nhandles = 0;
    ClHandleT *pHandles = NULL;
    ClRcT rc = CL_OK;

    /*
     * Faster to rip this small handle db out and recreate then merging changes.
     */
    
    clAmsCCBHandlesDelete();

    rc = clXdrUnmarshallClInt32T(inMsgHdl, &nhandles);
    if(rc != CL_OK)
        return rc;

    if(nhandles == 0) return CL_OK;

    pHandles = clHeapCalloc(nhandles, sizeof(ClHandleT));
    CL_ASSERT(pHandles != NULL);
    for(i = 0 ; i < nhandles; ++i)
    {
        ClHandleT handle = 0;
        rc = clXdrUnmarshallClHandleT(inMsgHdl, &handle);
        if(rc != CL_OK)
            goto out_free;
        pHandles[i] = handle;
    }

    for(i = 0; i < nhandles; ++i)
    {
        clAmsMgmtCCBT *ccbInstance = NULL;
        rc = clHandleCreateSpecifiedHandle(gAms.ccbHandleDB, (ClUint32T)sizeof(clAmsMgmtCCBT), pHandles[i]);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("DB unmarshall ccbhandle create [%#llx] returned [%#x]\n",
                                     pHandles[i], rc));
            goto out_free;
        }
        rc = clHandleCheckout(gAms.ccbHandleDB, pHandles[i], (ClPtrT)&ccbInstance);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("DB unmarshall ccb handle checkout [%#llx] returned [%#x]\n",
                                     pHandles[i], rc));
            goto out_free;
        }
        rc = clAmsCCBOpListInstantiate(&ccbInstance->ccbOpListHandle);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("DB unmarshall ccb handle list instantiate [%llx] returned [%#x]\n",
                                     pHandles[i], rc));
            goto out_free;
        }

        rc = clHandleCheckin(gAms.ccbHandleDB, pHandles[i]);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("DB unmarshall ccb handle checkin [%llx] returned [%#x]\n",
                                     pHandles[i], rc));
            goto out_free;
        }
    }

    rc = CL_OK;

    out_free:
    if(pHandles)
        clHeapFree(pHandles);

    return rc;
}

ClRcT
clAmsServerDataMarshall(ClAmsT *ams, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_SERVER_DATA;
    ClRcT rc = CL_OK;

    AMS_CHECK_RC_ERROR( clXdrMarshallClInt32T(&op, inMsgHdl, 0) );

    AMS_CHECK_RC_ERROR( clXdrMarshallClInt64T(&ams->epoch, inMsgHdl, 0) );

    AMS_CHECK_RC_ERROR( clXdrMarshallClUint8T(&ams->debugFlags, inMsgHdl, 0) );

    AMS_CHECK_RC_ERROR( clXdrMarshallClUint32T(&ams->timerCount, inMsgHdl, 0) );

    clAmsCCBHandleDBMarshall(inMsgHdl);

    exitfn:
    return rc;
}

ClRcT clAmsServerDataUnmarshall(ClAmsT *ams, ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;

    AMS_CHECK_RC_ERROR( clXdrUnmarshallClInt64T(inMsgHdl, &ams->epoch) );

    AMS_CHECK_RC_ERROR( clXdrUnmarshallClUint8T(inMsgHdl, &ams->debugFlags) );

    AMS_CHECK_RC_ERROR( clXdrUnmarshallClUint32T(inMsgHdl, &ams->timerCount) );

    clAmsCCBHandleDBUnmarshall(inMsgHdl);

    exitfn:
    return rc;
}

ClRcT clAmsEntityUserDataMarshall(ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;

    ClAmsCkptOperationT op = CL_AMS_CKPT_OPERATION_USER_DATA;

    AMS_CHECK_RC_ERROR(clXdrMarshallClInt32T(&op, inMsgHdl, 0));

    AMS_CHECK_RC_ERROR(clAmsEntityUserDataPackAll(inMsgHdl));

    exitfn:
    return rc;
}

ClRcT clAmsEntityUserDataUnmarshall(ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClRcT rc = CL_OK;

    AMS_CHECK_RC_ERROR(clAmsEntityUserDataUnpackAll(inMsgHdl));

    exitfn:
    return rc;
}

ClRcT   
clAmsDBGAmsXMLize(
       CL_IN  ClAmsT  *ams )
{

    ClRcT  rc = CL_OK;
    ClCharT  *timerCount = NULL;
    ClCharT  *debugFlags = NULL;

    AMS_CHECKPTR ( !ams );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint32ToStr(
                ams->timerCount,
                &timerCount) );

    AMS_CHECK_RC_ERROR ( clAmsDBClUint8ToStr(
                ams->debugFlags,
                &debugFlags) );


    CL_PARSER_SET_ATTR(
            amfPtr.gAmsPtr,
            "timerCount",
            timerCount);

    CL_PARSER_SET_ATTR(
            amfPtr.gAmsPtr,
            "debugFlags",
            debugFlags);


exitfn:

    clAmsFreeMemory (timerCount);
    clAmsFreeMemory (debugFlags);
    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDBGAmsDeXMLize(
       CL_IN  ClParserPtrT gAmsPtr,
       CL_INOUT  ClAmsT  *ams )
{

    const ClCharT  *timerCount = NULL;
    const ClCharT  *debugFlags = NULL;

    AMS_CHECKPTR ( !ams );

    timerCount = clParserAttr(
            gAmsPtr,
            "timerCount");

    debugFlags = clParserAttr(
            gAmsPtr,
            "debugFlags");


    if ( !debugFlags || !timerCount )
    {
        AMS_LOG (CL_DEBUG_ERROR,("debugFlags / timerCount for gAms does not "
                    "exist \n"));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    ams->debugFlags = atoi (debugFlags);
    ams->timerCount = atoi (timerCount);

    return CL_OK;

}

ClRcT clAmsDBWriteEntityStatus(
        ClAmsEntityStatusT  *entityStatus,
        ClParserPtrT  statusPtr )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityStatus || !statusPtr );

    char  *epoch = NULL;
    char  *timerCount = NULL;

    ClParserPtrT ptr =
        CL_PARSER_ADD_CHILD(statusPtr ,"entityStatus", 0);

    AMS_CHECKPTR (!ptr);

    AMS_CHECK_RC_ERROR( clAmsDBClInt64ToStr(
                entityStatus->epoch,
                &epoch) );

    AMS_CHECK_RC_ERROR( clAmsDBClUint32ToStr(
                entityStatus->timerCount,
                &timerCount) );

    CL_PARSER_SET_ATTR ( ptr, "epoch", epoch);

    CL_PARSER_SET_ATTR ( ptr, "timerCount", timerCount);

exitfn:

    clAmsFreeMemory (epoch);
    clAmsFreeMemory (timerCount);

    return CL_AMS_RC (rc);

}

ClRcT clAmsDBReadEntityStatus(
        ClAmsEntityStatusT  *entityStatus,
        ClParserPtrT  statusPtr )
{

    AMS_CHECKPTR ( !entityStatus || !statusPtr );

    const char  *epoch = NULL;
    const char  *timerCount = NULL;

    ClParserPtrT ptr = clParserChild(statusPtr,"entityStatus");

    AMS_CHECKPTR (!ptr);

    epoch = clParserAttr( ptr, "epoch"); 
    if ( !epoch )
    {
        AMS_LOG (CL_DEBUG_ERROR,("entityStatus tag has a missing attribute - epoch\n"));
        entityStatus->epoch = 0;
    }
    else
    {
        entityStatus->epoch = atoi (epoch);
    }

    timerCount = clParserAttr( ptr, "timerCount");
    if ( !timerCount )
    {
        AMS_LOG (CL_DEBUG_ERROR,("entityStatus tag has a missing attribute - timerCount\n"));
        entityStatus->timerCount = 0;
    }
    else
    {
        entityStatus->timerCount = atoi (timerCount);
    }

    return CL_OK;

}

ClRcT clAmsDBMarshallEnd(ClBufferHandleT inMsgHdl, ClUint32T versionCode)
{
    ClAmsCkptOperationT end = CL_AMS_CKPT_OPERATION_END;
    AMS_CALL(clXdrMarshallClInt32T(&end, inMsgHdl, 0));
    return CL_OK;
}
