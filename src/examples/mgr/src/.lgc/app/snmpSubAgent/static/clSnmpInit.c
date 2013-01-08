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
 * ModuleName  : SystemTest
 * File        : clSnmpInit.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>
#include <clAlarmDefinitions.h>
#include <clAlarmUtils.h>
#include <clAppSubAgentConfig.h>
#include <clSnmpOp.h>


ClCntHandleT gClSnmpOidContainerHandle;

/*****************************************************************************/
/* ASP error id to SNMP error id mapping table                              */
/*****************************************************************************/
ClSnmpOidErrMapTableItemT oidErrMapTable[] = {
    { CL_SNMP_ANY_MODULE, CL_OK, CL_SNMP_ERR_NOERROR },
    { CL_SNMP_ANY_MODULE, CL_ERR_NO_MEMORY, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_PARAMETER, CL_SNMP_ERR_NOSUCHNAME },
    { CL_SNMP_ANY_MODULE, CL_ERR_NULL_POINTER, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_NOT_EXIST, CL_SNMP_ERR_NOSUCHNAME },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_HANDLE, CL_SNMP_ERR_INCONSISTENTVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_INVALID_BUFFER, CL_SNMP_ERR_INCONSISTENTVALUE }, 
    { CL_SNMP_ANY_MODULE, CL_ERR_NOT_IMPLEMENTED, CL_SNMP_ERR_NOCREATION },
    { CL_SNMP_ANY_MODULE, CL_ERR_DUPLICATE, CL_SNMP_ERR_BADVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_OUT_OF_RANGE, CL_SNMP_ERR_WRONGVALUE },
    { CL_SNMP_ANY_MODULE, CL_ERR_NO_RESOURCE, CL_SNMP_ERR_RESOURCEUNAVAILABLE },
    { CL_SNMP_ANY_MODULE, CL_ERR_INITIALIZED, CL_SNMP_ERR_NOACCESS },
    { CL_SNMP_ANY_MODULE, CL_ERR_BAD_OPERATION, CL_SNMP_ERR_GENERR },
    { CL_SNMP_ANY_MODULE, CL_COR_ERR_INVALID_SIZE, CL_SNMP_ERR_WRONGLENGTH },
    { CL_SNMP_ANY_MODULE, CL_COR_ERR_OI_NOT_REGISTERED, CL_SNMP_ERR_NOACCESS},
    { CL_SNMP_ANY_MODULE, CL_SNMP_ANY_OTHER_ERROR, CL_SNMP_ERR_GENERR }
};

#define CL_MAX_SNMP_ERR    (sizeof(oidErrMapTable)/sizeof(ClSnmpOidErrMapTableItemT))

ClEoExecutionObjT* gSnmpEOObj;

static ClRcT clSnmpOidAdd(ClSnmpOidInfoT *pTable);
static ClInt32T clSnmpOidCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void clSnmpOidNodeDelete(ClCntKeyHandleT key, ClCntDataHandleT pData);

