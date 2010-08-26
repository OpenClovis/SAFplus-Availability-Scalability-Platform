#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include "port_mgmt.h"

#define PORT_MGMT_REFRESH_DELAY 0x5
#define PORT_MGMT_MONITOR_DELAY 0x2
#define PORT_MGMT_NETDEV "/proc/net/dev"
#define PORT_FLAGS_STR(flags) ( ( (flags) & (IFF_UP| IFF_RUNNING) ) ? "UP" : "DOWN" )

static int proc_netdev_fmt = 0;
static int port_interface_monitor_status = 0;
static pthread_mutex_t port_interface_lock = PTHREAD_MUTEX_INITIALIZER;
static int (*port_mgmt_notify_callback)(const char *name, int status);

struct port_interface
{
    struct port_interface *next;
    char name[IFNAMSIZ];
    int virtual;
    int index;
    int flags;
    struct port_interface_stats stats;
};

struct port_interface_notify
{
    struct port_interface_notify *next;
    char name[IFNAMSIZ];
    int status;
};

struct port_interface_monitor
{
    int poll_time;
};

static int g_sock = -1;
static struct port_interface *port_interface_list;
static struct port_interface *port_interface_list_last;

static int64_t forward_clock(void)
{
    struct timespec t;
    int err = clock_gettime(CLOCK_MONOTONIC, &t);
    if(err < 0 ) 
    {
        perror("clock_gettime");
        return err;
    }
    return (int64_t)t.tv_nsec/1000 + (int64_t)t.tv_sec * 1000000;
}

static void port_interface_list_destroy(void)
{
    struct port_interface *iter;
    struct port_interface *next;
    struct port_interface *list = NULL;
    /*
     * Shift the current list to the last interface list.
     */
    if(!port_interface_list_last)
    {
        port_interface_list_last = port_interface_list;
    }
    else
    {
        list = port_interface_list_last;
        port_interface_list_last = port_interface_list;
    }
    port_interface_list = NULL;
    for(iter = list; iter; iter = next)
    {
        next = iter->next;
        free(iter);
    }
}

static struct port_interface *port_interface_list_find(struct port_interface *list, 
                                                       const char *name, int index, int skip_virtual)
{
    struct port_interface *iter;
    if(!name && index < 0 ) return NULL;
    if(!list) list = port_interface_list;
    for(iter = list; iter; iter = iter->next)
    {
        int cmp = 1;
        if(skip_virtual && iter->virtual) continue;
        if(name &&
           !strncmp(iter->name, name, sizeof(iter->name)))
        {
            cmp = 0;
        }
        if(index >= 0)
        {
            cmp = 1;
            if(iter->index == index)
                cmp = 0;
        }
        if(!cmp) return iter;
    }
    return NULL;
}

static __inline__ struct port_interface *port_interface_list_add(const char *name, int index, int flags)
{
    struct port_interface *p = NULL;
    char *s;
    int virtual = 0;
    if((p = port_interface_list_find(port_interface_list, name, -1, 0)))
        return p;
    p = calloc(1, sizeof(*p));
    assert(p);
    strncpy(p->name, name, sizeof(p->name)-1);
    p->index = index;
    p->flags = flags;
    if( (s = strrchr(p->name, ':')))
    {
        if(isdigit(s[1])) virtual = 1;
    }
    p->virtual = virtual;
    port_mgmt_log("Interface name [%s], index [%d], flags [%s], virtual [%d]\n", p->name, p->index, PORT_FLAGS_STR(p->flags), p->virtual);
    p->next = port_interface_list;
    port_interface_list = p;
    return p;
}

