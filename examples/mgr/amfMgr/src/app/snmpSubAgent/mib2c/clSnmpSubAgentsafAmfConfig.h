#ifndef _CL_SNMP_SUBAGENT_SAFAMF_CONFIG_H_
#define _CL_SNMP_SUBAGENT_SAFAMF_CONFIG_H_

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

typedef struct clSnmpsaAmfApplicationTableIndexInfo
{
    ClUint32T saAmfApplicationNameId;
} ClSnmpsaAmfApplicationTableIndexInfoT;

typedef struct clSnmpsaAmfNodeTableIndexInfo
{
    ClUint32T saAmfNodeNameId;
} ClSnmpsaAmfNodeTableIndexInfoT;

typedef struct clSnmpsaAmfSGTableIndexInfo
{
    ClUint32T saAmfSGNameId;
} ClSnmpsaAmfSGTableIndexInfoT;

typedef struct clSnmpsaAmfSUTableIndexInfo
{
    ClUint32T saAmfSUNameId;
} ClSnmpsaAmfSUTableIndexInfoT;

typedef struct clSnmpsaAmfSITableIndexInfo
{
    ClUint32T saAmfSINameId;
} ClSnmpsaAmfSITableIndexInfoT;

typedef struct clSnmpsaAmfSUsperSIRankTableIndexInfo
{
    ClUint32T saAmfSUsperSINameId;
    ClUint32T saAmfSUsperSIRank;
} ClSnmpsaAmfSUsperSIRankTableIndexInfoT;

typedef struct clSnmpsaAmfSGSIRankTableIndexInfo
{
    ClUint32T saAmfSGSIRankSGNameId;
    ClUint32T saAmfSGSIRank;
} ClSnmpsaAmfSGSIRankTableIndexInfoT;

typedef struct clSnmpsaAmfSGSURankTableIndexInfo
{
    ClUint32T saAmfSGSURankSGNameId;
    ClUint32T saAmfSGSURank;
} ClSnmpsaAmfSGSURankTableIndexInfoT;

typedef struct clSnmpsaAmfSISIDepTableIndexInfo
{
    ClUint32T saAmfSISIDepSINameId;
    ClUint32T saAmfSISIDepDepndSINameId;
} ClSnmpsaAmfSISIDepTableIndexInfoT;

typedef struct clSnmpsaAmfCompTableIndexInfo
{
    ClUint32T saAmfCompNameId;
} ClSnmpsaAmfCompTableIndexInfoT;

typedef struct clSnmpsaAmfCompCSTypeSupportedTableIndexInfo
{
    ClUint32T saAmfCompCSTypeSupportedCompNameId;
    ClUint32T saAmfCompCSTypeSupportedCSTypeNameId;
} ClSnmpsaAmfCompCSTypeSupportedTableIndexInfoT;

typedef struct clSnmpsaAmfCSITableIndexInfo
{
    ClUint32T saAmfCSINameId;
} ClSnmpsaAmfCSITableIndexInfoT;

typedef struct clSnmpsaAmfCSICSIDepTableIndexInfo
{
    ClUint32T saAmfCSICSIDepCSINameId;
    ClUint32T saAmfCSICSIDepDepndCSINameId;
} ClSnmpsaAmfCSICSIDepTableIndexInfoT;

typedef struct clSnmpsaAmfCSINameValueTableIndexInfo
{
    ClUint32T saAmfCSINameValueCSINameId;
    ClUint32T saAmfCSINameValueAttrNameId;
} ClSnmpsaAmfCSINameValueTableIndexInfoT;

typedef struct clSnmpsaAmfCSTypeAttrNameTableIndexInfo
{
    ClUint32T saAmfCSTypeNameId;
    ClUint32T saAmfCSTypeAttrNameId;
} ClSnmpsaAmfCSTypeAttrNameTableIndexInfoT;

typedef struct clSnmpsaAmfSUSITableIndexInfo
{
    ClUint32T saAmfSUSISuNameId;
    ClUint32T saAmfSUSISiNameId;
} ClSnmpsaAmfSUSITableIndexInfoT;

typedef struct clSnmpsaAmfHealthCheckTableIndexInfo
{
    ClUint32T saAmfHealthCompNameId;
    ClCharT saAmfHealthCheckKey[32];

} ClSnmpsaAmfHealthCheckTableIndexInfoT;

typedef struct clSnmpsaAmfSCompCsiTableIndexInfo
{
    ClUint32T saAmfSCompCsiCompNameId;
    ClUint32T saAmfSCompCsiCsiNameId;
} ClSnmpsaAmfSCompCsiTableIndexInfoT;

typedef struct clSnmpsaAmfProxyProxiedTableIndexInfo
{
    ClUint32T saAmfProxyProxiedProxyNameId;
    ClUint32T saAmfProxyProxiedProxiedNameId;
} ClSnmpsaAmfProxyProxiedTableIndexInfoT;


/* function prototypes.*/
ClInt32T  clSnmpsafAmfDefaultInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfApplicationTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfNodeTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSGTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSUTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSITableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSUsperSIRankTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSGSIRankTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSGSURankTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSISIDepTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCompTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCompCSTypeSupportedTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCSITableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCSICSIDepTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCSINameValueTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfCSTypeAttrNameTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSUSITableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfHealthCheckTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfSCompCsiTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);
ClInt32T  clSnmpsaAmfProxyProxiedTableInstCompare (ClCntKeyHandleT key1,
                            ClCntKeyHandleT key2);

extern void init_safAmfMIB(void);
/* Global structures.*/
extern ClSnmpOidInfoT appOidInfo[];
extern ClSnmpNtfyCallbackTableT clSnmpAppCallback[];

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_SUBAGENT_SAFAMF_CONFIG_H_ */