static ClInt32T  snmpInstCompare (ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static ClRcT snmpInstXlator (const struct ClMedAgentId *pAgntId,
        ClCorMOIdPtrT         hmoId,
        ClCorAttrPathPtrT containedPath,
        void**        pRetInstData,
        ClUint32T     *instLen,
        ClPtrT        pCorAttrValueDescList,
        ClMedInstXlationOpEnumT     instXlnOp,
        ClPtrT cookie);

static ClRcT clSnmpAppNotifyHandler(CL_IN ClCharT* trapOid, CL_IN ClCharT* pData, ClUint32T dataLen, CL_IN ClCharT* trapDes);
static ClRcT clSnmpIndexToOidGet(CL_IN ClAlarmUtilTlvInfoPtrT pIndexTlvInfo, CL_OUT ClUint32T *pObjOid);
static ClRcT clSnmpNotifyHandler (ClCorMOIdPtrT moId_h,         /**< Handle to ClCorMOId */
        ClUint32T probableCause,             /**< Alarm Probable Cause  */
        ClAlarmSpecificProblemT specificProblem, /** Alarm Specific Problem */
        ClCharT *pData,               /**< Optional data */
        ClUint32T dataLen);            /**< Data length*/

static ClRcT clSnmpTrapPayloadGet(CL_IN ClAlarmUtilTlvPtrT pTlv,
        CL_IN ClUint32T oidLen,
        CL_IN ClUint32T *pObjOid, 
        CL_OUT ClTrapPayLoadListT *pTrapPayloadList);

ClRcT initMMSnmpOpCodeTranslations ()
{
    ClMedAgntOpCodeT agntOpCode;
    ClMedTgtOpCodeT  tgtOpCode;
    ClInt32T i = 0;
    ClRcT retVal = CL_OK;
    ClSnmpOpCodeT snmpOpCodeList[] = { CL_SNMP_SET, 
        CL_SNMP_GET, 
        CL_SNMP_GET_NEXT,
        CL_SNMP_CREATE, 
        CL_SNMP_DELETE, 
        CL_SNMP_WALK,
        CL_SNMP_GET_FIRST,
        CL_SNMP_GET_SVCS,
        CL_SNMP_CONTAINMENT_SET
    };
    ClMedCorOpEnumT corOpCodeList[] = { CL_COR_SET, 
        CL_COR_GET, 
        CL_COR_GET_NEXT,
        CL_COR_CREATE, 
        CL_COR_DELETE, 
        CL_COR_WALK,
        CL_COR_GET_FIRST,
        CL_COR_GET_SVCS,
        CL_COR_CONTAINMENT_SET
    };
    tgtOpCode.type = CL_MED_XLN_TYPE_COR;
    for (i = 0; i < CL_SNMP_MAX - 1; i++)
    {
        agntOpCode.opCode = snmpOpCodeList[i];
        tgtOpCode.opCode = corOpCodeList[i];
        retVal = clMedOperationCodeTranslationAdd (gMedSnmpHdl, agntOpCode, &tgtOpCode, 1);
        if (retVal != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("init_MMSnmpOpCodeTranslations MM OP code translation addition failed\n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_SNMP_LOG_1_OPCODE_INSERT_FAILED, retVal);
        }
    }
    return (retVal);
}

ClRcT initMMSnmpIdTranslations ()
{
    ClRcT retVal = CL_OK;
    ClMedTgtIdT corIdentifier;
    int count = 0;

    /* add the ID translations */
    for (count = 0; (clOidMapTable[count].agntId.id) != NULL; count++)
    {
        memset(&corIdentifier, 0, sizeof(ClMedTgtIdT));
        corIdentifier.type = CL_MED_XLN_TYPE_COR;
        clOidMapTable[count].agntId.len = (ClUint32T)strlen((ClCharT*)clOidMapTable[count].agntId.id) + 1;
        corIdentifier.info.corId.moId = &clOidMapTable[count].moId;
        corIdentifier.info.corId.attrId = clOidMapTable[count].corAttrId;
        retVal = clMedIdentifierTranslationInsert (gMedSnmpHdl, clOidMapTable[count].agntId, 
                &corIdentifier, 1);
        if (retVal != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("init_MMSnmpIdTranslations MM ID translation addition failed\n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_SNMP_LOG_1_ID_XLN_INSERT_FAILED, retVal);
        }
    }

    for (count = 0; (clOidMapTable[count].agntId.id) != NULL; count++)
    {
        memset(&corIdentifier, 0, sizeof(ClMedTgtIdT));
        corIdentifier.type = CL_MED_XLN_TYPE_COR;
        clOidMapTable[count].agntId.len = (ClUint32T)strlen((ClCharT*)clOidMapTable[count].agntId.id) + 1;
        corIdentifier.info.corId.moId = &clOidMapTable[count].moId;
        corIdentifier.info.corId.attrId = clOidMapTable[count].corAttrId;
        retVal = clMedInstanceTranslationInsert (gMedSnmpHdl, clOidMapTable[count].agntId, 
                &corIdentifier, 1);
        if (retVal != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("init_MMSnmpIdTranslations MM ID instance translation addition failed\n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_SNMP_LOG_1_ID_XLN_INSERT_FAILED, retVal);
        }
    }

    return (retVal);
}

ClRcT initMMSnmpErrCodeTranslations()
{
    ClRcT retVal = CL_OK;
    ClUint32T i = 0;

    for (i = 0; i < CL_MAX_SNMP_ERR; i++)
    {
        retVal = clMedErrorIdentifierTranslationInsert (gMedSnmpHdl, 
                oidErrMapTable[i].sysErrorType,
                oidErrMapTable[i].sysErrorId, 
                oidErrMapTable[i].agntErrorId);
    }
    if (retVal != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("init_MMSnmpErrCodeTranslations  MM errorcode translation addition failed\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_SNMP_LOG_1_ERROR_ID_XLN_INSERT_FAILED, retVal);
    }
    return (retVal);
}

ClRcT initMM (void)
{
    ClRcT retCode = CL_OK;
    ClUint32T count = 0;

    clEoMyEoIocPortSet (CL_IOC_SNMP_PORT);
    clEoMyEoObjectSet (gSnmpEOObj);
    if( ( retCode = clCntLlistCreate(clSnmpOidCompare, 
                    clSnmpOidNodeDelete, 
                    clSnmpOidNodeDelete, 
                    CL_CNT_UNIQUE_KEY,
                    &gClSnmpOidContainerHandle) ) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not create container Lib for oidContainer. %d", retCode));
        exit(1);
    }
    while(appOidInfo[count].oid != NULL)
    {
        clSnmpOidAdd(&appOidInfo[count]);
        count++;
    }
    count = 0;

    if ((retCode = clMedInitialize (&gMedSnmpHdl, clSnmpNotifyHandler, snmpInstXlator, 
                    snmpInstCompare)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("initMM Unable to initialize the   mediation component for CLI!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, "INT", CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "mediation"); 
        return retCode;
    }
    if ((retCode = initMMSnmpOpCodeTranslations ()) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("initMM Unable to add the opcode  translations in MM!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, "INT", CL_SNMP_LOG_1_OPCODE_INSERT_FAILED, retCode);
        return retCode;
    }
    if ((retCode = initMMSnmpIdTranslations ()) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("initMM Unable to add the Id        translations in MM!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, "INT", CL_SNMP_LOG_1_ID_XLN_INSERT_FAILED, retCode);
        return retCode;
    }
    if ((retCode = initMMSnmpErrCodeTranslations ()) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("initMM Unable to add the error  code translations in MM!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, "INT", CL_SNMP_LOG_1_ERROR_ID_XLN_INSERT_FAILED, retCode);
        return retCode;
    }
    return CL_OK;
}

ClRcT clSnmpNotifyHandler (ClCorMOIdPtrT pMoId,        /**< Handle to ClCorMOId */
        ClUint32T probableCause,      /**< AttrId  */
        ClAlarmSpecificProblemT specificProblem,
        ClCharT *pData,               /**< Optional data */
        ClUint32T dataLen)            /**< Data length*/
{
    ClRcT rc = CL_OK;
    ClCharT pTrapOid[CL_SNMP_MAX_STR_LEN];
    ClCharT pTrapDes[CL_SNMP_MAX_STR_LEN];

    if (pMoId == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("NULL pointer to ClCorMOId!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        return (CL_ERR_NULL_POINTER);
    }
    memset(pTrapOid, 0, CL_SNMP_MAX_STR_LEN);
    memset(pTrapDes, 0, CL_SNMP_MAX_STR_LEN);

    if (clSnmpAlarmIdToOidGet(pMoId, probableCause, specificProblem, pTrapOid, pTrapDes) == CL_OK)
    {
        rc = clSnmpAppNotifyHandler(pTrapOid, pData, dataLen, pTrapDes);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clSnmpAppNotifyHandler failed. rc = [0x%x]", rc));
            return rc;
        }
    }
    else
    {
        clLogError("SNP", "INT", "Traps not configured");
    }
    return rc;
}

ClRcT snmpInstXlator (const struct ClMedAgentId *pAgntId,
        ClCorMOIdPtrT         hmoId,
        ClCorAttrPathPtrT containedPath,
        void**         pRetInstData,
        ClUint32T     *instLen,
        ClPtrT        pCorAttrValueDescList,
        ClMedInstXlationOpEnumT     instXlnOp,
        ClPtrT cookie)
{
    ClRcT          rc = CL_OK; /* Return code */ 
    ClUint32T      found = 0;
    ClUint32T      tableId = 0;
    ClCntNodeHandleT nodeHandle;
    ClSnmpOidInfoT* pOidInfo=NULL;

    if (hmoId == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("snmpInstXlator NULL pointer to ClCorMOId!\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, "INT", CL_LOG_MESSAGE_0_NULL_ARGUMENT);
        return (CL_ERR_NULL_POINTER);
    }
    tableId = 0;
    if(CL_OK == (rc = clCntNodeFind(gClSnmpOidContainerHandle, (ClCntKeyHandleT)pAgntId->id, &nodeHandle) ) )
    {
        found = 1;
        if(CL_OK != (rc = clCntNodeUserDataGet(gClSnmpOidContainerHandle,
                        nodeHandle, (ClCntDataHandleT*)&pOidInfo) ) )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not find node for oid %s. RC = 0x%x",(ClCharT *) pAgntId->id, rc) );
            return rc;
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntNodeFind returned error rc = 0x%x", (ClUint32T)rc));
        return rc;
    }

    if(found == 1)
    {
        rc = pOidInfo->fpInstXlator(pAgntId,
                hmoId,
                containedPath,
                pRetInstData,
                instLen,
                NULL,
                instXlnOp,
                NULL);

        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("InstXlator returned error! rc = 0x%x", rc));
            return rc;
        }

        if(instXlnOp == CL_MED_CREATION_BY_NORTHBOUND)
        {
            if(pOidInfo->fpTblIndexCorAttrGet == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("NULL callback to get COR attributes for table indices!"));
                return CL_ERR_NULL_POINTER;
            }
            rc = pOidInfo->fpTblIndexCorAttrGet(*pRetInstData, pCorAttrValueDescList);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("TblIndexCorAttrGet returned error! rc = 0x%x", rc));
                return rc;
            }
        }
    }
    return (rc);
}

ClInt32T  snmpInstCompare (ClCntKeyHandleT key1,
        ClCntKeyHandleT key2)
{
    ClSNMPRequestInfoT* reqInfo1 = NULL;
    ClSNMPRequestInfoT* reqInfo2 = NULL;
    ClRcT           rc = CL_OK;
    ClUint32T       retVal = 0;
    ClSnmpOidInfoT *pOidInfo = NULL;
    ClCntNodeHandleT nodeHandle;

    if (key1 != 0) reqInfo1 = (ClSNMPRequestInfoT*)key1;
    if (key2 != 0) reqInfo2 = (ClSNMPRequestInfoT*)key2;

    if ((reqInfo1 == NULL) && (reqInfo2 != NULL))
    {
        return (-1);
    }
    else if ((reqInfo1 != NULL) && (reqInfo2 == NULL))
    {
        return (1);
    }
    else if ((reqInfo1 == NULL) && (reqInfo2 == NULL))
    {
        return (0);
    }

    if(CL_OK != (rc = clCntNodeFind(gClSnmpOidContainerHandle,
                    (ClCntKeyHandleT)reqInfo1->oid, &nodeHandle) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not find node for oid %s. RC = 0x%x",(ClCharT *) reqInfo1->oid, rc) );
        return rc;
    }
    if(CL_OK != (rc = clCntNodeUserDataGet(gClSnmpOidContainerHandle,
                    nodeHandle, (ClCntDataHandleT*)&pOidInfo) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not get data for oid %s. RC = 0x%x",(ClCharT *) reqInfo2->oid, rc) );
        return rc;
    }
    retVal = pOidInfo->fpInstCompare(key1, key2);
    return retVal;
}

ClRcT clSnmpAlarmIdToOidGet(ClCorMOIdPtrT pMoId, ClUint32T probableCause, ClAlarmSpecificProblemT specificProblem, 
        ClCharT* pTrapOid, ClCharT* pDes)
{
    ClInt32T retVal = 0;
    ClSnmpTrapOidMapTableItemT* pItem = clTrapOidMapTable;

    while(pItem->agntId.id != NULL)
    {
        if ((pItem->probableCause == probableCause) && (pItem->specificProblem == specificProblem))
        {
            retVal = clCorMoIdCompare(&pItem->moId, pMoId);
            if(retVal == 0 || retVal == 1)
            {
                strcpy(pTrapOid,(ClCharT*) pItem->agntId.id);
                if(NULL != pItem->pTrapDes)
                    strcpy(pDes, (ClCharT*)pItem->pTrapDes);
                return CL_OK;
            }
        }
        pItem++;
    }

    return CL_ERR_NOT_EXIST;
}

ClInt32T clSnmpOidToAlarmIdGet(ClCharT* pTrapOid, ClUint32T *pProbableCause, 
                    ClAlarmSpecificProblemT* pSpecificProblem, ClCorMOIdPtrT pMoId)
{
    ClSnmpTrapOidMapTableItemT* pItem = clTrapOidMapTable;

    if (!pTrapOid || !pProbableCause || !pSpecificProblem || !pMoId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("%s(): Invalid Parameters!\r\n", __FUNCTION__));
        return CL_ERR_INVALID_PARAMETER;
    }

    while(pItem->agntId.id != NULL)
    {
        if (strcmp((ClCharT *)pItem->agntId.id, pTrapOid) == 0)
        {
            *pProbableCause = pItem->probableCause;
            *pSpecificProblem = pItem->specificProblem;
            memcpy(pMoId, &pItem->moId, sizeof(ClCorMOIdT) );
            return(CL_OK);
        }
        pItem++;
    }

    return(CL_ERR_NOT_EXIST);
}

ClInt32T clSnmpOidCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClCharT *oid1 = NULL;
    ClCharT *oid2 = NULL;
    ClInt32T result = 0;

    if (key1 != 0) oid1 = (ClCharT *)key1;
    if (key2 != 0) oid2 = (ClCharT *)key2;

    /* Check the opcode first */
    result = strncmp(oid1, oid2, strlen(oid1) );
    if(result == 0)
    {
        if(oid2[strlen(oid1)] != '\0' && oid2[strlen(oid1)] != '.')
            result = 1;
    }
    return result;
}

void clSnmpOidNodeDelete(ClCntKeyHandleT key, ClCntDataHandleT pData)
{
    /*Currently, we don't need anything here*/
    return;
}

ClRcT clSnmpOidAdd(ClSnmpOidInfoT *pTable)
{
    ClRcT rc = CL_OK;
    if(CL_OK != (rc = clCntNodeAdd(gClSnmpOidContainerHandle, 
                    (ClCntKeyHandleT)pTable->oid,
                    (ClCntDataHandleT)pTable,
                    NULL) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not add data in oid list") );
    }
    return rc;
}

/*
 * This function takes the trapOid and payload for trap and converts the payload in the 
 * format which can directly be consumed by mib2c generated trap function.
 * parameters:
 * pTrapOid (IN) : oid of the trap
 * pData   (IN) : AlarmInfoHandle structure passed as part of alarm raise API.
 * dataLen (IN) : length of the pData.
 * pTrapDes (IN) : any additional information to be passed with the trap.
 */
ClRcT clSnmpAppNotifyHandler(CL_IN ClCharT* pTrapOid, CL_IN ClCharT* pData, ClUint32T dataLen, CL_IN ClCharT* pTrapDes)
{
    ClRcT     rc = CL_OK;
    ClUint32T count = 0;
    ClUint32T index = 0;
    ClUint32T oidLen = 0;
    ClUint32T tlvCount = 0;
    ClUint32T indexCount = 0;
    ClUint32T allocatedOidLen = 0;
    ClUint32T *pObjOid = NULL;
    ClUint32T numIndexEntryInTable = 0;
    ClAlarmHandleInfoT* pAlarmHandleInfo = NULL;
    ClAlarmUtilTlvInfoT indexTlvInfo = {0};
    ClAlarmUtilPayLoadListT *pAlarmPayLoadList = NULL;
    ClTrapPayLoadListT   trapPayloadList = {0};

    if(NULL == pTrapOid)
    {
        CL_DEBUG_PRINT( CL_DEBUG_ERROR,("pTrapOid is passed as NULL.") );
        return CL_ERR_NULL_POINTER;
    }
    /*Find out which trap is required for this event notification.*/
    while(clSnmpAppCallback[index].pTrapOid != NULL)
    {
        if(strcmp(clSnmpAppCallback[index].pTrapOid, pTrapOid ) == 0)
        {
            break;
        }
        index++;
    }
    if(clSnmpAppCallback[index].pTrapOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("No callback registered for the alarm for trapOid %s.\n",pTrapOid) );
        return CL_ERR_NOT_EXIST;
    }

    clLogDebug("SNP", "NTF", "Data length size [%d]", dataLen);
    if(dataLen > 0)
    {
        pAlarmHandleInfo = (ClAlarmHandleInfoT*) pData;
        clLogDebug("SNP", "NTF", "Buffer length size [%d]", pAlarmHandleInfo->alarmInfo.len);
        if(pAlarmHandleInfo->alarmInfo.len > 0)
        {
            rc = clAlarmUtilPayLoadExtract( (ClUint8T *)pAlarmHandleInfo->alarmInfo.buff, 
                    pAlarmHandleInfo->alarmInfo.len, &pAlarmPayLoadList);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not extract alarm payload. rc = [0x%x]", rc) );
                return rc;
            }
            if(NULL == pAlarmPayLoadList)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("pAlarmPayLoadList is returned as NULL."));
                return CL_ERR_NULL_POINTER;
            }

            /*Calculate total number of object entries in the trap notification.*/ 
            for(count = 0; count < pAlarmPayLoadList->numPayLoadEnteries; count++)
            {
                numIndexEntryInTable += pAlarmPayLoadList->pPayload[count].numTlvs;
            }
            trapPayloadList.pPayLoad = (ClTrapPayLoadT *)clHeapAllocate(numIndexEntryInTable * sizeof(ClTrapPayLoadT) );
            if(NULL == trapPayloadList.pPayLoad)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory of size %zd",numIndexEntryInTable * sizeof(ClTrapPayLoadT)) );
                return CL_ERR_NULL_POINTER;
            }

            /*Go through all the alarm payload entries in alarmPayLoadList*/
            for(count = 0; count < pAlarmPayLoadList->numPayLoadEnteries; count++)
            {
                /*For each alarm payload entry go through all the tlvs*/
                for(indexCount = 0; indexCount < pAlarmPayLoadList->pPayload[count].numTlvs; indexCount++)
                {
                    ClUint32T numTlvInIndex = 0;
                    oidLen = 0;

                    /*
                     * This is kept inside the loop to consider the case where same moId can map to multiple table.
                     * In this case for same moID, we will have different indices. 
                     */
                    /*Convert the moId information to index and then fill the indexTlvInfo structure*/
                    rc = clSnmpAppCallback[index].fpAppNotifyIndexGet(tlvCount, 
                            pAlarmPayLoadList->pPayload[count].pMoId, &indexTlvInfo);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT( CL_DEBUG_ERROR, ("moId to index get failed. rc = [0x%x]",rc) );
                        return rc;
                    }

                    /*Calculate space required for index oid*/
                    for (numTlvInIndex = 0; numTlvInIndex < indexTlvInfo.numTlvs; numTlvInIndex++)
                    {
                        if(CL_COR_INT8 == indexTlvInfo.pTlv[numTlvInIndex].type || 
                                CL_COR_UINT8 == indexTlvInfo.pTlv[numTlvInIndex].type)
                        {
                            oidLen += (indexTlvInfo.pTlv[numTlvInIndex].length * 4);
                            if(indexTlvInfo.pTlv[numTlvInIndex].length > 1)
                                oidLen += 4;
                        }
                        else if(CL_COR_INT32 == indexTlvInfo.pTlv[numTlvInIndex].type || 
                                CL_COR_UINT32 == indexTlvInfo.pTlv[numTlvInIndex].type)
                        {
                            oidLen += indexTlvInfo.pTlv[numTlvInIndex].length;
                            if(indexTlvInfo.pTlv[numTlvInIndex].length > 4)
                                oidLen += 4;
                        }
                    }
                    if(oidLen > allocatedOidLen)
                    {
                        pObjOid = (ClUint32T *)clHeapRealloc(pObjOid, oidLen);
                        if(NULL == pObjOid)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory of size %d\n",oidLen) );
                            return CL_ERR_NO_MEMORY;
                        }
                        allocatedOidLen = oidLen;
                    }
                    rc = clSnmpIndexToOidGet(&indexTlvInfo, pObjOid);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not get oid format for tlv. rc = [0x%x]",rc));
                        return rc;
                    }
                    /*Form a structure of type trapPayLoad from this*/
                    rc = clSnmpTrapPayloadGet((pAlarmPayLoadList->pPayload[count].pTlv + indexCount), oidLen, pObjOid, &trapPayloadList);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not get trap payload from trap oid rc = [0x%x]",rc));
                        return rc;
                    }
                    /*Sachin : this has internal pointers so need to freed carefully.*/
                    {
                        ClUint32T tlvCount = 0;
                        for(tlvCount =0; tlvCount < indexTlvInfo.numTlvs; tlvCount++)
                        {
                            clHeapFree(indexTlvInfo.pTlv[tlvCount].value);
                        }
                        clHeapFree(indexTlvInfo.pTlv);
                    }
                    tlvCount++;
                }
            }
            clSnmpAppCallback[index].fpAppNotify(&trapPayloadList);
            if(NULL != pObjOid)
            {
                clHeapFree(pObjOid);
                pObjOid = NULL;
            }
            for(count = 0; count < trapPayloadList.numTrapPayloadEntry; count++)
            {
                if(NULL != trapPayloadList.pPayLoad[count].objOid)
                {
                    clHeapFree(trapPayloadList.pPayLoad[count].objOid);
                    trapPayloadList.pPayLoad[count].objOid = NULL;
                }
            }
            if(NULL != trapPayloadList.pPayLoad)
            {
                clHeapFree(trapPayloadList.pPayLoad);
                trapPayloadList.pPayLoad = NULL;
            }
            clAlarmUtilPayloadListFree(pAlarmPayLoadList);
        }
        else
        {
            clSnmpAppCallback[index].fpAppNotify(NULL);
        }
    }
    else
    {
        clLogError("SNP", "NTF", "Error in data length, size [%d]", dataLen);
        return CL_ERR_INVALID_PARAMETER;
    }
    return CL_OK;
}

