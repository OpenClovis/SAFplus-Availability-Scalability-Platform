/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
#include "clCommon.h"
#include "clDebugApi.h"
#include <ctype.h>
#ifdef VXWORKS_BUILD
#include <sysLib.h>
#include <tickLib.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <ioLib.h>
#include <pipeDrv.h>
#define C99_VARIADIC_MACROS
#include <applUtilLib.h>

#define CL_VXWORKS_PIPE_MSGS (128)
#define CL_VXWORKS_PIPE_MSGSIZE (256)

static ClInt32T gClSyslogFacility;
#endif

/** \brief  Load the SaNameT structure.
    \param  name The structure you want to load
    \param  str  The value to be put into the SaNameT structure

    If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
*/
void saNameSet(SaNameT* name, const char* str)
{
    int sz = strlen(str);
    /* Make sure that the name is not too big when in debugging mode */
    CL_ASSERT(sz < CL_MAX_NAME_LENGTH);
    strncpy((ClCharT *)name->value,str,CL_MAX_NAME_LENGTH-1);
    /* If the name is too big, then crop it */
    if (sz >= CL_MAX_NAME_LENGTH)
    {
        name->value[CL_MAX_NAME_LENGTH-1] = 0;
        name->length = CL_MAX_NAME_LENGTH-1;
    }
    else name->length = sz;
}
/** \brief  Load the SaNameT structure.
    \param  name The structure you want to load
    \param  str  The structure to be put into the SaNameT structure

    If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
*/
void saNameCopy(SaNameT* nameOut, const SaNameT *nameIn)
{
    /* Make sure that the name is not too big when in debugging mode */
    CL_ASSERT(nameIn->length < CL_MAX_NAME_LENGTH);

    nameOut->length = nameIn->length;
    /* If the name is too big, then crop it */
    if( nameIn->length >= CL_MAX_NAME_LENGTH )
    {
        nameOut->length = CL_MAX_NAME_LENGTH - 1;
    }
    memcpy(nameOut->value, nameIn->value, nameOut->length);
    nameOut->value[nameOut->length] = 0;
}

void saNameConcat(SaNameT* nameOut, const SaNameT *prefix, const char* separator, const SaNameT *suffix)
{
    int curpos = 0;

    if (prefix)
    {
        memcpy(nameOut->value, prefix->value, prefix->length);
        curpos += prefix->length;
    }
    if (separator)
    {
        int amt2Copy = CL_MIN((CL_MAX_NAME_LENGTH-1) - curpos,strlen(separator));
        if (amt2Copy) memcpy(&nameOut->value[curpos], separator, amt2Copy);
        curpos += amt2Copy;
    }
    if (suffix)
    {
        int amt2Copy = CL_MIN((CL_MAX_NAME_LENGTH-1) - curpos,suffix->length);
        if (amt2Copy) memcpy(&nameOut->value[curpos], suffix->value, amt2Copy);
        curpos += amt2Copy;
    }
    assert(curpos < CL_MAX_NAME_LENGTH);
    nameOut->value[curpos]=0;
    nameOut->length=curpos;
}

ClBoolT clParseEnvBoolean(const ClCharT* envvar)
{
    ClCharT* val = NULL;
    ClBoolT rc = CL_FALSE;

    val = getenv(envvar);
    if (val != NULL)
    {
        /* If the env var is defined as "". */
        if (strlen(val) == 0)
        {
            rc = CL_TRUE;
        }
        else if (!strncmp(val, "1", 1) ||
                 !strncasecmp(val, "yes", 3) || 
                 !strncasecmp(val, "y", 1) ||
                 !strncasecmp(val, "true", 4) ||
                 !strncasecmp(val, "t", 1) )
        {
            rc = CL_TRUE;
        }
        else if (!strncmp(val, "0", 1) ||
                 !strncasecmp(val, "no", 2) || 
                 !strncasecmp(val, "n", 1) ||
                 !strncasecmp(val, "false", 5) ||
                 !strncasecmp(val, "f", 1) )
        {
            rc = CL_FALSE;
        }
        else 
        {
            clLogWarning("UTL", "ENV", "Environment variable [%s] value is not understood. "
                         "Please use 'YES' or 'NO'. Assuming 'NO'", envvar);
            rc = CL_FALSE;
        }
    }
    else
    {
        rc = CL_FALSE;
    }

    return rc;
}

