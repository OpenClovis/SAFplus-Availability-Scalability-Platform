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
/*******************************************************************************
 * ModuleName  : SystemTest
 * File        : clSnmpOp.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clCorUtilityApi.h>
#include <clBitApi.h>

#include <clSnmpDefs.h>
#include <clSnmpOp.h>
#include <clSnmpSubagentClient.h>

/********************************
* Global variables
********************************/
ClSnmpOperT gOper  = 
{ 
    NULL, /* Mutex */
    0, /* refCount */
    CL_FALSE, /*committed */
    {
       0, /* opCode */
       0, /* varCount */
       NULL, /* varInfo */
    },
    {/* Err list */
        0,  /* count */
        NULL, /* pErrList */
    },
    NULL
};
ClSnmpReqInfoT gReqInfo;
static ClBoolT gErrorHappened = CL_FALSE;
extern ClSnmpSubagentInfoT gSubAgentInfo;

/********************************
* Global functions definition
********************************/

ClRcT clSnmpSendSyncRequestToServer (void **my_loop_context, 
        void **my_data_context, 
        netsnmp_variable_list *put_index_data, 
        ClUint32T tableType,
        ClSnmpOpCodeT opCode)
{
    netsnmp_variable_list *vptr = NULL;
    ClInt32T errorCode = 0;
    ClInt32T retVal = 0;
    ClSnmpReqInfoT reqInfo = {0};
    ClUint32T count = 0;
    ClUint32T tblIndex = 0;
    ClCharT *sendData = NULL;
    ClRcT rc = CL_OK;
    ClSnmpOidInfoT *appOidInfo = NULL;

    /*Set my_loop_context to an application specific iterator context.
     * Typically it will be a linked list of values where each node
     * corresponds to a row in the table. But here we are not maintaining any
     * linked list.*/
    /*Actually we can set my_loop_context to NULL pointer also as we are not
     * maintaining a linked list of rows in a table here. All the required
     * things are maintained in COR. And net-SNMP uses the index we set by
     * calling snmp_set_var_value() function for the next index. So just set
     * it to &reqInfo here*/
    *my_loop_context = &gReqInfo;
    *my_data_context = &gReqInfo;

    memset(&reqInfo, 0, sizeof(ClSnmpReqInfoT));

    vptr = put_index_data;

    sendData = (ClCharT*) &reqInfo.index;


    extern ClRcT snmpBuildAppOIDInfo(ClSnmpOidInfoT **);
    rc = snmpBuildAppOIDInfo(&appOidInfo);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    /* Get table entry from appOidInfo[] */
    while (appOidInfo[tblIndex].numAttr != 0)
    {
        if (appOidInfo[tblIndex].tableType == tableType)
        {
            break;
        }
        tblIndex++;
    }

    clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Finding Index Information for OID[%s] with SNMP GET_FIRST[%d]/GET-NEXT[%d] CURRENT[%d]", appOidInfo[tblIndex].oid, CL_SNMP_GET_FIRST, CL_SNMP_GET_NEXT, opCode );

    if (appOidInfo[tblIndex].numAttr == 0)
    {
        /* Table entry not found after entries being exhausted */
        retVal = CL_MED_SET_RC(CL_ERR_NOT_EXIST);
        clLog(CL_LOG_SEV_ERROR, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "appOidInfo don't have necessary index information");
        return retVal;
    }

    if((opCode ==  CL_SNMP_GET_NEXT) || (my_loop_context !=  NULL && (*(ClInt32T*)*my_loop_context)!= 1))
    {
        for(count =0; (count < appOidInfo[tblIndex].numAttr) && sendData; count++)
        {
            /*We are assuming that the ClSnmpReqInfoT structure is
             * properly completed by the application writer and the union part
             * contains all indices*/

            /*Copy the index got from snmp agent*/
            /*COR supports only Integer / array of character types (string).
             * So we are not handling the case for float/bitstring/double etc
             * which are part of the put_index_data->val union*/
            switch(appOidInfo[tblIndex].attr[count].type)
            {
                case CL_SNMP_STRING_ATTR:
                    clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Index Info Input : Possion[%d] AttrType[STRING] Value [%s] Size[%d]",count, vptr->val.string, appOidInfo[tblIndex].attr[count].size);
                    memcpy(sendData, vptr->val.string, (vptr->val_len + 1));
                    /*Add 1 here so that when
                     * vptr->val_len is added after the
                     * switch() it makes up for the
                     * length copied above*/
                    sendData += 1;     
                    break;
                case CL_SNMP_NON_STRING_ATTR:
                    clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Index Info Input : Possion[%d] AttrType[NON-STRING] Value[%d] Size[%d]",count, *(ClUint32T*)vptr->val.integer, appOidInfo[tblIndex].attr[count].size);
                    memcpy(sendData, vptr->val.integer, appOidInfo[tblIndex].attr[count].size);
                    break;
                default:
                    break;
            }
            sendData += appOidInfo[tblIndex].attr[count].size;
            vptr = vptr->next_variable;
        }
    }

    {

        ClUint32T oidLen = strlen(appOidInfo[tblIndex].oid) + 1;
        if( oidLen > sizeof reqInfo.oid)
        {
            return CL_ERR_NO_SPACE;
        }
        else
            strncpy(reqInfo.oid, appOidInfo[tblIndex].oid, oidLen);
    }


    reqInfo.oidLen = strlen(reqInfo.oid);
    reqInfo.opCode = opCode;
    reqInfo.tableType = tableType;
    reqInfo.dataLen = sizeof(reqInfo.index);

    clLogInfo("SNMP", "SUB", "Before calling execute SNMP, oid is [%s]", reqInfo.oid);

    /*Now call clExecuteSnmp which talks to mediation library and fills the
     * index information for the next data or first data based on request
     * opcode*/
    retVal = clExecuteSnmp( &reqInfo,
            &reqInfo.index,
            NULL,
            &errorCode);
    vptr = put_index_data;
    /*Now set the result in vptr*/
    switch (retVal)
    {
        case CL_OK:   /* No errors : successful operation */
            {
                clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Successfully got index information for OID[%s]",appOidInfo[tblIndex].oid);
                put_index_data->val.string = NULL;
                memset(vptr->buf, '\0', sizeof(vptr->buf));

                sendData = (ClCharT*) &reqInfo.index;
                for(count =0; (count < appOidInfo[tblIndex].numAttr) && sendData; count++)
                {
                    switch(appOidInfo[tblIndex].attr[count].type)
                    {
                        case CL_SNMP_STRING_ATTR:
                            clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Index Info Output : Possion[%d] AttrType[STRING] Value[%s] Size[%d] ",count, sendData, appOidInfo[tblIndex].attr[count].size);
                            snmp_set_var_value(vptr, (u_char *) sendData, strlen(sendData) + 1);
                            sendData += 1;
                            break;
                        case CL_SNMP_NON_STRING_ATTR:
                            clLog(CL_LOG_SEV_DEBUG, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "Index Info Output : Possion[%d] AttrType[NON-STRING] Value[%d] Size[%d]",count, *(ClUint32T*)sendData, appOidInfo[tblIndex].attr[count].size);
                            snmp_set_var_value(vptr, (u_char *)sendData, appOidInfo[tblIndex].attr[count].size);
                            break;
                        default:
                            clLog(CL_LOG_SEV_ERROR, CL_SNMP_AREA, CL_SNMP_CTX_SOP, "AttrType is not Supported");
                    }
                    sendData += appOidInfo[tblIndex].attr[count].size;
                    vptr = vptr->next_variable;
                }
                break;
            }
        default:                 /* Failed due to other reasons */
            clLogDebug("SNP", "OPE", "Failed while getting next entry. rc[0x%x]", retVal);
            return retVal;
    }
    return CL_OK;
}

