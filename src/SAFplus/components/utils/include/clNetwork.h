#ifndef _CL_NETWORK_H_
#define _CL_NETWORK_H_

#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

static int __endianType = -1;
static const int __endianTest = 1;

static  __inline__ ClInt64T clHtonl64(ClInt64T v)
{
    ClUint32T v1 = v & 0xffffffffU;
    ClUint32T v2 = (v >> 32) & 0xffffffffU;
    if(__endianType < 0)
    {
        __endianType = (*(char*)&__endianTest == 1 )  ? 0 : 1;
    }
    if(!__endianType)
    {
        /*
         *Swap if little endian
         */
        v1 ^= v2;
        v2 ^= v1;
        v1 ^= v2;
        v1 = htonl(v1);
        v2 = htonl(v2);
    }
    v = ((ClInt64T)v2 << 32) | v1;
    return v;
}

static __inline__ ClInt64T clNtohl64(ClInt64T v)
{
    ClUint32T v1 = v & 0xffffffffU;
    ClUint32T v2 = (v >> 32) & 0xffffffffU;
    if(__endianType < 0)
    {
        __endianType = ( *(char*)&__endianTest == 1 ) ? 0 : 1;
    }
    if(!__endianType)
    {
        v1 ^= v2;
        v2 ^= v1;
        v1 ^= v2;
        v1 = ntohl(v1);
        v2 = ntohl(v2);
    }
    v = ((ClInt64T)v2 << 32) | v1;
    return v;
}

#ifdef __cplusplus
}
#endif

#endif
