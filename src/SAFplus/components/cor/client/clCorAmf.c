#include <clCorAmf.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsUtils.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clAmsMgmtOI.h>
#include <xdrClCorMOIdT.h>

static ClAmsMgmtHandleT mgmtHandle;
static ClCorTxnSessionIdT sessionId = 0;

static ClRcT corAmfObjectCommit(void)
{
    ClRcT rc = CL_OK;
    if(sessionId) 
    {
        rc = clCorTxnSessionCommit(sessionId);
        sessionId = 0;
    }
    return rc;
}


/*
 * Create the object and stuff in the moid inside the user data for the entity
 */

ClRcT corAmfObjectCreate(ClUint32T entityId, ClAmsEntityT *entity, ClCorMOIdT *pMoId)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msg = 0;
    ClUint32T moIdBufferLen = 0;
    ClUint8T *moIdBuffer = NULL;
    ClCorMOIdT tempMoId;
    ClCorMOServiceIdT svcId = CL_COR_SVC_ID_PROVISIONING_MANAGEMENT;
    ClCorAttributeValueListT attrValueList = {0};

    if(!gClAmfMibLoaded) 
        svcId = CL_COR_SVC_ID_AMF_MANAGEMENT;

    /*
     *Faster than a clone
     */
    memcpy(&tempMoId, pMoId, sizeof(tempMoId));

    rc = clCorObjectCreate(&sessionId, &tempMoId, NULL);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "COR MO object create for entity [%s] returned [%#x]",
                   entity->name.value, rc);
        goto out;
    }

    rc = clCorMoIdServiceSet(&tempMoId, svcId);
    CL_ASSERT(rc == CL_OK);

    rc = corAmfEntityAttributeListGet(&entityId, entity, NULL, &attrValueList);
    if(rc != CL_OK) goto out;
    
    rc = clCorObjectCreateAndSet(&sessionId, &tempMoId, &attrValueList, NULL);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "COR MSO object create for entity [%s] returned [%#x]",
                   entity->name.value, rc);
        goto out;
    }

    rc = clBufferCreate(&msg);
    CL_ASSERT(rc == CL_OK);

    rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)((ClUint8T*)&tempMoId, msg, 0);
    CL_ASSERT(rc == CL_OK);

    rc = clBufferLengthGet(msg, &moIdBufferLen);
    CL_ASSERT(rc == CL_OK);

    rc = clBufferFlatten(msg, &moIdBuffer);
    CL_ASSERT(rc == CL_OK);

    /*
     * Now store this moid buffer into the entity user area for the entity key.
     */
    rc = clAmsMgmtEntityUserDataSetKey(mgmtHandle, entity, &entity->name, 
                                       (ClCharT*)moIdBuffer, moIdBufferLen);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Entity user data set for [%s] returned [%#x]",
                   entity->name.value, rc);
        goto out;
    }

    out:
    if(attrValueList.pAttributeValue) clHeapFree(attrValueList.pAttributeValue);
    if(moIdBuffer) clHeapFree(moIdBuffer);
    if(msg) clBufferDelete(&msg);
    return rc;
}

