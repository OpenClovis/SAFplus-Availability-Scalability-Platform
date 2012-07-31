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
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <clEoApi.h>
#include <clPluginHelper.h>

#define CL_LOG_PLUGIN_HELPER_AREA "PLUGIN_HELPER"
#define CL_PLUGIN_HELPER_ARP_REQUEST (1)
#define CL_PLUGIN_HELPER_ARP_REPLY (2)
#define CL_PLUGIN_HELPER_ARP_HW_TYPE_ETHERNET (1)
#define CL_PLUGIN_HELPER_IP_PROTO_TYPE 0x0800

extern ClIocNodeAddressT gIocLocalBladeAddress;

/*
 *  Network to Host
 *
 *  Convert from network to host type
 */
static ClRcT _clPluginHelperConvertInternetToHostAddress(ClUint32T *addr, ClCharT *internetAddress) {
    ClUint32T val[4] = { 0 };
    *addr = 0;
    ClCharT *token = NULL;
    ClCharT *nextToken = NULL;
    ClInt32T n = 0;

    token = strtok_r(internetAddress, ".", &nextToken);
    while (token) {
        val[n++] = atoi(token);
        token = strtok_r(NULL, ".", &nextToken);
    }

    if (val[0] > 255 || val[1] > 255 || val[2] > 255 || val[3] > 255) {
        return CL_ERR_INVALID_PARAMETER;
    }

    *addr |= val[3];
    val[2] <<= 8;
    *addr |= val[2];
    val[1] <<= 16;
    *addr |= val[1];
    val[0] <<= 24;
    *addr |= val[0];

    return CL_OK;

}

/*
 *  Host to Network
 *
 *  Convert from host to network type
 */
static ClRcT _clPluginHelperConvertHostToInternetAddress(ClUint32T addr, ClCharT *internetAddress) {

    ClUint32T val1, val2, val3, val4;

    val1 = addr & 0xff000000;
    val1 = val1 >> 24;
    val2 = addr & 0x00ff0000;
    val2 = val2 >> 16;
    val3 = addr & 0x0000ff00;
    val3 = val3 >> 8;
    val4 = addr & 0x000000ff;

    snprintf(internetAddress, CL_MAX_NAME_LENGTH, "%u.%u.%u.%u", val1, val2, val3, val4);

    return CL_OK;

}

/*
 * Fill and shift right bit
 * numBits range: 1 - 31
 */
static ClUint32T _clPluginHelperBitFillRShift(ClUint32T numBits) {
    ClUint32T mask = ~0U;
    mask <<= (32 - numBits);
    return mask;
}

static ClRcT _clPluginHelperGetLinkName(const ClCharT *xportType, ClCharT *inf) {
    ClCharT net_addr[CL_MAX_FIELD_LENGTH] = "eth0";
    ClCharT *linkName = NULL;
    ClCharT envlinkNameType[CL_MAX_FIELD_LENGTH] = { 0 };
    if (!xportType) {
        return CL_ERR_INVALID_PARAMETER;
    } else {
        snprintf(envlinkNameType, CL_MAX_FIELD_LENGTH, "ASP_%s_LINK_NAME", xportType);
        linkName = getenv(envlinkNameType);
        if (linkName == NULL)
        {
            // try with default LINK_NAME
            linkName = getenv("LINK_NAME");
            if (linkName == NULL)
            {
                clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA,
                        "%s and LINK_NAME environment variable is not exported. Using 'eth0:10' interface as default", envlinkNameType);
            } else {
                clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "LINK_NAME env is exported. Value is %s", linkName);
                net_addr[0] = 0;
                strncat(net_addr, linkName, sizeof(net_addr)-1);
                ClCharT *token = NULL;
                strtok_r(net_addr, ":", &token);
            }
            snprintf(inf, CL_MAX_FIELD_LENGTH, "%s:%d", net_addr, gIocLocalBladeAddress + 10);
        } else {
            clLogInfo("IOC", CL_LOG_PLUGIN_HELPER_AREA, "%s env is exported. Value is %s", envlinkNameType, linkName);
            snprintf(inf, CL_MAX_FIELD_LENGTH, "%s", linkName);
        }
    }
    return CL_OK;
}

