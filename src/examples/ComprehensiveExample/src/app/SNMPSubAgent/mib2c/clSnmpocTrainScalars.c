#include <clDebugApi.h>
#include <clSnmpOp.h>
#include <clSnmpocTrainScalars.h>

static oid clockRunningStatusOid[] = { 1,3,6,1,4,1,103,1,1 };
static oid clockRedStatusOid[] = { 1,3,6,1,4,1,103,1,2 };

/** Initializes the ocTrain module */
void
clSnmpocTrainScalarsInit(void)
{
    netsnmp_mib_handler *callback_handler;
    netsnmp_handler_registration    *reg = NULL;

    DEBUGMSGTL(("ocTrainScalars", "Initializing\n"));

    reg = netsnmp_create_handler_registration("clockRunningStatus", clSnmpclockRunningStatusHandler,
                               clockRunningStatusOid, OID_LENGTH(clockRunningStatusOid),
                               HANDLER_CAN_RONLY
        );
    netsnmp_register_scalar (reg);

    callback_handler = netsnmp_get_mode_end_call_handler(
                               netsnmp_mode_end_call_add_mode_callback(NULL,
                               NETSNMP_MODE_END_ALL_MODES, netsnmp_create_handler("my_end_callback_handler",
                                                         clSnmpModeEndCallback)
                                ));
    netsnmp_inject_handler (reg, callback_handler);

    reg = netsnmp_create_handler_registration("clockRedStatus", clSnmpclockRedStatusHandler,
                               clockRedStatusOid, OID_LENGTH(clockRedStatusOid),
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
clSnmpclockRunningStatusHandler(netsnmp_mib_handler *handler,
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

    requestInfo.oidLen = OID_LENGTH(clockRunningStatusOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_OCTRAIN_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    clockRunningStatusVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &clockRunningStatusVal;

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
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpclockRunningStatusHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
clSnmpclockRedStatusHandler(netsnmp_mib_handler *handler,
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

    requestInfo.oidLen = OID_LENGTH(clockRedStatusOid);
    clSnmpOidCpy(&requestInfo, reqVar->name);

    requestInfo.tableType = CL_OCTRAIN_SCALARS;
    
    switch(reqinfo->mode) {

        case MODE_GET:
        {
            ClInt32T retVal = 0, errorCode = 0;
            _ClSnmpGetReqInfoT  reqAddInfo = {0};
            void * pVal = NULL;
            ClInt32T    clockRedStatusVal = 0;

            requestInfo.dataLen = sizeof(ClInt32T);
            pVal = &clockRedStatusVal;

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
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("unknown mode (%d) in clSnmpclockRedStatusHandler\n", reqinfo->mode) );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