ClRcT clExecuteSnmp (ClSnmpReqInfoT* reqInfo,
        void* data,
        ClInt32T *pOpNum,
        ClInt32T *pErrCode)
{
    ClRcT rc = 0;

    if(reqInfo->opCode == CL_SNMP_GET_FIRST ||
            reqInfo->opCode == CL_SNMP_GET_NEXT)
    {
        rc = clGetTable(reqInfo, data, pErrCode);
    }
    else if(reqInfo->opCode == CL_SNMP_GET) 
    {
        rc = clGetTableAttr(reqInfo, data, pErrCode);
    }
    else
    {
    //    rc = clSetTableAttr(reqInfo, data, pOpNum, pErrCode);
        clLogError("SNP","OPE",
                "Invalid opcode[%d]", reqInfo->opCode);
    }
    return rc;
}

ClRcT clRequestAdd (ClSnmpReqInfoT* reqInfo,
        void* data,
        ClInt32T *pOpNum,
        ClInt32T *pErrCode,
        ClPtrT   pReqAddInfo)
{
    ClRcT rc = 0;

    switch (reqInfo->opCode)
    {
        case CL_SNMP_GET :
            rc = clSnmpJobTableAttrAdd(reqInfo, data, pOpNum, pErrCode, pReqAddInfo);
            break;
        case CL_SNMP_SET :
        case CL_SNMP_CREATE :
        case CL_SNMP_DELETE:
            rc = clSnmpJobTableAttrAdd(reqInfo, data, pOpNum, pErrCode, NULL);
            break;
        default:
            clLogError("SNP","OPE", "Invalid request type.");
    }
    
    return rc;
}
ClUint32T clGetTable ( ClSnmpReqInfoT* reqInfo,
        void* data, 
        ClInt32T* pErrCode)
{
    ClMedOpT        opInfo;
    ClMedVarBindT   *tempVarInfo = NULL;
    ClInt32T        errorCode = CL_OK;
    ClInt32T        retVal = 0;
    ClCharT         oid[CL_SNMP_DISPLAY_STRING_SIZE];

    opInfo.varCount = 1;
    tempVarInfo =
        (ClMedVarBindT *) clHeapCalloc(1,opInfo.varCount * sizeof (ClMedVarBindT));   
    if (tempVarInfo == NULL)
    {
        clLogError("SNM","OPE", "Failed while allocating the varbind.");
        return (CL_RC(CL_CID_SNMP, CL_ERR_NO_MEMORY));
    }
    retVal = 0;
    strcpy(oid, reqInfo->oid);
    /*oid received till this point is that of the table. Add .1.1 to 
      get oid of the first entry in the table
     */
    strcat(oid,".1.1");

    opInfo.varInfo = tempVarInfo;
    opInfo.varInfo[0].pVal = data;
    opInfo.varInfo[0].errId = 0;
    opInfo.varInfo[0].pInst = reqInfo;
    opInfo.opCode = reqInfo->opCode;
    opInfo.varInfo[0].attrId.id = (ClUint8T*)oid;
    opInfo.varInfo[0].attrId.len = (ClUint32T)strlen(oid) + 1;
    opInfo.varInfo[0].len = reqInfo->dataLen;

    errorCode = clMedOperationExecute (gSubAgentInfo.medHdl, &opInfo);

    if ((opInfo.varInfo[0].errId == 0) && (errorCode == 0))
    {
        clHeapFree (tempVarInfo);
        return (CL_OK);
    }
    else
    {
        if (errorCode != CL_OK)
            *pErrCode = errorCode;
        else
            *pErrCode = opInfo.varInfo[0].errId;

        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                   "clMedOperationExecute returned error. error code = %x, error id = %d",
                   errorCode, opInfo.varInfo[0].errId);
    }
    clHeapFree (tempVarInfo);
    return (errorCode);
}