static ClRcT corAmfExtendedClassObjectCreate(const ClCharT *pExtendedClassName,
                                             ClAmsEntityT *pEntity,
                                             ClCorMOIdT *pChassisMoId,
                                             ClUint32T rankListCount,
                                             ClUint32T *pClassIndex,
                                             ClUint32T *pIndex)
{
    ClCorAttributeValueListT attrList = {0};
    ClCorClassTypeT classType = 0;
    ClCorMOIdT moId;
    ClRcT rc = CL_OK;
    ClUint32T i;
    ClUint32T classIndex = 0;

    if(!pExtendedClassName || !pEntity || !pChassisMoId || !pClassIndex || !pIndex)
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    if(!rankListCount)
        return CL_OK;
    rc = corAmfExtendedClassAttributeListGet(pExtendedClassName, &attrList, &classType);
    if(rc != CL_OK)
        return rc;
    if(attrList.numOfValues != 2)
    {
        clLogError("COR", "AMF", "Expected [%d] keys but got [%d] keys in table [%s]",
                   2, attrList.numOfValues, pExtendedClassName);
        if(attrList.pAttributeValue)
        {
            clHeapFree(attrList.pAttributeValue);
            return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
        }
    }
    classIndex = *pClassIndex;
    for(i = 0; i < rankListCount; ++i)
    {
        memcpy(&moId, pChassisMoId, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classType, classIndex);
        CL_ASSERT(rc == CL_OK);
        rc = clCorObjectCreate(&sessionId, &moId, NULL);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "COR MO object create for entity [%s], table [%s] returned [%#x]",
                       pEntity->name.value, pExtendedClassName, rc);
            goto out_free;
        }
        rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        CL_ASSERT(rc == CL_OK);
        attrList.pAttributeValue[0].bufferPtr = (ClPtrT)pIndex;
        attrList.pAttributeValue[0].bufferSize = (ClUint32T)sizeof(*pIndex);
        attrList.pAttributeValue[1].bufferPtr = (ClPtrT)&i;
        attrList.pAttributeValue[1].bufferSize = (ClUint32T)sizeof(i);
        rc = clCorObjectCreateAndSet(&sessionId, &moId, &attrList, NULL);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Cor MSO object create for entity [%s], table [%s] returned [%#x]",
                       pEntity->name.value, pExtendedClassName, rc);
            goto out_free;
        }
        ++classIndex;
    }

    out_free:
    if(attrList.pAttributeValue)
    {
        clHeapFree(attrList.pAttributeValue);
    }
    *pClassIndex = classIndex;
    return rc;
}

