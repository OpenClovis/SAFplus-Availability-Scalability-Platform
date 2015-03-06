#ifndef _CL_SNMP_SAFAMF_H
#define _CL_SNMP_SAFAMF_H

#include <clCommon.h>
#include <clSnmpDefs.h>

/* function declarations */
void clSnmpSendsaAmfAlarmServiceImpairedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfAlarmCompInstantiationFailedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfAlarmCompCleanupFailedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfAlarmClusterResetTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfAlarmSIUnassignedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfAlarmProxiedCompUnproxiedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgClusterAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgApplicationAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgNodeAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSGAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSUAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSIAdminTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgNodeOperTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSUOperTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSUPresenceTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSUHaStateTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgSIAssignmentTrap(ClTrapPayLoadListT   *pTrapPayLoadList);
void clSnmpSendsaAmfStateChgUnproxiedCompProxiedTrap(ClTrapPayLoadListT   *pTrapPayLoadList);

#endif /* _CL_SNMP_SAFAMF_H */