void port_mgmt_stats_dump(struct port_interface_stats *stats)
{
    switch(proc_netdev_fmt)
    {
    case 1:

        port_mgmt_log("%-10s%-10s%-7s%-7s%-11s%-13s%-13s%-10s\n",
               "RxBytes", "RxPkts", "RxErrs", "RxDrop", "RxFifoErrs", "RxFrameErrs", "RxCompressed", "RxMcast");
        port_mgmt_log(
               "%-11llu %-11llu %-8lu %-8lu %-10lu %-10lu %-12lu %-10lu\n",
               stats->rx_bytes,
               stats->rx_packets,
               stats->rx_errors,
               stats->rx_dropped,
               stats->rx_fifo_errors,
               stats->rx_frame_errors,
               stats->rx_compressed,
               stats->rx_multicast);

        port_mgmt_log("%-10s%-10s%-7s%-7s%-11s%-13s%-10s%-12s\n",
               "TxBytes", "TxPkts", "TxErrs", "TxDrop", "TxFifoErrs", "TxCollisions", "TxCarErrs", "TxCompressed");
        port_mgmt_log("%-11llu %-11llu %-7lu %-7lu %-10lu %-12lu %-10lu %-12lu\n",
               stats->tx_bytes,
               stats->tx_packets,
               stats->tx_errors,
               stats->tx_dropped,
               stats->tx_fifo_errors,
               stats->tx_collisions,
               stats->tx_carrier_errors,
               stats->tx_compressed);
        break;
        
    case 2:
        port_mgmt_log("%-11s%-11s%-8s%-8s%-12s%-10s\n",
               "RxBytes", "RxPkts", "RxErrs", "RxDrop", "RxFifoErrs", "RxFrameErrs");
        port_mgmt_log(
               "%-11llu %-11llu %-8lu %-8lu %-12lu %-10lu\n",
               stats->rx_bytes,
               stats->rx_packets,
               stats->rx_errors,
               stats->rx_dropped,
               stats->rx_fifo_errors,
               stats->rx_frame_errors);

        port_mgmt_log("%-11s%-11s%-7s%-7s%-12s%-12s%-10s\n",
               "TxBytes", "TxPkts", "TxErrs", "TxDrop", "TxFifoErrs", "TxCollisions", "TxCarErrs");
        port_mgmt_log("%-11llu %-11llu %-7lu %-7lu %-12lu %-12lu %-10lu\n",
               stats->tx_bytes,
               stats->tx_packets,
               stats->tx_errors,
               stats->tx_dropped,
               stats->tx_fifo_errors,
               stats->tx_collisions,
               stats->tx_carrier_errors);
        break;

    case 3:
        port_mgmt_log("%-11s%-8s%-8s%-12s%-12s\n",
               "RxPkts", "RxErrs", "RxDrop", "RxFifoErrs", "RxFrameErrs");
        port_mgmt_log(
               "%-11llu %-8lu %-8lu %-12lu %-12lu\n",
               stats->rx_packets,
               stats->rx_errors,
               stats->rx_dropped,
               stats->rx_fifo_errors,
               stats->rx_frame_errors);

        port_mgmt_log("%-11s%-7s%-7s%-12s%-13s%-10s\n",
               "TxPkts", "TxErrs", "TxDrop", "TxFifoErrs", "TxCollisions", "TxCarErrs");
        port_mgmt_log("%-11llu %-7lu %-7lu %-10lu %-12lu %-10lu\n",
               stats->tx_packets,
               stats->tx_errors,
               stats->tx_dropped,
               stats->tx_fifo_errors,
               stats->tx_collisions,
               stats->tx_carrier_errors);
        break;

    default:
        break;
    }
}

#ifdef DEBUG
static void port_stats_dump(struct port_interface_stats *stats)
{
    port_mgmt_stats_dump(stats);
}
#else
static void port_stats_dump(struct port_interface_stats *stats) { }
#endif

static int get_port_fields(char *buf, struct port_interface *port)
{
    switch(proc_netdev_fmt)
    {
    case 1:
        sscanf(buf,
               "%llu %llu %lu %lu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu %lu",
               &port->stats.rx_bytes,
               &port->stats.rx_packets,
               &port->stats.rx_errors,
               &port->stats.rx_dropped,
               &port->stats.rx_fifo_errors,
               &port->stats.rx_frame_errors,
               &port->stats.rx_compressed,
               &port->stats.rx_multicast,

               &port->stats.tx_bytes,
               &port->stats.tx_packets,
               &port->stats.tx_errors,
               &port->stats.tx_dropped,
               &port->stats.tx_fifo_errors,
               &port->stats.tx_collisions,
               &port->stats.tx_carrier_errors,
               &port->stats.tx_compressed);
        break;
        
    case 2:
        sscanf(buf,
               "%llu %llu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu",
               &port->stats.rx_bytes,
               &port->stats.rx_packets,
               &port->stats.rx_errors,
               &port->stats.rx_dropped,
               &port->stats.rx_fifo_errors,
               &port->stats.rx_frame_errors,

               &port->stats.tx_bytes,
               &port->stats.tx_packets,
               &port->stats.tx_errors,
               &port->stats.tx_dropped,
               &port->stats.tx_fifo_errors,
               &port->stats.tx_collisions,
               &port->stats.tx_carrier_errors);
        break;

    case 3:
        sscanf(buf,
               "%llu %lu %lu %lu %lu %llu %lu %lu %lu %lu %lu",
               &port->stats.rx_packets,
               &port->stats.rx_errors,
               &port->stats.rx_dropped,
               &port->stats.rx_fifo_errors,
               &port->stats.rx_frame_errors,

               &port->stats.tx_packets,
               &port->stats.tx_errors,
               &port->stats.tx_dropped,
               &port->stats.tx_fifo_errors,
               &port->stats.tx_collisions,
               &port->stats.tx_carrier_errors);
        break;

    default: return -1;
    }

    port_stats_dump(&port->stats);

    return 0;
}

