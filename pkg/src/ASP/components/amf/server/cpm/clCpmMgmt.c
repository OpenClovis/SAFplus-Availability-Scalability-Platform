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

#include <string.h>

#include <clLogApi.h>
#include <clDebugApi.h>
#include <clAmsDatabase.h>
#include <clAms.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsMgmtCommon.h>
#include <clAmsServerUtils.h>
#include <clCpmExtApi.h>
#include <clCpmInternal.h>
#include <clCpmCor.h>
#include <xdrClCpmCompConfigSetT.h>

#define CL_CPM_COMP_CLEANUP_SCRIPT "asp_cleanup.sh"

void cpmFillCompConfig(const ClCpmMgmtCompT *compInfo,
                       ClCpmCompConfigT *compConfig)
{
    ClUint32T i = 0;
    
    strncpy(compConfig->compName, compInfo->compName, CL_MAX_NAME_LENGTH-1);
    compConfig->compProperty = compInfo->compProperty;
    compConfig->compProcessRel = compInfo->compProcessRel;
    strncpy(compConfig->instantiationCMD,
            compInfo->instantiationCMD,
            CL_MAX_NAME_LENGTH-1);

    /**
     * Put the image name as first argument.
     */
    
    compConfig->argv[0] = clHeapAllocate(strlen(compInfo->instantiationCMD)+1);
    if (!compConfig->argv[0])
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Unable to allocate memory");
        goto failure;
    }

    strncpy(compConfig->argv[0],
            compInfo->instantiationCMD,
            strlen(compInfo->instantiationCMD));
    
    for (i = 1; compInfo->argv[i]; ++i)
    {
        compConfig->argv[i] = clHeapAllocate(strlen(compInfo->argv[i]) + 1);
        if (!compConfig->argv[i])
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                       "Unable to allocate memory");
            goto failure;
        }

        strncpy(compConfig->argv[i],
                compInfo->argv[i],
                strlen(compInfo->argv[i]));
    }
    compConfig->argv[i] = NULL;

failure:
    return;
}

static ClRcT cpmComponentAdd(ClCharT *compName)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
    ClCpmCompConfigT compConfig = 
    {
        "",
        CL_FALSE,
        CL_AMS_COMP_PROPERTY_SA_AWARE,
        CL_CPM_COMP_SINGLE_PROCESS,
        "invalid",
        {"invalid", NULL},
        {NULL},
        "",
        "",
        0,
        0,
        0,
        0,
        0,
        {
            CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT,
            2 * CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT,
            CL_AMS_RECOVERY_NO_RECOMMENDATION
        }
    };
    ClCpmComponentT *comp = NULL;
    ClUint16T compKey = 0;
    const ClCharT *config = NULL;
    ClCharT cleanupCMD[CL_MAX_NAME_LENGTH];
        
    if (!compName)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "NULL pointer passed.");
        goto failure;
    }

    cleanupCMD[0] = 0;
    if( (config = getenv("ASP_CONFIG")) )
    {
        snprintf(cleanupCMD, sizeof(cleanupCMD), "%s/%s", config, CL_CPM_COMP_CLEANUP_SCRIPT);
        if(access(cleanupCMD, R_OK | X_OK))
            cleanupCMD[0] = 0;
    }
    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if (comp)
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                     "Component [%s] already exists on this node",
                     compName);
        rc = CL_CPM_RC(CL_ERR_ALREADY_EXIST);
        goto failure;
    }

    strncpy(compConfig.compName, compName, CL_MAX_NAME_LENGTH-1);
    if(cleanupCMD[0])
        strncpy(compConfig.cleanupCMD, cleanupCMD, sizeof(compConfig.cleanupCMD)-1);

    rc = cpmCompConfigure(&compConfig, &comp);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to configure component, error [%#x]",
                   rc);
        goto failure;
    }

    rc = clCksm16bitCompute((ClUint8T *) comp->compConfig->compName,
                            strlen(comp->compConfig->compName),
                            &compKey);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to compute checksum for component, "
                   "error [%#x]",
                   rc);
        goto failure;
    }

    clOsalMutexLock(gpClCpm->compTableMutex);
    rc = clCntNodeAdd(gpClCpm->compTable,
                      (ClCntKeyHandleT)(ClWordT)compKey,
                      (ClCntDataHandleT) comp,
                      NULL);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to add node to the CPM component table, "
                   "error [%#x]",
                   rc);
        clOsalMutexUnlock(gpClCpm->compTableMutex);
        goto failure;
    }
    ++gpClCpm->noOfComponent;
    clOsalMutexUnlock(gpClCpm->compTableMutex);

    return CL_OK;

