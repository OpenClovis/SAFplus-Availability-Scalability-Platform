#ifndef _PORT_MGMT_H_
#define _PORT_MGMT_H_

#include <stdio.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define port_mgmt_log(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#else
#define port_mgmt_log(fmt, ...) do {;}while(0)
#endif

struct port_interface_stats
{
    unsigned long long rx_bytes;
    unsigned long long rx_packets;
    unsigned long rx_errors;
    unsigned long rx_dropped;
    unsigned long rx_fifo_errors;
    unsigned long rx_frame_errors;
    unsigned long rx_compressed;
    unsigned long rx_multicast;
    
    unsigned long long tx_bytes;
    unsigned long long tx_packets;
    unsigned long tx_errors;
    unsigned long tx_dropped;
    unsigned long tx_fifo_errors;
    unsigned long tx_collisions;
    unsigned long tx_carrier_errors;
    unsigned long tx_compressed;
};

struct port_mgmt_info
{
    char name[IFNAMSIZ];
    int index;
    int flags;
    int virtual;
    struct port_interface_stats stats;
};

int port_mgmt_initialize(int (*notify_callback)(const char *, int), int poll_time);
int port_mgmt_finalize(void);

int port_mgmt_get_stats(const char *name, struct port_interface_stats *stats);
int port_mgmt_index_get_stats(int index, struct port_interface_stats *stats);

int port_mgmt_set_status(const char *name, int up);
int port_mgmt_get_status(const char *name, int *status);
int port_mgmt_index_set_status(int index, int up);
int port_mgmt_index_get_status(int index, int *status);

int port_mgmt_set_promiscuous(const char *name, int up);
int port_mgmt_get_promiscuous(const char *name, int *status);
int port_mgmt_index_set_promiscuous(int index, int up);
int port_mgmt_index_get_promiscuous(int index, int *status);

int port_mgmt_set_mtu(const char *name, int mtu);
int port_mgmt_index_set_mtu(int index, int mtu);

int port_mgmt_refresh(void);

int port_mgmt_list_info(struct port_mgmt_info **info, int *nentries);
int port_mgmt_get_info(const char *name, struct port_mgmt_info *info);

void port_mgmt_stats_dump(struct port_interface_stats *stats);

static __inline__ unsigned long long port_mgmt_rx_bytes(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.rx_bytes;
}

static __inline__ unsigned long long port_mgmt_rx_packets(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.rx_packets;
}

static __inline__ unsigned long long port_mgmt_tx_bytes(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.tx_bytes;
}

static __inline__ unsigned long long port_mgmt_tx_packets(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.tx_packets;
}

static __inline__ unsigned long port_mgmt_rx_errors(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.rx_errors;
}

static __inline__ unsigned long port_mgmt_tx_errors(const char *name)
{
    struct port_interface_stats stats = {0};
    if(port_mgmt_get_stats(name, &stats))
    {
        return 0;
    }
    return stats.tx_errors;
}

#ifdef __cplusplus
}
#endif

#endif