static char *get_port_name(char *buf, char *name)
{
    char *p = buf;
    while(isspace(*p)) ++p;
    while(*p)
    {
        if(isspace(*p)) break;
        if(*p == ':') 
        {
            ++p;
            break;
        }
        *name++ = *p++;
    }
    *name++ = 0;
    return p;
}

static int proc_netdev_set_fmt(const char *buf)
{
    if(strstr(buf, "compressed"))
        return 1;
    else if(strstr(buf, "bytes"))
        return 2;
    else return 3;
}

static int port_interface_proc_refresh(void)
{
    FILE *fptr = NULL;
    char buf[1024];
    int err = -1;

    fptr = fopen(PORT_MGMT_NETDEV, "r");
    if(!fptr)
    {
        perror("fopen:");
        goto out;
    }
    
    /*
     * strip first 2 lines of output.
     */
    if(!fgets(buf, sizeof(buf), fptr)) goto out;
    if(!fgets(buf, sizeof(buf), fptr)) goto out;

    if(!proc_netdev_fmt)
    {
        proc_netdev_fmt = proc_netdev_set_fmt(buf);
    }

    while(fgets(buf, sizeof(buf), fptr))
    {
        char name[IFNAMSIZ];
        char *p = get_port_name(buf, name);
        struct port_interface *port = port_interface_list_find(port_interface_list, name, -1, 0);
        if(!port) continue;
        port_mgmt_log("Getting port fields for port [%s]\n", name);
        get_port_fields(p, port);
    }
    fclose(fptr);
    err = 0;
    out:
    return err;
}

static int port_interface_list_refresh(int force_refresh)
{
    struct ifconf ifconf;
    struct ifreq *ifreq;
    static int64_t last_refresh_time;
    int64_t current_time = 0;
    int s;
    int i;
    int err = 0;
    int num_reqs = 0;
    s = g_sock;
    if(g_sock < 0)
    {
        g_sock = s = socket(PF_INET, SOCK_DGRAM, 0);
    }
    assert(g_sock >= 0);
    current_time = forward_clock();
    if(!force_refresh && port_interface_list && last_refresh_time)
    {
        if(current_time - last_refresh_time < PORT_MGMT_REFRESH_DELAY)
        {
            last_refresh_time = current_time;
            goto out;
        }
    }
    last_refresh_time = current_time;
    port_interface_list_destroy();
    memset(&ifconf, 0, sizeof(ifconf));
    do
    {
        num_reqs += 5;
        ifconf.ifc_len = sizeof(struct ifreq) * num_reqs;
        ifconf.ifc_buf = realloc(ifconf.ifc_buf, num_reqs * sizeof(struct ifreq));
        assert(ifconf.ifc_buf);
        err = ioctl(s, SIOCGIFCONF, (void*)&ifconf);
        if(err < 0)
        {
            perror("ioctl gifconf:");
            goto out_free;
        }
    } while(ifconf.ifc_len == sizeof(struct ifreq) * num_reqs);

    ifreq = ifconf.ifc_req;
    for(i = 0; i < ifconf.ifc_len; i += sizeof(*ifreq))
    {
        int index;
        char name[IFNAMSIZ];
        int flags;
        strncpy(name, ifreq->ifr_name, sizeof(name)-1);
        err = ioctl(s, SIOCGIFINDEX, ifreq);
        if(err < 0 )
        {
            perror("ioctl gifindex:");
            goto out_free;
        }
        index = ifreq->ifr_ifindex;
        err = ioctl(s, SIOCGIFFLAGS, ifreq);
        if(err < 0 )
        {
            perror("ioctl gifflags:");
            goto out_free;
        }
        flags = ifreq->ifr_flags;
        port_interface_list_add(name, index, flags);
        ++ifreq;
    }

    port_interface_proc_refresh();

    port_mgmt_log("Port interface list refreshed at time [%lld] usecs\n", (long long int)current_time);
    out_free:
    if(ifconf.ifc_buf) free(ifconf.ifc_buf);

    out:
    return err;
}

