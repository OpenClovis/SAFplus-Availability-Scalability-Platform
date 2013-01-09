#ifndef _CL_SNMP_SUBAGENT_OCTRAIN_CONFIG_H_
#define _CL_SNMP_SUBAGENT_OCTRAIN_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************
* General ASP header files
****************************/
#include <clCorMetaData.h>
#include <clMedApi.h>
#include <clSnmpDefs.h>
#include <clSnmpDataDefinition.h>

/***************************
* Constant definition 
****************************/

/*
 * Index information for all tables.
 */

typedef struct clSnmpclockTableIndexInfo
{
    ClInt32T clockRow;
} ClSnmpclockTableIndexInfoT;

typedef struct clSnmptimeSetTableIndexInfo
{
    ClInt32T timeSetRow;
} ClSnmptimeSetTableIndexInfoT;

typedef struct clSnmpnameTableIndexInfo
{
    ClUint32T nodeAdd;
} ClSnmpnameTableIndexInfoT;


/* function prototypes.*/
ClInt32T  clSnmpocTrainDefaultInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpclockTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmptimeSetTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpnameTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);

extern void init_ocTrainMIB(void);
/* Global structures.*/
extern ClSnmpOidInfoT appOidInfo[];
extern ClSnmpNtfyCallbackTableT clSnmpAppCallback[];

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_SUBAGENT_OCTRAIN_CONFIG_H_ */
