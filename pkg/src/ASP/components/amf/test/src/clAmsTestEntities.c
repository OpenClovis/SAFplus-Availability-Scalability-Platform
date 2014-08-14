/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#include <clAmsTestEntities.h>

#define CL_AMS_TEST_ENTITY_GET_LIST(handle,entity,retBuffer,inBuffer,mgmtFun) clAmsTestEntityGetList(handle,entity,retBuffer,inBuffer,#mgmtFun,mgmtFun)

#define CL_AMS_TEST_ENTITIES_ADD_MARKER(str,type,buffer) do {       \
        if(gClAmsTestEntitiesMarker == CL_TRUE)                     \
        {                                                           \
            clAmsTestEntityAddMarker(str,type,(ClPtrT)(buffer));    \
        }                                                           \
}while(0)

typedef enum ClAmsTestEntitiesType
{
    CL_AMS_TEST_ENTITIES_TYPE_ENTITY_BUFFER,
    CL_AMS_TEST_ENTITIES_TYPE_SU_SI_BUFFER,
    CL_AMS_TEST_ENTITIES_TYPE_SI_SU_BUFFER,
    CL_AMS_TEST_ENTITIES_TYPE_COMP_CSI_BUFFER,
    CL_AMS_TEST_ENTITIES_TYPE_CSI_NVP_BUFFER,
}ClAmsTestEntitiesTypeT;

typedef struct {
    const ClCharT *pName;
    ClAmsTestEntitiesTypeT type;
    ClPtrT pMarker;
    ClListHeadT list;
} ClAmsTestEntitiesMarkerT;

ClAmsTestEntitiesT gClAmsTestEntities;

static ClBoolT gClAmsTestEntitiesMarker = CL_FALSE;

static CL_LIST_HEAD_DECLARE(gClAmsTestEntitiesMarkerList);

/*
 * To start marking loads.
 */
void clAmsTestEntityMark(void)
{
    if(gClAmsTestEntitiesMarker == CL_FALSE)
    {
        gClAmsTestEntitiesMarker = CL_TRUE;
    }
}

static void clAmsTestEntityDeleteMarker(ClAmsTestEntitiesMarkerT *pMarker)
{
    switch(pMarker->type)
    {
    case CL_AMS_TEST_ENTITIES_TYPE_ENTITY_BUFFER:
        {
            ClAmsEntityBufferT *pEntityBuffer = pMarker->pMarker;
            if(pEntityBuffer->entity)
            {
                clHeapFree(pEntityBuffer->entity);
            }
            memset(pEntityBuffer,0,sizeof(*pEntityBuffer));
        }
        break;
    case CL_AMS_TEST_ENTITIES_TYPE_SU_SI_BUFFER:
        {
            ClAmsSUSIRefBufferT *pSUSIRefBuffer = pMarker->pMarker;
            if(pSUSIRefBuffer->entityRef)
            {
                clHeapFree(pSUSIRefBuffer->entityRef);
            }
            memset(pSUSIRefBuffer,0,sizeof(*pSUSIRefBuffer));
        }
        break;

    case CL_AMS_TEST_ENTITIES_TYPE_SI_SU_BUFFER:
        {
            ClAmsSISURefBufferT *pSISURefBuffer = pMarker->pMarker;
            if(pSISURefBuffer->entityRef)
            {
                clHeapFree(pSISURefBuffer->entityRef);
            }
            memset(pSISURefBuffer,0,sizeof(*pSISURefBuffer));
        }
        break;
        
    case CL_AMS_TEST_ENTITIES_TYPE_COMP_CSI_BUFFER:
        {
            ClAmsCompCSIRefBufferT *pCompCSIRefBuffer = pMarker->pMarker;
            if(pCompCSIRefBuffer->entityRef)
            {
                register ClInt32T i;
                ClUint32T size = (ClUint32T)sizeof(ClAmsCompCSIRefT);
                for(i = 0; i < pCompCSIRefBuffer->count;++i)
                {
                    ClAmsCompCSIRefT  *pCompCSIRef = 
                        (ClAmsCompCSIRefT *)(((ClInt8T *)pCompCSIRefBuffer->entityRef) + i*size );
                    if(pCompCSIRef->activeComp)
                    {
                        clHeapFree(pCompCSIRef->activeComp);
                    }
                }
                clHeapFree(pCompCSIRefBuffer->entityRef);
            }
            memset(pCompCSIRefBuffer,0,sizeof(*pCompCSIRefBuffer));
        }
        break;

    case CL_AMS_TEST_ENTITIES_TYPE_CSI_NVP_BUFFER:
        {
            ClAmsCSINVPBufferT *pNVP = pMarker->pMarker;
            if(pNVP->nvp)
            {
                clHeapFree(pNVP->nvp);
            }
            memset(pNVP,0,sizeof(*pNVP));
        }
        break;
    default:;
    }
}

