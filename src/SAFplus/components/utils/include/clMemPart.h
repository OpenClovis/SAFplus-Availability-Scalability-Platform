#ifndef _CL_MEM_PART_H_
#define _CL_MEM_PART_H_

#include <clCommon.h>
#include <clOsalApi.h>
#include <memPartLib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ClMemPart;
typedef struct ClMemPart *ClMemPartHandleT;

ClRcT clMemPartInitialize(ClMemPartHandleT *pMemPartHandle, ClUint32T memPartSize);
ClPtrT clMemPartAlloc(ClMemPartHandleT handle, ClUint32T size);
ClPtrT clMemPartRealloc(ClMemPartHandleT handle, ClPtrT memBase, ClUint32T size);
void clMemPartFree(ClMemPartHandleT handle, ClPtrT mem);
ClRcT clMemPartFinalize(ClMemPartHandleT *pHandle);

#ifdef __cplusplus
}
#endif

#endif