/*
 * This function takes the index information in tlv format and converts it into oid format.
 * parameters:
 * pIndexTlvInfo : Indices in form of tlv. These correspond to objects in notification.
 * pObjOid       : Indices converted into OID format.
 */
ClRcT clSnmpIndexToOidGet(CL_IN ClAlarmUtilTlvInfoPtrT pIndexTlvInfo, CL_OUT ClUint32T *pObjOid)
{
    ClUint32T count = 0;
    ClUint32T numIndexTlv = 0;

    if(NULL == pIndexTlvInfo)
    {
        CL_DEBUG_PRINT( CL_DEBUG_ERROR,("index TLV is passed as NULL") );
        return CL_ERR_NULL_POINTER;
    }
    if(pIndexTlvInfo->numTlvs > 0 && NULL == pObjOid)
    {
        CL_DEBUG_PRINT( CL_DEBUG_ERROR,("pObjOid is passed as NULL") );
        return CL_ERR_NULL_POINTER;
    }

    for(numIndexTlv= 0; numIndexTlv < pIndexTlvInfo->numTlvs; numIndexTlv++)
    {
        switch(pIndexTlvInfo->pTlv[numIndexTlv].type)
        {
            case CL_COR_INT8:
            case CL_COR_UINT8:
                if(pIndexTlvInfo->pTlv[numIndexTlv].length == 1)
                {
                    *pObjOid++ = (ClUint32T )(*(ClUint8T *)pIndexTlvInfo->pTlv[numIndexTlv].value);
                }
                else
                {
                    *pObjOid++ = pIndexTlvInfo->pTlv[numIndexTlv].length;
                    for(count = 0; count < pIndexTlvInfo->pTlv[numIndexTlv].length; count++)
                    {
                        *pObjOid++ = 
                            (ClUint32T)(*(ClUint8T *)((ClUint8T *)(pIndexTlvInfo->pTlv[numIndexTlv].value) + count) );
                    }
                }
                break;
            case CL_COR_INT32:
            case CL_COR_UINT32:
                if(pIndexTlvInfo->pTlv[numIndexTlv].length == 4)
                {
                    *pObjOid++ = *(ClUint32T *)pIndexTlvInfo->pTlv[numIndexTlv].value;
                }
                else
                {
                    *pObjOid++ =  pIndexTlvInfo->pTlv[numIndexTlv].length / 4;
                    memcpy(pObjOid, pIndexTlvInfo->pTlv[numIndexTlv].value, pIndexTlvInfo->pTlv[numIndexTlv].length);
                }
                break;
            default:
                break;
        }
    }
    return CL_OK;
}
/*
 * This function takes the tlv value and object oids and convert them in the form of pTrapPayloadList
 * strcuture. This structure is used in the callback function where trap is raised.
 * parameters:
 * pTlv             : pointer to the tlv structure
 * oidLen           : length of the index oid
 * pObjOid          : pointer to index oid.
 * pTrapPayloadList : trap payload having value and index oid info.
 */
