#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <clDebugApi.h>
#include <clLogApi.h>
#include <clHeapApi.h>
#include "clSnmpsafAmfNotifications.h"
#include <clSnmpsafAmfUtil.h>

static oid snmpTrapOid[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

void clSnmpSendsaAmfAlarmServiceImpairedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;


    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmServiceImpairedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,1 };
    oid saAmfProbableCauseOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,1,2, 0 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmServiceImpairedOid, sizeof(saAmfAlarmServiceImpairedOid));
    
    /*
     * Add any objects from the trap definition
     */
    snmp_varlist_add_variable(&pVarList,
        saAmfProbableCauseOid, OID_LENGTH(saAmfProbableCauseOid),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfAlarmCompInstantiationFailedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmCompInstantiationFailedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,2 };
    oid *saAmfCompNameOid = NULL;
    oid saAmfCompNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmCompInstantiationFailedOid, sizeof(saAmfAlarmCompInstantiationFailedOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfCompNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfCompNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfCompNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfCompNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfCompNameOid, saAmfCompNameTempOid, sizeof(saAmfCompNameTempOid));
    memcpy(saAmfCompNameOid + (sizeof(saAmfCompNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfCompNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfCompNameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfAlarmCompCleanupFailedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmCompCleanupFailedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,3 };
    oid *saAmfCompNameOid = NULL;
    oid saAmfCompNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmCompCleanupFailedOid, sizeof(saAmfAlarmCompCleanupFailedOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfCompNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfCompNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfCompNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfCompNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfCompNameOid, saAmfCompNameTempOid, sizeof(saAmfCompNameTempOid));
    memcpy(saAmfCompNameOid + (sizeof(saAmfCompNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfCompNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfCompNameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfAlarmClusterResetTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmClusterResetOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,4 };
    oid *saAmfCompNameOid = NULL;
    oid saAmfCompNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmClusterResetOid, sizeof(saAmfAlarmClusterResetOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfCompNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfCompNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfCompNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfCompNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfCompNameOid, saAmfCompNameTempOid, sizeof(saAmfCompNameTempOid));
    memcpy(saAmfCompNameOid + (sizeof(saAmfCompNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfCompNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfCompNameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfAlarmSIUnassignedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmSIUnassignedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,5 };
    oid *saAmfSINameOid = NULL;
    oid saAmfSINameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmSIUnassignedOid, sizeof(saAmfAlarmSIUnassignedOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSINameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSINameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSINameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSINameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSINameOid, saAmfSINameTempOid, sizeof(saAmfSINameTempOid));
    memcpy(saAmfSINameOid + (sizeof(saAmfSINameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSINameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSINameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfAlarmProxiedCompUnproxiedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfAlarmProxiedCompUnproxiedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,6 };
    oid *saAmfCompNameOid = NULL;
    oid saAmfCompNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfAlarmProxiedCompUnproxiedOid, sizeof(saAmfAlarmProxiedCompUnproxiedOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfCompNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfCompNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfCompNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfCompNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfCompNameOid, saAmfCompNameTempOid, sizeof(saAmfCompNameTempOid));
    memcpy(saAmfCompNameOid + (sizeof(saAmfCompNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfCompNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfCompNameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgClusterAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;


    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgClusterAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,7 };
    oid saAmfClusterAdminStateOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,1,7, 0 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgClusterAdminOid, sizeof(saAmfStateChgClusterAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    snmp_varlist_add_variable(&pVarList,
        saAmfClusterAdminStateOid, OID_LENGTH(saAmfClusterAdminStateOid),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgApplicationAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgApplicationAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,8 };
    oid *saAmfApplicationNameOid = NULL;
    oid saAmfApplicationNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,1,1,2 };
    oid *saAmfApplicationAdminStateOid = NULL;
    oid saAmfApplicationAdminStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,1,1,3 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgApplicationAdminOid, sizeof(saAmfStateChgApplicationAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfApplicationNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfApplicationNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfApplicationNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfApplicationNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfApplicationNameOid, saAmfApplicationNameTempOid, sizeof(saAmfApplicationNameTempOid));
    memcpy(saAmfApplicationNameOid + (sizeof(saAmfApplicationNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfApplicationNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfApplicationNameOid);
    count++;
    oidLen = sizeof(saAmfApplicationAdminStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfApplicationAdminStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfApplicationAdminStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfApplicationAdminStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfApplicationAdminStateOid, saAmfApplicationAdminStateTempOid, sizeof(saAmfApplicationAdminStateTempOid));
    memcpy(saAmfApplicationAdminStateOid + (sizeof(saAmfApplicationAdminStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfApplicationAdminStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfApplicationAdminStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgNodeAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgNodeAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,9 };
    oid *saAmfNodeNameOid = NULL;
    oid saAmfNodeNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,2,1,2 };
    oid *saAmfNodeAdminStateOid = NULL;
    oid saAmfNodeAdminStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,2,1,6 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgNodeAdminOid, sizeof(saAmfStateChgNodeAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfNodeNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfNodeNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfNodeNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfNodeNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfNodeNameOid, saAmfNodeNameTempOid, sizeof(saAmfNodeNameTempOid));
    memcpy(saAmfNodeNameOid + (sizeof(saAmfNodeNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfNodeNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfNodeNameOid);
    count++;
    oidLen = sizeof(saAmfNodeAdminStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfNodeAdminStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfNodeAdminStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfNodeAdminStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfNodeAdminStateOid, saAmfNodeAdminStateTempOid, sizeof(saAmfNodeAdminStateTempOid));
    memcpy(saAmfNodeAdminStateOid + (sizeof(saAmfNodeAdminStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfNodeAdminStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfNodeAdminStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSGAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSGAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,10 };
    oid *saAmfSGNameOid = NULL;
    oid saAmfSGNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,3,1,2 };
    oid *saAmfSGAdminStateOid = NULL;
    oid saAmfSGAdminStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,3,1,10 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSGAdminOid, sizeof(saAmfStateChgSGAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSGNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSGNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSGNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSGNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSGNameOid, saAmfSGNameTempOid, sizeof(saAmfSGNameTempOid));
    memcpy(saAmfSGNameOid + (sizeof(saAmfSGNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSGNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSGNameOid);
    count++;
    oidLen = sizeof(saAmfSGAdminStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSGAdminStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSGAdminStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSGAdminStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSGAdminStateOid, saAmfSGAdminStateTempOid, sizeof(saAmfSGAdminStateTempOid));
    memcpy(saAmfSGAdminStateOid + (sizeof(saAmfSGAdminStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSGAdminStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSGAdminStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSUAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSUAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,11 };
    oid *saAmfSUNameOid = NULL;
    oid saAmfSUNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,2 };
    oid *saAmfSUAdminStateOid = NULL;
    oid saAmfSUAdminStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,7 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSUAdminOid, sizeof(saAmfStateChgSUAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSUNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUNameOid, saAmfSUNameTempOid, sizeof(saAmfSUNameTempOid));
    memcpy(saAmfSUNameOid + (sizeof(saAmfSUNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUNameOid);
    count++;
    oidLen = sizeof(saAmfSUAdminStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUAdminStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUAdminStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUAdminStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUAdminStateOid, saAmfSUAdminStateTempOid, sizeof(saAmfSUAdminStateTempOid));
    memcpy(saAmfSUAdminStateOid + (sizeof(saAmfSUAdminStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUAdminStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUAdminStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSIAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSIAdminOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,12 };
    oid *saAmfSINameOid = NULL;
    oid saAmfSINameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5,1,2 };
    oid *saAmfSIAdminStateOid = NULL;
    oid saAmfSIAdminStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5,1,6 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSIAdminOid, sizeof(saAmfStateChgSIAdminOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSINameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSINameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSINameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSINameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSINameOid, saAmfSINameTempOid, sizeof(saAmfSINameTempOid));
    memcpy(saAmfSINameOid + (sizeof(saAmfSINameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSINameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSINameOid);
    count++;
    oidLen = sizeof(saAmfSIAdminStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSIAdminStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSIAdminStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSIAdminStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSIAdminStateOid, saAmfSIAdminStateTempOid, sizeof(saAmfSIAdminStateTempOid));
    memcpy(saAmfSIAdminStateOid + (sizeof(saAmfSIAdminStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSIAdminStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSIAdminStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgNodeOperTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgNodeOperOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,13 };
    oid *saAmfNodeNameOid = NULL;
    oid saAmfNodeNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,2,1,2 };
    oid *saAmfNodeOperationalStateOid = NULL;
    oid saAmfNodeOperationalStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,2,1,7 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgNodeOperOid, sizeof(saAmfStateChgNodeOperOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfNodeNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfNodeNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfNodeNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfNodeNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfNodeNameOid, saAmfNodeNameTempOid, sizeof(saAmfNodeNameTempOid));
    memcpy(saAmfNodeNameOid + (sizeof(saAmfNodeNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfNodeNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfNodeNameOid);
    count++;
    oidLen = sizeof(saAmfNodeOperationalStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfNodeOperationalStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfNodeOperationalStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfNodeOperationalStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfNodeOperationalStateOid, saAmfNodeOperationalStateTempOid, sizeof(saAmfNodeOperationalStateTempOid));
    memcpy(saAmfNodeOperationalStateOid + (sizeof(saAmfNodeOperationalStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfNodeOperationalStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfNodeOperationalStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSUOperTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSUOperOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,14 };
    oid *saAmfSUNameOid = NULL;
    oid saAmfSUNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,2 };
    oid *saAmfSUOperStateOid = NULL;
    oid saAmfSUOperStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,10 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSUOperOid, sizeof(saAmfStateChgSUOperOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSUNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUNameOid, saAmfSUNameTempOid, sizeof(saAmfSUNameTempOid));
    memcpy(saAmfSUNameOid + (sizeof(saAmfSUNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUNameOid);
    count++;
    oidLen = sizeof(saAmfSUOperStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUOperStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUOperStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUOperStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUOperStateOid, saAmfSUOperStateTempOid, sizeof(saAmfSUOperStateTempOid));
    memcpy(saAmfSUOperStateOid + (sizeof(saAmfSUOperStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUOperStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUOperStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSUPresenceTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSUPresenceOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,15 };
    oid *saAmfSUNameOid = NULL;
    oid saAmfSUNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,2 };
    oid *saAmfSUPresenceStateOid = NULL;
    oid saAmfSUPresenceStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,4,1,11 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSUPresenceOid, sizeof(saAmfStateChgSUPresenceOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSUNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUNameOid, saAmfSUNameTempOid, sizeof(saAmfSUNameTempOid));
    memcpy(saAmfSUNameOid + (sizeof(saAmfSUNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUNameOid);
    count++;
    oidLen = sizeof(saAmfSUPresenceStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUPresenceStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUPresenceStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUPresenceStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUPresenceStateOid, saAmfSUPresenceStateTempOid, sizeof(saAmfSUPresenceStateTempOid));
    memcpy(saAmfSUPresenceStateOid + (sizeof(saAmfSUPresenceStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUPresenceStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUPresenceStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSUHaStateTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSUHaStateOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,16 };
    oid *saAmfSUSISuNameOid = NULL;
    oid saAmfSUSISuNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,16,1,3 };
    oid *saAmfSUSISiNameOid = NULL;
    oid saAmfSUSISiNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,16,1,4 };
    oid *saAmfSUSIHAStateOid = NULL;
    oid saAmfSUSIHAStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,16,1,5 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(3 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 3); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSUHaStateOid, sizeof(saAmfStateChgSUHaStateOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSUSISuNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUSISuNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUSISuNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUSISuNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUSISuNameOid, saAmfSUSISuNameTempOid, sizeof(saAmfSUSISuNameTempOid));
    memcpy(saAmfSUSISuNameOid + (sizeof(saAmfSUSISuNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUSISuNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUSISuNameOid);
    count++;
    oidLen = sizeof(saAmfSUSISiNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUSISiNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUSISiNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUSISiNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUSISiNameOid, saAmfSUSISiNameTempOid, sizeof(saAmfSUSISiNameTempOid));
    memcpy(saAmfSUSISiNameOid + (sizeof(saAmfSUSISiNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUSISiNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUSISiNameOid);
    count++;
    oidLen = sizeof(saAmfSUSIHAStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSUSIHAStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSUSIHAStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSUSIHAStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSUSIHAStateOid, saAmfSUSIHAStateTempOid, sizeof(saAmfSUSIHAStateTempOid));
    memcpy(saAmfSUSIHAStateOid + (sizeof(saAmfSUSIHAStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSUSIHAStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSUSIHAStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgSIAssignmentTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgSIAssignmentOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,17 };
    oid *saAmfSINameOid = NULL;
    oid saAmfSINameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5,1,2 };
    oid *saAmfSIAssignmentStateOid = NULL;
    oid saAmfSIAssignmentStateTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,5,1,7 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(2 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 2); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgSIAssignmentOid, sizeof(saAmfStateChgSIAssignmentOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfSINameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSINameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSINameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSINameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSINameOid, saAmfSINameTempOid, sizeof(saAmfSINameTempOid));
    memcpy(saAmfSINameOid + (sizeof(saAmfSINameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSINameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSINameOid);
    count++;
    oidLen = sizeof(saAmfSIAssignmentStateTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfSIAssignmentStateOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfSIAssignmentStateOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfSIAssignmentStateOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfSIAssignmentStateOid, saAmfSIAssignmentStateTempOid, sizeof(saAmfSIAssignmentStateTempOid));
    memcpy(saAmfSIAssignmentStateOid + (sizeof(saAmfSIAssignmentStateTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfSIAssignmentStateOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfSIAssignmentStateOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
void clSnmpSendsaAmfStateChgUnproxiedCompProxiedTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid saAmfStateChgUnproxiedCompProxiedOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,3,2,18 };
    oid *saAmfCompNameOid = NULL;
    oid saAmfCompNameTempOid[] = { 1,3,6,1,4,1,18568,2,2,2,1,2,10,1,2 };

    clLogDebug("SNM", "NTF", "Inside trap handler");
    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
        return;
    }
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)saAmfStateChgUnproxiedCompProxiedOid, sizeof(saAmfStateChgUnproxiedCompProxiedOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(saAmfCompNameTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    saAmfCompNameOid = (oid *)clHeapAllocate(oidLen);

    if(saAmfCompNameOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for saAmfCompNameOid, size = %d", oidLen));
        return;
    }

    memcpy(saAmfCompNameOid, saAmfCompNameTempOid, sizeof(saAmfCompNameTempOid));
    memcpy(saAmfCompNameOid + (sizeof(saAmfCompNameTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, saAmfCompNameOid,
        oidLen / sizeof(ClUint32T),
        ASN_OCTET_STR,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(saAmfCompNameOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", "NTF", "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