ClCharT *clParseEnvStr(const ClCharT *envvar)
{
    ClCharT *val = NULL;
    if(!envvar || !(val = getenv(envvar)))
        return NULL;
    while(*val && isspace(*val)) ++val;
    if(!*val) return NULL;
    return val;
}

ClCharT *clStrdup(const ClCharT *str)
{
    ClCharT *dest = NULL;
    if(!str) return NULL;
    dest = clHeapCalloc(1, strlen(str)+1);
    if(!dest) return NULL;
    strcpy(dest, str);
    return dest;
}

ClStringT *clStringDup(const ClStringT *str)
{
    ClStringT *dest;
    if(!str) return NULL;
    dest = clHeapCalloc(1, sizeof(*dest));
    if(!dest) return NULL;
    dest->pValue = clHeapCalloc(str->length, sizeof(*dest->pValue));
    if(!dest->pValue)
    {
        clHeapFree(dest);
        return NULL;
    }
    if(str->pValue)
        memcpy(dest->pValue, str->pValue, str->length);
    dest->length = str->length;
    return dest;
}

/*
 * Get the power of 2 of an integer.
 * First align the input to a power of 2
 */
ClUint32T clBinaryPower(ClUint32T size)
{
    ClInt32T order = -1;
    size += 1;
    size &= ~1;
    do
    {
        ++order;
        size >>= 1;
    } while(size >  0);

    return order;
}

#ifdef VXWORKS_BUILD

static pthread_mutex_t pipeLock = PTHREAD_MUTEX_INITIALIZER;

int usleep(int usec)
{
    ClTimerTimeOutT delay = {0};
    delay.tsMilliSec = usec/1000;
    if(clOsalTaskDelay(delay) != CL_OK)
        sleep(1);
    return 0;
}

/*
 * Vxworks doesnt seem to support the concept of symbolic links.
 * So just fake an entry by writing the name of oldpath to the newpath
 * file name in the .link suffixed filename and just make a copy of the current
 */
int symlink(const char *oldpath, const char *newpath)
{
    FILE *fptr;
    char linkbuf[256];
    int status = 0;
    if(!oldpath || !newpath) return -1;
    /*
     * check if newpath is already present.
     */
    status = link(oldpath, newpath);
    if(status < 0)
    {
        int readFd = 0;
        int writeFd = 0;
        char buf[1024];
        int nbytes;
        if(errno != ENOSYS)
        {
            printf("Unable to create a link [%s] to file [%s]. Error [%s]\n", newpath, oldpath, 
                   strerror(errno));
            return -1;
        }
        /*
         * Make a copy of the file.
         */
        readFd = open(oldpath, O_RDONLY);
        if(readFd < 0)
        {
            printf("Unable to open source file [%s] to link\n", oldpath);
            return -1;
        }
        writeFd = open(newpath, O_CREAT | O_TRUNC | O_RDWR, 0777);
        if(writeFd < 0)
        {
            printf("Unable to open link copy file [%s] for writing\n", newpath);
            return -1;
        }
        while( (nbytes = read(readFd, buf, sizeof(buf)) ) > 0 )
        {
            if(write(writeFd, buf, nbytes) != nbytes )
            {
                printf("Unable to copy to link file [%s]\n", newpath);
                close(writeFd);
                close(readFd);
                return -1;
            }
        }
        close(readFd);
        fsync(writeFd);
        close(writeFd);
    }
    snprintf(linkbuf, sizeof(linkbuf), "%s.link", oldpath);

    fptr = fopen(linkbuf, "w");
    if(!fptr)
    {
        /* 
         * remove symlink
         */
        unlink(newpath);
        return -1;
    }
    fputs(oldpath, fptr);
    fclose(fptr);
    return 0;
}

