
#include <pthread.h>
#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>
#include <clSnmpsafAmfTables.h>
#include <clSnmpsafAmfUtil.h>
#include <clSnmpOp.h>
#include <clBitApi.h>

 
static const ClCharT * gErrList[] = {
    "",                 /* SNMP_ERR_NOERROR */
    "Value specified is too big!",     /* SNMP_ERR_TOOBIG */
    "No such name exists!",        /* SNMP_ERR_NOSUCHNAME */
    "Duplicate or illegal value!",    /* SNMP_ERR_BADVALUE */
    "Read only access!",        /* SNMP_ERR_READONLY */
    "Error returned by the system!",    /* SNMP_ERR_GENERR */
    "No access!",            /* SNMP_ERR_NOACCESS */
    "Wrong type!",            /* SNMP_ERR_WRONGTYPE */
    "Wrong length!",            /* SNMP_ERR_WRONGLENGTH */
    "Wrong encoding!",            /* SNMP_ERR_WRONGENCODING */
    "Wrong value!",            /* SNMP_ERR_WRONGVALUE */
    "No creation!",            /* SNMP_ERR_NOCREATION */
    "Inconsistent value!",        /* SNMP_ERR_INCONSISTENTVALUE */
    "Resource unavailable!",        /* SNMP_ERR_RESOURCEUNAVAILABLE    */
    "Commit failed!",            /* SNMP_ERR_COMMITFAILED */
    "Undo failed!",            /* SNMP_ERR_UNDOFAILED */
    "Authorization error!",         /* SNMP_ERR_AUTHORIZATIONERROR */
    "Not writable!",            /* SNMP_ERR_NOTWRITABLE */
    "Inconsistent name!",        /* SNMP_ERR_INCONSISTENTNAME */
    "Unknown error!"            /* unknown error */
};

static oid saAmfApplicationTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,1 };
static oid saAmfNodeTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,2 };
static oid saAmfSGTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,3 };
static oid saAmfSUTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4 };
static oid saAmfSITableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5 };
static oid saAmfSUsperSIRankTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,6 };
static oid saAmfSGSIRankTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,7 };
static oid saAmfSGSURankTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,8 };
static oid saAmfSISIDepTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,9 };
static oid saAmfCompTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10 };
static oid saAmfCompCSTypeSupportedTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,11 };
static oid saAmfCSITableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,12 };
static oid saAmfCSICSIDepTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,13 };
static oid saAmfCSINameValueTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,14 };
static oid saAmfCSTypeAttrNameTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,15 };
static oid saAmfSUSITableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,16 };
static oid saAmfHealthCheckTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,17 };
static oid saAmfSCompCsiTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,18 };
static oid saAmfProxyProxiedTableOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,19 };


ClSnmpTableIndexCorAttrIdInfoT saAmfApplicationTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfApplicationNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfNodeTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfNodeNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSGTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSGNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSUTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSUNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSITableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSINameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSUsperSIRankTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSUsperSINameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSUsperSIRank */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSGSIRankTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSGSIRankSGNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSGSIRank */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSGSURankTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSGSURankSGNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSGSURank */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSISIDepTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSISIDepSINameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSISIDepDepndSINameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCompTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCompNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCompCSTypeSupportedTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCompCSTypeSupportedCompNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCompCSTypeSupportedCSTypeNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCSITableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSINameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCSICSIDepTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSICSIDepCSINameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSICSIDepDepndCSINameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCSINameValueTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSINameValueCSINameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSINameValueAttrNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfCSTypeAttrNameTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSTypeNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfCSTypeAttrNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSUSITableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSUSISuNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSUSISiNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfHealthCheckTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfHealthCompNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfHealthCheckKey */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfSCompCsiTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSCompCsiCompNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfSCompCsiCsiNameId */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT saAmfProxyProxiedTableIndexCorAttrIdList[2+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfProxyProxiedProxyNameId */
                                                                  { NULL, 0}, /* COR attribute ID for index saAmfProxyProxiedProxiedNameId */
                                                                  { NULL, -1}
                                                                 };

/** Initializes the safAmf module */
void
clSnmpsafAmfTablesInit(void)
{
  /* here we initialize all the tables we're planning on supporting */
  clSnmpsaAmfApplicationTableInitialize();
  clSnmpsaAmfNodeTableInitialize();
  clSnmpsaAmfSGTableInitialize();
  clSnmpsaAmfSUTableInitialize();
  clSnmpsaAmfSITableInitialize();
  clSnmpsaAmfSUsperSIRankTableInitialize();
  clSnmpsaAmfSGSIRankTableInitialize();
  clSnmpsaAmfSGSURankTableInitialize();
  clSnmpsaAmfSISIDepTableInitialize();
  clSnmpsaAmfCompTableInitialize();
  clSnmpsaAmfCompCSTypeSupportedTableInitialize();
  clSnmpsaAmfCSITableInitialize();
  clSnmpsaAmfCSICSIDepTableInitialize();
  clSnmpsaAmfCSINameValueTableInitialize();
  clSnmpsaAmfCSTypeAttrNameTableInitialize();
  clSnmpsaAmfSUSITableInitialize();
  clSnmpsaAmfHealthCheckTableInitialize();
  clSnmpsaAmfSCompCsiTableInitialize();
  clSnmpsaAmfProxyProxiedTableInitialize();
}

void clSnmpsafAmfTablesDeInit(void)
{
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Unloading safAmfTables"); 
}

