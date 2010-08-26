#ifndef _CL_RADIX_TREE_H_
#define _CL_RADIX_TREE_H_

#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ClPtrT ClRadixTreeHandleT;

ClRcT clRadixTreeInit(ClRadixTreeHandleT *handle);
ClRcT clRadixTreeInsert(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT item, ClPtrT *lastItem);
ClRcT clRadixTreeLookup(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT *item);
ClRcT clRadixTreeDelete(ClRadixTreeHandleT handle, ClUint32T index, ClPtrT *item);

#ifdef __cplusplus
}
#endif

#endif
