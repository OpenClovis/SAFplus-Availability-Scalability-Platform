#ifndef _CL_SNMP_SAFAMF_INST_XLATION_H_
#define _CL_SNMP_SAFAMF_INST_XLATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clMedApi.h>

#define BY_COR 0

ClRcT clSnmpsafAmfDefaultInstXlator (CL_IN const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfApplicationTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfNodeTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSGTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSUTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSITableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSUsperSIRankTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSGSIRankTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSGSURankTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSISIDepTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCompTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCompCSTypeSupportedTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCSITableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCSICSIDepTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCSINameValueTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfCSTypeAttrNameTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSUSITableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfHealthCheckTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfSCompCsiTableInstXlator (const struct ClMedAgentId *pAgntId,
                     CL_IN ClCorMOIdPtrT         hmoId,
                     CL_IN ClCorAttrPathPtrT containedPath,
                     CL_OUT void**         pRetInstData,
                     CL_OUT ClUint32T     *pInstLen,
                     CL_OUT ClPtrT        pCorAttrValueDescList,
                     CL_IN ClMedInstXlationOpEnumT instXlnOp,
                     CL_IN ClPtrT cookie);
ClRcT clSnmpsaAmfProxyProxiedTableInstXlator (const struct ClMedAgentId *pAgntId,
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
#endif /* _CL_SNMP_SAFAMF_INST_XLATION_H_ */
