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
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include <clPluginHelper.hxx>

#define CL_LOG_PLUGIN_HELPER_AREA "PLUGIN_HELPER"
#define LOG_CTX "MSG"
#define CL_PLUGIN_HELPER_ARP_REQUEST (1)
#define CL_PLUGIN_HELPER_ARP_REPLY (2)
#define CL_PLUGIN_HELPER_ARP_HW_TYPE_ETHERNET (1)
#define CL_PLUGIN_HELPER_IP_PROTO_TYPE 0x0800

namespace SAFplusI
{
/*
 *  Network to Host
 *
 *  Convert from network to host type
 */
ClRcT clPluginHelperConvertInternetToHostAddress(ClUint32T *addr, const ClCharT *internetAddress)
{
    ClCharT ipAddress[INET_ADDRSTRLEN] = { 0 };
    ClUint32T val[4] = { 0 };
    *addr = 0;
    ClCharT *token = NULL;
    ClCharT *nextToken = NULL;
    ClInt32T n = 0;

    strncat(ipAddress, internetAddress, sizeof(ipAddress) - 1);

    token = strtok_r(ipAddress, ".", &nextToken);
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
ClRcT clPluginHelperConvertHostToInternetAddress(ClUint32T addr, ClCharT *internetAddress)
{

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
ClUint32T clPluginHelperBitFillRShift(ClUint32T numBits)
{
    ClUint32T mask = ~0U;
    mask <<= (32 - numBits);
    return mask;
}

#if 0
static ClRcT _clPluginHelperGetLinkName(const ClCharT *xportType, ClCharT *inf)
{
    ClCharT net_addr[CL_MAX_FIELD_LENGTH] = "eth0";
    ClCharT *linkName = NULL;
    ClCharT envlinkNameType[CL_MAX_FIELD_LENGTH] = { 0 };
    if (!xportType)
    {
        return CL_ERR_INVALID_PARAMETER;
    }
    else
    {
        snprintf(envlinkNameType, CL_MAX_FIELD_LENGTH, "ASP_%s_LINK_NAME", xportType);
        linkName = getenv(envlinkNameType);
        if (linkName == NULL)
        {
            // try with default LINK_NAME
            linkName = getenv("LINK_NAME");
            if (linkName == NULL)
            {
                logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "%s and LINK_NAME environment variable is not exported. Using 'eth0:10' interface as default", envlinkNameType);
            }
            else
            {
                logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "LINK_NAME env is exported. Value is %s", linkName);
                net_addr[0] = 0;
                strncat(net_addr, linkName, sizeof(net_addr)-1);
                ClCharT *token = NULL;
                strtok_r(net_addr, ":", &token);
            }
            /* If we are not using the existing IP addr then we need to use a virtual device to make sure we don't overwrite an already-configured address */
            if (!ASP_UDP_USE_EXISTING_IP)
              snprintf(inf, CL_MAX_FIELD_LENGTH, "%s:%d", net_addr, gIocLocalBladeAddress + 10);
            /* If we ARE using the existing IP, then whatever interfaces LINK_NAME is set to IS that address */
            else snprintf(inf, CL_MAX_FIELD_LENGTH, "%s", net_addr);            
        }
        else
        {
            logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "%s env is exported. Value is %s", envlinkNameType, linkName);
            snprintf(inf, CL_MAX_FIELD_LENGTH, "%s", linkName);
        }
    }
    return CL_OK;
}
#endif