int readlink(const char *path, char *buf, int len)
{
    FILE *fptr ;
    int bytes = 0;
    char linkbuf[256];
    if(!path || !buf || !len) return -1;
    snprintf(linkbuf, sizeof(linkbuf), "%s.link", path);
    fptr = fopen(linkbuf, "r");
    if(!fptr)
        return -1;
    if(!fgets(buf, len, fptr))
    {
        fclose(fptr);
        return -1;
    }
    fclose(fptr);
    bytes = strlen(buf);
    if(!bytes)
        return -1;
    if(buf[bytes-1] == '\n')
        buf[bytes-1] = 0;
    if(bytes > 1 && buf[bytes-2] == '\r')
        buf[bytes-2] = 0;
    return 0;
}

int getuid(void) 
{
    return 0;
}
int geteuid(void)
{
    return 0;
}
int getgid(void)
{
    return 0;
}
int getegid(void)
{
    return 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while(*s1 && *s2 && n)
    {
        int cmp = toupper(*s1) - toupper(*s2);
        if(cmp) return cmp > 0 ? 1 : -1;
        ++s1,++s2;
        --n;
    }
    if(!n) return 0;
    if(*s1) return 1;
    if(*s2) return -1;
    return 0;
}

int strcasecmp(const char *s1, const char *s2)
{
    return strncasecmp(s1, s2, (size_t)-1);
}

int gettimeofday(struct timeval *pTV, void *tz)
{
    struct timespec tv = {0};
    if(!pTV) return -1;
    if(clock_gettime(CLOCK_REALTIME, &tv) < 0 )
    {
        clLogError("TIME","GET","gettimeofday returned with [%s]\n", strerror(errno));
        return -1;
    }
    pTV->tv_sec = tv.tv_sec;
    pTV->tv_usec = tv.tv_nsec/1000;
    return 0;
}

/*
 * wrap a poll function over a select.
 */
int poll(struct pollfd *fds, unsigned int nfds, int timeout)
{
    int ret = -1;
    int i;
    fd_set in_fdset;
    fd_set out_fdset;
    fd_set *p_in_fdset = NULL;
    fd_set *p_out_fdset = NULL;
    int maxfd = 0;
    int navail = 0;
    struct timeval tv = {0};
    struct timeval *pTV = NULL;
    FD_ZERO(&in_fdset);
    FD_ZERO(&out_fdset);
    if(timeout >= 0)
    {
        pTV = &tv;
        pTV->tv_sec = timeout/1000;
        pTV->tv_usec = timeout%1000;
        pTV->tv_usec*=1000;
    }
        
    for(i = 0; i < nfds; ++i)
    {
        fds[i].revents = 0;
        if(fds[i].events & POLLIN)
        {
            FD_SET(fds[i].fd, &in_fdset);
            if(!p_in_fdset)
                p_in_fdset = &in_fdset;
        }
        if(fds[i].events & POLLOUT)
        {
            FD_SET(fds[i].fd, &out_fdset);
            if(!p_out_fdset)
                p_out_fdset = &out_fdset;
        }
        if(fds[i].fd > maxfd)
            maxfd = fds[i].fd;
    }
    
    retry:
    ret = select(maxfd + 3, p_in_fdset, p_out_fdset, NULL, pTV);
    if(ret < 0 )
    {
        if(errno == EINTR)
            goto retry;
        goto out;
    }
    if(ret == 0)
    {
        goto out;
    }
    for(i = 0; i < nfds; ++i)
    {
        int on = 0;
        if(p_in_fdset && FD_ISSET(fds[i].fd, &in_fdset))
        {
            on = 1;
            fds[i].revents |= POLLIN | POLLRDNORM;
        }
        if(p_out_fdset && FD_ISSET(fds[i].fd, p_out_fdset))
        {
            on = 1;
            fds[i].revents |= POLLOUT;
        }
        if(on) ++navail;
        else fds[i].revents = 0;
            
    }
    ret = navail;
    out:
    return ret;
    
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *mattr, int pshared)
{
    return -1;
}
int pthread_condattr_setpshared(pthread_condattr_t *cattr, int pshared)
{
    return -1;
}

STATUS taskLock(void)
{
    return taskRtpLock();
}

STATUS taskUnlock(void)
{
    return taskRtpUnlock();
}

void openlog(const char *id, int opt, int fac)
{
    gClSyslogFacility = fac;
    applLoggerInit(fac, 1); /*initialize on the stdout*/
}

void closelog(void)
{
    applLoggerStop(gClSyslogFacility);
}