static ClRcT _clPluginHelperGetIpNodeAddress(const ClCharT *xportType, ClCharT *hostAddress, ClCharT *networkMask, ClCharT *broadcast, ClUint32T *ipAddressMask, ClCharT *xportSubnetPrefix) {
    ClCharT envSubNetType[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT xportSubnet[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT *subnetMask = NULL;
    ClCharT *subnetPrefix = NULL;
    ClUint32T CIDR, ipMask, ip, mask;
    if (!xportType) 
    {
        return CL_ERR_INVALID_PARAMETER;
    } 
    else 
    {
        snprintf(envSubNetType, CL_MAX_FIELD_LENGTH, "ASP_%s_SUBNET", xportType);
        subnetMask = getenv(envSubNetType);
        if (subnetMask == NULL) 
        {
            subnetMask = ASP_PLUGIN_SUBNET_DEFAULT;
        }
        subnetPrefix = strrchr(subnetMask, '/');
        if(!subnetPrefix)
        {
            subnetPrefix = ASP_PLUGIN_SUBNET_PREFIX_DEFAULT; 
        }
        else 
        {
            ++subnetPrefix;
        }
        if(! (CIDR = atoi(subnetPrefix) ) )
        {
            clLogInfo("IOC", CL_LOG_PLUGIN_HELPER_AREA, "%s", subnetMask);
            return CL_ERR_INVALID_PARAMETER;
        }
        xportSubnetPrefix[0] = 0;
        strncat(xportSubnetPrefix, subnetPrefix, CL_MAX_FIELD_LENGTH-1);
        snprintf(xportSubnet, sizeof(xportSubnet), "%s", subnetMask);
        mask = _clPluginHelperBitFillRShift(CIDR);
        _clPluginHelperConvertInternetToHostAddress(&ip, xportSubnet);

        /* network address */
        ipMask = (ip & mask);
        _clPluginHelperConvertHostToInternetAddress(ipMask + gIocLocalBladeAddress, hostAddress);
        _clPluginHelperConvertHostToInternetAddress(mask, networkMask);
        _clPluginHelperConvertHostToInternetAddress(ip | ~mask, broadcast);
        *ipAddressMask = ipMask;
    }
    return CL_OK;
}

static ClRcT _clPluginHelperDevToMac(const ClCharT* dev, char mac[CL_MAC_ADDRESS_LENGTH]) {
    struct ifreq req;

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = AF_UNSPEC;
    if (ioctl(sd, SIOCGIFHWADDR, &req) == -1) {
        int err = errno;
        clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "No device %s, hwaddr errno: %d", dev, err);
        return CL_ERR_LIBRARY;
    }
    memcpy(mac, &(req.ifr_addr.sa_data), CL_MAC_ADDRESS_LENGTH);

    close(sd);
    return CL_OK;
}

static ClRcT _clPluginHelperDevToIfIndex(const ClCharT *dev, int *ifindex) {
    struct ifreq req;

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = AF_UNSPEC;
    if (ioctl(sd, SIOCGIFINDEX, &req) == -1) {
        int err = errno;
        clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "No device %s, ifindex errno: %d", dev, err);
        *ifindex = 0;
        return CL_ERR_LIBRARY;
    }

    *ifindex = req.ifr_ifindex;

    close(sd);
    return CL_OK;
}