#if 0
static ClRcT _clPluginHelperGetIpNodeAddress(const ClCharT *xportType, const ClCharT *devIf, ClCharT *hostAddress, ClCharT *networkMask, ClCharT *broadcast, ClUint32T *ipAddressMask, ClCharT *xportSubnetPrefix)
{
    ClCharT envSubNetType[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT xportSubnet[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT *subnetMask = NULL;
    ClCharT *subnetPrefix = NULL;
    ClUint32T CIDR, ipMask, ip, mask;
    ClRcT rc;
    
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
            logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "%s", subnetMask);
            return CL_ERR_INVALID_PARAMETER;
        }
        xportSubnetPrefix[0] = 0;
        strncat(xportSubnetPrefix, subnetPrefix, CL_MAX_FIELD_LENGTH-1);
        snprintf(xportSubnet, sizeof(xportSubnet), "%s", subnetMask);
        mask = _clPluginHelperBitFillRShift(CIDR);
        _clPluginHelperConvertInternetToHostAddress(&ip, xportSubnet);

        /* network address */
        ipMask = (ip & mask);


        /* Try to get address from devif */
        
        rc = CL_OK+1;  /* start with any error condition, so the if below this one will be taken  */
        if (clParseEnvBoolean("ASP_UDP_USE_EXISTING_IP"))
        {
            rc = _clPluginHelperDevToIpAddress(devIf, hostAddress);
            if (rc == CL_OK) logInfo(LOG_CTX,CL_LOG_PLUGIN_HELPER_AREA,"Use existing IP address [%s] as this nodes transport address.", hostAddress);
            else logError(LOG_CTX,CL_LOG_PLUGIN_HELPER_AREA,"Configured to use an existing IP address for message transport.  But address lookup failed on device [%s] error [0x%x]", devIf, rc);
        }
        
        if (rc != CL_OK)
        {
             /* Automatic assignment of IP address */
            _clPluginHelperConvertHostToInternetAddress(ipMask + gIocLocalBladeAddress, hostAddress);
        }

        _clPluginHelperConvertHostToInternetAddress(mask, networkMask);
        _clPluginHelperConvertHostToInternetAddress(ip | ~mask, broadcast);
        *ipAddressMask = ipMask;
    }
    return CL_OK;
}
#endif

static ClRcT _clPluginHelperDevToMac(const ClCharT* dev, char mac[CL_MAC_ADDRESS_LENGTH]) {
    struct ifreq req;

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = AF_UNSPEC;
    if (ioctl(sd, SIOCGIFHWADDR, &req) == -1) {
        int err = errno;
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "No device %s, hwaddr errno: %d", dev, err);
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
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "No device %s, ifindex errno: %d", dev, err);
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
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Cannot resolve host string %s", myHost);
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
                logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Cannot create a socket, arp not sent; error %d: %s", err, "");
            } else {
                if (sendto(sd, &pkt, sizeof(pkt), 0, (struct sockaddr *) &sal, sizeof(sal)) < 0) {
                    int err = errno;
                    logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Cannot send the arp packet; error %d: %s", err, "");
                    rc = CL_ERR_LIBRARY;
                }
                logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Gratuitous arp sent: IP %s device %s mac %02x:%02x:%02x:%02x:%02x:%02x", host, dev, myMac[0],
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
 * Returns ip address on a network interface given its name...
 */

void devToIpAddress(const char *dev, char *addrStr)
{
    int sd;
    struct ifreq req;
    ClCharT ipAddress[INET_ADDRSTRLEN] = { 0 };
    int rc = 1;

    /* Get a socket handle. */
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        logError(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "open socket failed with error [%s]", strerror(errno));
        assert(0); // Something is REALLY wrong if I can't create a socket
    }

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = PF_UNSPEC;

    if (ioctl(sd, SIOCGIFADDR, &req) == -1)
    {
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Operation command failed: [%s]", strerror(errno));
        close(sd);
        assert(0);  // TODO: Any "normal" errors?  If so I should throw
    }

    struct in_addr *in4_addr = &((struct sockaddr_in*) &((struct ifreq *) &req)->ifr_addr)->sin_addr;
    if (!inet_ntop(PF_INET, (const void *) in4_addr, ipAddress, sizeof(ipAddress)))
    {
        struct in6_addr *in6_addr = &((struct sockaddr_in6*) &((struct ifreq *) &req)->ifr_addr)->sin6_addr;
        if (!inet_ntop(PF_INET6, (const void *) in6_addr, ipAddress, sizeof(ipAddress)))
        {
            assert(0);
        }
    }

    /* successful */
    rc = 0;
    strncat(addrStr, ipAddress, sizeof(ipAddress) - 1);
    close(sd);
}

/*
 * Returns ip address on a network interface given its name...
 */

struct in_addr devToIpAddress(const char *dev)
{
    int sd;
    struct ifreq req;

