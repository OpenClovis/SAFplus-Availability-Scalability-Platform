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

#include <clAmsMgmtMigrate.h>
#include <clCpmInternal.h>

static ClRcT clAmsMgmtSGRedundancyModelNoRedundancy(ClAmsSGRedundancyModelT model,
                                                    const ClCharT *sg,
                                                    const ClCharT *prefix,
                                                    ClUint32T numActiveSUs,
                                                    ClUint32T numStandbySUs,
                                                    ClAmsMgmtMigrateListT *migrateList)
{
    ClAmsSGConfigT sgConfig = {{0}};
    ClAmsEntityConfigT *entityConfig = NULL;
    ClRcT rc = CL_OK;
    ClAmsEntityT sgName = {0};
    ClUint64T bitMask = 0;

    sgName.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&sgName.name, sg);
    ++sgName.name.length;
    rc = clAmsMgmtEntityGetConfig(gHandle, &sgName, &entityConfig);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS entity get config returned [%#x]", rc);
        goto out;
    }

    memcpy(&sgConfig, entityConfig, sizeof(sgConfig));
    clHeapFree(entityConfig);

    if(sgConfig.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_NONE)
    {
        clLogWarning("AMS", "MIGRATE", "Redundancy model is already no redundancy");
        goto out;
    }

    bitMask |= SG_CONFIG_REDUNDANCY_MODEL;
    sgConfig.redundancyModel = CL_AMS_SG_REDUNDANCY_MODEL_NONE;

    bitMask |= SG_CONFIG_NUM_PREF_ACTIVE_SUS_PER_SI;
    sgConfig.numPrefActiveSUsPerSI = 1;
    
    bitMask |= SG_CONFIG_NUM_PREF_ACTIVE_SUS;
    sgConfig.numPrefActiveSUs = numActiveSUs;

    bitMask |= SG_CONFIG_NUM_PREF_STANDBY_SUS;
    sgConfig.numPrefStandbySUs = numStandbySUs;
    
    bitMask |= SG_CONFIG_NUM_PREF_INSERVICE_SUS;
    sgConfig.numPrefInserviceSUs = numActiveSUs + numStandbySUs;

    bitMask |= SG_CONFIG_NUM_PREF_ASSIGNED_SUS;
    sgConfig.numPrefAssignedSUs = numActiveSUs + numStandbySUs;

    rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle, &sgConfig.entity, bitMask);

    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS sg set config returned [%#x]", rc);
        goto out;
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);

    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS database commit returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT clAmsMgmtSGRedundancyModelEstimate(ClAmsSGRedundancyModelT model,
                                                ClAmsEntityT *sgName,
                                                ClUint32T numActiveSUs,
                                                ClUint32T numStandbySUs,
                                                ClInt32T *extraSIs,
                                                ClInt32T *extraSUs,
                                                ClInt32T *extraNodes)
{
    ClRcT rc = CL_OK;
    ClAmsEntityConfigT *entityConfig = NULL;
    ClAmsSGConfigT sgConfig = {{0}};
    ClAmsEntityBufferT suBuffer = {0};
    ClAmsEntityBufferT nodeBuffer = {0};
    ClAmsEntityBufferT siBuffer = {0};

    if(!sgName || !extraSIs || !extraSUs || !extraNodes) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    *extraSIs = 0, *extraSUs = 0, *extraNodes = 0;
    rc = clAmsMgmtEntityGetConfig(gHandle, sgName, &entityConfig);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "SG redundancy estimate -get config returned [%#x]", rc);
        goto out;
    }
    memcpy(&sgConfig, entityConfig, sizeof(sgConfig));
    clHeapFree(entityConfig);

    if(sgConfig.numPrefActiveSUs < numActiveSUs)
    {
        rc = clAmsMgmtGetSGSIList(gHandle, sgName, &siBuffer);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS sg si list returned [%#x]", rc);
            goto out_free;
        }
        if(siBuffer.count < numActiveSUs)
        {
            *extraSIs = numActiveSUs - siBuffer.count;
        }
    }
    rc = clAmsMgmtGetSGSUList(gHandle, sgName, &suBuffer);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS sg su list returned [%#x]", rc);
        goto out_free;
    }
    if(suBuffer.count < numStandbySUs + numActiveSUs)
    {
        *extraSUs = (numStandbySUs + numActiveSUs)-suBuffer.count;
        *extraSIs *= sgConfig.maxActiveSIsPerSU;
        rc = clAmsMgmtGetNodeList(gHandle, &nodeBuffer);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS get node list returned [%#x]", rc);
            goto out_free;
        }
        if(nodeBuffer.count < numActiveSUs + numStandbySUs)
        {
            *extraNodes = (numActiveSUs + numStandbySUs) - nodeBuffer.count;
        }
    }

    rc = CL_OK;

    out_free:
    if(suBuffer.entity) clHeapFree(suBuffer.entity);
    if(nodeBuffer.entity) clHeapFree(nodeBuffer.entity);
    if(siBuffer.entity) clHeapFree(siBuffer.entity);

    out:
    return rc;
}

/*
 * Try distributing it equally among the existing nodes.
 * First try distributing them in a non-colocated way. And then push
 * the remaining in the existing list.
 */