failure:
    return rc;
}

static ClRcT cpmComponentRmv(ClCharT *compName)
{
    ClRcT rc = CL_OK;
    ClUint16T compKey = 0;
    ClCntNodeHandleT hNode = 0;
    ClUint32T numNode = 0;
    ClCpmComponentT *comp = NULL;
    ClBoolT done = CL_NO;
    ClNameT entity = {0};

    if (!compName)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "NULL pointer passed.");
        goto failure;
    }

    rc = clCksm16bitCompute((ClUint8T *)compName,
                            strlen(compName),
                            &compKey);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to compute checksum for component, "
                   "error [%#x]",
                   rc);
        goto failure;
    }
    clNameSet(&entity, compName);
    /*
     * Clear pending invocations for the component being removed.
     */
    (void)cpmInvocationClearCompInvocation(&entity);
    clOsalMutexLock(gpClCpm->compTableMutex);
    clOsalMutexLock(&gpClCpm->compTerminateMutex);

    rc = clCntNodeFind(gpClCpm->compTable,
                       (ClCntKeyHandleT)(ClWordT)compKey,
                       &hNode);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to find component [%s], error [%#x]",
                   compName,
                   rc);
        goto unlock;
    }

    rc = clCntKeySizeGet(gpClCpm->compTable,
                         (ClCntKeyHandleT)(ClWordT)compKey,
                         &numNode);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Unable to get key size, error [%#x]",
                   rc);
        goto unlock;
    }

    while (numNode > 0)
    {
        rc = clCntNodeUserDataGet(gpClCpm->compTable,
                                  hNode,
                                  (ClCntNodeHandleT *) &comp);
        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                       "Unable to get user data, error [%#x]",
                       rc);
            goto unlock;
        }

        if (!strncmp(compName,
                     comp->compConfig->compName,
                     CL_MAX_NAME_LENGTH-1))
        {
            clCntNodeDelete(gpClCpm->compTable, hNode);
            done = CL_YES;
            break;
        }

        if (--numNode)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            if (CL_OK != rc)
            {
                break;
            }
        }
    }

    if (!done)
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        goto unlock;
    }
    
    --gpClCpm->noOfComponent;

    clOsalMutexUnlock(&gpClCpm->compTerminateMutex);
    clOsalMutexUnlock(gpClCpm->compTableMutex);

    return CL_OK;

unlock:
    clOsalMutexUnlock(&gpClCpm->compTerminateMutex);
    clOsalMutexUnlock(gpClCpm->compTableMutex);
failure:
    return rc;
}