/*
 * To be called during a reset or a free of the loads.
 */
void clAmsTestEntityUnmark(void)
{
    ClListHeadT *pHead = &gClAmsTestEntitiesMarkerList;
    if(gClAmsTestEntitiesMarker == CL_FALSE)
        return;
    gClAmsTestEntitiesMarker = CL_FALSE;
    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pTemp = pHead->pNext;
        ClAmsTestEntitiesMarkerT *pMarker = CL_LIST_ENTRY(pTemp,ClAmsTestEntitiesMarkerT,list);
        clAmsTestEntityDeleteMarker(pMarker);
        clListDel(pTemp);
        free(pMarker);
    }
}

static ClAmsTestEntitiesMarkerT *clAmsTestEntityFindMarker(const ClCharT *pName)
{
    register ClListHeadT *pTemp;
    ClListHeadT *pHead = &gClAmsTestEntitiesMarkerList;
    CL_LIST_FOR_EACH(pTemp,pHead)
    {
        ClAmsTestEntitiesMarkerT *pMarker = CL_LIST_ENTRY(pTemp,ClAmsTestEntitiesMarkerT,list);
        if(!strcmp(pMarker->pName,pName))
        {
            return pMarker;
        }
    }
    return NULL;
}

static ClBoolT clAmsTestEntityFindAndDeleteMarker(const ClCharT *pName)
{
    ClAmsTestEntitiesMarkerT *pMarker;
    pMarker = clAmsTestEntityFindMarker(pName);
    if(pMarker)
    {
        clAmsTestEntityDeleteMarker(pMarker);
        clListDel(&pMarker->list);
        free(pMarker);
        return CL_TRUE;
    }
    return CL_FALSE;
}

static __inline__ void clAmsTestEntityAddMarker(const ClCharT *pName,ClAmsTestEntitiesTypeT type,ClPtrT pEntity)
{
    ClAmsTestEntitiesMarkerT *pMarker = NULL;
    pMarker = calloc(1,sizeof(*pMarker));
    CL_ASSERT(pMarker != NULL);
    pMarker->pName = pName;
    pMarker->type = type;
    pMarker->pMarker = pEntity;
    clListAddTail(&pMarker->list,&gClAmsTestEntitiesMarkerList);
}
                                     
