#ifndef __CL_ARCH_HEADERS_H__
#define __CL_ARCH_HEADERS_H__

/*==========================================*/
#ifdef POSIX_BUILD

#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/uio.h>

#ifdef VXWORKS_BUILD
#include <clVxWorks.h>
#elif defined(QNX_BUILD)
#include <clQnx.h>
#endif

#define OS_VERSION_CODE             0

#define OS_KERNEL_VERSION(x,y,z)    1

#define CL_TIPC_PACKET_SIZE 1500

#define CL_ENABLE_ASP_TRAFFIC_SHAPING

/*==========================================*/

#elif defined SOLARIS_BUILD

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/sem.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <poll.h>
#include <getopt.h>
#include "tipc.h"

#define OS_VERSION_CODE             0
#define OS_KERNEL_VERSION(x,y,z)    1

#define CL_TIPC_PACKET_SIZE 0xFFFF

/*==========================================*/
#elif defined __linux__

#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/fcntl.h>
#include <linux/version.h>
#include <string.h>
#include <poll.h>
#include <getopt.h>
#include <execinfo.h>
#include <unistd.h>

#define OS_VERSION_CODE LINUX_VERSION_CODE
#define OS_KERNEL_VERSION KERNEL_VERSION

#define CL_TIPC_PACKET_SIZE 0xFFFF


#endif

#if !defined __linux__

#define CL_NODE_RESET() do { system("reboot"); } while(0)

#else

#include <unistd.h>
#include <sys/reboot.h>

#define CL_NODE_RESET() do { reboot(RB_AUTOBOOT); } while(0)

#endif
/*==========================================*/

#endif
