
#include <string.h>

#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>

#include <clAlarmUtils.h>
#include <clSnmpocTrainInstXlation.h>
#include <clSnmpSubAgentocTrainConfig.h>
#include <clSnmpocTrainTables.h>
#include <clSnmpocTrainUtil.h>


ClRcT clSnmpalarmTrapIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
{
    ClSNMPRequestInfoT *pInst = NULL;
    ClUint32T instLen = 0;
    ClMedAgentIdT oid = { (ClUint8T *)"0.0", 4 }; /* Dummy OID. */
    ClRcT rc = CL_OK;

    if(NULL == pMoId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("moId is passed as NULL"));
        return CL_ERR_NULL_POINTER;
    }
    if(NULL == pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("pTlvList is passed as NULL"));
        return CL_ERR_NULL_POINTER;
    }

    switch(objectType)
    {
        case ALARMTRAP_CLOCKID:
        {
            /* OID = .1.3.6.1.4.1.103.2.1.1.2 */
            rc = clSnmpclockTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, 0);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clockTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpclockTableIndexTlvGet(&(pInst->index.clockTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clockTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpclockTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpclockTableIndexInfoT * pclockTableIndex = (ClSnmpclockTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clockTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 1;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClInt32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(pclockTableIndex->clockRow), sizeof (ClInt32T));
    pTlv->length = sizeof (ClInt32T);
    pTlv->type = CL_COR_INT32;
    return CL_OK;
}

ClRcT clSnmpnameTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpnameTableIndexInfoT * pnameTableIndex = (ClSnmpnameTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "nameTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 1;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(pnameTableIndex->nodeAdd), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpnameTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
{
    ClRcT rc = CL_OK;
    ClSNMPRequestInfoT *pReqInfo = (ClSNMPRequestInfoT *) pInst;
    ClAlarmUtilTlvInfoT indexTlvList = {0};
    ClCorAttrValueDescriptorListPtrT pAttrDescList = (ClCorAttrValueDescriptorListPtrT) pCorAttrValueDescList;
    ClCorAttrValueDescriptorPtrT pAttrDescriptor = NULL;
    ClAlarmUtilTlvT * pTlv = NULL;
    ClUint32T count = 0;

    if(pInst == NULL || pCorAttrValueDescList == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "NULL arguments received!");
        return CL_ERR_NULL_POINTER;
    }

    rc = clSnmpnameTableIndexTlvGet(&(pReqInfo->index.nameTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 1;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(1 * sizeof(ClCorAttrValueDescriptorT)); 

    pAttrDescriptor = pAttrDescList->pAttrDescriptor;
    if(pAttrDescriptor == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
    }

    if(CL_OK == rc)
    {
        for(count = 0;count < pAttrDescList->numOfDescriptor;count++)
        {
            ClCorAttrValueDescriptorT * pAttrDesc = pAttrDescriptor + count;
            pTlv = (indexTlvList.pTlv) + count;
     
            pAttrDesc->pAttrPath = nameTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = nameTableIndexCorAttrIdList[count].attrId;
            pAttrDesc->index = -1;
            pAttrDesc->pJobStatus = (ClCorJobStatusT *)clHeapAllocate(sizeof(ClCorJobStatusT));
            if(pAttrDesc->pJobStatus == NULL)
            {
                rc = CL_ERR_NO_MEMORY;
                break;
            }
    
            pAttrDesc->bufferSize = pTlv->length;
            pAttrDesc->bufferPtr = clHeapAllocate(pAttrDesc->bufferSize);
            if(pAttrDesc->bufferPtr == NULL)
            {
                rc = CL_ERR_NO_MEMORY;
                break;
            }
            memcpy(pAttrDesc->bufferPtr, pTlv->value, pAttrDesc->bufferSize);
        }
    }
    /* CLEAN UP */
    {
        if(rc != CL_OK && pAttrDescriptor != NULL)
        {
            for(count = 0;count < pAttrDescList->numOfDescriptor;count++)
            {
                ClCorAttrValueDescriptorT * pAttrDesc = pAttrDescriptor + count;
                clHeapFree(pAttrDesc->pJobStatus);
                clHeapFree(pAttrDesc->bufferPtr);
            }
        }
        for(count = 0;count < indexTlvList.numTlvs; count++)
        {
            pTlv = (indexTlvList.pTlv) + count;
            clHeapFree(pTlv->value);
        }
        clHeapFree(indexTlvList.pTlv);
        indexTlvList.pTlv = NULL;
    }

    return CL_OK;
}