ClRcT clAmsTestEntityGetConfig(ClAmsMgmtHandleT handle,
                               ClAmsEntityT *pEntity,
                               ClAmsEntityConfigT **ppEntityConfig)
{
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClRcT rc = CL_OK;
    const ClCharT *pType = NULL;
    ClAmsEntityConfigT *pConfig = NULL;
    ClUint32T configSize = 0;

    switch(pEntity->type)
    {

    case CL_AMS_ENTITY_TYPE_SG:
        pEntityConfig = &gClAmsTestEntities.sg.sgConfig.entity;
        configSize = sizeof(ClAmsSGConfigT);
        pType = "SG";
        break;
    case CL_AMS_ENTITY_TYPE_SI:
        pEntityConfig = &gClAmsTestEntities.si.siConfig.entity;
        configSize = sizeof(ClAmsSIConfigT);
        pType = "SI";
        break;
    case CL_AMS_ENTITY_TYPE_SU:
        pEntityConfig = &gClAmsTestEntities.su.suConfig.entity;
        configSize = sizeof(ClAmsSUConfigT);
        pType = "SU";
        break;
    case CL_AMS_ENTITY_TYPE_NODE:
        pEntityConfig = &gClAmsTestEntities.node.nodeConfig.entity;
        configSize = sizeof(ClAmsNodeConfigT);
        pType = "NODE";
        break;
    case CL_AMS_ENTITY_TYPE_COMP:
        pEntityConfig = &gClAmsTestEntities.comp.compConfig.entity;
        configSize = sizeof(ClAmsCompConfigT);
        pType = "COMP";
        break;
    case CL_AMS_ENTITY_TYPE_CSI:
        pEntityConfig = &gClAmsTestEntities.csi.csiConfig.entity;
        configSize = sizeof(ClAmsCSIConfigT);
        pType = "CSI";
        break;
    default:
        return CL_AMS_TEST_RC(CL_ERR_NOT_IMPLEMENTED);
    }

    rc = clAmsMgmtEntityGetConfig(handle,pEntity,&pConfig);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Error in getting AMS config for entity type [%s]\n",pType));
        goto out;
    }
    memcpy(pEntityConfig,pConfig,configSize);

    clHeapFree(pConfig);

    if(ppEntityConfig)
        *ppEntityConfig = pEntityConfig;
    out:
    return rc;
}

ClRcT clAmsTestEntityGetStatus(ClAmsMgmtHandleT handle,
                               ClAmsEntityT *pEntity,
                               ClAmsEntityStatusT **ppEntityStatus)
{
    ClAmsEntityStatusT *pEntityStatus = NULL;
    ClAmsEntityStatusT *pStatus = NULL;
    ClRcT rc = CL_OK;
    const ClCharT *pType = NULL;
    ClUint32T statusSize = 0;

    switch(pEntity->type)
    {

    case CL_AMS_ENTITY_TYPE_SG:
        pEntityStatus = &gClAmsTestEntities.sg.sgStatus.entity;
        statusSize = sizeof(ClAmsSGStatusT);
        pType = "SG";
        break;
    case CL_AMS_ENTITY_TYPE_SI:
        pEntityStatus = &gClAmsTestEntities.si.siStatus.entity;
        statusSize = sizeof(ClAmsSIStatusT);
        pType = "SI";
        break;
    case CL_AMS_ENTITY_TYPE_SU:
        pEntityStatus = &gClAmsTestEntities.su.suStatus.entity;
        statusSize = sizeof(ClAmsSUStatusT);
        pType = "SU";
        break;
    case CL_AMS_ENTITY_TYPE_NODE:
        pEntityStatus = &gClAmsTestEntities.node.nodeStatus.entity;
        statusSize = sizeof(ClAmsNodeStatusT);
        pType = "NODE";
        break;
    case CL_AMS_ENTITY_TYPE_COMP:
        pEntityStatus = &gClAmsTestEntities.comp.compStatus.entity;
        statusSize = sizeof(ClAmsCompStatusT);
        pType = "COMP";
        break;
    case CL_AMS_ENTITY_TYPE_CSI:
        pEntityStatus = &gClAmsTestEntities.csi.csiStatus.entity;
        statusSize = sizeof(ClAmsCSIStatusT);
        pType = "CSI";
        break;
    default:
        return CL_AMS_TEST_RC(CL_ERR_NOT_IMPLEMENTED);
    }

    rc = clAmsMgmtEntityGetStatus(handle,pEntity,&pStatus);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Error in getting AMS status for entity type [%s]\n",pType));
        goto out;
    }
    memcpy(pEntityStatus,pStatus,statusSize);
    clHeapFree(pStatus);
    if(ppEntityStatus)
        *ppEntityStatus = pEntityStatus;
    out:
    return rc;
}

