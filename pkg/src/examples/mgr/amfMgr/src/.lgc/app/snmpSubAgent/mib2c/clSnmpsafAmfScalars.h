#ifndef _CL_SNMP_SAFAMF_SCALARS_H
#define _CL_SNMP_SAFAMF_SCALARS_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */
void clSnmpsafAmfScalarsInit(void);
Netsnmp_Node_Handler clSnmpsaAmfAgentSpecVersionHandler;
Netsnmp_Node_Handler clSnmpsaAmfAgentVendorHandler;
Netsnmp_Node_Handler clSnmpsaAmfAgentVendorProductRevHandler;
Netsnmp_Node_Handler clSnmpsaAmfServiceStartEnabledHandler;
Netsnmp_Node_Handler clSnmpsaAmfServiceStateHandler;
Netsnmp_Node_Handler clSnmpsaAmfClusterStartupTimeoutHandler;
Netsnmp_Node_Handler clSnmpsaAmfClusterAdminStateHandler;
Netsnmp_Node_Handler clSnmpsaAmfClusterAdminStateTriggerHandler;
Netsnmp_Node_Handler clSnmpsaAmfSvcUserNameHandler;
Netsnmp_Node_Handler clSnmpsaAmfProbableCauseHandler;

/* enum definitions */
#include "clSnmpsafAmfEnums.h"

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_SAFAMF_SCALARS_H */
