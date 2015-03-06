#ifndef __CL_VXWORKS_H__
#define __CL_VXWORKS_H__

#ifdef VXWORKS_BUILD

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vxWorks.h>
#include <errnoLib.h>
#include <taskLib.h>
#include <rtpLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <ioLib.h>
#include <hostLib.h>

/******************************************************************/
/*The following work..... */

#define MADV_SEQUENTIAL 0
#define MADV_NORMAL     0

typedef pid_t __pid_t;

#define M_CHECK_ACTION  (-5)
#define REG_EIP         (14)
#define WAIT_ANY        (-1)
#define PTHREAD_PROCESS_PRIVATE (0)
#define PTHREAD_PROCESS_SHARED (1)

extern int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int pshared);
extern int pthread_condattr_setpshared(pthread_condattr_t *, int pshared);
extern unsigned long long strtoll(const char *, char *, int);
extern unsigned long long atoll(const char *);
extern int usleep(int usec);
extern int symlink(const char *, const char *);
extern int readlink(const char *, char *, int);
extern int getuid();
extern int geteuid();
extern int getgid();
extern int getegid();
extern int getpagesize(void);
extern int backtrace(void **arr, int size);
extern int madvise(void *addr, int size, int advice);
int scandir(const char *path, struct dirent ***dlist, int (*filter)(struct dirent *d),
            int (*cmp)(const void *, const void *));
extern int alphasort(const void *a, const void *b);
extern int random(void);
extern int umask(int mode);

#endif

#endif