ClRcT clSnmpTrapPayloadGet(CL_IN ClAlarmUtilTlvPtrT pTlv,
        CL_IN ClUint32T oidLen,
        CL_IN ClUint32T *pObjOid,
        CL_OUT ClTrapPayLoadListT *pTrapPayloadList)
{
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].oidLen = oidLen;
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid =
        (ClUint32T *)clHeapAllocate(oidLen);
    if(NULL == pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory of size %d",oidLen));
        return CL_ERR_NULL_POINTER;
    }
    memcpy(pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].objOid, pObjOid, oidLen);
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry].valLen = pTlv->length;
    pTrapPayloadList->pPayLoad[pTrapPayloadList->numTrapPayloadEntry++].val    = pTlv->value;
    return CL_OK;
}

/*
 * Function to show the container information for a given oid.
 */
ClInt32T dumpOidInfo()
{
    ClSnmpOidInfoT *pOidInfo = NULL;
    ClCntNodeHandleT     currentNode = 0;
    ClCntNodeHandleT     nextNode = 0;
    ClInt32T rc;

    if (clCntFirstNodeGet(gClSnmpOidContainerHandle, &currentNode) != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
    }
    if (CL_OK != (rc = clCntNodeUserDataGet(gClSnmpOidContainerHandle,
                    currentNode, (ClCntDataHandleT*)&pOidInfo)))
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
        return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
    }
    while(clCntNextNodeGet(gClSnmpOidContainerHandle, currentNode, &nextNode) == CL_OK)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(gClSnmpOidContainerHandle,
                        nextNode,
                        (ClCntDataHandleT*)&pOidInfo)))
        {
            CL_FUNC_EXIT();
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Instance translation  found "));
            return CL_MED_SET_RC(CL_MED_ERR_NO_INSTANCE);
        }
        currentNode = nextNode;
        nextNode = 0;
    }
    return CL_OK;    
}

