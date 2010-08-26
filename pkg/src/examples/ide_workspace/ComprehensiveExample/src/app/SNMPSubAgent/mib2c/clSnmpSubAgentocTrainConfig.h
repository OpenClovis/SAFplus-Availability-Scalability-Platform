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
#define MODULE_NAME    "ocTrain"    /*Module name as specified in the MIB file.*/

/*
 * Information regarding all the tables on which the sub-agent is operating.
 */
enum CL_SNMP_OCTRAIN_OBJECTS {
    CL_OCTRAIN_SCALARS, 
    CL_CLOCKTABLE,
    CL_TIMESETTABLE,
    CL_NAMETABLE,
    CL_SNMP_TABLE_MAX
} clSnmpocTrainEnumType;

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



/* The union part of this structure will change according to application MIB
 * indices. The first 5 fields before the union should not be changed. */
 
#define CL_SNMP_MAX_OID_STR_LEN (CL_SNMP_MAX_OID_LEN * sizeof(unsigned long))

typedef struct ClSNMPRequestInfo
{
    /* This is the application independent part*/
    ClInt32T    tableType;
    ClCharT     oid[CL_SNMP_MAX_OID_STR_LEN];
    ClInt32T    oidLen;
    ClSnmpOpCodeT    opCode;
    ClUint32T   dataLen;    
    /* Change the definition of union based on the indices for the application
     * MIB*/
    union
    {
        ClUint32T scalarType;
        ClSnmpclockTableIndexInfoT clockTableInfo;
        ClSnmptimeSetTableIndexInfoT timeSetTableInfo;
        ClSnmpnameTableIndexInfoT nameTableInfo;
    }index;
}ClSNMPRequestInfoT;

/* function prototypes.*/
ClInt32T  clSnmpocTrainDefaultInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpclockTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmptimeSetTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpnameTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);

extern void init_appMIB(void);
/* Global structures.*/
extern ClSnmpOidInfoT appOidInfo[];
extern ClSnmpNtfyCallbackTableT clSnmpAppCallback[];

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_SUBAGENT_OCTRAIN_CONFIG_H_ */
