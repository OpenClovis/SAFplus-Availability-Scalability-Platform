#ifndef _CL_SNMP_OCTRAIN_TABLES_H
#define _CL_SNMP_OCTRAIN_TABLES_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <clSnmpDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClSnmpTableIndexCorAttrIdInfoT clockTableIndexCorAttrIdList[];
extern ClSnmpTableIndexCorAttrIdInfoT timeSetTableIndexCorAttrIdList[];
extern ClSnmpTableIndexCorAttrIdInfoT nameTableIndexCorAttrIdList[];


/* function declarations */
void clSnmpocTrainTablesInit(void);
void clSnmpclockTableInitialize(void);
void clSnmptimeSetTableInitialize(void);
void clSnmpnameTableInitialize(void);
Netsnmp_Node_Handler clSnmpclockTableHandler;
Netsnmp_Node_Handler clSnmptimeSetTableHandler;
Netsnmp_Node_Handler clSnmpnameTableHandler;

Netsnmp_First_Data_Point  clSnmpclockTableGetFirstDataPoint;
Netsnmp_Next_Data_Point   clSnmpclockTableGetNextDataPoint;
Netsnmp_First_Data_Point  clSnmptimeSetTableGetFirstDataPoint;
Netsnmp_Next_Data_Point   clSnmptimeSetTableGetNextDataPoint;
Netsnmp_First_Data_Point  clSnmpnameTableGetFirstDataPoint;
Netsnmp_Next_Data_Point   clSnmpnameTableGetNextDataPoint;

/* column number definitions. */
#include "clSnmpocTrainColumns.h"

/* enum definitions */
#include "clSnmpocTrainEnums.h"

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_OCTRAIN_TABLES_H */
