#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <clDebugApi.h>
#include <clLogApi.h>
#include <clHeapApi.h>
#include "clSnmpocTrainNotifications.h"
#include <clSnmpocTrainUtil.h>

static oid snmpTrapOid[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

void clSnmpSendalarmTrapTrap(ClTrapPayLoadListT   *pTrapPayLoadList)
{

    
    ClInt32T count = 0;

    ClInt32T oidLen = 0;

    netsnmp_variable_list  *pVarList = NULL;
    oid alarmTrapOid[] = { 1,3,6,1,4,1,103,5,1 };
    oid *clockIdOid = NULL;
    oid clockIdTempOid[] = { 1,3,6,1,4,1,103,2,1,1,2 };

    clLogDebug("SNM", NULL, "Inside trap handler");
    if(1 != pTrapPayLoadList->numTrapPayloadEntry)
    {
        clLogError(CL_SNMP_AREA, CL_SNMP_GEN_OP_COTX, "No of notification payload passed : [%d] but it suppose to be : [%d]", pTrapPayLoadList->numTrapPayloadEntry, 1); 
	return; 
    }

    if(pTrapPayLoadList == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ("NULL arguments received!"));
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&pVarList,
        snmpTrapOid, OID_LENGTH(snmpTrapOid),
        ASN_OBJECT_ID,
        (ClUint8T *)alarmTrapOid, sizeof(alarmTrapOid));
    
    /*
     * Add any objects from the trap definition
     */
    oidLen = sizeof(clockIdTempOid) + pTrapPayLoadList->pPayLoad[count].oidLen;
    clockIdOid = (oid *)clHeapAllocate(oidLen);

    if(clockIdOid == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory allocation failed for clockIdOid, size = %d", oidLen));
        return;
    }

    memcpy(clockIdOid, clockIdTempOid, sizeof(clockIdTempOid));
    memcpy(clockIdOid + (sizeof(clockIdTempOid)/sizeof(oid)), 
           pTrapPayLoadList->pPayLoad[count].objOid, pTrapPayLoadList->pPayLoad[count].oidLen);
    snmp_varlist_add_variable(&pVarList, clockIdOid,
        oidLen / sizeof(ClUint32T),
        ASN_INTEGER,
        (ClUint8T *)pTrapPayLoadList->pPayLoad[count].val,
        pTrapPayLoadList->pPayLoad[count].valLen
    );
    clHeapFree(clockIdOid);
    count++;

    /*
     * Add any extra (optional) objects here
     */

    /*
     * Send the trap to the list of configured destinations
     *  and clean up
     */
     clLogInfo("SNM", NULL, "Sending trap to trap manager");
    send_v2trap( pVarList );
    snmp_free_varbind( pVarList );

}