    /* Get a socket handle. */
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        logError(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "open socket failed with error [%s]", strerror(errno));
        assert(0); // Something is REALLY wrong if I can't create a socket
    }

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev);
    req.ifr_addr.sa_family = PF_UNSPEC;

    if (ioctl(sd, SIOCGIFADDR, &req) == -1)
    {
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Operation command failed: [%s]", strerror(errno));
        close(sd);
        assert(0);  // TODO: Any "normal" errors?  If so I should throw
    }

    struct in_addr *in4_addr = &((struct sockaddr_in*) &((struct ifreq *) &req)->ifr_addr)->sin_addr;
    close(sd);
    return *in4_addr;
}

/*
 * Before calling ifconfig, we want to check if interface and ip existing,
 * if so, just ignore the calling command
 */
static ClRcT _clCheckExistingDevIf(const ClCharT *ip, const ClCharT *dev)
{
    int sd = -1;
    int i = 0;
    ClUint32T reqs = 0;
    struct ifconf   ifc = {0};
    struct ifreq    *ifr = NULL;
    ClCharT addrStr[INET_ADDRSTRLEN] = {0};

    /* Get a socket handle. */
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        logError(
                LOG_CTX,
                CL_LOG_PLUGIN_HELPER_AREA,
                "open socket failed with error [%s]", strerror(errno));
        return (0);
    }

    memset(&ifc, 0, sizeof(ifc));

    /* Query available interfaces. */
    do
    {
        reqs += 5;
        ifc.ifc_len = reqs * sizeof(*ifr);
        ifc.ifc_buf = (char*) realloc(ifc.ifc_buf, sizeof(*ifr) * reqs);
        CL_ASSERT(ifc.ifc_buf != NULL);
        if (ioctl(sd, SIOCGIFCONF, &ifc) < 0) {
            logNotice(LOG_CTX,
                    CL_LOG_PLUGIN_HELPER_AREA,
                    "Operation command failed: [%s]", strerror(errno));
            goto out;
        }
    } while ((unsigned int )ifc.ifc_len == reqs * sizeof(*ifr));


    /* Iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    for(i = 0; i < ifc.ifc_len; i += sizeof(*ifr))
    {
//        logTrace(LOG_CTX,
//                        CL_LOG_PLUGIN_HELPER_AREA,
//                        "Checking interface name [%s]",
//                        ifr->ifr_name);

        /* Checking if match interface name first */
        if (strlen(ifr->ifr_name) == strlen(dev)
                && memcmp(ifr->ifr_name, dev, strlen(dev)) == 0)
        {

            /* Get the address
             * This may seem silly but it seems to be needed on some systems
             */
            if (ioctl(sd, SIOCGIFADDR, ifr) < 0) {
                logNotice(LOG_CTX,
                        CL_LOG_PLUGIN_HELPER_AREA,
                        "Operation command failed: [%s]", strerror(errno));
                break;
            }

            addrStr[0] = 0;
            struct in_addr *in4_addr = &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr;
            if(!inet_ntop(PF_INET, (const void *)in4_addr, addrStr, sizeof(addrStr)))
            {
                struct in6_addr *in6_addr = &((struct sockaddr_in6*)&ifr->ifr_addr)->sin6_addr;
                if(!inet_ntop(PF_INET6, (const void*)in6_addr, addrStr, sizeof(addrStr)))
                {
                    goto out;
                }
            }

//            logTrace(LOG_CTX,
//                            CL_LOG_PLUGIN_HELPER_AREA,
//                            "Checking IP address [%s]",
//                            addrStr);

            if (strlen(addrStr) == strlen(ip)
                    && memcmp(addrStr, ip, strlen(ip)) == 0)
            {
                free(ifc.ifc_buf);
                close(sd);
                return (1);
            }

            /* Ignore other interfaces */
            break;
        }
        ++ifr;
    }

out:
    if (ifc.ifc_buf)
        free(ifc.ifc_buf);
    close(sd);
    return (0);
}