ClUint32T clGetTableAttr ( ClSnmpReqInfoT* reqInfo,
        void *data, 
        ClInt32T *pErrCode)
{
    ClMedOpT opInfo;

    ClMedVarBindT *tempVarInfo = NULL;
    ClInt32T errorCode = CL_OK;

    opInfo.varCount = 1;
    tempVarInfo =
        (ClMedVarBindT *) clHeapCalloc(1,opInfo.varCount * sizeof (ClMedVarBindT));
    if (tempVarInfo == NULL)
    {
        clLogError("SNM","OPE", "Failed while allocating the varbind.");
        return (CL_RC(CL_CID_SNMP, CL_ERR_NO_MEMORY));
    }

    opInfo.varInfo = tempVarInfo;
    opInfo.varInfo[0].errId = 0;
    opInfo.varInfo[0].pInst = reqInfo;
    opInfo.opCode = reqInfo->opCode;
    opInfo.varInfo[0].pVal = data;
    opInfo.varInfo[0].len = reqInfo->dataLen;
    opInfo.varInfo[0].attrId.id = (ClUint8T*)reqInfo->oid;
    opInfo.varInfo[0].attrId.len = (ClUint32T)strlen(reqInfo->oid) + 1;
    opInfo.varInfo[0].len = reqInfo->dataLen;

    errorCode = clMedOperationExecute (gSubAgentInfo.medHdl, &opInfo);

    *pErrCode = opInfo.varInfo[0].errId;
    reqInfo->dataLen = opInfo.varInfo[0].len;

    if ((opInfo.varInfo[0].errId == 0) && (errorCode == 0))
    {
        clHeapFree(tempVarInfo);
        return (CL_OK);    
    }
    else
    {
        clHeapFree(tempVarInfo);
        return (errorCode);
    }
}
/*
ClUint32T clSetTableAttr(  ClSnmpReqInfoT* reqInfo,
        void* data,
        ClInt32T *pOpNum,
        ClInt32T *pErrCode)
{
    ClMedOpT opInfo;
    static ClMedVarBindT *tempVarInfo = NULL;
    static ClInt32T arrIndex = 0;
    ClInt32T count = 0;
    ClInt32T errorCode = CL_OK;

    if(NULL == tempVarInfo)
    {
        tempVarInfo = (ClMedVarBindT *) clHeapCalloc(1,*pOpNum * sizeof (ClMedVarBindT));
        if(NULL == tempVarInfo)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory. Bulk set for %d objects", *pOpNum) );
            return CL_SNMP_ERR_NOCREATION;
        }
    }
    opInfo.varInfo = tempVarInfo;
    if(arrIndex < *pOpNum)
    {
        opInfo.varInfo[arrIndex].errId = CL_OK;
        opInfo.varInfo[arrIndex].attrId.len = (ClUint32T)strlen(reqInfo->oid) + 1;
        opInfo.varInfo[arrIndex].attrId.id = (ClUint8T *)clHeapCalloc(1,opInfo.varInfo[arrIndex].attrId.len);
        if(NULL == opInfo.varInfo[arrIndex].attrId.id)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory. Size asked for %d", opInfo.varInfo[arrIndex].attrId.len) );
            return CL_SNMP_ERR_NOCREATION;
        }
        memcpy(opInfo.varInfo[arrIndex].attrId.id, reqInfo->oid, opInfo.varInfo[arrIndex].attrId.len);
        opInfo.varInfo[arrIndex].len = reqInfo->dataLen;
        opInfo.varInfo[arrIndex].pVal = clHeapCalloc(1,reqInfo->dataLen);
        if(NULL == opInfo.varInfo[arrIndex].pVal)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory. Size asked for %d",reqInfo->dataLen) );
            return CL_SNMP_ERR_NOCREATION;
        }
        memcpy(opInfo.varInfo[arrIndex].pVal, data, reqInfo->dataLen);
        opInfo.varInfo[arrIndex].pInst = (ClSnmpReqInfoT *)clHeapCalloc(1,sizeof(ClSnmpReqInfoT) );
        if(NULL == opInfo.varInfo[arrIndex].pInst)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not allocate memory. Size asked for %u", (ClUint32T)sizeof(ClSnmpReqInfoT) ) );
            return CL_SNMP_ERR_NOCREATION;
        }
        memcpy(opInfo.varInfo[arrIndex].pInst, reqInfo, sizeof(ClSnmpReqInfoT) );

        if(arrIndex == (*pOpNum - 1) )
        {
            opInfo.opCode = reqInfo->opCode;
            opInfo.varCount = *pOpNum;
            errorCode = clMedOperationExecute (gSubAgentInfo.medHdl, &opInfo);
            if(CL_OK != errorCode)
            {
                for(count = 0; count <= arrIndex; count++)
                {
                    if(opInfo.varInfo[count].errId != CL_OK)
                    {
                        *pErrCode = opInfo.varInfo[count].errId;
                        *pOpNum = count;
                        break;
                    }
                }

            }
            arrIndex = 0;
            for(arrIndex = 0; arrIndex < *pOpNum; arrIndex++)
            {
                clHeapFree(opInfo.varInfo[arrIndex].pVal);
                clHeapFree(opInfo.varInfo[arrIndex].pInst);
            }
            arrIndex = 0;
            clHeapFree (tempVarInfo);
            tempVarInfo = NULL;
            return errorCode;
        }
        arrIndex++;
        return CL_OK;
    }
    return errorCode;
}
*/
ClUint32T clSnmpJobTableAttrAdd(  
        ClSnmpReqInfoT* reqInfo,
        void* data,
        ClInt32T *pOpNum,
        ClInt32T *pErrCode, 
        ClPtrT pReqAddInfo)
{
    ClInt32T arrIndex = 0;
    ClRcT    rc = CL_OK;

    clSnmpMutexLock(gOper.mtx);
    
    gOper.opInfo.varInfo = (ClMedVarBindT *) clHeapRealloc(gOper.opInfo.varInfo, 
    (gOper.opInfo.varCount+1) * sizeof (ClMedVarBindT));
    if(NULL == gOper.opInfo.varInfo)
    {
        clLogError("SNP","OPE", 
                "Could not allocate memory. Bulk set for [%d] objects", *pOpNum) ;
        clSnmpMutexUnlock(gOper.mtx);
        return CL_SNMP_ERR_NOCREATION;
    }
    arrIndex = gOper.opInfo.varCount;
    clLogDebug("SNP","OPE",
            "Adding request[%d] into store", arrIndex);
    gOper.opInfo.varInfo[arrIndex].errId = CL_OK;
    gOper.opInfo.varInfo[arrIndex].attrId.len = (ClUint32T)strlen(reqInfo->oid) + 1;
    gOper.opInfo.varInfo[arrIndex].attrId.id = (ClUint8T *)clHeapCalloc(1, gOper.opInfo.varInfo[arrIndex].attrId.len);
    if(NULL == gOper.opInfo.varInfo[arrIndex].attrId.id)
    {
        clLogError("SNP","OPE", 
                "Could not allocate memory. Size asked for [%d]", 
                gOper.opInfo.varInfo[arrIndex].attrId.len);
        clSnmpMutexUnlock(gOper.mtx);
        return CL_SNMP_ERR_NOCREATION;
    }
    memcpy(gOper.opInfo.varInfo[arrIndex].attrId.id, reqInfo->oid, gOper.opInfo.varInfo[arrIndex].attrId.len);
    gOper.opInfo.varInfo[arrIndex].len = reqInfo->dataLen;
    gOper.opInfo.varInfo[arrIndex].pVal = clHeapAllocate(reqInfo->dataLen);
    if(NULL == gOper.opInfo.varInfo[arrIndex].pVal)
    {
        clLogError("SNP","OPE", 
                "Could not allocate memory. Size asked for [%d]",
                reqInfo->dataLen);
        clSnmpMutexUnlock(gOper.mtx);
        return CL_SNMP_ERR_NOCREATION;
    }
    memset(gOper.opInfo.varInfo[arrIndex].pVal, 0, reqInfo->dataLen);
    if (data)
    {
        memcpy(gOper.opInfo.varInfo[arrIndex].pVal, data, reqInfo->dataLen);
    }
    gOper.opInfo.varInfo[arrIndex].pInst = (ClSnmpReqInfoT *)clHeapCalloc(1,sizeof(ClSnmpReqInfoT) );
    if(NULL == gOper.opInfo.varInfo[arrIndex].pInst)
    {
        clLogError("SNP","OPE", 
                "Could not allocate memory. Size asked for [%u]", 
                (ClUint32T)sizeof(ClSnmpReqInfoT) );
        clSnmpMutexUnlock(gOper.mtx);
        return CL_SNMP_ERR_NOCREATION;
    }
    memcpy(gOper.opInfo.varInfo[arrIndex].pInst, reqInfo, sizeof(ClSnmpReqInfoT) );

    gOper.opInfo.opCode = reqInfo->opCode; /* This could be a problem if a SET and a CREATE comes */

    /* Validate here */
    clLogDebug("SNP","OPE",
            "Validating instance, tableType [%d],oid [%s], oidLen[%d], opCode [%d]",
            reqInfo->tableType, reqInfo->oid, reqInfo->oidLen, reqInfo->opCode); 
    if(CL_SNMP_CREATE != reqInfo->opCode) /* Dont validate instance for CREATE as it wont be there */
    {
        rc = clMedInstValidate(gSubAgentInfo.medHdl, &gOper.opInfo.varInfo[arrIndex]);
        if(CL_OK != rc)
        {
            clLogError("SNM","OPE",
                    "Failed to validate instance information, rc=[0x%x], errorCode=[0x%x]",
                    rc, gOper.opInfo.varInfo[arrIndex].errId);
            *pErrCode = gOper.opInfo.varInfo[arrIndex].errId;
            gErrorHappened = CL_TRUE;
        }
        
        if (reqInfo->opCode == CL_SNMP_GET)
        {
            clLogDebug("SNM","OPE", "Adding the get job.");
            gOper.pOpAddData = (_ClSnmpGetReqInfoT *) clHeapRealloc (gOper.pOpAddData, 
                                        (arrIndex + 1)* sizeof(_ClSnmpGetReqInfoT));
            if (NULL == gOper.pOpAddData)
            {
                clLogError("SNP","OPE",
                        "Could not allocate memory for the get info object. ");
                clSnmpMutexUnlock(gOper.mtx);
                return CL_SNMP_ERR_NOCREATION;
            }

            (((_ClSnmpGetReqInfoT *)gOper.pOpAddData) + arrIndex)->pRequest = 
                                ((_ClSnmpGetReqInfoT *)pReqAddInfo)->pRequest;
            (((_ClSnmpGetReqInfoT *)gOper.pOpAddData) + arrIndex)->columnType = 
                                ((_ClSnmpGetReqInfoT *)pReqAddInfo)->columnType;
        }
    }

    arrIndex++; /* Should this be incremented when inst validation failed? */
    gOper.opInfo.varCount = arrIndex;
    gOper.cmtd = CL_FALSE; /* Not committed */
    gOper.refCount = arrIndex; /* Ref count to delete this global structure */
    clLogDebug("SNP","OPE",
            "No of operations in store [%d]", gOper.opInfo.varCount);

    clSnmpMutexUnlock(gOper.mtx);
    return rc;
}