ClRcT cpmNodeAdd(ClCharT *nodeName)
{
    ClRcT rc = CL_OK;
    ClCpmLT *cpm = NULL;
    ClUint16T nodeKey = 0;
    ClCpmSlotInfoT slotInfo = {0};

    if (!nodeName)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "NULL pointer passed.");
        goto failure;
    }

    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFind(nodeName, &cpm);
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    if (cpm)
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                     "Node [%s] already exists on this node.",
                     nodeName);
        rc = CL_CPM_RC(CL_ERR_ALREADY_EXIST);
        goto failure;
    }

    cpm = clHeapAllocate(sizeof(ClCpmLT));
    if (!cpm)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Unable to allocate memory");
        goto failure;
    }

    memset(cpm, 0, sizeof(ClCpmLT));
    
    strncpy(cpm->nodeName, nodeName, strlen(nodeName));
    
    rc = clCksm16bitCompute((ClUint8T *)cpm->nodeName,
                            strlen(cpm->nodeName),
                            &nodeKey);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to compute checksum for node, "
                   "error [%#x]",
                   rc);
        goto failure;
    }

    /* 
     * Filling the CPML with defaults.  
     */

    clNameSet(&cpm->nodeType, nodeName);
    clNameSet(&cpm->nodeIdentifier, nodeName);
    /*
     * Set the class type to CLASS_C.
     */
    strncpy(cpm->classType, "CL_AMS_NODE_CLASS_C", sizeof(cpm->classType)-1);
    /*
     * Get the MOID of the master node. Assuming that node add is
     * only invoked on the master.
     */
    slotInfo.slotId = clIocLocalAddressGet();

    rc = clCpmSlotInfoGet(CL_CPM_SLOT_ID, &slotInfo);
    if(rc == CL_OK)
    {
        rc = cpmCorMoIdToMoIdNameGet(&slotInfo.nodeMoId,
                                     &cpm->nodeMoIdStr);
    }
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_COMP_UNREACHABLE
           &&
           CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            goto failure;
        }
        else
        {
            rc = CL_OK;
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM, 
                         "COR server not running. Hence ignoring the error");
        }
    }

    clOsalMutexLock(gpClCpm->cpmTableMutex);

    rc = clCntNodeAdd(gpClCpm->cpmTable,
                      (ClCntKeyHandleT)(ClWordT)nodeKey,
                      (ClCntDataHandleT) cpm,
                      NULL);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to add node to the CPM node table, "
                   "error [%#x]",
                   rc);
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        goto failure;
    }

    ++gpClCpm->noOfCpm;

    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    /*
     * Add this to the slotinfo.
     */

    clOsalMutexLock(&gpClCpm->cpmMutex);
    rc = cpmSlotClassAdd(&cpm->nodeType, &cpm->nodeIdentifier, 0);
    clOsalMutexUnlock(&gpClCpm->cpmMutex);

    if(rc != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Node [%s] class add returned [%#x]", cpm->nodeType.value, rc);
    }

    clLogNotice("DYN", "NODE", "Node [%s] added to the cpm table with identity [%.*s]",
                cpm->nodeName, cpm->nodeIdentifier.length, cpm->nodeIdentifier.value);
                
    return CL_OK;

    failure:
    return rc;
}

static ClRcT cpmNodeRmv(ClCharT *nodeName)
{
    ClRcT rc = CL_OK;
    ClUint16T nodeKey = 0;
    ClCntNodeHandleT hNode = 0;
    ClUint32T numNode = 0;
    ClCpmLT *cpm = NULL;
    ClBoolT done = CL_NO;
    if (!nodeName)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "NULL pointer passed.");
        goto failure;
    }

    rc = clCksm16bitCompute((ClUint8T *)nodeName,
                            strlen(nodeName),
                            &nodeKey);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to compute checksum for node, "
                   "error [%#x]",
                   rc);
        goto failure;
    }

    clOsalMutexLock(gpClCpm->cpmTableMutex);

    rc = clCntNodeFind(gpClCpm->cpmTable,
                       (ClCntKeyHandleT)(ClWordT)nodeKey,
                       &hNode);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Failed to find node [%s], error [%#x]",
                   nodeName,
                   rc);
        goto unlock;
    }

    rc = clCntKeySizeGet(gpClCpm->cpmTable,
                         (ClCntKeyHandleT)(ClWordT)nodeKey,
                         &numNode);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                   "Unable to get key size, error [%#x]",
                   rc);
        goto unlock;
    }

    while (numNode > 0)
    {
        rc = clCntNodeUserDataGet(gpClCpm->cpmTable,
                                  hNode,
                                  (ClCntNodeHandleT *) &cpm);
        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                       "Unable to get user data, error [%#x]",
                       rc);
            goto unlock;
        }

        if (!strncmp(nodeName, cpm->nodeName, CL_MAX_NAME_LENGTH-1))
        {
            clCntNodeDelete(gpClCpm->cpmTable, hNode);
            done = CL_YES;
            break;
        }

        if (--numNode)
        {
            rc = clCntNextNodeGet(gpClCpm->cpmTable, hNode, &hNode);
            if (CL_OK != rc)
            {
                break;
            }
        }
    }

    if (!done)
    {
        rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
        goto unlock;
    }

    --gpClCpm->noOfCpm;

    clOsalMutexUnlock(gpClCpm->cpmTableMutex);

    /*
     * Delete from slot table
     */
    clOsalMutexLock(&gpClCpm->cpmMutex);
    if(cpmSlotClassDelete(nodeName) != CL_OK)
    {
        clLogWarning("CPM", "MGMT", "Slot class delete failed for node [%s]", nodeName);
    }
    clOsalMutexUnlock(&gpClCpm->cpmMutex);

    return CL_OK;