/** Initialize the saAmfApplicationTable table by defining its contents and how it's structured */
void
clSnmpsaAmfApplicationTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfApplicationTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfApplicationTable",     clSnmpsaAmfApplicationTableHandler,
              saAmfApplicationTableOid, OID_LENGTH(saAmfApplicationTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfApplicationTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfApplicationNameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 7;
    
    iinfo->get_first_data_point = clSnmpsaAmfApplicationTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfApplicationTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfApplicationTableInitialize",
                "Registering table saAmfApplicationTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.1.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfApplicationTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfApplicationTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFAPPLICATIONTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfApplicationTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFAPPLICATIONTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfApplicationTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFAPPLICATIONTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfApplicationTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfApplicationTableInfo.saAmfApplicationNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFAPPLICATIONNAMEID:
                    {
                        ClUint32T   saAmfApplicationNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfApplicationNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONNAME:
                    {
                        ClCharT saAmfApplicationNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfApplicationNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONADMINSTATE:
                    {
                        ClInt32T    saAmfApplicationAdminStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfApplicationAdminStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONCURRNUMSG:
                    {
                        ClUint32T   saAmfApplicationCurrNumSGVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfApplicationCurrNumSGVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONADMINSTATETRIGGER:
                    {
                        ClInt32T    saAmfApplicationAdminStateTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfApplicationAdminStateTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONRESTARTSTATE:
                    {
                        ClInt32T    saAmfApplicationRestartStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfApplicationRestartStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONROWSTATUS:
                    {
                        ClInt32T    saAmfApplicationRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfApplicationRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFAPPLICATIONNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONADMINSTATETRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFAPPLICATIONADMINSTATETRIGGER_STABLE:
                                break;
                            case SAAMFAPPLICATIONADMINSTATETRIGGER_UNLOCK:
                                break;
                            case SAAMFAPPLICATIONADMINSTATETRIGGER_LOCKINSTANTIATION:
                                break;
                            case SAAMFAPPLICATIONADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONRESTARTSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFAPPLICATIONRESTARTSTATE_STABLE:
                                break;
                            case SAAMFAPPLICATIONRESTARTSTATE_RESTARTING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFAPPLICATIONROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFAPPLICATIONROWSTATUS_ACTIVE:
                                break;
                            case SAAMFAPPLICATIONROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFAPPLICATIONROWSTATUS_NOTREADY:
                                break;
                            case SAAMFAPPLICATIONROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFAPPLICATIONROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFAPPLICATIONROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFAPPLICATIONNAME:
                    {	
                        ClCharT saAmfApplicationNameVal[255] = {0};

                        pVal = saAmfApplicationNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFAPPLICATIONADMINSTATETRIGGER:
                    {	
                        ClInt32T    saAmfApplicationAdminStateTriggerVal = 0;
                        pVal = &saAmfApplicationAdminStateTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFAPPLICATIONRESTARTSTATE:
                    {	
                        ClInt32T    saAmfApplicationRestartStateVal = 0;
                        pVal = &saAmfApplicationRestartStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFAPPLICATIONROWSTATUS:
                    {	
                        ClInt32T    saAmfApplicationRowStatusVal = 0;
                        pVal = &saAmfApplicationRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFAPPLICATIONROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFAPPLICATIONROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfApplicationTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfApplicationTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfNodeTable table by defining its contents and how it's structured */
void
clSnmpsaAmfNodeTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfNodeTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfNodeTable",     clSnmpsaAmfNodeTableHandler,
              saAmfNodeTableOid, OID_LENGTH(saAmfNodeTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfNodeTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfNodeNameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 14;
    
    iinfo->get_first_data_point = clSnmpsaAmfNodeTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfNodeTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfNodeTableInitialize",
                "Registering table saAmfNodeTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.2.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfNodeTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfNodeTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFNODETABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfNodeTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFNODETABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfNodeTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFNODETABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfNodeTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfNodeTableInfo.saAmfNodeNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFNODENAMEID:
                    {
                        ClUint32T   saAmfNodeNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfNodeNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODENAME:
                    {
                        ClCharT saAmfNodeNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfNodeNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODECLMNODE:
                    {
                        ClCharT saAmfNodeClmNodeVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfNodeClmNodeVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODESUFAILOVERPROB:
                    {
                        ClCharT saAmfNodeSuFailoverProbVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfNodeSuFailoverProbVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODESUFAILOVERMAX:
                    {
                        ClUint32T   saAmfNodeSuFailoverMaxVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfNodeSuFailoverMaxVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEADMINSTATE:
                    {
                        ClInt32T    saAmfNodeAdminStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeAdminStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEOPERATIONALSTATE:
                    {
                        ClInt32T    saAmfNodeOperationalStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeOperationalStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEAUTOREPAIROPTION:
                    {
                        ClInt32T    saAmfNodeAutoRepairOptionVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeAutoRepairOptionVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREBOOTONTERMINATIONFAIL:
                    {
                        ClInt32T    saAmfNodeRebootOnTerminationFailVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeRebootOnTerminationFailVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREBOOTONINSTANTIATIONFAIL:
                    {
                        ClInt32T    saAmfNodeRebootOnInstantiationFailVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeRebootOnInstantiationFailVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEADMINSTATETRIGGER:
                    {
                        ClInt32T    saAmfNodeAdminStateTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeAdminStateTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODERESTARTSTATE:
                    {
                        ClInt32T    saAmfNodeRestartStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeRestartStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREPAIR:
                    {
                        ClInt32T    saAmfNodeRepairVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeRepairVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEROWSTATUS:
                    {
                        ClInt32T    saAmfNodeRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfNodeRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFNODENAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODECLMNODE:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODESUFAILOVERPROB:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODESUFAILOVERMAX:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFNODEAUTOREPAIROPTION:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEAUTOREPAIROPTION_TRUE:
                                break;
                            case SAAMFNODEAUTOREPAIROPTION_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREBOOTONTERMINATIONFAIL:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEREBOOTONTERMINATIONFAIL_TRUE:
                                break;
                            case SAAMFNODEREBOOTONTERMINATIONFAIL_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREBOOTONINSTANTIATIONFAIL:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEREBOOTONINSTANTIATIONFAIL_TRUE:
                                break;
                            case SAAMFNODEREBOOTONINSTANTIATIONFAIL_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEADMINSTATETRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEADMINSTATETRIGGER_STABLE:
                                break;
                            case SAAMFNODEADMINSTATETRIGGER_UNLOCK:
                                break;
                            case SAAMFNODEADMINSTATETRIGGER_LOCKINSTANTIATION:
                                break;
                            case SAAMFNODEADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODERESTARTSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODERESTARTSTATE_STABLE:
                                break;
                            case SAAMFNODERESTARTSTATE_RESTARTING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEREPAIR:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEREPAIR_STABLE:
                                break;
                            case SAAMFNODEREPAIR_REPAIRING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFNODEROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFNODEROWSTATUS_ACTIVE:
                                break;
                            case SAAMFNODEROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFNODEROWSTATUS_NOTREADY:
                                break;
                            case SAAMFNODEROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFNODEROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFNODEROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFNODENAME:
                    {	
                        ClCharT saAmfNodeNameVal[255] = {0};

                        pVal = saAmfNodeNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFNODECLMNODE:
                    {	
                        ClCharT saAmfNodeClmNodeVal[255] = {0};

                        pVal = saAmfNodeClmNodeVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFNODESUFAILOVERPROB:
                    {	
                        ClCharT saAmfNodeSuFailoverProbVal[8] = {0};

                        pVal = saAmfNodeSuFailoverProbVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFNODESUFAILOVERMAX:
                    {	
                        ClUint32T   saAmfNodeSuFailoverMaxVal = 0;
                        
                        pVal = &saAmfNodeSuFailoverMaxVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEAUTOREPAIROPTION:
                    {	
                        ClInt32T    saAmfNodeAutoRepairOptionVal = 0;
                        pVal = &saAmfNodeAutoRepairOptionVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEREBOOTONTERMINATIONFAIL:
                    {	
                        ClInt32T    saAmfNodeRebootOnTerminationFailVal = 0;
                        pVal = &saAmfNodeRebootOnTerminationFailVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEREBOOTONINSTANTIATIONFAIL:
                    {	
                        ClInt32T    saAmfNodeRebootOnInstantiationFailVal = 0;
                        pVal = &saAmfNodeRebootOnInstantiationFailVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEADMINSTATETRIGGER:
                    {	
                        ClInt32T    saAmfNodeAdminStateTriggerVal = 0;
                        pVal = &saAmfNodeAdminStateTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODERESTARTSTATE:
                    {	
                        ClInt32T    saAmfNodeRestartStateVal = 0;
                        pVal = &saAmfNodeRestartStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEREPAIR:
                    {	
                        ClInt32T    saAmfNodeRepairVal = 0;
                        pVal = &saAmfNodeRepairVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFNODEROWSTATUS:
                    {	
                        ClInt32T    saAmfNodeRowStatusVal = 0;
                        pVal = &saAmfNodeRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFNODEROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFNODEROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfNodeTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfNodeTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSGTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSGTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSGTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSGTable",     clSnmpsaAmfSGTableHandler,
              saAmfSGTableOid, OID_LENGTH(saAmfSGTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSGTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSGNameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 23;
    
    iinfo->get_first_data_point = clSnmpsaAmfSGTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSGTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSGTableInitialize",
                "Registering table saAmfSGTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.3.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSGTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSGTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSGTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSGTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSGTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSGTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSGTableInfo.saAmfSGNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSGNAMEID:
                    {
                        ClUint32T   saAmfSGNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNAME:
                    {
                        ClCharT saAmfSGNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSGNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGREDMODEL:
                    {
                        ClInt32T    saAmfSGRedModelVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGRedModelVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFACTIVESUS:
                    {
                        ClUint32T   saAmfSGNumPrefActiveSUsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumPrefActiveSUsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFSTANDBYSUS:
                    {
                        ClUint32T   saAmfSGNumPrefStandbySUsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumPrefStandbySUsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFINSERVICESUS:
                    {
                        ClUint32T   saAmfSGNumPrefInserviceSUsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumPrefInserviceSUsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFASSIGNEDSUS:
                    {
                        ClUint32T   saAmfSGNumPrefAssignedSUsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumPrefAssignedSUsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGMAXACTIVESISPERSU:
                    {
                        ClUint32T   saAmfSGMaxActiveSIsperSUVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGMaxActiveSIsperSUVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGMAXSTANDBYSISPERSU:
                    {
                        ClUint32T   saAmfSGMaxStandbySIsperSUVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGMaxStandbySIsperSUVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGADMINSTATE:
                    {
                        ClInt32T    saAmfSGAdminStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGAdminStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGCOMPRESTARTPROB:
                    {
                        ClCharT saAmfSGCompRestartProbVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfSGCompRestartProbVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGCOMPRESTARTMAX:
                    {
                        ClUint32T   saAmfSGCompRestartMaxVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGCompRestartMaxVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURESTARTPROB:
                    {
                        ClCharT saAmfSGSuRestartProbVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfSGSuRestartProbVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURESTARTMAX:
                    {
                        ClUint32T   saAmfSGSuRestartMaxVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGSuRestartMaxVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGAUTOREPAIROPTION:
                    {
                        ClInt32T    saAmfSGAutoRepairOptionVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGAutoRepairOptionVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMCURRASSIGNEDSU:
                    {
                        ClUint32T   saAmfSGNumCurrAssignedSUVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumCurrAssignedSUVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMCURRNONINSTANTIATEDSPARESU:
                    {
                        ClUint32T   saAmfSGNumCurrNonInstantiatedSpareSUVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumCurrNonInstantiatedSpareSUVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMCURRINSTANTIATEDSPARESU:
                    {
                        ClUint32T   saAmfSGNumCurrInstantiatedSpareSUVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGNumCurrInstantiatedSpareSUVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGAUTOADJUSTOPTION:
                    {
                        ClInt32T    saAmfSGAutoAdjustOptionVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGAutoAdjustOptionVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGAUTOADJUSTPROB:
                    {
                        ClCharT saAmfSGAutoAdjustProbVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfSGAutoAdjustProbVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGADJUSTSTATE:
                    {
                        ClInt32T    saAmfSGAdjustStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGAdjustStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGADMINSTATETRIGGER:
                    {
                        ClInt32T    saAmfSGAdminStateTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGAdminStateTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGROWSTATUS:
                    {
                        ClInt32T    saAmfSGRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSGRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSGNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGREDMODEL:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGREDMODEL_TWON:
                                break;
                            case SAAMFSGREDMODEL_NPLUSM:
                                break;
                            case SAAMFSGREDMODEL_NWAY:
                                break;
                            case SAAMFSGREDMODEL_NWAYACTIVE:
                                break;
                            case SAAMFSGREDMODEL_NOREDUNDANCY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFACTIVESUS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFSTANDBYSUS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFINSERVICESUS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGNUMPREFASSIGNEDSUS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGMAXACTIVESISPERSU:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGMAXSTANDBYSISPERSU:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGCOMPRESTARTPROB:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGCOMPRESTARTMAX:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGSURESTARTPROB:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURESTARTMAX:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSGAUTOREPAIROPTION:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGAUTOREPAIROPTION_TRUE:
                                break;
                            case SAAMFSGAUTOREPAIROPTION_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGAUTOADJUSTOPTION:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGAUTOADJUSTOPTION_TRUE:
                                break;
                            case SAAMFSGAUTOADJUSTOPTION_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGAUTOADJUSTPROB:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGADJUSTSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGADJUSTSTATE_STABLE:
                                break;
                            case SAAMFSGADJUSTSTATE_ADJUSTING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGADMINSTATETRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGADMINSTATETRIGGER_STABLE:
                                break;
                            case SAAMFSGADMINSTATETRIGGER_UNLOCK:
                                break;
                            case SAAMFSGADMINSTATETRIGGER_LOCKINSTANTIATION:
                                break;
                            case SAAMFSGADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSGROWSTATUS_ACTIVE:
                                break;
                            case SAAMFSGROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFSGROWSTATUS_NOTREADY:
                                break;
                            case SAAMFSGROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFSGROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFSGROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSGNAME:
                    {	
                        ClCharT saAmfSGNameVal[255] = {0};

                        pVal = saAmfSGNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSGREDMODEL:
                    {	
                        ClInt32T    saAmfSGRedModelVal = 0;
                        pVal = &saAmfSGRedModelVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGNUMPREFACTIVESUS:
                    {	
                        ClUint32T   saAmfSGNumPrefActiveSUsVal = 0;
                        
                        pVal = &saAmfSGNumPrefActiveSUsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGNUMPREFSTANDBYSUS:
                    {	
                        ClUint32T   saAmfSGNumPrefStandbySUsVal = 0;
                        
                        pVal = &saAmfSGNumPrefStandbySUsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGNUMPREFINSERVICESUS:
                    {	
                        ClUint32T   saAmfSGNumPrefInserviceSUsVal = 0;
                        
                        pVal = &saAmfSGNumPrefInserviceSUsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGNUMPREFASSIGNEDSUS:
                    {	
                        ClUint32T   saAmfSGNumPrefAssignedSUsVal = 0;
                        
                        pVal = &saAmfSGNumPrefAssignedSUsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGMAXACTIVESISPERSU:
                    {	
                        ClUint32T   saAmfSGMaxActiveSIsperSUVal = 0;
                        
                        pVal = &saAmfSGMaxActiveSIsperSUVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGMAXSTANDBYSISPERSU:
                    {	
                        ClUint32T   saAmfSGMaxStandbySIsperSUVal = 0;
                        
                        pVal = &saAmfSGMaxStandbySIsperSUVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGCOMPRESTARTPROB:
                    {	
                        ClCharT saAmfSGCompRestartProbVal[8] = {0};

                        pVal = saAmfSGCompRestartProbVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSGCOMPRESTARTMAX:
                    {	
                        ClUint32T   saAmfSGCompRestartMaxVal = 0;
                        
                        pVal = &saAmfSGCompRestartMaxVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGSURESTARTPROB:
                    {	
                        ClCharT saAmfSGSuRestartProbVal[8] = {0};

                        pVal = saAmfSGSuRestartProbVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSGSURESTARTMAX:
                    {	
                        ClUint32T   saAmfSGSuRestartMaxVal = 0;
                        
                        pVal = &saAmfSGSuRestartMaxVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGAUTOREPAIROPTION:
                    {	
                        ClInt32T    saAmfSGAutoRepairOptionVal = 0;
                        pVal = &saAmfSGAutoRepairOptionVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGAUTOADJUSTOPTION:
                    {	
                        ClInt32T    saAmfSGAutoAdjustOptionVal = 0;
                        pVal = &saAmfSGAutoAdjustOptionVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGAUTOADJUSTPROB:
                    {	
                        ClCharT saAmfSGAutoAdjustProbVal[8] = {0};

                        pVal = saAmfSGAutoAdjustProbVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSGADJUSTSTATE:
                    {	
                        ClInt32T    saAmfSGAdjustStateVal = 0;
                        pVal = &saAmfSGAdjustStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGADMINSTATETRIGGER:
                    {	
                        ClInt32T    saAmfSGAdminStateTriggerVal = 0;
                        pVal = &saAmfSGAdminStateTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSGROWSTATUS:
                    {	
                        ClInt32T    saAmfSGRowStatusVal = 0;
                        pVal = &saAmfSGRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFSGROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFSGROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSGTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSUTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSUTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSUTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSUTable",     clSnmpsaAmfSUTableHandler,
              saAmfSUTableOid, OID_LENGTH(saAmfSUTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSUTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSUNameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 19;
    
    iinfo->get_first_data_point = clSnmpsaAmfSUTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSUTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSUTableInitialize",
                "Registering table saAmfSUTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.4.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSUTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSUTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSUTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSUTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSUTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSUTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSUTableInfo.saAmfSUNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSUNAMEID:
                    {
                        ClUint32T   saAmfSUNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUNAME:
                    {
                        ClCharT saAmfSUNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSURANK:
                    {
                        ClUint32T   saAmfSURankVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSURankVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUNUMCOMPONENTS:
                    {
                        ClUint32T   saAmfSUNumComponentsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUNumComponentsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUNUMCURRACTIVESIS:
                    {
                        ClUint32T   saAmfSUNumCurrActiveSIsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUNumCurrActiveSIsVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUNUMCURRSTANDBYSIS:
                    {
                        ClUint32T   saAmfSUNumCurrStandbySIsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUNumCurrStandbySIsVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUADMINSTATE:
                    {
                        ClInt32T    saAmfSUAdminStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUAdminStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUFAILOVER:
                    {
                        ClInt32T    saAmfSUFailOverVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUFailOverVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUREADINESSSTATE:
                    {
                        ClInt32T    saAmfSUReadinessStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUReadinessStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUOPERSTATE:
                    {
                        ClInt32T    saAmfSUOperStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUOperStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUPRESENCESTATE:
                    {
                        ClInt32T    saAmfSUPresenceStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUPresenceStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUPREINSTANTIABLE:
                    {
                        ClInt32T    saAmfSUPreInstantiableVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUPreInstantiableVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUISEXTERNAL:
                    {
                        ClInt32T    saAmfSUIsExternalVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUIsExternalVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUHOSTEDBYNODE:
                    {
                        ClCharT saAmfSUHostedByNodeVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUHostedByNodeVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUADMINSTATETRIGGER:
                    {
                        ClInt32T    saAmfSUAdminStateTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUAdminStateTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSURESTARTSTATE:
                    {
                        ClInt32T    saAmfSURestartStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSURestartStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUEAMTRIGGER:
                    {
                        ClInt32T    saAmfSUEamTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUEamTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUREPAIR:
                    {
                        ClInt32T    saAmfSURepairVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSURepairVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUROWSTATUS:
                    {
                        ClInt32T    saAmfSURowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSURowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSUNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSURANK:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSUNUMCOMPONENTS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSUFAILOVER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUFAILOVER_TRUE:
                                break;
                            case SAAMFSUFAILOVER_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUISEXTERNAL:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUISEXTERNAL_TRUE:
                                break;
                            case SAAMFSUISEXTERNAL_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUADMINSTATETRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUADMINSTATETRIGGER_STABLE:
                                break;
                            case SAAMFSUADMINSTATETRIGGER_UNLOCK:
                                break;
                            case SAAMFSUADMINSTATETRIGGER_LOCKINSTANTIATION:
                                break;
                            case SAAMFSUADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSURESTARTSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSURESTARTSTATE_STABLE:
                                break;
                            case SAAMFSURESTARTSTATE_RESTARTING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUEAMTRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUEAMTRIGGER_START:
                                break;
                            case SAAMFSUEAMTRIGGER_STOP:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUREPAIR:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUREPAIR_STABLE:
                                break;
                            case SAAMFSUREPAIR_REPAIRING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUROWSTATUS_ACTIVE:
                                break;
                            case SAAMFSUROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFSUROWSTATUS_NOTREADY:
                                break;
                            case SAAMFSUROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFSUROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFSUROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSUNAME:
                    {	
                        ClCharT saAmfSUNameVal[255] = {0};

                        pVal = saAmfSUNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSURANK:
                    {	
                        ClUint32T   saAmfSURankVal = 0;
                        
                        pVal = &saAmfSURankVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUNUMCOMPONENTS:
                    {	
                        ClUint32T   saAmfSUNumComponentsVal = 0;
                        
                        pVal = &saAmfSUNumComponentsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUFAILOVER:
                    {	
                        ClInt32T    saAmfSUFailOverVal = 0;
                        pVal = &saAmfSUFailOverVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUISEXTERNAL:
                    {	
                        ClInt32T    saAmfSUIsExternalVal = 0;
                        pVal = &saAmfSUIsExternalVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUADMINSTATETRIGGER:
                    {	
                        ClInt32T    saAmfSUAdminStateTriggerVal = 0;
                        pVal = &saAmfSUAdminStateTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSURESTARTSTATE:
                    {	
                        ClInt32T    saAmfSURestartStateVal = 0;
                        pVal = &saAmfSURestartStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUEAMTRIGGER:
                    {	
                        ClInt32T    saAmfSUEamTriggerVal = 0;
                        pVal = &saAmfSUEamTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUREPAIR:
                    {	
                        ClInt32T    saAmfSURepairVal = 0;
                        pVal = &saAmfSURepairVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSUROWSTATUS:
                    {	
                        ClInt32T    saAmfSURowStatusVal = 0;
                        pVal = &saAmfSURowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFSUROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFSUROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSUTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSITable table by defining its contents and how it's structured */
void
clSnmpsaAmfSITableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSITableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSITable",     clSnmpsaAmfSITableHandler,
              saAmfSITableOid, OID_LENGTH(saAmfSITableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSITableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSINameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 13;
    
    iinfo->get_first_data_point = clSnmpsaAmfSITableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSITableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSITableInitialize",
                "Registering table saAmfSITable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.5.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfSITableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSITableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSITABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSITableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSITABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSITableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSITABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSITableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSITableInfo.saAmfSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSINAMEID:
                    {
                        ClUint32T   saAmfSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSINAME:
                    {
                        ClCharT saAmfSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIRANK:
                    {
                        ClUint32T   saAmfSIRankVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSIRankVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSINUMCSIS:
                    {
                        ClUint32T   saAmfSINumCSIsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSINumCSIsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIPREFNUMASSIGNMENTS:
                    {
                        ClUint32T   saAmfSIPrefNumAssignmentsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSIPrefNumAssignmentsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIADMINSTATE:
                    {
                        ClInt32T    saAmfSIAdminStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSIAdminStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIASSIGNMENTSTATE:
                    {
                        ClInt32T    saAmfSIAssignmentStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSIAssignmentStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSINUMCURRACTIVEASSIGNMENTS:
                    {
                        ClUint32T   saAmfSINumCurrActiveAssignmentsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSINumCurrActiveAssignmentsVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSINUMCURRSTANDBYASSIGNMENTS:
                    {
                        ClUint32T   saAmfSINumCurrStandbyAssignmentsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSINumCurrStandbyAssignmentsVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIPROTECTEDBYSG:
                    {
                        ClCharT saAmfSIProtectedbySGVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSIProtectedbySGVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIADMINSTATETRIGGER:
                    {
                        ClInt32T    saAmfSIAdminStateTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSIAdminStateTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISWAPSTATE:
                    {
                        ClInt32T    saAmfSISwapStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSISwapStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIROWSTATUS:
                    {
                        ClInt32T    saAmfSIRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSIRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIRANK:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSINUMCSIS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSIPREFNUMASSIGNMENTS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFSIADMINSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSIADMINSTATE_UNLOCKED:
                                break;
                            case SAAMFSIADMINSTATE_LOCKED:
                                break;
                            case SAAMFSIADMINSTATE_SHUTTINGDOWN:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIPROTECTEDBYSG:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIADMINSTATETRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSIADMINSTATETRIGGER_STABLE:
                                break;
                            case SAAMFSIADMINSTATETRIGGER_UNLOCK:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISWAPSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSISWAPSTATE_STABLE:
                                break;
                            case SAAMFSISWAPSTATE_SWAPPING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSIROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSIROWSTATUS_ACTIVE:
                                break;
                            case SAAMFSIROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFSIROWSTATUS_NOTREADY:
                                break;
                            case SAAMFSIROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFSIROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFSIROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSINAME:
                    {	
                        ClCharT saAmfSINameVal[255] = {0};

                        pVal = saAmfSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSIRANK:
                    {	
                        ClUint32T   saAmfSIRankVal = 0;
                        
                        pVal = &saAmfSIRankVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSINUMCSIS:
                    {	
                        ClUint32T   saAmfSINumCSIsVal = 0;
                        
                        pVal = &saAmfSINumCSIsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSIPREFNUMASSIGNMENTS:
                    {	
                        ClUint32T   saAmfSIPrefNumAssignmentsVal = 0;
                        
                        pVal = &saAmfSIPrefNumAssignmentsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSIADMINSTATE:
                    {	
                        ClInt32T    saAmfSIAdminStateVal = 0;
                        pVal = &saAmfSIAdminStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSIPROTECTEDBYSG:
                    {	
                        ClCharT saAmfSIProtectedbySGVal[255] = {0};

                        pVal = saAmfSIProtectedbySGVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSIADMINSTATETRIGGER:
                    {	
                        ClInt32T    saAmfSIAdminStateTriggerVal = 0;
                        pVal = &saAmfSIAdminStateTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSISWAPSTATE:
                    {	
                        ClInt32T    saAmfSISwapStateVal = 0;
                        pVal = &saAmfSISwapStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFSIROWSTATUS:
                    {	
                        ClInt32T    saAmfSIRowStatusVal = 0;
                        pVal = &saAmfSIRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFSIROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFSIROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSITableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSITableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSUsperSIRankTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSUsperSIRankTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSUsperSIRankTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSUsperSIRankTable",     clSnmpsaAmfSUsperSIRankTableHandler,
              saAmfSUsperSIRankTableOid, OID_LENGTH(saAmfSUsperSIRankTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSUsperSIRankTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSUsperSINameId */
                           ASN_UNSIGNED,  /* index: saAmfSUsperSIRank */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfSUsperSIRankTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSUsperSIRankTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSUsperSIRankTableInitialize",
                "Registering table saAmfSUsperSIRankTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSUsperSIRankTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUSPERSIRANKTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSUsperSIRankTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUSPERSIRANKTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSUsperSIRankTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSUSPERSIRANKTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSUsperSIRankTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSUsperSIRankTableInfo.saAmfSUsperSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSUsperSIRankTableInfo.saAmfSUsperSIRank), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSUSPERSINAMEID:
                    {
                        ClUint32T   saAmfSUsperSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUsperSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSIRANK:
                    {
                        ClUint32T   saAmfSUsperSIRankVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUsperSIRankVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSINAME:
                    {
                        ClCharT saAmfSUsperSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUsperSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSISUNAME:
                    {
                        ClCharT saAmfSUsperSISUNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUsperSISUNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSIROWSTATUS:
                    {
                        ClInt32T    saAmfSUsperSIRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUsperSIRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSUSPERSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSISUNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSPERSIROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSUSPERSIROWSTATUS_ACTIVE:
                                break;
                            case SAAMFSUSPERSIROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFSUSPERSIROWSTATUS_NOTREADY:
                                break;
                            case SAAMFSUSPERSIROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFSUSPERSIROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFSUSPERSIROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSUSPERSINAME:
                    {	
                        ClCharT saAmfSUsperSINameVal[255] = {0};

                        pVal = saAmfSUsperSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSUSPERSISUNAME:
                    {	
                        ClCharT saAmfSUsperSISUNameVal[255] = {0};

                        pVal = saAmfSUsperSISUNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSUSPERSIROWSTATUS:
                    {	
                        ClInt32T    saAmfSUsperSIRowStatusVal = 0;
                        pVal = &saAmfSUsperSIRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFSUSPERSIROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFSUSPERSIROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUsperSIRankTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSUsperSIRankTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSGSIRankTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSGSIRankTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSGSIRankTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSGSIRankTable",     clSnmpsaAmfSGSIRankTableHandler,
              saAmfSGSIRankTableOid, OID_LENGTH(saAmfSGSIRankTableOid),
              HANDLER_CAN_RONLY
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSGSIRankTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSGSIRankSGNameId */
                           ASN_UNSIGNED,  /* index: saAmfSGSIRank */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmpsaAmfSGSIRankTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSGSIRankTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSGSIRankTableInitialize",
                "Registering table saAmfSGSIRankTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);


}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSGSIRankTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGSIRANKTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSGSIRankTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGSIRANKTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSGSIRankTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSGSIRANKTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSGSIRankTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSGSIRankTableInfo.saAmfSGSIRankSGNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSGSIRankTableInfo.saAmfSGSIRank), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSGSIRANKSGNAMEID:
                    {
                        ClUint32T   saAmfSGSIRankSGNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGSIRankSGNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSIRANK:
                    {
                        ClUint32T   saAmfSGSIRankVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGSIRankVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSIRANKSGNAME:
                    {
                        ClCharT saAmfSGSIRankSGNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSGSIRankSGNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSIRANKSINAME:
                    {
                        ClCharT saAmfSGSIRankSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSGSIRankSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSIRankTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSGSIRankTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSGSURankTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSGSURankTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSGSURankTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSGSURankTable",     clSnmpsaAmfSGSURankTableHandler,
              saAmfSGSURankTableOid, OID_LENGTH(saAmfSGSURankTableOid),
              HANDLER_CAN_RONLY
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSGSURankTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSGSURankSGNameId */
                           ASN_UNSIGNED,  /* index: saAmfSGSURank */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmpsaAmfSGSURankTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSGSURankTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSGSURankTableInitialize",
                "Registering table saAmfSGSURankTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);


}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSGSURankTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGSURANKTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSGSURankTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSGSURANKTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSGSURankTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSGSURANKTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSGSURankTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSGSURankTableInfo.saAmfSGSURankSGNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSGSURankTableInfo.saAmfSGSURank), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSGSURANKSGNAMEID:
                    {
                        ClUint32T   saAmfSGSURankSGNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGSURankSGNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURANK:
                    {
                        ClUint32T   saAmfSGSURankVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSGSURankVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURANKSGNAME:
                    {
                        ClCharT saAmfSGSURankSGNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSGSURankSGNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSGSURANKSUNAME:
                    {
                        ClCharT saAmfSGSURankSUNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSGSURankSUNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSGSURankTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSGSURankTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSISIDepTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSISIDepTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSISIDepTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSISIDepTable",     clSnmpsaAmfSISIDepTableHandler,
              saAmfSISIDepTableOid, OID_LENGTH(saAmfSISIDepTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSISIDepTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSISIDepSINameId */
                           ASN_UNSIGNED,  /* index: saAmfSISIDepDepndSINameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 6;
    
    iinfo->get_first_data_point = clSnmpsaAmfSISIDepTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSISIDepTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSISIDepTableInitialize",
                "Registering table saAmfSISIDepTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSISIDepTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSISIDEPTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSISIDepTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSISIDEPTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSISIDepTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSISIDEPTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSISIDepTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSISIDepTableInfo.saAmfSISIDepSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSISIDepTableInfo.saAmfSISIDepDepndSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSISIDEPSINAMEID:
                    {
                        ClUint32T   saAmfSISIDepSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSISIDepSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPDEPNDSINAMEID:
                    {
                        ClUint32T   saAmfSISIDepDepndSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSISIDepDepndSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPSINAME:
                    {
                        ClCharT saAmfSISIDepSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSISIDepSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPDEPNDSINAME:
                    {
                        ClCharT saAmfSISIDepDepndSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSISIDepDepndSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPTOLTIME:
                    {
                        ClCharT saAmfSISIDepToltimeVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfSISIDepToltimeVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPROWSTATUS:
                    {
                        ClInt32T    saAmfSISIDepRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSISIDepRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSISIDEPSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPDEPNDSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPTOLTIME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSISIDEPROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFSISIDEPROWSTATUS_ACTIVE:
                                break;
                            case SAAMFSISIDEPROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFSISIDEPROWSTATUS_NOTREADY:
                                break;
                            case SAAMFSISIDEPROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFSISIDEPROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFSISIDEPROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFSISIDEPSINAME:
                    {	
                        ClCharT saAmfSISIDepSINameVal[255] = {0};

                        pVal = saAmfSISIDepSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSISIDEPDEPNDSINAME:
                    {	
                        ClCharT saAmfSISIDepDepndSINameVal[255] = {0};

                        pVal = saAmfSISIDepDepndSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSISIDEPTOLTIME:
                    {	
                        ClCharT saAmfSISIDepToltimeVal[8] = {0};

                        pVal = saAmfSISIDepToltimeVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFSISIDEPROWSTATUS:
                    {	
                        ClInt32T    saAmfSISIDepRowStatusVal = 0;
                        pVal = &saAmfSISIDepRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFSISIDEPROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFSISIDEPROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSISIDepTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSISIDepTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCompTable table by defining its contents and how it's structured */
void
clSnmpsaAmfCompTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCompTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCompTable",     clSnmpsaAmfCompTableHandler,
              saAmfCompTableOid, OID_LENGTH(saAmfCompTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCompTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCompNameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 40;
    
    iinfo->get_first_data_point = clSnmpsaAmfCompTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCompTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCompTableInitialize",
                "Registering table saAmfCompTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.10.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfCompTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCompTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCOMPTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCompTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCOMPTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCompTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCOMPTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCompTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCompTableInfo.saAmfCompNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCOMPNAMEID:
                    {
                        ClUint32T   saAmfCompNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNAME:
                    {
                        ClCharT saAmfCompNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCAPABILITY:
                    {
                        ClInt32T    saAmfCompCapabilityVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompCapabilityVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCATEGORY:
                    {
                        ClInt32T    saAmfCompCategoryVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompCategoryVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATECMD:
                    {
                        ClCharT saAmfCompInstantiateCmdVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompInstantiateCmdVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATECMD:
                    {
                        ClCharT saAmfCompTerminateCmdVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompTerminateCmdVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCLEANUPCMD:
                    {
                        ClCharT saAmfCompCleanupCmdVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompCleanupCmdVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTARTCMD:
                    {
                        ClCharT saAmfCompAmStartCmdVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompAmStartCmdVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTOPCMD:
                    {
                        ClCharT saAmfCompAmStopCmdVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompAmStopCmdVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATIONLEVEL:
                    {
                        ClUint32T   saAmfCompInstantiationLevelVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompInstantiationLevelVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDEFAULTCLCCLITIMEOUT:
                    {
                        ClCharT saAmfCompDefaultClcCliTimeOutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompDefaultClcCliTimeOutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDEFAULTCALLBACKTIMEOUT:
                    {
                        ClCharT saAmfCompDefaultCallbackTimeOutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompDefaultCallbackTimeOutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATETIMEOUT:
                    {
                        ClCharT saAmfCompInstantiateTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompInstantiateTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDELAYBETWEENINSTANTIATEATTEMPTS:
                    {
                        ClCharT saAmfCompDelayBetweenInstantiateAttemptsVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompDelayBetweenInstantiateAttemptsVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATETIMEOUT:
                    {
                        ClCharT saAmfCompTerminateTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompTerminateTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCLEANUPTIMEOUT:
                    {
                        ClCharT saAmfCompCleanupTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompCleanupTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTARTTIMEOUT:
                    {
                        ClCharT saAmfCompAmStartTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompAmStartTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTOPTIMEOUT:
                    {
                        ClCharT saAmfCompAmStopTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompAmStopTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATECALLBACKTIMEOUT:
                    {
                        ClCharT saAmfCompTerminateCallbackTimeOutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompTerminateCallbackTimeOutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSISETCALLBACKTIMEOUT:
                    {
                        ClCharT saAmfCompCSISetCallbackTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompCSISetCallbackTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPQUIESCINGCOMPLETETIMEOUT:
                    {
                        ClCharT saAmfCompQuiescingCompleteTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompQuiescingCompleteTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSIRMVCALLBACKTIMEOUT:
                    {
                        ClCharT saAmfCompCSIRmvCallbackTimeoutVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfCompCSIRmvCallbackTimeoutVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPRECOVERYONERROR:
                    {
                        ClInt32T    saAmfCompRecoveryOnErrorVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompRecoveryOnErrorVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHOUTDELAY:
                    {
                        ClUint32T   saAmfCompNumMaxInstantiateWithoutDelayVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumMaxInstantiateWithoutDelayVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHDELAY:
                    {
                        ClUint32T   saAmfCompNumMaxInstantiateWithDelayVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumMaxInstantiateWithDelayVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXAMSTARTSTOPATTEMPTS:
                    {
                        ClUint32T   saAmfCompNumMaxAmStartStopAttemptsVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumMaxAmStartStopAttemptsVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDISABLERESTART:
                    {
                        ClInt32T    saAmfCompDisableRestartVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompDisableRestartVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXACTIVECSI:
                    {
                        ClUint32T   saAmfCompNumMaxActiveCsiVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumMaxActiveCsiVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXSTANDBYCSI:
                    {
                        ClUint32T   saAmfCompNumMaxStandbyCsiVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumMaxStandbyCsiVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMCURRACTIVECSI:
                    {
                        ClUint32T   saAmfCompNumCurrActiveCsiVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumCurrActiveCsiVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMCURRSTANDBYCSI:
                    {
                        ClUint32T   saAmfCompNumCurrStandbyCsiVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompNumCurrStandbyCsiVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPRESTARTCOUNT:
                    {
                        ClUint32T   saAmfCompRestartCountVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompRestartCountVal;

                        reqAddInfo.columnType = ASN_COUNTER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPOPERSTATE:
                    {
                        ClInt32T    saAmfCompOperStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompOperStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPPRESENCESTATE:
                    {
                        ClInt32T    saAmfCompPresenceStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompPresenceStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPREADINESSSTATE:
                    {
                        ClInt32T    saAmfCompReadinessStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompReadinessStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCURRPROXYNAME:
                    {
                        ClCharT saAmfCompCurrProxyNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompCurrProxyNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPPROXYCSI:
                    {
                        ClCharT saAmfCompProxyCSIVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompProxyCSIVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPRESTARTSTATE:
                    {
                        ClInt32T    saAmfCompRestartStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompRestartStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPEAMTRIGGER:
                    {
                        ClInt32T    saAmfCompEamTriggerVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompEamTriggerVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPROWSTATUS:
                    {
                        ClInt32T    saAmfCompRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCOMPNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCAPABILITY:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPCAPABILITY_XACTIVEANDYSTANDBY:
                                break;
                            case SAAMFCOMPCAPABILITY_XACTIVEORYSTANDBY:
                                break;
                            case SAAMFCOMPCAPABILITY_ONEACTIVEORXSTANDBY:
                                break;
                            case SAAMFCOMPCAPABILITY_ONEACTIVEORONESTANDBY:
                                break;
                            case SAAMFCOMPCAPABILITY_XACTIVE:
                                break;
                            case SAAMFCOMPCAPABILITY_ONEACTIVE:
                                break;
                            case SAAMFCOMPCAPABILITY_NONPREINSTANTIABLE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCATEGORY:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPCATEGORY_SAAWARE:
                                break;
                            case SAAMFCOMPCATEGORY_PROXIEDLOCALPREINSTANTIABLE:
                                break;
                            case SAAMFCOMPCATEGORY_PROXIEDLOCALNONPREINSTANTIABLE:
                                break;
                            case SAAMFCOMPCATEGORY_EXTERNALPREINSTANTIABLE:
                                break;
                            case SAAMFCOMPCATEGORY_EXTERNALNONPREINSTANTIABLE:
                                break;
                            case SAAMFCOMPCATEGORY_UNPROXIEDNONSAAWARE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATECMD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATECMD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCLEANUPCMD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTARTCMD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTOPCMD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATIONLEVEL:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPDEFAULTCLCCLITIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDEFAULTCALLBACKTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPINSTANTIATETIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPDELAYBETWEENINSTANTIATEATTEMPTS:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATETIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCLEANUPTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTARTTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPAMSTOPTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPTERMINATECALLBACKTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSISETCALLBACKTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPQUIESCINGCOMPLETETIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSIRMVCALLBACKTIMEOUT:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPRECOVERYONERROR:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPRECOVERYONERROR_NONE:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_COMPONENTRESTART:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_COMPONENTFAILOVER:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_NODESWITCHOVER:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_NODEFAILOVER:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_NODEFAILFAST:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_CLUSTERRESET:
                                break;
                            case SAAMFCOMPRECOVERYONERROR_APPLICATIONRESTART:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHOUTDELAY:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHDELAY:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXAMSTARTSTOPATTEMPTS:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPDISABLERESTART:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPDISABLERESTART_TRUE:
                                break;
                            case SAAMFCOMPDISABLERESTART_FALSE:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXACTIVECSI:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPNUMMAXSTANDBYCSI:
                    {
                        columnType = ASN_UNSIGNED;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 2147483647);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_SAAMFCOMPPROXYCSI:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPRESTARTSTATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPRESTARTSTATE_STABLE:
                                break;
                            case SAAMFCOMPRESTARTSTATE_RESTARTING:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPEAMTRIGGER:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPEAMTRIGGER_START:
                                break;
                            case SAAMFCOMPEAMTRIGGER_STOP:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCOMPROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCOMPROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCOMPROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCOMPROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCOMPROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCOMPNAME:
                    {	
                        ClCharT saAmfCompNameVal[255] = {0};

                        pVal = saAmfCompNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCAPABILITY:
                    {	
                        ClInt32T    saAmfCompCapabilityVal = 0;
                        pVal = &saAmfCompCapabilityVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCATEGORY:
                    {	
                        ClInt32T    saAmfCompCategoryVal = 0;
                        pVal = &saAmfCompCategoryVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPINSTANTIATECMD:
                    {	
                        ClCharT saAmfCompInstantiateCmdVal[255] = {0};

                        pVal = saAmfCompInstantiateCmdVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPTERMINATECMD:
                    {	
                        ClCharT saAmfCompTerminateCmdVal[255] = {0};

                        pVal = saAmfCompTerminateCmdVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCLEANUPCMD:
                    {	
                        ClCharT saAmfCompCleanupCmdVal[255] = {0};

                        pVal = saAmfCompCleanupCmdVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPAMSTARTCMD:
                    {	
                        ClCharT saAmfCompAmStartCmdVal[255] = {0};

                        pVal = saAmfCompAmStartCmdVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPAMSTOPCMD:
                    {	
                        ClCharT saAmfCompAmStopCmdVal[255] = {0};

                        pVal = saAmfCompAmStopCmdVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPINSTANTIATIONLEVEL:
                    {	
                        ClUint32T   saAmfCompInstantiationLevelVal = 0;
                        
                        pVal = &saAmfCompInstantiationLevelVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPDEFAULTCLCCLITIMEOUT:
                    {	
                        ClCharT saAmfCompDefaultClcCliTimeOutVal[8] = {0};

                        pVal = saAmfCompDefaultClcCliTimeOutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPDEFAULTCALLBACKTIMEOUT:
                    {	
                        ClCharT saAmfCompDefaultCallbackTimeOutVal[8] = {0};

                        pVal = saAmfCompDefaultCallbackTimeOutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPINSTANTIATETIMEOUT:
                    {	
                        ClCharT saAmfCompInstantiateTimeoutVal[8] = {0};

                        pVal = saAmfCompInstantiateTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPDELAYBETWEENINSTANTIATEATTEMPTS:
                    {	
                        ClCharT saAmfCompDelayBetweenInstantiateAttemptsVal[8] = {0};

                        pVal = saAmfCompDelayBetweenInstantiateAttemptsVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPTERMINATETIMEOUT:
                    {	
                        ClCharT saAmfCompTerminateTimeoutVal[8] = {0};

                        pVal = saAmfCompTerminateTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCLEANUPTIMEOUT:
                    {	
                        ClCharT saAmfCompCleanupTimeoutVal[8] = {0};

                        pVal = saAmfCompCleanupTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPAMSTARTTIMEOUT:
                    {	
                        ClCharT saAmfCompAmStartTimeoutVal[8] = {0};

                        pVal = saAmfCompAmStartTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPAMSTOPTIMEOUT:
                    {	
                        ClCharT saAmfCompAmStopTimeoutVal[8] = {0};

                        pVal = saAmfCompAmStopTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPTERMINATECALLBACKTIMEOUT:
                    {	
                        ClCharT saAmfCompTerminateCallbackTimeOutVal[8] = {0};

                        pVal = saAmfCompTerminateCallbackTimeOutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCSISETCALLBACKTIMEOUT:
                    {	
                        ClCharT saAmfCompCSISetCallbackTimeoutVal[8] = {0};

                        pVal = saAmfCompCSISetCallbackTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPQUIESCINGCOMPLETETIMEOUT:
                    {	
                        ClCharT saAmfCompQuiescingCompleteTimeoutVal[8] = {0};

                        pVal = saAmfCompQuiescingCompleteTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCSIRMVCALLBACKTIMEOUT:
                    {	
                        ClCharT saAmfCompCSIRmvCallbackTimeoutVal[8] = {0};

                        pVal = saAmfCompCSIRmvCallbackTimeoutVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPRECOVERYONERROR:
                    {	
                        ClInt32T    saAmfCompRecoveryOnErrorVal = 0;
                        pVal = &saAmfCompRecoveryOnErrorVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHOUTDELAY:
                    {	
                        ClUint32T   saAmfCompNumMaxInstantiateWithoutDelayVal = 0;
                        
                        pVal = &saAmfCompNumMaxInstantiateWithoutDelayVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPNUMMAXINSTANTIATEWITHDELAY:
                    {	
                        ClUint32T   saAmfCompNumMaxInstantiateWithDelayVal = 0;
                        
                        pVal = &saAmfCompNumMaxInstantiateWithDelayVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPNUMMAXAMSTARTSTOPATTEMPTS:
                    {	
                        ClUint32T   saAmfCompNumMaxAmStartStopAttemptsVal = 0;
                        
                        pVal = &saAmfCompNumMaxAmStartStopAttemptsVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPDISABLERESTART:
                    {	
                        ClInt32T    saAmfCompDisableRestartVal = 0;
                        pVal = &saAmfCompDisableRestartVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPNUMMAXACTIVECSI:
                    {	
                        ClUint32T   saAmfCompNumMaxActiveCsiVal = 0;
                        
                        pVal = &saAmfCompNumMaxActiveCsiVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPNUMMAXSTANDBYCSI:
                    {	
                        ClUint32T   saAmfCompNumMaxStandbyCsiVal = 0;
                        
                        pVal = &saAmfCompNumMaxStandbyCsiVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPPROXYCSI:
                    {	
                        ClCharT saAmfCompProxyCSIVal[255] = {0};

                        pVal = saAmfCompProxyCSIVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPRESTARTSTATE:
                    {	
                        ClInt32T    saAmfCompRestartStateVal = 0;
                        pVal = &saAmfCompRestartStateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPEAMTRIGGER:
                    {	
                        ClInt32T    saAmfCompEamTriggerVal = 0;
                        pVal = &saAmfCompEamTriggerVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_SAAMFCOMPROWSTATUS:
                    {	
                        ClInt32T    saAmfCompRowStatusVal = 0;
                        pVal = &saAmfCompRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCOMPROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCOMPROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCompTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCompCSTypeSupportedTable table by defining its contents and how it's structured */
void
clSnmpsaAmfCompCSTypeSupportedTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCompCSTypeSupportedTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCompCSTypeSupportedTable",     clSnmpsaAmfCompCSTypeSupportedTableHandler,
              saAmfCompCSTypeSupportedTableOid, OID_LENGTH(saAmfCompCSTypeSupportedTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCompCSTypeSupportedTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCompCSTypeSupportedCompNameId */
                           ASN_UNSIGNED,  /* index: saAmfCompCSTypeSupportedCSTypeNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfCompCSTypeSupportedTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCompCSTypeSupportedTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCompCSTypeSupportedTableInitialize",
                "Registering table saAmfCompCSTypeSupportedTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCompCSTypeSupportedTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCOMPCSTYPESUPPORTEDTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCompCSTypeSupportedTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCOMPCSTYPESUPPORTEDTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCompCSTypeSupportedTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCOMPCSTYPESUPPORTEDTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCompCSTypeSupportedTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCompNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCompCSTypeSupportedTableInfo.saAmfCompCSTypeSupportedCSTypeNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCOMPNAMEID:
                    {
                        ClUint32T   saAmfCompCSTypeSupportedCompNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompCSTypeSupportedCompNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCSTYPENAMEID:
                    {
                        ClUint32T   saAmfCompCSTypeSupportedCSTypeNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCompCSTypeSupportedCSTypeNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCOMPNAME:
                    {
                        ClCharT saAmfCompCSTypeSupportedCompNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompCSTypeSupportedCompNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCSINAME:
                    {
                        ClCharT saAmfCompCSTypeSupportedCSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCompCSTypeSupportedCSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDROWSTATUS:
                    {
                        ClInt32T    saAmfCompCSTypeSupportedRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCompCSTypeSupportedRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCOMPNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCOMPNAME:
                    {	
                        ClCharT saAmfCompCSTypeSupportedCompNameVal[255] = {0};

                        pVal = saAmfCompCSTypeSupportedCompNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDCSINAME:
                    {	
                        ClCharT saAmfCompCSTypeSupportedCSINameVal[255] = {0};

                        pVal = saAmfCompCSTypeSupportedCSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCOMPCSTYPESUPPORTEDROWSTATUS:
                    {	
                        ClInt32T    saAmfCompCSTypeSupportedRowStatusVal = 0;
                        pVal = &saAmfCompCSTypeSupportedRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCOMPCSTYPESUPPORTEDROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCompCSTypeSupportedTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCompCSTypeSupportedTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCSITable table by defining its contents and how it's structured */
void
clSnmpsaAmfCSITableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCSITableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCSITable",     clSnmpsaAmfCSITableHandler,
              saAmfCSITableOid, OID_LENGTH(saAmfCSITableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCSITableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCSINameId */
                           0);


    table_info->min_column = 2;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmpsaAmfCSITableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCSITableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCSITableInitialize",
                "Registering table saAmfCSITable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

    {
	    ClUint8T * indexOidList[] = {
                                        (ClUint8T *) "1.3.6.1.4.1.18568.2.2.2.1.2.12.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, saAmfCSITableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCSITableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSITABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCSITableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSITABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCSITableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCSITABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCSITableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSITableInfo.saAmfCSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCSINAMEID:
                    {
                        ClUint32T   saAmfCSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAME:
                    {
                        ClCharT saAmfCSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPE:
                    {
                        ClCharT saAmfCSTypeVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSTypeVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSIROWSTATUS:
                    {
                        ClInt32T    saAmfCSIRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCSIRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPE:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSIROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCSIROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCSIROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCSIROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCSIROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCSIROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCSIROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSINAME:
                    {	
                        ClCharT saAmfCSINameVal[255] = {0};

                        pVal = saAmfCSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSTYPE:
                    {	
                        ClCharT saAmfCSTypeVal[255] = {0};

                        pVal = saAmfCSTypeVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSIROWSTATUS:
                    {	
                        ClInt32T    saAmfCSIRowStatusVal = 0;
                        pVal = &saAmfCSIRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCSIROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCSIROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSITableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCSITableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCSICSIDepTable table by defining its contents and how it's structured */
void
clSnmpsaAmfCSICSIDepTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCSICSIDepTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCSICSIDepTable",     clSnmpsaAmfCSICSIDepTableHandler,
              saAmfCSICSIDepTableOid, OID_LENGTH(saAmfCSICSIDepTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCSICSIDepTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCSICSIDepCSINameId */
                           ASN_UNSIGNED,  /* index: saAmfCSICSIDepDepndCSINameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfCSICSIDepTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCSICSIDepTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCSICSIDepTableInitialize",
                "Registering table saAmfCSICSIDepTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCSICSIDepTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSICSIDEPTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCSICSIDepTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSICSIDEPTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCSICSIDepTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCSICSIDEPTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCSICSIDepTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepCSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSICSIDepTableInfo.saAmfCSICSIDepDepndCSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCSICSIDEPCSINAMEID:
                    {
                        ClUint32T   saAmfCSICSIDepCSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSICSIDepCSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPDEPNDCSINAMEID:
                    {
                        ClUint32T   saAmfCSICSIDepDepndCSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSICSIDepDepndCSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPCSINAME:
                    {
                        ClCharT saAmfCSICSIDepCSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSICSIDepCSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPDEPNDCSINAME:
                    {
                        ClCharT saAmfCSICSIDepDepndCSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSICSIDepDepndCSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPROWSTATUS:
                    {
                        ClInt32T    saAmfCSICSIDepRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCSICSIDepRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSICSIDEPCSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPDEPNDCSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSICSIDEPROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCSICSIDEPROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCSICSIDEPROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCSICSIDEPROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCSICSIDEPROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCSICSIDEPROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCSICSIDEPROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSICSIDEPCSINAME:
                    {	
                        ClCharT saAmfCSICSIDepCSINameVal[255] = {0};

                        pVal = saAmfCSICSIDepCSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSICSIDEPDEPNDCSINAME:
                    {	
                        ClCharT saAmfCSICSIDepDepndCSINameVal[255] = {0};

                        pVal = saAmfCSICSIDepDepndCSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSICSIDEPROWSTATUS:
                    {	
                        ClInt32T    saAmfCSICSIDepRowStatusVal = 0;
                        pVal = &saAmfCSICSIDepRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCSICSIDEPROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCSICSIDEPROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSICSIDepTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCSICSIDepTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCSINameValueTable table by defining its contents and how it's structured */
void
clSnmpsaAmfCSINameValueTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCSINameValueTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCSINameValueTable",     clSnmpsaAmfCSINameValueTableHandler,
              saAmfCSINameValueTableOid, OID_LENGTH(saAmfCSINameValueTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCSINameValueTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCSINameValueCSINameId */
                           ASN_UNSIGNED,  /* index: saAmfCSINameValueAttrNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 6;
    
    iinfo->get_first_data_point = clSnmpsaAmfCSINameValueTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCSINameValueTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCSINameValueTableInitialize",
                "Registering table saAmfCSINameValueTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCSINameValueTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSINAMEVALUETABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCSINameValueTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSINAMEVALUETABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCSINameValueTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCSINAMEVALUETABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCSINameValueTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSINameValueTableInfo.saAmfCSINameValueCSINameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSINameValueTableInfo.saAmfCSINameValueAttrNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCSINAMEVALUECSINAMEID:
                    {
                        ClUint32T   saAmfCSINameValueCSINameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSINameValueCSINameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEATTRNAMEID:
                    {
                        ClUint32T   saAmfCSINameValueAttrNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSINameValueAttrNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUECSINAME:
                    {
                        ClCharT saAmfCSINameValueCSINameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSINameValueCSINameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEATTRNAME:
                    {
                        ClCharT saAmfCSINameValueAttrNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSINameValueAttrNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEATTRVALUE:
                    {
                        ClCharT saAmfCSINameValueAttrValueVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSINameValueAttrValueVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEROWSTATUS:
                    {
                        ClInt32T    saAmfCSINameValueRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCSINameValueRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSINAMEVALUECSINAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEATTRNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEATTRVALUE:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSINAMEVALUEROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCSINAMEVALUEROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCSINAMEVALUEROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCSINAMEVALUEROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCSINAMEVALUEROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCSINAMEVALUEROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCSINAMEVALUEROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSINAMEVALUECSINAME:
                    {	
                        ClCharT saAmfCSINameValueCSINameVal[255] = {0};

                        pVal = saAmfCSINameValueCSINameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSINAMEVALUEATTRNAME:
                    {	
                        ClCharT saAmfCSINameValueAttrNameVal[255] = {0};

                        pVal = saAmfCSINameValueAttrNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSINAMEVALUEATTRVALUE:
                    {	
                        ClCharT saAmfCSINameValueAttrValueVal[255] = {0};

                        pVal = saAmfCSINameValueAttrValueVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSINAMEVALUEROWSTATUS:
                    {	
                        ClInt32T    saAmfCSINameValueRowStatusVal = 0;
                        pVal = &saAmfCSINameValueRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCSINAMEVALUEROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCSINAMEVALUEROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSINameValueTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCSINameValueTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfCSTypeAttrNameTable table by defining its contents and how it's structured */
void
clSnmpsaAmfCSTypeAttrNameTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfCSTypeAttrNameTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfCSTypeAttrNameTable",     clSnmpsaAmfCSTypeAttrNameTableHandler,
              saAmfCSTypeAttrNameTableOid, OID_LENGTH(saAmfCSTypeAttrNameTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfCSTypeAttrNameTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfCSTypeNameId */
                           ASN_UNSIGNED,  /* index: saAmfCSTypeAttrNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfCSTypeAttrNameTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfCSTypeAttrNameTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfCSTypeAttrNameTableInitialize",
                "Registering table saAmfCSTypeAttrNameTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfCSTypeAttrNameTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSTYPEATTRNAMETABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfCSTypeAttrNameTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFCSTYPEATTRNAMETABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfCSTypeAttrNameTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFCSTYPEATTRNAMETABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfCSTypeAttrNameTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfCSTypeAttrNameTableInfo.saAmfCSTypeAttrNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFCSTYPENAMEID:
                    {
                        ClUint32T   saAmfCSTypeNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSTypeNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPEATTRNAMEID:
                    {
                        ClUint32T   saAmfCSTypeAttrNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfCSTypeAttrNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPENAME:
                    {
                        ClCharT saAmfCSTypeNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSTypeNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPEATTRNAME:
                    {
                        ClCharT saAmfCSTypeAttrNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfCSTypeAttrNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPEATTRROWSTATUS:
                    {
                        ClInt32T    saAmfCSTypeAttrRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfCSTypeAttrRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSTYPENAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPEATTRNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFCSTYPEATTRROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFCSTYPEATTRROWSTATUS_ACTIVE:
                                break;
                            case SAAMFCSTYPEATTRROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFCSTYPEATTRROWSTATUS_NOTREADY:
                                break;
                            case SAAMFCSTYPEATTRROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFCSTYPEATTRROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFCSTYPEATTRROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFCSTYPENAME:
                    {	
                        ClCharT saAmfCSTypeNameVal[255] = {0};

                        pVal = saAmfCSTypeNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSTYPEATTRNAME:
                    {	
                        ClCharT saAmfCSTypeAttrNameVal[255] = {0};

                        pVal = saAmfCSTypeAttrNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFCSTYPEATTRROWSTATUS:
                    {	
                        ClInt32T    saAmfCSTypeAttrRowStatusVal = 0;
                        pVal = &saAmfCSTypeAttrRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFCSTYPEATTRROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFCSTYPEATTRROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfCSTypeAttrNameTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfCSTypeAttrNameTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSUSITable table by defining its contents and how it's structured */
void
clSnmpsaAmfSUSITableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSUSITableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSUSITable",     clSnmpsaAmfSUSITableHandler,
              saAmfSUSITableOid, OID_LENGTH(saAmfSUSITableOid),
              HANDLER_CAN_RONLY
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSUSITableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSUSISuNameId */
                           ASN_UNSIGNED,  /* index: saAmfSUSISiNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfSUSITableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSUSITableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSUSITableInitialize",
                "Registering table saAmfSUSITable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);


}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSUSITableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUSITABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSUSITableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSUSITABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSUSITableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSUSITABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSUSITableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSUSITableInfo.saAmfSUSISuNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSUSITableInfo.saAmfSUSISiNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSUSISUNAMEID:
                    {
                        ClUint32T   saAmfSUSISuNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUSISuNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSISINAMEID:
                    {
                        ClUint32T   saAmfSUSISiNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSUSISiNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSISUNAME:
                    {
                        ClCharT saAmfSUSISuNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUSISuNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSISINAME:
                    {
                        ClCharT saAmfSUSISiNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSUSISiNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSUSIHASTATE:
                    {
                        ClInt32T    saAmfSUSIHAStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSUSIHAStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSUSITableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSUSITableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfHealthCheckTable table by defining its contents and how it's structured */
void
clSnmpsaAmfHealthCheckTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfHealthCheckTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfHealthCheckTable",     clSnmpsaAmfHealthCheckTableHandler,
              saAmfHealthCheckTableOid, OID_LENGTH(saAmfHealthCheckTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfHealthCheckTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfHealthCompNameId */
                           ASN_OCTET_STR,  /* index: saAmfHealthCheckKey */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 6;
    
    iinfo->get_first_data_point = clSnmpsaAmfHealthCheckTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfHealthCheckTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfHealthCheckTableInitialize",
                "Registering table saAmfHealthCheckTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);

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
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfHealthCheckTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFHEALTHCHECKTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfHealthCheckTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFHEALTHCHECKTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfHealthCheckTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFHEALTHCHECKTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfHealthCheckTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfHealthCheckTableInfo.saAmfHealthCompNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof (requestInfo.index.saAmfHealthCheckTableInfo.saAmfHealthCheckKey))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfHealthCheckTableInfo.saAmfHealthCheckKey), (void *)index->val.integer, index->val_len);
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFHEALTHCOMPNAMEID:
                    {
                        ClUint32T   saAmfHealthCompNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfHealthCompNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKKEY:
                    {
                        ClCharT saAmfHealthCheckKeyVal[33] = {0};

                        requestInfo.dataLen = 32 * sizeof(ClCharT);
                        pVal = saAmfHealthCheckKeyVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCOMPNAME:
                    {
                        ClCharT saAmfHealthCompNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfHealthCompNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKPERIOD:
                    {
                        ClCharT saAmfHealthCheckPeriodVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfHealthCheckPeriodVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKMAXDURATION:
                    {
                        ClCharT saAmfHealthCheckMaxDurationVal[9] = {0};

                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                        pVal = saAmfHealthCheckMaxDurationVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKROWSTATUS:
                    {
                        ClInt32T    saAmfHealthCheckRowStatusVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfHealthCheckRowStatusVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFHEALTHCOMPNAME:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 255);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKPERIOD:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKMAXDURATION:
                    {
                        columnType = ASN_OCTET_STR;

                        retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }

                        break;
                    }
                    case COLUMN_SAAMFHEALTHCHECKROWSTATUS:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case SAAMFHEALTHCHECKROWSTATUS_ACTIVE:
                                break;
                            case SAAMFHEALTHCHECKROWSTATUS_NOTINSERVICE:
                                break;
                            case SAAMFHEALTHCHECKROWSTATUS_NOTREADY:
                                break;
                            case SAAMFHEALTHCHECKROWSTATUS_CREATEANDGO:
                                break;
                            case SAAMFHEALTHCHECKROWSTATUS_CREATEANDWAIT:
                                break;
                            case SAAMFHEALTHCHECKROWSTATUS_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    case COLUMN_SAAMFHEALTHCOMPNAME:
                    {	
                        ClCharT saAmfHealthCompNameVal[255] = {0};

                        pVal = saAmfHealthCompNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFHEALTHCHECKPERIOD:
                    {	
                        ClCharT saAmfHealthCheckPeriodVal[8] = {0};

                        pVal = saAmfHealthCheckPeriodVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFHEALTHCHECKMAXDURATION:
                    {	
                        ClCharT saAmfHealthCheckMaxDurationVal[8] = {0};

                        pVal = saAmfHealthCheckMaxDurationVal;
                        requestInfo.dataLen = 8 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_SAAMFHEALTHCHECKROWSTATUS:
                    {	
                        ClInt32T    saAmfHealthCheckRowStatusVal = 0;
                        pVal = &saAmfHealthCheckRowStatusVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == SAAMFHEALTHCHECKROWSTATUS_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == SAAMFHEALTHCHECKROWSTATUS_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfHealthCheckTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfHealthCheckTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfSCompCsiTable table by defining its contents and how it's structured */
void
clSnmpsaAmfSCompCsiTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfSCompCsiTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfSCompCsiTable",     clSnmpsaAmfSCompCsiTableHandler,
              saAmfSCompCsiTableOid, OID_LENGTH(saAmfSCompCsiTableOid),
              HANDLER_CAN_RONLY
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfSCompCsiTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfSCompCsiCompNameId */
                           ASN_UNSIGNED,  /* index: saAmfSCompCsiCsiNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 5;
    
    iinfo->get_first_data_point = clSnmpsaAmfSCompCsiTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfSCompCsiTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfSCompCsiTableInitialize",
                "Registering table saAmfSCompCsiTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);


}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfSCompCsiTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSCOMPCSITABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfSCompCsiTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFSCOMPCSITABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfSCompCsiTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFSCOMPCSITABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfSCompCsiTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSCompCsiTableInfo.saAmfSCompCsiCompNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfSCompCsiTableInfo.saAmfSCompCsiCsiNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFSCOMPCSICOMPNAMEID:
                    {
                        ClUint32T   saAmfSCompCsiCompNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSCompCsiCompNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSCOMPCSICSINAMEID:
                    {
                        ClUint32T   saAmfSCompCsiCsiNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfSCompCsiCsiNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSCOMPCSICOMPNAME:
                    {
                        ClCharT saAmfSCompCsiCompNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSCompCsiCompNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSCOMPCSICSINAME:
                    {
                        ClCharT saAmfSCompCsiCsiNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfSCompCsiCsiNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFSCOMPCSIHASTATE:
                    {
                        ClInt32T    saAmfSCompCsiHAStateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &saAmfSCompCsiHAStateVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfSCompCsiTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfSCompCsiTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the saAmfProxyProxiedTable table by defining its contents and how it's structured */
void
clSnmpsaAmfProxyProxiedTableInitialize(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;
    netsnmp_iterator_info           *iinfo = NULL;
    netsnmp_table_registration_info *table_info = NULL;;


    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->flags |= NETSNMP_ITERATOR_FLAG_SORTED;

    if (!table_info || !iinfo) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpsaAmfProxyProxiedTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "saAmfProxyProxiedTable",     clSnmpsaAmfProxyProxiedTableHandler,
              saAmfProxyProxiedTableOid, OID_LENGTH(saAmfProxyProxiedTableOid),
              HANDLER_CAN_RONLY
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpsaAmfProxyProxiedTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: saAmfProxyProxiedProxyNameId */
                           ASN_UNSIGNED,  /* index: saAmfProxyProxiedProxiedNameId */
                           0);


    table_info->min_column = 3;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmpsaAmfProxyProxiedTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpsaAmfProxyProxiedTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpsaAmfProxyProxiedTableInitialize",
                "Registering table saAmfProxyProxiedTable as a table iterator\n"));
    netsnmp_register_table_iterator( reg, iinfo );

    /**************************************************
     * Registering the Mode End Callback.
     */
    callback_handler = netsnmp_get_mode_end_call_handler(
                        netsnmp_mode_end_call_add_mode_callback(NULL,
                        NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("mode_end_callback_handler",
                                                   clSnmpModeEndCallback)
                        ));

    netsnmp_inject_handler( reg, callback_handler);


}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpsaAmfProxyProxiedTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFPROXYPROXIEDTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpsaAmfProxyProxiedTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_SAAMFPROXYPROXIEDTABLE, CL_SNMP_GET_NEXT);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
        if(my_loop_context)
            *(ClInt32T*)*my_loop_context = 1; /* No more rows in table */
    }

    return put_index_data; 
}


int
clSnmpsaAmfProxyProxiedTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_SAAMFPROXYPROXIEDTABLE;

    for (request = requests; request; request = request->next) 
    {
        noOfOp++;
    }

    for (request = requests; request; request = request->next) 
    {
        netsnmp_variable_list * reqVar = request->requestvb;
        netsnmp_table_request_info *table_info = NULL;
        netsnmp_variable_list * index = NULL;

        if (request->processed != 0)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "The job is already processed, skipping this job.");
            continue;
        }

       if ((netsnmp_extract_iterator_context(request) == NULL) && (reqinfo->mode == MODE_GET))
       {
            netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHINSTANCE);
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "No iterator context for the GET job, skipping it.");
            continue;
       }

        if((table_info = netsnmp_extract_table_info(request)) == NULL)
        {
            clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX,
                    "Not able to find the table_info for this job, skipping it.");
            continue;
        }
        
        /* COPY COLUMN OID FROM reqVar->name TO requestInfo.oid */
        /* oidLen is tableoid length + depth till attribute oid. So, in this case depth till column will be 2. 
            1-entry oid of table
            2-actual column. */
        requestInfo.oidLen = OID_LENGTH (saAmfProxyProxiedTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxyNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;

        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.saAmfProxyProxiedTableInfo.saAmfProxyProxiedProxiedNameId), (void *)index->val.integer, sizeof(ClUint32T));
        }
        index = index->next_variable;


        switch (reqinfo->mode) 
        {
            case MODE_GET:
            {
                ClInt32T retVal = 0, errorCode = 0;
                void * pVal = NULL;
                _ClSnmpGetReqInfoT reqAddInfo = {0};
                clLogDebug("SUB", "TBL",
                "Received GET request");
                requestInfo.opCode = CL_SNMP_GET;
                reqAddInfo.pRequest = request;
                switch (table_info->colnum)
                {

                    /*@foreach  nonindex@*/
                    case COLUMN_SAAMFPROXYPROXIEDPROXYNAMEID:
                    {
                        ClUint32T   saAmfProxyProxiedProxyNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfProxyProxiedProxyNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFPROXYPROXIEDPROXIEDNAMEID:
                    {
                        ClUint32T   saAmfProxyProxiedProxiedNameIdVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &saAmfProxyProxiedProxiedNameIdVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFPROXYPROXIEDPROXYNAME:
                    {
                        ClCharT saAmfProxyProxiedProxyNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfProxyProxiedProxyNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_SAAMFPROXYPROXIEDPROXIEDNAME:
                    {
                        ClCharT saAmfProxyProxiedProxiedNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = saAmfProxyProxiedProxiedNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }

                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableHandler: Unknown column for GET request");
                        return SNMP_ERR_GENERR;
                }
                break;
            }
            case MODE_SET_RESERVE1:
            {
                ClInt32T retVal = 0;
                ClUint32T columnType = ASN_INTEGER;

                clLogDebug("SUB", "TBL",
                "Received RESERVE1 request");
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableHandler: unknown column");
                        netsnmp_request_set_error(request, SNMP_ERR_NOSUCHNAME);
                        break;
                }
                retVal = netsnmp_check_vb_type(reqVar, columnType);
                if(retVal != SNMP_ERR_NOERROR ) 
                {
                    netsnmp_request_set_error(request, retVal );
                    return retVal;
                }
                break;
            }
            case MODE_SET_FREE:
                clLogDebug("SUB", "TBL",
                "Received SET FREE request");
                clSnmpUndo();
                break;
  
            case MODE_SET_RESERVE2:
            {
                ClInt32T retVal = 0, errorCode = 0;
		        ClPtrT pVal = NULL;
                clLogDebug("SUB", "TBL",
                "Received RESERVE2 request");
                requestInfo.opCode = CL_SNMP_SET;
                switch(table_info->colnum)
                {
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpsaAmfProxyProxiedTableHandler: unknown column");
                        break;
                }

                memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

                clLogDebug("SUB", "TBL",
                        "Calling request add");

                retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
                if(CL_OK != retVal)
                {
                    clLogError("SUB", "TBL",
                            "Failed to add request in store, rc=[0x%x], errorCode=[0x%x]", 
                            retVal, errorCode);
                    netsnmp_request_set_error(request, errorCode);
                    return retVal;
                }
                clLogDebug("SUB", "TBL",
                        "Successfully added request in store");
                break;
            }
            case MODE_SET_ACTION:
            {
                ClInt32T    retVal = 0;
                ClMedErrorListT    medErrList = {0};

                clLogDebug("SUB", "TBL",
                        "Received ACTION request for oid [%s]",requestInfo.oid);
                retVal = clSnmpCommit(&medErrList);
                if (retVal != CL_OK)
                {
                    clLogDebug("SUB", "TBL",
                            "Error in commit errCount[%d]",
                            medErrList.count);
                    if(retVal == CL_ERR_NO_MEMORY)
                    {
                        retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                        clLogError("SUB", "TBL",
                                "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                retVal, requestInfo.oid);
                        netsnmp_request_set_error(request, retVal);
                        return retVal;
                    }
                    else
                    {
                        
                        ClUint32T   i;
                        for(i = 0; i < medErrList.count; i++)
                        {
                            /* the oid matches the request, set error and return*/
                            if(!memcmp((ClUint8T*)requestInfo.oid, 
                                        medErrList.pErrorList[i].oidInfo.id, medErrList.pErrorList[i].oidInfo.len)) 
                            {
                                clLogError("SUB", "TBL",
                                        "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                        medErrList.pErrorList[i].errId, requestInfo.oid);
                                netsnmp_request_set_error(request, medErrList.pErrorList[i].errId);
                                return retVal;

                            }
                            
                        }
                    }
                }     
                break;
            }
            case MODE_SET_COMMIT:
            {
                clLogDebug("SUB", "TBL",
                        "Received COMMIT request");
                clSnmpUndo();
                break;
            }
            case MODE_SET_UNDO:
            {
                clLogDebug("SUB", "TBL",
                        "Received UNDO request");
                clSnmpUndo();
                break;
            }
            default:
                   /* we should never get here, so this is a really bad error */
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpsaAmfProxyProxiedTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
