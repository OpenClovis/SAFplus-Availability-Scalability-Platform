/*
 * Copyright (c) 2005-2007 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Patrick Caulfield (pcaulfie@redhat.com)
 *
 * This software licensed under BSD license, the text of which follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* IPv4/6 abstraction */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
#include <sys/sockio.h>
#include <net/if.h>
#ifdef QNX_BUILD
#include <ifaddrs.h>
#include <netinet/in_var.h>
#elif defined(VXWORKS_BUILD)
#include <net/ifaddrs.h>
#include <sockLib.h>
#else
#include <net/if_var.h>
#include <netinet/in_var.h>
#endif
#endif
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(OPENAIS_LINUX)
#include <net/if.h>

/* ARGH!! I hate netlink */
#include <asm/types.h>
#include <linux/rtnetlink.h>
#endif

#ifndef s6_addr16
#define s6_addr16 __u6_addr.__u6_addr16
#endif

#include "totemip.h"
#include "swab.h"
#include "print.h"
#include "totem.h"

#define LOCALHOST_IPV4 "127.0.0.1"
#define LOCALHOST_IPV6 "::1"

#ifdef OPENAIS_TIPC
#define LOCALHOST_TIPC "1111.1"
#endif

#define NETLINK_BUFSIZE 16384

#ifdef SO_NOSIGPIPE
void totemip_nosigpipe(int s)
{
	int on = 1;
	setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void *)&on, sizeof(on));
}
#endif

#ifdef OPENAIS_TIPC
/*
 * We are assuming that the TIPC addresses are in the form
 * of xxx.yyy For ex, 12345.11, 22.22, etc. We will remove the dot
 * and the combined number can be used as an integer for nodeid, etc
 */
int totem_tipc_addr_to_int_convert(char *addr)
{
    char *tok = NULL;
    char temp_str[TOTEMIP_ADDRLEN] = {0};
    char res[TOTEMIP_ADDRLEN] = {0};
    int size1=0;

    /* Copy input string to a local variable */
    strncpy(temp_str,addr,TOTEMIP_ADDRLEN);

    tok = strtok(temp_str,":");
    if (tok == NULL)
        return -1;
    size1 = strlen(tok);
    memcpy(res, tok, size1);

    tok = strtok(NULL,".");
    if (tok != NULL)
        memcpy(res+size1, tok, strlen(tok));

    return atoi(res);
}
#endif
/* Compare two addresses */
int totemip_equal(struct totem_ip_address *addr1, struct totem_ip_address *addr2)
{
	int addrlen = 0;

	if (addr1->family != addr2->family)
		return 0;

	if (addr1->family == AF_INET) {
		addrlen = sizeof(struct in_addr);
	}
	if (addr1->family == AF_INET6) {
		addrlen = sizeof(struct in6_addr);
	}
#ifdef OPENAIS_TIPC
    addrlen = TOTEMIP_ADDRLEN;
#endif
	assert(addrlen);

	if (memcmp(addr1->addr, addr2->addr, addrlen) == 0)
		return 1;
	else
		return 0;

}

/* Copy a totem_ip_address */
void totemip_copy(struct totem_ip_address *addr1, struct totem_ip_address *addr2)
{
	memcpy(addr1, addr2, sizeof(struct totem_ip_address));
}

void totemip_copy_endian_convert(struct totem_ip_address *addr1, struct totem_ip_address *addr2)
{
	addr1->nodeid = swab32(addr2->nodeid);
	addr1->family = swab16(addr2->family);
	memcpy(addr1->addr, addr2->addr, TOTEMIP_ADDRLEN);
}