static ClRcT clAmsMgmtGetSUFreeNodes(ClAmsEntityT *sgName,
                                     const ClCharT *prefix,
                                     ClInt32T extraSUs,
                                     ClInt32T extraNodes,
                                     ClAmsEntityT *nodes,
                                     ClInt32T *pNumNodes)
{
    ClRcT rc = CL_OK;
    ClInt32T i;
    ClAmsEntityBufferT suBuffer = {0};
    ClAmsEntityBufferT nodeBuffer = {0};
    ClAmsEntityT *nodeList = NULL;
    ClAmsEntityConfigT *entityConfig = NULL;
    ClAmsSUConfigT suConfig = {{0}};
    ClAmsEntityT *controllers = NULL;
    ClAmsEntityT *workers = NULL;
    ClUint32T numControllers = 0;
    ClUint32T numWorkers = 0;
    ClUint32T totalNodes = 0;

    if(!nodes || !pNumNodes)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!extraSUs) return rc;

    *pNumNodes = 0;

    rc = clAmsMgmtGetNodeList(gHandle, &nodeBuffer);

    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "Get node list returned [%#x]", rc);
        goto out;
    }        

    /*
     * First try distributing to the extra nodes.
     */
    if(extraNodes > 0)
    {
        for(i = nodeBuffer.count; i < nodeBuffer.count + extraNodes; ++i)
        {
            nodes[i - nodeBuffer.count].type= CL_AMS_ENTITY_TYPE_NODE;
            snprintf(nodes[i-nodeBuffer.count].name.value, sizeof(nodes[i-nodeBuffer.count].name.value),
                     "%s_Node%d", prefix, i);
            nodes[i-nodeBuffer.count].name.length = strlen(nodes[i-nodeBuffer.count].name.value)+1;
            nodes[i-nodeBuffer.count].debugFlags = 1;
        }
        *pNumNodes = extraNodes;
    }

    if(extraSUs <= extraNodes) 
    {
        clHeapFree(nodeBuffer.entity);
        return CL_OK;
    }

    controllers = clHeapCalloc(nodeBuffer.count, sizeof(*controllers));
    workers = clHeapCalloc(nodeBuffer.count, sizeof(*controllers));
    CL_ASSERT(controllers && workers);

    /*
     * Separate controllers and workers as node distribution preference is
     * higher for workers than controllers.
     */
    for(i = 0 ; i < nodeBuffer.count; ++i)
    {
        ClCpmLT *cpmL = NULL;
        rc = cpmNodeFind(nodeBuffer.entity[i].name.value, &cpmL);
        if(rc != CL_OK)
        {
            /*
             * We skip the controller and worker ordering.
             */
            if(controllers)
            {
                clHeapFree(controllers);
            }
            if(workers)
            {
                clHeapFree(workers);
            }
            controllers = nodeBuffer.entity;
            numControllers = nodeBuffer.count;
            break;
        }
        if(!strcmp(cpmL->classType, "CL_AMS_NODE_CLASS_C"))
        {
            memcpy(workers+numWorkers, nodeBuffer.entity+i, 
                   sizeof(*workers));
            ++numWorkers;
        }
        else
        {
            memcpy(controllers+numControllers, nodeBuffer.entity+i,
                   sizeof(*controllers));
            ++numControllers;
        }
    }

    nodeList = clHeapCalloc(nodeBuffer.count + extraNodes,
                            sizeof(ClAmsEntityT));
    CL_ASSERT(nodeList);
    
    memcpy(nodeList, nodes, sizeof(*nodes)*extraNodes);
    if(workers)
    {
        memcpy(nodeList + extraNodes, workers, 
               sizeof(*workers) * numWorkers);
        clHeapFree(workers);
    }
    if(controllers)
    {
        memcpy(nodeList + extraNodes + numWorkers, controllers,
               sizeof(*controllers) * numControllers);
        clHeapFree(controllers);
    }

    clHeapFree(nodeBuffer.entity);
    extraSUs -= extraNodes;

    for(i = 0; i < extraNodes; ++i)
        nodeList[i].debugFlags = 1;

    for(; i < nodeBuffer.count + extraNodes; ++i)
        nodeList[i].debugFlags = 0;
    
    /*
     * Now find nodes that are unmapped to the SUs of this SG
     */
    rc = clAmsMgmtGetSGSUList(gHandle, sgName, &suBuffer);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "SG su list returned [%#x]", rc);
        goto out_free;
    }

    for(i = 0; i < suBuffer.count; ++i)
    {
        ClInt32T j;
        rc = clAmsMgmtEntityGetConfig(gHandle, suBuffer.entity+i,
                                      &entityConfig);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "SU get config returned [%#x]", rc);
            goto out_free;
        }
        memcpy(&suConfig, entityConfig, sizeof(suConfig));
        clHeapFree(entityConfig);
        for(j = 0; j < nodeBuffer.count+extraNodes; ++j)
        {
            if(!strncmp(nodeList[j].name.value, suConfig.parentNode.entity.name.value,
                        nodeList[j].name.length-1))
            {
                /*
                 * Mark this entry as seen.
                 */
                nodeList[j].debugFlags = 1;
            }
        }
    }

    clLogInfo("AMS", "MIGRATE", "Scanning free node list");
    totalNodes = nodeBuffer.count + extraNodes;
    for(i = 0; (i < totalNodes) && extraSUs ; ++i)
    {
        if(!nodeList[i].debugFlags)
        {
            clLogInfo("AMS", "MIGRATE", "Copying node [%s]", nodeList[i].name.value);
            memcpy(nodes + extraNodes, nodeList+i, sizeof(ClAmsEntityT));
            ++extraNodes;
            --extraSUs;
        }
    }

    /*
     * Distribute the remaining cyclically.
     */
    if(extraSUs && extraNodes)
    {
        ClInt32T index = 0;
        ClInt32T currentNodes = extraNodes;
        while(extraSUs--)
        {
            memcpy(nodes+extraNodes, nodes+index, sizeof(ClAmsEntityT));
            ++index;
            index %= currentNodes;
            ++extraNodes;
        }
    }

    *pNumNodes = extraNodes;
    rc = CL_OK;

    out_free:
    if(suBuffer.entity) clHeapFree(suBuffer.entity);
    if(nodeList) clHeapFree(nodeList);
    
    out:
    return rc;
}                                     