ClRcT clAmsTestEntityGetList(ClAmsMgmtHandleT handle,
                             ClAmsEntityT *pEntity,
                             ClAmsEntityBufferT **ppRetBuffer,
                             ClAmsEntityBufferT *pInBuffer,
                             const ClCharT *pMgmtFunStr,
                             ClRcT (*pMgmtFun)(ClAmsMgmtHandleT,
                                               ClAmsEntityT *,
                                               ClAmsEntityBufferT*))
{
    ClRcT rc = CL_AMS_TEST_RC(CL_ERR_INVALID_PARAMETER);
    ClAmsEntityBufferT entityBuffer;
    if(pMgmtFun == NULL)
    {
        CL_AMS_TEST_PRINT(("Ams MGMT fun is NULL\n"));
        goto out;
    }
    memset(&entityBuffer,0,sizeof(entityBuffer));
    
    rc = pMgmtFun(handle,pEntity,&entityBuffer);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Error running mgmtFun [%s].Failed with [0x%x]\n",pMgmtFunStr,rc));
        goto out;
    }
    
    /*
     * Now this is read correctly.
     * Check for the existence of this marker and rip it off.
     */
    clAmsTestEntityFindAndDeleteMarker(pMgmtFunStr);
    
    memcpy(pInBuffer,&entityBuffer,sizeof(*pInBuffer));

    if(ppRetBuffer)
        *ppRetBuffer = pInBuffer;

    CL_AMS_TEST_ENTITIES_ADD_MARKER(pMgmtFunStr,
                                    CL_AMS_TEST_ENTITIES_TYPE_ENTITY_BUFFER,
                                    pInBuffer);
    out:
    return rc;
}


ClRcT clAmsTestEntityGetSGSUList(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT*pEntity,
                                 ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUList,clAmsMgmtGetSGSUList);
}

ClRcT clAmsTestEntityGetSGSIList(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT *pEntity,
                                 ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSIList,clAmsMgmtGetSGSIList);
}

ClRcT clAmsTestEntityGetSGSUInstantiableList(ClAmsMgmtHandleT handle,
                                             ClAmsEntityT *pEntity,
                                             ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUInstantiableList,clAmsMgmtGetSGInstantiableSUList);
}
                                             

ClRcT clAmsTestEntityGetSGSUInstantiatedList(ClAmsMgmtHandleT handle,
                                             ClAmsEntityT *pEntity,
                                             ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUInstantiatedList,clAmsMgmtGetSGInstantiatedSUList);
}

ClRcT clAmsTestEntityGetSGSUInserviceSpareList(ClAmsMgmtHandleT handle,
                                               ClAmsEntityT *pEntity,
                                               ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUInserviceSpareList,clAmsMgmtGetSGInServiceSpareSUList);
}

ClRcT clAmsTestEntityGetSGSUAssignedList(ClAmsMgmtHandleT handle,
                                         ClAmsEntityT *pEntity,
                                         ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUAssignedList,clAmsMgmtGetSGAssignedSUList);
}

ClRcT clAmsTestEntityGetSGSUFaultyList(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *pEntity,
                                       ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.sg.sgSUFaultyList,clAmsMgmtGetSGFaultySUList);
}

ClRcT clAmsTestEntityGetSISUList(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT *pEntity,
                                 ClAmsSISURefBufferT **ppRetBuffer)
{
    ClRcT rc = CL_OK;
    ClAmsSISURefBufferT siSURefBuffer;
    ClAmsSISURefBufferT *pBuffer = &gClAmsTestEntities.si.siSUList;

    memset(&siSURefBuffer,0,sizeof(siSURefBuffer));
    
    rc = clAmsMgmtGetSISUList(handle,pEntity,&siSURefBuffer);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Ams Mgmt Get SISU List failed with error [0x%x]\n",rc));
        goto out;
    }
    clAmsTestEntityFindAndDeleteMarker(__FUNCTION__);
    memcpy(pBuffer,&siSURefBuffer,sizeof(*pBuffer));
    if(ppRetBuffer)
        *ppRetBuffer = pBuffer;
    CL_AMS_TEST_ENTITIES_ADD_MARKER(__FUNCTION__,CL_AMS_TEST_ENTITIES_TYPE_SI_SU_BUFFER,pBuffer);
    out:
    return rc;
}