/* For sorting etc. params are void * for qsort's benefit */
int totemip_compare(const void *a, const void *b)
{
#ifndef OPENAIS_TIPC
	int i;
	struct totem_ip_address *totemip_a = (struct totem_ip_address *)a;
	struct totem_ip_address *totemip_b = (struct totem_ip_address *)b;
	struct in_addr ipv4_a1;
	struct in_addr ipv4_a2;
	struct in6_addr ipv6_a1;
	struct in6_addr ipv6_a2;
	unsigned short family;

	/*
	 * Use memcpy to align since totem_ip_address is unaligned on various archs
	 */
	memcpy (&family, &totemip_a->family, sizeof (unsigned short));

	if (family == AF_INET) {
		memcpy (&ipv4_a1, totemip_a->addr, sizeof (struct in_addr));
		memcpy (&ipv4_a2, totemip_b->addr, sizeof (struct in_addr));
		if (ipv4_a1.s_addr == ipv4_a2.s_addr) {
			return (0);
		}
		if (htonl(ipv4_a1.s_addr) < htonl(ipv4_a2.s_addr)) {
			return -1;
		} else {
			return +1;
		}
	} else
	if (family == AF_INET6) {
		/*
		 * Compare 16 bits at a time the ipv6 address
		 */
		memcpy (&ipv6_a1, totemip_a->addr, sizeof (struct in6_addr));
		memcpy (&ipv6_a2, totemip_b->addr, sizeof (struct in6_addr));
		for (i = 0; i < 8; i++) {
			int res = htons(ipv6_a1.s6_addr16[i]) -
				htons(ipv6_a2.s6_addr16[i]);
			if (res) {
				return res;
			}
		}
		return 0;
	} else {
		/*
		 * Family not set, should be!
	 	 */
		assert (0);
	}
#else
    /* 
     * This is running on TIPC, so need to handle it quite differently
     */
    struct totem_ip_address *totemip_a = (struct totem_ip_address *)a;
    struct totem_ip_address *totemip_b = (struct totem_ip_address *)b;
    int addr1 = totem_tipc_addr_to_int_convert((char*)totemip_a->addr);
    int addr2 = totem_tipc_addr_to_int_convert((char*)totemip_b->addr);

    if (addr1 == addr2)
        return 0;
    else if (addr1 > addr2)
        return +1;
    else
        return -1;
#endif
}

/* Build a localhost totem_ip_address */
int totemip_localhost(int family, struct totem_ip_address *localhost)
{
#ifndef OPENAIS_TIPC
	char *addr_text;

	memset (localhost, 0, sizeof (struct totem_ip_address));

	if (family == AF_INET) {
		addr_text = LOCALHOST_IPV4;
		if (inet_pton(family, addr_text, (char *)&localhost->nodeid) <= 0) {
			return -1;
		}
	} else {
		addr_text = LOCALHOST_IPV6;
	}

	if (inet_pton(family, addr_text, (char *)localhost->addr) <= 0)
		return -1;

	localhost->family = family;
#else
	memset (localhost, 0, sizeof (struct totem_ip_address));
    localhost->family = AF_TIPC;
    strncpy((char*)localhost->addr, LOCALHOST_TIPC, TOTEMIP_ADDRLEN);
#endif

	return 0;
}

int totemip_localhost_check(struct totem_ip_address *addr)
{
	struct totem_ip_address localhost;

	if (totemip_localhost(addr->family, &localhost))
		return 0;
	return totemip_equal(addr, &localhost);
}

const char *totemip_print(struct totem_ip_address *addr)
{
#ifndef OPENAIS_TIPC
	static char buf[INET6_ADDRSTRLEN];

	return inet_ntop(addr->family, addr->addr, buf, sizeof(buf));
#else
    /* 
     * assuming that the addr->addr can be used as string. i.e.
     * the address is terminated by NULL
     */
    return (char *)addr->addr;
#endif
}

/* Make a totem_ip_address into a usable sockaddr_storage */
int totemip_totemip_to_sockaddr_convert(struct totem_ip_address *ip_addr,
					uint16_t port, struct sockaddr_storage *saddr, int *addrlen)
{
	int ret = -1;

	if (ip_addr->family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)saddr;

		memset(sin, 0, sizeof(struct sockaddr_in));
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
		sin->sin_len = sizeof(struct sockaddr_in);
#endif
		sin->sin_family = ip_addr->family;
		sin->sin_port = port;
		memcpy(&sin->sin_addr, ip_addr->addr, sizeof(struct in_addr));
		*addrlen = sizeof(struct sockaddr_in);
		ret = 0;
	}

	if (ip_addr->family == AF_INET6) {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)saddr;

		memset(sin, 0, sizeof(struct sockaddr_in6));
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
		sin->sin6_len = sizeof(struct sockaddr_in6);
#endif
		sin->sin6_family = ip_addr->family;
		sin->sin6_port = port;
		sin->sin6_scope_id = 2;
		memcpy(&sin->sin6_addr, ip_addr->addr, sizeof(struct in6_addr));

		*addrlen = sizeof(struct sockaddr_in6);
		ret = 0;
	}

	return ret;
}