static void *clAmsMgmtMigrateListUnlock(void *arg)
{
    ClAmsMgmtMigrateListT *unlockList = arg;
    ClInt32T i;
    ClRcT rc = CL_OK;

    if(!unlockList) return NULL;

#if 0
    /*
     * The user could modify the SI-CSI list for the newly created SI.
     * So we don't want to unlock this SI and have this assigned active to an SU 
     * when running under SG reduction procedure.
     */
    for(i = 0; i < unlockList->si.count; ++i)
    {
        ClAmsEntityT *si = unlockList->si.entity+i;
        clLogNotice("AMS", "MIGRATE", "Unlocking created SI [%s]",
                    si->name.value);
        clAmsMgmtEntityUnlock(gHandle, si);
    }
#endif

    for(i = 0; i < unlockList->node.count; ++i)
    {
        ClAmsEntityT *node = unlockList->node.entity + i;
        clLogNotice("AMS", "MIGRATE", "Unlocking created Node [%s]",
                    node->name.value);
        rc = clAmsMgmtEntityLockAssignment(gHandle, node);
        if(rc == CL_OK)
        {
            rc = clAmsMgmtEntityUnlock(gHandle, node);
        }
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "Node [%s] unlock returned [%#x]", 
                       node->name.value, rc);
        }
    }

#if 0
    for(i = 0; i < unlockList->su.count; ++i)
    {
        ClAmsEntityT *su = unlockList->su.entity + i;
        clLogNotice("AMS", "MIGRATE", "Unlocking created SU [%s]",
                    su->name.value);
        rc = clAmsMgmtEntityLockAssignment(gHandle, su);
        if(rc == CL_OK)
        {
            rc = clAmsMgmtEntityUnlock(gHandle, su);
        }
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "SU [%s] unlock returned [%#x]",
                       su->name.value, rc);
        }
    }
#endif

    clHeapFree(unlockList->si.entity);
    clHeapFree(unlockList->node.entity);
    clHeapFree(unlockList->su.entity);
    clHeapFree(unlockList);

    return NULL;
}

