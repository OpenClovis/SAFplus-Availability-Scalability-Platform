#ifndef _CL_GMS_SERVER_RMD_TABLE_H_
#define _CL_GMS_SERVER_RMD_TABLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#undef  __CLIENT__
#define __SERVER__
#include <clGmsClientRmdFunc.h>

ClRcT clGmsClientRmdTableInstall(ClEoExecutionObjT* eo);

ClRcT clGmsClientRmdTableUnInstall(ClEoExecutionObjT* eo);


#ifdef __cplusplus
}
#endif

#endif


