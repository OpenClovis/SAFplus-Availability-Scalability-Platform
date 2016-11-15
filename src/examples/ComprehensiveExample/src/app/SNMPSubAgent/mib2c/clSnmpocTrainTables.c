
#include <pthread.h>
#include <clLogApi.h>
#include <clSnmpLog.h>
#include <clDebugApi.h>
#include <clSnmpocTrainTables.h>
#include <clSnmpocTrainUtil.h>
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

static oid clockTableOid[] = { 1,3,6,1,4,1,103,2,1 };
static oid timeSetTableOid[] = { 1,3,6,1,4,1,103,3,1 };
static oid nameTableOid[] = { 1,3,6,1,4,1,103,4,1 };


ClSnmpTableIndexCorAttrIdInfoT clockTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index clockRow */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT timeSetTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index timeSetRow */
                                                                  { NULL, -1}
                                                                 };
ClSnmpTableIndexCorAttrIdInfoT nameTableIndexCorAttrIdList[1+1] = {
                                                                  { NULL, 0}, /* COR attribute ID for index nodeAdd */
                                                                  { NULL, -1}
                                                                 };

/** Initializes the ocTrain module */
void
clSnmpocTrainTablesInit(void)
{
  /* here we initialize all the tables we're planning on supporting */
  clSnmpclockTableInitialize();
  clSnmptimeSetTableInitialize();
  clSnmpnameTableInitialize();
}

void clSnmpocTrainTablesDeInit(void)
{
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Unloading ocTrainTables"); 
}

