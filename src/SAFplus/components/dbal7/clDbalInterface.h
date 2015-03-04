

#ifndef _CL_DBAL_INTERFACE_H_
#define _CL_DBAL_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

ClRcT clDbalInterface(ClDbalFunctionPtrsT  *funcDbPtr);

ClRcT clDbalEngineFinalize();

ClRcT clDbalConfigInitialize(void* pDbalConfiguration);

#ifdef __cplusplus
 }
#endif

#endif
