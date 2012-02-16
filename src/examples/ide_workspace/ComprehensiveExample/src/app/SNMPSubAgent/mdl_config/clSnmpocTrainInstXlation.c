
#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>
#include <clCorUtilityApi.h>
#include <clSnmpOp.h>
#include <clSnmpSubAgentocTrainConfig.h>
#include "clSnmpocTrainInstXlation.h"
#include <clSnmpocTrainTables.h>

#define CL_SNMP_GEN_XLA_COTX "GXL"

#define CL_MED_COR_STR_MOID_PRINT(corMoId)          \
do\
{ \
    ClNameT strMoId;                        \
    clCorMoIdToMoIdNameGet(corMoId, &strMoId );       \
    clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "%s", strMoId.value);\
}while(0)                                   \


ClRcT clSnmpocTrainDefaultInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp)
{
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Default Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);

    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpocTrainDefaultInstXlator Unable to allocate memory for the instance translation!");
            return (CL_ERR_NO_MEMORY);
        }
        *pInstLen = sizeof (ClSNMPRequestInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void**)pInst;
    }
    for(len = 0; len < pAgntId->len; len++)
    {
        pInst->oid[len] = pAgntId->id[len];
    }

    pInst->tableType = CL_OCTRAIN_SCALARS;
    return CL_OK;
}

/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpclockTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpclockTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpocTrainDefaultInstXlator Unable to allocate memory for the instance translation!");
            return (CL_ERR_NO_MEMORY);
        }
        *pInstLen = sizeof (ClSNMPRequestInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void**)pInst;
    }
    for(len = 0; len < pAgntId->len; len++)
    {
        pInst->oid[len] = pAgntId->id[len];
    }

    pInst->tableType = CL_CLOCKTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpclockTableInstXlator: Translating hmoId to Index for SNMP table clockTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.clockTableInfo
         */
        rc = clCorMoIdServiceSet(hmoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clCorMoIdServiceSet returned error rc : [0x%x]!", rc);
            return rc;
        }

        rc = clCorObjectHandleGet(hmoId, &objHdl);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpfileTableInstXlator: clCorObjectHandleGet returned error rc : [0x%x]!", rc);
            return rc;
        }

        if(clockTableIndexCorAttrIdList[0].attrId != 0 || clockTableIndexCorAttrIdList[0].attrId != -1)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.103.2.1.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, clockTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.clockTableInfo.clockRow);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", clockTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, clockTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.clockTableInfo.clockRow, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmptimeSetTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmptimeSetTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpocTrainDefaultInstXlator Unable to allocate memory for the instance translation!");
            return (CL_ERR_NO_MEMORY);
        }
        *pInstLen = sizeof (ClSNMPRequestInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void**)pInst;
    }
    for(len = 0; len < pAgntId->len; len++)
    {
        pInst->oid[len] = pAgntId->id[len];
    }

    pInst->tableType = CL_TIMESETTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmptimeSetTableInstXlator: Translating hmoId to Index for SNMP table timeSetTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.timeSetTableInfo
         */
        rc = clCorMoIdServiceSet(hmoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clCorMoIdServiceSet returned error rc : [0x%x]!", rc);
            return rc;
        }

        rc = clCorObjectHandleGet(hmoId, &objHdl);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpfileTableInstXlator: clCorObjectHandleGet returned error rc : [0x%x]!", rc);
            return rc;
        }

        if(timeSetTableIndexCorAttrIdList[0].attrId != 0 || timeSetTableIndexCorAttrIdList[0].attrId != -1)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.103.3.1.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, timeSetTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.timeSetTableInfo.timeSetRow);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", timeSetTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, timeSetTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.timeSetTableInfo.timeSetRow, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpnameTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpnameTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpocTrainDefaultInstXlator Unable to allocate memory for the instance translation!");
            return (CL_ERR_NO_MEMORY);
        }
        *pInstLen = sizeof (ClSNMPRequestInfoT);
        memset(pInst, 0, *pInstLen);
        *pRetInstData = (void**)pInst;
    }
    for(len = 0; len < pAgntId->len; len++)
    {
        pInst->oid[len] = pAgntId->id[len];
    }

    pInst->tableType = CL_NAMETABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpnameTableInstXlator: Translating hmoId to Index for SNMP table nameTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.nameTableInfo
         */
        rc = clCorMoIdServiceSet(hmoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clCorMoIdServiceSet returned error rc : [0x%x]!", rc);
            return rc;
        }

        rc = clCorObjectHandleGet(hmoId, &objHdl);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpfileTableInstXlator: clCorObjectHandleGet returned error rc : [0x%x]!", rc);
            return rc;
        }

        if(nameTableIndexCorAttrIdList[0].attrId != 0 || nameTableIndexCorAttrIdList[0].attrId != -1)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.103.4.1.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, nameTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.nameTableInfo.nodeAdd);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", nameTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, nameTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.nameTableInfo.nodeAdd, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        ClNameT moIdStr = {0};
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.smCfgTblAllWrTableInfo
         */
         
        /* Get targetId- CorMOID and fill the instance to the index */
        
        sprintf(moIdStr.value, "%s","\\Chassis:0\\nameTable:-1");
        moIdStr.length = strlen(moIdStr.value);
        clLogDebug("XLT", NULL,
                "Creating MOID[%s] for object creation", moIdStr.value);
        rc = clCorMoIdNameToMoIdGet(&moIdStr, hmoId);
        if(CL_OK != rc)
        {
            clLogError("XLT", NULL,
                    "Error in converting from moid name to moid, rc=[0x%x]", rc);
            return rc;
        }
        rc = clCorMoIdServiceSet(hmoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
