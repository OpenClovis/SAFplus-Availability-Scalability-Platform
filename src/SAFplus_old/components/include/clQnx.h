#ifndef __CL_QNX_H__
#define __CL_QNX_H__

#ifdef QNX_BUILD

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/procmgr.h>
#include <ucontext.h>
#include <bind/sys/bitypes.h>
#include <stdint.h>
#include <tipc.h>
#include <tipc_config.h>

/******************************************************************/
/*The following work..... */

#define madvise posix_madvise
#define MADV_SEQUENTIAL POSIX_MADV_SEQUENTIAL
#define MADV_NORMAL POSIX_MADV_NORMAL

#define getpagesize() sysconf(_SC_PAGESIZE) 

typedef pid_t __pid_t;



/******************************************************************/
/*Making following calls dummy..... */

#define backtrace(x, y)                 (0)

/******************************************************************/
/* Dont know whether these will do on QNX......*/

#define M_CHECK_ACTION  (-5)
#define REG_EIP         (14)
#define SA_RESTART      (0)
#define WAIT_ANY        (-1)

#endif

#endif