/* Converts an address string string into a totem_ip_address.
   family can be AF_INET, AF_INET6 or 0 ("for "don't care")
*/
int totemip_parse(struct totem_ip_address *totemip, char *addr, int family)
{
#ifndef OPENAIS_TIPC
	struct addrinfo *ainfo;
	struct addrinfo ahints;
	struct sockaddr_in *sa;
	struct sockaddr_in6 *sa6;
	int ret;

	memset(&ahints, 0, sizeof(ahints));
	ahints.ai_socktype = SOCK_DGRAM;
	ahints.ai_protocol = IPPROTO_UDP;
	ahints.ai_family   = family;

	/* Lookup the nodename address */
	ret = getaddrinfo(addr, NULL, &ahints, &ainfo);
	if (ret)
		return -1;

	sa = (struct sockaddr_in *)ainfo->ai_addr;
	sa6 = (struct sockaddr_in6 *)ainfo->ai_addr;
	totemip->family = ainfo->ai_family;

	if (ainfo->ai_family == AF_INET)
		memcpy(totemip->addr, &sa->sin_addr, sizeof(struct in_addr));
	else
		memcpy(totemip->addr, &sa6->sin6_addr, sizeof(struct in6_addr));
#else
    totemip->family = AF_TIPC;
    memcpy(totemip->addr, addr, TOTEMIP_ADDRLEN);
#endif

	return 0;
}

/* Make a sockaddr_* into a totem_ip_address */
int totemip_sockaddr_to_totemip_convert(struct sockaddr_storage *saddr, struct totem_ip_address *ip_addr)
{
	int ret = -1;

	ip_addr->family = saddr->ss_family;
	ip_addr->nodeid = 0;

	if (saddr->ss_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)saddr;

		memcpy(ip_addr->addr, &sin->sin_addr, sizeof(struct in_addr));
		ret = 0;
	}

	if (saddr->ss_family == AF_INET6) {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)saddr;

		memcpy(ip_addr->addr, &sin->sin6_addr, sizeof(struct in6_addr));

		ret = 0;
	}
	return ret;
}

#if defined(POSIX_BUILD)
int totemip_iface_check(struct totem_ip_address *bindnet,
			struct totem_ip_address *boundto,
			int *interface_up,
			int *interface_num)
{
    struct sockaddr_in *intf_addr;
    struct sockaddr_in *intf_addr_mask;
    struct sockaddr_storage bindnet_ss; 
    struct sockaddr_in *bindnet_sin = (struct sockaddr_in *)&bindnet_ss;
    struct ifaddrs *ifaddrs, *ifap;
    int	addrlen;
    int	res=0;

    *interface_up = 0;
    *interface_num = 0;

    totemip_totemip_to_sockaddr_convert(bindnet,
            0, &bindnet_ss, &addrlen);

    /* Get the interface details */
    if (getifaddrs(&ifaddrs) == -1) {
        log_printf(LOG_LEVEL_ERROR,
                "getifaddrs failed: %s",strerror(errno));
        return -1;
    }

    for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {

        intf_addr = (struct sockaddr_in*)ifap->ifa_addr;  //Address of current interface
        intf_addr_mask = (struct sockaddr_in*)ifap->ifa_netmask; //Netmask of current interface

        /* Is current address an ipv4 or ipv6 address? */
        if (intf_addr->sin_family == AF_INET) {

            /* Check if this interface matches the with bindnetaddr
            */
            if (bindnet_sin->sin_family == AF_INET &&
                    (intf_addr->sin_addr.s_addr ^ bindnet_sin->sin_addr.s_addr) == 0) {

                totemip_copy(boundto, bindnet);
                memcpy(boundto->addr, &intf_addr->sin_addr, sizeof(intf_addr->sin_addr));

                *interface_up = ifap->ifa_flags & IFF_UP;

                *interface_num = if_nametoindex(ifap->ifa_name);
                if (*interface_up == 0) {
                    log_printf(LOG_LEVEL_NOTICE,
                            "%s interface is down. So binding to loopback IP",ifap->ifa_name);
                }
                res = 0;
                break; /* for */
            }
        }
    }

    freeifaddrs(ifaddrs);
    return res;
}