static ClRcT _clPluginHelperHostToIp(const ClCharT *myHost, ClUint32T *ip) {
    struct hostent *host = NULL;
    struct hostent hostdata;
    ClCharT buf[129];
    int errnum = 0;

    if (gethostbyname_r(myHost, &hostdata, buf, 128, &host, &errnum) || !host) {
        clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Cannot resolve host string %s", myHost);
        return CL_ERR_LIBRARY;
    }

    *ip = *((ClUint32T*) host->h_addr_list[0]);
    return CL_OK;
}

static ClRcT _clPluginHelperSendArp(const ClCharT *host, const ClCharT *dev) {
    int i;
    ClRcT rc = CL_OK;
    ClCharT myMac[CL_MAC_ADDRESS_LENGTH];
    ClUint32T myIp = 0;
    ClPluginHelperEthIpv4ArpPacketT pkt;
    int ifindex = 0;

    _clPluginHelperDevToMac(dev, myMac);
    if ((rc = _clPluginHelperHostToIp(host, &myIp)) != CL_OK)
    {
        return rc;
    }

    _clPluginHelperDevToIfIndex(dev, &ifindex);

    for (i = CL_PLUGIN_HELPER_ARP_REQUEST; i <= CL_PLUGIN_HELPER_ARP_REPLY; i++) {
        memcpy(pkt.myMac, myMac, CL_MAC_ADDRESS_LENGTH);
        memset(pkt.dstMac, 0xFF, CL_MAC_ADDRESS_LENGTH);

        pkt.type = htons(ETHERTYPE_ARP);

        pkt.hrd = htons(CL_PLUGIN_HELPER_ARP_HW_TYPE_ETHERNET);
        pkt.pro = htons(CL_PLUGIN_HELPER_IP_PROTO_TYPE); //htons(ETHERTYPE_IP); //htons(ETH_P_IP); //ArpAddressResolutionType;
        pkt.hln = CL_MAC_ADDRESS_LENGTH; /* ETH_HW_ADDR_LEN; Length in bytes of ethernet address */
        pkt.pln = 4; // IP_ADDR_LEN;
        pkt.op = htons(i); // htons(ARPOP_REQUEST);

        memcpy(pkt.sha, myMac, CL_MAC_ADDRESS_LENGTH);
        memset(pkt.tha, 0xFF, CL_MAC_ADDRESS_LENGTH);
        memcpy(pkt.spa, &myIp, 4);
        memcpy(pkt.tpa, &myIp, 4);

        if (1) {
            int sd;
            struct sockaddr_ll sal;

            bzero(&sal, sizeof(sal));
            sal.sll_family = AF_PACKET;
            sal.sll_protocol = htons(ETH_P_ARP);
            sal.sll_ifindex = ifindex;
            sal.sll_hatype = htons(i);
            sal.sll_pkttype = PACKET_BROADCAST;
            memcpy(sal.sll_addr, myMac, CL_MAC_ADDRESS_LENGTH);
            sal.sll_halen = CL_MAC_ADDRESS_LENGTH;

            if ((sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) < 0) {
                int err = errno;
                clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Cannot create a socket, arp not sent; error %d: %s", err, "");
            } else {
                if (sendto(sd, &pkt, sizeof(pkt), 0, (struct sockaddr *) &sal, sizeof(sal)) < 0) {
                    int err = errno;
                    clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Cannot send the arp packet; error %d: %s", err, "");
                    rc = CL_ERR_LIBRARY;
                }
                clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Gratuitous arp sent: IP %s device %s mac %02x:%02x:%02x:%02x:%02x:%02x", host, dev, myMac[0],
                        myMac[1], myMac[2], myMac[3], myMac[4], myMac[5]);
                shutdown(sd, SHUT_RDWR);
                close(sd);
            }
        }
    }
    return CL_OK;
}

/* We want to make extremely sure that the IP address changes across the network.  But ARP table
 updates in all the devices on the network are unreliable since they are dependent on lots of
 embedded implementations in switches, routers, etc.
 So we send a bunch of gratituous ARPs spaced across time to get the message across!
 */
