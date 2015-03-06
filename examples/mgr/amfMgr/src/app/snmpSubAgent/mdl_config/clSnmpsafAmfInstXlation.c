
#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>
#include <clCorUtilityApi.h>
#include <clSnmpOp.h>
#include <clSnmpSubAgentsafAmfConfig.h>
#include "clSnmpsafAmfInstXlation.h"
#include <clSnmpsafAmfTables.h>

#define CL_SNMP_GEN_XLA_COTX "GXL"

#define CL_MED_COR_STR_MOID_PRINT(corMoId)          \
do\
{ \
    SaNameT strMoId;                        \
    clCorMoIdToMoIdNameGet(corMoId, &strMoId );       \
    clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "%s", strMoId.value);\
}while(0)                                   \


ClRcT clSnmpsafAmfDefaultInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
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
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAFAMF_SCALARS;
    return CL_OK;
}

/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfApplicationTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfApplicationTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFAPPLICATIONTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfApplicationTableInstXlator: Translating hmoId to Index for SNMP table saAmfApplicationTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfApplicationTableInfo
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

        if(saAmfApplicationTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.1.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfApplicationTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfApplicationTableInfo.saAmfApplicationNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfApplicationTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfApplicationTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfApplicationTableInfo.saAmfApplicationNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfApplicationTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfNodeTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfNodeTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFNODETABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfNodeTableInstXlator: Translating hmoId to Index for SNMP table saAmfNodeTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfNodeTableInfo
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

        if(saAmfNodeTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.2.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfNodeTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfNodeTableInfo.saAmfNodeNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfNodeTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfNodeTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfNodeTableInfo.saAmfNodeNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfNodeTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSGTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSGTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGTableInstXlator: Translating hmoId to Index for SNMP table saAmfSGTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSGTableInfo
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

        if(saAmfSGTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.3.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSGTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSGTableInfo.saAmfSGNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSGTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSGTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSGTableInfo.saAmfSGNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfSGTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSUTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSUTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUTableInstXlator: Translating hmoId to Index for SNMP table saAmfSUTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSUTableInfo
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

        if(saAmfSUTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.4.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSUTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSUTableInfo.saAmfSUNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSUTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSUTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSUTableInfo.saAmfSUNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfSUTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSITableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSITableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSITABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSITableInstXlator: Translating hmoId to Index for SNMP table saAmfSITable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSITableInfo
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

        if(saAmfSITableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.5.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSITableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSITableInfo.saAmfSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSITableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSITableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSITableInfo.saAmfSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfSITableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSUsperSIRankTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUsperSIRankTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSUSPERSIRANKTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUsperSIRankTableInstXlator: Translating hmoId to Index for SNMP table saAmfSUsperSIRankTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSUsperSIRankTableInfo
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

        if(saAmfSUsperSIRankTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.6.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.6.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSUsperSIRankTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSUsperSIRankTableInfo.saAmfSUsperSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSUsperSIRankTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSUsperSIRankTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSUsperSIRankTableInfo.saAmfSUsperSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSUsperSIRankTableInfo.saAmfSUsperSIRank);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSUsperSIRankTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSUsperSIRankTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSUsperSIRankTableInfo.saAmfSUsperSIRank, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfSUsperSIRankTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSGSIRankTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGSIRankTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSGSIRANKTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGSIRankTableInstXlator: Translating hmoId to Index for SNMP table saAmfSGSIRankTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSGSIRankTableInfo
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

        if(saAmfSGSIRankTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.7.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.7.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSGSIRankTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSGSIRankTableInfo.saAmfSGSIRankSGNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSGSIRankTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSGSIRankTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSGSIRankTableInfo.saAmfSGSIRankSGNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSGSIRankTableInfo.saAmfSGSIRank);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSGSIRankTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSGSIRankTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSGSIRankTableInfo.saAmfSGSIRank, &attrSize);
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
  
ClRcT clSnmpsaAmfSGSURankTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGSURankTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSGSURANKTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSGSURankTableInstXlator: Translating hmoId to Index for SNMP table saAmfSGSURankTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSGSURankTableInfo
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

        if(saAmfSGSURankTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.8.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.8.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSGSURankTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSGSURankTableInfo.saAmfSGSURankSGNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSGSURankTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSGSURankTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSGSURankTableInfo.saAmfSGSURankSGNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSGSURankTableInfo.saAmfSGSURank);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSGSURankTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSGSURankTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSGSURankTableInfo.saAmfSGSURank, &attrSize);
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
  
ClRcT clSnmpsaAmfSISIDepTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSISIDepTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSISIDEPTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSISIDepTableInstXlator: Translating hmoId to Index for SNMP table saAmfSISIDepTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSISIDepTableInfo
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

        if(saAmfSISIDepTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.9.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.9.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSISIDepTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSISIDepTableInfo.saAmfSISIDepSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSISIDepTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSISIDepTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSISIDepTableInfo.saAmfSISIDepSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSISIDepTableInfo.saAmfSISIDepDepndSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSISIDepTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSISIDepTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSISIDepTableInfo.saAmfSISIDepDepndSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfSISIDepTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCompTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCompTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCOMPTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCompTableInstXlator: Translating hmoId to Index for SNMP table saAmfCompTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCompTableInfo
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

        if(saAmfCompTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.10.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfCompTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCompTableInfo.saAmfCompNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCompTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCompTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCompTableInfo.saAmfCompNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCompTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCompCSTypeSupportedTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCompCSTypeSupportedTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCOMPCSTYPESUPPORTEDTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCompCSTypeSupportedTableInstXlator: Translating hmoId to Index for SNMP table saAmfCompCSTypeSupportedTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCompCSTypeSupportedTableInfo
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

        if(saAmfCompCSTypeSupportedTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.11.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.11.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfCompCSTypeSupportedTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCompNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCompCSTypeSupportedTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCompCSTypeSupportedTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCompNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCSTypeNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCompCSTypeSupportedTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCompCSTypeSupportedTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCSTypeNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCompCSTypeSupportedTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCSITableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSITableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCSITABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSITableInstXlator: Translating hmoId to Index for SNMP table saAmfCSITable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCSITableInfo
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

        if(saAmfCSITableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.12.1.1",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfCSITableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCSITableInfo.saAmfCSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSITableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSITableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCSITableInfo.saAmfCSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCSITableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCSICSIDepTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSICSIDepTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCSICSIDEPTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSICSIDepTableInstXlator: Translating hmoId to Index for SNMP table saAmfCSICSIDepTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCSICSIDepTableInfo
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

        if(saAmfCSICSIDepTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.13.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.13.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfCSICSIDepTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepCSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSICSIDepTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSICSIDepTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepCSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepDepndCSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSICSIDepTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSICSIDepTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepDepndCSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCSICSIDepTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCSINameValueTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSINameValueTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCSINAMEVALUETABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSINameValueTableInstXlator: Translating hmoId to Index for SNMP table saAmfCSINameValueTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCSINameValueTableInfo
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

        if(saAmfCSINameValueTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.14.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.14.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfCSINameValueTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCSINameValueTableInfo.saAmfCSINameValueCSINameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSINameValueTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSINameValueTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCSINameValueTableInfo.saAmfCSINameValueCSINameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfCSINameValueTableInfo.saAmfCSINameValueAttrNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSINameValueTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSINameValueTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfCSINameValueTableInfo.saAmfCSINameValueAttrNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCSINameValueTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfCSTypeAttrNameTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSTypeAttrNameTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFCSTYPEATTRNAMETABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfCSTypeAttrNameTableInstXlator: Translating hmoId to Index for SNMP table saAmfCSTypeAttrNameTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfCSTypeAttrNameTableInfo
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

        if(saAmfCSTypeAttrNameTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.15.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.15.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfCSTypeAttrNameTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSTypeAttrNameTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSTypeAttrNameTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeAttrNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfCSTypeAttrNameTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfCSTypeAttrNameTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeAttrNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfCSTypeAttrNameTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSUSITableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUSITableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSUSITABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSUSITableInstXlator: Translating hmoId to Index for SNMP table saAmfSUSITable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSUSITableInfo
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

        if(saAmfSUSITableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.16.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.16.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSUSITableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSUSITableInfo.saAmfSUSISuNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSUSITableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSUSITableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSUSITableInfo.saAmfSUSISuNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSUSITableInfo.saAmfSUSISiNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSUSITableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSUSITableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSUSITableInfo.saAmfSUSISiNameId, &attrSize);
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
  
ClRcT clSnmpsaAmfHealthCheckTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfHealthCheckTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFHEALTHCHECKTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfHealthCheckTableInstXlator: Translating hmoId to Index for SNMP table saAmfHealthCheckTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfHealthCheckTableInfo
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

        if(saAmfHealthCheckTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.17.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.17.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfHealthCheckTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfHealthCheckTableInfo.saAmfHealthCompNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfHealthCheckTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfHealthCheckTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfHealthCheckTableInfo.saAmfHealthCompNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfHealthCheckTableInfo.saAmfHealthCheckKey);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfHealthCheckTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfHealthCheckTableIndexCorAttrIdList[1].attrId, -1, pInst->index.saAmfHealthCheckTableInfo.saAmfHealthCheckKey, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    else if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
    {
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Creating Object from North bound interface");
        /* Row Creation/Deletion from management station. */

        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to get hmoId
         * from index value in pInst->index.saAmfHealthCheckTableInfo
         */
    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
/*
 * This function needs to be written for every application MIB table. 
 * This implements the extraction part of the MIB table index. The user 
 * fills the logic for obtaining the index.
 */
  
ClRcT clSnmpsaAmfSCompCsiTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSCompCsiTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFSCOMPCSITABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfSCompCsiTableInstXlator: Translating hmoId to Index for SNMP table saAmfSCompCsiTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfSCompCsiTableInfo
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

        if(saAmfSCompCsiTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.18.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.18.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfSCompCsiTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfSCompCsiTableInfo.saAmfSCompCsiCompNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSCompCsiTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSCompCsiTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfSCompCsiTableInfo.saAmfSCompCsiCompNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfSCompCsiTableInfo.saAmfSCompCsiCsiNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfSCompCsiTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfSCompCsiTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfSCompCsiTableInfo.saAmfSCompCsiCsiNameId, &attrSize);
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
  
ClRcT clSnmpsaAmfProxyProxiedTableInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie)
{
    ClRcT       rc = CL_OK; /* Return code */ 
    ClUint32T   len = 0;
    ClSNMPRequestInfoT *pInst = NULL;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Instant Xlation for MOID : ");
    CL_MED_COR_STR_MOID_PRINT(hmoId);


    if(!pAgntId || !pRetInstData || !pInstLen)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfProxyProxiedTableInstXlator received NULL arguments!");
        return CL_ERR_NULL_POINTER;   
    }

    pInst = (ClSNMPRequestInfoT*)*pRetInstData;

    if (pInst == NULL)
    {
        pInst = (ClSNMPRequestInfoT *) clHeapAllocate (sizeof (ClSNMPRequestInfoT));
        if (pInst == NULL)
        {
            clLog(CL_LOG_CRITICAL, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsafAmfDefaultInstXlator Unable to allocate memory for the instance translation!");
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

    pInst->tableType = CL_SAAMFPROXYPROXIEDTABLE;
    if (CL_MED_CREATION_BY_OI == instXlnOp)
    {
        ClCorObjectHandleT objHdl;
        ClUint32T attrSize = 0;
        clLog(CL_LOG_INFO, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpsaAmfProxyProxiedTableInstXlator: Translating hmoId to Index for SNMP table saAmfProxyProxiedTable");
        /* USER_CODE_MODIFICATION_NEEDS_TO_BE_PERFORMED
         * User should supply the logic to translate hmoId
         * to index value in pInst->index.saAmfProxyProxiedTableInfo
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

        if(saAmfProxyProxiedTableIndexCorAttrIdList[0].attrId <= 0)
        {
            ClUint8T * indexOidList[] = {
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.19.1.1",
                    (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.19.1.2",
                    NULL /* End of Index OID List */
            };
            ClRcT rc = CL_OK;
                rc = clSnmpTableIndexCorAttrIdInit(2, indexOidList, saAmfProxyProxiedTableIndexCorAttrIdList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc : [0x%x]", rc);
            }
        }
        attrSize = sizeof(pInst->index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxyNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfProxyProxiedTableIndexCorAttrIdList[0].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfProxyProxiedTableIndexCorAttrIdList[0].attrId, -1, &pInst->index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxyNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }
        attrSize = sizeof(pInst->index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxiedNameId);
        clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Getting attribute value for AttrId : [%d] Attr Len : [%d]", saAmfProxyProxiedTableIndexCorAttrIdList[1].attrId, attrSize);
        rc = clCorObjectAttributeGet(objHdl, NULL, saAmfProxyProxiedTableIndexCorAttrIdList[1].attrId, -1, &pInst->index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxiedNameId, &attrSize);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_XLA_COTX, "Failed to get attribute information : [0x%x]", rc);
            return rc;
        }

    }
    *pRetInstData = (void**)pInst;
    return (rc);
}
