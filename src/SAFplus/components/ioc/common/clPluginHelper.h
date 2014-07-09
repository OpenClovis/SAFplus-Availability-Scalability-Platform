#ifndef _CL_PLUGIN_HELPER_H_
#define _CL_PLUGIN_HELPER_H_

#define clBitSet(bitNum) ((bitNum) |= 0x80000000)
#define ASP_PLUGIN_SUBNET_PREFIX_DEFAULT "24"
#define ASP_PLUGIN_SUBNET_DEFAULT "169.254.100/" ASP_PLUGIN_SUBNET_PREFIX_DEFAULT
#define CL_MAX_FIELD_LENGTH 64
#define CL_MAC_ADDRESS_LENGTH 6

#ifdef __cplusplus
extern "C" {
#endif

#include <clIocApi.h>

typedef struct {
    ClCharT ip[CL_MAX_FIELD_LENGTH];
    ClCharT dev[CL_MAX_FIELD_LENGTH];
    ClCharT netmask[CL_MAX_FIELD_LENGTH];
    ClCharT broadcast[CL_MAX_FIELD_LENGTH];
    ClCharT subnetPrefix[CL_MAX_FIELD_LENGTH];
    ClUint32T ipAddressMask;
} ClPluginHelperVirtualIpAddressT;

typedef struct {
    ClUint8T dstMac[6];
    ClUint8T myMac[6];
    ClUint16T type;

    ClUint16T hrd;
    ClUint16T pro;
    ClUint8T hln;
    ClUint8T pln;
    ClUint16T op;
    ClUint8T sha[6];
    ClUint8T spa[4];
    ClUint8T tha[6];
    ClUint8T tpa[4];
} ClPluginHelperEthIpv4ArpPacketT;

void clPluginHelperAddRemVirtualAddress(const char *cmd, const ClPluginHelperVirtualIpAddressT *vip);
ClRcT clPluginHelperGetVirtualAddressInfo(const ClCharT *xportType, ClPluginHelperVirtualIpAddressT *vip);
void clPluginHelperGetIpAddress(const ClUint32T ipAddressMask, const ClIocNodeAddressT iocAddress, ClCharT *hostAddress);

ClUint32T clPluginHelperBitFillRShift(ClUint32T numBits);
ClRcT clPluginHelperConvertHostToInternetAddress(ClUint32T addr, ClCharT *internetAddress);
ClRcT clPluginHelperConvertInternetToHostAddress(ClUint32T *addr, const ClCharT *internetAddress);
ClRcT clPluginHelperDevToIpAddress(const ClCharT *dev, ClCharT *addrStr);

void clPluginHelperAddRouteAddress(const ClCharT *ipAddress, const ClCharT *ifDevName);

#ifdef __cplusplus
}
#endif
#endif                          /* _CL_PLUGIN_HELPER_H_ */
