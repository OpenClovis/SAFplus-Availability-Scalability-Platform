#ifndef _CL_SNMP_OCTRAIN_INST_XLATION_H_
#define _CL_SNMP_OCTRAIN_INST_XLATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clMedApi.h>

#define BY_COR 0

ClRcT clSnmpocTrainDefaultInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpclockTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmptimeSetTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpnameTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
#ifdef __cplusplus
}
#endif
#endif /* _CL_SNMP_OCTRAIN_INST_XLATION_H_ */