ClRcT clAmsTestEntityGetSISURankList(ClAmsMgmtHandleT handle,
                                     ClAmsEntityT *pEntity,
                                     ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.si.siSURankList,clAmsMgmtGetSISURankList);
}


ClRcT clAmsTestEntityGetSIDependenciesList(ClAmsMgmtHandleT handle,
                                           ClAmsEntityT *pEntity,
                                           ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.si.siDependenciesList,clAmsMgmtGetSIDependenciesList);
}

ClRcT clAmsTestEntityGetSICSIList(ClAmsMgmtHandleT handle,
                                  ClAmsEntityT *pEntity,
                                  ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.si.siCSIList,clAmsMgmtGetSICSIList);
}

ClRcT clAmsTestEntityGetSUCompList(ClAmsMgmtHandleT handle,
                                   ClAmsEntityT *pEntity,
                                   ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.su.suCompList,clAmsMgmtGetSUCompList);
}

ClRcT clAmsTestEntityGetSUSIList(ClAmsMgmtHandleT handle,
                                 ClAmsEntityT *pEntity,
                                 ClAmsSUSIRefBufferT **ppRetBuffer)
{
    ClRcT rc = CL_OK;
    ClAmsSUSIRefBufferT suSIRefBuffer;
    ClAmsSUSIRefBufferT *pBuffer = &gClAmsTestEntities.su.suSIList;

    memset(&suSIRefBuffer,0,sizeof(suSIRefBuffer));
    rc = clAmsMgmtGetSUAssignedSIsList(handle,pEntity,&suSIRefBuffer);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Ams Mgmt GET SU Assigned SI List failed with error [0x%x]\n",rc));
        goto out;
    }
    
    clAmsTestEntityFindAndDeleteMarker(__FUNCTION__);
    memcpy(pBuffer,&suSIRefBuffer,sizeof(*pBuffer));

    if(ppRetBuffer)
        *ppRetBuffer = pBuffer;
    CL_AMS_TEST_ENTITIES_ADD_MARKER(__FUNCTION__,
                                    CL_AMS_TEST_ENTITIES_TYPE_SU_SI_BUFFER,
                                    pBuffer);
    out:
    return rc;
}
                                 
ClRcT clAmsTestEntityGetNodeDependenciesList(ClAmsMgmtHandleT handle,
                                             ClAmsEntityT *pEntity,
                                             ClAmsEntityBufferT **ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,
                                       &gClAmsTestEntities.node.nodeDependenciesList,
                                       clAmsMgmtGetNodeDependenciesList);
                                  
}

ClRcT clAmsTestEntityGetNodeSUList(ClAmsMgmtHandleT handle,
                                   ClAmsEntityT*pEntity,
                                   ClAmsEntityBufferT**ppRetBuffer)
{
    return CL_AMS_TEST_ENTITY_GET_LIST(handle,pEntity,ppRetBuffer,&gClAmsTestEntities.node.nodeSUList,clAmsMgmtGetNodeSUList);
}