static void *_clPluginHelperPummelArps(void *arg) {
    int i;
    ClPluginHelperVirtualIpAddressT* vip = (ClPluginHelperVirtualIpAddressT*) arg;

    for (i = 0; i < 5; i++) {
        sleep(1);
        _clPluginHelperSendArp(vip->ip, vip->dev);
    }
    free(arg);
    return NULL;
}

/*
 * Before calling ifconfig, we want to check if interface and ip existing,
 * if so, just ignore the calling command
 */
static ClRcT _clCheckExistingDevIf(const ClCharT *ip, const ClCharT *dev)
{
    int sd = -1;
    int i = 0;
    ClUint8T buffer[0xffff+1];
    struct ifconf   ifc = {0};
    struct ifreq    *ifr = NULL;
    struct ifreq    *interface;
    struct sockaddr *addr;
    ClCharT addrStr[INET_ADDRSTRLEN];

    /* Get a socket handle. */
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        clLogError(
                "IOC",
                CL_LOG_PLUGIN_HELPER_AREA,
                "open socket failed with error [%s]", strerror(errno));
        return (0);
    }

    /* Query available interfaces. */
    ifc.ifc_len = sizeof(buffer);
    ifc.ifc_buf = (void *)buffer;

    if (ioctl(sd, SIOCGIFCONF, &ifc) < 0) {
        clLogNotice("IOC",
                CL_LOG_PLUGIN_HELPER_AREA,
                "Operation command failed: [%s]", strerror(errno));
        close(sd);
        return (0);
    }

    /* Iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    for (i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++) {
        interface = &ifr[i];

        /* Show the device name and IP address */
        addr = &(interface->ifr_addr);

        //clLogTrace("IOC",
        //                CL_LOG_PLUGIN_HELPER_AREA,
        //                "Checking interface name [%s]",
        //                interface->ifr_name);

        /* Checking if match interface name first */
        if (strlen(interface->ifr_name) == strlen(dev)
                && memcmp(interface->ifr_name, dev, strlen(dev)) == 0)
        {

            /* Get the address
             * This may seem silly but it seems to be needed on some systems
             */
            if (ioctl(sd, SIOCGIFADDR, interface) < 0) {
                clLogNotice("IOC",
                        CL_LOG_PLUGIN_HELPER_AREA,
                        "Operation command failed: [%s]", strerror(errno));
                goto out;
            }

            struct in_addr *in4_addr = &((struct sockaddr_in*)addr)->sin_addr;
            if(!inet_ntop(PF_INET, (const void *)in4_addr, addrStr, sizeof(addrStr)))
            {
                struct in6_addr *in6_addr = &((struct sockaddr_in6*)addr)->sin6_addr;
                if(!inet_ntop(PF_INET6, (const void*)in6_addr, addrStr, sizeof(addrStr)))
                {
                    goto out;
                }
            }

            //clLogTrace("IOC",
            //                CL_LOG_PLUGIN_HELPER_AREA,
            //                "Checking IP address [%s]",
            //                addrStr);

            if (strlen(addrStr) == strlen(ip)
                    && memcmp(addrStr, ip, strlen(ip)) == 0)
            {
                close(sd);
                return (1);
            }

            /* Ignore other interfaces */
            goto out;
        }
    }

out:
    close(sd);
    return (0);
}