static ClRcT clAmsMgmtSGMigrateMPlusN(ClAmsSGRedundancyModelT model,
                                      ClAmsEntityT *sgName,
                                      const ClCharT *prefix,
                                      ClUint32T numActiveSUs,
                                      ClUint32T numStandbySUs,
                                      ClAmsMgmtMigrateListT *migrateList)
{
    ClInt32T i;
    ClRcT rc = CL_OK;
    ClAmsEntityBufferT siBuffer = {0};
    ClAmsEntityBufferT suBuffer = {0};
    ClAmsEntityBufferT nodeBuffer = {0};
    ClInt32T extraSIs = 0;
    ClInt32T extraSUs = 0;
    ClInt32T extraNodes = 0;
    ClAmsEntityT *nodeList = NULL;
    ClAmsEntityT *nodes = NULL;
    ClAmsEntityT *sus = NULL;
    ClAmsEntityT *comps = NULL;
    ClAmsEntityT *sis =  NULL;
    ClAmsEntityT *csis = NULL;
    ClInt32T numNodes = 0;
    ClAmsEntityConfigT *pSURefComp = NULL;
    ClAmsEntityConfigT *pSGRefSI = NULL;
    ClAmsEntityConfigT *pSIRefCSI = NULL;
    ClAmsEntityConfigT *pSGConfig = NULL;
    ClAmsSGConfigT sgConfig = {{0}};
    ClUint32T numSupportedCSITypes = 0;
    ClNameT *pNumSupportedCSITypes = NULL;
    ClAmsMgmtCCBHandleT ccbHandle = 0;
    ClAmsMgmtMigrateListT *unlockList = NULL;

    rc = clAmsMgmtEntityGetConfig(gHandle, sgName, &pSGConfig);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "SG [%.*s] config get returned [%#x]",
                   sgName->name.length-1, sgName->name.value, rc);
        goto out;
    }

    memcpy(&sgConfig, pSGConfig, sizeof(sgConfig));
    clHeapFree(pSGConfig);

    /*
     * If scaling down actives, ensure that those many service units are locked.
     */
    if(numActiveSUs < sgConfig.numPrefActiveSUs)
    {
        ClInt32T numShrinkSUs = sgConfig.numPrefActiveSUs - numActiveSUs;
        ClAmsEntityBufferT suList = {0};
        ClInt32T numOutOfServiceSUs = 0;
        rc = clAmsMgmtGetSGSUList(gHandle, sgName, &suList);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "SG [%.*s] su list returned [%#x]",
                       sgName->name.length-1, sgName->name.value, rc);
            goto out;
        }
        for(i = 0; i < suList.count; ++i)
        {
            ClAmsSUConfigT *pSUConfig = NULL;
            rc = clAmsMgmtEntityGetConfig(gHandle, suList.entity+i,
                                          (ClAmsEntityConfigT**)&pSUConfig);
            if(rc != CL_OK)
            {
                clHeapFree(suList.entity);
                clLogError("AMS", "MIGRATE", "SU [%.*s] get config returned [%#x]",
                           suList.entity[i].name.length-1, suList.entity[i].name.value, rc);
                goto out;
            }
            if(pSUConfig->adminState == CL_AMS_ADMIN_STATE_LOCKED_A
               ||
               pSUConfig->adminState == CL_AMS_ADMIN_STATE_LOCKED_I)
            {
                ++numOutOfServiceSUs;
            }
            clHeapFree(pSUConfig);
        }
        clHeapFree(suList.entity);
        if(numOutOfServiceSUs < numShrinkSUs)
        {
            clLogError("AMS", "MIGRATE", "Expected a minimum of [%d] SUs to be out of service to satisfy SG. "
                       "redundancy model shrink. Got [%d] out of service", numShrinkSUs,
                       numOutOfServiceSUs);
            rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY_STATE);
            goto out;
        }
    }

    rc = clAmsMgmtSGRedundancyModelEstimate(model, sgName, numActiveSUs, numStandbySUs,
                                            &extraSIs, &extraSUs, &extraNodes);

    if(rc != CL_OK)
    {
        goto out;
    }
    
    rc = clAmsMgmtCCBInitialize(gHandle, &ccbHandle);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS ccb initialize returned [%#x]", rc);
        goto out;
    }

    /* 
     * Add the existing SI CSI list to the supported list.
     */
    rc = clAmsMgmtGetSGSIList(gHandle, sgName, &siBuffer);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS sg si list returned [%#x]", rc);
        goto out;
    }

    if(siBuffer.count)
    {
        rc = clAmsMgmtEntityGetConfig(gHandle, siBuffer.entity,
                                      &pSGRefSI);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS reference si get config returned [%#x]",
                       rc);
            goto out_free;
        }
    }

    for(i = 0; i < siBuffer.count; ++i)
    {
        ClInt32T j; 
        ClAmsEntityBufferT csiBuffer = {0};
        ClAmsSIConfigT siConfig = {{0}};
        ClUint64T mask = 0;
        memcpy(&siConfig.entity, siBuffer.entity+i, 
               sizeof(siConfig.entity));
        mask |= SI_CONFIG_NUM_STANDBY_ASSIGNMENTS;
        siConfig.numStandbyAssignments = numStandbySUs;
        if(numActiveSUs > 1)
            siConfig.numStandbyAssignments = (numStandbySUs+1)&~1;
        siConfig.numStandbyAssignments = CL_MAX(1, siConfig.numStandbyAssignments/
                                                (numActiveSUs?numActiveSUs:1));
        /*
         * Update the num standby assignments.
         */
        rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &siConfig.entity, mask);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "SI [%.*s] num standby set returned [%#x]",
                       siConfig.entity.name.length-1, siConfig.entity.name.value, rc);
        }

        rc = clAmsMgmtGetSICSIList(gHandle, siBuffer.entity+i,
                                   &csiBuffer);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS get si csi list returned [%#x]", rc);
            goto out_free;
        }
        pNumSupportedCSITypes = clHeapRealloc(pNumSupportedCSITypes,
                                              (numSupportedCSITypes+csiBuffer.count)*sizeof(ClNameT));
        for(j = 0; j < csiBuffer.count ; ++j)
        {
            ClAmsEntityConfigT *entityConfig = NULL;
            ClAmsCSIConfigT csiConfig = {{0}};
            ClInt32T k;

            rc = clAmsMgmtEntityGetConfig(gHandle, csiBuffer.entity+j,
                                          &entityConfig);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS csi get config returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&csiConfig, entityConfig, sizeof(csiConfig));
            if(!pSIRefCSI)
            {
                pSIRefCSI = entityConfig;
            }
            else
            {
                clHeapFree(entityConfig);
            }
            /*
             * Search for this csi type in the list to see if its 
             * already present
             */
            for(k = 0; k < numSupportedCSITypes; ++k)
            {
                if(!memcmp(pNumSupportedCSITypes[k].value,
                           csiConfig.type.value,
                           pNumSupportedCSITypes[k].length))
                    break;
            }

            if(k == numSupportedCSITypes)
            {
                memcpy(pNumSupportedCSITypes+numSupportedCSITypes,
                       &csiConfig.type, sizeof(csiConfig.type));
                ++numSupportedCSITypes;
            }
        }

        clHeapFree(csiBuffer.entity);
    }
    
    if(extraSIs)
    {
        sis = clHeapCalloc(extraSIs, sizeof(ClAmsEntityT));
        CL_ASSERT(sis != NULL);
        csis = clHeapCalloc(extraSIs, sizeof(ClAmsEntityT));
        for(i = siBuffer.count; i < siBuffer.count + extraSIs; ++i)
        {
            ClAmsEntityT si ={0};
            ClAmsEntityT csi = {0};
            ClUint64T bitMask = 0;
            ClAmsSIConfigT siConfig = {{0}};
            ClAmsCSIConfigT csiConfig = {{0}};
            si.type = CL_AMS_ENTITY_TYPE_SI;
            snprintf(si.name.value, sizeof(si.name.value)-1, "%s_%.*s_SI%d", prefix,
                     sgName->name.length-1, sgName->name.value, i);
            clLogNotice("AMS", "MIGRATE", "Creating SI [%s]", si.name.value);
            si.name.length = strlen(si.name.value)+1;
            rc = clAmsMgmtCCBEntityCreate(ccbHandle, &si);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS entity create returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&sis[i-siBuffer.count], &si, sizeof(si));
            
            rc = clAmsMgmtCCBSetSGSIList(ccbHandle, sgName, &si);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS set sg silist returned [%#x]", rc);
                goto out_free;
            }

            if(pSGRefSI)
            {
                /*
                 * Set config to the base SI.
                 */
                bitMask = CL_AMS_CONFIG_ATTR_ALL;
                memcpy(&siConfig, pSGRefSI, sizeof(siConfig));
                memcpy(&siConfig.entity, &si, sizeof(siConfig.entity));
                siConfig.numStandbyAssignments = numStandbySUs;
                if(numActiveSUs > 1 )
                    siConfig.numStandbyAssignments = (numStandbySUs+1)&~1;
                siConfig.numStandbyAssignments = CL_MAX(1,siConfig.numStandbyAssignments/
                                                        (numActiveSUs?numActiveSUs:1));
                siConfig.numCSIs = 1;
                siConfig.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
                rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &siConfig.entity, bitMask);
                if(rc != CL_OK)
                {
                    clLogError("AMS", "MIGRATE", "AMS entity set config returned [%#x]", rc);
                    goto out_free;
                }
            }

            csi.type = CL_AMS_ENTITY_TYPE_CSI;
            snprintf(csi.name.value, sizeof(csi.name.value),
                     "%s_CSI%d", si.name.value, i-siBuffer.count);
            csi.name.length = strlen(csi.name.value)+1;
            clLogNotice("AMS", "MIGRATE", "Creating CSI [%s]", csi.name.value);
            rc = clAmsMgmtCCBEntityCreate(ccbHandle, &csi);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS csi create returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&csis[i-siBuffer.count], &csi, sizeof(csi));
            rc = clAmsMgmtCCBSetSICSIList(ccbHandle, &si, &csi);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "SET si csi list returned [%#x]", rc);
                goto out_free;
            }
            
            if(pSIRefCSI)
            {
                /*
                 * Load the config. for the base csi type.
                 */
                memcpy(&csiConfig, pSIRefCSI, sizeof(csiConfig));
                memcpy(&csiConfig.entity, &csi, sizeof(csiConfig.entity));
                csiConfig.isProxyCSI = CL_FALSE;
                memcpy(&csiConfig.type, &csiConfig.entity.name, sizeof(csiConfig.type));
                bitMask = CL_AMS_CONFIG_ATTR_ALL;
                rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &csiConfig.entity, bitMask);
                if(rc != CL_OK)
                {
                    clLogError("AMS", "MIGRATE", "AMS ref csi set config returned [%#x]", rc);
                    goto out_free;
                }
            }
            /*
             * Add this to the supported list.
             */
            pNumSupportedCSITypes = clHeapRealloc(pNumSupportedCSITypes,
                                                  (numSupportedCSITypes+1)*sizeof(ClNameT));
            CL_ASSERT(pNumSupportedCSITypes != NULL);
            memcpy(pNumSupportedCSITypes+numSupportedCSITypes, &csi.name, sizeof(ClNameT));
            ++numSupportedCSITypes;
        }
    }

    if(extraNodes)
    {
        nodes = clHeapCalloc(extraNodes, sizeof(ClAmsEntityT));
        CL_ASSERT(nodes != NULL);

        rc = clAmsMgmtGetNodeList(gHandle, &nodeBuffer);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS get node list returned [%#x]", rc);
            goto out_free;
        }
        for(i = nodeBuffer.count ; i < nodeBuffer.count + extraNodes; ++i)
        {
            ClAmsEntityT node = {0};
            node.type = CL_AMS_ENTITY_TYPE_NODE;
            snprintf(node.name.value,  sizeof(node.name.value),
                     "%s_Node%d", prefix, i);
            node.name.length = strlen(node.name.value)+1;
            clLogNotice("AMS", "MIGRATE", "Creating node [%s]", node.name.value);
            rc = clAmsMgmtCCBEntityCreate(ccbHandle, &node);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS ccb create returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&nodes[i-nodeBuffer.count], &node, sizeof(node));
        }
    }

    rc = clAmsMgmtGetSGSUList(gHandle, sgName, &suBuffer);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "Get SG su list returned [%#x]", rc);
        goto out_free;
    }

    for(i = 0 ; i < suBuffer.count; ++i)
    {
        ClInt32T j;
        ClAmsEntityBufferT compBuffer=
            {
                0
            }
        ;
        rc = clAmsMgmtGetSUCompList(gHandle, &suBuffer.entity[i],
                                    &compBuffer);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "Get SU comp list returned [%#x]", rc);
            goto out_free;
        }
        /*
         * Get the first component properties.
         */
        if(!pSURefComp)
        {
            rc = clAmsMgmtEntityGetConfig(gHandle, compBuffer.entity,
                                          &pSURefComp);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS base comp get config returned [%#x]",
                           rc);
                goto out_free;
            }
        }

        /*
         * Update all config. with supported csi types.
         * and correct comp config whereever appropriate
         */
        for(j = 0; j < compBuffer.count; ++j)
        {
            ClAmsEntityConfigT *entityConfig =NULL;
            ClAmsCompConfigT compConfig = {{0}};
            ClUint64T bitMask = 0;
            ClInt32T k ;
            rc = clAmsMgmtEntityGetConfig(gHandle, compBuffer.entity+j,
                                          &entityConfig);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS comp get config returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&compConfig, entityConfig, sizeof(compConfig));
            clHeapFree(entityConfig);
            /*
             * update supported CSI type incase of SI additions.
             */
            if(extraSIs)
            {
                bitMask |= COMP_CONFIG_SUPPORTED_CSI_TYPE;
                compConfig.pSupportedCSITypes = 
                    clHeapRealloc(compConfig.pSupportedCSITypes, 
                                  (compConfig.numSupportedCSITypes
                                   + extraSIs)*
                                  sizeof(ClNameT));
                CL_ASSERT(compConfig.pSupportedCSITypes);
                for(k = compConfig.numSupportedCSITypes;
                    k < compConfig.numSupportedCSITypes + extraSIs;
                    ++k)
                {
                    memcpy(compConfig.pSupportedCSITypes+k,
                           &csis[k-compConfig.numSupportedCSITypes].name,
                           sizeof(ClNameT));
                }
                compConfig.numSupportedCSITypes += extraSIs;
            }
            bitMask |= COMP_CONFIG_NUM_MAX_STANDBY_CSIS;
            /*
             * take active to standby ratio
             */
            compConfig.numMaxStandbyCSIs = numActiveSUs;
            if(numStandbySUs > 1 )
                compConfig.numMaxStandbyCSIs = (numActiveSUs+1)&~1;
            compConfig.numMaxStandbyCSIs = CL_MAX(1, compConfig.numMaxStandbyCSIs/
                                                  (numStandbySUs?numStandbySUs:1));
            rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                             &compConfig.entity,
                                             bitMask);
            clHeapFree(compConfig.pSupportedCSITypes);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS entity set config returned [%#x]", rc);
                goto out_free;
            }
        }
        clHeapFree(compBuffer.entity);
    }

    if(extraSUs)
    {
        sus = clHeapCalloc(extraSUs, sizeof(ClAmsEntityT));
        CL_ASSERT(sus != NULL);
        comps = clHeapCalloc(extraSUs, sizeof(ClAmsEntityT));
        CL_ASSERT(comps != NULL);
        nodeList = clHeapCalloc(extraSUs + extraNodes, sizeof(ClAmsEntityT));
        CL_ASSERT(nodeList != NULL);

        rc = clAmsMgmtGetSUFreeNodes(sgName, prefix, extraSUs, extraNodes,
                                     nodeList, &numNodes);

        for(i = suBuffer.count; i < suBuffer.count + extraSUs; ++i)
        {
            ClAmsEntityT su = {0};
            ClAmsEntityT comp = {0};
            ClAmsSUConfigT suConfig = 
                {
                    {
                        0
                    }
                }
            ;
            ClAmsCompConfigT compConfig = {{0}};
            ClUint64T bitMask = 0;

            su.type = CL_AMS_ENTITY_TYPE_SU;
            snprintf(su.name.value, sizeof(su.name.value), 
                     "%s_%s_SU%d", prefix, nodeList[i-suBuffer.count].name.value, i);

            su.name.length = strlen(su.name.value)+1;
            clLogNotice("AMS", "MIGRATE", "Creating SU [%s]", su.name.value);
            rc = clAmsMgmtCCBEntityCreate(ccbHandle, &su);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "SU create returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&sus[i-suBuffer.count], &su, sizeof(su));
            /*
             * Assign this SU under the parent node and SG
             */
            rc = clAmsMgmtCCBSetNodeSUList(ccbHandle, &nodeList[i-suBuffer.count], &su);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "Node su list set returned [%#x]", rc);
                goto out_free;
            }

            /*
             * Give the parent SG. for this SU
             */
            rc = clAmsMgmtCCBSetSGSUList(ccbHandle, sgName, &su);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "Set SG su list returned [%#x]", rc);
                goto out_free;
            }
            bitMask = SU_CONFIG_NUM_COMPONENTS;
            memcpy(&suConfig.entity, &su, sizeof(suConfig.entity));
            suConfig.numComponents = 1;
            rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &suConfig.entity, bitMask);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "SU set config returned [%#x]", rc);
                goto out_free;
            }
            
            comp.type = CL_AMS_ENTITY_TYPE_COMP;
            snprintf(comp.name.value, sizeof(comp.name.value),
                     "%s_Comp%d",
                     su.name.value, i - suBuffer.count);
            comp.name.length = strlen(comp.name.value)+1;
            clLogNotice("AMS", "MIGRATE", "Creating component [%s]",
                        comp.name.value);
            rc = clAmsMgmtCCBEntityCreate(ccbHandle, &comp);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "Comp create returned [%#x]", rc);
                goto out_free;
            }
            memcpy(&comps[i-suBuffer.count], &comp, sizeof(comp));
            rc = clAmsMgmtCCBSetSUCompList(ccbHandle, &su, 
                                           &comp);
            if(rc != CL_OK)
            {
                clLogError("AMS", "MIGRATE", "AMS set su comp list returned [%#x]", rc);
                goto out_free;
            }
            
            if(pSURefComp)
            {
                /*
                 * At this stage, we have created the hierarchy. 
                 * Set the comp property to the base component type and
                 * add the num supported CSI types to be part of every component
                 * added to the SU.
                 */

                bitMask = CL_AMS_CONFIG_ATTR_ALL;
                memcpy(&compConfig, pSURefComp, sizeof(compConfig));
                memcpy(&compConfig.entity, &comp, sizeof(compConfig.entity));
                compConfig.numSupportedCSITypes = numSupportedCSITypes;
                compConfig.pSupportedCSITypes = pNumSupportedCSITypes;
                memcpy(&compConfig.parentSU.entity, &su, sizeof(compConfig.parentSU.entity));
                
                /*
                 * Distribute the standbys based on the active/standby ratio.
                 */
                compConfig.numMaxStandbyCSIs = numActiveSUs;
                if(numStandbySUs > 1 )
                    compConfig.numMaxStandbyCSIs = (numActiveSUs+1)&~1;
                compConfig.numMaxStandbyCSIs = CL_MAX(1, compConfig.numMaxStandbyCSIs/
                                                      (numStandbySUs?numStandbySUs:1));
                rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &compConfig.entity,
                                                 bitMask);
                if(rc != CL_OK)
                {
                    clLogError("AMS", "MIGRATE", "AMS set config returned [%#x]", rc);
                    goto out_free;
                }
            }
        }
    }

    /*
     * At this stage, we are all set to commit. after updating SG config.
     */
    {
        ClUint64T bitMask = 0;
        bitMask |= SG_CONFIG_REDUNDANCY_MODEL;
        sgConfig.redundancyModel = model;
        sgConfig.numPrefActiveSUs = numActiveSUs;
        bitMask |= SG_CONFIG_NUM_PREF_ACTIVE_SUS;
        sgConfig.numPrefStandbySUs = numStandbySUs;
        bitMask |= SG_CONFIG_NUM_PREF_STANDBY_SUS;
        if(sgConfig.numPrefInserviceSUs < numActiveSUs + numStandbySUs)
        {
            sgConfig.numPrefInserviceSUs = numActiveSUs + numStandbySUs;
            bitMask |= SG_CONFIG_NUM_PREF_INSERVICE_SUS;
        }
        sgConfig.numPrefAssignedSUs = numActiveSUs + numStandbySUs;
        bitMask |= SG_CONFIG_NUM_PREF_ASSIGNED_SUS;
        /*
         * Active standby ratio.
         */
        sgConfig.maxStandbySIsPerSU = numActiveSUs;
        if(numStandbySUs > 1 )
            sgConfig.maxStandbySIsPerSU = (numActiveSUs+1)&~1;
        sgConfig.maxStandbySIsPerSU = CL_MAX(1,sgConfig.maxStandbySIsPerSU/
                                             (numStandbySUs?numStandbySUs:1));
        bitMask |= SG_CONFIG_MAX_STANDBY_SIS_PER_SU;
        rc = clAmsMgmtCCBEntitySetConfig(ccbHandle, &sgConfig.entity, bitMask);
        if(rc != CL_OK)
        {
            clLogError("AMS", "MIGRATE", "AMS sg set config returned [%#x]", rc);
            goto out_free;
        }
    }

    rc = clAmsMgmtCCBCommit(ccbHandle);
    if(rc != CL_OK)
    {
        clLogError("AMS", "MIGRATE", "AMS database commit returned [%#x]", rc);
    }

    /*
     * Okay, the commit is successful. Now unlock all added entities
     * except SU so that other attributes could be updated before unlocking
     * Do that in a separate thread as there could be pending invocations.
     */
    unlockList = clHeapCalloc(1, sizeof(*unlockList));
    CL_ASSERT(unlockList != NULL);
    unlockList->si.count = extraSIs;
    unlockList->node.count = extraNodes;
    unlockList->su.count = extraSUs;
                                                
    unlockList->si.entity = clHeapCalloc(extraSIs, sizeof(*sis));
    unlockList->node.entity = clHeapCalloc(extraNodes, sizeof(*nodes));
    unlockList->su.entity = clHeapCalloc(extraSUs, sizeof(*sus));

    CL_ASSERT(unlockList->si.entity && unlockList->node.entity && unlockList->su.entity);

    memcpy(unlockList->si.entity, sis, sizeof(*sis)*extraSIs);
    memcpy(unlockList->node.entity, nodes, sizeof(*nodes)*extraNodes);
    memcpy(unlockList->su.entity, sus, sizeof(*sus) * extraSUs);

    clOsalTaskCreateDetached("MIGRATE-UNLOCK-THREAD", CL_OSAL_SCHED_OTHER, 0, 0, 
                             clAmsMgmtMigrateListUnlock, (void*)unlockList);

    /*
     * Return the newly created info. in the migrated list.
     */
    if(migrateList)
    {
        if(extraSIs)
        {
            migrateList->si.count = extraSIs;
            migrateList->si.entity = sis;
            migrateList->csi.count = extraSIs;
            migrateList->csi.entity = csis;
            sis = csis = NULL;
        }

        if(extraNodes)
        {
            migrateList->node.count = extraNodes;
            migrateList->node.entity = nodes;
            nodes = NULL;
        }

        if(extraSUs)
        {
            migrateList->su.count = extraSUs;
            migrateList->su.entity = sus;
            migrateList->comp.count = extraSUs;
            migrateList->comp.entity = comps;
            sus = comps = NULL;
        }
    }

    out_free:

    clAmsMgmtCCBFinalize(ccbHandle);

    if(siBuffer.entity) clHeapFree(siBuffer.entity);
    if(nodeBuffer.entity) clHeapFree(nodeBuffer.entity);
    if(suBuffer.entity) clHeapFree(suBuffer.entity);
    if(nodeList) clHeapFree(nodeList);
    if(nodes) clHeapFree(nodes);
    if(sus) clHeapFree(sus);
    if(comps) clHeapFree(comps);
    if(sis) clHeapFree(sis);
    if(csis) clHeapFree(csis);
    if(pSGRefSI) clHeapFree(pSGRefSI);
    if(pSIRefCSI) clHeapFree(pSIRefCSI);
    if(pSURefComp) clHeapFree(pSURefComp);
    if(pNumSupportedCSITypes) clHeapFree(pNumSupportedCSITypes);

    out:
    return rc;
}