ClRcT clAmsTestEntityGetCompCSIList(ClAmsMgmtHandleT handle,
                                    ClAmsEntityT*pEntity,
                                    ClAmsCompCSIRefBufferT **ppRetBuffer)
{
    ClRcT rc;
    ClAmsCompCSIRefBufferT compCSIRefBuffer;
    ClAmsCompCSIRefBufferT *pBuffer = &gClAmsTestEntities.comp.compCSIList;

    memset(&compCSIRefBuffer,0,sizeof(compCSIRefBuffer));
    rc = clAmsMgmtGetCompCSIList(handle,pEntity,&compCSIRefBuffer);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Ams MGMT Get Comp CSI LIST failed with error [0x%x]\n",rc));
        goto out;
    }

    clAmsTestEntityFindAndDeleteMarker(__FUNCTION__);
    memcpy(pBuffer,&compCSIRefBuffer,sizeof(*pBuffer));
    
    if(ppRetBuffer)
        *ppRetBuffer=pBuffer;
    CL_AMS_TEST_ENTITIES_ADD_MARKER(__FUNCTION__,
                                    CL_AMS_TEST_ENTITIES_TYPE_COMP_CSI_BUFFER,
                                    pBuffer);
    out:
    return rc;
}

ClRcT clAmsTestEntityGetCSINVPList(ClAmsMgmtHandleT handle,
                                   ClAmsEntityT*pEntity,
                                   ClAmsCSINVPBufferT **ppRetBuffer)
{
    ClRcT rc = CL_OK;
    ClAmsCSINVPBufferT csiNVPBuffer;
    ClAmsCSINVPBufferT *pBuffer = &gClAmsTestEntities.csi.csiNVPList;
    
    memset(&csiNVPBuffer,0,sizeof(csiNVPBuffer));
    rc = clAmsMgmtGetCSINVPList(handle,pEntity,&csiNVPBuffer);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Ams MGMT Get CSI NVP List failed with error [0x%x]\n",rc));
        goto out;
    }
    clAmsTestEntityFindAndDeleteMarker(__FUNCTION__);
    memcpy(pBuffer,&csiNVPBuffer,sizeof(*pBuffer));

    if(ppRetBuffer)
        *ppRetBuffer = pBuffer;
    CL_AMS_TEST_ENTITIES_ADD_MARKER(__FUNCTION__,
                                    CL_AMS_TEST_ENTITIES_TYPE_CSI_NVP_BUFFER,
                                    pBuffer);
    out:
    return rc;
}


