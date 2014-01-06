
#ifndef _CL_GMS_CLIENT_TABLE_REGISTER_H_
#define _CL_GMS_CLIENT_TABLE_REGISTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#undef  __SERVER__
#define __CLIENT__
#include <clGmsServerFuncTable.h>

ClRcT clGmsClientTableRegister(ClEoExecutionObjT* eo);

ClRcT clGmsClientTableDeregister(ClEoExecutionObjT* eo);

#ifdef __cplusplus
}
#endif

#endif

