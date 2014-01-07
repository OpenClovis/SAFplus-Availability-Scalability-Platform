#ifndef CSA701_DEFS_H
#define CSA701_DEFS_H

#define SIZES_DELIM      ":,"
#define MAX_N_SIZES      1000

#define DEF_IOC_PORT     	 0x80
#define CSA701_SENDER_IOC_PORT 0x8001

#define IOC_SEND_TIMEOUT     6000
#define IOC_SEND_PRIORITY       1
#define IOC_SEND_MSGOPTION  CL_IOC_PERSISTENT_MSG

#define CSA701_RECV_UDP_BUF_SIZE 65536 /* 64 K */
#define LOCAL_BUF_SIZE		80

#endif /* CSA701_DEFS_H */
