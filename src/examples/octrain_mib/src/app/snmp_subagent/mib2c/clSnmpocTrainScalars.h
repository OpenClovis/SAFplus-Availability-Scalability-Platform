#ifndef _CL_SNMP_OCTRAIN_SCALARS_H
#define _CL_SNMP_OCTRAIN_SCALARS_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */
void clSnmpocTrainScalarsInit(void);
Netsnmp_Node_Handler clSnmpclockRunningStatusHandler;
Netsnmp_Node_Handler clSnmpclockRedStatusHandler;

/* enum definitions */
#include "clSnmpocTrainEnums.h"

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_OCTRAIN_SCALARS_H */