unlock:
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
failure:
    return rc;
}

ClRcT cpmEntityAdd(ClAmsEntityRefT *entityRef)
{
    switch (entityRef->entity.type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
            
            return cpmComponentAdd(comp->config.entity.name.value);
        }
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *node = (ClAmsNodeT *) entityRef->ptr;
            
            return cpmNodeAdd(node->config.entity.name.value);
        }
        default:
        {
            CL_ASSERT(0);
        }
    }

    return CL_OK;
}

ClRcT cpmEntityRmv(ClAmsEntityRefT *entityRef)
{
    switch (entityRef->entity.type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

            return cpmComponentRmv(comp->config.entity.name.value);
        }
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *node = (ClAmsNodeT *) entityRef->ptr;
            
            return cpmNodeRmv(node->config.entity.name.value);
        }
        default:
        {
            CL_ASSERT(0);
        }
    }

    return CL_OK;
}

static ClRcT cpmParseArgs(ClCpmComponentT *comp, ClCharT *cmd)
{
    ClCharT tmp[CL_MAX_NAME_LENGTH] = {0};
    ClCharT imageName[CL_MAX_NAME_LENGTH];
    ClCharT *c = NULL;
    ClCharT *saveptr = NULL;
    ClUint32T i = 0;
    ClUint32T argIndex = 0;
    ClRcT rc = CL_OK;
    
    strncpy(tmp, cmd, CL_MAX_NAME_LENGTH-1);

    for(i = 0; comp->compConfig->argv[i]; ++i)
    {
        clHeapFree(comp->compConfig->argv[i]);
        comp->compConfig->argv[i] = NULL;
    }

    i = 0;

    if(cpmIsValgrindBuild(cmd)) 
    { 
        cpmModifyCompArgs(comp->compConfig, &i);
    }
    argIndex = i;
    c = strtok_r(tmp, " ", &saveptr);
    while (c && (i < CPM_MAX_ARGS - 1))
    {
        ClUint32T len = strlen(c);
        if(i == argIndex && *c != '/')
        {
            ClCharT *binPath = getenv(CL_ASP_BINDIR_PATH);
            snprintf(imageName, sizeof(imageName)-1, "%s%s%s", 
                     binPath ? binPath : "", binPath ? "/" : "", c);
            len = strlen(imageName);
            c = imageName;
        }
        comp->compConfig->argv[i] = clHeapAllocate(len + 1);
        if (!comp->compConfig->argv[i])
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_MGM,
                       "Unable to allocate memory");
            rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
            goto failure;
        }

        strcpy(comp->compConfig->argv[i], c);
        ++i;

        c = strtok_r(NULL, " ", &saveptr);
    }
    comp->compConfig->argv[i] = NULL;

    if(comp->compConfig->argv[argIndex])
    {
        strncpy(comp->compConfig->instantiationCMD,
                comp->compConfig->argv[argIndex],
                CL_MAX_NAME_LENGTH-1);
    }
    return CL_OK;
    
failure:
    return rc;
}