void clPluginHelperAddRemVirtualAddress(const ClCharT *cmd, const ClPluginHelperVirtualIpAddressT *vip) {
    int up = 0;
    if (cmd[0] == 'u')
        up = 1;

    ClPluginHelperVirtualIpAddressT* vipCopy = malloc(sizeof(ClPluginHelperVirtualIpAddressT));
    memcpy(vipCopy, vip, sizeof(ClPluginHelperVirtualIpAddressT));

    /*
     * Ignore configure if IP and dev are already existing on node
     */
    if (_clCheckExistingDevIf(vipCopy->ip, vipCopy->dev))
    {
        clLogInfo("IOC",
                CL_LOG_PLUGIN_HELPER_AREA,
                "Ignored assignment IP address: %s, for device: %s",
                vipCopy->ip,
                vipCopy->dev);
        goto out;
    }

    if (vipCopy->ip && vipCopy->dev && vipCopy->netmask) 
    {
        char execLine[301];
        snprintf(execLine, 300, "%s/virtualIp %s %s %s %s %s ", getenv("ASP_BINDIR"), cmd, vipCopy->ip, vipCopy->netmask, vipCopy->dev, vipCopy->subnetPrefix);
        clLogInfo("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Executing %s", execLine);
        __attribute__((unused)) ClRcT result = system(execLine);

        if (up) /* If we are coming up, do a gratuitous arp */
        {
            clLogInfo("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Sending gratuitous arps: IP address: %s, device: %s", vipCopy->ip, vipCopy->dev);
            _clPluginHelperSendArp(vipCopy->ip, vipCopy->dev);
            clOsalTaskCreateDetached("arpTask", CL_OSAL_SCHED_OTHER, 0, 0, _clPluginHelperPummelArps, vipCopy);
            vipCopy = NULL; /* freed by the arp thread*/
        } 
        else 
        {
            /* If we are going down, delay a bit so that the machine that takes over will not do so too soon.
             (the assumption being that the machine taking over will not do so until this remove returns)
             */
            clLogInfo("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Removing IP; not sending gratuitous arps");
            /* sleep(1); */
        }
    } 
    else 
    {
        clLogNotice("IOC", CL_LOG_PLUGIN_HELPER_AREA, "Virtual IP work assignment values incorrect: got IP address: %s, device: %s, mask: %s, net prefix: %s", 
                    vipCopy->ip, vipCopy->dev, vipCopy->netmask, vipCopy->subnetPrefix);
    }

out:
    if(vipCopy)
        free(vipCopy);
}

void clPluginHelperGetIpAddress(const ClUint32T ipAddressMask, const ClIocNodeAddressT iocAddress, ClCharT *hostAddress) {
    _clPluginHelperConvertHostToInternetAddress(ipAddressMask + iocAddress, hostAddress);
}

ClRcT clPluginHelperGetVirtualAddressInfo(const ClCharT *xportType,
                                          ClPluginHelperVirtualIpAddressT* vip)
{

    ClRcT rc = CL_OK;
    ClCharT dev[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT ip[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT netmask[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT broadcast[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT subnetPrefix[CL_MAX_FIELD_LENGTH] = {0};
    ClUint32T ipAddressMask = 0;

    rc = _clPluginHelperGetLinkName(xportType, dev);
    if (rc != CL_OK)
    {
        clLogError("IOC", CL_LOG_PLUGIN_HELPER_AREA,
                "Get link name failed with [%#x]", rc);
        return rc;
    }

    rc = _clPluginHelperGetIpNodeAddress(xportType, ip, netmask, broadcast, 
                                         &ipAddressMask, subnetPrefix);
    if (rc != CL_OK)
    {
        clLogError("IOC", CL_LOG_PLUGIN_HELPER_AREA,
                "Get link name failed with [%#x]", rc);
        return rc;
    }

    vip->ipAddressMask = ipAddressMask;

    vip->dev[0] = 0;
    vip->ip[0] = 0;
    vip->netmask[0] = 0;
    vip->broadcast[0] = 0;
    vip->subnetPrefix[0] = 0;
    strncat(vip->dev, dev, sizeof(vip->dev)-1);
    strncat(vip->ip, ip, sizeof(vip->ip)-1);
    strncat(vip->netmask, netmask, sizeof(vip->netmask)-1);
    strncat(vip->broadcast, broadcast, sizeof(vip->broadcast)-1);
    strncat(vip->subnetPrefix, subnetPrefix, sizeof(vip->subnetPrefix)-1);
    return CL_OK;

}