ClRcT clSnmpCommit(
        ClMedErrorListT *pMedErrList)
{
    ClMedOpT    opInfo = {0};
    ClRcT       errorCode = CL_OK;
    ClUint32T   count = 0;

    clSnmpMutexLock(gOper.mtx); /* Might as well check if the operation is complete before locking */
    if(!gOper.cmtd) /* CL_FALSE means not committed */ 
    {
        opInfo = gOper.opInfo;
        gOper.cmtd = CL_TRUE; /* Indicates that the request is executed */

        clLogDebug("SNP","OPE",
                "Calling med to execute operation, no of ops[%d]", opInfo.varCount);
        errorCode = clMedOperationExecute (gSubAgentInfo.medHdl, &opInfo);
        if(CL_OK != errorCode)
        {
            clLogError("SNM","OPE",
                    "Failed to execute operation, rc=[0x%x]", errorCode);
            for(count = 0; count < opInfo.varCount; count++)
            {
                if(opInfo.varInfo[count].errId != CL_OK)
                {
                    clLogError("SNM","OPE",
                            "Setting errorCode [0x%x] to OID [%s]", 
                            opInfo.varInfo[count].errId, 
                            opInfo.varInfo[count].attrId.id);
                    /*pErrCode = opInfo.varInfo[count].errId;*/

                    /* Construct error list here */ 
                    gOper.medErrList.pErrorList = (ClMedErrorIdT *)clHeapRealloc(gOper.medErrList.pErrorList, 
                            (gOper.medErrList.count + 1)*sizeof(ClMedErrorIdT));
                    if(!gOper.medErrList.pErrorList)
                    {
                        errorCode = CL_ERR_NO_MEMORY;
                        goto exitOnError;
                    }

                    gOper.medErrList.pErrorList[gOper.medErrList.count].errId = opInfo.varInfo[count].errId;

                    gOper.medErrList.pErrorList[gOper.medErrList.count].oidInfo.id = 
                        (ClUint8T*)clHeapAllocate(opInfo.varInfo[count].attrId.len);
                    if(!gOper.medErrList.pErrorList[gOper.medErrList.count].oidInfo.id)
                    {
                        errorCode = CL_ERR_NO_MEMORY;
                        goto exitOnError;   
                    }

                    memcpy(gOper.medErrList.pErrorList[gOper.medErrList.count].oidInfo.id, 
                        opInfo.varInfo[count].attrId.id, opInfo.varInfo[count].attrId.len);
                    gOper.medErrList.pErrorList[gOper.medErrList.count].oidInfo.len = opInfo.varInfo[count].attrId.len;
                    gOper.medErrList.count++;
                }
            }
            *pMedErrList = gOper.medErrList; /* Copy the error list, this is freed at clSnmpUndo() */

        }
#if 0
        for(arrIndex = 0; arrIndex < opInfo.varCount; arrIndex++)
        {
            clHeapFree(opInfo.varInfo[arrIndex].pVal);
            clHeapFree(opInfo.varInfo[arrIndex].pInst);
        }
        gOper.opInfo.varCount = 0; 
        clHeapFree (gOper.opInfo.varInfo);
        gOper.opInfo.varInfo = NULL; /* Next operation would see this and return */
#endif
    }
    else if (gOper.medErrList.count)
    {
        clLogDebug("SNM","OPE", 
                "Already committed, return error list");
        *pMedErrList = gOper.medErrList; /* Copy the error list, this is freed at clSnmpUndo() */
        errorCode = CL_SNMP_ERR_GENERR; /* Return a general error, this is not set to the varbin var */

    }
exitOnError:
    clSnmpMutexUnlock(gOper.mtx);
    return errorCode; /* Return error code */ 
}
void clSnmpUndo(void)
{
    ClUint32T arrIndex = 0;
    clSnmpMutexLock(gOper.mtx);

    gOper.refCount--;
    if(!gOper.refCount)
    {
        clLogDebug("SNM","OPE",
                "Ref count 0 ,removing store");
        /* Remove the error list */
        if(gOper.medErrList.count)
        {
            clLogDebug("SNM","OPE",
                    "ErrList count[%d], removing errlist", gOper.medErrList.count);
            for(arrIndex = 0; arrIndex < gOper.medErrList.count; arrIndex++)
            {
                clHeapFree(gOper.medErrList.pErrorList[arrIndex].oidInfo.id);
            }
            gOper.medErrList.count = 0;
            clHeapFree(gOper.medErrList.pErrorList);
            gOper.medErrList.pErrorList = NULL;
        }
        /* Remove the var bind information */
        if(gOper.opInfo.varCount)
        {
            clLogDebug("SNM","OPE",
                    "VarBind count[%d], removing varbinds", gOper.opInfo.varCount);
            for(arrIndex = 0; arrIndex < gOper.opInfo.varCount; arrIndex++)
            {
                clHeapFree(gOper.opInfo.varInfo[arrIndex].pVal);
                clHeapFree(gOper.opInfo.varInfo[arrIndex].pInst);
				clHeapFree(gOper.opInfo.varInfo[arrIndex].attrId.id);
            }
            gOper.opInfo.varCount = 0; 
            clHeapFree (gOper.opInfo.varInfo);
            gOper.opInfo.varInfo = NULL; /* Next operation would see this and return */
            if (gOper.pOpAddData != NULL)
            {
                clHeapFree(gOper.pOpAddData);
                gOper.pOpAddData = NULL;
            }
            gOper.cmtd = CL_FALSE;
        }
    }

    clSnmpMutexUnlock(gOper.mtx);
}