static ClRcT compConfigSet(ClCpmComponentT *comp, 
                           ClUint64T mask,
                           ClCharT *instantiateCommand,
                           ClAmsCompPropertyT property)
{
    ClRcT rc = CL_OK;
                           
    if ((mask == CL_AMS_CONFIG_ATTR_ALL) ||
        (mask & COMP_CONFIG_INSTANTIATE_COMMAND))
    {
        rc = cpmParseArgs(comp, instantiateCommand);
        if (rc != CL_OK) goto out;
    }

    if ((mask == CL_AMS_CONFIG_ATTR_ALL) ||
        (mask & COMP_CONFIG_PROPERTY))
    {
        comp->compConfig->compProperty = property;
    }

    clLogNotice("COMP", "CONFIG", "Updated comp config for component [%s]",
                comp->compConfig->compName);

    out:
    return rc;
}

ClRcT VDECL_VER(cpmCompConfigSet, 5, 1, 0)(ClEoDataT data,
                                           ClBufferHandleT inMsgHdl,
                                           ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;
    VDECL_VER(ClCpmCompConfigSetT, 5, 1, 0) compConfig;

    memset(&compConfig, 0, sizeof(compConfig));
    rc = VDECL_VER(clXdrUnmarshallClCpmCompConfigSetT, 5, 1, 0)(inMsgHdl, &compConfig);
    if(rc != CL_OK)
    {
        clLogError("COMP", "CONFIG", "Comp set config unmarshall returned [%#x]", rc);
        goto out;
    }

    clOsalMutexLock(gpClCpm->compTableMutex);
    rc = cpmCompFindWithLock((ClCharT*)compConfig.name, gpClCpm->compTable, &comp);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->compTableMutex);
        clLogError("COMP", "CONFIG", "Failed to find comp [%s]. Error [%#x]", 
                   compConfig.name, rc);
        goto out;
    }

    clOsalMutexLock(comp->compMutex);
    rc = compConfigSet(comp, compConfig.bitmask, 
                  (ClCharT*)compConfig.instantiateCommand,
                  (ClAmsCompPropertyT)compConfig.property);
    clOsalMutexUnlock(comp->compMutex);

    clOsalMutexUnlock(gpClCpm->compTableMutex);

    out:
    return rc;
}

/*
 * if the comp. is not node local, then update the config of the remote node.
 */
static ClRcT compSetConfig(ClAmsCompConfigT *compConfig, ClUint64T mask)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsEntityRefT *targetRef = &entityRef;
    ClAmsSUT *su = NULL;
    ClIocAddressT nodeAddress = {{0}};
    ClRcT rc = CL_OK;

    if(compConfig->parentSU.entity.type == CL_AMS_ENTITY_TYPE_SU
       && 
       compConfig->parentSU.entity.name.length > 0)
    {
        targetRef = &compConfig->parentSU;
    }
    else
    {
        ClAmsCompT *comp = NULL;
        memcpy(&targetRef->entity, &compConfig->entity, sizeof(targetRef->entity));
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                     targetRef);
        if(rc != CL_OK)
        {
            clLogError("COMP", "CONFIG", "Unable to find amf config for comp [%s]",
                       targetRef->entity.name.value);
            goto out;
        }
        comp  = (ClAmsCompT*)targetRef->ptr;
        AMS_CHECK_COMP(comp);
        targetRef = &comp->config.parentSU;
    }

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                 targetRef);
    if(rc != CL_OK)
    {
        clLogWarning("COMP", "CONFIG", "Unable to find parent SU [%s] for comp [%s]."
                     "Might not be configured.",
                     targetRef->entity.name.value, compConfig->entity.name.value);
        goto out;
    }
    
    su = (ClAmsSUT*)targetRef->ptr;
    AMS_CHECK_SU(su);

    rc = _cpmIocAddressForNodeGet(&su->config.parentNode.entity.name, &nodeAddress);
    if(rc != CL_OK)
    {
        clLogInfo("COMP", "CONFIG", "Unable to get ioc address for node [%s] "
                  "during config set for comp [%s]. Node might be down.", 
                  su->config.parentNode.entity.name.value,
                  compConfig->entity.name.value);
        goto out;
    }

    /*
     * If its node local, its a no-op
     */
    if(nodeAddress.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
    {
        rc = CL_CPM_RC(CL_ERR_NO_OP);
        goto out;
    }

    clLogNotice("COMP", "CONFIG", "Updating component config at node [%d]",
                nodeAddress.iocPhyAddress.nodeAddress);
    rc = clCpmCompConfigSet(nodeAddress.iocPhyAddress.nodeAddress,
                            compConfig->entity.name.value,
                            compConfig->instantiateCommand,
                            compConfig->property,
                            mask);

    out:
    return rc;
}