static struct port_interface *port_interface_find(const char *name, int index)
{
    struct port_interface *port;
    int force_refresh = 0;
    int skip_virtual = 0;

    if(!name && index < 0) return NULL;
    if(!name) skip_virtual = 1;

    retry:
    port_interface_list_refresh(force_refresh);
    port = port_interface_list_find(port_interface_list, name, index, skip_virtual);
    if(!port)
    {
        if(!force_refresh)
        {
            force_refresh = 1;
            goto retry;
        }
    }
    return port;
}

static int do_port_mgmt_set_mtu(const char *name, int mtu)
{
    int err = 0;
    struct ifreq req;
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, name, sizeof(req.ifr_name));
    req.ifr_mtu = mtu;
    err = ioctl(g_sock, SIOCSIFMTU, &req);
    if(err < 0)
    {
        perror("ioctl sifmtu:");
        return err;
    }
    return 0;
}

int port_mgmt_set_mtu(const char *name, int mtu)
{
    struct port_interface *port;
    if(!name) return -EINVAL;

    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    pthread_mutex_unlock(&port_interface_lock);
    if(!port)
    {
        port_mgmt_log("Port interface [%s] not found\n", name);
        return -ENOENT;
    }
    return do_port_mgmt_set_mtu(name, mtu);
}

int port_mgmt_index_set_mtu(int index, int mtu)
{
    struct port_interface *port;
    char name[IFNAMSIZ];
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port index [%d] not found\n", index);
        return -ENOENT;
    }
    strncpy(name, port->name, sizeof(name)-1);
    pthread_mutex_unlock(&port_interface_lock);
    return do_port_mgmt_set_mtu(name, mtu);
}

static int do_port_mgmt_update_flag(const char *name, short int flag, int clear)
{
    int err = 0;
    struct ifreq req;

    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, name, sizeof(req.ifr_name)-1);
    err = ioctl(g_sock, SIOCGIFFLAGS, &req);
    if(err < 0)
    {
        perror("ioctl gifflags:");
        return err;
    }

    if(clear)
    {
        req.ifr_flags &= ~flag;
    }
    else
    {
        req.ifr_flags |= flag;
    }

    err = ioctl(g_sock, SIOCSIFFLAGS, &req);
    if(err < 0)
    {
        perror("ioctl sifflags:");
        return err;
    }
    return 0;
}

static int do_port_mgmt_set_status(const char *name, int up)
{
    short int flag = 0;
    int clear = 0;
    if(up)
    {
        flag = (IFF_UP | IFF_RUNNING);
    }
    else
    {
        clear = 1;
        flag = IFF_UP;
    }
    return do_port_mgmt_update_flag(name, flag, clear);
}

int port_mgmt_set_status(const char *name, int up)
{
    struct port_interface *port;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    pthread_mutex_unlock(&port_interface_lock);
    if(!port)
    {
        port_mgmt_log("Port interface [%s] not found\n", name);
        return -ENOENT;
    }
    return do_port_mgmt_set_status(name, up);
}

int port_mgmt_index_set_status(int index, int up)
{
    struct port_interface *port;
    char name[IFNAMSIZ];
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port interface not found for index [%d]\n", index);
        return -ENOENT;
    }
    strncpy(name, port->name, sizeof(name)-1);
    pthread_mutex_unlock(&port_interface_lock);
    return do_port_mgmt_set_status(name, up);
}

static int do_port_mgmt_get_status(const char *name, int *status)
{
    int err = 0;
    struct ifreq req;

    if(!status) return -EINVAL;

    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, name, sizeof(req.ifr_name)-1);
    err = ioctl(g_sock, SIOCGIFFLAGS, &req);
    if(err < 0)
    {
        perror("ioctl gifflags:");
        return err;
    }
    if((req.ifr_flags & (IFF_UP|IFF_RUNNING)))
        *status = 1;
    else
        *status = 0;
    return 0;
}

int port_mgmt_get_status(const char *name, int *status)
{
    struct port_interface *port;
    if(!status) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    pthread_mutex_unlock(&port_interface_lock);
    if(!port)
    {
        port_mgmt_log("Port interface [%s] not found\n", name);
        return -ENOENT;
    }
    return do_port_mgmt_get_status(name, status);
}