ClRcT clAmsTestEntityLockAssignment(ClAmsMgmtHandleT handle,
                                    ClAmsEntityT *pEntity)
{
    ClRcT rc = CL_OK;
    rc = clAmsMgmtEntityLockAssignment(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity lock assignment failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityLockInstantiation(ClAmsMgmtHandleT handle,
                                       ClAmsEntityT *pEntity)
{
    ClRcT rc = CL_OK;
    rc = clAmsMgmtEntityLockInstantiation(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity lock instantiation failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityUnlock(ClAmsMgmtHandleT handle,
                            ClAmsEntityT *pEntity)
{
    ClRcT rc;
    rc = clAmsMgmtEntityUnlock(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity unlock failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityRestart(ClAmsMgmtHandleT handle,
                             ClAmsEntityT *pEntity)
{
    ClRcT rc ;
    rc = clAmsMgmtEntityRestart(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity unlock failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityShutdown(ClAmsMgmtHandleT handle,
                              ClAmsEntityT *pEntity)
{
    ClRcT rc;
    rc = clAmsMgmtEntityShutdown(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity shutdown failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityShutdownWithRestart(ClAmsMgmtHandleT handle,
                                         ClAmsEntityT *pEntity)
{
    ClRcT rc;
    rc = clAmsMgmtEntityShutdownWithRestart(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity shutdown with restart failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

ClRcT clAmsTestEntityRepaired(ClAmsMgmtHandleT handle,
                              ClAmsEntityT *pEntity)
{
    ClRcT rc;
    rc = clAmsMgmtEntityRepaired(handle,pEntity);
    if(rc != CL_OK)
    {
        CL_AMS_TEST_PRINT(("Entity repaired failed for [%s]\n",pEntity->name.value));
        goto out;
    }
    out:
    return rc;
}

/*
 * Interface to fetch required AMS mgmt. entity information.
 */

ClRcT clAmsTestEntityGet(ClAmsMgmtHandleT handle,ClUint32T info,ClAmsEntityT *pEntity,ClBoolT insideTest)
{
#define CL_AMS_TEST_HOOK(reason,predicate,execOnFailure) do {           \
        if(insideTest == CL_TRUE)                                       \
        {                                                               \
            CL_AMS_TEST_CASE_MALFUNCTION(reason,predicate,execOnFailure); \
        }                                                               \
        else                                                            \
        {                                                               \
            ClInt32T __status;                                          \
            __status = predicate;                                       \
            if(__status)                                                \
            {                                                           \
                clLogError(CL_AMS_TEST_AMS_AREA,CL_AMS_TEST_ENTITY_CONTEXT,"%s failed with rc [0x%x]",#predicate,rc); \
            }                                                           \
        }                                                               \
}while(0)

    ClRcT rc = CL_OK;

    switch(pEntity->type)
    {
    case CL_AMS_ENTITY_TYPE_SG:
        {
            if((info&CL_AMS_TEST_SG_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] config",pEntity->name.value), (rc = clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SI_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] SI List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSIList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] SU List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_ASSIGNED_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] SU assigned list",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUAssignedList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_INSTANTIABLE_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] SU Instantiable List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUInstantiableList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_INSTANTIATED_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] SU Instantiated List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUInstantiatedList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_INSERVICE_SPARE_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] inservice Spare List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUInserviceSpareList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SG_SU_FAULTY_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SG [%s] faulty List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSGSUFaultyList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    case CL_AMS_ENTITY_TYPE_SI:
        {
            if((info&CL_AMS_TEST_SI_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] config",pEntity->name.value),
                                 (rc=clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SI_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SI_SU_RANK_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] SU Rank List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSISURankList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SI_SU_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] SU List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSISUList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SI_DEPENDENCIES_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] Dependencies List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSIDependenciesList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SI_CSI_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SI [%s] CSI List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSICSIList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    case CL_AMS_ENTITY_TYPE_SU:
        {
            if((info&CL_AMS_TEST_SU_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting SU [%s] config",pEntity->name.value),
                                 (rc=clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SU_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting SU [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SU_COMP_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SU [%s] comp List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSUCompList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_SU_SI_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting SU [%s] SI List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetSUSIList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    case CL_AMS_ENTITY_TYPE_NODE:
        {
            if((info&CL_AMS_TEST_NODE_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting Node [%s] config",pEntity->name.value),
                                 (rc=clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_NODE_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting Node [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_NODE_DEPENDENCIES_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting Node [%s] Dependencies List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetNodeDependenciesList(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_NODE_SU_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting Node [%s] SU List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetNodeSUList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    case CL_AMS_ENTITY_TYPE_COMP:
        {
            if((info&CL_AMS_TEST_COMP_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting Comp [%s] config",pEntity->name.value),
                                 (rc=clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_COMP_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting Comp [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_COMP_CSI_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting Comp [%s] CSI List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetCompCSIList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    case CL_AMS_ENTITY_TYPE_CSI:
        {
            if((info&CL_AMS_TEST_CSI_CONFIG))
            {
                CL_AMS_TEST_HOOK(("Getting CSI [%s] config",pEntity->name.value),
                                 (rc=clAmsTestEntityGetConfig(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_CSI_STATUS))
            {
                CL_AMS_TEST_HOOK(("Getting CSI [%s] status",pEntity->name.value),
                                 (rc=clAmsTestEntityGetStatus(handle,pEntity,NULL))==CL_OK,goto out);
            }
            if((info&CL_AMS_TEST_CSI_NVP_LIST))
            {
                CL_AMS_TEST_HOOK(("Getting CSI [%s] NVP List",pEntity->name.value),
                                 (rc=clAmsTestEntityGetCSINVPList(handle,pEntity,NULL))==CL_OK,goto out);
            }
        }
        break;
    default:;
    }
    out:
    return rc;
#undef CL_AMS_TEST_HOOK
}
                         
