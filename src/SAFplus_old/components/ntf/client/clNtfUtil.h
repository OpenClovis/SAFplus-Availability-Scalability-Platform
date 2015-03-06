
#ifndef _CL_NTF_UTILS_H_
#define _CL_NTF_UTILS_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <saNtf.h>

extern ClFdT clNtfNotificationIdFileOpen();

extern void clNtfNotificationIdFileClose(ClFdT fd);

extern SaNtfIdentifierT  clNtfNotificationIdGenerate(ClFdT fd, ClIocNodeAddressT nodeId);

# ifdef __cplusplus
}
# endif

#endif