int port_mgmt_index_get_status(int index, int *status)
{
    struct port_interface *port;
    char name[IFNAMSIZ];
    if(!status) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port interface not found for index [%d]\n", index);
        return -ENOENT;
    }
    strncpy(name, port->name, sizeof(name)-1);
    pthread_mutex_unlock(&port_interface_lock);
    return do_port_mgmt_get_status(name, status);
}

static int do_port_mgmt_set_promiscuous(const char *name, int up)
{
    short int flag = IFF_PROMISC;
    int clear = 0;
    if(!up)
    {
        clear = 1;
    }
    return do_port_mgmt_update_flag(name, flag, clear);
}

int port_mgmt_set_promiscuous(const char *name, int up)
{
    struct port_interface *port;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    pthread_mutex_unlock(&port_interface_lock);
    if(!port)
    {
        port_mgmt_log("Port interface [%s] not found\n", name);
        return -ENOENT;
    }
    return do_port_mgmt_set_promiscuous(name, up);
}

int port_mgmt_index_set_promiscuous(int index, int up)
{
    struct port_interface *port;
    char name[IFNAMSIZ];
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port interface not found for index [%d]\n", index);
        return -ENOENT;
    }
    strncpy(name, port->name, sizeof(name)-1);
    pthread_mutex_unlock(&port_interface_lock);
    return do_port_mgmt_set_promiscuous(name, up);
}

static int do_port_mgmt_get_promiscuous(const char *name, int *status)
{
    int err = 0;
    struct ifreq req;

    if(!status) return -EINVAL;

    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, name, sizeof(req.ifr_name)-1);
    err = ioctl(g_sock, SIOCGIFFLAGS, &req);
    if(err < 0)
    {
        perror("ioctl gifflags:");
        return err;
    }
    if((req.ifr_flags & IFF_PROMISC))
        *status = 1;
    else
        *status = 0;
    return 0;
}

int port_mgmt_get_promiscuous(const char *name, int *status)
{
    struct port_interface *port;
    if(!status) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    pthread_mutex_unlock(&port_interface_lock);
    if(!port)
    {
        port_mgmt_log("Port interface [%s] not found\n", name);
        return -ENOENT;
    }
    return do_port_mgmt_get_promiscuous(name, status);
}

int port_mgmt_index_get_promiscuous(int index, int *status)
{
    struct port_interface *port;
    char name[IFNAMSIZ];
    if(!status) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port interface not found for index [%d]\n", index);
        return -ENOENT;
    }
    strncpy(name, port->name, sizeof(name)-1);
    pthread_mutex_unlock(&port_interface_lock);
    return do_port_mgmt_get_promiscuous(name, status);
}

int port_mgmt_get_stats(const char *name, struct port_interface_stats *stats)
{
    struct port_interface *port;
    if(!stats) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(name, -1);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port [%s] not found\n", name);
        return -ENOENT;
    }
    memcpy(stats, &port->stats, sizeof(*stats));
    pthread_mutex_unlock(&port_interface_lock);
    return 0;
}

int port_mgmt_index_get_stats(int index, struct port_interface_stats *stats)
{
    struct port_interface *port;
    if(!stats) return -EINVAL;
    pthread_mutex_lock(&port_interface_lock);
    port = port_interface_find(NULL, index);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        port_mgmt_log("Port interface index [%d] not found\n", index);
        return -ENOENT;
    }
    memcpy(stats, &port->stats, sizeof(*stats));
    pthread_mutex_unlock(&port_interface_lock);
    return 0;
}

static int port_interface_notify(struct port_interface_notify **notify_list)
{
    struct port_interface_notify *iter;
    struct port_interface_notify *next;
    if(!notify_list || !*notify_list) return 0;
    for(iter = *notify_list; iter; iter = next)
    {
        next = iter->next;
        if(port_mgmt_notify_callback)
            port_mgmt_notify_callback(iter->name, iter->status);
        free(iter);
    }
    *notify_list = NULL;
    return 0;
}

static int port_interface_notify_add(struct port_interface_notify **monitor, 
                                     struct port_interface *port, 
                                     int status)
{
    struct port_interface_notify *m = NULL;
    m = calloc(1, sizeof(*m));
    assert(m);
    strncpy(m->name, port->name, sizeof(m->name)-1);
    m->status = status;
    m->next = *monitor;
    *monitor = m;
    return 0;
}