void syslog(int priority, const char *fmt, ...)
{
    char msg[256];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(msg, sizeof(msg), fmt, arg);
    va_end(arg);
    _applLog(priority, __FILE__, __LINE__, msg);
}

int umask(int mode)
{
    return 0;
}

int random(void)
{
    return rand();
}

ClInt32T clCreatePipe(ClInt32T fds[2], ClUint32T numMsgs, ClUint32T msgSize)
{
    static ClInt32T numPipes;
    char pipeName[CL_MAX_NAME_LENGTH];
    static char compName[CL_MAX_NAME_LENGTH]; 
    int fd;
    if(!numMsgs)
        numMsgs = CL_VXWORKS_PIPE_MSGS;
    if(!msgSize)
        msgSize = CL_VXWORKS_PIPE_MSGSIZE;
    if(!compName[0])
    {
        char *str = getenv("ASP_COMPNAME");
        if(!str)
        {
            str = "nodeRep";
        }
        strncpy(compName, str, sizeof(compName)-1);
    }
    pthread_mutex_lock(&pipeLock);
    snprintf(pipeName, sizeof(pipeName)-1, "/%s%d", compName, numPipes);
    fd = pipeDevCreate(pipeName, numMsgs, msgSize);
    if(fd < 0)
    {
        pthread_mutex_unlock(&pipeLock);
        printf("pipe dev create for device [%s] returned [%s]\n", pipeName, strerror(errno));
        return fd;
    }
    fd = open(pipeName, O_RDWR, 0666);
    if(fd < 0 )
    {
        pthread_mutex_unlock(&pipeLock);
        printf("pipe dev open [%s] returned [%s]\n", pipeName, strerror(errno));
        return fd;
    }
    ++numPipes;
    pthread_mutex_unlock(&pipeLock);
    fds[0] = fd;
    fds[1] = fd;
    return 0;
}

int pipe(int fds[2])
{
    return clCreatePipe(fds, CL_VXWORKS_PIPE_MSGS, CL_VXWORKS_PIPE_MSGSIZE);
}

int access(const char *path, int mode)
{
    struct stat statbuf;
    if(!path) return -1;
    if(stat(path, &statbuf))
        return -1;
    if( (mode & X_OK) )
    {
        if(!(statbuf.st_mode & S_IXUSR))
            return -1;
    }
    return 0;
}

int alphasort(const void *a, const void *b)
{
    struct dirent *d1 = *(struct dirent **)a;
    struct dirent *d2 = *(struct dirent **)b;
    return strcoll(d1->d_name, d2->d_name);
}

int scandir(const char *path, struct dirent ***dlist, int (*filter)(struct dirent *d),
            int (*cmp)(const void *, const void *))
{
    int ret = 0;
    DIR *dir = NULL;
    struct dirent *ent = NULL;
    struct dirent *res =  NULL;
    int nentries = 0;
    struct dirent **dentries = NULL;
    if(!path || !dlist || !cmp) return -1;
    dir = opendir(path);
    if(!dir)
        return -1;
    ent = malloc(sizeof(*ent));
    assert(ent != NULL);
    while( !(ret = readdir_r(dir, ent, &res)) )
    {
        if(!res) break;
        if(!strcmp(ent->d_name, ".") 
           ||
           !strcmp(ent->d_name, ".."))
            continue;
        if(!filter || filter(ent) == 1)
        {
            dentries = realloc(dentries, sizeof(*dentries) * (nentries + 1));
            assert(dentries != NULL);
            dentries[nentries] = malloc(sizeof(*ent));
            assert(dentries[nentries] != NULL);
            memcpy(dentries[nentries++], ent, sizeof(*ent));
        }
    }
    if(ret) return -1;
    if(!nentries) return 0;
    *dlist = dentries;
    qsort(dentries, nentries, sizeof(*dentries), cmp);
    return nentries;
}

int getpagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

int backtrace(void **arr, int size)
{
    return 0;
}

int madvise(void *addr, int size, int advice)
{
    return 0;
}

#else

ClInt32T clCreatePipe(ClInt32T fds[2], ClUint32T numMsgs, ClUint32T msgSize)
{
    return pipe(fds);
}

#endif