static ClRcT cpmCompSetConfig(ClAmsEntityConfigT *entityConfig,
                              ClUint64T bitMask)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;
    ClAmsCompConfigT *compConfig = (ClAmsCompConfigT *)entityConfig;

    clOsalMutexLock(gpClCpm->compTableMutex);

    rc = cpmCompFindWithLock(entityConfig->name.value, gpClCpm->compTable, &comp);
    if (!comp || comp->compPresenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
    {
        /*
         * if this comp. is part of another node, update remote node cpm config
         */
        clOsalMutexUnlock(gpClCpm->compTableMutex);
        rc = compSetConfig(compConfig, bitMask);
        if(rc != CL_OK && comp)
            goto out_set;

        goto out;
    }

    clOsalMutexUnlock(gpClCpm->compTableMutex);

    out_set:
    clOsalMutexLock(comp->compMutex);
        
    rc = compConfigSet(comp, bitMask, compConfig->instantiateCommand,
                       compConfig->property);

    clOsalMutexUnlock(comp->compMutex);

    out:
    return rc;
}

ClRcT cpmEntitySetConfig(ClAmsEntityConfigT *entityConfig, ClUint64T bitMask)
{
    switch(entityConfig->type)
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            return cpmCompSetConfig(entityConfig, bitMask);
        }
        default:
        {
            CL_ASSERT(0);
        }
    }
    
    return CL_OK;
}

static ClRcT cpmShowComp(ClCntNodeHandleT key,
                         ClCntDataHandleT data,
                         ClCntArgHandleT arg,
                         ClUint32T size)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)data;
    FILE *fp = (FILE *)arg;
    ClUint32T i = 0;

    fprintf(fp, "----------------------------------------\n");
    
    fprintf(fp, "Component name : [%s]\n", comp->compConfig->compName);

    fprintf(fp,
            "ASP component ? [%s]\n",
            (comp->compConfig->isAspComp ? "Yes" : "No"));

    fprintf(fp,
            "Component property : [%s]\n",
            (comp->compConfig->compProperty ==
             CL_AMS_COMP_PROPERTY_SA_AWARE ? "SA aware" :
             CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE ?
             "Proxied preinstantiable":
             CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE ?
             "Proxied Non preinstantiable": "Invalid"));

    fprintf(fp,
            "Component process relationship : [%s]\n",
            (comp->compConfig->compProcessRel ==
             CL_CPM_COMP_SINGLE_PROCESS ? "single process" :
             CL_CPM_COMP_MULTI_PROCESS  ? "multi process" :
             CL_CPM_COMP_THREADED ? "threaded" : "Invalid"));

    fprintf(fp, "Executable name : [%s]\n", comp->compConfig->instantiationCMD);
    
    for (i = 0; comp->compConfig->argv[i]; ++i)
    {
        fprintf(fp, "Argument [%d] -> [%s]\n", i, comp->compConfig->argv[i]);
    }

    for (i = 0; comp->compConfig->env[i]; ++i)
    {
        fprintf(fp,
                "Env variable name [%d] -> [%s]\n",
                i,
                comp->compConfig->env[i]->envName);
        
        fprintf(fp,
                "Env variable value [%d] -> [%s]\n",
                i,
                comp->compConfig->env[i]->envValue);
    }

    fprintf(fp,
            "Termination command : [%s]\n",
            comp->compConfig->terminationCMD);
              
    fprintf(fp,
            "Cleanup command : [%s]\n",
            comp->compConfig->cleanupCMD);
          
    fprintf(fp,
            "Instantiation timeout : [%d]\n",
            comp->compConfig->compInstantiateTimeout);
    
    fprintf(fp,
            "Termination timeout : [%d]\n",
            comp->compConfig->compTerminateCallbackTimeOut);
    
    fprintf(fp,
            "Cleanup timeout : [%d]\n",
            comp->compConfig->compCleanupTimeout);
    
    fprintf(fp,
            "Proxied component instantiate callback timeout : [%d]\n",
            comp->compConfig->compProxiedCompInstantiateCallbackTimeout);

    fprintf(fp,
            "Proxied component cleanup callback timeout : [%d]\n",
            comp->compConfig->compProxiedCompCleanupCallbackTimeout);
          
    fprintf(fp, "Component ID : [%#x]\n", comp->compId);

    fprintf(fp, "----------------------------------------\n");

    return CL_OK;
}