static void *port_interface_monitor(void *arg)
{
    struct port_interface_monitor *m = arg;
    int poll_time = 0;
    assert(m);
    if( !(poll_time = m->poll_time))
        poll_time = PORT_MGMT_MONITOR_DELAY;
    free(m);
    while(port_interface_monitor_status)
    {
        struct port_interface *iter;
        struct port_interface_notify *notify_list = NULL;
        pthread_mutex_lock(&port_interface_lock);
        port_interface_list_refresh(1);

        /*
         * First scan the current list for deleted interfaces;
         */
        for(iter = port_interface_list_last; iter; iter = iter->next)
        {
            if(port_interface_list_find(port_interface_list, iter->name, -1, 0))
            {
                continue;
            }
            /*
             * This interface has been deleted since last scanned. Mark it down.
             */
            port_interface_notify_add(&notify_list, iter, 0);
        }

        /*
         * Next scan the list for newly added interfaces.
         */
        for(iter = port_interface_list; iter; iter = iter->next)
        {
            if(port_interface_list_find(port_interface_list_last, iter->name, -1, 0))
                continue;

            port_interface_notify_add(&notify_list, iter, 1);
        }

        /*
         * Next scan for status change for existing interfaces.
         */
        for(iter = port_interface_list; iter; iter = iter->next)
        {
            int flags = iter->flags;
            int last_flags;
            struct port_interface *last = port_interface_list_find(port_interface_list_last, iter->name, -1, 0);

            if(!last) continue;
            last_flags = last->flags;
            if(last_flags == flags) continue;

            if((last_flags & IFF_UP)
               &&
               (!(flags & IFF_UP)))
            {
                port_interface_notify_add(&notify_list, iter, 0);
            }
            
            if( (!(last_flags & IFF_UP))
                &&
                (flags & IFF_UP))
            {
                port_interface_notify_add(&notify_list, iter, 1);
            }
        }
        pthread_mutex_unlock(&port_interface_lock);
        if(notify_list)
        {
            port_interface_notify(&notify_list);
        }
        sleep(poll_time);
    }
    return NULL;
}

int port_mgmt_initialize(int (*notify_callback)(const char *, int), int poll_time)
{
    static pthread_t tid;
    struct port_interface_monitor *m = NULL;
    pthread_attr_t attr;
    if(port_mgmt_notify_callback) return -EEXIST;
    port_mgmt_notify_callback = notify_callback;
    m = calloc(1, sizeof(*m));
    assert(m);
    m->poll_time = poll_time;
    assert(pthread_attr_init(&attr) == 0);
    assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)==0);
    port_interface_monitor_status = 1;
    assert(pthread_create(&tid, &attr, port_interface_monitor, m)==0);
    return 0;
}

int port_mgmt_finalize(void)
{
    port_interface_monitor_status = 0;
    port_mgmt_notify_callback = NULL;
    return 0;
}

int port_mgmt_refresh(void)
{
    return port_interface_list_refresh(1);
}

int port_mgmt_list_info(struct port_mgmt_info **list, int *nentries)
{
    struct port_mgmt_info *entries = NULL;
    struct port_interface *iter = NULL;
    int c = 0;
    if(!list || !nentries) return -EINVAL;
    *list = NULL;
    *nentries = 0;
    pthread_mutex_lock(&port_interface_lock);
    port_interface_list_refresh(1);
    for(iter = port_interface_list; iter; iter = iter->next)
    {
        entries = realloc(entries, (c+1) * sizeof(*entries));
        assert(entries);
        strncpy(entries[c].name, iter->name, sizeof(entries[c].name)-1);
        entries[c].flags = iter->flags;
        entries[c].index = iter->index;
        entries[c].virtual = iter->virtual;
        memcpy(&entries[c].stats, &iter->stats, sizeof(entries[c].stats));
        ++c;
    }
    pthread_mutex_unlock(&port_interface_lock);
    *list = entries;
    *nentries = c;
    return 0;
}

int port_mgmt_get_info(const char *name, struct port_mgmt_info *info)
{
    struct port_interface *port = NULL;
    if(!name || !info) return -EINVAL;

    pthread_mutex_lock(&port_interface_lock);
    port_interface_list_refresh(1);
    port = port_interface_list_find(port_interface_list, name, -1, 0);
    if(!port)
    {
        pthread_mutex_unlock(&port_interface_lock);
        return -ENOENT;
    }
    strncpy(info->name, port->name, sizeof(info->name)-1);
    info->index = port->index;
    info->flags = port->flags;
    info->virtual = port->virtual;
    memcpy(&info->stats, &port->stats, sizeof(info->stats));
    pthread_mutex_unlock(&port_interface_lock);

    return 0;
}