/** Initialize the clockTable table by defining its contents and how it's structured */
void
clSnmpclockTableInitialize(void)
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
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpclockTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "clockTable",     clSnmpclockTableHandler,
              clockTableOid, OID_LENGTH(clockTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpclockTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_INTEGER,  /* index: clockRow */
                           0);


    table_info->min_column = 1;
    table_info->max_column = 9;
    
    iinfo->get_first_data_point = clSnmpclockTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpclockTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpclockTableInitialize",
                "Registering table clockTable as a table iterator\n"));
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
clSnmpclockTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_CLOCKTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpclockTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_CLOCKTABLE, CL_SNMP_GET_NEXT);
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
clSnmpclockTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_CLOCKTABLE;

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
        requestInfo.oidLen = OID_LENGTH (clockTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.clockTableInfo.clockRow), (void *)index->val.integer, sizeof(ClInt32T));
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
                    case COLUMN_CLOCKROW:
                    {
                        ClInt32T    clockRowVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &clockRowVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_CLOCKID:
                    {
                        ClInt32T    clockIdVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &clockIdVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_CLOCKHOUR:
                    {
                        ClInt32T    clockHourVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &clockHourVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_CLOCKMINUTE:
                    {
                        ClInt32T    clockMinuteVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &clockMinuteVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_CLOCKSECOND:
                    {
                        ClInt32T    clockSecondVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &clockSecondVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_ALARMHOUR:
                    {
                        ClInt32T    alarmHourVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &alarmHourVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_ALARMMINUTE:
                    {
                        ClInt32T    alarmMinuteVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &alarmMinuteVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_ALARMREACTION:
                    {
                        ClInt32T    alarmReactionVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &alarmReactionVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_ALARMSET:
                    {
                        ClInt32T    alarmSetVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &alarmSetVal;

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
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableHandler: Unknown column for GET request");
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
                    case COLUMN_CLOCKROW:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 256);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_CLOCKID:
                    {
                        columnType = ASN_INTEGER;
                        break;
                    }
                    case COLUMN_ALARMHOUR:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 23);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_ALARMMINUTE:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 59);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_ALARMREACTION:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 1);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_ALARMSET:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 1);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableHandler: unknown column");
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
                    case COLUMN_CLOCKROW:
                    {	
                        ClInt32T    clockRowVal = 0;
                        pVal = &clockRowVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_CLOCKID:
                    {	
                        ClInt32T    clockIdVal = 0;
                        pVal = &clockIdVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_ALARMHOUR:
                    {	
                        ClInt32T    alarmHourVal = 0;
                        pVal = &alarmHourVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_ALARMMINUTE:
                    {	
                        ClInt32T    alarmMinuteVal = 0;
                        pVal = &alarmMinuteVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_ALARMREACTION:
                    {	
                        ClInt32T    alarmReactionVal = 0;
                        pVal = &alarmReactionVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_ALARMSET:
                    {	
                        ClInt32T    alarmSetVal = 0;
                        pVal = &alarmSetVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpclockTableHandler: unknown column");
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
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpclockTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the timeSetTable table by defining its contents and how it's structured */
void
clSnmptimeSetTableInitialize(void)
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
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmptimeSetTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "timeSetTable",     clSnmptimeSetTableHandler,
              timeSetTableOid, OID_LENGTH(timeSetTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmptimeSetTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_INTEGER,  /* index: timeSetRow */
                           0);


    table_info->min_column = 1;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmptimeSetTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmptimeSetTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmptimeSetTableInitialize",
                "Registering table timeSetTable as a table iterator\n"));
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
clSnmptimeSetTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_TIMESETTABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmptimeSetTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_TIMESETTABLE, CL_SNMP_GET_NEXT);
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
clSnmptimeSetTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_TIMESETTABLE;

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
        requestInfo.oidLen = OID_LENGTH (timeSetTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.timeSetTableInfo.timeSetRow), (void *)index->val.integer, sizeof(ClInt32T));
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
                    case COLUMN_TIMESETROW:
                    {
                        ClInt32T    timeSetRowVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &timeSetRowVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_TIMESETHOUR:
                    {
                        ClInt32T    timeSetHourVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &timeSetHourVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_TIMESETMINUTE:
                    {
                        ClInt32T    timeSetMinuteVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &timeSetMinuteVal;

                        reqAddInfo.columnType = ASN_INTEGER;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_TIMESETSECOND:
                    {
                        ClInt32T    timeSetSecondVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &timeSetSecondVal;

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
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableHandler: Unknown column for GET request");
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
                    case COLUMN_TIMESETROW:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 256);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_TIMESETHOUR:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 23);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_TIMESETMINUTE:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 59);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    case COLUMN_TIMESETSECOND:
                    {
                        columnType = ASN_INTEGER;

                        retVal = netsnmp_check_vb_range(reqVar, 0, 59);
                        if(retVal != SNMP_ERR_NOERROR)
                        {
                            netsnmp_request_set_error(request, retVal );
                            return retVal;
                        }
                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableHandler: unknown column");
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
                    case COLUMN_TIMESETROW:
                    {	
                        ClInt32T    timeSetRowVal = 0;
                        pVal = &timeSetRowVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_TIMESETHOUR:
                    {	
                        ClInt32T    timeSetHourVal = 0;
                        pVal = &timeSetHourVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_TIMESETMINUTE:
                    {	
                        ClInt32T    timeSetMinuteVal = 0;
                        pVal = &timeSetMinuteVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    case COLUMN_TIMESETSECOND:
                    {	
                        ClInt32T    timeSetSecondVal = 0;
                        pVal = &timeSetSecondVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmptimeSetTableHandler: unknown column");
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
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmptimeSetTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
/** Initialize the nameTable table by defining its contents and how it's structured */
void
clSnmpnameTableInitialize(void)
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
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "malloc failed to allocate %zd or %zd bytes in clSnmpnameTableInitialize", 
             sizeof(netsnmp_table_registration_info), sizeof(netsnmp_iterator_info)); 
        return;                 /* Serious error. */
    }

    reg = netsnmp_create_handler_registration(
              "nameTable",     clSnmpnameTableHandler,
              nameTableOid, OID_LENGTH(nameTableOid),
              HANDLER_CAN_RWRITE
              );

    if (!reg) {
        SNMP_FREE(table_info);
        SNMP_FREE(iinfo);
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "Creating handler registration failed in clSnmpnameTableInitialize");
        return;                 /* Serious error. */
    }


    /***************************************************
     * Setting up the table's definition
     */
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_UNSIGNED,  /* index: nodeAdd */
                           0);


    table_info->min_column = 1;
    table_info->max_column = 4;
    
    iinfo->get_first_data_point = clSnmpnameTableGetFirstDataPoint;
    iinfo->get_next_data_point  = clSnmpnameTableGetNextDataPoint;
    iinfo->table_reginfo        = table_info;
    

    /***************************************************
     * registering the table with the master agent
     */
    DEBUGMSGTL(("clSnmpnameTableInitialize",
                "Registering table nameTable as a table iterator\n"));
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
                                        (ClUint8T *) "1.3.6.1.4.1.103.4.1.1.1",
                                        NULL /* End of Index OID List */
                                     };
        ClRcT rc = CL_OK;
        rc = clSnmpTableIndexCorAttrIdInit(1, indexOidList, nameTableIndexCorAttrIdList);
        if(rc != CL_OK)
        {
             clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpTableIndexCorAttrIdInit returned error rc = 0x%x",(ClUint32T)rc);
        }
    }

}

/* Iterator hook routines */
netsnmp_variable_list *
clSnmpnameTableGetFirstDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;
    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableGetFirstDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_NAMETABLE, CL_SNMP_GET_FIRST);
    if(rc != CL_OK)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpSendSyncRequestToServer returns error. rc[0x%x]",rc);
        put_index_data = NULL;
    }
    return put_index_data; 
}

netsnmp_variable_list *
clSnmpnameTableGetNextDataPoint(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    ClRcT rc = CL_OK;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableGetNextDataPoint");

    rc = clSnmpSendSyncRequestToServer(my_loop_context, my_data_context, put_index_data, CL_NAMETABLE, CL_SNMP_GET_NEXT);
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
clSnmpnameTableHandler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests)
{
    netsnmp_request_info       *request = NULL;
    ClSNMPRequestInfoT requestInfo;
    ClInt32T noOfOp = 0;

    clLog(CL_LOG_DEBUG, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableHandler");

    if(!requests || !reqinfo)
    {
        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
        return SNMP_ERR_GENERR;
    }

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

    requestInfo.tableType = CL_NAMETABLE;

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
        requestInfo.oidLen = OID_LENGTH (nameTableOid) + 2;
        clSnmpOidCpy(&requestInfo, reqVar->name);

        /* COPY THE INDEX VALUES */

        if(!(table_info->indexes))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            return SNMP_ERR_GENERR;
        }

        index = table_info->indexes;
        if(index->val_len > sizeof(unsigned long))
        {
            clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_GENERR]);
            /* Index value length exceeds specified limit. */
            return SNMP_ERR_GENERR;
        }
        else
        {
            memcpy(& (requestInfo.index.nameTableInfo.nodeAdd), (void *)index->val.integer, sizeof(ClUint32T));
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
                    case COLUMN_NODEADD:
                    {
                        ClUint32T   nodeAddVal = 0;

                        requestInfo.dataLen = sizeof(ClUint32T);
                        pVal = &nodeAddVal;

                        reqAddInfo.columnType = ASN_UNSIGNED;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_NODENAME:
                    {
                        ClCharT nodeNameVal[256] = {0};

                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                        pVal = nodeNameVal;

                        reqAddInfo.columnType = ASN_OCTET_STR;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_NODEIP:
                    {
                        ClUint8T nodeIpVal[4] = {0}; /* IPv4 address length */

                        requestInfo.dataLen = 4 * sizeof(ClUint8T);
                        pVal = nodeIpVal;

                        reqAddInfo.columnType = ASN_IPADDRESS;
                        retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);
                        if(retVal != 0 || errorCode < 0)
                        {
                            netsnmp_request_set_error (request, errorCode);
                            return errorCode;
                        }

                        break;
                    }
                    case COLUMN_NODECREATE:
                    {
                        ClInt32T    nodeCreateVal = 0;

                        requestInfo.dataLen = sizeof(ClInt32T);
                        pVal = &nodeCreateVal;

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
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableHandler: Unknown column for GET request");
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
                    case COLUMN_NODEADD:
                    {
                        columnType = ASN_UNSIGNED;
                        break;
                    }
                    case COLUMN_NODENAME:
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
                    case COLUMN_NODEIP:
                    {
                        columnType = ASN_IPADDRESS;
                        break;
                    }
                    case COLUMN_NODECREATE:
                    {
                        columnType = ASN_INTEGER;
                        switch(*(reqVar->val.integer))
                        {
                            case NODECREATE_ACTIVE:
                                break;
                            case NODECREATE_NOTINSERVICE:
                                break;
                            case NODECREATE_NOTREADY:
                                break;
                            case NODECREATE_CREATEANDGO:
                                break;
                            case NODECREATE_CREATEANDWAIT:
                                break;
                            case NODECREATE_DESTROY:
                                break;
                            /** not a legal enum value.  return an error */
                            default:
                                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "%s",gErrList[SNMP_ERR_WRONGVALUE]);
                                netsnmp_request_set_error(request, SNMP_ERR_WRONGVALUE); /* Set error */
                                return SNMP_ERR_WRONGVALUE;
                        }

                        break;
                    }
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableHandler: unknown column");
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
                    case COLUMN_NODEADD:
                    {	
                        ClUint32T   nodeAddVal = 0;
                        
                        pVal = &nodeAddVal;
                        requestInfo.dataLen = sizeof(ClUint32T); 
                    } 
                        break;
                    case COLUMN_NODENAME:
                    {	
                        ClCharT nodeNameVal[255] = {0};

                        pVal = nodeNameVal;
                        requestInfo.dataLen = 255 * sizeof(ClCharT);
                    } 
                        break;
                    case COLUMN_NODEIP:
                    {	
                        ClUint8T nodeIpVal[4] = {0}; /* IPv4 address length */

                        pVal = nodeIpVal;
                        requestInfo.dataLen = 4 * sizeof(ClUint8T); /* IPv4 Address length */
                    } 
                        break;
                    case COLUMN_NODECREATE:
                    {	
                        ClInt32T    nodeCreateVal = 0;
                        pVal = &nodeCreateVal;
                        requestInfo.dataLen = sizeof(ClInt32T); 
                        if((*(reqVar->val.integer)) == NODECREATE_CREATEANDGO)
                                requestInfo.opCode = CL_SNMP_CREATE;
                        else if((*(reqVar->val.integer)) == NODECREATE_DESTROY)
                                requestInfo.opCode = CL_SNMP_DELETE;
                    } 
                        break;
                    default:
                        clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "clSnmpnameTableHandler: unknown column");
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
                clLog(CL_LOG_ERROR, CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "unknown mode (%d) in clSnmpnameTableHandler", reqinfo->mode);
                return SNMP_ERR_GENERR;
        }
        memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));
    }
    return SNMP_ERR_NOERROR;
}
