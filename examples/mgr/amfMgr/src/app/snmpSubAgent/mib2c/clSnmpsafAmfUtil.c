
#include <string.h>

#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>

#include <clAlarmUtils.h>
#include <clSnmpsafAmfInstXlation.h>
#include <clAppSubAgentConfig.h>
#include <clSnmpSubAgentsafAmfConfig.h>
#include <clSnmpsafAmfTables.h>
#include <clSnmpsafAmfUtil.h>


ClRcT clSnmpsaAmfAlarmServiceImpairedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
{
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
        case SAAMFALARMSERVICEIMPAIRED_SAAMFPROBABLECAUSE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.3.1.2 */
            /* Scalar objects do not have any index. */
            pTlvList->numTlvs = 0;
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfAlarmCompInstantiationFailedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFALARMCOMPINSTANTIATIONFAILED_SAAMFCOMPNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.10.1.2 */
            rc = clSnmpsaAmfCompTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfCompTableIndexTlvGet(&(pInst->index.saAmfCompTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfAlarmCompCleanupFailedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFALARMCOMPCLEANUPFAILED_SAAMFCOMPNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.10.1.2 */
            rc = clSnmpsaAmfCompTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfCompTableIndexTlvGet(&(pInst->index.saAmfCompTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfAlarmClusterResetIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFALARMCLUSTERRESET_SAAMFCOMPNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.10.1.2 */
            rc = clSnmpsaAmfCompTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfCompTableIndexTlvGet(&(pInst->index.saAmfCompTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfAlarmSIUnassignedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFALARMSIUNASSIGNED_SAAMFSINAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.5.1.2 */
            rc = clSnmpsaAmfSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSITableIndexTlvGet(&(pInst->index.saAmfSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfAlarmProxiedCompUnproxiedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFALARMPROXIEDCOMPUNPROXIED_SAAMFCOMPNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.10.1.2 */
            rc = clSnmpsaAmfCompTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfCompTableIndexTlvGet(&(pInst->index.saAmfCompTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgClusterAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
{
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
        case SAAMFSTATECHGCLUSTERADMIN_SAAMFCLUSTERADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.1.7 */
            /* Scalar objects do not have any index. */
            pTlvList->numTlvs = 0;
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgApplicationAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGAPPLICATIONADMIN_SAAMFAPPLICATIONNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.1.1.2 */
            rc = clSnmpsaAmfApplicationTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfApplicationTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfApplicationTableIndexTlvGet(&(pInst->index.saAmfApplicationTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfApplicationTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGAPPLICATIONADMIN_SAAMFAPPLICATIONADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.1.1.3 */
            rc = clSnmpsaAmfApplicationTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfApplicationTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfApplicationTableIndexTlvGet(&(pInst->index.saAmfApplicationTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfApplicationTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgNodeAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGNODEADMIN_SAAMFNODENAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.2.1.2 */
            rc = clSnmpsaAmfNodeTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfNodeTableIndexTlvGet(&(pInst->index.saAmfNodeTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGNODEADMIN_SAAMFNODEADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.2.1.6 */
            rc = clSnmpsaAmfNodeTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfNodeTableIndexTlvGet(&(pInst->index.saAmfNodeTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSGAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSGADMIN_SAAMFSGNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.3.1.2 */
            rc = clSnmpsaAmfSGTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSGTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSGTableIndexTlvGet(&(pInst->index.saAmfSGTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSGTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSGADMIN_SAAMFSGADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.3.1.10 */
            rc = clSnmpsaAmfSGTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSGTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSGTableIndexTlvGet(&(pInst->index.saAmfSGTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSGTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSUAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSUADMIN_SAAMFSUNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.2 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSUADMIN_SAAMFSUADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.7 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSIAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSIADMIN_SAAMFSINAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.5.1.2 */
            rc = clSnmpsaAmfSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSITableIndexTlvGet(&(pInst->index.saAmfSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSIADMIN_SAAMFSIADMINSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.5.1.6 */
            rc = clSnmpsaAmfSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSITableIndexTlvGet(&(pInst->index.saAmfSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgNodeOperIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGNODEOPER_SAAMFNODENAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.2.1.2 */
            rc = clSnmpsaAmfNodeTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfNodeTableIndexTlvGet(&(pInst->index.saAmfNodeTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGNODEOPER_SAAMFNODEOPERATIONALSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.2.1.7 */
            rc = clSnmpsaAmfNodeTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfNodeTableIndexTlvGet(&(pInst->index.saAmfNodeTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSUOperIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSUOPER_SAAMFSUNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.2 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSUOPER_SAAMFSUOPERSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.10 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSUPresenceIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSUPRESENCE_SAAMFSUNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.2 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSUPRESENCE_SAAMFSUPRESENCESTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.4.1.11 */
            rc = clSnmpsaAmfSUTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUTableIndexTlvGet(&(pInst->index.saAmfSUTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSUHaStateIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSUHASTATE_SAAMFSUSISUNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.16.1.3 */
            rc = clSnmpsaAmfSUSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUSITableIndexTlvGet(&(pInst->index.saAmfSUSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSUHASTATE_SAAMFSUSISINAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.16.1.4 */
            rc = clSnmpsaAmfSUSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUSITableIndexTlvGet(&(pInst->index.saAmfSUSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSUHASTATE_SAAMFSUSIHASTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.16.1.5 */
            rc = clSnmpsaAmfSUSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSUSITableIndexTlvGet(&(pInst->index.saAmfSUSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgSIAssignmentIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGSIASSIGNMENT_SAAMFSINAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.5.1.2 */
            rc = clSnmpsaAmfSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSITableIndexTlvGet(&(pInst->index.saAmfSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
        case SAAMFSTATECHGSIASSIGNMENT_SAAMFSIASSIGNMENTSTATE:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.5.1.7 */
            rc = clSnmpsaAmfSITableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfSITableIndexTlvGet(&(pInst->index.saAmfSITableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfStateChgUnproxiedCompProxiedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList )
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
        case SAAMFSTATECHGUNPROXIEDCOMPPROXIED_SAAMFCOMPNAME:
        {
            /* OID = .1.3.6.1.4.1.18568.2.2.2.1.2.10.1.2 */
            rc = clSnmpsaAmfCompTableInstXlator ( &oid, pMoId, NULL, (void **)&pInst, &instLen, NULL, CL_MED_CREATION_BY_OI, NULL);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableInstXlator returned : [0x%x]", rc);
            }
            rc = clSnmpsaAmfCompTableIndexTlvGet(&(pInst->index.saAmfCompTableInfo), pTlvList);
            if(rc != CL_OK)
            {
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet returned : [0x%x]", rc);
                return rc;
            }
            break;
        }
    }
    return rc;
}

ClRcT clSnmpsaAmfApplicationTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfApplicationTableIndexInfoT * psaAmfApplicationTableIndex;

    psaAmfApplicationTableIndex = (ClSnmpsaAmfApplicationTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfApplicationTableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfApplicationTableIndex->saAmfApplicationNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfNodeTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfNodeTableIndexInfoT * psaAmfNodeTableIndex;

    psaAmfNodeTableIndex = (ClSnmpsaAmfNodeTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfNodeTableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfNodeTableIndex->saAmfNodeNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSGTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSGTableIndexInfoT * psaAmfSGTableIndex;

    psaAmfSGTableIndex = (ClSnmpsaAmfSGTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSGTableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfSGTableIndex->saAmfSGNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSUTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSUTableIndexInfoT * psaAmfSUTableIndex;

    psaAmfSUTableIndex = (ClSnmpsaAmfSUTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUTableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfSUTableIndex->saAmfSUNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSITableIndexInfoT * psaAmfSITableIndex;

    psaAmfSITableIndex = (ClSnmpsaAmfSITableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSITableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfSITableIndex->saAmfSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSUsperSIRankTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSUsperSIRankTableIndexInfoT * psaAmfSUsperSIRankTableIndex;

    psaAmfSUsperSIRankTableIndex = (ClSnmpsaAmfSUsperSIRankTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUsperSIRankTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSUsperSIRankTableIndex->saAmfSUsperSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSUsperSIRankTableIndex->saAmfSUsperSIRank), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSISIDepTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSISIDepTableIndexInfoT * psaAmfSISIDepTableIndex;

    psaAmfSISIDepTableIndex = (ClSnmpsaAmfSISIDepTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSISIDepTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSISIDepTableIndex->saAmfSISIDepSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSISIDepTableIndex->saAmfSISIDepDepndSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCompTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCompTableIndexInfoT * psaAmfCompTableIndex;

    psaAmfCompTableIndex = (ClSnmpsaAmfCompTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompTableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfCompTableIndex->saAmfCompNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCompCSTypeSupportedTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCompCSTypeSupportedTableIndexInfoT * psaAmfCompCSTypeSupportedTableIndex;

    psaAmfCompCSTypeSupportedTableIndex = (ClSnmpsaAmfCompCSTypeSupportedTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCompCSTypeSupportedTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCompCSTypeSupportedTableIndex->saAmfCompCSTypeSupportedCompNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCompCSTypeSupportedTableIndex->saAmfCompCSTypeSupportedCSTypeNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCSITableIndexInfoT * psaAmfCSITableIndex;

    psaAmfCSITableIndex = (ClSnmpsaAmfCSITableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (1 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCSITableIndexTlvGet unable to allocate memory!");
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
    memcpy(pTlv->value, &(psaAmfCSITableIndex->saAmfCSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCSICSIDepTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCSICSIDepTableIndexInfoT * psaAmfCSICSIDepTableIndex;

    psaAmfCSICSIDepTableIndex = (ClSnmpsaAmfCSICSIDepTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCSICSIDepTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSICSIDepTableIndex->saAmfCSICSIDepCSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSICSIDepTableIndex->saAmfCSICSIDepDepndCSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCSINameValueTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCSINameValueTableIndexInfoT * psaAmfCSINameValueTableIndex;

    psaAmfCSINameValueTableIndex = (ClSnmpsaAmfCSINameValueTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCSINameValueTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSINameValueTableIndex->saAmfCSINameValueCSINameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSINameValueTableIndex->saAmfCSINameValueAttrNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfCSTypeAttrNameTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfCSTypeAttrNameTableIndexInfoT * psaAmfCSTypeAttrNameTableIndex;

    psaAmfCSTypeAttrNameTableIndex = (ClSnmpsaAmfCSTypeAttrNameTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfCSTypeAttrNameTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSTypeAttrNameTableIndex->saAmfCSTypeNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfCSTypeAttrNameTableIndex->saAmfCSTypeAttrNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfSUSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfSUSITableIndexInfoT * psaAmfSUSITableIndex;

    psaAmfSUSITableIndex = (ClSnmpsaAmfSUSITableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfSUSITableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSUSITableIndex->saAmfSUSISuNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfSUSITableIndex->saAmfSUSISiNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    return CL_OK;
}

ClRcT clSnmpsaAmfHealthCheckTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList)
{
    ClAlarmUtilTlvT * pTlv = NULL;
    ClSnmpsaAmfHealthCheckTableIndexInfoT * psaAmfHealthCheckTableIndex;

    psaAmfHealthCheckTableIndex = (ClSnmpsaAmfHealthCheckTableIndexInfoT *)pIndexInfo;

    if(!pIndexInfo || !pTlvList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL arguments received!"));
        return CL_ERR_NULL_POINTER;
    }
    pTlvList->pTlv = (ClAlarmUtilTlvT *) clHeapAllocate (2 * sizeof (ClAlarmUtilTlvT));
    if (pTlvList->pTlv == NULL)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "saAmfHealthCheckTableIndexTlvGet unable to allocate memory!");
        return (CL_ERR_NO_MEMORY);
    }
    pTlvList->numTlvs = 2;

    pTlv = pTlvList->pTlv + 0;
    pTlv->value = clHeapAllocate (sizeof (ClUint32T));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        return (CL_ERR_NO_MEMORY);
    }
    memcpy(pTlv->value, &(psaAmfHealthCheckTableIndex->saAmfHealthCompNameId), sizeof (ClUint32T));
    pTlv->length = sizeof (ClUint32T);
    pTlv->type = CL_COR_UINT32;
    pTlv = pTlvList->pTlv + 1;
    pTlv->length = strlen(psaAmfHealthCheckTableIndex->saAmfHealthCheckKey) + 1;
    pTlv->value = clHeapAllocate (pTlv->length * sizeof (ClCharT));
    if (pTlv->value == NULL)
    {
        clHeapFree(pTlvList->pTlv);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("saAmfHealthCheckTableIndexTlvGet unable to allocate memory!\n"));
        return (CL_ERR_NO_MEMORY);
    }
    strcpy((ClCharT *)pTlv->value, psaAmfHealthCheckTableIndex->saAmfHealthCheckKey);
    pTlv->type = CL_COR_INT8;
    return CL_OK;
}

ClRcT clSnmpsaAmfApplicationTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfApplicationTableIndexTlvGet(&(pReqInfo->index.saAmfApplicationTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfApplicationTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfApplicationTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfNodeTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfNodeTableIndexTlvGet(&(pReqInfo->index.saAmfNodeTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfNodeTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfNodeTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfSGTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfSGTableIndexTlvGet(&(pReqInfo->index.saAmfSGTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfSGTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfSGTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfSUTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfSUTableIndexTlvGet(&(pReqInfo->index.saAmfSUTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfSUTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfSUTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfSITableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfSITableIndexTlvGet(&(pReqInfo->index.saAmfSITableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfSITableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfSITableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfSUsperSIRankTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfSUsperSIRankTableIndexTlvGet(&(pReqInfo->index.saAmfSUsperSIRankTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfSUsperSIRankTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfSUsperSIRankTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfSISIDepTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfSISIDepTableIndexTlvGet(&(pReqInfo->index.saAmfSISIDepTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfSISIDepTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfSISIDepTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCompTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCompTableIndexTlvGet(&(pReqInfo->index.saAmfCompTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfCompTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCompTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCompCSTypeSupportedTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCompCSTypeSupportedTableIndexTlvGet(&(pReqInfo->index.saAmfCompCSTypeSupportedTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfCompCSTypeSupportedTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCompCSTypeSupportedTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCSITableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCSITableIndexTlvGet(&(pReqInfo->index.saAmfCSITableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
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
     
            pAttrDesc->pAttrPath = saAmfCSITableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCSITableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCSICSIDepTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCSICSIDepTableIndexTlvGet(&(pReqInfo->index.saAmfCSICSIDepTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfCSICSIDepTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCSICSIDepTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCSINameValueTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCSINameValueTableIndexTlvGet(&(pReqInfo->index.saAmfCSINameValueTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfCSINameValueTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCSINameValueTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfCSTypeAttrNameTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfCSTypeAttrNameTableIndexTlvGet(&(pReqInfo->index.saAmfCSTypeAttrNameTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfCSTypeAttrNameTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfCSTypeAttrNameTableIndexCorAttrIdList[count].attrId;
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

ClRcT clSnmpsaAmfHealthCheckTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList)
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

    rc = clSnmpsaAmfHealthCheckTableIndexTlvGet(&(pReqInfo->index.saAmfHealthCheckTableInfo), &indexTlvList);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableIndexTlvGet returned error rc : [0x%x]", (ClUint32T)rc);
        return rc;
    }

    pAttrDescList->numOfDescriptor = 2;
    pAttrDescList->pAttrDescriptor = (ClCorAttrValueDescriptorT *) 
                                             clHeapAllocate(2 * sizeof(ClCorAttrValueDescriptorT)); 

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
     
            pAttrDesc->pAttrPath = saAmfHealthCheckTableIndexCorAttrIdList[count].pAttrPath;
            pAttrDesc->attrId = saAmfHealthCheckTableIndexCorAttrIdList[count].attrId;
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


