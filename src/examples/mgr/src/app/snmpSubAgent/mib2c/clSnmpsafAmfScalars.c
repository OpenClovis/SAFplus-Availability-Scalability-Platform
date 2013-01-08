#include <clDebugApi.h>
#include <clSnmpOp.h>
#include <clSnmpsafAmfScalars.h>

static oid saAmfAgentSpecVersionOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,1 };
static oid saAmfAgentVendorOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,2 };
static oid saAmfAgentVendorProductRevOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,3 };
static oid saAmfServiceStartEnabledOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,4 };
static oid saAmfServiceStateOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,5 };
static oid saAmfClusterStartupTimeoutOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,6 };
static oid saAmfClusterAdminStateOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,7 };
static oid saAmfClusterAdminStateTriggerOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,8 };
static oid saAmfSvcUserNameOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,1,1 };
static oid saAmfProbableCauseOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,1,2 };

/** Initializes the safAmf module */
void
clSnmpsafAmfScalarsInit(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;

    DEBUGMSGTL(("safAmfScalars", "Initializing\n"));

    reg = netsnmp_create_handler_registration("saAmfAgentSpecVersion", clSnmpsaAmfAgentSpecVersionHandler,
                               saAmfAgentSpecVersionOid, OID_LENGTH(saAmfAgentSpecVersionOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfAgentVendor", clSnmpsaAmfAgentVendorHandler,
                               saAmfAgentVendorOid, OID_LENGTH(saAmfAgentVendorOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfAgentVendorProductRev", clSnmpsaAmfAgentVendorProductRevHandler,
                               saAmfAgentVendorProductRevOid, OID_LENGTH(saAmfAgentVendorProductRevOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfServiceStartEnabled", clSnmpsaAmfServiceStartEnabledHandler,
                               saAmfServiceStartEnabledOid, OID_LENGTH(saAmfServiceStartEnabledOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfServiceState", clSnmpsaAmfServiceStateHandler,
                               saAmfServiceStateOid, OID_LENGTH(saAmfServiceStateOid),
                               HANDLER_CAN_RWRITE
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfClusterStartupTimeout", clSnmpsaAmfClusterStartupTimeoutHandler,
                               saAmfClusterStartupTimeoutOid, OID_LENGTH(saAmfClusterStartupTimeoutOid),
                               HANDLER_CAN_RWRITE
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfClusterAdminState", clSnmpsaAmfClusterAdminStateHandler,
                               saAmfClusterAdminStateOid, OID_LENGTH(saAmfClusterAdminStateOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfClusterAdminStateTrigger", clSnmpsaAmfClusterAdminStateTriggerHandler,
                               saAmfClusterAdminStateTriggerOid, OID_LENGTH(saAmfClusterAdminStateTriggerOid),
                               HANDLER_CAN_RWRITE
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfSvcUserName", clSnmpsaAmfSvcUserNameHandler,
                               saAmfSvcUserNameOid, OID_LENGTH(saAmfSvcUserNameOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("saAmfProbableCause", clSnmpsaAmfProbableCauseHandler,
                               saAmfProbableCauseOid, OID_LENGTH(saAmfProbableCauseOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

}

int
clSnmpsaAmfAgentSpecVersionHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfAgentSpecVersionOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClCharT saAmfAgentSpecVersionVal[256] = {0};

            requestInfo.dataLen = 255 * sizeof(ClCharT);
            pVal = saAmfAgentSpecVersionVal;
            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_OCTET_STR;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfAgentSpecVersionHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfAgentVendorHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfAgentVendorOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClCharT saAmfAgentVendorVal[256] = {0};

            requestInfo.dataLen = 255 * sizeof(ClCharT);
            pVal = saAmfAgentVendorVal;
            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_OCTET_STR;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfAgentVendorHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfAgentVendorProductRevHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfAgentVendorProductRevOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfAgentVendorProductRevVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfAgentVendorProductRevVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfAgentVendorProductRevHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfServiceStartEnabledHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfServiceStartEnabledOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfServiceStartEnabledVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfServiceStartEnabledVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfServiceStartEnabledHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfServiceStateHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfServiceStateOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfServiceStateVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfServiceStateVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }

        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
        {
            ClInt32T retVal = 0;
            clLogDebug("SUB", "SCL",
            "Received RESERVE1 request");
            retVal = netsnmp_check_vb_type(reqVar, ASN_INTEGER);
            if(retVal != SNMP_ERR_NOERROR ) 
            {
                netsnmp_request_set_error(requests, retVal );
                return retVal;
            }

            switch(*(reqVar->val.integer))
            {
                case SAAMFSERVICESTATE_RUNNING:
                    break;
                case SAAMFSERVICESTATE_STOPPED:
                    break;
                case SAAMFSERVICESTATE_STARTINGUP:
                    break;
                case SAAMFSERVICESTATE_SHUTTINGDOWN:
                    break;
                case SAAMFSERVICESTATE_UNAVAILABLE:
                    break;
                /** not a legal enum value.  return an error */
                default:
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Inconsistent value."));
                    netsnmp_request_set_error(requests, SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
            }
            break;
        }
        case MODE_SET_RESERVE2:
        {
            ClInt32T retVal = 0, errorCode = 0, noOfOp = 1;
            ClPtrT pVal = NULL;


            ClInt32T    Val = 0;

            pVal = &Val;
            requestInfo.dataLen = sizeof(ClInt32T); 
            requestInfo.opCode = CL_SNMP_SET;
            requestInfo.index.scalarType = 0;

            memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

            clLogDebug("SUB", "SCL",
                    "Calling request add");
            retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
            if(CL_OK != retVal)
            {
                clLogError("SUB", "SCL",
                        "Failed to add request to store, rc=[0x%x], errorCode=[0x%x]", 
                        retVal, errorCode);
                netsnmp_request_set_error(requests, errorCode);
                return retVal;
            }
            clLogDebug("SUB", "SCL",
                    "Successfully added request in store");
            break;
        }

        case MODE_SET_FREE:
        {
            clLogDebug("SUB", "SCL",
            "Received FREE request");
            clSnmpUndo();
            break;
        }

        case MODE_SET_ACTION:
        {
            ClInt32T    retVal = 0;
            ClMedErrorListT    medErrList;

            clLogDebug("SUB", "SCL",
                    "Received ACTION request for oid[%s] ",requestInfo.oid);
            retVal = clSnmpCommit(&medErrList);
            if (retVal != 0)
            {
                clLogDebug("SUB", "SCL",
                        "Error in commit errCount[%d]",
                        medErrList.count);
                if(retVal == CL_ERR_NO_MEMORY)
                {
                    retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                    clLogError("SUB", "SCL",
                            "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                            retVal, requestInfo.oid);
                    netsnmp_request_set_error(requests, retVal);
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
                            clLogError("SUB", "SCL",
                                    "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                    medErrList.pErrorList[i].errId, requestInfo.oid);
                            netsnmp_request_set_error(requests, medErrList.pErrorList[i].errId);
                            return retVal;

                        }
                        
                    }
                }
            }     
            break;
        }
        case MODE_SET_COMMIT:
        {
            clLogDebug("SUB", "SCL",
                    "Received COMMIT request");
            clSnmpUndo();
            break;
        }
        case MODE_SET_UNDO:
        {
            clLogDebug("SUB", "SCL", 
                    "Received UNDO request");
            clSnmpUndo();
            break;
        }

        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfServiceStateHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfClusterStartupTimeoutHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfClusterStartupTimeoutOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClCharT saAmfClusterStartupTimeoutVal[9] = {0};

            requestInfo.dataLen = 8 * sizeof(ClCharT);
            pVal = saAmfClusterStartupTimeoutVal;
            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_OCTET_STR;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }

        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
        {
            ClInt32T retVal = 0;
            clLogDebug("SUB", "SCL",
            "Received RESERVE1 request");
            retVal = netsnmp_check_vb_type(reqVar, ASN_OCTET_STR);
            if(retVal != SNMP_ERR_NOERROR ) 
            {
                netsnmp_request_set_error(requests, retVal );
                return retVal;
            }

            retVal = netsnmp_check_vb_size_range(reqVar, 0, 8);
            if(retVal != SNMP_ERR_NOERROR)
            {
                netsnmp_request_set_error(requests, retVal );
                return retVal;
            }
            break;
        }
        case MODE_SET_RESERVE2:
        {
            ClInt32T retVal = 0, errorCode = 0, noOfOp = 1;
            ClPtrT pVal = NULL;


            ClCharT Val[8] = {0};

            pVal = Val;
            requestInfo.dataLen = 8 * sizeof(ClCharT);
            requestInfo.opCode = CL_SNMP_SET;
            requestInfo.index.scalarType = 0;

            memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

            clLogDebug("SUB", "SCL",
                    "Calling request add");
            retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
            if(CL_OK != retVal)
            {
                clLogError("SUB", "SCL",
                        "Failed to add request to store, rc=[0x%x], errorCode=[0x%x]", 
                        retVal, errorCode);
                netsnmp_request_set_error(requests, errorCode);
                return retVal;
            }
            clLogDebug("SUB", "SCL",
                    "Successfully added request in store");
            break;
        }

        case MODE_SET_FREE:
        {
            clLogDebug("SUB", "SCL",
            "Received FREE request");
            clSnmpUndo();
            break;
        }

        case MODE_SET_ACTION:
        {
            ClInt32T    retVal = 0;
            ClMedErrorListT    medErrList;

            clLogDebug("SUB", "SCL",
                    "Received ACTION request for oid[%s] ",requestInfo.oid);
            retVal = clSnmpCommit(&medErrList);
            if (retVal != 0)
            {
                clLogDebug("SUB", "SCL",
                        "Error in commit errCount[%d]",
                        medErrList.count);
                if(retVal == CL_ERR_NO_MEMORY)
                {
                    retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                    clLogError("SUB", "SCL",
                            "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                            retVal, requestInfo.oid);
                    netsnmp_request_set_error(requests, retVal);
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
                            clLogError("SUB", "SCL",
                                    "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                    medErrList.pErrorList[i].errId, requestInfo.oid);
                            netsnmp_request_set_error(requests, medErrList.pErrorList[i].errId);
                            return retVal;

                        }
                        
                    }
                }
            }     
            break;
        }
        case MODE_SET_COMMIT:
        {
            clLogDebug("SUB", "SCL",
                    "Received COMMIT request");
            clSnmpUndo();
            break;
        }
        case MODE_SET_UNDO:
        {
            clLogDebug("SUB", "SCL", 
                    "Received UNDO request");
            clSnmpUndo();
            break;
        }

        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfClusterStartupTimeoutHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfClusterAdminStateHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfClusterAdminStateOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfClusterAdminStateVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfClusterAdminStateVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfClusterAdminStateHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfClusterAdminStateTriggerHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfClusterAdminStateTriggerOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfClusterAdminStateTriggerVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfClusterAdminStateTriggerVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }

        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
        {
            ClInt32T retVal = 0;
            clLogDebug("SUB", "SCL",
            "Received RESERVE1 request");
            retVal = netsnmp_check_vb_type(reqVar, ASN_INTEGER);
            if(retVal != SNMP_ERR_NOERROR ) 
            {
                netsnmp_request_set_error(requests, retVal );
                return retVal;
            }

            switch(*(reqVar->val.integer))
            {
                case SAAMFCLUSTERADMINSTATETRIGGER_STABLE:
                    break;
                case SAAMFCLUSTERADMINSTATETRIGGER_UNLOCK:
                    break;
                case SAAMFCLUSTERADMINSTATETRIGGER_LOCK:
                    break;
                case SAAMFCLUSTERADMINSTATETRIGGER_LOCKINSTANTIATION:
                    break;
                case SAAMFCLUSTERADMINSTATETRIGGER_UNLOCKINSTANTIATION:
                    break;
                case SAAMFCLUSTERADMINSTATETRIGGER_SHUTDOWN:
                    break;
                /** not a legal enum value.  return an error */
                default:
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Inconsistent value."));
                    netsnmp_request_set_error(requests, SNMP_ERR_WRONGVALUE);
                    return SNMP_ERR_WRONGVALUE;
            }
            break;
        }
        case MODE_SET_RESERVE2:
        {
            ClInt32T retVal = 0, errorCode = 0, noOfOp = 1;
            ClPtrT pVal = NULL;


            ClInt32T    Val = 0;

            pVal = &Val;
            requestInfo.dataLen = sizeof(ClInt32T); 
            requestInfo.opCode = CL_SNMP_SET;
            requestInfo.index.scalarType = 0;

            memcpy(pVal, (void *)reqVar->val.integer, requestInfo.dataLen);

            clLogDebug("SUB", "SCL",
                    "Calling request add");
            retVal = clRequestAdd( &requestInfo, pVal, &noOfOp, &errorCode, NULL);
            if(CL_OK != retVal)
            {
                clLogError("SUB", "SCL",
                        "Failed to add request to store, rc=[0x%x], errorCode=[0x%x]", 
                        retVal, errorCode);
                netsnmp_request_set_error(requests, errorCode);
                return retVal;
            }
            clLogDebug("SUB", "SCL",
                    "Successfully added request in store");
            break;
        }

        case MODE_SET_FREE:
        {
            clLogDebug("SUB", "SCL",
            "Received FREE request");
            clSnmpUndo();
            break;
        }

        case MODE_SET_ACTION:
        {
            ClInt32T    retVal = 0;
            ClMedErrorListT    medErrList;

            clLogDebug("SUB", "SCL",
                    "Received ACTION request for oid[%s] ",requestInfo.oid);
            retVal = clSnmpCommit(&medErrList);
            if (retVal != 0)
            {
                clLogDebug("SUB", "SCL",
                        "Error in commit errCount[%d]",
                        medErrList.count);
                if(retVal == CL_ERR_NO_MEMORY)
                {
                    retVal = CL_SNMP_ERR_NOCREATION; /* Set SNMP equivalent error from oidErrMapTable in clSnmpInit.c */
                    clLogError("SUB", "SCL",
                            "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                            retVal, requestInfo.oid);
                    netsnmp_request_set_error(requests, retVal);
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
                            clLogError("SUB", "SCL",
                                    "ACTION phase failed, setting errorCode[0x%x] for oid[%s]", 
                                    medErrList.pErrorList[i].errId, requestInfo.oid);
                            netsnmp_request_set_error(requests, medErrList.pErrorList[i].errId);
                            return retVal;

                        }
                        
                    }
                }
            }     
            break;
        }
        case MODE_SET_COMMIT:
        {
            clLogDebug("SUB", "SCL",
                    "Received COMMIT request");
            clSnmpUndo();
            break;
        }
        case MODE_SET_UNDO:
        {
            clLogDebug("SUB", "SCL", 
                    "Received UNDO request");
            clSnmpUndo();
            break;
        }

        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfClusterAdminStateTriggerHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfSvcUserNameHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfSvcUserNameOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClCharT saAmfSvcUserNameVal[256] = {0};

            requestInfo.dataLen = 255 * sizeof(ClCharT);
            pVal = saAmfSvcUserNameVal;
            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_OCTET_STR;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfSvcUserNameHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpsaAmfProbableCauseHandler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    ClSNMPRequestInfoT requestInfo;
    netsnmp_variable_list *reqVar = NULL;

    if(!requests || !reqinfo)
        return SNMP_ERR_GENERR;


    reqVar = requests->requestvb;

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* An instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    memset(&requestInfo, 0, sizeof(ClSNMPRequestInfoT));

/* Extract the scalar OID, convert it to string form and assign it to requestInfo.oid */

    requestInfo.oidLen = OID_LENGTH(saAmfProbableCauseOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_SAFAMF_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    saAmfProbableCauseVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &saAmfProbableCauseVal;

            requestInfo.index.scalarType = 0;
            requestInfo.opCode = CL_SNMP_GET;
            reqAddInfo.pRequest = requests;
            reqAddInfo.columnType = ASN_INTEGER;
            clLogDebug("SUB", "SCL",
            "Received GET request");

            retVal = clRequestAdd(&requestInfo, pVal, NULL, &errorCode, &reqAddInfo);

            if(retVal != 0 || errorCode < 0)
            {
                netsnmp_request_set_error (requests, errorCode);
                return errorCode;
            }
            break;
        }


        default:
            /* we should never get here, so this is a really bad error */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpsaAmfProbableCauseHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