ClRcT corAmfMibTreeInitialize(void)
{
    ClRcT rc = CL_OK;
    ClVersionT version = {'B', 0x1 , 0x1};
    ClCorMOIdT moId;
    ClCorMOIdT chassisMoid;
    ClAmsEntityBufferT nodeList = {0};
    ClAmsEntityBufferT suList = {0};
    ClAmsEntityBufferT compList = {0};
    ClAmsEntityBufferT sgList = {0};
    ClAmsEntityBufferT siList = {0};
    ClAmsEntityBufferT csiList = {0};
    ClCorClassTypeT classIds[CL_AMS_ENTITY_TYPE_MAX+2] = {0};
    ClInt32T chassisId = 0;
    SaNameT entityName = {0};
    ClUint32T i = 0;
    static ClUint32T extendedIndexTable[CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX];

    rc = corAmfEntityInitialize();
    if(rc != CL_OK) goto out;

    rc = clAmsMgmtInitialize(&mgmtHandle,  NULL, &version);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Mgmt initialize returned [%#x]", rc);
        goto out;
    }
    
    snprintf(entityName.value, sizeof(entityName.value), "%s:%d", "\\Chassis", chassisId);
    entityName.length = strlen(entityName.value);
    rc = clCorMoIdNameToMoIdGet(&entityName, &chassisMoid);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Cor moid get returned [%#x] for name [%s]",
                   rc, entityName.value);
        goto out;
    }

    for(i = 0; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        rc = corAmfEntityClassGet(i, classIds + i);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Entity class for [%s] not found",
                       CL_AMS_STRING_ENTITY_TYPE(i));
            goto out;
        }
    }

    if( (rc = clAmsMgmtGetNodeList(mgmtHandle, &nodeList)) != CL_OK)
    {
        clLogError("COR", "AMF", "Node list returned [%#x]", rc);
        goto out;
    }


    /*
     * Create objects for nodes/sus/comps/sgs/sis/csis
     */
    for(i = 0; i < nodeList.count; ++i)
    {
        memcpy(&moId, &chassisMoid, sizeof(moId));
        clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_NODE], i);
        rc = corAmfObjectCreate(i, &nodeList.entity[i], &moId);
        if(rc != CL_OK) goto out;
    }

    rc = clAmsMgmtGetSUList(mgmtHandle, &suList);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "SU list get returned [%#x]", rc);
        goto out;
    }

    for(i = 0; i < suList.count; ++i)
    {
        memcpy(&moId, &chassisMoid, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_SU], i);
        CL_ASSERT(rc == CL_OK);
        rc = corAmfObjectCreate(i, &suList.entity[i], &moId);
        if(rc != CL_OK) goto out;
    }
    
    rc = clAmsMgmtGetCompList(mgmtHandle, &compList);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Comp list get returned [%#x]", rc);
        goto out;
    }

    for(i = 0; i < compList.count; ++i)
    {
        memcpy(&moId, &chassisMoid, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_COMP], i);
        CL_ASSERT(rc == CL_OK);
        rc = corAmfObjectCreate(i, &compList.entity[i], &moId);
        if(rc != CL_OK) goto out;
    }

    rc = clAmsMgmtGetSGList(mgmtHandle, &sgList);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "SG list returned [%#x]", rc);
        goto out;
    }

    for(i = 0; i < sgList.count; ++i)
    {
        ClAmsEntityBufferT rankList = {0};
        memcpy(&moId, &chassisMoid, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_SG], i);
        CL_ASSERT(rc == CL_OK);
        rc = corAmfObjectCreate(i, &sgList.entity[i], &moId);
        if(rc != CL_OK) goto out;
        rc = clAmsMgmtGetSGSIList(mgmtHandle, &sgList.entity[i], &rankList);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SG [%s] si list returned [%#x]",
                       sgList.entity[i].name.value, rc);
            goto out;
        }
        rc = corAmfExtendedClassObjectCreate("saAmfSGSIRankTable",
                                             &sgList.entity[i],
                                             &chassisMoid,
                                             rankList.count,
                                             &extendedIndexTable[CL_AMS_MGMT_OI_SGSIRANK],
                                             &i);
        if(rankList.entity)
        {
            clHeapFree(rankList.entity);
            rankList.entity = NULL;
            rankList.count = 0;
        }
        
        rc = clAmsMgmtGetSGSUList(mgmtHandle, &sgList.entity[i], &rankList);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SG [%s] su list returned [%#x]",
                       sgList.entity[i].name.value, rc);
            goto out;
        }
        rc = corAmfExtendedClassObjectCreate("saAmfSGSURankTable",
                                             &sgList.entity[i],
                                             &chassisMoid,
                                             rankList.count,
                                             &extendedIndexTable[CL_AMS_MGMT_OI_SGSURANK],
                                             &i);
        if(rankList.entity)
        {
            clHeapFree(rankList.entity);
            rankList.count = 0;
        }
    }

    rc = clAmsMgmtGetSIList(mgmtHandle, &siList);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "SI list get returned [%#x]", rc);
        goto out;
    }

    for(i = 0; i < siList.count; ++i)
    {
        ClAmsEntityBufferT list = {0};
        memcpy(&moId, &chassisMoid, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_SI], i);
        CL_ASSERT(rc == CL_OK);
        rc = corAmfObjectCreate(i, &siList.entity[i], &moId);
        if(rc != CL_OK) goto out;
        
        rc = clAmsMgmtGetSIDependenciesList(mgmtHandle, &siList.entity[i],
                                            &list);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SI [%s] dependencies list returned [%#x]",
                       siList.entity[i].name.value, rc);
            goto out;
        }
        if(list.count)
        {
            rc = corAmfExtendedClassObjectCreate("saAmfSISIDepTable",
                                                 &siList.entity[i],
                                                 &chassisMoid,
                                                 list.count,
                                                 &extendedIndexTable[CL_AMS_MGMT_OI_SISIDEP],
                                                 &i);
        }

        if(list.entity)
        {
            clHeapFree(list.entity);
            list.count = 0;
        }
        rc = clAmsMgmtGetSISURankList(mgmtHandle, &siList.entity[i], &list);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SI [%s] su rank list returned [%#x]",
                       siList.entity[i].name.value, rc);
            goto out;
        }
        if(list.count)
        {
            rc = corAmfExtendedClassObjectCreate("saAmfSUsperSIRankTable",
                                                 &siList.entity[i],
                                                 &chassisMoid,
                                                 list.count,
                                                 &extendedIndexTable[CL_AMS_MGMT_OI_SUSPERSIRANK],
                                                 &i);
        }
        if(list.entity)
        {
            list.count = 0;
            clHeapFree(list.entity);
        }
    }
    
    rc = clAmsMgmtGetCSIList(mgmtHandle, &csiList);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "CSI list get returned [%#x]", rc);
        goto out;
    }
    for(i = 0; i < csiList.count; ++i)
    {
        ClAmsCSINVPBufferT buffer = {0};
        ClAmsEntityBufferT list = {0};
        memcpy(&moId, &chassisMoid, sizeof(moId));
        rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_CSI], i);
        CL_ASSERT(rc == CL_OK);
        rc = corAmfObjectCreate(i, &csiList.entity[i], &moId);
        if(rc != CL_OK) goto out;

        rc = clAmsMgmtGetCSINVPList(mgmtHandle, &csiList.entity[i], &buffer);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "CSI [%s] nvp list returned [%#x]",
                       csiList.entity[i].name.value, rc);
            goto out;
        }
        
        rc = corAmfExtendedClassObjectCreate("saAmfCSINameValueTable",
                                             &csiList.entity[i],
                                             &chassisMoid,
                                             buffer.count,
                                             &extendedIndexTable[CL_AMS_MGMT_OI_CSINAMEVALUE],
                                             &i);
        if(buffer.nvp)
        {
            clHeapFree(buffer.nvp);
        }

        rc = clAmsMgmtGetCSIDependenciesList(mgmtHandle, &csiList.entity[i],
                                             &list);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "CSI [%s] dependencies list returned [%#x]",
                       csiList.entity[i].name.value, rc);
            goto out;
        }
        
        rc = corAmfExtendedClassObjectCreate("saAmfCSICSIDepTable",
                                             &csiList.entity[i],
                                             &chassisMoid,
                                             list.count,
                                             &extendedIndexTable[CL_AMS_MGMT_OI_CSICSIDEP],
                                             &i);
        if(list.entity)
        {
            clHeapFree(list.entity);
        }
    }

    /*
     * We are here when the objects are all created. Do a commit
     */
    rc = corAmfObjectCommit();
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Object commit returned [%#x]", rc);
        goto out;
    }

    clLogNotice("COR", "AMF", "COR AMF tree successfully initialized");

    out:
    if(nodeList.entity) 
        clHeapFree(nodeList.entity);
    if(suList.entity)   
        clHeapFree(suList.entity);
    if(compList.entity) 
        clHeapFree(compList.entity);
    if(sgList.entity)   
        clHeapFree(sgList.entity);
    if(siList.entity)   
        clHeapFree(siList.entity);
    if(csiList.entity) 
        clHeapFree(csiList.entity);
    return rc;
}