static void clSnmpDataReset(void)
{
    clLogDebug("SNP","OPE", "Reseting the static data structure.");
    gOper.refCount = 1;
    clSnmpUndo();
}

void clSnmpOidCpy(ClSnmpReqInfoT * pReqInfo, ClWordT * pOid)
{
    ClCharT tempOid[CL_SNMP_MAX_OID_LEN];
    ClInt32T i = 0;

    if(!pReqInfo || !pOid)
    {
        return;
    }

    for(i = 0; i < pReqInfo->oidLen; i++)
    {
        sprintf(tempOid, "%lu", pOid[i]);
        strcat(pReqInfo->oid, tempOid);
        strcat(pReqInfo->oid, ".");
    }

    /* Strip off the last '.' */
    pReqInfo->oidLen = strlen(pReqInfo->oid) - 1;
    pReqInfo->oid[pReqInfo->oidLen] = '\0';
}

ClRcT clSnmpTableIndexCorAttrIdInit(ClUint32T numOfIndices, ClUint8T **pIndexOidList,
        ClSnmpTableIndexCorAttrIdInfoT * pTableIndexCorAttrIdList)
{
    ClUint32T index = 0;
    ClRcT rc = CL_OK;

    if(pIndexOidList == NULL || pTableIndexCorAttrIdList == NULL)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"NULL arguments received!");
        return CL_ERR_NULL_POINTER;
    }

    while((index < numOfIndices) &&  (pIndexOidList[index] != NULL) && 
            (pTableIndexCorAttrIdList[index].attrId != -1))
    {
        ClMedAgentIdT indexId = { pIndexOidList[index], 
            strlen((ClCharT *)pIndexOidList[index]) + 1};
        ClMedTgtIdT *pTgtId = NULL;

        rc = clMedTgtIdGet(gSubAgentInfo.medHdl, &indexId, &pTgtId);
        if(rc != CL_OK || pTgtId == NULL)
        {
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                       "clMedTgtIdGet returned error rc = 0x%x", (ClUint32T)rc);
            return rc;
        }

        pTableIndexCorAttrIdList[index].attrId = pTgtId->info.corId.attrId[0];

        if(pTgtId->info.corId.attrId[0] == CL_MED_ATTR_VALUE_END ||
                pTgtId->info.corId.attrId[1] == CL_MED_ATTR_VALUE_END)
        {
            pTableIndexCorAttrIdList[index].pAttrPath = NULL;
        }
        else 
        {
            ClInt32T depth = 0;

            clCorAttrPathAlloc(&pTableIndexCorAttrIdList[index].pAttrPath);
            clCorAttrPathInitialize(pTableIndexCorAttrIdList[index].pAttrPath);
            while(pTgtId->info.corId.attrId[depth + 1] != CL_MED_ATTR_VALUE_END)
            {
                clCorAttrPathAppend(pTableIndexCorAttrIdList[index].pAttrPath,
                        pTgtId->info.corId.attrId[depth],
                        pTgtId->info.corId.attrId[depth + 1]);
                depth += 2;
            }
        }
        index++;

        /* CLEAN UP */
        clHeapFree(pTgtId);
        pTgtId = NULL;
    }

    if(index < numOfIndices)
    {
        clLogError(CL_LOG_ARE_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                   "Failed processing all indices in table! Total indices = %d, Processed indices = %d",
                   numOfIndices, index);
        return 1; /* FIXME: Return proper error value. */
    }
    return CL_OK;
}
ClRcT clSnmpMutexInit(void)
{
    return clOsalMutexCreate(&gOper.mtx);
}

