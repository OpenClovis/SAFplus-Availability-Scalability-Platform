#ifndef _RULE_H_
#define _RULE_H_

# ifdef __cplusplus
extern "C" {
# endif

ClRcT rbeExprPrintOne (ClRuleExprT* expr);

ClRcT rbeExprConvertOne (ClRuleExprT* expr);

ClRuleResultT rbeExprEvaluateOne (ClRuleExprT *expr, ClUint32T *data, int dataLen);

ClRuleResultT clRuleExprEvaluateOne (ClRuleExprT* expr1, ClRuleExprT* expr2);

ClRcT clRuleExprPrintToMessageBuffer(ClDebugPrintHandleT msgHandle, ClRuleExprT* pExpr);

ClRuleResultT clRuleExprEvaluate (ClRuleExprT* expr, ClUint32T *data, int dataLen);

ClRuleResultT clRuleExprEvaluateOne (ClRuleExprT* expr1, ClRuleExprT* expr2);

#ifdef __cplusplus
 }
#endif

#endif