/*
 * Create objects for the static AMF entities.
 */
ClRcT corAmfTreeInitialize(void)
{
    ClRcT rc = CL_OK;

    if(gClAmfMibLoaded)
    {
        rc = corAmfMibTreeInitialize();
    }
    else
    {
        ClVersionT version = {'B', 0x1 , 0x1};
        ClCorMOIdT moId;
        ClAmsEntityBufferT nodeList = {0};
        ClAmsEntityBufferT suList = {0};
        ClAmsEntityBufferT compList = {0};
        ClAmsEntityBufferT sgList = {0};
        ClAmsEntityBufferT siList = {0};
        ClAmsEntityBufferT csiList = {0};
        ClCorClassTypeT classIds[CL_AMS_ENTITY_TYPE_MAX+2] = {0};
        ClCorMOClassPathT sgClassPath;
        ClInt32T i;

        rc = corAmfEntityInitialize();
        if(rc != CL_OK) goto out;

        rc = clAmsMgmtInitialize(&mgmtHandle,  NULL, &version);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Mgmt initialize returned [%#x]", rc);
            goto out;
        }

        if( (rc = clAmsMgmtGetNodeList(mgmtHandle, &nodeList)) != CL_OK)
        {
            clLogError("COR", "AMF", "Node list returned [%#x]", rc);
            goto out;
        }

        /*
         * Create object heirarchy for nodes/sus/comps. 
         */
        for(i = 0; i < nodeList.count; ++i)
        {
            ClInt32T j;
            ClCorMOClassPathT nodeClassPath;

            rc = clCorMoIdInitialize(&moId);
            CL_ASSERT(rc == CL_OK);

            rc = clCorNodeNameToMoIdGet(nodeList.entity[i].name, &moId);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Node name to moid get for [%s] failed with [%#x]",
                           nodeList.entity[i].name.value, rc);
                goto out;
            }

            rc = clCorMoClassPathInitialize(&nodeClassPath);
            CL_ASSERT(rc == CL_OK);
        
            rc = clCorMoIdToMoClassPathGet(&moId, &nodeClassPath);
            CL_ASSERT(rc == CL_OK);

            /*
             * Now get nodes su list.
             */
            rc = clAmsMgmtGetNodeSUList(mgmtHandle, &nodeList.entity[i], &suList);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Node [%s] su list returned [%#x]", 
                           nodeList.entity[i].name.value, rc);
                goto out;
            }

            if(!classIds[CL_AMS_ENTITY_TYPE_SU])
            {
                rc = corAmfEntityClassGet(CL_AMS_ENTITY_TYPE_SU, classIds+CL_AMS_ENTITY_TYPE_SU);
                if(rc != CL_OK)
                {
                    clLogError("COR", "AMF", "Class for entity SU not found");
                    goto out;
                }
            }
        
            if(!classIds[CL_AMS_ENTITY_TYPE_COMP])
            {
                rc = corAmfEntityClassGet(CL_AMS_ENTITY_TYPE_COMP, classIds+CL_AMS_ENTITY_TYPE_COMP);
                if(rc != CL_OK)
                {
                    clLogError("COR", "AMF", "Class for entity comp not found");
                    goto out;
                }
            }

            rc = clCorMoClassPathAppend(&nodeClassPath, classIds[CL_AMS_ENTITY_TYPE_SU]);
            CL_ASSERT(rc == CL_OK);

            rc = corAmfMoClassCreate("AMFSu", &nodeClassPath, NULL);
            if(rc != CL_OK)
            {
                if(CL_GET_ERROR_CODE(rc) != CL_COR_MO_TREE_ERR_FAILED_TO_ADD_NODE)
                    goto out;
            }

            if(CL_GET_ERROR_CODE(rc) != CL_COR_MO_TREE_ERR_FAILED_TO_ADD_NODE)
            {
                rc = clCorMoClassPathAppend(&nodeClassPath, classIds[CL_AMS_ENTITY_TYPE_COMP]);
                CL_ASSERT(rc == CL_OK);
            
                rc = corAmfMoClassCreate("AMFComp", &nodeClassPath, NULL);
                if(rc != CL_OK) goto out;
            }
            else
            {
                rc = CL_OK;
            }

            for(j = 0; j < suList.count; ++j)
            {
                ClInt32T k;
                ClCorMOIdT suMoId;

                memcpy(&suMoId, &moId, sizeof(suMoId));
                rc = clCorMoIdAppend(&suMoId, classIds[CL_AMS_ENTITY_TYPE_SU], j);
                CL_ASSERT(rc == CL_OK);

                rc = corAmfObjectCreate(0, &suList.entity[j], &suMoId);
                if(rc != CL_OK) goto out;

                rc = clAmsMgmtGetSUCompList(mgmtHandle, &suList.entity[j], &compList);
                if(rc != CL_OK)
                {
                    goto out;
                }
            
                for(k = 0; k < compList.count; ++k)
                {
                    ClCorMOIdT compMoId;
   
                    memcpy(&compMoId, &suMoId, sizeof(compMoId));
                    rc = clCorMoIdAppend(&compMoId, classIds[CL_AMS_ENTITY_TYPE_COMP], k);
                    CL_ASSERT(rc == CL_OK);

                    rc = corAmfObjectCreate(0, &compList.entity[k], &compMoId);
                    if(rc != CL_OK) goto out;
                }
                clHeapFree(compList.entity);
                compList.entity = NULL;
            }
            clHeapFree(suList.entity);
            suList.entity = NULL;
        }

        /*
         * Configure object heirarchy for SGs.
         */
        rc = clAmsMgmtGetSGList(mgmtHandle, &sgList);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SG list returned [%#x]", rc);
            goto out;
        }

        rc = corAmfEntityClassGet(CL_AMS_ENTITY_TYPE_SG, &classIds[CL_AMS_ENTITY_TYPE_SG]);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SG entity class not found");
            goto out;
        }

        rc = corAmfEntityClassGet(CL_AMS_ENTITY_TYPE_SG, &classIds[CL_AMS_ENTITY_TYPE_SI]);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "SI entity class not found");
            goto out;
        }

        rc = corAmfEntityClassGet(CL_AMS_ENTITY_TYPE_SG, &classIds[CL_AMS_ENTITY_TYPE_CSI]);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "CSI entity class not found");
            goto out;
        }

        rc = clCorMoClassPathInitialize(&sgClassPath);
        CL_ASSERT(rc == CL_OK);
        rc = clCorMoClassPathAppend(&sgClassPath, classIds[CL_AMS_ENTITY_TYPE_SG]);
        CL_ASSERT(rc == CL_OK);

        rc = corAmfMoClassCreate("AMFSg", &sgClassPath, NULL);
        if(rc != CL_OK) goto out;
       
        rc = clCorMoClassPathAppend(&sgClassPath, classIds[CL_AMS_ENTITY_TYPE_SI]);
        CL_ASSERT(rc == CL_OK);
            
        rc = corAmfMoClassCreate("AMFSi", &sgClassPath, NULL);
        if(rc != CL_OK) goto out;

        rc = clCorMoClassPathAppend(&sgClassPath, classIds[CL_AMS_ENTITY_TYPE_CSI]);
        CL_ASSERT(rc == CL_OK);

        rc = corAmfMoClassCreate("AMFCsi", &sgClassPath, NULL);
        if(rc != CL_OK) goto out;

        for(i = 0; i < sgList.count; ++i)
        {
            ClInt32T j;

            rc = clCorMoIdInitialize(&moId);
            CL_ASSERT(rc == CL_OK);
            rc = clCorMoIdAppend(&moId, classIds[CL_AMS_ENTITY_TYPE_SG], i);
            CL_ASSERT(rc == CL_OK);

            rc = corAmfObjectCreate(0, &sgList.entity[i], &moId);
            if(rc != CL_OK) goto out;

            rc = clAmsMgmtGetSGSIList(mgmtHandle, &sgList.entity[i], &siList);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "SI list get for SG [%s] returned [%#x]",
                           sgList.entity[i].name.value, rc);
                goto out;
            }
        
            for(j = 0; j < siList.count; ++j)
            {
                ClInt32T k;
                ClCorMOIdT siMoId;

                memcpy(&siMoId, &moId, sizeof(siMoId));
                rc = clCorMoIdAppend(&siMoId, classIds[CL_AMS_ENTITY_TYPE_SI], j);
                CL_ASSERT(rc == CL_OK);

                rc = corAmfObjectCreate(0, &siList.entity[j], &siMoId);
                if(rc != CL_OK) goto out;

                rc = clAmsMgmtGetSICSIList(mgmtHandle, &siList.entity[j], &csiList);
                if(rc != CL_OK)
                {
                    clLogError("COR", "AMF", "CSI list for SI [%s] returned [%#x]",
                               siList.entity[j].name.value, rc);
                    goto out;
                }

                for(k = 0; k < csiList.count; ++k)
                {
                    ClCorMOIdT csiMoId;
                    memcpy(&csiMoId, &siMoId, sizeof(csiMoId));
                    rc = clCorMoIdAppend(&csiMoId, classIds[CL_AMS_ENTITY_TYPE_CSI], k);
                    CL_ASSERT(rc == CL_OK);
                    rc = corAmfObjectCreate(0, &csiList.entity[k], &csiMoId);
                    if(rc != CL_OK) goto out;
                }
                clHeapFree(csiList.entity);
                csiList.entity = NULL;
            }
            clHeapFree(siList.entity);
            siList.entity = NULL;
        }

        /*
         * We are here when the objects are all created. Do a commit
         */
        rc = corAmfObjectCommit();
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Object commit returned [%#x]", rc);
            goto out;
        }

        clLogNotice("COR", "AMF", "COR AMF tree successfully initialized");

        out:
        if(nodeList.entity) 
            clHeapFree(nodeList.entity);
        if(suList.entity)   
            clHeapFree(suList.entity);
        if(compList.entity) 
            clHeapFree(compList.entity);
        if(sgList.entity)   
            clHeapFree(sgList.entity);
        if(siList.entity)   
            clHeapFree(siList.entity);
        if(csiList.entity) 
            clHeapFree(csiList.entity);
    }
    return rc;
}

