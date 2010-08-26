#ifndef _CL_SNMP_OCTRAIN_UTIL_H_
#define _CL_SNMP_OCTRAIN_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <clSnmpLog.h>

#define CL_SNMP_GEN_OP_COTX "GOP"
#define CL_SNMP_GEN_CMP_COTX "GCM"

typedef enum ALARMTRAP_ENUM {
    ALARMTRAP_CLOCKID, 
} alarmTrapEnumType;

ClRcT clSnmpalarmTrapIndexGet(ClUint32T objectType, ClCorMOIdPtrT pMoId, ClAlarmUtilTlvInfoPtrT pTlvList);


ClRcT clSnmpclockTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);
ClRcT clSnmpnameTableIndexTlvGet(ClPtrT pIndexInfo, ClAlarmUtilTlvInfoPtrT pTlvList);

ClRcT clSnmpclockTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmptimeSetTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);
ClRcT clSnmpnameTableIndexCorAttrGet(ClPtrT pInst, ClPtrT pCorAttrValueDescList);

#ifdef __cplusplus
}
#endif
#endif /*  _CL_SNMP_OCTRAIN_UTIL_H_ */