#if 0
void clPluginHelperAddRemVirtualAddress(const ClCharT *cmd, const ClPluginHelperVirtualIpAddressT *vip) 
  {
    int up = 0;
    if (cmd[0] == 'u')
        up = 1;

    ClPluginHelperVirtualIpAddressT* vipCopy = (ClPluginHelperVirtualIpAddressT*) malloc(sizeof(ClPluginHelperVirtualIpAddressT));
    memcpy(vipCopy, vip, sizeof(ClPluginHelperVirtualIpAddressT));

    /*
     * Ignore configure if IP and dev are already existing on node
     */
    if (_clCheckExistingDevIf(vipCopy->ip, vipCopy->dev))
    {
        logInfo(LOG_CTX,
                CL_LOG_PLUGIN_HELPER_AREA,
                "Ignored assignment IP address: %s, for device: %s",
                vipCopy->ip,
                vipCopy->dev);
        goto out;
    }

    if (vipCopy->ip && vipCopy->dev && vipCopy->netmask) 
    {
        char execLine[301];
        snprintf(execLine, 300, "%s/virtualIp %s %s %s %s %s ", SAFplus::ASP_BINDIR, cmd, vipCopy->ip, vipCopy->netmask, vipCopy->dev, vipCopy->subnetPrefix);
        logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Executing %s", execLine);
        __attribute__((unused)) ClRcT result = system(execLine);

        if (up) /* If we are coming up, do a gratuitous arp */
        {
            logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Sending gratuitous arps: IP address: %s, device: %s", vipCopy->ip, vipCopy->dev);
            _clPluginHelperSendArp(vipCopy->ip, vipCopy->dev);
            clOsalTaskCreateDetached("arpTask", CL_OSAL_SCHED_OTHER, 0, 0, _clPluginHelperPummelArps, vipCopy);
            vipCopy = NULL; /* freed by the arp thread*/
        } 
        else 
        {
            /* If we are going down, delay a bit so that the machine that takes over will not do so too soon.
             (the assumption being that the machine taking over will not do so until this remove returns)
             */
            logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Removing IP; not sending gratuitous arps");
            /* sleep(1); */
        }
    } 
    else 
    {
        logNotice(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Virtual IP work assignment values incorrect: got IP address: %s, device: %s, mask: %s, net prefix: %s", 
                    vipCopy->ip, vipCopy->dev, vipCopy->netmask, vipCopy->subnetPrefix);
    }

out:
    if(vipCopy)
        free(vipCopy);
}
#endif

/* ip route configure */
void clPluginHelperAddRouteAddress(const ClCharT *ipAddress, const ClCharT *ifDevName)
{
    FILE *route_file;
    ClUint32T dest;
    ClCharT dummyStr[CL_MAX_NAME_LENGTH];
    ClCharT dummyDev[CL_MAX_FIELD_LENGTH];
    ClUint32T ipMulticast;
    ClBoolT foundDestRoute = CL_FALSE;
    __attribute__((unused)) ClRcT result;

    clPluginHelperConvertInternetToHostAddress(&ipMulticast, ipAddress);

    // open route file to get the route destination
    route_file = fopen("/proc/net/route", "r");

    result = fscanf(route_file, "%[^\n]", dummyStr);

    while (!feof(route_file))
    {
        result = fscanf(route_file, "%s %8X %[^\n]", dummyDev, &dest, dummyStr);       
        if (!(ipMulticast ^ dest))
        {
            foundDestRoute = CL_TRUE;
            break;
        }
    }
    fclose(route_file);

    if (!foundDestRoute)
    {
        char execLine[301];
        snprintf(execLine, 300, "/sbin/ip route add %s dev %s", ipAddress, ifDevName);
        logInfo(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA, "Executing %s", execLine);
        result = system(execLine);
    }
}

#if 0
ClRcT clPluginHelperGetVirtualAddressInfo(const ClCharT *xportType, ClPluginHelperVirtualIpAddressT* vip)
{

    ClRcT rc = CL_OK;
    ClCharT dev[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT ip[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT netmask[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT broadcast[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT subnetPrefix[CL_MAX_FIELD_LENGTH] = {0};
    ClUint32T ipAddressMask = 0;

    rc = clPluginHelperGetLinkName(xportType, dev);
    if (rc != CL_OK)
    {
        logError(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA,
                "Get link name failed with [%#x]", rc);
        return rc;
    }

    rc = _clPluginHelperGetIpNodeAddress(xportType, dev, ip, netmask, broadcast,
                                         &ipAddressMask, subnetPrefix);
    if (rc != CL_OK)
    {
        logError(LOG_CTX, CL_LOG_PLUGIN_HELPER_AREA,
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
#endif


};