ClRcT clCorAmfMoIdGet(const ClCharT *name,
                      ClAmsEntityTypeT type,
                      ClCorMOIdT *pMoId)
{
    ClAmsEntityT entity = {0};
    ClBufferHandleT msg = 0;
    ClRcT rc = CL_OK;
    ClVersionT version = {'B', 0x1, 0x1};
    ClCharT *data = NULL;
    ClUint32T dataLen = 0;
    ClCorObjectHandleT objHandle;

    if(!name || !pMoId) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

    if(!mgmtHandle)
    {
        rc = clAmsMgmtInitialize(&mgmtHandle, NULL, &version);
        if(rc != CL_OK) return rc;
    }

    entity.type = type;
    saNameSet(&entity.name, name);
    ++entity.name.length;
    rc = clAmsMgmtEntityUserDataGetKey(mgmtHandle, &entity, &entity.name, &data, &dataLen);
    if(rc != CL_OK)
    {
        clLogError("FLT", "REPAIR", "Entity data get for [%s] returned [%#x]",
                   entity.name.value, rc);
        goto out_free;
    }
    rc = clBufferCreate(&msg);
    CL_ASSERT(rc == CL_OK);
    rc = clBufferNBytesWrite(msg, (ClUint8T*)data, dataLen);
    CL_ASSERT(rc == CL_OK);
    
    rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(msg, pMoId);
    CL_ASSERT(rc == CL_OK);
    
    clBufferDelete(&msg);

    clLogNotice("COR", "AMF", "MOID for faulty entity [%s] ", entity.name.value);
    clCorMoIdShow(pMoId);
    
    /*
     * Validating moid
     */
    rc = clCorObjectHandleGet(pMoId, &objHandle);
    CL_ASSERT(rc == CL_OK);

    out_free:
    if(msg) clBufferDelete(&msg);
    if(data) clHeapFree(data);

    return rc;
}