static ClRcT clAmsMgmtSGRedundancyModelTwoN(ClAmsSGRedundancyModelT model,
                                            const ClCharT *sg,
                                            const ClCharT *prefix,
                                            ClUint32T numActiveSUs,
                                            ClUint32T numStandbySUs,
                                            ClAmsMgmtMigrateListT *migrateList)
                                            
{
    ClAmsEntityT sgName = {0};
    sgName.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&sgName.name, sg);
    ++sgName.name.length;
    return clAmsMgmtSGMigrateMPlusN(model, &sgName, prefix, numActiveSUs, numStandbySUs,
                                    migrateList);
}

static ClRcT clAmsMgmtSGRedundancyModelMPlusN(ClAmsSGRedundancyModelT model,
                                              const ClCharT *sg,
                                              const ClCharT *prefix,
                                              ClUint32T numActiveSUs,
                                              ClUint32T numStandbySUs,
                                              ClAmsMgmtMigrateListT *migrateList)
{
    ClAmsEntityT sgName = {0};
    sgName.type = CL_AMS_ENTITY_TYPE_SG;
    clNameSet(&sgName.name, sg);
    ++sgName.name.length;
    return clAmsMgmtSGMigrateMPlusN(model, &sgName, prefix, numActiveSUs, numStandbySUs, migrateList);
}