#elif defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
int totemip_iface_check(struct totem_ip_address *bindnet,
			struct totem_ip_address *boundto,
			int *interface_up,
			int *interface_num)
{
#define NEXT_IFR(a)	((struct ifreq *)((u_char *)&(a)->ifr_addr +\
	((a)->ifr_addr.sa_len ? (a)->ifr_addr.sa_len : sizeof((a)->ifr_addr))))

	struct sockaddr_in *intf_addr_mask;
	struct sockaddr_storage bindnet_ss, intf_addr_ss;
	struct sockaddr_in *intf_addr_sin = (struct sockaddr_in *)&intf_addr_ss;
	struct sockaddr_in *bindnet_sin = (struct sockaddr_in *)&bindnet_ss;
	struct ifreq *ifr, *lifr;
	int id_fd;
	struct ifconf ifc;
	struct ifreq ifrb;
	int numreqs = 0;
	int res;
	int addrlen;

	*interface_up = 0;
	*interface_num = 0;

	totemip_totemip_to_sockaddr_convert(bindnet,
		0, &bindnet_ss, &addrlen);

	/*
	 * Generate list of local interfaces in ifc.ifc_req structure
	 */
	id_fd = socket (AF_INET, SOCK_DGRAM, 0);
	ifc.ifc_buf = 0;
	do {
		numreqs += 32;
		ifc.ifc_len = sizeof (struct ifreq) * numreqs;
		ifc.ifc_buf = (void *)realloc(ifc.ifc_buf, ifc.ifc_len);
		res = ioctl (id_fd, SIOCGIFCONF, &ifc);
		if (res < 0) {
			close (id_fd);
			return -1;
		}
	} while (ifc.ifc_len == sizeof (struct ifreq) * numreqs);
	res = -1;

	/*
	 * Find interface address to bind to
	 */
	lifr = (struct ifreq *)ifc.ifc_buf + (ifc.ifc_len / sizeof(*lifr));

	for (ifr = ifc.ifc_req; ifr < lifr; ifr = NEXT_IFR(ifr)) {
		strcpy(ifrb.ifr_name, ifr->ifr_name);

		/* Skip if no address set
		 */
		if (ioctl(id_fd, SIOCGIFADDR, &ifrb) < 0)
			continue;

		memcpy(&intf_addr_ss, &ifrb.ifr_addr, sizeof(intf_addr_ss));
		if (intf_addr_sin->sin_family == AF_INET) {
			/* Retrieve mask
			 */
			if (ioctl(id_fd, SIOCGIFNETMASK, &ifrb) < 0) {
				break;
			}
			intf_addr_mask = (struct sockaddr_in *)&ifrb.ifr_addr;

			if ( bindnet_sin->sin_family == AF_INET &&
				 (intf_addr_sin->sin_addr.s_addr & intf_addr_mask->sin_addr.s_addr) ==
			     (bindnet_sin->sin_addr.s_addr & intf_addr_mask->sin_addr.s_addr)) {

				totemip_copy(boundto, bindnet);
				memcpy(boundto->addr, &intf_addr_sin->sin_addr, sizeof(intf_addr_sin->sin_addr));

				/* Get inteface state
				 */
				if (ioctl(id_fd, SIOCGIFFLAGS, &ifrb) < 0) {
					break;
				}
				*interface_up = ifrb.ifr_flags & IFF_UP;

				/* Get interface index
				 */
#ifdef SIOCGIFINDEX
				if (ioctl(id_fd, SIOCGIFINDEX, &ifrb) < 0) {
					break;
				}
				*interface_num = ifrb.ifr_index;
#else
				*interface_num = if_nametoindex(ifrb.ifr_name);
#endif
				res = 0;
				break; /* for */
			}
		}
	}
	free (ifc.ifc_buf);
	close (id_fd);

	return (res);
}
#elif defined(OPENAIS_LINUX)

static void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
        while (RTA_OK(rta, len)) {
                if (rta->rta_type <= max)
                        tb[rta->rta_type] = rta;
                rta = RTA_NEXT(rta,len);
        }
}