static ClRcT cpmShowNode(ClCntNodeHandleT key,
                         ClCntDataHandleT data,
                         ClCntArgHandleT arg,
                         ClUint32T size)
{
    ClCpmLT *node = (ClCpmLT *)data;
    FILE *fp = (FILE *)arg;

    fprintf(fp, "----------------------------------------\n");
    
    fprintf(fp, "Node name : [%s]\n", node->nodeName);

    fprintf(fp, "----------------------------------------\n");

    return CL_OK;
}

void cpmShowAll(ClCharT *entity)
{
    FILE *fp = NULL;
    ClCharT p[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *e = getenv("ASP_LOGDIR");
    ClCntHandleT cnt = 0;
    ClCntWalkCallbackT fn = NULL;

    if (!entity) return;

    if (!strcmp(entity, "comp"))
    {
        cnt = gpClCpm->compTable;
        fn = cpmShowComp;
    }
    else if (!strcmp(entity, "node"))
    {
        cnt = gpClCpm->cpmTable;
        fn = cpmShowNode;
    }
    else
    {
        CL_ASSERT(0); 
    }
    
        
    if (!e) e = "/tmp";

    clLogDebug("---","---","ASP_LOGDIR value %s",e);

    snprintf(p, CL_MAX_NAME_LENGTH-1, "%s/cpm_%ss.txt", e, entity);
    
    fp = fopen(p, "w");
    if (!fp) return;
    
    clCntWalk(cnt, fn, fp, 0);

    fclose(fp);
}

/*
 * Add all the components for this node created dynamically.
 */
ClRcT cpmComponentsAddDynamic(const ClCharT *node)
{
    ClAmsMgmtHandleT mgmtHandle = 0;
    ClVersionT version = {'B', 0x1, 0x1 };
    ClAmsEntityT entityNode = {0};
    ClAmsEntityBufferT suBuffer = {0};
    ClAmsEntityBufferT compBuffer = {0};
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    rc = clAmsMgmtInitialize(&mgmtHandle, NULL, &version);
    if(rc != CL_OK)
    {
        clLogError("DYNCOMPS", "SET", "Mgmt initialize returned [%#x]", rc);
        goto out;
    }
    
    clNameSet(&entityNode.name, node);
    ++entityNode.name.length;
    entityNode.type = CL_AMS_ENTITY_TYPE_NODE;

    rc = clAmsMgmtGetNodeSUList(mgmtHandle, &entityNode,
                                &suBuffer);
    if(rc != CL_OK)
    {
        clLogError("DYNCOMPS", "SET", "Get SU list for node [%s] returned [%#x]",
              node, rc);
        goto out_free;
    }

    for(i = 0; i < suBuffer.count; ++i)
    {
        ClAmsEntityT *su = suBuffer.entity+i;
        ClUint32T j = 0;
        rc = clAmsMgmtGetSUCompList(mgmtHandle, su,
                                    &compBuffer);
        if(rc != CL_OK)
        {
            clLogError("DYNCOMPS", "SET", "SU [%.*s] comp list returned [%#x]",
                       su->name.length-1, su->name.value, rc);
            goto out_free;
        }
        for(j = 0; j < compBuffer.count; ++j)
        {
            ClAmsEntityT *comp = compBuffer.entity+j;
            ClAmsCompConfigT compConfig = {{0}};
            ClAmsEntityConfigT *entityConfig = NULL;
            rc = clAmsMgmtEntityGetConfig(mgmtHandle, comp,
                                          &entityConfig);
            if(rc != CL_OK)
            {
                clLogError("DYNCOMPS", "SET", "Comp [%.*s] get config returned [%#x]", 
                           comp->name.length-1, comp->name.value, rc);
                goto out_free;
            }
            memcpy(&compConfig, entityConfig, sizeof(compConfig));
            clHeapFree(entityConfig);
            clLogNotice("DYNCOMPS", "SET", "Adding comp [%.*s] - node [%s]",
                        comp->name.length-1, comp->name.value, node);
            rc = cpmComponentAdd(comp->name.value);
            if(rc != CL_OK)
            {
                clLogError("DYNCOMPS", "SET", "Component add returned [%#x]", rc);
                goto out_free;
            }
            rc = cpmCompSetConfig(&compConfig.entity, CL_AMS_CONFIG_ATTR_ALL);
            if(rc != CL_OK)
            {
                cpmComponentRmv(comp->name.value);
                clLogError("DYNCOMPS", "SET", "Comp set config returned [%#x]", rc);
                goto out_free;
            }
        }
        clHeapFree(compBuffer.entity);
        compBuffer.entity = NULL;
        compBuffer.count = 0;
    }

    out_free:

    if(suBuffer.entity) clHeapFree(suBuffer.entity);
    if(compBuffer.entity) clHeapFree(compBuffer.entity);
    clAmsMgmtFinalize(mgmtHandle);

    out:
    return rc;
}

/*
 * Add the component to this node.
 */

ClRcT cpmComponentAddDynamic(const ClCharT *compName)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT comp = {0};
    ClAmsCompConfigT compConfig = {{0}};
    ClAmsMgmtHandleT mgmtHandle = 0;
    ClVersionT version = {'B', 0x1, 0x1 };

    if(!compName) return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);

    rc = clAmsMgmtInitialize(&mgmtHandle, 0, &version);
    if(rc != CL_OK)
    {
        clLogError("DYNCOMP", "SET", "Mgmt initialize returned [%#x]", rc);
        goto out;
    }
    
    comp.type = CL_AMS_ENTITY_TYPE_COMP;
    clNameSet(&comp.name, compName);
    ++comp.name.length;

    /*
     * If its the master, assume that instantiate is invoked with
     * AMS db lock held and directly check the DB.
     */
    if(CL_CPM_IS_ACTIVE())
    {
        ClAmsEntityRefT entityRef = {{0}};
        memcpy(&entityRef.entity, &comp, sizeof(entityRef.entity));
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                     &entityRef);
        if(rc != CL_OK)
        {
            clLogError("DYNCOMP", "SET", "Component [%s] not found in AMS db",
                       compName);
            goto out_free;
        }
        memcpy(&compConfig, &((ClAmsCompT*)entityRef.ptr)->config, sizeof(compConfig));
    }
    else
    {
        ClAmsEntityConfigT *entityConfig = NULL;

        rc = clAmsMgmtEntityGetConfig(mgmtHandle, &comp,
                                      &entityConfig);
        if(rc != CL_OK)
        {
            clLogError("DYNCOMP", "SET", "Comp [%s] not found in AMS db",
                       compName);
            goto out_free;
        }
    
        memcpy(&compConfig, entityConfig, sizeof(compConfig));
        clHeapFree(entityConfig);
    }
    clLogNotice("DYNCOMP", "SET", "Adding comp [%s]", compName);

    rc = cpmComponentAdd(comp.name.value);
    if(rc != CL_OK)
    {
        clLogError("DYNCOMP", "SET", "Component add returned [%#x]", rc);
        goto out_free;
    }
    rc = cpmCompSetConfig(&compConfig.entity, CL_AMS_CONFIG_ATTR_ALL);
    if(rc != CL_OK)
    {
        cpmComponentRmv(comp.name.value);
        clLogError("DYNCOMP", "SET", "Comp set config returned [%#x]", rc);
        goto out_free;
    }
    
    out_free:
    clAmsMgmtFinalize(mgmtHandle);

    out:
    return rc;
}