static ClRcT (*gClAmsMgmtSGRedundancyModelMigrationTable[CL_AMS_SG_REDUNDANCY_MODEL_MAX])
(ClAmsSGRedundancyModelT model, const ClCharT *sg, const ClCharT *prefix, ClUint32T numActiveSUs, ClUint32T numStandbySUs,
 ClAmsMgmtMigrateListT *migrateList) =
{
    [CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY] =
    clAmsMgmtSGRedundancyModelNoRedundancy,
    [CL_AMS_SG_REDUNDANCY_MODEL_TWO_N] =
    clAmsMgmtSGRedundancyModelTwoN,
    [CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N] =
    clAmsMgmtSGRedundancyModelMPlusN,
};


/*
* Change the redundancy model of the SG. on the fly.
* If its a forward move, create si/csi/su/comp with the prefix provided.
*/
ClRcT clAmsMgmtMigrateSGRedundancy(ClAmsSGRedundancyModelT model, 
                                   const ClCharT *sgName,
                                   const ClCharT *prefix,
                                   ClUint32T numActiveSUs,
                                   ClUint32T numStandbySUs,
                                   ClAmsMgmtMigrateListT *migrateList)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!sgName || !prefix) return rc;

    if(model >= CL_AMS_SG_REDUNDANCY_MODEL_MAX)
    {
        clLogError("AMS", "MIGRATE", "Invalid model [%d] specified.", model);
        goto out;
    }

    if(!gClAmsMgmtSGRedundancyModelMigrationTable[model])
    {
        clLogError("AMS", "MIGRATE", "Migration not supported model [%d]", model);
        goto out;
    }

    return gClAmsMgmtSGRedundancyModelMigrationTable[model](model, sgName, prefix, numActiveSUs, numStandbySUs, migrateList);

    out:
    return rc;
}
