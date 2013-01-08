/* POSIX / linux includes */
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>       /* for AF_INET */
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/* OpenClovis includes */
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clEoApi.h>

/* Local includes */
#include "net.h"

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                           CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, \
                                           __VA_ARGS__)

/* From the RFC
   Ethernet packet data:
   16.bit: (ar$hrd) Hardware address space (e.g., Ethernet,
   Packet Radio Net.)
   16.bit: (ar$pro) Protocol address space.  For Ethernet
   hardware, this is from the set of type
   fields ether_typ$<protocol>.
   8.bit: (ar$hln) byte length of each hardware address
   8.bit: (ar$pln) byte length of each protocol address
   16.bit: (ar$op)  opcode (ares_op$REQUEST | ares_op$REPLY)
   nbytes: (ar$sha) Hardware address of sender of this
   packet, n from the ar$hln field.
   mbytes: (ar$spa) Protocol address of sender of this
   packet, m from the ar$pln field.
   nbytes: (ar$tha) Hardware address of target of this
   packet (if known).
   mbytes: (ar$tpa) Protocol address of target.
*/

typedef struct
{
    ClUint8T  dstMac[6];
    ClUint8T  myMac[6];
    ClUint16T type;

    ClUint16T hrd;
    ClUint16T pro;
    ClUint8T  hln;
    ClUint8T  pln;
    ClUint16T op;
    ClUint8T  sha[6];
    ClUint8T  spa[4];
    ClUint8T  tha[6];
    ClUint8T  tpa[4];
} EthIpv4ArpPacket;

enum
{
    ArpRequest = 1,
    ArpReply   = 2,
    ArpHwTypeEthernet = 1,

    IpProtoType = 0x0800,
    //ArpAddressResolutionType = 
    MacAddrLen  = 6
};

ClRcT DevToMac(const char* dev, char mac[MacAddrLen])
{
    struct ifreq req;

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = AF_UNSPEC;
    if(ioctl(sd, SIOCGIFHWADDR, &req)==-1)
    {
        int err = errno;
        clprintf(CL_LOG_SEV_CRITICAL, "No device %s, hwaddr errno: %d", dev,err);
        return CL_ERR_LIBRARY;
    }
    memcpy(mac, &(req.ifr_addr.sa_data), MacAddrLen); 

    close(sd);
    return CL_OK;
}

ClRcT DevToIfIndex(const char* dev, int* ifindex)
{
    struct ifreq req;

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = AF_UNSPEC;
    if (ioctl(sd, SIOCGIFINDEX, &req)==-1)
    {
        int err = errno;
        clprintf(CL_LOG_SEV_CRITICAL, "No device %s, ifindex errno: %d", dev,err);
        *ifindex = 0;
        return CL_ERR_LIBRARY;
    }
    
    *ifindex = req.ifr_ifindex;
 
    close(sd);
    return CL_OK;
}

ClRcT HostToIp(const char* myHost, unsigned int* ip)
{
    struct hostent* host = NULL;
    struct hostent hostdata;
    char buf[129];
    int errnum = 0;
  
    if (gethostbyname_r(myHost,&hostdata,buf,128,&host,&errnum)
        || !host)
    {
        clprintf(CL_LOG_SEV_CRITICAL, "Cannot resolve host string %s", myHost);        
        return CL_ERR_LIBRARY;
    }
 
    *ip = *((unsigned int*) host->h_addr_list[0]);
    return CL_OK; 
}

ClRcT SendArp(const char* host, const char* dev)
{
    int i;
    ClRcT rc = CL_OK;
    char myMac[MacAddrLen];
    unsigned int myIp = 0;
    EthIpv4ArpPacket pkt;
    int ifindex = 0;

    
    DevToMac(dev,myMac);
    if((rc = HostToIp(host,&myIp)) != CL_OK)
    {
      return rc;
    }

    DevToIfIndex(dev, &ifindex);

    for (i = ArpRequest; i<= ArpReply; i++)
    {
        memcpy(pkt.myMac, myMac, MacAddrLen);
        memset(pkt.dstMac, 0xFF, MacAddrLen);
        
        pkt.type = htons(ETHERTYPE_ARP);

        pkt.hrd = htons(ArpHwTypeEthernet);
        pkt.pro = htons(IpProtoType); //htons(ETHERTYPE_IP); //htons(ETH_P_IP); //ArpAddressResolutionType;
        pkt.hln = MacAddrLen;  /* ETH_HW_ADDR_LEN; Length in bytes of ethernet address */
        pkt.pln = 4; // IP_ADDR_LEN;
        pkt.op  = htons(i);     // htons(ARPOP_REQUEST);

        memcpy(pkt.sha,myMac,MacAddrLen);
        memset(pkt.tha, 0xFF, MacAddrLen);
        memcpy(pkt.spa,&myIp,4);
        memcpy(pkt.tpa,&myIp,4);

        if (1)
        {
            int             sd;
            struct sockaddr_ll sal; 

            bzero(&sal,sizeof(sal)); 
            sal.sll_family    = AF_PACKET; 
            sal.sll_protocol  = htons(ETH_P_ARP); 
            sal.sll_ifindex   = ifindex; 
            sal.sll_hatype    = htons(i); 
            sal.sll_pkttype   = PACKET_BROADCAST; 
            memcpy(sal.sll_addr, myMac, MacAddrLen); 
            sal.sll_halen     = MacAddrLen;

            if ((sd = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ARP))) < 0) 
            { 
                int err = errno;
                clprintf(CL_LOG_SEV_CRITICAL, "Cannot create a socket, arp not sent; error %d: %s", err, strerror(err)); 
            }
            else
            {
                if (sendto(sd,&pkt,sizeof(pkt),0,(struct sockaddr *)&sal,sizeof(sal)) < 0)
                { 
                    int err = errno;
                    clprintf(CL_LOG_SEV_CRITICAL, "Cannot send the arp packet; error %d: %s", err, strerror(err));
                    rc = CL_ERR_LIBRARY; 
                }
                clprintf(CL_LOG_SEV_INFO, "Gratuitous arp sent: IP %s device %s mac %02x:%02x:%02x:%02x:%02x:%02x",host, dev,myMac[0],myMac[1],myMac[2],myMac[3],myMac[4],myMac[5]);
                shutdown(sd,SHUT_RDWR);
                close(sd);
            }            
        }
    }
    return CL_OK;
}

