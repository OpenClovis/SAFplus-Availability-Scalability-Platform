#include "saAmf.h"

enum
  {
  VipFieldLen = 64
  };

typedef struct
{
  char ip[VipFieldLen];
  char dev[VipFieldLen];
  char netmask[VipFieldLen];
} VirtualIpAddress;

void AddRemVirtualAddress(const char *cmd,const VirtualIpAddress* vip);
void GetVirtualAddressInfo(SaAmfCSIDescriptorT* csiDescriptor, VirtualIpAddress* vip);
void GetVirtualAddressInfoAsp(ClAmsCSIDescriptorT* csiDescriptor, VirtualIpAddress* vip);
ClRcT SendArp(const char* host, const char* dev);
ClRcT HostToIp(const char* myHost, unsigned int* ip);
ClRcT DevToMac(const char* dev, char mac[6]);
