#include <clCommon.h>

#include <clAmsErrors.h>
#include <clAmsTypes.h>
#include <clAmsEntities.h>
#include <clAmsMgmtOI.h>

#include <clCorApi.h>
#include <clProvApi.h>

#include <clLogApi.h>

#include <clSnmpsafAmfEnums.h>

#include <clCorMetaStruct.h>

ClRcT clAmfNodeSet(ClAmsMgmtHandleT mgmtHandle, const ClProvTxnDataT *pProvTxnData)
{
    ClAmsEntityT entity = {0};
    ClRcT rc = CL_OK;

    if(!mgmtHandle)
        return CL_OK;

    if(!pProvTxnData || !pProvTxnData->pProvData || !pProvTxnData->pMoId)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    entity.type = CL_AMS_ENTITY_TYPE_NODE;
    rc = clAmsMgmtOIGet(pProvTxnData->pMoId, &entity);
    if(rc != CL_OK)
    {
        ClNameT moidName = {0};
        if(clCorMoIdToMoIdNameGet(pProvTxnData->pMoId, &moidName) == CL_OK)
            clLogError("OI", "READ", "AMF entity not found for moid [%.*s]",
                       moidName.length, moidName.value);
        return rc;
    }

    switch (pProvTxnData->attrId)
    {
        case SAAMFNODETABLE_SAAMFNODEADMINSTATETRIGGER:
        {
            ClAmsNodeConfigT *pNodeConfig = NULL;
            ClInt32T adminState = *(ClInt32T *)pProvTxnData->pProvData;

            rc = clAmsMgmtEntityGetConfig(mgmtHandle, &entity, 
                                          (ClAmsEntityConfigT**)&pNodeConfig);
            if (rc != CL_OK)
            {
                clLogError("OI", "WRITE", "Node [%s] config returned [%#x]",
                           entity.name.value, rc);
                return rc;
            }
            if (pProvTxnData->size != (ClUint32T)sizeof(pNodeConfig->adminState))
            {
                clLogError("OI", "WRITE", "Read size [%d] doesnt match expected size [%d]",
                           pProvTxnData->size, (ClUint32T)pNodeConfig->adminState);
                clHeapFree(pNodeConfig);
                return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            }

            switch (adminState)
            {
                case SAAMFNODEADMINSTATETRIGGER_STABLE:
                {
                    return CL_OK;
                }
         
                case SAAMFNODEADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                case SAAMFNODEADMINSTATETRIGGER_LOCK:
                {
                    rc = clAmsMgmtEntityLockAssignment(mgmtHandle, &entity);
                    break;
                }
                case SAAMFNODEADMINSTATETRIGGER_UNLOCK:
                {
                    if(clAmsMgmtEntityLockAssignment(mgmtHandle, &entity) == CL_OK)
                        sleep(1);
                    rc = clAmsMgmtEntityUnlock(mgmtHandle, &entity);
                    break;
                }
                case SAAMFNODEADMINSTATETRIGGER_LOCKINSTANTIATION:
                {
                    /*
                     * Careful. If snmp subagent is running on only 1 node
                     * and is not redundant, then the snmp query would timeout
                     * as it would take down the subagent as well.
                     */
                    if(clAmsMgmtEntityLockAssignment(mgmtHandle, &entity) == CL_OK)
                        sleep(1);
                    rc = clAmsMgmtEntityLockInstantiation(mgmtHandle, &entity);
                    break;
                }
            }
 
            clHeapFree(pNodeConfig);
            break;
        }
        case SAAMFNODETABLE_SAAMFNODEAUTOREPAIROPTION:
        {
            break;
        }
        case SAAMFNODETABLE_SAAMFNODECLMNODE:
        {
            break;
        }
        case SAAMFNODETABLE_SAAMFNODENAME:
        {
            break;
        }
        case SAAMFNODETABLE_SAAMFNODEREPAIR:
        {
            break;
        }
        case SAAMFNODETABLE_SAAMFNODESUFAILOVERPROB:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    return rc;
}