ClRcT clSnmpMutexFini(void)
{
    return clOsalMutexDelete(gOper.mtx);
}


/**
 * Process the get request. This is called in the mode end callback.
 */
int 
clSnmpProcessRequest(netsnmp_request_info *requests)
{
    ClRcT               rc = CL_SNMP_ERR_NOERROR;
    ClUint32T           i = 0;
    ClMedErrorListT     medErrList = {0};
    _ClSnmpGetReqInfoT  *pReqInfo = NULL;

    clLogDebug("SNP","OPE", "Calling the snmpCommit now.");

    rc = clSnmpCommit (&medErrList);
    if (CL_OK != rc)
    {
        clSnmpDataReset();
        clLogError("SNP","OPE", "Failed while committing the get request. rc[0x%x]", rc);
        if (CL_ERR_NO_MEMORY == rc)
            rc = CL_SNMP_ERR_NOCREATION;
        else
            rc = CL_SNMP_ERR_GENERR;
        netsnmp_request_set_error(requests, rc);
        return rc;
    }

    for (i = 0 ; i < gOper.opInfo.varCount ; i++)
    {
        pReqInfo = ((_ClSnmpGetReqInfoT *)(gOper.pOpAddData) + i);
        clLogDebug("SNP","OPE", "Processing the agentId [%s]", gOper.opInfo.varInfo[i].attrId.id);

        if (gOper.opInfo.varInfo[i].errId == CL_OK)
        {
            if (pReqInfo->columnType == ASN_COUNTER64)
            {
                ClUint64T val;

                val = *(ClUint64T *) gOper.opInfo.varInfo[i].pVal;
                val = CL_BIT_WORD_SWAP(val);
                *(ClUint64T *) gOper.opInfo.varInfo[i].pVal = val;
            }
            else if (pReqInfo->columnType == ASN_UNSIGNED64)
            {
                ClUint64T val;

                val = *(ClUint64T *) gOper.opInfo.varInfo[i].pVal;
                val = CL_BIT_WORD_SWAP(val);
                *(ClUint64T *) gOper.opInfo.varInfo[i].pVal = val;
            }
            else if (pReqInfo->columnType == ASN_INTEGER64)
            {
                ClInt64T val;

                val = *(ClInt64T *) gOper.opInfo.varInfo[i].pVal;
                val = CL_BIT_WORD_SWAP(val);
                *(ClInt64T *) gOper.opInfo.varInfo[i].pVal = val;
            }

            snmp_set_var_typed_value(pReqInfo->pRequest->requestvb, pReqInfo->columnType, 
                    (const u_char*)gOper.opInfo.varInfo[i].pVal, gOper.opInfo.varInfo[i].len);
        }
        else
        {
            clLogDebug("SNP","OPE", "Job failed with error [0x%x]", gOper.opInfo.varInfo[i].errId);
            netsnmp_request_set_error(pReqInfo->pRequest, gOper.opInfo.varInfo[i].errId);
        }
    }

    clSnmpDataReset();
    return rc;
}



