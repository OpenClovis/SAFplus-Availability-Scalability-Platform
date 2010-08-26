#ifndef _CL_NETWORK_H_
#define _CL_NETWORK_H_

#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

static  __inline__ ClInt64T clHtonl64(ClInt64T v)
{
    ClUint32T v1 = htonl(v & 0xffffffffU);
    ClUint32T v2 = htonl( (v >> 32) & 0xffffffffU);
    v = ((ClInt64T)v2 << 32) | v1;
    return v;
}

static __inline__ ClInt64T clNtohl64(ClInt64T v)
{
    ClUint32T v1 = ntohl(v & 0xffffffffU);
    ClUint32T v2 = ntohl( (v >> 32) & 0xffffffffU);
    v = ((ClInt64T)v2 << 32) | v1;
    return v;
}

#ifdef __cplusplus
}
#endif

#endif
