#ifndef _CL_FAULT_SERVER_FUNC_TABLE_H_
#define _CL_FAULT_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FAULT_FUNC_ID(n) CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, n)    
/* function table */
CL_EO_CALLBACK_TABLE_DECL(clFaultFuncList)[] =
{

    VSYM_EMPTY(NULL, FAULT_FUNC_ID(0)),                         /* 0 */
    VSYM(clFaultReportProcess, CL_FAULT_REPORT_API_HANDLER),    /* CL_FAULT_REPORT_API_HANDLER */        
    VSYM(clFaultServerRepairAction, CL_FAULT_REPAIR_API_HANDLER), /*CL_FAULT_REPAIR_API_HANDLER */
    VSYM_NULL
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, FLT)[] = 
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, clFaultFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif

#endif
