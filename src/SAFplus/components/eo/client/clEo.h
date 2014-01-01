#ifndef _CL_EO_H_
#define _CL_E0_H_

#ifdef __cplusplus
extern "C" {
#endif

#define  CL_LOG_AREA "EO"
#define  CL_LOG_CTXT_INI "INI"
#define  CL_LOG_CTXT_FIN "FIN"


/*
 * List of Library Initialize Functions 
 */
typedef ClRcT (*ClInitFinalizeFunc) (void);

typedef struct
{
    ClInitFinalizeFunc fn;
    const char*        libName;
} ClInitFinalizeDef;

/*
 * These declarations are present here to satisfy the compiler. The actual
 * function declarations and definitions must be present in the component's
 * Api.h files in the component's <include> directory. 
 */

extern ClRcT clIocLibInitialize(ClPtrT pConfig);
extern ClRcT clIocLibFinalize(void);

extern ClRcT clRmdLibInitialize(ClPtrT pConfig);
extern ClRcT clRmdLibFinalize(void);

extern ClRcT clOmLibInitialize(void);
extern ClRcT clOmLibFinalize(void);

extern ClRcT clHalLibInitialize(void);
extern ClRcT clHalLibFinalize(void);

extern ClRcT clDbalLibInitialize(void);
extern ClRcT clDbalLibFinalize(void);

extern ClRcT clCorClientInitialize(void);
extern ClRcT clCorClientFinalize(void);


extern ClRcT clNameLibInitialize(void);
extern ClRcT clNameLibFinalize(void);

extern ClRcT clTraceLibInitialize(void);
extern ClRcT clTraceLibFinalize(void);

extern ClRcT clTxnLibInitialize(void);
extern ClRcT clTxnLibFinalize(void);

extern ClRcT clMsoLibInitialize(void);
extern ClRcT clMsoLibFinalize(void);

extern ClRcT clProvInitialize(void);
extern ClRcT clProvFinalize(void);

extern ClRcT clAlarmLibInitialize(void);
extern ClRcT clAlarmLibFinalize(void);

extern ClRcT clGmsLibInitialize(void);
extern ClRcT clGmsLibFinalize(void);

extern ClRcT clPMLibInitialize(void);
extern ClRcT clPMLibFinalize(void);

extern ClRcT clEoPriorityQueuesFinalize(ClBoolT force);

#ifdef __cplusplus
}
#endif


#endif