/**
 * The mode end function which is called after every mode is finished.
 * This is used currently to do bulk get. When the vbcount of the requests
 * is equal to the varcount.
 */
int
clSnmpModeEndCallback(
        netsnmp_mib_handler               *handler,
        netsnmp_handler_registration      *reginfo,
        netsnmp_agent_request_info        *reqinfo,
        netsnmp_request_info              *requests)
{
    ClRcT   rc = CL_SNMP_ERR_NOERROR;
    netsnmp_request_info * request = NULL;
    static ClUint16T noOfOps = 0;

    clLogDebug("SNM","OPE", "Inside MODE-END callback [%s] ...mode [%d]", 
            __FUNCTION__, reqinfo->mode );
    
        switch (reqinfo->mode)
    {
        case MODE_GETBULK:
            clLogDebug("SNM","OPE", "Inside mode BULK-GET");
            break;
        case MODE_GETNEXT:
        case MODE_GET:
            {
                for (request = requests; request != NULL; request = request->next)
                {
                    noOfOps++;
                }

                clLogDebug("SNM","OPE", "Inside mode [%s] vbC[%d] noOfOps [%d]",
                        reqinfo->mode == MODE_GET ? "GET": "GET-NEXT", reqinfo->asp->vbcount, noOfOps);

                if (gErrorHappened == CL_TRUE)
                {
                    noOfOps = 0;
                    clSnmpDataReset();
                    gErrorHappened = CL_FALSE;
                    clLogNotice("SNM","OPE", "Error happened in the validating the request.");
                    netsnmp_request_set_error(requests, CL_SNMP_ERR_NOSUCHNAME);
                    return CL_SNMP_ERR_NOSUCHNAME;
                }
                        
                if (reqinfo->asp->vbcount == noOfOps)
                {
                    clLogDebug("SNP","OPE", "Sending the get request now as the vbCount and varCount are equal.");
                    rc = clSnmpProcessRequest(requests);
                    if (CL_OK != rc)
                        clLogError("SNP","OPE", "Failed while processing the GET request. rc[0x%x]", rc);
  		            noOfOps = 0;
                }
            }
            break;
        case MODE_SET_RESERVE1:
            clLogDebug("SNM","OPE", "Inside mode RESERVE1");
            break;
        case MODE_SET_RESERVE2:
            clLogDebug("SNM","OPE", "Inside mode RESERVE2");
            break;
        case MODE_SET_ACTION:
            clLogDebug("SNM","OPE", "Inside mode ACTION");
            break;
        case MODE_SET_COMMIT:
            clLogDebug("SNM","OPE", "Inside mode COMMIT");
            break;
        case MODE_SET_UNDO:
            clLogDebug("SNM","OPE", "Inside mode UNDO");
            break;
        case MODE_SET_FREE:
            clLogDebug("SNM","OPE", "Inside mode FREE");
            break;
        default:
            clLogDebug("SNM","OPE", "Inside default case");
    }

   return rc;
}

