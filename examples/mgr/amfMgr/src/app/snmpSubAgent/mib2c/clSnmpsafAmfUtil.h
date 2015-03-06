#ifndef _CL_SNMP_SAFAMF_UTIL_H_
#define _CL_SNMP_SAFAMF_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <clSnmpLog.h>

#define CL_SNMP_GEN_OP_COTX "GOP"
#define CL_SNMP_GEN_CMP_COTX "GCM"

typedef enum SAAMFALARMSERVICEIMPAIRED_ENUM {
    SAAMFALARMSERVICEIMPAIRED_SAAMFPROBABLECAUSE, 
} saAmfAlarmServiceImpairedEnumType;
typedef enum SAAMFALARMCOMPINSTANTIATIONFAILED_ENUM {
    SAAMFALARMCOMPINSTANTIATIONFAILED_SAAMFCOMPNAME, 
} saAmfAlarmCompInstantiationFailedEnumType;
typedef enum SAAMFALARMCOMPCLEANUPFAILED_ENUM {
    SAAMFALARMCOMPCLEANUPFAILED_SAAMFCOMPNAME, 
} saAmfAlarmCompCleanupFailedEnumType;
typedef enum SAAMFALARMCLUSTERRESET_ENUM {
    SAAMFALARMCLUSTERRESET_SAAMFCOMPNAME, 
} saAmfAlarmClusterResetEnumType;
typedef enum SAAMFALARMSIUNASSIGNED_ENUM {
    SAAMFALARMSIUNASSIGNED_SAAMFSINAME, 
} saAmfAlarmSIUnassignedEnumType;
typedef enum SAAMFALARMPROXIEDCOMPUNPROXIED_ENUM {
    SAAMFALARMPROXIEDCOMPUNPROXIED_SAAMFCOMPNAME, 
} saAmfAlarmProxiedCompUnproxiedEnumType;
typedef enum SAAMFSTATECHGCLUSTERADMIN_ENUM {
    SAAMFSTATECHGCLUSTERADMIN_SAAMFCLUSTERADMINSTATE, 
} saAmfStateChgClusterAdminEnumType;
typedef enum SAAMFSTATECHGAPPLICATIONADMIN_ENUM {
    SAAMFSTATECHGAPPLICATIONADMIN_SAAMFAPPLICATIONNAME, 
    SAAMFSTATECHGAPPLICATIONADMIN_SAAMFAPPLICATIONADMINSTATE, 
} saAmfStateChgApplicationAdminEnumType;
typedef enum SAAMFSTATECHGNODEADMIN_ENUM {
    SAAMFSTATECHGNODEADMIN_SAAMFNODENAME, 
    SAAMFSTATECHGNODEADMIN_SAAMFNODEADMINSTATE, 
} saAmfStateChgNodeAdminEnumType;
typedef enum SAAMFSTATECHGSGADMIN_ENUM {
    SAAMFSTATECHGSGADMIN_SAAMFSGNAME, 
    SAAMFSTATECHGSGADMIN_SAAMFSGADMINSTATE, 
} saAmfStateChgSGAdminEnumType;
typedef enum SAAMFSTATECHGSUADMIN_ENUM {
    SAAMFSTATECHGSUADMIN_SAAMFSUNAME, 
    SAAMFSTATECHGSUADMIN_SAAMFSUADMINSTATE, 
} saAmfStateChgSUAdminEnumType;
typedef enum SAAMFSTATECHGSIADMIN_ENUM {
    SAAMFSTATECHGSIADMIN_SAAMFSINAME, 
    SAAMFSTATECHGSIADMIN_SAAMFSIADMINSTATE, 
} saAmfStateChgSIAdminEnumType;
typedef enum SAAMFSTATECHGNODEOPER_ENUM {
    SAAMFSTATECHGNODEOPER_SAAMFNODENAME, 
    SAAMFSTATECHGNODEOPER_SAAMFNODEOPERATIONALSTATE, 
} saAmfStateChgNodeOperEnumType;
typedef enum SAAMFSTATECHGSUOPER_ENUM {
    SAAMFSTATECHGSUOPER_SAAMFSUNAME, 
    SAAMFSTATECHGSUOPER_SAAMFSUOPERSTATE, 
} saAmfStateChgSUOperEnumType;
typedef enum SAAMFSTATECHGSUPRESENCE_ENUM {
    SAAMFSTATECHGSUPRESENCE_SAAMFSUNAME, 
    SAAMFSTATECHGSUPRESENCE_SAAMFSUPRESENCESTATE, 
} saAmfStateChgSUPresenceEnumType;
typedef enum SAAMFSTATECHGSUHASTATE_ENUM {
    SAAMFSTATECHGSUHASTATE_SAAMFSUSISUNAME, 
    SAAMFSTATECHGSUHASTATE_SAAMFSUSISINAME, 
    SAAMFSTATECHGSUHASTATE_SAAMFSUSIHASTATE, 
} saAmfStateChgSUHaStateEnumType;
typedef enum SAAMFSTATECHGSIASSIGNMENT_ENUM {
    SAAMFSTATECHGSIASSIGNMENT_SAAMFSINAME, 
    SAAMFSTATECHGSIASSIGNMENT_SAAMFSIASSIGNMENTSTATE, 
} saAmfStateChgSIAssignmentEnumType;
typedef enum SAAMFSTATECHGUNPROXIEDCOMPPROXIED_ENUM {
    SAAMFSTATECHGUNPROXIEDCOMPPROXIED_SAAMFCOMPNAME, 
} saAmfStateChgUnproxiedCompProxiedEnumType;

ClRcT clSnmpsaAmfAlarmServiceImpairedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfAlarmCompInstantiationFailedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfAlarmCompCleanupFailedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfAlarmClusterResetIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfAlarmSIUnassignedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfAlarmProxiedCompUnproxiedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgClusterAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgApplicationAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgNodeAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSGAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSUAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSIAdminIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgNodeOperIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSUOperIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSUPresenceIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSUHaStateIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgSIAssignmentIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfStateChgUnproxiedCompProxiedIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);


ClRcT clSnmpsaAmfApplicationTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfNodeTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSGTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSUTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSUsperSIRankTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSISIDepTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCompTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCompCSTypeSupportedTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCSICSIDepTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCSINameValueTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfCSTypeAttrNameTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfSUSITableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpsaAmfHealthCheckTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);

ClRcT clSnmpsaAmfApplicationTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfNodeTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSGTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSUTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSITableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSUsperSIRankTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSGSIRankTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSGSURankTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSISIDepTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCompTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCompCSTypeSupportedTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCSITableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCSICSIDepTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCSINameValueTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfCSTypeAttrNameTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSUSITableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfHealthCheckTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfSCompCsiTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpsaAmfProxyProxiedTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);

#ifdef __cplusplus
}
#endif
#endif /*  _CL_SNMP_SAFAMF_UTIL_H_ */