int totemip_iface_check(struct totem_ip_address *bindnet,
			struct totem_ip_address *boundto,
			int *interface_up,
			int *interface_num)
{
	int fd;
	struct {
                struct nlmsghdr nlh;
                struct rtgenmsg g;
        } req = {{0}};
        struct sockaddr_nl nladdr = {0};
	struct totem_ip_address ipaddr;
	static char rcvbuf[NETLINK_BUFSIZE] __attribute__((aligned (4)));

	*interface_up = 0;
	*interface_num = 0;
	memset(&ipaddr, 0, sizeof(ipaddr));

	/* Make sure we preserve these */
	ipaddr.family = bindnet->family;
	ipaddr.nodeid = bindnet->nodeid;

	/* Ask netlink for a list of interface addresses */
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd <0)
		return -1;

        setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf));

        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;

        req.nlh.nlmsg_len = sizeof(req);
        req.nlh.nlmsg_type = RTM_GETADDR;
        req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
        req.nlh.nlmsg_pid = 0;
        req.nlh.nlmsg_seq = 1;
        req.g.rtgen_family = bindnet->family;

        if (sendto(fd, (void *)&req, sizeof(req), 0,
		   (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0)  {
		close(fd);
		return -1;
	}

	/* Look through the return buffer for our address */
	while (1)
	{
		int status;
		struct nlmsghdr *h;
		struct iovec iov = { rcvbuf, sizeof(rcvbuf) };
		struct msghdr msg = {
			(void*)&nladdr, sizeof(nladdr),
			&iov,   1,
			NULL,   0,
			0
		};

		status = recvmsg(fd, &msg, 0);
		if (!status) {
			close(fd);
			return -1;
		}

		h = (struct nlmsghdr *)rcvbuf;
		if (h->nlmsg_type == NLMSG_DONE)
			break;

		if (h->nlmsg_type == NLMSG_ERROR) {
			close(fd);
			return -1;
		}

		while (NLMSG_OK(h, status)) {
			if (h->nlmsg_type == RTM_NEWADDR) {
				struct ifaddrmsg *ifa = NLMSG_DATA(h);
				struct rtattr *tb[IFA_MAX+1];
				int len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa));
				int found_if = 0;

				memset(tb, 0, sizeof(tb));

				parse_rtattr(tb, IFA_MAX, IFA_RTA(ifa), len);

				memcpy(ipaddr.addr, RTA_DATA(tb[IFA_ADDRESS]), TOTEMIP_ADDRLEN);
				if (totemip_equal(&ipaddr, bindnet)) {
					found_if = 1;
				}

				/* If the address we have is an IPv4 network address, then
				   substitute the actual IP address of this interface */
				if (!found_if && tb[IFA_BROADCAST] && ifa->ifa_family == AF_INET) {
					uint32_t network;
					uint32_t addr;
					uint32_t netmask = htonl(~((1<<(32-ifa->ifa_prefixlen))-1));

					memcpy(&network, RTA_DATA(tb[IFA_BROADCAST]), sizeof(uint32_t));
					memcpy(&addr, bindnet->addr, sizeof(uint32_t));

					if (addr == (network & netmask)) {
						memcpy(ipaddr.addr, RTA_DATA(tb[IFA_ADDRESS]), TOTEMIP_ADDRLEN);
						found_if = 1;
					}
				}

				if (found_if) {

					/* Found it - check I/F is UP */
					struct ifreq ifr;
					int ioctl_fd; /* Can't do ioctls on netlink FDs */

					ioctl_fd = socket(AF_INET, SOCK_STREAM, 0);
					if (ioctl_fd < 0) {
						close(fd);
						return -1;
					}
					memset(&ifr, 0, sizeof(ifr));
					ifr.ifr_ifindex = ifa->ifa_index;

					/* SIOCGIFFLAGS needs an interface name */
					status = ioctl(ioctl_fd, SIOCGIFNAME, &ifr);
					status = ioctl(ioctl_fd, SIOCGIFFLAGS, &ifr);
					if (status) {
						close(ioctl_fd);
						close(fd);
						return -1;
					}

					if (ifr.ifr_flags & IFF_UP)
						*interface_up = 1;

					*interface_num = ifa->ifa_index;
					close(ioctl_fd);
					goto finished;
				}
			}

			h = NLMSG_NEXT(h, status);
		}
	}
finished:
	totemip_copy (boundto, &ipaddr);
	close(fd);
	return 0;
}
#endif /* OPENAIS_LINUX */