//void GetVirtualAddressInfoAsp(ClAmsCSIDescriptorT* csiDescriptor, VirtualIpAddress* vip)
//{
//    int i;
//    vip->ip[0] = 0;
//    vip->netmask[0] = 0;
//    vip->dev[0] = 0;
//
//    /* Pull the appropriate values out of the workload dictionary */
//    for (i = 0; i < csiDescriptor->csiAttributeList.numAttributes; i++)
//    {
//        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualIpAddress") == 0)
//            strncpy(vip->ip, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue, VipFieldLen);
//        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualNetMask") == 0)
//            strncpy(vip->netmask, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue,VipFieldLen);
//        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualDevice") == 0)
//            strncpy(vip->dev, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue,VipFieldLen);
//    }
//
//}
void GetVirtualAddressInfoAsp(ClAmsCSIDescriptorT* csiDescriptor, VirtualIpAddress* vip)
{
    int i;
    vip->ip[0] = 0;
    vip->netmask[0] = 0;
    vip->dev[0] = 0;

    /* Pull the appropriate values out of the workload dictionary */
    for (i = 0; i < csiDescriptor->csiAttributeList.numAttributes; i++)
    {
        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualIpAddress") == 0)
            strncpy(vip->ip, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue, VipFieldLen);
        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualNetMask") == 0)
            strncpy(vip->netmask, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue,VipFieldLen);
        if (strcmp((const char*) csiDescriptor->csiAttributeList.attribute[i].attributeName,"VirtualDevice") == 0)
            strncpy(vip->dev, (const char*) csiDescriptor->csiAttributeList.attribute[i].attributeValue,VipFieldLen);
    }

}


void GetVirtualAddressInfo(SaAmfCSIDescriptorT* csiDescriptor, VirtualIpAddress* vip)
{
    int i;
    vip->ip[0] = 0;
    vip->netmask[0] = 0;
    vip->dev[0] = 0;

    /* Pull the appropriate values out of the workload dictionary */
    for (i = 0; i < csiDescriptor->csiAttr.number; i++)
    {
        if (strcmp((const char*) csiDescriptor->csiAttr.attr[i].attrName,"VirtualIpAddress") == 0) 
            strncpy(vip->ip, (const char*) csiDescriptor->csiAttr.attr[i].attrValue, VipFieldLen);
        if (strcmp((const char*) csiDescriptor->csiAttr.attr[i].attrName,"VirtualNetMask") == 0)   
            strncpy(vip->netmask, (const char*) csiDescriptor->csiAttr.attr[i].attrValue,VipFieldLen);
        if (strcmp((const char*) csiDescriptor->csiAttr.attr[i].attrName,"VirtualDevice") == 0) 
            strncpy(vip->dev, (const char*) csiDescriptor->csiAttr.attr[i].attrValue,VipFieldLen);
    }
  
}

/* We want to make extremely sure that the IP address changes across the network.  But ARP table
   updates in all the devices on the network are unreliable since they are dependent on lots of
   embedded implementations in switches, routers, etc.
   So we send a bunch of gratituous ARPs spaced across time to get the message across!
*/
static void *pummelArps(void *arg)
{
    int i;
    VirtualIpAddress* vip = (VirtualIpAddress*) arg;
    
    for (i=0;i<5;i++) 
    {
        sleep(1);
        SendArp(vip->ip,vip->dev);    
    }
    free(arg);
    return NULL;
}


void AddRemVirtualAddress(const char *cmd,const VirtualIpAddress* vip)
{
    int up = 0;
    if (cmd[0] == 'u') up = 1;
    
    
    VirtualIpAddress* vipCopy = (VirtualIpAddress*)malloc(sizeof(VirtualIpAddress));
    memcpy(vipCopy,vip,sizeof(VirtualIpAddress));
    
    if (vipCopy->ip && vipCopy->dev && vipCopy->netmask)
    {
        char execLine[301];
        snprintf(execLine,300,"%s/virtualIp %s %s %s %s ",CL_APP_BINDIR,cmd,vipCopy->ip,vipCopy->netmask,vipCopy->dev);
        printf("Executing %s", execLine);
        system(execLine);
      
        if (up)  /* If we are coming up, do a gratuitous arp */
        {
            clprintf(CL_LOG_SEV_INFO, "Sending gratuitous arps: IP address: %s, device: %s",vipCopy->ip,vipCopy->dev);
            SendArp(vipCopy->ip,vipCopy->dev);
            clOsalTaskCreateDetached ("arpTask", CL_OSAL_SCHED_OTHER, 0, 0, pummelArps, vipCopy);
        }
        else
        {
            /* If we are going down, delay a bit so that the machine that takes over will not do so too soon.
               (the assumption being that the machine taking over will not do so until this remove returns)
             */
            clprintf(CL_LOG_SEV_INFO, "Removing IP; not sending gratuitous arps");
            /* sleep(1); */
        }        
    }
    else
    {
        clprintf(CL_LOG_SEV_ERROR, "Virtual IP work assignment values incorrect: got IP address: %s, device: %s, mask: %s",vipCopy->ip, vipCopy->dev, vipCopy->netmask);
    }
    
}
